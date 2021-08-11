/* マクロ定義 */
#define POW2(n)  (1<<(n))            // 2のn乗
#define MOD(x,n) ((x)&((1<<(n))-1))  // x を 2のn乗 で割った余り
#define ABS32(x) (((x)^((x)>>31)) - ((x)>>31))  // 符号付き32bit整数の絶対値
#define F_CLK 16000000   // Arduinoのクロック周波数[Hz]
#define F_INTR_BIT 14    // 割り込み周波数は2の何乗Hzか
#define FS_OFFSET 20     // fsの固定小数点は20ビット目(上位12ビットが整数部、下位20ビットが小数部)
#define FC_OFFSET 20     // fcの固定小数点は20ビット目(上位12ビットが整数部、下位20ビットが小数部)

/* 搬送波のインクルード(別ファイルに記載) */
#include "waveform.h"

/* VVVF計算用グローバル変数 */
volatile uint8_t out_U = 0, out_V = 0, out_W = 0;  // 各相出力のバッファ

volatile uint16_t theta = 0;      // U相信号波位相(0-65535で1周)
volatile uint16_t theta_prev = 0; // 直前のtheta
volatile int32_t fs = 0;          // 信号波周波数(FS_OFFSETビット目に小数点がある符号有固定小数点表記で、-2048～2047)
volatile uint16_t Vs = 0;         // 電圧(0-65535)

volatile uint32_t fc = 0;            // [非同期]キャリア周波数(FC_OFFSETビット目に小数点がある符号無固定小数点表記で、0～4095)
volatile uint16_t theta_c = 0;       // [非同期]キャリア位相(0-65535で1周)
volatile uint16_t theta_c_prev = 0;  // [非同期]直前のtheta_c

volatile uint32_t isr_cnt = 0;    // 割込みが何回実行されたか
volatile uint32_t startTime, endTime;  // 処理時間計測用

/* 運転指令用グローバル変数 */
volatile uint8_t kind = 'd';    // アシストモードは'a', デモモードは'd'
volatile uint8_t notch = 'N';   // ノッチ。'P','N','B'のどれか
volatile uint8_t ctrlFlag = 0;  // 0だとOFF、1だとON 
volatile int32_t acc_ref = 0;   // 加速度指令(fsが1secあたりいくつ増えるか)
volatile int32_t fs_car = 0;    // 車輪の回転速度をモータ周波数に換算した値
/* 運転指令のアルゴリズム 
 * kind=='a' (アシストモード) のとき、
 *  1. 車輪回転速度fs_carを取得(反射センサで読む)
 *  2. notchを判定(ペダル動いてれば'P',ブレーキレバー握ってれば'B',何もなければ'N')
 *     さしあたり、GPIOで入力する
 *  3. notchから加速度指令acc_refを判定
 *  4. fs_carおよびacc_refから、モータ周波数fsを計算(すべり周波数制御)
 * kind=='d' (デモモード)のとき、
 *  1. notchを判定(上と同様)
 *  2. notchから加速度指令acc_refを判定
 *  3. fsにacc_refを加え、モータ周波数fsを計算する
 */


void setup() {
  Serial.begin(115200);
  /* GPIO設定 */
  pinMode(2, OUTPUT);  // U相
  pinMode(7, OUTPUT);  // V相
  pinMode(9, OUTPUT);  // W相
  pinMode(11, INPUT_PULLUP);
  pinMode(12, INPUT_PULLUP);

  /* タイマーTCB0の割り込み設定 */
  TCA0_SINGLE_CTRLA = 0;            // TCA0を無効->delayが使えない:かわりにwaiting()を使ってね
  TCB0_CTRLA = 0b00000001;          // 分周比1,動作を許可
  TCB0_CTRLB = 0b00000000;          // CNTMODEを周期的割り込み動作
  TCB0_INTCTRL = 1;                 // 割込みを許可
  TCB0_CCMP = F_CLK >> F_INTR_BIT;  // TOP値
}

void loop() {
  // put your main code here, to run repeatedly:
  //Serial.println(fs);
  //waiting(500);
  
}

