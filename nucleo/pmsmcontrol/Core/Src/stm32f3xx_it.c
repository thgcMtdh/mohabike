/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    stm32f3xx_it.c
  * @brief   Interrupt Service Routines.
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2021 STMicroelectronics.
  * All rights reserved.</center></h2>
  *
  * This software component is licensed by ST under BSD 3-Clause license,
  * the "License"; You may not use this file except in compliance with the
  * License. You may obtain a copy of the License at:
  *                        opensource.org/licenses/BSD-3-Clause
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "stm32f3xx_it.h"
/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#define ARM_MATH_CM4
#include "stdlib.h"
#include "arm_math.h"
#include "arm_const_structs.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN TD */

/* USER CODE END TD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
/* USER CODE BEGIN PV */
const int16_t asintable[257] = {-32768,-30159,-29076,-28243,-27540,-26919,-26356,-25838,-25354,-24899,-24468,-24057,-23663,-23285,-22921,-22568,-22226,-21894,-21571,-21257,-20950,-20649,-20355,-20068,-19785,-19508,-19236,-18969,-18706,-18447,-18191,-17940,-17692,-17447,-17205,-16967,-16731,-16498,-16267,-16039,-15814,-15591,-15369,-15150,-14933,-14718,-14505,-14294,-14084,-13876,-13670,-13465,-13262,-13060,-12860,-12661,-12463,-12266,-12071,-11877,-11684,-11492,-11302,-11112,-10923,-10735,-10549,-10363,-10178,-9994,-9811,-9628,-9447,-9266,-9086,-8906,-8728,-8550,-8372,-8195,-8019,-7844,-7669,-7495,-7321,-7147,-6975,-6802,-6631,-6459,-6288,-6118,-5948,-5778,-5609,-5440,-5272,-5103,-4936,-4768,-4601,-4434,-4268,-4101,-3935,-3769,-3604,-3439,-3273,-3109,-2944,-2779,-2615,-2451,-2287,-2123,-1959,-1795,-1632,-1468,-1305,-1142,-979,-816,-653,-489,-326,-163,0,162,325,488,652,815,978,1141,1304,1467,1631,1794,1958,2122,2286,2450,2614,2778,2943,3108,3272,3438,3603,3768,3934,4100,4267,4433,4600,4767,4935,5102,5271,5439,5608,5777,5947,6117,6287,6458,6630,6801,6974,7146,7320,7494,7668,7843,8018,8194,8371,8549,8727,8905,9085,9265,9446,9627,9810,9993,10177,10362,10548,10734,10922,11111,11301,11491,11683,11876,12070,12265,12462,12660,12859,13059,13261,13464,13669,13875,14083,14293,14504,14717,14932,15149,15368,15590,15813,16038,16266,16497,16730,16966,17204,17446,17691,17939,18190,18446,18705,18968,19235,19507,19784,20067,20354,20648,20949,21256,21570,21893,22225,22567,22920,23284,23662,24056,24467,24898,25353,25837,26355,26918,27539,28242,29075,30158,32767};  // arc sine lookup table. input=[0 256] mapped to [-1 1], y=[-32768 32767] mapped to [-pi/2 pi/2)

int timCounter;  // for sync mode
uint32_t TIM2CCRvals[dtSAMPLENUM] = {429496729, 429496729, 429496729, 429496729, 429496729};  // to calculate average fs. smaller than LONGMAX/SAMPLENUM
int i_dt = 0;

volatile uint16_t adcDCV = 0;
volatile uint16_t adcIU = 0;
volatile uint16_t adcIW = 0;

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN PFP */
void async_pwm(void);
void sync_pwm(int);
float calc_phase_delta(float, float);
float arcsin_f32(float);
float triangle(uint16_t);
void get_pulsemodeNo(float, float, int*, int*);
void calc_fc(int, float, int*, float*, float*);
float newton_downslope(float, float, float, uint32_t);
float newton_upslope(float, float, float, uint32_t);
int compare_int(const void *a, const void *b);
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

