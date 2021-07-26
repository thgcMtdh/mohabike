/*
 * pwm.c
 *
 *  Created on: 2021/06/06
 *      Author: denjo
 */

/**
  ******************************************************************************
  * @file           : pwm.c
  * @brief          : Generate PWM gate pulse according to pulse mode data
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include "stm32f3xx_hal.h"

/* Private includes ----------------------------------------------------------*/
#include "pwm.h"
#include "constants.h"

/* Private typedef -----------------------------------------------------------*/

/* Private define ------------------------------------------------------------*/

/* Private macro -------------------------------------------------------------*/

/* Private variables ---------------------------------------------------------*/
static volatile float fc = PWM_DEFAULT_FC; // carrer frequency [Hz]
static volatile float frand = 0.0;         // random modulation frequency [Hz]
static volatile size_t pmIdx = 0;          // current pulse mode index

/* Private function prototypes -----------------------------------------------*/
static float Pwm_Rand(void);

/* Private user code ---------------------------------------------------------*/

/**
  * @brief Start TIM1 PWM
  * @param pointer to TIM_HandleTypeDef object
  * @retval None
  */
void Pwm_init(TIM_HandleTypeDef* htim1ptr) {
	TIM1->PSC = PWM_DEFAULT_PSC - 1;
	TIM1->ARR = (uint32_t)((float)FCLK / 2 / PWM_DEFAULT_PSC / PWM_DEFAULT_FC);
	TIM1->CCR1 = TIM1->ARR / 2;
	TIM1->CCR2 = TIM1->ARR / 2;
	TIM1->CCR3 = TIM1->ARR / 2;
	HAL_TIM_PWM_Start(htim1ptr, TIM_CHANNEL_1);
	HAL_TIM_PWM_Start(htim1ptr, TIM_CHANNEL_2);
	HAL_TIM_PWM_Start(htim1ptr, TIM_CHANNEL_3);
	HAL_TIMEx_PWMN_Start(htim1ptr, TIM_CHANNEL_1);
	HAL_TIMEx_PWMN_Start(htim1ptr, TIM_CHANNEL_2);
	HAL_TIMEx_PWMN_Start(htim1ptr, TIM_CHANNEL_3);
	Pwm_toggle(E_PWM_TOGGLE_OFF);
}

/**
  * @brief Switch pwm output
  * @param toggle: E_PWM_TOGGLE_OFF: PWM output is set to idle state
  *                E_PWM_TOGGLE_ON:  PWM output is enable
  * @retval None
  */
void Pwm_toggle(E_PWM_TOGGLE toggle) {
	if (toggle == E_PWM_TOGGLE_OFF) {
		TIM1->BDTR |= TIM_BDTR_BKE_Msk;
		TIM1->CCR1 = TIM1->ARR / 2;
		TIM1->CCR2 = TIM1->ARR / 2;
		TIM1->CCR3 = TIM1->ARR / 2;
	} else {
		TIM1->CCR1 = TIM1->ARR / 2;
		TIM1->CCR2 = TIM1->ARR / 2;
		TIM1->CCR3 = TIM1->ARR / 2;
		TIM1->BDTR &= ~TIM_BDTR_BKE_Msk;
	}
}

/**
  * @brief PWM wave generator function called by controller
  * @param 3-phase voltages Vu,Vv,Vw [-1 to 1], signal freq[Hz], pointer to the PulseMode
  * @retval None
  */
void Pwm_IT_main(float Vu, float Vv, float Vw, float fs, PulseMode* pulseMode) {
	// --- select pulsemode ---

	// get target pulsemode
	size_t pmIdx_ref = pulseMode->N;
	while (pmIdx_ref > 0) {
		if (fs > pulseMode->fs[pmIdx_ref]) break;
		pmIdx_ref--;
	}

	// switch pulsemode at the appropriate timing
	if (pmIdx != pmIdx_ref){
		int pm_now = pulseMode->pm[pmIdx];
		int pm_ref = pulseMode->pm[pmIdx_ref];

		// async -> sync
		if (pm_now == 0 && pm_ref > 0) {
			pmIdx = pmIdx_ref;

		// sync -> sync
		} else if (pm_now > 0 && pm_ref >0) {
			pmIdx = pmIdx_ref;

		// sync -> async
		} else if (pm_now > 0 && pm_ref == 0) {
			pmIdx = pmIdx_ref;

		} else {
			pmIdx = pmIdx_ref;
		}
	}

	// calculate fc


	// --- asynchronus PWM---
	Pwm_asyncpwm(Vu, Vv, Vw);
}

/**
  * @brief Calculate asynchronus pwm carrier and control TIM1
  * @param fc(carrer freq), frand(random modulation freq), Vu,Vv,Vw(3phase voltages [-1 to 1])
  * @retval None
  */
void Pwm_asyncpwm(float Vu, float Vv, float Vw) {

	// calculate freq deviation of random modulation
	float fc_next = fc + frand * Pwm_Rand();

	// calculate ARR
	uint32_t PSC_next = TIM1->PSC;
	uint32_t ARR_next = (uint32_t)((float)FCLK/2/PSC_next/fc_next);  // set ARR
	TIM1->PSC = PSC_next;
	TIM1->ARR = ARR_next;

	// set compare register
	Vu = (Vu > 1.0)?   1.0 : Vu;
	Vu = (Vu < -1.0)? -1.0 : Vu;
	Vv = (Vv > 1.0)?   1.0 : Vv;
	Vv = (Vv < -1.0)? -1.0 : Vv;
	Vw = (Vw > 1.0)?   1.0 : Vw;
	Vw = (Vw < -1.0)? -1.0 : Vw;
	TIM1->CCR1 = (uint32_t)(((float)ARR_next+1.0)/2.0 * (1.0 + Vu));
	TIM1->CCR2 = (uint32_t)(((float)ARR_next+1.0)/2.0 * (1.0 + Vv));
	TIM1->CCR3 = (uint32_t)(((float)ARR_next+1.0)/2.0 * (1.0 + Vw));
}

/**
  * @brief Generate random float number
  * @param None
  * @retval Random float number (-1 to 1)
  * @reference http://kobayashi.hub.hit-u.ac.jp/topics/rand.html  Xorshift method
  */
float Pwm_Rand(void) {
	static uint32_t x = 123456789;
	static uint32_t y = 362436069;
	static uint32_t z = 521288629;
	static uint32_t w = 88675123;
	uint32_t t;
	t = x ^ (x<<11);
	x = y; y = z; z = w;
	w ^= t ^ (t>>8) ^ (w>>19);
	return 2.0 * (float)w / UINT32TMAX - 1.0;
}
