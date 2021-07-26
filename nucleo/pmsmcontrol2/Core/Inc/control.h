/*
 * control.h
 *
 *  Created on: Jun 6, 2021
 *      Author: denjo
 */

#ifndef INC_CONTROL_H_
#define INC_CONTROL_H_

/* Private includes ----------------------------------------------------------*/
#include "stm32f3xx_hal.h"
#include "cardata.h"

/* Exported types ------------------------------------------------------------*/
typedef enum {E_CTRL_TOGGLE_OFF, E_CTRL_TOGGLE_ON} E_CTRL_TOGGLE;

/* Exported constants --------------------------------------------------------*/

/* Exported macro ------------------------------------------------------------*/

/* Exported functions prototypes ---------------------------------------------*/
void Ctrl_init(TIM_HandleTypeDef*, PulseMode*, CarParam*);
void Ctrl_toggle(E_CTRL_TOGGLE);
int Ctrl_setParam(PulseMode*, CarParam*);
void Ctrl_IT_main(void);

/* Private defines -----------------------------------------------------------*/

#endif /* INC_CONTROL_H_ */
