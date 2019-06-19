#include <msp430.h>
#include <UART.h>

/**
 * main.c
 */
/*
void init_clocks();

int main(void) {
	WDTCTL = WDTPW | WDTHOLD;	// stop watchdog timer
	init_clocks();
	UART_init();
	__enable_interrupt();
	for(;;) {
	    UART_transmit_bytes("hey\r\n", 5, true);
	    __delay_cycles(4000000);
	}
	return 0;
}

void init_clocks() {
    CSCTL3 = SELREF_1;         //set FLL reference to 32.768kHz internal clk
    CSCTL4 = SELA__VLOCLK + SELMS__DCOCLKDIV; //set ACLK to 10kHz VLO, MCLK and SMCLK to DCO
    CSCTL5 = DIVS__4;           //div SMCLK src by 4, so SMCLK is 4MHz
    CSCTL6 = DIVA__1;            //div ACLK by 1, so ACLK is 10kHz

    __bis_SR_register(SCG0);    // Disable the FLL control loop
    CSCTL0 = 0x0000;            //Set lowest possible DCOx, MODx
    CSCTL1 = DCORSEL_5;         //set range to 16MHz
    CSCTL2 = FLLD_0 + 487;               //set FLLD to 487 => f_DCO = 32768 * 487 = 16MHz

    __bic_SR_register(SCG0);                  // Enable the FLL control loop
    __delay_cycles(500000);
}
*/

void init_clocks();
void Init_GPIO();
void Software_Trim();                       // Software Trim to get the best DCOFTRIM value
#define MCLK_FREQ_MHZ 8                     // MCLK = 8MHz

int main(void)
{
  WDTCTL = WDTPW | WDTHOLD;                // Stop watchdog timer
                                      // to activate 1previously configured port settings
  init_clocks();
  UART_init();
  //__enable_interrupt();
     /* for(;;) {
          UCA1TXBUF = 0x61;
          //UART_transmit_bytes("hey\r\n", 5, false);
          //__delay_cycles(4000000);
      }*/

  __bis_SR_register(LPM0_bits|GIE);         // Enter LPM3, interrupts enabled
  __no_operation();                         // For debugger
  return 0;
}

void Software_Trim()
{
    unsigned int oldDcoTap = 0xffff;
    unsigned int newDcoTap = 0xffff;
    unsigned int newDcoDelta = 0xffff;
    unsigned int bestDcoDelta = 0xffff;
    unsigned int csCtl0Copy = 0;
    unsigned int csCtl1Copy = 0;
    unsigned int csCtl0Read = 0;
    unsigned int csCtl1Read = 0;
    unsigned int dcoFreqTrim = 3;
    unsigned char endLoop = 0;

    do
    {
        CSCTL0 = 0x100;                         // DCO Tap = 256
        do
        {
            CSCTL7 &= ~DCOFFG;                  // Clear DCO fault flag
        }while (CSCTL7 & DCOFFG);               // Test DCO fault flag

        __delay_cycles((unsigned int)3000 * MCLK_FREQ_MHZ);// Wait FLL lock status (FLLUNLOCK) to be stable
                                                           // Suggest to wait 24 cycles of divided FLL reference clock
        while((CSCTL7 & (FLLUNLOCK0 | FLLUNLOCK1)) && ((CSCTL7 & DCOFFG) == 0));

        csCtl0Read = CSCTL0;                   // Read CSCTL0
        csCtl1Read = CSCTL1;                   // Read CSCTL1

        oldDcoTap = newDcoTap;                 // Record DCOTAP value of last time
        newDcoTap = csCtl0Read & 0x01ff;       // Get DCOTAP value of this time
        dcoFreqTrim = (csCtl1Read & 0x0070)>>4;// Get DCOFTRIM value

        if(newDcoTap < 256)                    // DCOTAP < 256
        {
            newDcoDelta = 256 - newDcoTap;     // Delta value between DCPTAP and 256
            if((oldDcoTap != 0xffff) && (oldDcoTap >= 256)) // DCOTAP cross 256
                endLoop = 1;                   // Stop while loop
            else
            {
                dcoFreqTrim--;
                CSCTL1 = (csCtl1Read & (~DCOFTRIM)) | (dcoFreqTrim<<4);
            }
        }
        else                                   // DCOTAP >= 256
        {
            newDcoDelta = newDcoTap - 256;     // Delta value between DCPTAP and 256
            if(oldDcoTap < 256)                // DCOTAP cross 256
                endLoop = 1;                   // Stop while loop
            else
            {
                dcoFreqTrim++;
                CSCTL1 = (csCtl1Read & (~DCOFTRIM)) | (dcoFreqTrim<<4);
            }
        }

        if(newDcoDelta < bestDcoDelta)         // Record DCOTAP closest to 256
        {
            csCtl0Copy = csCtl0Read;
            csCtl1Copy = csCtl1Read;
            bestDcoDelta = newDcoDelta;
        }

    }while(endLoop == 0);                      // Poll until endLoop == 1

    CSCTL0 = csCtl0Copy;                       // Reload locked DCOTAP
    CSCTL1 = csCtl1Copy;                       // Reload locked DCOFTRIM
    while(CSCTL7 & (FLLUNLOCK0 | FLLUNLOCK1)); // Poll until FLL is locked
}


#if defined(__TI_COMPILER_VERSION__) || defined(__IAR_SYSTEMS_ICC__)
#pragma vector=USCI_A1_VECTOR
__interrupt void USCI_A1_ISR(void)
#elif defined(__GNUC__)
void __attribute__ ((interrupt(USCI_A0_VECTOR))) USCI_A1_ISR (void)
#else
#error Compiler not supported!
#endif
{
  switch(__even_in_range(UCA1IV,USCI_UART_UCTXCPTIFG))
  {
    case USCI_NONE: break;
    case USCI_UART_UCRXIFG:
      while(!(UCA1IFG&UCTXIFG));
      UCA1TXBUF = UCA1RXBUF;
      __no_operation();
      break;
    case USCI_UART_UCTXIFG: break;
    case USCI_UART_UCSTTIFG: break;
    case USCI_UART_UCTXCPTIFG: break;
    default: break;
  }
}

void Init_GPIO()
{
    P1DIR = 0xFF; P2DIR = 0xFF;
    P1REN = 0xFF; P2REN = 0xFF;
    P1OUT = 0x00; P2OUT = 0x00;
}

void init_clocks() {
    __bis_SR_register(SCG0);                 // disable FLL
     CSCTL3 |= SELREF__REFOCLK;               // Set REFO as FLL reference source
     CSCTL1 = DCOFTRIMEN_1 | DCOFTRIM0 | DCOFTRIM1 | DCORSEL_3;// DCOFTRIM=3, DCO Range = 8MHz
     CSCTL2 = FLLD_0 + 243;                  // DCODIV = 8MHz
     __delay_cycles(3);
     __bic_SR_register(SCG0);                // enable FLL
     CSCTL4 = SELMS__DCOCLKDIV | SELA__REFOCLK; // set default REFO(~32768Hz) as ACLK source, ACLK = 32768Hz
}