/* External variables --------------------------------------------------------*/
extern ADC_HandleTypeDef hadc1;
extern TIM_HandleTypeDef htim2;
extern DMA_HandleTypeDef hdma_usart2_rx;
/* USER CODE BEGIN EV */

/* USER CODE END EV */

/******************************************************************************/
/*           Cortex-M4 Processor Interruption and Exception Handlers          */
/******************************************************************************/
/**
  * @brief This function handles Non maskable interrupt.
  */
void NMI_Handler(void)
{
  /* USER CODE BEGIN NonMaskableInt_IRQn 0 */

  /* USER CODE END NonMaskableInt_IRQn 0 */
  /* USER CODE BEGIN NonMaskableInt_IRQn 1 */
  while (1)
  {
  }
  /* USER CODE END NonMaskableInt_IRQn 1 */
}

/**
  * @brief This function handles Hard fault interrupt.
  */
void HardFault_Handler(void)
{
  /* USER CODE BEGIN HardFault_IRQn 0 */

  /* USER CODE END HardFault_IRQn 0 */
  while (1)
  {
    /* USER CODE BEGIN W1_HardFault_IRQn 0 */
    /* USER CODE END W1_HardFault_IRQn 0 */
  }
}

/**
  * @brief This function handles Memory management fault.
  */
void MemManage_Handler(void)
{
  /* USER CODE BEGIN MemoryManagement_IRQn 0 */

  /* USER CODE END MemoryManagement_IRQn 0 */
  while (1)
  {
    /* USER CODE BEGIN W1_MemoryManagement_IRQn 0 */
    /* USER CODE END W1_MemoryManagement_IRQn 0 */
  }
}

/**
  * @brief This function handles Pre-fetch fault, memory access fault.
  */
void BusFault_Handler(void)
{
  /* USER CODE BEGIN BusFault_IRQn 0 */

  /* USER CODE END BusFault_IRQn 0 */
  while (1)
  {
    /* USER CODE BEGIN W1_BusFault_IRQn 0 */
    /* USER CODE END W1_BusFault_IRQn 0 */
  }
}

/**
  * @brief This function handles Undefined instruction or illegal state.
  */
void UsageFault_Handler(void)
{
  /* USER CODE BEGIN UsageFault_IRQn 0 */

  /* USER CODE END UsageFault_IRQn 0 */
  while (1)
  {
    /* USER CODE BEGIN W1_UsageFault_IRQn 0 */
    /* USER CODE END W1_UsageFault_IRQn 0 */
  }
}

/**
  * @brief This function handles System service call via SWI instruction.
  */
void SVC_Handler(void)
{
  /* USER CODE BEGIN SVCall_IRQn 0 */

  /* USER CODE END SVCall_IRQn 0 */
  /* USER CODE BEGIN SVCall_IRQn 1 */

  /* USER CODE END SVCall_IRQn 1 */
}

/**
  * @brief This function handles Debug monitor.
  */
void DebugMon_Handler(void)
{
  /* USER CODE BEGIN DebugMonitor_IRQn 0 */

  /* USER CODE END DebugMonitor_IRQn 0 */
  /* USER CODE BEGIN DebugMonitor_IRQn 1 */

  /* USER CODE END DebugMonitor_IRQn 1 */
}

/**
  * @brief This function handles Pendable request for system service.
  */
void PendSV_Handler(void)
{
  /* USER CODE BEGIN PendSV_IRQn 0 */

  /* USER CODE END PendSV_IRQn 0 */
  /* USER CODE BEGIN PendSV_IRQn 1 */

  /* USER CODE END PendSV_IRQn 1 */
}

/**
  * @brief This function handles System tick timer.
  */
void SysTick_Handler(void)
{
  /* USER CODE BEGIN SysTick_IRQn 0 */

  /* USER CODE END SysTick_IRQn 0 */
  HAL_IncTick();
  /* USER CODE BEGIN SysTick_IRQn 1 */

  /* USER CODE END SysTick_IRQn 1 */
}

