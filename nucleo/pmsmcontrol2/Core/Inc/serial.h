/*
 * serial.h
 *
 *  Created on: Jun 16, 2021
 *      Author: denjo
 */

#ifndef INC_SERIAL_H_
#define INC_SERIAL_H_

/* Private includes ----------------------------------------------------------*/

/* Exported types ------------------------------------------------------------*/

/* Exported constants --------------------------------------------------------*/
#define SERIAL_RXBUFSIZE 16   // must be power of two
#define SERIAL_TXBUFSIZE 128

/* Exported macro ------------------------------------------------------------*/

/* Exported functions prototypes ---------------------------------------------*/
void Serial_init(UART_HandleTypeDef*);
int Serial_isEmpty(void);
uint8_t Serial_read(void);
HAL_StatusTypeDef Serial_send(uint8_t*, uint16_t, uint32_t);

/* Private defines -----------------------------------------------------------*/

#endif /* INC_SERIAL_H_ */
