#include "driverlib.h"
#include "Board.h"
#include "UART.h"

#define MCLK_FREQ_KHZ 24000
#define MCLK_FLLREF_RATIO 732   // 32.768kHz * 732 = 24MHz

uint8_t string[10] = {0};

void init_clocks();
int main(void) {
    WDT_A_hold(WDT_A_BASE);
    PMM_unlockLPM5();

    init_clocks();
    UART_init();
    __enable_interrupt();

    while (1) {
        if (UART_receive_count) {
            int count = UART_receive_count;
            UART_receive_bytes(string);
            UART_transmit_bytes(string, count, false);
        }

        __no_operation();

    }
    return (0);
}

void init_clocks() {
    CS_initClockSignal(
          CS_FLLREF,         // The reference for Frequency Locked Loop
          CS_REFOCLK_SELECT,  // Select REFOCLK
          CS_CLOCK_DIVIDER_1 // The FLL reference will be 32.768KHz
    );

    CS_initFLLSettle(
            MCLK_FREQ_KHZ,
            MCLK_FLLREF_RATIO
    );

    //Set ACLK = REFOCLK with clock divider of 1,  32.768kHz
    CS_initClockSignal(CS_ACLK,CS_REFOCLK_SELECT,CS_CLOCK_DIVIDER_1);
    //Set SMCLK = DCO with frequency divider of 2, 12MHz
    CS_initClockSignal(CS_SMCLK,CS_DCOCLKDIV_SELECT,CS_CLOCK_DIVIDER_2);
    //Set MCLK = DCO with frequency divider of 2, 24MHz
    CS_initClockSignal(CS_MCLK,CS_DCOCLKDIV_SELECT,CS_CLOCK_DIVIDER_1);

}


