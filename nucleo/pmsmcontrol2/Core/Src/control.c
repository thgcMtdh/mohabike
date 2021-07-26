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
#include "hall.h"

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
static float Vdref = 0.0;   // d-axis voltage command [V]
static float Vqref = 0.0;   // q-axis voltage command [V]

static float fs = 0.0;  // actual electric frequency [Hz]
static float Iu = 0.0, Iv = 0.0, Iw = 0.0;
static float Id = 0.0;  // actual d-axis current [A]
static float Iq = 0.0;  // actual q-axis current [A]
static float Vdc = 12.0;  // DC bus voltage [V]

/* Private function prototypes -----------------------------------------------*/
void Ctrl_currentControl_Closed(float, float, float, float, float*, float*);
void Ctrl_currentControl_Open(float, float, float*, float*);
void Ctrl_speedControl_Closed(float, float, float*);
void Ctrl_speedControl_Open(float, float*, float*);
void Ctrl_calcCurrent(float, float*, float*);

/* Private user code ---------------------------------------------------------*/

void Ctrl_IT_main (void) {
	if (ctrlFlag) {
		// --- get current cycle CtrlPrd ---
		CtrlPrd = (float)TIM1->ARR*TIM1->PSC/FCLK;

		// --- generate torque or speed command ---

		// --- controller functions ---
		switch (kind) {
		case E_CTRL_KIND_SPD:  // speed control mode
			wref += accref * CtrlPrd;  // calculate speed ref from acclereration command
			if (wref < 0.0) wref = 0.0;

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

		// --- estimate rotor position ---
		switch (posest) {
		case E_CTRL_POSEST_NONE:
			theta += (uint32_t)(wref / TWOPI * CtrlPrd * UINT32TMAX);
			fs = wref/2.0/TWOPI;
			break;
		case E_CTRL_POSEST_HALL:
			theta = Hall_getTheta();
			fs = Hall_getFs();
			break;
		case E_CTRL_POSEST_SENSORLESS:
			break;
		}

		// --- dq->uvw conversion ---
		float Valpha, Vbeta;
		float Vu, Vv, Vw;
		float sinVal = arm_sin_f32((float)theta/UINT32TMAX * 2*PI);
		float cosVal = arm_cos_f32((float)theta/UINT32TMAX * 2*PI);
		arm_inv_park_f32(Vdref, Vqref, &Valpha, &Vbeta, sinVal, cosVal);
		arm_inv_clarke_f32(Valpha, Vbeta, &Vu, &Vv);
		Vu *= SQRT_2_3;  // absolute transformation
		Vv *= SQRT_2_3;
		Vw = - Vu - Vv;

		// --- get DC bus voltage ---
		Vdc = 12.0;

		// --- PWM ---
		Pwm_IT_main(Vu/Vdc, Vv/Vdc, Vw/Vdc, fs, pm);

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
  * @retval Car id
  */
int Ctrl_setParam(PulseMode* pPm, CarParam* pParam) {
	if (pPm->carid >= -1 && pParam->carid >= -1) {
		pm = pPm;
		param = pParam;
	}
	return pm->carid;
}

/**
  * @brief Turn On/Off VVVF controller.
  * @param toggle: E_CTRL_TOGGLE_OFF: VVVF control algorithm is disabled. Inverter output is set to idle state.
  *                E_CTRL_TOGGLE_ON:  VVVF control algorithm is enabled.
  * @retval None
  */
void Ctrl_toggle(E_CTRL_TOGGLE toggle) {
	ctrlFlag = toggle;
	Pwm_toggle((E_PWM_TOGGLE)toggle);
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
	Ctrl_toggle(E_CTRL_TOGGLE_OFF);
	Ctrl_setParam(pPm, pParam);
}
