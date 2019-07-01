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

uint8_t UART_receive_buffer[UART_RECEIVE_BUF_SIZE];
uint8_t UART_transmit_buffer[UART_TRANSMIT_BUF_SIZE];

int UART_receive_count = 0;
bool UART_transmitting = false;

int transmit_ptr = 0;
int transmit_count;

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
    //Configure UART pins
    GPIO_setAsPeripheralModuleFunctionInputPin(
        GPIO_PORT_UCA1TXD,
        GPIO_PIN_UCA1TXD,
        GPIO_FUNCTION_UCA1TXD
    );
    GPIO_setAsPeripheralModuleFunctionInputPin(
        GPIO_PORT_UCA1RXD,
        GPIO_PIN_UCA1RXD,
        GPIO_FUNCTION_UCA1RXD
    );

    PMM_unlockLPM5();
    //Configure UART
    //SMCLK = 12MHz, Baudrate = 115200
    //UCBRx = 6, UCBRFx = 8, UCBRSx = 0x20, UCOS16 = 1

    EUSCI_A_UART_initParam param = {0};
    param.selectClockSource = EUSCI_A_UART_CLOCKSOURCE_SMCLK;
    param.clockPrescalar = 6;
    param.firstModReg = 8;
    param.secondModReg = 0x20;
    param.parity = EUSCI_A_UART_NO_PARITY;
    param.msborLsbFirst = EUSCI_A_UART_LSB_FIRST;
    param.numberofStopBits = EUSCI_A_UART_ONE_STOP_BIT;
    param.uartMode = EUSCI_A_UART_MODE;
    param.overSampling = EUSCI_A_UART_OVERSAMPLING_BAUDRATE_GENERATION;

    if (STATUS_FAIL == EUSCI_A_UART_init(EUSCI_A1_BASE, &param)) {
        return;
    }



    EUSCI_A_UART_enable(EUSCI_A1_BASE);

    EUSCI_A_UART_clearInterrupt(EUSCI_A1_BASE,
            EUSCI_A_UART_RECEIVE_INTERRUPT);

    EUSCI_A_UART_enableInterrupt(EUSCI_A1_BASE,
            EUSCI_A_UART_RECEIVE_INTERRUPT);
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
    while (UART_transmitting);
    if (blocking) {
        int i;
        for (i = 0; i < count; i++) {
            EUSCI_A_UART_transmitData(EUSCI_A1_BASE,
                buffer[i]);
        }
    } else {
        UART_copy_array(buffer, UART_transmit_buffer, count);
        transmit_ptr = 0;
        transmit_count = count;
        UART_transmitting = true;

        EUSCI_A_UART_clearInterrupt(EUSCI_A1_BASE,
                EUSCI_A_UART_TRANSMIT_INTERRUPT);

        EUSCI_A_UART_enableInterrupt(EUSCI_A1_BASE,
                EUSCI_A_UART_TRANSMIT_INTERRUPT);

        EUSCI_A_UART_transmitData(EUSCI_A1_BASE,
                        UART_transmit_buffer[transmit_ptr]);
    }
}

/*
 * Puts bytes from receive buffer into array, the receive buffer gets
 * emptied whenever this function is called. Can check the number of bytes
 * available with the UART_receive_count variable.
 *
 * Params:
 *          buffer: array put received bytes into
 */
void UART_receive_bytes(uint8_t *buffer) {
    // Disable interrupts to prevent potential race condition
    __disable_interrupt();
    // Enter sleep mode if blocking and there aren't enough bytes available
    UART_copy_array(UART_receive_buffer, buffer, UART_receive_count);
    UART_receive_count = 0;
    __enable_interrupt();
}


#pragma vector=USCI_A1_VECTOR
__interrupt
void UART_ISR(void) {
    switch(__even_in_range(UCA1IV,USCI_UART_UCTXCPTIFG)) {
        // Transmit complete interrupt
        case USCI_UART_UCTXIFG:
            transmit_ptr++;
            if (transmit_ptr >= transmit_count) {
                EUSCI_A_UART_disableInterrupt(EUSCI_A1_BASE,
                        EUSCI_A_UART_TRANSMIT_INTERRUPT);
                UART_transmitting = false;
            } else {
                EUSCI_A_UART_transmitData(EUSCI_A1_BASE,
                        UART_transmit_buffer[transmit_ptr]);
            }

            EUSCI_A_UART_clearInterrupt(EUSCI_A1_BASE,
                    EUSCI_A_UART_TRANSMIT_INTERRUPT);
            break;

        case USCI_UART_UCRXIFG:
            if (UART_receive_count < UART_RECEIVE_BUF_SIZE) {
                UART_receive_buffer[UART_receive_count] = EUSCI_A_UART_receiveData(EUSCI_A1_BASE);
                EUSCI_A_UART_transmitData(EUSCI_A1_BASE,
                    UART_receive_buffer[UART_receive_count]
                );

                UART_receive_count++;
            }
            command_state = COMMAND_RECEIVE;
            __bic_SR_register_on_exit(LPM0_bits);
            EUSCI_A_UART_clearInterrupt(
                    EUSCI_A1_BASE,
                    EUSCI_A_UART_RECEIVE_INTERRUPT);
            break;
    }
}
