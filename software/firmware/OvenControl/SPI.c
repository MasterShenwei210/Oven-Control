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
void SPI_init(bool clk_phase_rising, bool clk_pol_high, bool msb_first, uint32_t clk_freq, bool cs_idle_high) {
    GPIO_setAsPeripheralModuleFunctionInputPin(
            GPIO_PORT_UCB1CLK,
            GPIO_PIN_UCB1CLK,
            GPIO_FUNCTION_UCB1CLK
    );

    GPIO_setAsPeripheralModuleFunctionInputPin(
            GPIO_PORT_UCB1SIMO,
            GPIO_PIN_UCB1SIMO,
            GPIO_FUNCTION_UCB1SIMO
    );

    GPIO_setAsPeripheralModuleFunctionInputPin(
            GPIO_PORT_UCB1SOMI,
            GPIO_PIN_UCB1SOMI,
            GPIO_FUNCTION_UCB1SOMI
    );

    GPIO_setAsOutputPin(GPIO_PORT_CSB, GPIO_PIN_CSB);

    if (cs_idle_high) {
        GPIO_setOutputHighOnPin(GPIO_PORT_CSB, GPIO_PIN_CSB);
    } else {
        GPIO_setOutputLowOnPin(GPIO_PORT_CSB, GPIO_PIN_CSB);
    }

    EUSCI_B_SPI_initMasterParam param = {0};
    if (clk_phase_rising) param.clockPhase = EUSCI_B_SPI_PHASE_DATA_CAPTURED_ONFIRST_CHANGED_ON_NEXT;
    else param.clockPhase = EUSCI_B_SPI_PHASE_DATA_CHANGED_ONFIRST_CAPTURED_ON_NEXT;

    if (clk_pol_high) param.clockPolarity = EUSCI_B_SPI_CLOCKPOLARITY_INACTIVITY_HIGH;
    else param.clockPolarity = EUSCI_B_SPI_CLOCKPOLARITY_INACTIVITY_LOW;

    if (msb_first) param.msbFirst = EUSCI_B_SPI_MSB_FIRST;
    else param.msbFirst = EUSCI_B_SPI_LSB_FIRST;

    param.selectClockSource = EUSCI_B_SPI_CLOCKSOURCE_SMCLK;
    param.clockSourceFrequency = SMCLK_FREQ;
    param.desiredSpiClock = clk_freq;
    param.spiMode = EUSCI_B_SPI_3PIN;

    EUSCI_B_SPI_initMaster(EUSCI_B1_BASE, &param);
    EUSCI_B_SPI_enable(EUSCI_B1_BASE);
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
    if (blocking) {
        int i = 0;
        GPIO_toggleOutputOnPin(GPIO_PORT_CSB, GPIO_PIN_CSB);
        for (i = 0; i < count; i++) {
            EUSCI_B_SPI_transmitData(EUSCI_B1_BASE, tx_buffer[i]);
            while (EUSCI_B_SPI_isBusy(EUSCI_B1_BASE) == EUSCI_B_SPI_BUSY);
            rx_buffer[i] = EUSCI_B_SPI_receiveData(EUSCI_B1_BASE);
        }
        GPIO_toggleOutputOnPin(GPIO_PORT_CSB, GPIO_PIN_CSB);
    } else {
        SPI_copy_array(tx_buffer, SPI_transmit_buffer, count);

        SPI_receive_buffer = rx_buffer;
        transfer_ptr = 0;
        transfer_count = count;
        SPI_transfering = true;

        EUSCI_B_SPI_clearInterrupt(
                EUSCI_B1_BASE,
                EUSCI_B_SPI_RECEIVE_INTERRUPT
        );
        EUSCI_B_SPI_enableInterrupt(
                EUSCI_B1_BASE,
                EUSCI_B_SPI_RECEIVE_INTERRUPT
        );
        __bis_SR_register(GIE);
        GPIO_toggleOutputOnPin(GPIO_PORT_CSB, GPIO_PIN_CSB);
        EUSCI_B_SPI_transmitData(EUSCI_B1_BASE, SPI_transmit_buffer[transfer_ptr]);
    }
}

#pragma vector=USCI_B1_VECTOR
__interrupt void SPI_ISR(void) {
    switch(__even_in_range(UCB1IV, USCI_SPI_UCTXIFG)) {
        case USCI_SPI_UCRXIFG:
            SPI_receive_buffer[transfer_ptr] = EUSCI_B_SPI_receiveData(EUSCI_B1_BASE);
            transfer_ptr++;
            if (transfer_ptr < transfer_count) {
                EUSCI_B_SPI_transmitData(EUSCI_B1_BASE, SPI_transmit_buffer[transfer_ptr]);
            } else {
                GPIO_toggleOutputOnPin(GPIO_PORT_CSB, GPIO_PIN_CSB);
                SPI_transfering = false;
                EUSCI_B_SPI_disableInterrupt(
                        EUSCI_B1_BASE,
                        EUSCI_B_SPI_RECEIVE_INTERRUPT
                );
            }
            break;
    }
}


