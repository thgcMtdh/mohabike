/*
 * hall.c
 *
 *  Created on: 2021/07/18
 *      Author: denjo
 */

/**
  ******************************************************************************
  * @file           : hall.c
  * @brief          : Rotor speed calculation using hall sensor.
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include <stdlib.h>
#include "stm32f3xx_hal.h"

/* Private includes ----------------------------------------------------------*/
#include "hall.h"
#include "constants.h"

/* Private typedef -----------------------------------------------------------*/
typedef enum {E_HALLSTATE_STOP, E_HALLSTATE_0, E_HALLSTATE_1, E_HALLSTATE_2} HallState;

/* Private define ------------------------------------------------------------*/
#define dtSAMPLENUM 10

/* Private macro -------------------------------------------------------------*/

/* Private variables ---------------------------------------------------------*/
static TIM_HandleTypeDef* phtim2;
static volatile uint32_t TIM2_CCR_vals[dtSAMPLENUM];
static volatile size_t i_dt = 0;
static volatile HallState hallstate = E_HALLSTATE_STOP;
static volatile float fs;  // Measured electric frequency [Hz]

/* Private function prototypes -----------------------------------------------*/
static int compare_int(const void *a, const void *b);

/* Private user code ---------------------------------------------------------*/

/**
  * @brief Initialize hall sensor
  * @param pointer to TIM_HandleTypeDef object
  * @retval None
  */
void Hall_init(TIM_HandleTypeDef* htim2ptr) {
	phtim2 = htim2ptr;
	HAL_TIM_IC_Start(phtim2, TIM_CHANNEL_1);
	HAL_TIM_Base_Stop_IT(phtim2);
	for (size_t i=0; i<dtSAMPLENUM; i++) TIM2_CCR_vals[i] = UINT32TMAX-1;
}

/**
  * @brief Switch on/off hall sensor
  * @param E_HALL_TOGGLE_ON:  Speed calculation by hall sensor is enabled.
  *        E_HALL_TOGGLE_OFF: Hall sensor is disabled. If Hall_getFs is called, zero is returned.
  * @retval None
  */
void Hall_toggle(E_HALL_TOGGLE toggle) {
	if (toggle == E_HALL_TOGGLE_ON) {
		HAL_TIM_Base_Start_IT(phtim2);
	} else {
		HAL_TIM_Base_Stop_IT(phtim2);
	}
}

/**
  * @brief Return measured fs
  * @param None
  * @retval Electric rotor frequency [Hz]
  */
float Hall_getFs(void) {
	if (phtim2->Instance->DIER & 0b1) {  // if interrupt is enable
		return fs;
	} else {
		return 0.0;
	}
}

/**
  * @brief Estimate current rotor phase
  * @param None
  * @retval rotor phase (uint32_t)
  */
uint32_t Hall_getTheta(void) {
	// read hall sensor signal
	uint8_t hall_u = HAL_GPIO_ReadPin(GPIO_HALL_U, PIN_HALL_U);
	uint8_t hall_v = HAL_GPIO_ReadPin(GPIO_HALL_V, PIN_HALL_V);
	uint8_t hall_w = HAL_GPIO_ReadPin(GPIO_HALL_W, PIN_HALL_W);

	// calc sector
	uint8_t sector;
	switch ((hall_u<<2) + (hall_v<<1) + hall_w) {
		case (1<<2) + (0<<1) + 1 : sector = 5; break;
		case (1<<2) + (0<<1) + 0 : sector = 6; break;
		case (1<<2) + (1<<1) + 0 : sector = 1; break;
		case (0<<2) + (1<<1) + 0 : sector = 2; break;
		case (0<<2) + (1<<1) + 1 : sector = 3; break;
		case (0<<2) + (0<<1) + 1 : sector = 4; break;
	}

	// estimate rotor position
	uint32_t theta_est0 = (sector - 1) *  UINT32TMAX_6;

	if (hallstate != E_HALLSTATE_2) {  // add 30deg
		return theta_est0 + UINT32TMAX_12;
	} else {  // we can acquire dt*FCLK = TIM2->CNT, so current phase is
		return theta_est0 + (uint32_t)((float)TIM2->CNT/FCLK*fs * UINT32TMAX);
	}

}

/**
  * @brief Calculate fs on hall sensor input capture interrupt.
  * 	   Call this function in "TIM2_IRQHandler(void)"
  * @param None
  * @retval None
  */
void Hall_IT_calcFs(void) {

	// avoid zero division and chattering
	if (TIM2->CCR1 > 15000) {

		// --- update hallstate ---
		switch (hallstate) {
		case E_HALLSTATE_0:
			hallstate = E_HALLSTATE_1;
			for (size_t i=0; i<dtSAMPLENUM; i++) TIM2_CCR_vals[i] = UINT32TMAX-1;  // initialize array
			break;
		case E_HALLSTATE_1:
			hallstate = E_HALLSTATE_2;
			break;
		default:
			break;
		}

		// --- put new captured value to array ---
		TIM2_CCR_vals[i_dt] = TIM2->CCR1;
		i_dt++;
		if (i_dt >= dtSAMPLENUM) i_dt = 0;

		// --- calculate median and update fs ---
		uint32_t sortedval[dtSAMPLENUM];
		for (size_t i=0; i<dtSAMPLENUM; i++) sortedval[i] = TIM2_CCR_vals[i];  // copy
		qsort(sortedval, dtSAMPLENUM, sizeof(uint32_t), compare_int);  // sort
		fs = (float)FCLK/6/sortedval[dtSAMPLENUM/2+1];
	}
}

static int compare_int(const void *a, const void *b) {
	return *(int*)a - *(int*)b;
}
