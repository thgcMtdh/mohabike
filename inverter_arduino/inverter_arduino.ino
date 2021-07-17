/* マクロ定義 */
#define POW2(n) (1<<(n))          // 2のn乗
#define MOD(x,n) ((x)&((1<<(n))-1)) // x を 2のn乗 で割った余り
#define F_CLK 16000000   // Arduinoのクロック周波数[Hz]
#define F_INTR_BIT 14    // 割り込み周波数は2の何乗Hzか
#define FS_OFFSET 7      // fsの固定小数点は何ビット目か
#define FC_OFFSET 4      // fcの固定小数点は何ビット目か

/* 搬送波のインクルード(別ファイルに記載) */
#include "waveform.h"

/* グローバル変数 */
volatile uint32_t isr_cnt = 0;  // 割込みが何回実行されたか

volatile uint16_t theta = 0;      // U相信号波位相(0-65535で1周)
volatile uint16_t theta_prev = 0; // 直前のtheta
volatile int16_t fs = 0;         // 信号波周波数(FS_OFFSETビット目に小数点がある固定小数点表記)
volatile uint8_t Vs = 0;         // 電圧(0-255)

volatile uint16_t fc = 0;            // [非同期]キャリア周波数(FC_OFFSETビット目に小数点がある固定小数点表記)
volatile uint16_t theta_c = 0;       // [非同期]キャリア位相
volatile uint16_t theta_c_prev = 0;  // [非同期]直前のtheta_c

volatile uint16_t startTime, endTime;

void setup() {
  Serial.begin(115200);
  /* GPIO設定 */
  pinMode(2, OUTPUT);  // U相
  pinMode(7, OUTPUT);  // V相
  pinMode(18, OUTPUT); // W相

  /* タイマーTCB0の割り込み設定 */
  TCA0_SINGLE_CTRLA = 0;            // TCA0を無効->delayが使えないの:かわりにwaiting()を使ってね
  TCB0_CTRLA = 0b00000001;          // 分周比1,動作を許可
  TCB0_CTRLB = 0b00000000;          // CNTMODE:周期的割り込み動作
  TCB0_INTCTRL = 1;                 // 割込みを許可
  TCB0_CCMP = F_CLK >> F_INTR_BIT;  // TOP値
}

/* 割り込み時に実行される関数 */
ISR(TCB0_INT_vect) {
  /* 割り込み要求を解除 */
  TCB0_INTFLAGS = 1;
  isr_cnt++;

  startTime = TCB0_CNT;

  /* 周波数計算 */
  if (fs < (120<<FS_OFFSET)) {
    fs = isr_cnt>>5;
  } else {
    fs = 120<<FS_OFFSET;
  }
//  fs = 100<<FS_OFFSET;
  
  /* 電圧計算 */
  if (fs>>FS_OFFSET < 64) {
    Vs = fs>>5;  // 64Hzまで線形に上昇
  } else {
    Vs = 255;  // 64Hz以上は最大値
  }
  
  /* 信号波位相を進める */
  theta_prev = theta;
  theta += fs >> (FS_OFFSET + F_INTR_BIT - 16);

  /* パルスモード判定 */
  uint8_t* wave;
  uint8_t wave_bitlen;
  if (fs > 7372) {
    wave = wave_W3P;
    wave_bitlen = WAVE_BIT_W3P;
  } else if (fs > 4864) {  // 38.0*2^7
    wave = wave_3P;
    wave_bitlen = WAVE_BIT_3P;
  } else if (fs > 3584) {  // 28.0*2^7
    wave = wave_5P;
    wave_bitlen = WAVE_BIT_5P;
  } else if (fs > 3200) {  // 25.0*2^7
    wave = wave_9P;
    wave_bitlen = WAVE_BIT_9P;
  } else if (fs > 1754) {  // 13.7*2^7
    wave = wave_15P;
    wave_bitlen = WAVE_BIT_15P;
  } else if (fs > 896)  {  // 7.0*2^7
    wave = wave_27P;
    wave_bitlen = WAVE_BIT_27P;
  } else if (fs > 691)  {  // 5.4*2^7
    wave = wave_45P;
    wave_bitlen = WAVE_BIT_45P;
  } else {
    wave = NULL;
    fc = 200<<FC_OFFSET;
  }

  /* 搬送波位相を進める */
  theta_c_prev = theta_c;
  theta_c += fc >> (FC_OFFSET + F_INTR_BIT - 16);
  
  /* ゲート波形計算 */
  uint8_t out_U, out_V, out_W;
  if (wave) {  // 同期モード
    out_U = calc_gate_sync(theta      , wave, wave_bitlen);
    out_V = calc_gate_sync(theta-21845, wave, wave_bitlen);
    out_W = calc_gate_sync(theta-43690, wave, wave_bitlen);
  } else {     // 非同期モード
    out_U = calc_gate_async(theta);
    out_V = calc_gate_async(theta-21845);
    out_W = calc_gate_async(theta-43690);
  }
 
  // U相:PA0(D2), V相:PA1(D7), W相:PA2(D18)
  PORTA_OUT = out_U | (out_V<<1) | (out_W<<2);

  endTime = TCB0_CNT;
}

void loop() {
  // put your main code here, to run repeatedly:
  Serial.println(startTime);
  Serial.println(endTime);
  waiting(1000);
}


uint8_t calc_gate_sync(uint16_t phase, uint8_t* wave, uint8_t wave_bitlen) {
  /* キャリア振幅を計算 */
  uint8_t shift = 15 - wave_bitlen;
  uint16_t theta_index = (phase & 32767) >> shift;  // pi~2piを0~piに移し、index範囲を合わせる
  uint8_t x0 = wave[theta_index];
  uint8_t x1 = wave[MOD(theta_index+1,wave_bitlen)];  // x0の次(indexをはみださないようにmodをかけている
  uint16_t a1 = MOD(phase,shift);
  uint16_t a0 = POW2(shift) - a1;

  uint8_t wave_val = (a0*x0 + a1*x1) >> shift;  // x0とx1の直線補間
  
  /* 比較 */
  if (phase & 32768) {  // 最上位ビットが1、つまり位相がpi~2piのとき
    return (Vs < wave_val);
  } else {
    return (Vs >= wave_val);
  }
}

uint8_t calc_gate_async(uint16_t phase) {  
  /* 搬送波振幅を計算 */
  uint8_t Ac;
  if (theta_c & 32768) {
    Ac = ~(theta_c >> 7);
  } else {
    Ac = theta_c >> 7;
  }
  
  /* 信号波振幅を計算 */
  uint8_t theta_index = phase >> 8;
  int16_t sin_val = wave_sin[theta_index];
  uint8_t As = ( 32768 + sin_val*Vs ) >> 8;

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
