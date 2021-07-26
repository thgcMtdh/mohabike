/*
 * hall.h
 *
 *  Created on: 2021/07/18
 *      Author: denjo
 */

#ifndef INC_HALL_H_
#define INC_HALL_H_

/* Private includes ----------------------------------------------------------*/

/* Exported types ------------------------------------------------------------*/
typedef enum {E_HALL_TOGGLE_OFF, E_HALL_TOGGLE_ON} E_HALL_TOGGLE;

/* Exported constants --------------------------------------------------------*/

/* Exported macro ------------------------------------------------------------*/

/* Exported functions prototypes ---------------------------------------------*/
void Hall_init(TIM_HandleTypeDef* tim2ptr);
void Hall_toggle(E_HALL_TOGGLE toggle);
uint32_t Hall_getTheta(void);
float Hall_getFs(void);

void Hall_IT_calcFs(void);

/* Private defines -----------------------------------------------------------*/

#endif /* INC_HALL_H_ */
