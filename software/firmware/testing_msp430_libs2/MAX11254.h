/*
 * MAX11254.h
 *
 *  Created on: Jun 12, 2019
 *      Author: Sherwin
 */

#ifndef MAX11254_H_
#define MAX11254_H_

#include <SPI.h>
#include <float.h>

#define SPI_CLK_DIV         4000000 / 500000
#define VREF                2.5
#define COMMAND_BYTE        BIT7

/**********REGISTER MAP***************/
#define STAT_ADDR           0x00
#define CTRL1_ADDR          0x01
#define CTRL2_ADDR          0x02
#define CTRL3_ADDR          0x03
#define GPIO_CTRL_ADDR      0x04
#define DELAY_ADDR          0x05
#define CHMAP1_ADDR         0x06
#define CHMAP0_ADDR         0x07
#define SEQ_ADDR            0x08
#define GPIO_DIR_ADDR       0x09
#define SOC_ADDR            0x0A
#define SGC_ADDR            0x0B
#define SCOC_ADDR           0x0C
#define SCGC_ADDR           0x0D
#define DATA0_ADDR          0x0E
#define DATA1_ADDR          0x0F
#define DATA2_ADDR          0x10
#define DATA3_ADDR          0x11
#define DATA4_ADDR          0x12
#define DATA5_ADDR          0x13
/*************************************/


/***********GENERAL UTILITY************/
void MAX11254_write_register(uint8_t addr, uint8_t *data, int byte_count);
void MAX11254_read_register(uint8_t addr, uint8_t *data, int byte_count);


/***********APPLICATION SPECIFIC*******/
void MAX11254_config_ctrl2(bool lpm, uint8_t pga_gain);
void MAX11254_internal_calibration();
void MAX11254_enter_seq_mode1(bool continuous, uint8_t mux_delay);
void MAX11254_enter_seq_mode2(bool continuous, uint8_t mux_delay);
void MAX11254_select_channel(uint8_t channel);
void MAX11254_conversion_command(uint8_t rate);

int32_t MAX11254_get_raw_data(int channel);
double MAX11254_convert_data(int32_t bin_value, double pga_gain);

#endif /* MAX11254_H_ */
