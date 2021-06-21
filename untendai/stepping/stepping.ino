// constants (pin configuration)
#define PIN_METER1_A 3
#define PIN_METER1_B 5
#define PIN_METER2_A 6
#define PIN_METER2_B 9
#define PIN_PWM_YOBI 10

// constants (system)
#define FCLK 16000000    // clock freq [Hz] (don't change)
#define PRSC 64          // TCA0 prescaler (don't change)
#define CTRLFREQ 1000     // control frequency [Hz]
#define PHASE   4294967296
#define PHASE_2 2147483648
#define PHASE_4 1073741824

// constants (meter parameter)
#define METER_GEAR 118.95  // gear ratio 23.79 * pole pair 5
#define METER_TAU 0.5      // time constant [s]
#define METER_WN (1.5*PI)  // meter 2nd-order intrinsic frequency [rad/s]
#define METER_ETA 0.6      // meter 2nd-order damping factor
#define METER1_MINPOS (1.0/6.0)
#define METER1_MAXPOS (5.0/6.0)
#define METER1_INITPOS (1.0/6.0)
#define METER1_MINVAL 0.0
#define METER1_MAXVAL 40.0
#define METER1_RANGE (METER1_MAXVAL - METER1_MINVAL)
#define METER2_MINPOS (1.0/6.0)
#define METER2_MAXPOS (5.0/6.0)
#define METER2_INITPOS (1.0/6.0)
#define METER2_MINVAL 0.0
#define METER2_MAXVAL 60.0
#define METER2_RANGE (METER2_MAXVAL - METER2_MINVAL)

int8_t sinTable[256];

// calculated constants
const uint16_t tca0_per = FCLK/PRSC/CTRLFREQ;  // TCA0 timer period
const float dt = 1.0/(FCLK/PRSC/tca0_per);     // control period [s]

// global variables
volatile long duration = 0;
volatile unsigned long cntisr = 0;   // interrupt counter
float spd_data;  // later, data will be written on uint_8 array
float Vdc_data;
float* spd = &spd_data;
float* Vdc = &Vdc_data;
volatile struct Meter {
  uint32_t phase;    // electric rotor phase [0 to uint32tMax]
  float pos_ref[3];  // normalized position command [0 to 1] (0=-180deg, 1=180deg)
  float pos[3];      // normalized position [0 to 1]
  int pinA;
  int pinB;
  // *[0]=current value, [2]=oldest
} meter1, meter2;

// function prototype
void pwm_write(int, uint8_t);
void update_meterPosition(struct Meter*, float);

void setup() {
  for (int i=0; i<256; i++) {
    sinTable[i]=(int8_t)(128 + sin(2.0*PI*i/256)*127);
  }
  Serial.begin(115200);
  
  // pin configuration
  pinMode(PIN_METER1_A, OUTPUT);
  pinMode(PIN_METER1_B, OUTPUT);
  pinMode(PIN_METER2_A, OUTPUT);
  pinMode(PIN_METER2_B, OUTPUT);
  
  // setup interrupt & PWM generation
  PORTMUX_TCAROUTEA = 1;              // TCA0 to port B
  TCA0_SINGLE_CTRLA = 0b00001011;     // Prescaler=64(default), peripheral enable
  TCA0_SINGLE_CTRLB = 0b01110011;     // Compare 0-2 enable, wave gen mode SINGLESLOPE
  TCA0_SINGLE_INTCTRL = 0b00000001;   // Enable OVF interrupt
  TCA0_SINGLE_PER = tca0_per;         // Timer period (16bit)
  TCA0_SINGLE_CTRLD = 0;              // disable split mode
  pwm_write(PIN_METER1_A, 128);
  pwm_write(PIN_METER1_B, 128);
  pwm_write(PIN_METER2_A, 128);
  pwm_write(PIN_METER2_B, 128);
  
  // initialize meter object
  meter1.phase = 0;
  meter1.pos_ref[0] = METER1_INITPOS;
  meter1.pos_ref[1] = METER1_INITPOS;
  meter1.pos_ref[2] = METER1_INITPOS;
  meter1.pos[0] = METER1_INITPOS;
  meter1.pos[1] = METER1_INITPOS;
  meter1.pos[2] = METER1_INITPOS;
  meter1.pinA = PIN_METER1_A;
  meter1.pinB = PIN_METER1_B;
  meter2.phase = 0;
  meter2.pos_ref[0] = METER2_INITPOS;
  meter2.pos_ref[1] = METER2_INITPOS;
  meter2.pos_ref[2] = METER2_INITPOS;
  meter2.pos[0] = METER2_INITPOS;
  meter2.pos[1] = METER2_INITPOS;
  meter2.pos[2] = METER2_INITPOS;
  meter2.pinA = PIN_METER2_A;
  meter2.pinB = PIN_METER2_B;

  // initialize pointer value
  *spd = 0.0;
  *Vdc = 0.0;
}


