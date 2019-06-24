/*
 * SAC.c
 *
 *  Created on: Jun 21, 2019
 *      Author: Sherwin
 */

#include "SAC.h"

void SAC_init() {
    GPIO_setAsPeripheralModuleFunctionInputPin(
            GPIO_PORT_OA2N,
            GPIO_PIN_OA2N,
            GPIO_FUNCTION_OA2N
    );

    GPIO_setAsPeripheralModuleFunctionInputPin(
            GPIO_PORT_OA2O,
            GPIO_PIN_OA2O,
            GPIO_FUNCTION_OA2O
    );

  //  PMM_selectVoltageReference(PMM_REFVSEL_2_5V);
  //  PMM_enableInternalReference();

 //   while (PMM_getBufferedBandgapVoltageStatus() != PMM_REFBG_READY);

    SAC_DAC_selectRefVoltage(SAC2_BASE, SAC_DAC_PRIMARY_REFERENCE);
    SAC_DAC_setData(SAC2_BASE, 0);
    SAC_DAC_enable(SAC2_BASE);

    SAC_OA_init(
            SAC2_BASE,
            SAC_OA_POSITIVE_INPUT_SOURCE_DAC,
            SAC_OA_NEGATIVE_INPUT_SOURCE_EXTERNAL
    );
    SAC_OA_selectPowerMode(SAC2_BASE, SAC_OA_POWER_MODE_LOW_SPEED_LOW_POWER);
    SAC_OA_enable(SAC2_BASE);
    SAC_enable(SAC2_BASE);
}

void SAC_set_dac(uint16_t val) {
    SAC_DAC_setData(SAC2_BASE, val);
}

