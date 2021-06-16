/*
 * pwm.h
 *
 *  Created on: 2021/06/06
 *      Author: denjo
 */

#ifndef INC_PWM_H_
#define INC_PWM_H_

/* Private includes ----------------------------------------------------------*/

/* Exported types ------------------------------------------------------------*/
typedef enum {E_PWM_TOGGLE_OFF, E_PWM_TOGGLE_ON} E_PWM_TOGGLE;

/* Exported constants --------------------------------------------------------*/

/* Exported macro ------------------------------------------------------------*/

/* Exported functions prototypes ---------------------------------------------*/
void Pwm_init(TIM_HandleTypeDef*);
void Pwm_toggle(E_PWM_TOGGLE);
float Pwm_asyncpwm(float, float, float, float, float);

/* Private defines -----------------------------------------------------------*/
#define FCLK 72000000
#define PWM_DEFAULT_PSC 9
#define PWM_DEFAULT_FC 2000.0

#endif /* INC_PWM_H_ */
