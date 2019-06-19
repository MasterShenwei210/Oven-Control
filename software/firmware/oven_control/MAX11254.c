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
    int i;
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

void MAX11254_config_ctrl2(bool lpm, uint8_t pga_gain) {
    uint8_t data = BIT5 + BIT3 + (pga_gain & 0x07);
    if (lpm) data += BIT4;
    MAX11254_write_register(CTRL2_ADDR, &data, 1);
}

void MAX11254_enter_seq_mode1(bool continuous, uint8_t mux_delay) {
    uint8_t write_data[3];
    write_data[0] = BIT1;
    MAX11254_write_register(SEQ_ADDR, write_data, 1);

    write_data[0] = BIT5;
    if (continuous) write_data[0] += BIT0;
    MAX11254_write_register(CTRL1_ADDR, write_data, 1);

    write_data[0] = mux_delay;
    write_data[1] = 0;
    MAX11254_write_register(DELAY_ADDR, write_data, 2);
}

void MAX11254_enter_seq_mode2(bool continuous, uint8_t mux_delay) {
    uint8_t write_data[3];
    write_data[0] = BIT3 + BIT1;
    if (continuous) write_data[0] += BIT0;
    MAX11254_write_register(CTRL1_ADDR, write_data, 1);

    write_data[0] = BIT3 + BIT1 + BIT0;
    MAX11254_write_register(SEQ_ADDR, write_data, 1);

    write_data[0] = (6 << 2);
    write_data[1] = (5 << 2) + BIT1;
    write_data[2] = (4 << 2) + BIT1;
    MAX11254_write_register(CHMAP1_ADDR, write_data, 3);

    write_data[0] = (3 << 2) + BIT1;
    write_data[1] = (2 << 2) + BIT1;
    write_data[2] = (1 << 2) + BIT1;
    MAX11254_write_register(CHMAP0_ADDR, write_data, 3);

    write_data[0] = mux_delay;
    write_data[1] = 0;
    MAX11254_write_register(DELAY_ADDR, write_data, 2);
}

void MAX11254_select_channel(uint8_t channel) {
    uint8_t seq_val;
    MAX11254_read_register(SEQ_ADDR, &seq_val, 1);
    seq_val &= ~(BIT7 + BIT6 + BIT5);
    seq_val |= (channel << 5);
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

    int32_t bin_value = 0;
    bin_value|= (raw_data[0] << 16) + (raw_data[1] << 8) + raw_data[2];
    if (bin_value | 0x00800000) bin_value |= 0xFF000000;
    return bin_value;
}

double MAX11254_convert_data(int32_t bin_value, double pga_gain) {
    double data_out = (double) bin_value;
    double conversion = VREF / 8388608 / pga_gain;
    data_out *= conversion;
    return data_out;
}


