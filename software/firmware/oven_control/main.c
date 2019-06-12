#include <msp430.h> 
#include <UART.h>

/**
 * main.c
 */
void init_clocks();

int main(void) {
	WDTCTL = WDTPW | WDTHOLD;	// stop watchdog timer
	return 0;
}

void init_clocks() {
    __bis_SR_register(SCG0);    // Disable the FLL control loop
    CSCTL0 = 0x0000;            //Set lowest possible DCOx, MODx
    CSCTL1 = DCORSEL_5;         //set range to 16MHz
    CSCTL2 = 487;               //set FLLD to 488 => f_DCO = 32768 * 488 = 16MHz
    CSCTL3 = SELREF_1;         //set FLL reference to 32.768kHz internal clk
    CSCTL4 = SELA__VLOCLK | SELMS__DCOCLKDIV; //set ACLK to 10kHz VCO, MCLK and SMCLK to DCO
    CSCTL5 = DIVS__4;           //div SMCLK src by 4, so SMCLK is 4MHz
    CSCTL6 = DIVA__1;            //div ACLK by 1, so ACLK is 10kHz

    __bic_SR_register(SCG0);                  // Enable the FLL control loop
    __delay_cycles(500000);
}
