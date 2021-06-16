/*
 * control.c
 *
 *  Created on: Jun 6, 2021
 *      Author: denjo
 */

/**
  ******************************************************************************
  * @file           : control.c
  * @brief          : Motor control program
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/

/* Private includes ----------------------------------------------------------*/
#include "control.h"
#include "constants.h"
#include "pwm.h"

#define ARM_MATH_CM4
#include "arm_math.h"
#include "arm_const_structs.h"

/* Private typedef -----------------------------------------------------------*/
typedef enum {E_CTRL_SPEED_LOOP_OPEN, E_CTRL_SPEED_LOOP_CLOSED} E_CTRL_SPEED_LOOP;
typedef enum {E_CTRL_CURRENT_LOOP_OPEN, E_CTRL_CURRENT_LOOP_CLOSED} E_CTRL_CURRENT_LOOP;
typedef enum {E_CTRL_KIND_TRQ, E_CTRL_KIND_SPD} E_CTRL_KIND;

typedef enum {E_CTRL_POSEST_NONE, E_CTRL_POSEST_HALL, E_CTRL_POSEST_SENSORLESS} E_CTRL_POSEST;

/* Private define ------------------------------------------------------------*/

/* Private macro -------------------------------------------------------------*/

/* Private variables ---------------------------------------------------------*/
static E_CTRL_SPEED_LOOP speed_loop = E_CTRL_SPEED_LOOP_OPEN;
static E_CTRL_CURRENT_LOOP current_loop = E_CTRL_CURRENT_LOOP_OPEN;
static E_CTRL_KIND kind = E_CTRL_KIND_SPD;

static E_CTRL_POSEST posest = E_CTRL_POSEST_NONE;

static PulseMode* pm;    // pointer to pulse mode (pm is set in the main loop)
static CarParam* param;  // pointer to car param (param is set in the main loop)
static E_CTRL_TOGGLE ctrlFlag;  // control enable/disable flag

static float fc = PWM_DEFAULT_FC;
static float frand = 0.0;
static float CtrlPrd = 0.5/PWM_DEFAULT_FC;
static uint32_t theta = 0;

static float accref = 0.0;  // Electric angular acceleration command [rad/s/s] (set when speed control mode)
static float Tref = 0.0;    // Wheel-end Torque command [Nm] (set when Torque control mode / calculated when speed control (CL) mode)
static float wref = 0.0;    // Electric angular speed command [rad/s] (calculated when speed control mode)

static float Vdc = 12.0;  // DC bus voltage [V]
static float Vdref = 0.0;  // d-axis voltage [V]
static float Vqref = 0.0; // q-axis voltage [V]

/* Private function prototypes -----------------------------------------------*/
void Ctrl_dqToUVW(float, float);
void Ctrl_currentControl_Closed(float, float, float, float, float*, float*);
void Ctrl_currentControl_Open(float, float, float*, float*);
void Ctrl_speedControl_Closed(float, float, float*);
void Ctrl_speedControl_Open(float, float*, float*);
void Ctrl_calcCurrent(float, float*, float*);

/* Private user code ---------------------------------------------------------*/

void Ctrl_main (void) {
	if (ctrlFlag) {
		// controller
		switch (kind) {
		case E_CTRL_KIND_SPD:  // speed control mode
			wref += accref * CtrlPrd;  // calculate speed ref from acclereration command
			if (wref < 0.0) {
				wref = 0.0;
			}

			switch (speed_loop) {
			case E_CTRL_SPEED_LOOP_OPEN:
				Ctrl_speedControl_Open(wref, &Vdref, &Vqref);  // calculate voltage from speed command
				break;
			case E_CTRL_SPEED_LOOP_CLOSED:
				break;
			}
			break;

		case E_CTRL_KIND_TRQ:
			break;
		default:
			Pwm_toggle(E_PWM_TOGGLE_OFF);
			return;
		}

		// rotor position calculation
		switch (posest) {
		case E_CTRL_POSEST_NONE:
			theta += (uint32_t)(wref / TWOPI * CtrlPrd * UINT32TMAX);
			break;
		case E_CTRL_POSEST_HALL:
			break;
		case E_CTRL_POSEST_SENSORLESS:
			break;
		}

		// dq->uvw conversion
		float Valpha, Vbeta;
		float Vu, Vv, Vw;
		float sinVal = arm_sin_f32((float)theta/UINT32TMAX * 2*PI);
		float cosVal = arm_cos_f32((float)theta/UINT32TMAX * 2*PI);
		arm_inv_park_f32(Vdref, Vqref, &Valpha, &Vbeta, sinVal, cosVal);
		arm_inv_clarke_f32(Valpha, Vbeta, &Vu, &Vv);
		Vu *= SQRT_2_3;  // absolute transformation
		Vv *= SQRT_2_3;
		Vw = - Vu - Vv;

		Pwm_asyncpwm(fc, frand, Vu/Vdc, Vv/Vdc, Vw/Vdc);

	} else {
		Pwm_toggle(E_PWM_TOGGLE_OFF);
	}
}

