/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.h
  * @brief          : Header for main.c file.
  *                   This file contains the common defines of the application.
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

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __MAIN_H
#define __MAIN_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32f3xx_hal.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#define ARM_MATH_CM4
#include "arm_math.h"
#include "arm_const_structs.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
/* USER CODE END Includes */

/* Exported types ------------------------------------------------------------*/
/* USER CODE BEGIN ET */

/* USER CODE END ET */

/* Exported constants --------------------------------------------------------*/
/* USER CODE BEGIN EC */

/* USER CODE END EC */

/* Exported macro ------------------------------------------------------------*/
/* USER CODE BEGIN EM */

/* USER CODE END EM */

void HAL_TIM_MspPostInit(TIM_HandleTypeDef *htim);

/* Exported functions prototypes ---------------------------------------------*/
void Error_Handler(void);

/* USER CODE BEGIN EFP */

/* USER CODE END EFP */

/* Private defines -----------------------------------------------------------*/
/* USER CODE BEGIN Private defines */

#define FCLK 72000000  // system clock frequency [Hz]
#define PRSC 9         // prescaler of TIM1
#define R_TIRE 0.3302  // radius of tire [m]
#define GEAR 112.6533  // pole pair * gear ratio (motor freq -> tire rotation freq)
#define dtMAX 36000000   // if there is no hall transition in dtMAX/FCLK [s], assume motor7 has stopped
#define ADCBufferSize 3
#define dtSAMPLENUM 5    // the number of samples to calculate average speed

extern uint32_t theta_est, theta_u;
extern float CtrlPrd, omega_est, speed;
extern float fs, fc;
extern float Vd, Vq, Vs, Vdc;
extern float acc;
extern int pmNo, pmNo_ref;
extern int pulsemode, pulsemode_ref;
extern volatile uint32_t start, stop;
extern float list_fs[16], list_fc1[16], list_fc2[16], list_frand1[16], list_frand2[16];
extern int list_pulsemode[16], pulsenum;
extern volatile uint16_t ADCBuffer[ADCBufferSize];
extern UART_HandleTypeDef huart2;
extern uint8_t hall_u, hall_v, hall_w, sector;

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
