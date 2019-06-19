#include <UART.h>
#include <SPI.h>
#include <MAX11254.h>
/**
 * main.c
 */
void initClockTo16MHz();

uint8_t string[20] = {0};
int main(void)
{

    // P4.2 -> RSTB
    // P4.1 -> RDYB
    P4DIR |= BIT2;
    P4OUT |= BIT2;

    SPI_init(true, false, true, 40, true);

    uint8_t ctrl2;
    __enable_interrupt();
    MAX11254_config_ctrl2(false, 0x02);
    MAX11254_read_register(CTRL2_ADDR, &ctrl2, 1);

    return 0;
}


void initClockTo16MHz()
{

    //UCSCTL3 |= SELREF_2;                      // Set DCO FLL reference = REFO
    UCSCTL4 = SELA_2 + SELS_3 + SELM_3;       // Set ACLK = REFO, SMCLK = DCO
    UCSCTL5 = DIVS_2;                         // Divide SMCLK by 4, so SMCLK = 4MHz
    __bis_SR_register(SCG0);                  // Disable the FLL control loop
    UCSCTL0 = 0x0000;                         // Set lowest possible DCOx, MODx
    UCSCTL1 = DCORSEL_5;                      // Select DCO range 16MHz operation
    UCSCTL2 = FLLD_0 + 487;                   // Set DCO Multiplier for 16MHz
                                              // (N + 1) * FLLRef = Fdco
                                              // (487 + 1) * 32768 = 16MHz
                                              // Set FLL Div = fDCOCLK
    __bic_SR_register(SCG0);                  // Enable the FLL control loop

    // Worst-case settling time for the DCO when the DCO range bits have been
    // changed is n x 32 x 32 x f_MCLK / f_FLL_reference. See UCS chapter in 5xx
    // UG for optimization.
    // 32 x 32 x 16 MHz / 32,768 Hz = 500000 = MCLK cycles for DCO to settle
    __delay_cycles(500000);//
    // Loop until XT1,XT2 & DCO fault flag is cleared
    do
    {
        UCSCTL7 &= ~(XT2OFFG + XT1LFOFFG + DCOFFG); // Clear XT2,XT1,DCO fault flags
        SFRIFG1 &= ~OFIFG;                          // Clear fault flags
    }while (SFRIFG1&OFIFG);                         // Test oscillator fault flag

}
