#include "cs.h"
#include "gpio.h"
#include "wdt_a.h"
#include "pmm.h"

#include "Board.h"
#include "UART.h"
#include "SPI.h"
#include "MAX11254.h"
#include "SAC.h"

#define MCLK_FREQ_KHZ 24000
#define MCLK_FLLREF_RATIO 732   // 32.768kHz * 732 = 24MHz

#define CHANNEL_COUNT   4

uint8_t string[30] = {0};
uint8_t string_len = 0;

int32_t adc_data[CHANNEL_COUNT] = {0};
int32_t adc_self_cal[CHANNEL_COUNT];
uint8_t adc_rate = RATE_2;
uint8_t adc_channel = 0;
uint8_t adc_delay = 64;
uint8_t adc_pga = 0;
uint8_t self_calib = 0;


commandState command_state = COMMAND_IDLE;
readState read_state = READ_INITIATE_READ;

bool enter_lpm;
bool stream_adc = false;
bool calib_start_flg = false;

void init_clocks();

void initiate_adc_read();
void get_adc_data();
//void update_adc_data();

void parse_and_execute_command();
void read_adc_command();
void set_rate_command();
void set_dac_command();
void set_pga_command();
void set_delay_command();
void stream_adc_command();

bool strings_equal(uint8_t *str1, uint8_t *str2, int length);
void itoa(int32_t value, uint8_t* result, int base);
uint32_t string_to_int(uint8_t *str, uint8_t eos, uint8_t *invalid);

