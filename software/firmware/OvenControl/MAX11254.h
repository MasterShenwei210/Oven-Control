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

#define SPI_CLK_FREQ        4000000
#define VREF                2.5
#define COMMAND_BYTE        0x80

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

/************UTILITY*****************/
#define PGA_GAIN__1         0x00
#define PGA_GAIN__2         0x01
#define PGA_GAIN__4         0x02
#define PGA_GAIN__8         0x03
#define PGA_GAIN__16        0x04
#define PGA_GAIN__32        0x05
#define PGA_GAIN__64        0x06
#define PGA_GAIN__128       0x07

#define CHANNEL_0            0
#define CHANNEL_1            1
#define CHANNEL_2            2
#define CHANNEL_3            3
#define CHANNEL_4            4
#define CHANNEL_5            5

#define RATE_0              0x00
#define RATE_1              0x01
#define RATE_2              0x02
#define RATE_3              0x03
#define RATE_4              0x04
#define RATE_5              0x05
#define RATE_6              0x06
#define RATE_7              0x07
#define RATE_8              0x08
#define RATE_9              0x09
#define RATE_10             0x0A
#define RATE_11             0x0B
#define RATE_12             0x0C
#define RATE_13             0x0D
#define RATE_14             0x0E
#define RATE_15             0x0F

/***********************************/

/***********GENERAL UTILITY************/
void MAX11254_write_register(uint8_t addr, uint8_t *data, int byte_count);
void MAX11254_read_register(uint8_t addr, uint8_t *data, int byte_count);
void MAX11254_init_gpio();
bool MAX11254_data_ready();

/***********APPLICATION SPECIFIC*******/
void MAX11254_config_ctrl2(bool lpm, uint8_t pga_gain);
void MAX11254_internal_calibration();
void MAX11254_enter_seq_mode1(bool continuous, uint8_t mux_delay,  uint8_t rate);
void MAX11254_enter_seq_mode2(bool continuous, uint8_t mux_delay);
void MAX11254_select_channel(uint8_t channel, uint8_t pga, uint8_t delay);
void MAX11254_conversion_command(uint8_t rate);

int32_t MAX11254_get_raw_data(int channel);
int32_t MAX11254_mode1_read(uint8_t channel, uint8_t rate);


#endif /* MAX11254_H_ */