/* 割り込み時に実行される関数 */
ISR(TCB0_INT_vect) {
  /* 割り込み要求を解除 */
  TCB0_INTFLAGS = 1;
  isr_cnt++;

  /* U相:PA0(D2), V相:PA1(D7), W相:PB0(D9) へ出力 */
  PORTA_OUT = out_U | (out_V<<1);
  PORTB_OUT = out_W;

  startTime = TCB0_CNT;  // 処理開始時点のタイマカウント値を取得

  /* ノッチ判定 PE0(D11)==0のとき力行、PE1(D12)==0のとき制動)*/
  if (((PORTE_IN>>0) & 1) == LOW) {
    notch = 'B';
  } else if (((PORTE_IN>>1) & 1) == LOW) {
    notch = 'P';
  } else {
    notch = 'N';
  }

  /* 加速度指令計算 */
  switch (notch) {
    case 'P':
      acc_ref = (2L<<FS_OFFSET);  // 2Hz/sec で加速
      break;
    case 'N':
      acc_ref = 0;
      break;
    case 'B':
      acc_ref = (-2L<<FS_OFFSET);  // 2Hz/sec で減速
      break;
  }

  /* 周波数計算 */
  fs += (acc_ref >> F_INTR_BIT);
  if (fs > (70L<<FS_OFFSET)) { 
    fs = 70L<<FS_OFFSET;  // fsは最大70Hzとする
  }
  if (fs < 0L) {
    fs = 0L;  // fsは最小0Hzとする
  }
  
  /* 電圧計算 */
  if (fs>>FS_OFFSET < 60) {
    Vs = 4095 + ABS32(fs)>>10;  // 60Hzまで線形に上昇
  } else {
    Vs = 65535;  // 60Hz以上は最大値
  }
  
  /* 信号波位相を進める */
  theta_prev = theta;
  theta += fs >> (FS_OFFSET + F_INTR_BIT - 16);
  // ↑fsは実の値[Hz]より2^FS_OFFSET倍されているので、2^FS_OFFSETで割る
  //   dtは割り込み周波数2^F_INTR_BIT[Hz]の逆数なので、2^F_INTR_BITで割る
  //   位相は0から65535で1周するので、2^16を掛ける

  /* パルスモード判定 */
  uint8_t* wave;
  uint8_t wave_bitlen;
  if (fs > 54525952) {         // 52*2^20(この数値は適当)
    wave = wave_W3P;
    wave_bitlen = WAVE_BIT_W3P;
  } else if (fs > 39845888) {  // 38.0*2^20
    wave = wave_3P;
    wave_bitlen = WAVE_BIT_3P;
  } else if (fs > 29360128) {  // 28.0*2^20
    wave = wave_5P;
    wave_bitlen = WAVE_BIT_5P;
  } else if (fs > 26214400) {  // 25.0*2^20
    wave = wave_9P;
    wave_bitlen = WAVE_BIT_9P;
  } else if (fs > 14365491) {  // 13.7*2^20
    wave = wave_15P;
    wave_bitlen = WAVE_BIT_15P;
  } else if (fs > 7340032)  {  // 7.0*2^20
    wave = wave_27P;
    wave_bitlen = WAVE_BIT_27P;
  } else if (fs > 5662310)  {  // 5.4*2^20
    wave = wave_45P;
    wave_bitlen = WAVE_BIT_45P;
  } else {
    wave = NULL;
    fc = 200L<<FC_OFFSET;
  }

  /* 搬送波位相を進める */
  theta_c_prev = theta_c;
  theta_c += fc >> (FC_OFFSET + F_INTR_BIT - 16);
  
  /* ゲート波形計算 */
  if (wave) {  // 同期モード
    out_U = calc_gate_sync(theta      , wave, wave_bitlen);
    out_V = calc_gate_sync(theta-21845, wave, wave_bitlen);
    out_W = calc_gate_sync(theta-43690, wave, wave_bitlen);
  } else {     // 非同期モード
    out_U = calc_gate_async(theta);
    out_V = calc_gate_async(theta-21845);
    out_W = calc_gate_async(theta-43690);
  }

  endTime = TCB0_CNT;
}

uint8_t calc_gate_sync(uint16_t phase, uint8_t* wave, uint8_t wave_bitlen) {
  /* キャリア振幅を計算 */
  uint8_t shift = 15 - wave_bitlen;
  uint16_t theta_index = (phase & 32767) >> shift;  // pi~2piを0~piに移し、index範囲を合わせる
  uint16_t wave_val = (uint16_t)wave[theta_index] << 8;  //0-255なので、0-65535に換算
  
  /* 比較 */
  if (phase & 32768) {  // 最上位ビットが1、つまり位相がpi~2piのとき
    return (Vs < wave_val);
  } else {              // 最上位ビットが0、つまり位相が0~piのとき
    return (Vs >= wave_val);
  }
}

uint8_t calc_gate_async(uint16_t phase) {
  /* 搬送波振幅を計算 */
  uint16_t Ac;
  if (theta_c & 32768) {  // 三角波生成
    Ac = ~(theta_c << 1);
  } else {
    Ac = theta_c << 1;
  }
  
  /* 信号波振幅を計算 */
  uint8_t theta_index = phase >> 8;
  int8_t sin_val = wave_sin[theta_index];
  uint16_t As = 32768 + (int16_t)sin_val*(Vs>>8);

  /* 比較 */
  return (As > Ac);
}

void waiting(unsigned long millisec) {  //指定したミリ秒だけ待つ関数
  uint32_t beginCnt = isr_cnt;
  volatile uint32_t i;
  while (isr_cnt - beginCnt < (millisec<<F_INTR_BIT)/1000){
    yield();
  }
}