/******************************************************************************/
/* STM32F3xx Peripheral Interrupt Handlers                                    */
/* Add here the Interrupt Handlers for the used peripherals.                  */
/* For the available peripheral interrupt handler names,                      */
/* please refer to the startup file (startup_stm32f3xx.s).                    */
/******************************************************************************/

/**
  * @brief This function handles DMA1 channel6 global interrupt.
  */
void DMA1_Channel6_IRQHandler(void)
{
  /* USER CODE BEGIN DMA1_Channel6_IRQn 0 */

  /* USER CODE END DMA1_Channel6_IRQn 0 */
  HAL_DMA_IRQHandler(&hdma_usart2_rx);
  /* USER CODE BEGIN DMA1_Channel6_IRQn 1 */

  /* USER CODE END DMA1_Channel6_IRQn 1 */
}

/**
  * @brief This function handles ADC1 interrupt.
  */
void ADC1_IRQHandler(void)
{
  /* USER CODE BEGIN ADC1_IRQn 0 */

  /* USER CODE END ADC1_IRQn 0 */
  HAL_ADC_IRQHandler(&hadc1);
  /* USER CODE BEGIN ADC1_IRQn 1 */
  adcDCV = ADC1 -> JDR1;
  adcIU = ADC1 -> JDR2;
  adcIW = ADC1 -> JDR3;
  Vdc = 3.3 * adcDCV/4096 * 10.82/0.82;
  Iac = 3.3 * adcIU/4096;//(3.3 * adcIU/4096 - 2.5) * 20.0;

  // operate inverter
  if (invstate == INVON) {
	  TIM1->EGR &= ~TIM_EGR_BG_Msk;  // clear BRK

	  if (mode == DEMO) {  // when demo mode, set speed reference
		  omega_ref += acc*CtrlPrd;
		  if (omega_ref > (1-0.04)*890.0) omega_ref = (1-0.04)*890.0;
		  if (omega_ref < 0.0) omega_ref = 0.0;
	  } else {
		  omega_ref = 0.0;
	  }

	  // read hall sensor signal
	  uint8_t hall_u = HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_5);
	  uint8_t hall_v = HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_3);
	  uint8_t hall_w = HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_10);

	  // calc sector
	  switch ((hall_u<<2) + (hall_v<<1) + hall_w) {
		  case (1<<2) + (0<<1) + 1 : sector = 5; break;
		  case (1<<2) + (0<<1) + 0 : sector = 6; break;
		  case (1<<2) + (1<<1) + 0 : sector = 1; break;
		  case (0<<2) + (1<<1) + 0 : sector = 2; break;
		  case (0<<2) + (1<<1) + 1 : sector = 3; break;
		  case (0<<2) + (0<<1) + 1 : sector = 4; break;
	  }

	  // operate with hall sensor
	  if (ctrlmode == HALL) {
		  // get speed
		  if (hallstate == STOP) omega_est = 0.0;

		  // judge current HallState
		  if (mode == DEMO) {
			  switch (hallstate) {
			  case STOP:
				  if (omega_ref > 0) hallstate = SELFSTART;
				  break;
			  case SELFSTART:
			  case HALL1:
			  case HALLSTEADY:
				  if (TIM2->CNT > dtMAX) hallstate = STOP;  // if omega is too slow, regard rotor as stopped
				  break;
			  }
		  } else {
			  switch (hallstate) {
			  case HALL1:
			  case HALLSTEADY:
				  if (TIM2->CNT > dtMAX) hallstate = STOP;  // if omega is too slow, regard rotor as stopped
				  break;
			  }
		  }

		  // estimate rotor position
		  uint32_t theta_est0 = (sector - 1) *  715827882;

		  if (hallstate != HALLSTEADY) {  // add 30deg
			  theta_est = theta_est0 + 357913941;
		  } else {  // we can acquire T/6 = TIM2->CCR1, t = TIM2->CNT, so current phase is uint32max/6*t/T
			  theta_est = theta_est0 + (uint32_t)((float)TIM2->CNT/FCLK*fs * 4294967296);
		  }

		  // compute voltage command
		  if (mode == DEMO) {
			  // on demo mode, calculate voltage in order to meet speed command omega_ref given by notch.
			  // thus, demo mode is "speed control open loop"
			  if (hallstate == STOP || omega_ref < 0.1) {
				  Vd = 0.0;
				  Vq = 0.0;
			  } else {
				  Vd = 0.0;
				  Vq = (0.04+omega_ref/890.0)*0.33;
			  }
		  } else {
			  // on other mode, calculate voltage according to current rotor speed omega_est.
			  // thus, other mode is "torque control open loop"
			  if (acc > 0) {
				  Vd = 0.0;
				  Vq = (0.04+omega_est/890.0)*0.33;
			  } else {
				  Vd = 0.0;
				  Vq = 0.0;
			  }
		  }


	  // forced operation
	  } else if (ctrlmode == SPEAKER) {
		  theta_est += (uint32_t)(omega_ref * CtrlPrd * 4294967296/2/PI);  // forcedly decide omega and theta without any feedback
		  Vd = 0.0;
		  Vq = (0.04+omega_ref/890.0);
	  }

	  if (Vq>1.0) Vq = 1.0;
	  if (Vq<0.0) Vq = 0.0;
	  arm_sqrt_f32(Vd*Vd + Vq*Vq, &Vs);

	  fs = omega_est /2.0/PI;
	  speed = 3.6 * omega_est/GEAR*R_TIRE;

	  // compute U phase wave angle
	  int32_t phase_delta = (int32_t)(calc_phase_delta(Vd, Vq) / PI * 2147483648);
	  theta_u = theta_est + phase_delta;

	  // get target pulse mode
	  int pmNo_ref, pulsemode_ref;
	  if (mode == DEMO) {
		  get_pulsemodeNo(Vq, omega_ref/2/PI, &pmNo_ref, &pulsemode_ref);
	  } else {
		  get_pulsemodeNo(Vq, fs, &pmNo_ref, &pulsemode_ref);
	  }

	  int flagModeChanged = 0;
	  // change pulse mode at sine zero cross on the top of carrier
	  if (pmNo_ref != pmNo) {

		  // asynchronous -> asynchronous
		  if (pulsemode == 0 && pulsemode_ref == 0) {  // change immediately
			  pmNo = pmNo_ref;

		  // asynchronous -> synchronous
		  } else if (pulsemode == 0 && pulsemode_ref > 0) {
			  if (theta_u < 4294967296/360 && (TIM1->CR1 & TIM_CR1_DIR_Msk) == 0) {  // phase~0 && bottom(DIR==0)
				  pmNo = pmNo_ref;
				  timCounter = 0;  // initialize variables
				  flagModeChanged = 1;  // set flag which indicates pulse mode has changed
			  }

		  // synchronous -> synchronous
		  } else if (pulsemode > 0 && pulsemode_ref > 0) {
			  if (timCounter == 0) {  // at the first bottom of carrier
				  pmNo = pmNo_ref;
				  flagModeChanged = 1;  // set flag which indicates pulse mode has changed
			  }

		  // synchronous -> asynchronous
		  } else if (pulsemode > 0 && pulsemode_ref == 0) {
			  if (timCounter == 0) {  // at the first bottom of carrier
				  pmNo = pmNo_ref;
			  }
		  }
	  }

	  if (mode == DEMO) {
		  // on demo mode, calculate carrer frequency according to omega_ref (NOT omega_est) in order to generate fine sound
		  calc_fc(pmNo, omega_ref/2/PI, &pulsemode, &fc, &fc0);
	  } else {
		  calc_fc(pmNo, fs, &pulsemode, &fc, &fc0);
	  }


	  // PWM
	  async_pwm();
