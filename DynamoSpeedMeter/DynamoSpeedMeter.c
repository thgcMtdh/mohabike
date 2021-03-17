// GPIO入力から速度を計算し、PWM信号を生成するコードです
//  高速化のため、Cで処理する方針に変えました。python側から "./pwm.o shmid" と呼び出します
//  引数のshmidは、python側で確保したデータ共有用の共有メモリのshmidです
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/time.h>
#include <wiringPi.h>

#define PIN_DYNAMO 0  // GPIO17
#define NMAX 65536  // 記録するパルス数 (0.13465m/pulse)

int counter = 0;  // パルス入力で1ずつ増える
double t[NMAX];  // 立ち上がりパルスが入ってきた時刻を記録するメモリ
// t = (double*)calloc(NMAX, sizeof(double));

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

void countup() {
    t[counter] = gettime();
    counter++;
    if (counter >= NMAX) {  //  終端に達したら書き込んで終了
        for (int i=0; i<NMAX; i++) {
            // データがあるぶんを出力
            if (t[i] > 0) printf("%f\n", t[i]);
        }
        exit(0);
    }
}

int main() {
    /* t[]の初期化 */
    if (t == NULL) {
        printf("メモリ確保に失敗");
        return 1;
    }
    for (int i=0; i<NMAX; i++) t[i] = 0.0;
    
    /* GPIOの初期化*/
    if (wiringPiSetup() < 0) {
        perror("[pwm.c] wiringPiSetup");
        return 1;
    }
    
    /* 割り込みを設定 */
    pinMode(PIN_DYNAMO, INPUT);
    pullUpDnControl(PIN_DYNAMO, PUD_DOWN);
    wiringPiISR(PIN_DYNAMO, INT_EDGE_RISING, countup);
    
    /* パルス計測 */
    int buf;
    while (1) {
        //printf("1文字入力:現時点のcountを表示。c:カウントリセット\n");
        //buf = getchar();
        //if (buf == 'c') counter = 0;
        //printf("counter= %d\n", counter);
        buf = getchar();
        if (buf = 'q') break;
    }
    
    /* 出力 */
    //printf("%d\n",counter);
    for (int i=0; i<NMAX; i++) {
        // データがあるぶんを出力
        if (t[i] > 0) printf("%f\n", t[i]);
    }

    return 0;
}