void loop() {
  delay(2000);
  *spd = 20.0;
  delay(2000);
  *spd = 0.0;
  delay(100);
}

ISR(TCA0_OVF_vect) {
  TCA0_SINGLE_INTFLAGS = 1;  // clear interrupt
  cntisr++;

  // calculate normalized meter position according to current speed and Vdc
  float m1posref = METER1_MINPOS + (*spd - METER1_MINVAL)/METER1_RANGE * (METER1_MAXPOS - METER1_MINPOS);
  float m2posref = METER2_MINPOS + (*Vdc - METER2_MINVAL)/METER2_RANGE * (METER2_MAXPOS - METER2_MINPOS);
  
  // update position
  meter_updatePos(&meter1, m1posref);
  meter_updatePos(&meter2, m2posref);
}

void meter_updatePos(struct Meter* m, float pos_command) {
  // proceed one step
  m->pos_ref[2] = m->pos_ref[1];
  m->pos_ref[1] = m->pos_ref[0];
  m->pos_ref[0] = pos_command;
  m->pos[2] = m->pos[1];
  m->pos[1] = m->pos[0];

  // calculate new meter position [0 to 1]
  const float M = 4.0*METER_ETA / (METER_WN*dt);
  const float N = 4.0 / (METER_WN*METER_WN*dt*dt);
  const float a1 = 2.0*(1.0-N) / (1.0+M+N);
  const float a2 = (1.0-M+N) / (1.0+M+N);
  const float b0 = 1.0 / (1.0+M+N);
  const float b1 = 2.0 / (1.0+M+N);
  const float b2 = 1.0 / (1.0+M+N);
  m->pos[0] = -a1*m->pos[1] - a2*m->pos[2] + b0*m->pos_ref[0] + b1*m->pos_ref[1] + b2*m->pos_ref[2];
//  const float tau = METER_TAU;
//  m->pos[0] = (2.0*tau - dt)/(2.0*tau + dt)*m->pos[1] + dt/(2.0*tau + dt)*(m->pos_ref[0] + m->pos_ref[1]);

  // calculate current electric rotor phase
  m->phase += (int32_t)((m->pos[0] - m->pos[1])*METER_GEAR*PHASE);

  // generate sine wave and output
  uint8_t sinPhase = (m->phase) >> 24;
  uint8_t cosPhase = (m->phase + PHASE_4) >> 24;
  pwm_write(m->pinA, sinTable[cosPhase]);
  pwm_write(m->pinB, sinTable[sinPhase]);
}

void pwm_write(int pin, uint8_t val) {
  switch(pin) {
    case 3:
    case 6:
      analogWrite(pin, val);
      break;
    case 5:
      TCA0_SINGLE_CMP2BUF = (uint16_t)(((uint32_t)val * tca0_per) >> 8);  // val/256 * tca0_per
      break;
    case 9:
      TCA0_SINGLE_CMP0BUF = (uint16_t)(((uint32_t)val * tca0_per) >> 8);
      break;
    case 10:
      TCA0_SINGLE_CMP1BUF = (uint16_t)(((uint32_t)val * tca0_per) >> 8);
      break;
  }
}

/*
volatile unsigned int sequence = 0;  // 0,1,2,3
void step(){
  for (int i=0; i<50; i++) {
    sequence++;
    sequence &= 3;  // divide by 4
    switch (sequence) {
      case 0:
        digitalWrite(PIN1A, HIGH);
        digitalWrite(PIN1B, LOW);
        break;
      case 1:
        digitalWrite(PIN1A, HIGH);
        digitalWrite(PIN1B, HIGH);
        break;
      case 2:
        digitalWrite(PIN1A, LOW);
        digitalWrite(PIN1B, HIGH);
        break;
      case 3:
        digitalWrite(PIN1A, LOW);
        digitalWrite(PIN1B, LOW);
        break;
    }
    delay(5);
  }
}
*/