//	  if (pulsemode == 0) {
//		  async_pwm();
//	  } else {
//		  sync_pwm(flagModeChanged);
//	  }
  } else {  // INVOFF
	  TIM1->EGR |= TIM_EGR_BG_Msk;  // BRK Generation
  }

  /* USER CODE END ADC1_IRQn 1 */
}

/**
  * @brief This function handles TIM2 global interrupt.
  */
void TIM2_IRQHandler(void)
{
  /* USER CODE BEGIN TIM2_IRQn 0 */

  /* USER CODE END TIM2_IRQn 0 */
  HAL_TIM_IRQHandler(&htim2);
  /* USER CODE BEGIN TIM2_IRQn 1 */

  uint32_t capturedvalue = TIM2->CCR1;
  uint32_t capturedsum = 0;

  if (capturedvalue > 15000) {  // avoid zero division and chattering
	  TIM2CCRvals[i_dt] = capturedvalue;  // put new value to array
	  i_dt++;
	  if (i_dt > dtSAMPLENUM-1) i_dt = 0;
//	  // calculate average
//	  for (int i=0; i<dtSAMPLENUM; i++) {
//	  	  capturedsum += TIM2CCRvals[i];
//	  }
//	  omega_est = (float)FCLK/6/capturedsum*dtSAMPLENUM * 2*PI;

	  // calculate median
	  uint32_t valuesorted[dtSAMPLENUM];
	  memcpy(valuesorted, TIM2CCRvals, sizeof(uint32_t)*dtSAMPLENUM);
	  qsort(valuesorted, dtSAMPLENUM, sizeof(uint32_t), compare_int);
	  omega_est = (float)FCLK/6/valuesorted[dtSAMPLENUM/2+1] * 2*PI;


//	  omega_est = (float)FCLK/6/capturedvalue * 2*PI;

	  // update hallstate
	  if (mode == DEMO) {
		  switch(hallstate) {
		  case SELFSTART:
			  hallstate = HALL1; break;
		  case HALL1:
			  hallstate = HALLSTEADY; break;
		  }
	  } else {
		  switch(hallstate) {
		  case STOP:
			  hallstate = HALL1; break;
		  case HALL1:
			  hallstate = HALLSTEADY; break;
		  }
	  }
  }


  /* USER CODE END TIM2_IRQn 1 */
}

