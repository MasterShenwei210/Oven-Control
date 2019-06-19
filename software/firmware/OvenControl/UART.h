/*
 * UART.h
 *
 *  Created on: Jun 5, 2019
 *      Author: Sherwin
 */

#ifndef UART_H_
#define UART_H_

#include "driverlib.h"
#include "Board.h"

#define UART_TRANSMIT_BUF_SIZE  20
#define UART_RECEIVE_BUF_SIZE   20

extern bool UART_transmitting;
extern int UART_receive_count;

void UART_init();
void UART_copy_array(uint8_t *src, uint8_t *dest, int count);

void UART_transmit_bytes(uint8_t *buffer, int count, bool blocking);
void UART_receive_bytes(uint8_t *buffer);


#endif /* UART_H_ */