int main(void) {
    WDT_A_hold(WDT_A_BASE);
    PMM_unlockLPM5();
    init_clocks();
    UART_init();
    SPI_init(true, false, true, SPI_CLK_FREQ, true);
    MAX11254_init_gpio();
    SAC_init();
    GPIO_selectInterruptEdge(
            GPIO_PORT_RDYB,
            GPIO_PIN_RDYB,
            GPIO_HIGH_TO_LOW_TRANSITION
    );

    MAX11254_config_ctrl2(false, adc_pga);
    MAX11254_enter_seq_mode1(false, adc_delay, adc_rate);
    GPIO_clearInterrupt(GPIO_PORT_RDYB, GPIO_PIN_RDYB);
    GPIO_enableInterrupt(GPIO_PORT_RDYB, GPIO_PIN_RDYB);
    __enable_interrupt();
    for (;;) {
        enter_lpm = true;
        switch (command_state) {
            case COMMAND_IDLE: break;
            case COMMAND_RECEIVE:
                if (UART_receive_buffer[UART_receive_count-1] == '\r' ||
                        UART_receive_count == UART_RECEIVE_BUF_SIZE) {
                    parse_and_execute_command();
                }
                break;
            default: break;
        }

        switch (read_state) {
            case READ_IDLE: break;
            case READ_INITIATE_READ:
                initiate_adc_read();
                break;
            case READ_GET_DATA:
                get_adc_data();
                enter_lpm = false;
                break;
            default: break;
        }

        if (enter_lpm) {
            __bis_SR_register(LPM0_bits | GIE);
        }
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

void parse_and_execute_command() {
    UART_transmit_bytes("\n", 1, true);
    string_len = UART_receive_count;
    UART_receive_bytes(string);
    if (strings_equal("read_adc\r", string, 9)) {
        read_adc_command();

    } else if (strings_equal("set_rate ", string, 9)) {
        set_rate_command();

    } else if (strings_equal("set_dac ", string, 8)) {
        set_dac_command();

    } else if (strings_equal("set_pga ", string, 8)) {
        set_pga_command();

    } else if (strings_equal("set_delay ", string, 10)) {
        set_delay_command();

    } else if (strings_equal("stream_adc ", string, 11)) {
        stream_adc_command();

    } else {
        UART_transmit_bytes("Unrecognized Command: ", 22, true);
        UART_transmit_bytes(string, string_len, true);
        UART_transmit_bytes("\r\n", 2, true);
    }
}

void set_rate_command() {
    uint8_t msd = string[9];
    uint8_t lsd = string[10];
    msd -= 48;
    lsd -= 48;
    bool valid_arg = false;
    if (msd < 9) {
        if (string[10] == '\r') {
            valid_arg = true;
            adc_rate = msd;
        } else if (lsd < 9 && string[11] == '\r') {
            uint8_t val = msd*10 + lsd;
            if (val < 16) {
                valid_arg = true;
                adc_rate = val;
            }
        }
    }
    if (!valid_arg) {
        UART_transmit_bytes("Invalid Arguments for ", 22, true);
        UART_transmit_bytes("set_rate Command:\r\n\tArg: ", 25, true);
        UART_transmit_bytes(&string[9], string_len - 9, true);
        UART_transmit_bytes("\r\n", 2, true);
    } else {
        UART_transmit_bytes("ADC Set to Rate ", 16, true);
        int count = 3;
        if (string[10] == '\r') {
            count = 2;
        }
        UART_transmit_bytes(&string[9], count, true);
        UART_transmit_bytes("\r\n", 2, true);
    }
}

void read_adc_command() {
    int i;
    UART_transmit_bytes("ADC Readings:\r\n", 15, true);
    for (i = 0; i < CHANNEL_COUNT; i++) {
        UART_transmit_bytes("\tChannel ", 9, true);
        itoa((int32_t) i, string, 10);
        UART_transmit_bytes(string, 1, true);
        UART_transmit_bytes(": ", 2, true);

        itoa(adc_data[i], string, 10);
        int k = 0;
        while (string[k] != '\0') k++;
        UART_transmit_bytes(string, k, true);
        UART_transmit_bytes("\r\n", 2, true);
    }
    command_state = COMMAND_IDLE;
}

void set_dac_command() {
    uint8_t invalid_command = 0;
    uint32_t val = string_to_int(&string[8], '\r', &invalid_command);
    if (val > 4095) invalid_command = true;
    if (invalid_command) {
        UART_transmit_bytes("Invalid Arguments for ", 22, true);
        UART_transmit_bytes("set_dac Command:\r\n\tArg: ", 24, true);
        UART_transmit_bytes(&string[8], string_len - 8, true);
        UART_transmit_bytes("\r\n", 2, true);
    } else {
        SAC_set_dac(val);
        UART_transmit_bytes("DAC Set to ", 11, true);
        UART_transmit_bytes(&string[8], string_len - 9, true);
        UART_transmit_bytes(" LSB\r\n", 6, true);
    }
}

void set_pga_command() {
    uint8_t val = string[8] - 48;
    if (val < 8 && string[9] == '\r') {
        adc_pga = val;
        UART_transmit_bytes("PGA Set to Gain ", 16, true);
        UART_transmit_bytes(&string[8], 1, true);
        UART_transmit_bytes("\r\n", 2, true);

    } else {
        UART_transmit_bytes("Invalid Argument for ", 22, true);
        UART_transmit_bytes("set_pga Command:\r\n\tArg: ", 24, true);
        UART_transmit_bytes(&string[8], string_len - 8, true);
        UART_transmit_bytes("\r\n", 2, true);
    }
}

void set_delay_command() {
    uint8_t invalid = 0;
    uint32_t val = string_to_int(&string[10], '\r', &invalid);
    if (val < 256 && !invalid) {
        adc_delay = val;
        itoa((int32_t) val*4, string, 10);
        UART_transmit_bytes("Delay Set to ", 13, true);
        int k = 0;
        while (string[k] != '\0') k++;
        UART_transmit_bytes(string, k, true);
        UART_transmit_bytes(" usec\r\n", 7, true);

    } else {
        UART_transmit_bytes("Invalid Argument for ", 22, true);
        UART_transmit_bytes("set_delay Command:\r\n\tArg: ", 25, true);
        UART_transmit_bytes(&string[10], string_len - 9, true);
        UART_transmit_bytes("\r\n", 2, true);
    }
}

void stream_adc_command() {
    if (strings_equal("on\r", &string[11], 3)) stream_adc = true;
    else if (strings_equal("off\r", &string[11], 4)) stream_adc = false;
}

void initiate_adc_read() {
    MAX11254_select_channel(adc_channel, adc_pga, adc_delay);
    if (calib_start_flag) self_calib = CHANNEL_COUNT;
    MAX11254_conversion_command(adc_rate);
    read_state = READ_IDLE;
}

void get_adc_data() {
    adc_data[adc_channel] = MAX11254_get_raw_data(adc_channel);
    if (adc_channel == CHANNEL_COUNT - 1) {
        adc_channel = CHANNEL_0;
        if (stream_adc) read_adc_command();
    } else {
        adc_channel++;
    }
    read_state = READ_INITIATE_READ;
}

void update_adc_data() {
    uint8_t i;
    for (i = 0; i < 5; i++) {
        adc_data[i] = MAX11254_mode1_read(i, adc_rate);
    }
}


void itoa(int32_t value, uint8_t* result, int base) {
  // check that the base if valid
  if (base < 2 || base > 36) { *result = '\0';}

  uint8_t* ptr = result, *ptr1 = result, tmp_char;
  int tmp_value;

  do {
    tmp_value = value;
    value /= base;
    *ptr++ = "zyxwvutsrqponmlkjihgfedcba9876543210123456789abcdefghijklmnopqrstuvwxyz" [35 + (tmp_value - value * base)];
  } while ( value );

  // Apply negative sign
  if (tmp_value < 0) *ptr++ = '-';
  *ptr-- = '\0';
  while(ptr1 < ptr) {
    tmp_char = *ptr;
    *ptr--= *ptr1;
    *ptr1++ = tmp_char;
  }

}

bool strings_equal(uint8_t *str1, uint8_t *str2, int length) {
    int i;
    for (i = 0; i < length; i++) {
        if (str1[i] != str2[i]) return false;
    }
    return true;
}

uint32_t string_to_int(uint8_t *str, uint8_t eos, uint8_t *invalid) {
    uint32_t result = 0;
    *invalid = 0;
    int k = 0;
    while (str[k] != eos) {
        if (k > 5) {
            *invalid = 1;
            return 0;
        }
        uint8_t converted = str[k] - 48;
        if (converted > 9) {
            *invalid = 1;
            return 0;
        }
        result *= 10;
        result += converted;
        k++;
    }
    return result;
}

#pragma vector=PORT3_VECTOR
__interrupt void port3_isr(void) {
    switch(__even_in_range(P3IV, BIT6)) {
        case 0x0E:
            read_state = READ_GET_DATA;
            GPIO_clearInterrupt(GPIO_PORT_RDYB, GPIO_PIN_RDYB);
            __bic_SR_register_on_exit(LPM0_bits);
            break;
    }
}

