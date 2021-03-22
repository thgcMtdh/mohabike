// GPIO入力から速度を計算し、PWM信号を生成するコードです
//  高速化のため、Cで処理する方針に変えました。python側から "./pwm.o shmid" と呼び出します
//  引数のshmidは、python側で確保したデータ共有用の共有メモリのshmidです
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/time.h>
#include <bcm2835.h>

#define PIN_DYNAMO RPI_GPIO_P1_11  // GPIO17
#define PIN_U RPI_GPIO_P1_08  //GPIO14
#define PIN_V RPI_GPIO_P1_10  //GPIO15
#define PIN_W RPI_GPIO_P1_12  //GPIO18

struct timeval tv;
double gettime() {
    gettimeofday(&tv, NULL);
    return tv.tv_sec + tv.tv_usec/1000000.0;
}

int compare_double(const void* a, const void* b) {
    if (*(double*)a - *(double*)b > 0) return 1;  // a>bなら1を返す
    else if (*(double*)a - *(double*)b < 0) return -1;  // a<bなら-1を返す
    else return 0;
}

int main(int argc, char** argv) {
    /* 共有メモリの確保
       python側とデータを共有するために、共有メモリを用いています */
    if (argc <= 1) {
        fprintf(stderr, "[pwm.c] 確保した共有メモリのshmidを引数に入力すること");
        return 1;
    }
    int shmid = atoi(argv[1]);  // 共有メモリのshmid
    char* addr;

    if ((addr = shmat(shmid, 0, 0)) == (void*)-1) {  // 共有メモリをaddrにアタッチ
        perror("[pwm.c] shmat");
        return 1;
    }

    /* 共有する変数名を宣言し、共有メモリ領域を割り当てる */
    char *isdynamo = addr;  // char型のこれらはpython側で書き込む
    char *isvvvf = addr+1;
    char *notch = addr+2;
    char *run = addr+7;
    double *speed = (double*)(addr+8);       // C側で書き込む
    double *trainspeed = (double*)(addr+16);
    double *fs = (double*)(addr+24);
    double *fc = (double*)(addr+32);
    double *frand = (double*)(addr+40);
    double *Vs = (double*)(addr+48);
    int *pulsemode = (int*)(addr+64);
    *speed = 0.0;
    *trainspeed = 0.0;
    *fs = 0.0;
    *fc = 0.0;
    *frand = 0.0;
    *Vs = 0.0;
    *pulsemode = 0;

    /* GPIOの初期化と設定*/
    if (!bcm2835_init()) {
        perror("[pwm.c] bcm2835_init");
        return 1;
    }
    bcm2835_gpio_fsel(PIN_DYNAMO, BCM2835_GPIO_FSEL_INPT);
    bcm2835_gpio_fsel(PIN_U, BCM2835_GPIO_FSEL_OUTP);
    bcm2835_gpio_fsel(PIN_V, BCM2835_GPIO_FSEL_OUTP);
    bcm2835_gpio_fsel(PIN_W, BCM2835_GPIO_FSEL_OUTP);
    bcm2835_gpio_set_pud(PIN_DYNAMO, BCM2835_GPIO_PUD_DOWN);

    /* 速度の計算に使う変数 */
    const double TIMEOUT = 360.0;  // (デバッグ用)この秒数が経過後に処理を停止
    const double DYNAMORATE = 18.64;  // 自転車の時速1km/hあたり、ダイナモの周波数が何Hzか。
    const double MINSPEED = 0.5;
    const double ACC = 1.0;
    const double MAX_T = 1.0 / (MINSPEED*DYNAMORATE);
    const unsigned int dtSAMPLENUM = 19;  // 速度計算時に中央値をとるサンプル数(奇数にすること)
    double tmpspeed = 0.0;
    double newtime = 0.0;
    double oldtime = 0.0;
    double starttime = 0.0;
    double dt = 0.0;
    double* dts;
    double* dts_sorted;
    unsigned int dtcount = 0;
    uint8_t oldinput = LOW;
    uint8_t newinput = LOW;
    struct timeval tv;

    if ((dts = (double*)calloc(dtSAMPLENUM, sizeof(double))) == NULL){
        perror("[pwm.c] calloc dts");
        return 1;
    }
     if ((dts_sorted = (double*)calloc(dtSAMPLENUM, sizeof(double))) == NULL){
        perror("[pwm.c] calloc dts_sorted");
        return 1;
    }

    /* PWMの計算に使う変数 */
    const double VFRATE = 0.92;  // 電車の速度÷モータ周波数
    const double REGEN_LOST_SPEED = 6.0;  // 回生失効する実車速度
    double pwm_newtime = 0.0;
    double pwm_oldtime = 0.0;
    double pwm_starttime = 0.0;  // [★追加]PWMを開始した時刻
    double Phase_sin_u = 0.0;  // 信号波(正弦波)の位相 (0～1)
    double Phase_sin_v = 0.0;
    double Phase_sin_w = 0.0;
    double Phase_tri = 0.0;  // 搬送波(三角波)の位相 (0～1)
    double Vc = 0.0;  // 搬送波(三角波)の高さ (-1～1)
    double fdeviation = 0.0;  // ランダム変調の変調幅
    unsigned long count_for_debug = 0;
    
    starttime = gettime();  // 開始時刻を取得
    oldtime = starttime;  // 時刻変数を初期化
    newtime = starttime;
    pwm_oldtime = starttime;
    pwm_newtime = starttime;
    while (*run == '1' && gettime() - starttime < TIMEOUT){
        count_for_debug++;
        /* ---速度の計算--- */
           /* isdynamo=='1'のときはダイナモ入力を読み取る。'0'のときはデモモードで、*notchに応じて加減速 */
        if (*isdynamo == '1') {
            /* ダイナモ入力の立ち上がったタイミングで時刻を読み取り、前の立ち上がり時刻と比べることで、周期dtを計算
               これをもとに、自転車の速度*speedを計算する */
            oldinput = newinput;
            newinput = bcm2835_gpio_lev(PIN_DYNAMO);
            if (oldinput == LOW && newinput == HIGH) {  // 立ち上がりがあったら
                oldtime = newtime;
                newtime = gettime();
                dt = newtime - oldtime;
                if (dt > MAX_T) {
                    *speed = 0.0;
                }
                else {
                    dts[dtcount % dtSAMPLENUM] = dt;
                    dtcount += 1;
                    memcpy(dts_sorted, dts, dtSAMPLENUM * sizeof(double));
                    qsort(dts_sorted, dtSAMPLENUM, sizeof(double), compare_double);
                    dt = dts_sorted[dtSAMPLENUM/2 + 1];  // 中央値
                    if (dt > 0) *speed = 1.0 / dt / DYNAMORATE;  // 0わり防止
                }
            }
            if (gettime() - newtime > MAX_T) *speed = 0.0;  // 前回の立ち上がりから長時間経っていたら*speed=0
        }
        else {
            /* デモモードでは、加速度ACC * 経過時刻dtを現在の速度に加えていくことで、速度を計算
               ダイナモモードからデモモードに切り替わったとき(dtcount>0)、速度計算用の変数を初期化 */
            if (dtcount > 0) {
                dtcount = 0;
                for (size_t i = 0; i < sizeof(dts)/sizeof(double); i++) dts[i] = 0;
                newtime = gettime();
            }
            else {
                oldtime = newtime;
                newtime = gettime();
                dt = newtime - oldtime;
                if (*speed < MINSPEED && (*notch=='N' || *notch=='B')) {  // [★追加]速度ゼロでノッチがPへと変わったタイミング(=始動時)を記録しておく
                    pwm_starttime = newtime;
                }
                if (newtime - pwm_starttime < 3.0) {  // [★追加]始動から3秒間は、速度ゼロで位置決め
                    *speed = 0.0;
                } else if (*notch == 'P') {
                    *speed += ACC * dt;
                } else if (*notch == 'B') {
                    *speed -= ACC*dt;
                    if (*speed < 0.0) *speed = 0.0;
                }
            }
        }

        /* ---PWMの計算--- */
           /* isvvvf=='1'のとき、PWMを計算しGPIOに信号を出力する。'0'のときGPIOはすべてLOW */

        /* 自転車の速度→電車の速度へ換算 */
        *trainspeed = 3.0 * *speed;
        // if (*speed < 5.0) {
        //     *trainspeed = *speed;
        // } else if (*speed < 10.0) {
        //     *trainspeed = -5.0 + *speed * 2.0;
        // } else {
        //     *trainspeed = -25.0 + *speed * 4.0;
        // }
        pwm_oldtime = pwm_newtime;
        pwm_newtime = gettime();
        if (*isvvvf == '1' && (*notch == 'P' || (*notch != 'P' && *speed > MINSPEED))) {  // [★変更]Pノッチのときは速度によらずpwmを行う
            dt = pwm_newtime - pwm_oldtime;

            /* 信号波(モータに入力する正弦波)を計算 */
            *fs = *trainspeed / VFRATE;  // モータ周波数を計算
            Phase_sin_w += *fs * dt;  // 位相をdt秒ぶんだけ進ませる
            Phase_sin_v = Phase_sin_w + 1.0/3.0;  // V相はW相より1/3周期進んでいる
            Phase_sin_u = Phase_sin_w + 2.0/3.0;  // U相はW相より2/3周期進んでいる
            Phase_sin_u = Phase_sin_u - (int)Phase_sin_u;  // 整数部を引いて、0以上1未満にする
            Phase_sin_v = Phase_sin_v - (int)Phase_sin_v;
            Phase_sin_w = Phase_sin_w - (int)Phase_sin_w;
            // if (*fs > 44.3) {*Vs = 0.3;}        // 1パルスモードのとき、Vsは1.0
            // else if(*fs > 13.29) {*Vs = *fs/132.9;}  // それ以外の時、電圧は周波数に比例して上昇
            // else {*Vs = 0.1;}
            *Vs = *fs/180 + 0.1;

            /* パルスモード判定 */
            if (*fs > 44.3) {*pulsemode = 1;}
            else if (*fs > 38.0) {*pulsemode = 3;}
            //else if (*fs > 28.0) {*pulsemode = 5;}
            else if (*fs > 25.0) {*pulsemode = 9;}
            else if (*fs > 13.7) {*pulsemode = 15;}
            else if (*fs > 7.0)  {*pulsemode = 27;}
            else if (*fs > 5.4)  {*pulsemode = 45;}
            else {*pulsemode = 0;}

            /* 搬送波(三角波)を計算 */
            /*  非同期モード */
            if (*pulsemode == 0) {
                *fc = 200.0;  // 非同期キャリア周波数
                *frand = 0.0;  // ランダム変調幅
                Phase_tri += (*fc + fdeviation) * dt;  // dt秒ぶんだけ位相を進ませる
                if (Phase_tri > 1.0) {
                    fdeviation = *frand * ((double)rand()/RAND_MAX - 0.5);  // 1周期経過後に周波数をランダムに更新
                    Phase_tri = Phase_tri - (int)Phase_tri;
                }
            /*  広域3パルスモード */
            } else if (*pulsemode == 2) {
                return 1;
            /*  そのほかの同期モード */
            } else {
                *fc = *fs * *pulsemode;
                *frand = 0.0;
                if (*pulsemode == 1) {  // 1パルスモードのとき、キャリア周波数は信号波の3倍
                    Phase_tri = Phase_sin_u * 3;
                } else {  // ほかのパルスモードのとき、キャリア周波数は信号波のpulsemode倍
                    Phase_tri = Phase_sin_u * *pulsemode;
                }
                Phase_tri = Phase_tri - (int)Phase_tri;
            }
            /* 三角波の高さを計算 */
            if (Phase_tri < 0.25) {Vc = -4.0 * Phase_tri;}
            else if (Phase_tri < 0.75) {Vc = -2.0 + 4.0 * Phase_tri;}
            else {Vc = 4.0 - 4.0 * Phase_tri;}

            /* GPIOへ出力 */
            if (*Vs*sin(2*M_PI*Phase_sin_u) >= Vc) bcm2835_gpio_write(PIN_U, HIGH); else bcm2835_gpio_write(PIN_U, LOW);
            if (*Vs*sin(2*M_PI*Phase_sin_v) >= Vc) bcm2835_gpio_write(PIN_V, HIGH); else bcm2835_gpio_write(PIN_V, LOW);
            if (*Vs*sin(2*M_PI*Phase_sin_w) >= Vc) bcm2835_gpio_write(PIN_W, HIGH); else bcm2835_gpio_write(PIN_W, LOW);
        }
        else {
            *fs = 0.0;
            *fc = 0.0;
            *frand = 0.0;
            *Vs = 0.0;
            *pulsemode = 0;
            Phase_sin_w = 0.0;
            bcm2835_gpio_write(PIN_U, LOW);
            bcm2835_gpio_write(PIN_V, LOW);
            bcm2835_gpio_write(PIN_W, LOW);
        }
    }

    /* 終了時の処理 */
    free(dts);
    free(dts_sorted);
    *speed = 0.0;
    *trainspeed = 0.0;
    *fs = 0.0;
    *fc = 0.0;
    *frand = 0.0;
    *Vs = 0.0;
    *pulsemode = 0;
    bcm2835_gpio_write(PIN_U, LOW);
    bcm2835_gpio_write(PIN_V, LOW);
    bcm2835_gpio_write(PIN_W, LOW);

    printf("[pwm.c] PWM process successfully finished. count_for_debug=%d\n", count_for_debug);
    return 0;
}