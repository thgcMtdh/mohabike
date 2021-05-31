// constants
#define analogPin 0
#define AHPin 3
#define ALPin 9
#define BHPin 10
#define BLPin 11
#define PHASE_2 2147483648  // half cycle
#define PHASE_4 1073741824  // quarter cycle

// sin value (theta = 0 to 255, val = -127 to 127)
int8_t sinTable[] = {0,3,6,9,12,16,19,22,25,28,31,34,37,40,43,46,49,51,54,57,60,63,65,68,71,73,76,78,81,83,85,88,90,92,94,96,98,100,102,104,106,107,109,111,112,113,115,116,117,118,120,121,122,122,123,124,125,125,126,126,126,127,127,127,127,127,127,127,126,126,126,125,125,124,123,122,122,121,120,118,117,116,115,113,112,111,109,107,106,104,102,100,98,96,94,92,90,88,85,83,81,78,76,73,71,68,65,63,60,57,54,51,49,46,43,40,37,34,31,28,25,22,19,16,12,9,6,3,0,-3,-6,-9,-12,-16,-19,-22,-25,-28,-31,-34,-37,-40,-43,-46,-49,-51,-54,-57,-60,-63,-65,-68,-71,-73,-76,-78,-81,-83,-85,-88,-90,-92,-94,-96,-98,-100,-102,-104,-106,-107,-109,-111,-112,-113,-115,-116,-117,-118,-120,-121,-122,-122,-123,-124,-125,-125,-126,-126,-126,-127,-127,-127,-127,-127,-127,-127,-126,-126,-126,-125,-125,-124,-123,-122,-122,-121,-120,-118,-117,-116,-115,-113,-112,-111,-109,-107,-106,-104,-102,-100,-98,-96,-94,-92,-90,-88,-85,-83,-81,-78,-76,-73,-71,-68,-65,-63,-60,-57,-54,-51,-49,-46,-43,-40,-37,-34,-31,-28,-25,-22,-19,-16,-12,-9,-6,-3};

// varialbles
uint32_t nowtime=0, prevtime=0, phase=0, cnt=0;
int prevcommand=0, nowcommand=0;
float prevspdcommand=0.0, nowspdcommand=0.0;
float sumofspeed = 0.0;
 
void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  set_to_zero();
  nowtime=0;
  prevtime=0;
  phase=0;
  prevcommand=0;
  nowcommand=0;
  prevspdcommand=0;
  nowspdcommand=0;
}

void loop() {
  // put your main code here, to run repeatedly:

  // measure loop time
  prevtime = nowtime;
  nowtime = micros();
  uint32_t dt = nowtime - prevtime;

//  // to set speed
//  int command = analogRead(analogPin);
//  float freq = (float)((command - 512)/16); // div16=rshift4
//  set_speed(dt, freq);

  // to set position
  prevcommand = nowcommand;
  if (cnt == 0) {
    nowcommand = 0;  // 初回ループはdtが未確定なので、指令しない
  } else {
    nowcommand = analogRead(analogPin);//512+4*sinTable[((nowtime-1500000)>>15)%256];
  }
  prevspdcommand = nowspdcommand;
  nowspdcommand = set_position(dt);
  set_speed(dt, nowspdcommand);

  cnt += 1;
}

void set_speed(uint32_t dt, float freq) {  
  // calculate phase to accumulate
  //  theta[0to1] = f*t -> theta[0to2^32] = 2^32*freq[Hz]*t[us]/1e6
  int32_t phase_delta = (int32_t)(4295.0*freq*dt);
  
  phase += phase_delta;
  
  uint8_t sinPhase = (phase) >> 24;
  uint8_t cosPhase = (phase + PHASE_4) >> 24;

  // sine wave
  analogWrite(AHPin, sinTable[sinPhase] + 128);
  analogWrite(ALPin, sinTable[sinPhase+128] + 128);
  analogWrite(BHPin, sinTable[cosPhase] + 128);
  analogWrite(BLPin, sinTable[cosPhase+128] + 128);
}

void set_to_zero() {
  uint32_t begintime = micros();
  nowtime = begintime;
  while(nowtime - begintime < 1000000) {
    prevtime = nowtime;
    nowtime = micros();
    uint32_t dt = nowtime - prevtime;
    set_speed(dt, -60);
  }
  delay(500);
}

float set_position(uint32_t dtin) {
   const float tau = 0.3;  // time constant [s] (ステップ応答見るときは0.5以上でないと起動時に脱調する)
   const float gear = 23.79*5;
   const float theta_range = 2.0/3.0;
   const float command_max = 1024.0;
   float dt = (float)dtin / 1000000.0;
   float yk = (2.0*tau-dt)/(2.0*tau+dt)*prevspdcommand + 2.0/(2.0*tau+dt)*(nowcommand-prevcommand)/command_max*theta_range*gear;
   sumofspeed += yk*360*dt;
   return yk;
}
