/*
 * pwm.h
 *
 *  Created on: 2021/06/06
 *      Author: denjo
 */

#ifndef INC_PWM_H_
#define INC_PWM_H_

/* Private includes ----------------------------------------------------------*/
#include "cardata.h"

/* Exported types ------------------------------------------------------------*/
typedef enum {E_PWM_TOGGLE_OFF, E_PWM_TOGGLE_ON} E_PWM_TOGGLE;

/* Exported constants --------------------------------------------------------*/

/* Exported macro ------------------------------------------------------------*/

/* Exported functions prototypes ---------------------------------------------*/
void Pwm_init(TIM_HandleTypeDef*);
void Pwm_toggle(E_PWM_TOGGLE);
void Pwm_IT_main(float, float, float, float, PulseMode*);
void Pwm_asyncpwm(float, float, float);

/* Private defines -----------------------------------------------------------*/
#define PWM_DEFAULT_PSC 9
#define PWM_DEFAULT_FC 2000.0

#endif /* INC_PWM_H_ */