/* USER CODE BEGIN 1 */

/**
  * @brief This function handles asynchronous mode PWM.
  */
void async_pwm(void)
{

	// calculate Time and ARR
	float T_now = (float)PRSC*TIM1->ARR / FCLK;  // current carrier half cycle [s]
	float T_next = 0.5/fc;                       // next carrier half cycle [s]
	uint32_t ARR_next = (uint32_t)((float)FCLK/2/PRSC/fc);  // set ARR
	uint32_t theta_next = theta_est;// + (uint32_t)(fs * (T_now + T_next/2) * 4294967296);
	// in 6 pulse mode, fs is not precise. So commented out.
	TIM1->ARR = ARR_next;

	// calculate Vu Vv Vw
	float Vu, Vv, Vw;
	float Valpha, Vbeta;
	float sinVal = arm_sin_f32((float)theta_next/4294967296 * 2*PI);
	float cosVal = arm_cos_f32((float)theta_next/4294967296 * 2*PI);
	arm_inv_park_f32(Vd, Vq, &Valpha, &Vbeta, sinVal, cosVal);
	arm_inv_clarke_f32(Valpha, Vbeta, &Vu, &Vv);
	Vw = - Vu - Vv;

	// set compare register
	Vu = (Vu > 1.0)?   1.0 : Vu;
	Vu = (Vu < -1.0)? -1.0 : Vu;
	Vv = (Vv > 1.0)?   1.0 : Vv;
	Vv = (Vv < -1.0)? -1.0 : Vv;
	Vw = (Vw > 1.0)?   1.0 : Vw;
	Vw = (Vw < -1.0)? -1.0 : Vw;
	TIM1->CCR1 = (uint16_t)(((float)ARR_next+1.0)/2.0 * (1.0 + Vu));
	TIM1->CCR2 = (uint16_t)(((float)ARR_next+1.0)/2.0 * (1.0 + Vv));
	TIM1->CCR3 = (uint16_t)(((float)ARR_next+1.0)/2.0 * (1.0 + Vw));

	// update control period
	CtrlPrd = T_now;
}