// ======== Controller Function ========

/**
  * @brief OPEN LOOP SPEED CONTROLLER
  * @param wref: electric angular velocity command[rad/s]
  * @retval Vdref, Vqref [V]
  */
void Ctrl_speedControl_Open(float wref, float* pVdref, float* pVqref) {
	*pVdref = 0.0;
	*pVqref = 0.006742 * wref;
}

/**
  * @brief CLOSED LOOP SPEED CONTROLLER
  * @param wref: electric angular velocity command[rad/s],  west: measured velocity[rad/s]
  * @retval Tref: rotor torque command [Nm]
  */
void Ctrl_speedControl_Closed(float wref, float west, float* pTref) {

}

/**
  * @brief Calculate dq current from Torque command using MTPA method
  * @param Tref: Torque command[Nm]
  * @retval Idref,Iqref: current command[A]
  */
void Ctrl_calcCurrent(float Tref, float* pIdref, float* pIqref) {

}

/**
  * @brief OPEN LOOP CURRENT CONTROLLER
  * @param Idref,Iqref: current command[A]
  * @retval Vdref, Vqref[V]
  */
void Ctrl_currentControl_Open(float Idref, float Iqref, float* pVdref, float* pVqref) {

}

/**
  * @brief CLOSED LOOP CURRENT CONTROLLER
  * @param Idref,Iqref: current command[A], Id,Iq: measured current[A], loop: open-loop or closed-loop
  * @retval Vdref, Vqref[V]
  */
void Ctrl_currentControl_Closed(float Idref, float Iqref, float Id, float Iq, float* pVdref, float* pVqref) {

}

// ======== Rotor position estimation Function ========


// ======== Utility Function ========
/**
  * @brief Set VVVF parameters (call only when bike is stopping)
  * @param pPm: Pulse mode command.
  *        pParam: Car parameter.
  *        toggle: E_CTRL_TOGGLE_OFF: VVVF control algorithm is disabled. Inverter output is set to idle state.
  *                E_CTRL_TOGGLE_ON:  VVVF control algorithm is enabled.
  * @retval None
  */
void Ctrl_setParam(PulseMode* pPm, CarParam* pParam, E_CTRL_TOGGLE toggle) {
	pm = pPm;
	param = pParam;
	if (toggle == E_CTRL_TOGGLE_ON && pPm->carid > -1 && pParam->carid > -1) {
		ctrlFlag = E_CTRL_TOGGLE_ON;
		Pwm_toggle(E_PWM_TOGGLE_ON);
	} else {
		ctrlFlag = E_CTRL_TOGGLE_OFF;
		Pwm_toggle(E_PWM_TOGGLE_OFF);
	}
}

/**
 * @brief Initialize VVVF. Call when the main program body has started.
 * @param htim1ptr: pointer to TIM_HandleTypeDef object
 *        pPm: Pulse mode command.
 *        pParam: Car parameter.
 * @retval None
 */
void Ctrl_init(TIM_HandleTypeDef* htim1ptr, PulseMode* pPm, CarParam* pParam) {
	Pwm_init(htim1ptr);  // initalize PWM and get initial CtrlPrd
	Ctrl_setParam(pPm, pParam, E_CTRL_TOGGLE_OFF);
}
