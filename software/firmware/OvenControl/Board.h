/*
 * Board.h
 *
 *  Created on: Jun 18, 2019
 *      Author: Sherwin
 */


#ifndef BOARD_H_
#define BOARD_H_

#define MCLK_FREQ                       24000000
#define SMCLK_FREQ                      12000000
#define ACLK_FREQ                       32768

#define GPIO_PORT_UCA1TXD               GPIO_PORT_P4
#define GPIO_PIN_UCA1TXD                GPIO_PIN3
#define GPIO_FUNCTION_UCA1TXD           GPIO_PRIMARY_MODULE_FUNCTION

#define GPIO_PORT_UCA1RXD               GPIO_PORT_P4
#define GPIO_PIN_UCA1RXD                GPIO_PIN2
#define GPIO_FUNCTION_UCA1RXD           GPIO_PRIMARY_MODULE_FUNCTION

#define GPIO_PORT_UCB1CLK               GPIO_PORT_P4
#define GPIO_PIN_UCB1CLK                GPIO_PIN5
#define GPIO_FUNCTION_UCB1CLK           GPIO_PRIMARY_MODULE_FUNCTION

#define GPIO_PORT_UCB1SIMO              GPIO_PORT_P4
#define GPIO_PIN_UCB1SIMO               GPIO_PIN6
#define GPIO_FUNCTION_UCB1SIMO          GPIO_PRIMARY_MODULE_FUNCTION

#define GPIO_PORT_UCB1SOMI              GPIO_PORT_P4
#define GPIO_PIN_UCB1SOMI               GPIO_PIN7
#define GPIO_FUNCTION_UCB1SOMI          GPIO_PRIMARY_MODULE_FUNCTION

#define GPIO_PORT_CSB                   GPIO_PORT_P4
#define GPIO_PIN_CSB                    GPIO_PIN4

#define GPIO_PORT_RSTB                  GPIO_PORT_P6
#define GPIO_PIN_RSTB                   GPIO_PIN5

#define GPIO_PORT_RDYB                  GPIO_PORT_P3
#define GPIO_PIN_RDYB                   GPIO_PIN6

#define GPIO_PORT_OA2N                  GPIO_PORT_P3
#define GPIO_PIN_OA2N                   GPIO_PIN2
#define GPIO_FUNCTION_OA2N              GPIO_TERNARY_MODULE_FUNCTION

#define GPIO_PORT_OA2O                  GPIO_PORT_P3
#define GPIO_PIN_OA2O                   GPIO_PIN1
#define GPIO_FUNCTION_OA2O              GPIO_TERNARY_MODULE_FUNCTION

typedef enum commandStateEnum {
    COMMAND_IDLE,
    COMMAND_RECEIVE,
} commandState;

typedef enum readStateEnum {
    READ_IDLE,
    READ_INITIATE_READ,
    READ_GET_DATA,
    READ_INITIATE_SELF_CAL,
    READ_GET_SELF_CAL,
    READ_INITIATE_SYS_CAL,
    READ_GET_SYS_CAL
} readState;


extern commandState command_state;
#endif /* BOARD_H_ */