void sync_pwm(int flagModeChanged)
{
	// calculate theta and ARR
	uint32_t ARR_now = TIM1->ARR;

	uint32_t theta_u_ref = 4294967296/4/pulsemode * (2*timCounter + 1);  // theoretical U phase angle at the next interrupt
	uint32_t theta_proceed = 4294967296/2/pulsemode;
	int32_t sync_theta_error = (int32_t)(theta_u - theta_u_ref);  // error between theoretical phase angle and actual phase angle
	theta_proceed -= sync_theta_error/4;  // compensate phase error by changing carrier ARR
	uint32_t ARR_next = (uint32_t)((float)theta_proceed/4294967296 * FCLK/PRSC / fs);
	TIM1->ARR = ARR_next;

	// calculate Vu Vv Vw
	float Vu, Vv, Vw;
	uint32_t delta_theta_to_nextIT = (uint32_t)(fs*ARR_now*PRSC/FCLK*4294967296);
	if ((TIM1->CR1 & TIM_CR1_DIR_Msk) == 0) {  // bottom of carrier (DIR==UP)
		Vu = newton_downslope(Vs, omega_est, fs * pulsemode, theta_u            + delta_theta_to_nextIT);
		Vv = newton_downslope(Vs, omega_est, fs * pulsemode, theta_u-1431655765 + delta_theta_to_nextIT);
		Vw = newton_downslope(Vs, omega_est, fs * pulsemode, theta_u-2863311531 + delta_theta_to_nextIT);
	} else {  // top of carrier (DIR==DOWN)
		Vu = newton_upslope(Vs, omega_est, fs * pulsemode, theta_u            + delta_theta_to_nextIT);
		Vv = newton_upslope(Vs, omega_est, fs * pulsemode, theta_u-1431655765 + delta_theta_to_nextIT);
		Vw = newton_upslope(Vs, omega_est, fs * pulsemode, theta_u-2863311531 + delta_theta_to_nextIT);
	}

	// set compare register
	Vu = (Vu > 1.0)?   1.0 : Vu;
	Vu = (Vu < -1.0)? -1.0 : Vu;
	Vv = (Vv > 1.0)?   1.0 : Vv;
	Vv = (Vv < -1.0)? -1.0 : Vv;
	Vw = (Vw > 1.0)?   1.0 : Vw;
	Vw = (Vw < -1.0)? -1.0 : Vw;
	TIM1->CCR1 = (uint16_t)(((float)ARR_next+1.0)/2.0 * (1.0 + Vu));
	TIM1->CCR2 = (uint16_t)(((float)ARR_next+1.0)/2.0 * (1.0 + Vv));
	TIM1->CCR3 = (uint16_t)(((float)ARR_next+1.0)/2.0 * (1.0 + Vw));

	// update control period
	CtrlPrd = (float)ARR_now*PRSC/FCLK;

	timCounter++;
	if (timCounter >= 2*pulsemode) timCounter = 0;
}

float calc_phase_delta(float Vdin, float Vqin)
{
	// calculate x = Vd/sqrt(Vd^2+Vq^2)
	float bunbo;
	arm_sqrt_f32(Vdin*Vdin + Vqin*Vqin, &bunbo);
	float x = Vdin / bunbo;  // x = Vd/sqrt(Vd^2+Vq^2) [-1.0 1.0]

	// calculate delta
	if (Vqin >= 0.0) {
		return PI - arcsin_f32(x);
	} else {
		return arcsin_f32(x);
	}
}

