/*
 * SPI.h
 *
 *  Created on: Jun 10, 2019
 *      Author: Sherwin
 */

#ifndef SPI_H_
#define SPI_H_

#include <msp430.h>
#include <stdint.h>
#include <stdbool.h>

#define SPI_TRANSMIT_BUF_SIZE  20

#if defined (__MSP430F5529__)
#define SPI_CS_OUT      P1OUT
#define SPI_CS_DIR      P1DIR
#define SPI_CS_PIN      BIT0
#endif

extern bool SPI_transfering;

void SPI_init(bool clk_phase_rising, bool clk_pol_high, bool msb_first, uint16_t brclk_div, bool cs_idle_high);
void SPI_copy_array(uint8_t *src, uint8_t *dest, int count);

void SPI_transfer_bytes(uint8_t *tx_buffer, uint8_t *rx_buffer, int count, bool blocking);
#endif /* SPI_H_ */
