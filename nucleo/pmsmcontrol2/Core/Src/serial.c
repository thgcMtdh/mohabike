/*
 * communication.c
 *
 *  Created on: Jun 16, 2021
 *      Author: denjo
 */

/**
  ******************************************************************************
  * @file           : serial.c
  * @brief          : Handling USART communication
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/

/* Private includes ----------------------------------------------------------*/
#include "stm32f3xx_hal.h"
#include "serial.h"

/* Private typedef -----------------------------------------------------------*/

/* Private define ------------------------------------------------------------*/

/* Private macro -------------------------------------------------------------*/

/* Private variables ---------------------------------------------------------*/
static UART_HandleTypeDef* phuart;

static uint8_t dma_rxbuf[SERIAL_RXBUFSIZE];   // DMA buffer (directly written by DMA controller)
static uint32_t dma_read_ptr, dma_write_ptr;  // DMA pointer (where to write/read)

/* Private function prototypes -----------------------------------------------*/

/* Private user code ---------------------------------------------------------*/

/**
  * @brief Set UART handler and initialize serial communication
  * @param Pointer to the uart handle for PC
  * @retval None
  */
void Serial_init(UART_HandleTypeDef* huartptr) {
	phuart = huartptr;
	HAL_UART_Receive_DMA(huartptr, dma_rxbuf, SERIAL_RXBUFSIZE);  // start DMA
	dma_read_ptr = 0;
	for (int i=0; i<SERIAL_RXBUFSIZE; i++) {
		dma_rxbuf[i] = 0;
	}
	// These UART interrupts halt any ongoing transfer if an error occurs, disable them
	 __HAL_UART_DISABLE_IT(huartptr, UART_IT_PE);   // Disable the UART Parity Error Interrupt
	 __HAL_UART_DISABLE_IT(huartptr, UART_IT_ERR);  // Disable the UART Error Interrupt: (Frame error, noise error, overrun error)
}

/**
  * @brief Return if the UART recieve buffer is empty.
  * @param None
  * @retval 0:NOT empty (there is data), 1:Empty
  */
int Serial_isEmpty(void) {
	dma_write_ptr = (SERIAL_RXBUFSIZE - phuart->hdmarx->Instance->CNDTR) % SERIAL_RXBUFSIZE;
	return (dma_read_ptr == dma_write_ptr);
}

/**
  * @brief Read 1 byte from UART recieve buffer.
  * @param None
  * @retval A byte read from buffer. If the buffer is empty, returns 0.
  */
uint8_t Serial_read() {
	uint8_t c = 0;
	dma_write_ptr = (SERIAL_RXBUFSIZE - phuart->hdmarx->Instance->CNDTR) % SERIAL_RXBUFSIZE;
	if (dma_read_ptr != dma_write_ptr) {
		c = dma_rxbuf[dma_read_ptr++];
		dma_read_ptr %= SERIAL_RXBUFSIZE;
	}
	return c;
}

/**
  * @brief Send 1 bytes
  * @param pointer to data buffer, amount of data elements, timeout duration
  * @retval hal status
  */
HAL_StatusTypeDef Serial_send(uint8_t* pData, uint16_t size, uint32_t timeout) {
	return HAL_UART_Transmit(phuart, pData, size, timeout);
}
