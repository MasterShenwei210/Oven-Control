/*
 * MAX11254.c
 *
 *  Created on: Jun 12, 2019
 *      Author: Sherwin
 */

#include <MAX11254.h>

uint8_t transmit_buffer[5];
uint8_t receive_buffer[5];


void MAX11254_write_register(uint8_t addr, uint8_t *data, int byte_count) {
    transmit_buffer[0] = COMMAND_BYTE + BIT6 + (addr << 1);
    SPI_copy_array(data, &transmit_buffer[1], byte_count);
    while(SPI_transfering);
    SPI_transfer_bytes(transmit_buffer, receive_buffer, byte_count + 1, false);
}

void MAX11254_read_register(uint8_t addr, uint8_t *data, int byte_count) {
    transmit_buffer[0] = COMMAND_BYTE + BIT6 + (addr << 1) + BIT0;
    while(SPI_transfering);
    SPI_transfer_bytes(transmit_buffer, receive_buffer, byte_count + 1, true);
    SPI_copy_array(&receive_buffer[1], data, byte_count);
}

void MAX11254_init_gpio() {
    GPIO_setAsOutputPin(GPIO_PORT_RSTB, GPIO_PIN_RSTB);
    GPIO_setOutputLowOnPin(GPIO_PORT_RSTB, GPIO_PIN_RSTB);
    __delay_cycles(2400000);
    GPIO_setOutputHighOnPin(GPIO_PORT_RSTB, GPIO_PIN_RSTB);

    GPIO_setAsInputPin(GPIO_PORT_RDYB, GPIO_PIN_RDYB);
}

bool MAX11254_data_ready() {
    return GPIO_getInputPinValue(GPIO_PORT_RDYB, GPIO_PIN_RDYB) == GPIO_INPUT_PIN_LOW;
}

void MAX11254_config_ctrl2(bool lpm, uint8_t pga_gain) {
    uint8_t data = BIT5 + BIT3 + (pga_gain & 0x07);
    if (lpm) data += BIT4;
    MAX11254_write_register(CTRL2_ADDR, &data, 1);
}

void MAX11254_enter_seq_mode1(bool continuous, uint8_t mux_delay, uint8_t rate) {
    uint8_t write_data[3];

    write_data[0] = BIT1;
    MAX11254_write_register(SEQ_ADDR, write_data, 1);

    write_data[0] = BIT5;
    if (continuous) write_data[0] += BIT0;
    MAX11254_write_register(CTRL1_ADDR, write_data, 1);

    write_data[0] = mux_delay;
    write_data[1] = 0;
    MAX11254_write_register(DELAY_ADDR, write_data, 2);

    MAX11254_conversion_command(rate);
    __delay_cycles(MCLK_FREQ);
    MAX11254_get_raw_data(CHANNEL_0);
}


void MAX11254_select_channel(uint8_t channel) {
    transmit_buffer[0] = COMMAND_BYTE | BIT4;
    while (SPI_transfering);
    SPI_transfer_bytes(transmit_buffer, receive_buffer, 1, false);

    uint8_t seq_val;
    MAX11254_read_register(SEQ_ADDR, &seq_val, 1);
    seq_val &= ~(BIT7 + BIT6 + BIT5);
    seq_val |= ((channel & 0x07) << 5);
    MAX11254_write_register(SEQ_ADDR, &seq_val, 1);
}

void MAX11254_conversion_command(uint8_t rate) {
    transmit_buffer[0] = COMMAND_BYTE + BIT5 + BIT4 + (rate & 0x0F);
    while(SPI_transfering);
    SPI_transfer_bytes(transmit_buffer, receive_buffer, 1, false);
}

int32_t MAX11254_get_raw_data(int channel) {
    uint8_t raw_data[3];
    switch(channel) {
    case 0:
        MAX11254_read_register(DATA0_ADDR, raw_data, 3);
        break;
    case 1:
        MAX11254_read_register(DATA1_ADDR, raw_data, 3);
        break;
    case 2:
        MAX11254_read_register(DATA2_ADDR, raw_data, 3);
        break;
    case 3:
        MAX11254_read_register(DATA3_ADDR, raw_data, 3);
        break;
    case 4:
        MAX11254_read_register(DATA4_ADDR, raw_data, 3);
        break;
    case 5:
        MAX11254_read_register(DATA5_ADDR, raw_data, 3);
        break;
    }

    uint32_t bin_value = 0;
    bin_value |= ((uint32_t) raw_data[0]) << 16;
    bin_value |= ((uint32_t) raw_data[1]) << 8;
    bin_value |= (uint32_t) raw_data[2];
    if (bin_value | 0x00800000) bin_value |= 0xFF000000;
    return (int32_t) bin_value;
}

int32_t MAX11254_mode1_read(uint8_t channel, uint8_t rate) {
    MAX11254_select_channel(channel);
    MAX11254_conversion_command(rate);
    bool data_rdy = MAX11254_data_ready();

    while(!data_rdy) {
        data_rdy = MAX11254_data_ready();
        __no_operation();
    }
    int32_t data = MAX11254_get_raw_data(channel);
    return data;
}