float arcsin_f32(float x)
{
	// value check
	if (x > 1.0 || x < -1.0) {
		Error_Handler();
	}

	float indexf = (x+1.0)/2.0*256.0;
	uint8_t index = (indexf > 255)? 255 : (uint8_t)indexf;
	float frac = indexf - (float)index;  // 0-1
	return PI/2.0 * ((1.0 - frac) * asintable[index] + frac * asintable[index+1]) / 32768.0;
}

float triangle(uint16_t phase)
{
	if (phase < 16384) {
		return - (float)phase / 16384;
	} else if (phase < 49152) {
		return -2.0 + (float)phase / 16384;
	} else {
		return 4.0 - (float)phase / 16384;
	}
}

void get_pulsemodeNo(float Vq_in, float fs_in, int* pmNo_ref_out, int* pulsemode_ref_out)
{
	if (Vq_in < 0) {  // if Vq < 0 (regeneration brake at very low speed), force asynchronous
		*pmNo_ref_out = 0;
		*pulsemode_ref_out = list_pulsemode[0];
	} else {
		int i=0;
		for (i=pulsenum-1; i>=0; i--) {  // search pulsemode index
			if (fs_in >= list_fs[i]) break;
		}
		*pmNo_ref_out = i;
		*pulsemode_ref_out = list_pulsemode[i];
	}
}

/* param:  pulse mode No.
 * retval: pulse mode, fc */
void calc_fc(int i, float fs_in, int* pulsemode_out, float* fc_out, float* fc0_out) {
	*pulsemode_out = list_pulsemode[i];
	float fs1 = list_fs[i];
	float fs2 = (i==pulsenum-1)? fs1+10 : list_fs[i+1];
	frand = 0.0;

	if (*pulsemode_out == 0) {
		frand = list_frand1[i] + (list_frand2[i] - list_frand1[i]) * (fs_in - fs1) / (fs2 - fs1);
		*fc0_out = list_fc1[i] + (list_fc2[i] - list_fc1[i]) * (fs_in - fs1) / (fs2 - fs1);
		*fc_out = *fc0_out + frand * ((float)rand()*2.0/RAND_MAX - 1.0);
	} else {
		*fc0_out = fs_in * *pulsemode_out;
		*fc_out = *fc0_out;
	}
}

float newton_downslope(float Vs_in, float omega_in, float fc_in, uint32_t theta_in)
{
	float tprev = 0.0;
	float yprev = 0.0;
	float t = 1.0/4.0/fc_in;
	float y = 5.0;
	float sinVal, cosVal;
	float theta_0 = (float)theta_in/4294967296 * 2*PI;
	while (y - yprev < -0.01 || y - yprev > 0.01) {
		tprev = t;
		yprev = y;
		sinVal = arm_sin_f32(omega_in*t + theta_0);
		cosVal = arm_cos_f32(omega_in*t + theta_0);
		t = tprev - (Vs_in*sinVal + 4*fc_in*t - 1)/(Vs_in*omega_in*cosVal + 4*fc_in);
		y = 1 - 4*fc_in*t;
	}
	return y;
}

float newton_upslope(float Vs_in, float omega_in, float fc_in, uint32_t theta_in)
{
	float tprev = 0.0;
	float yprev = 0.0;
	float t = 1.0/4.0/fc_in;
	float y = 5.0;
	float sinVal, cosVal;
	float theta_0 = (float)theta_in/4294967296 * 2*PI;
	while (y - yprev < -0.01 || y - yprev > 0.01) {
		tprev = t;
		yprev = y;
		sinVal = arm_sin_f32(omega_in*t + theta_0);
		cosVal = arm_cos_f32(omega_in*t + theta_0);
		t = tprev - (Vs_in*sinVal - 4*fc_in*t + 1)/(Vs_in*omega_in*cosVal - 4*fc_in);
		y = 4*fc_in*t - 1;
	}
	return y;
}

int compare_int(const void *a, const void *b) {
	return *(int*)a - *(int*)b;
}

/* USER CODE END 1 */
/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
