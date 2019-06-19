/*
 * SPI.c
 *
 *  Created on: Jun 10, 2019
 *      Author: Sherwin
 */
#include <SPI.h>

uint8_t SPI_transmit_buffer[SPI_TRANSMIT_BUF_SIZE];
uint8_t *SPI_receive_buffer;

int transfer_count = 0;
bool SPI_transfering = false;
bool block_transfer = false;
int transfer_ptr = 0;

/*
 * Call after initializing clocks, sets up UART to 115200 and IO.
 * Sources SMCLK
 *
 * MSP430f5529:
 *              CLK -> P2.7
 *              SIMO -> P3.3
 *              SOMI -> P3.4
 * MSP430FR2355:
 *              CLK -> P4.5
 *              SIMO -> P4.6
 *              SOMI -> P4.7
 *
 * Params:
 *          clk_phase_rising:
 *                          if true, data is captured on rising edge and
 *                          changed on falling edge
 *          clk_pol_high:
 *                          if true, idle polarity for clock is high
 *          msb_first:
 *                          if true, data is shifted in/out MSB first
 *          brclk_div:
 *                          divide factor for BRCLK. f_CLK = BRCLK / brclk_div
 *          cs_idle_high:
 *                          if true, CS is idle high and goes low during every transfer
 */
void SPI_init(bool clk_phase_rising, bool clk_pol_high, bool msb_first, uint16_t brclk_div, bool cs_idle_high) {
    SPI_CS_DIR |= SPI_CS_PIN;
#if defined (__MSP430F5529__)
    P3SEL |= BIT3 + BIT4;
    P2SEL |= BIT7;

    UCA0CTL1 |= UCSWRST;
    UCA0CTL0 = UCSYNC + UCMST;
    if (clk_phase_rising) UCA0CTL0 |= UCCKPH;
    if (clk_pol_high) UCA0CTL0 |= UCCKPL;
    if (msb_first) UCA0CTL0 |= UCMSB;
    if (cs_idle_high) {
        SPI_CS_OUT |= SPI_CS_PIN;
    } else {
        SPI_CS_OUT &= ~SPI_CS_PIN;
    }
    UCA0CTL1 = UCSSEL_2 + UCSWRST;
    UCA0BR0 = (uint8_t) brclk_div;
    UCA0BR1 = (uint8_t) (brclk_div >> 8);
    UCA0IE |= UCRXIE;

    UCA0CTL1 &= ~UCSWRST;
#elif defined (__MSP430FR2355__)
    P4SEL0 = BIT7 + BIT6 + BIT5;

    UCB1CTLW0 |= UCSWRST;
    UCB1CTLW0 = UCSYNC + UCMST + UCSSEL_2 + UCSWRST;
    if (clk_phase_rising) UCB1CTLW0 |= UCCKPH;
    if (clk_pol_high) {
        UCB1CTLW0 |= UCCKPL;
        SPI_CS_OUT |= SPI_CS_PIN;
    } else {
        SPI_CS_OUT &= ~SPI_CS_PIN;
    }
    if (msb_first) UCB1CTLW0 |= UCMSB;
    UCB1BRW = brclk_div;
    UCB1IE |= UCRXIE;

    UCB1CTLW0 &= ~UCSWRST;
#endif
}


/*
 * Utility function
 *
 * Params:
 *          src: array to copy from
 *          dest: array to copy to
 *          count: number of elements to copy
 */
void SPI_copy_array(uint8_t *src, uint8_t *dest, int count) {
    int i;
    for (i = 0; i < count; i++) dest[i] = src[i];
}

/*
 * Puts bytes in transmit buffer and initiates SPI transfer. Puts
 * received bytes in rx_buffer
 *
 * Params:
 *          tx_buffer: array to transmit  from
 *          rx_buffer: array to write received bytes
 *          count: number of bytes to transfer
 *          blocking: if  true, function enters  LMP0 and doesn't
 *                  return until all bytes have been transfered
 *                  (status can be checked with the SPI_transfering
 *                  variable)
 */
void SPI_transfer_bytes(uint8_t *tx_buffer, uint8_t *rx_buffer, int count, bool blocking) {
    SPI_copy_array(tx_buffer, SPI_transmit_buffer, count);

    SPI_receive_buffer = rx_buffer;
    transfer_ptr = 0;
    transfer_count = count;
    SPI_transfering = true;
    block_transfer = blocking;

    SPI_CS_OUT ^= SPI_CS_PIN;
    UCA0IE |= UCRXIE;
#if defined (__MSP430F5529__)
    UCA0TXBUF = SPI_transmit_buffer[transfer_ptr];
#elif defined (__MSP430FR2355__)
    UCB1TXBUF_L = SPI_transmit_buffer[transfer_ptr];
#endif
    if (blocking) {
        __bis_SR_register(LPM0_bits + GIE);
    }
}

#if defined (__MSP430F5529__)
#pragma vector=USCI_A0_VECTOR
#elif defined (__MSP430FR2355__)
#pragma vector=EUSCI_B1_VECTOR
#endif
__interrupt void SPI_ISR(void) {
    uint8_t uciv = UCA0IV;
    switch(uciv) {
        case 0x02:
#if defined (__MSP430F5529__)
            SPI_receive_buffer[transfer_ptr] = UCA0RXBUF;
#elif defined (__MSP430FR2355__)
            SPI_receive_buffer[transfer_ptr] = UCB1RXBUF_L;
#endif
            transfer_ptr++;
            if (transfer_ptr < transfer_count) {
#if defined (__MSP430F5529__)
                UCA0TXBUF = SPI_transmit_buffer[transfer_ptr];
#elif defined (__MSP430FR2355__)
                UCB1TXBUF_L = SPI_transmit_buffer[transfer_ptr];
#endif
            } else {
                SPI_CS_OUT ^= SPI_CS_PIN;
                SPI_transfering = false;
                if (block_transfer) __bic_SR_register_on_exit(CPUOFF);
            }
#if defined (__MSP430F5529__)
            UCA0IFG &= ~(UCRXIFG);
#elif defined (__MSP430FR2355__)
            UCB1IFG &= ~UCRXIFG;
#endif
            break;
    }
}


