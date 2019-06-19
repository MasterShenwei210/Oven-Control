     /*
 * UART.c
 *
 *  Created on: Jun 5, 2019
 *      Author: Sherwin
 */
/*
 * NOTE: requires 4MHz SMCLK and general interrupts be enabled for RX interrupts
 */

#include <UART.h>

uint8_t UART_transmit_buffer[UART_TRANSMIT_BUF_SIZE];
int transmit_count = 0;
int transmit_ptr = 0;
bool UART_transmitting;
bool block_transmit = false;

uint8_t UART_receive_buffer[UART_RECEIVE_BUF_SIZE];
int UART_receive_count = 0;
int receive_ptr = 0;
bool block_receive = false;
int block_receive_count = 0;

/*
 * Call after initializing clocks, sets up UART to 115200 and IO.
 *
 * MSP430f5529:
 *              TX -> P4.4
 *              RX -> P4.5
 * MSP430FR2355:
 *              TX -> P4.3
 *              RX -> P4.2
 */
void UART_init() {
#if defined (__MSP430F5529__)
    P4SEL |= BIT4 | BIT5;

    UCA1CTL1 |= UCSWRST;
    UCA1CTL1 = UCSSEL_2 + UCSWRST;
    // Sets baud rate to 115200
    UCA1BR0 = 34;
    UCA1BR1 = 0;
    UCA1MCTL = 6 << 1;

    UCA1CTL1 &= ~UCSWRST;

    UCA1IE |= UCRXIE;
#elif defined (__MSP430FR2355__)
    // Configure UART pins
      P4SEL0 |= BIT2 | BIT3;                    // set 2-UART pin as second function

      // Configure UART
      UCA1CTLW0 |= UCSWRST;
      UCA1CTLW0 |= UCSSEL__SMCLK;

      // Baud Rate calculation
      // 8000000/(16*9600) = 52.083
      // Fractional portion = 0.083
      // User's Guide Table 17-4: UCBRSx = 0x49
      // UCBRFx = int ( (52.083-52)*16) = 1
      UCA1BR0 = 52;                             // 8000000/16/9600
      UCA1BR1 = 0x00;
      UCA1MCTLW = 0x4900 | UCOS16 | UCBRF_1;

      UCA1CTLW0 &= ~UCSWRST;                    // Initialize eUSCI
      UCA1IE |= UCRXIE;                         // Enable USCI_A0 RX interrupt

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
void UART_copy_array(uint8_t *src, uint8_t *dest, int count) {
    int i;
    for (i = 0; i < count; i++) dest[i] = src[i];
}

/*
 * Puts bytes in transmit buffer and initiates UART transmit
 *
 * Params:
 *          buffer: array to transmit from
 *          count: number of bytes to transmit
 *          blocking: if true, the function enters LPM0 and returns when
 *                      all the bytes have finished transmitting, otherwise
 *                      it exits and transmission happens in the background
 *                      (status can be checked with UART_transmitting variable)
 */
void UART_transmit_bytes(uint8_t *buffer, int count, bool blocking) {
    UART_copy_array(buffer, UART_transmit_buffer, count);

    transmit_ptr = 0;
    block_transmit = blocking;
    transmit_count = count;
    UART_transmitting = true;

    // Enable transmit complete interrupt and transmit 1st byte
#if defined (__MSP430F5529__)
    UCA1IE |= UCTXIE;
    UCA1TXBUF = UART_transmit_buffer[transmit_ptr];
#elif defined (__MSP430FR2355__)
    UCA1IE |= UCTXIE;
    UCA1TXBUF = (uint16_t) UART_transmit_buffer[transmit_ptr];
#endif
    // Enter sleep mode if blocking
    if (blocking) {
        __bis_SR_register(LPM0_bits | GIE);
    }
}

/*
 * Puts bytes from receive buffer into array, the receive buffer gets
 * emptied whenever this function is called. Can check the number of bytes
 * available with the UART_receive_count variable.
 *
 * Params:
 *          buffer: array put received bytes into
 *          blocking: if true, the function enters LPM0 and doesn't return until count
 *                      number of bytes have been received
 *          count: number of bytes to wait for if blocking (this param is ignored
 *                  if blockin = false)
 */
void UART_receive_bytes(uint8_t *buffer, bool blocking, int count) {
    // Disable interrupts to prevent potential race condition
    __disable_interrupt();
    // Enter sleep mode if blocking and there aren't enough bytes available
    if (blocking && UART_receive_count < count) {
        block_receive = true;
        block_receive_count = count;
        __bis_SR_register(LPM0_bits + GIE);
        __disable_interrupt();
    }
    UART_copy_array(UART_receive_buffer, buffer, UART_receive_count);
    UART_receive_count = 0;
    receive_ptr = 0;
    __enable_interrupt();
}

#if defined (__MSP430F5529__)
#pragma vector=USCI_A1_VECTOR
#elif defined (__MSP430FR2355__)
//#pragma vector=EUSCI_A1_VECTOR
#endif
__interrupt void UART_ISR(void) {
    uint16_t uciv = UCA1IV;
    switch(uciv) {
        // Transmit complete interrupt
        case 0x04:
            transmit_ptr++;
            if (transmit_ptr < transmit_count) {
#if defined (__MSP430F5529__)
                UCA1TXBUF = UART_transmit_buffer[transmit_ptr];
#elif defined (__MSP430FR2355__)
                UCA1TXBUF = (uint16_t) UART_transmit_buffer[transmit_ptr];
#endif
            } else {
                UART_transmitting = false;
                UCA1IE &= ~UCTXIE;
                // Exit LPM0 if blocking
                if (block_transmit) __bic_SR_register_on_exit(CPUOFF);
            }
            UCA1IFG &= ~UCTXIFG;
            break;

        // Received byte interrupt
        case 0x02:
            if (receive_ptr < UART_RECEIVE_BUF_SIZE) {
#if defined (__MSP430F5529__)
                UART_receive_buffer[receive_ptr] = UCA1RXBUF;
#elif defined (__MSP430FR2355__)
                UART_receive_buffer[receive_ptr] = UCA1RXBUF_L;
#endif
            }
            receive_ptr++;
            UART_receive_count++;

            // Exit LPM0 if blocking
            if (block_receive) {
               if (UART_receive_count >= block_receive_count) __bic_SR_register_on_exit(CPUOFF);
            }

            UCA1IFG &= ~UCRXIFG;
            break;
    }
}

/************EXAMPLE CODE*************
 * uint8_t string[20] = {0};
int main(void)
{
    uint8_t string[20] = {0};
    WDTCTL = WDTPW | WDTHOLD;   // stop watchdog timer
    initClockTo16MHz();
    UART_init();
    __enable_interrupt();
    UART_transmit_bytes("Enter your name: \r\n", 19, false);
    UART_receive_bytes(string, true, 20);
    int k = 0;
    while (string[k] != 0 && k < 20) k++;
    UART_transmit_bytes("Your name is: ", 14, true);
    UART_transmit_bytes(string, k-1, true);
    return 0;
}
 */


