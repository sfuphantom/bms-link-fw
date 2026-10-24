/*
 * GetBatteryVoltage.c
 *
 *  Created on: Sep 14, 2026
 *      Author: tanjo
 */

#include <math.h>
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

// Must come before the #if USE_HV_BOARD checks below
#include "GetBatteryVoltage.h"

#if USE_HV_BOARD
#include "spi.h"
#include "spi_helpers.h"
#else
#include "SlaveCommunation_Hardware.h"
#include "SlaveCommunation_Functions.h"
#endif

#include "FullBattery_Hardware.h"
#include "BatteryData.h"
#include "PhantomHelpers.h"



#if USE_HV_BOARD
#warning HV board path: scale/offset are placeholders and the hardware had a known problem. Verify before use.

// One 16-clock read. Returns false if the frame is clearly not a valid ADS7044 frame.
static bool ReadHvCode(int16_t *code_out){
    setCS(LOW, HV_CS_PIN_ID);
    const uint16_t frame = SPI_SR2Link_WORD_FAST(SPI_DUMMY_DATA_WORD);
    setCS(HIGH, HV_CS_PIN_ID);

    if(frame == SPI_DUMMY_DATA_WORD)        return false;   // nothing came back
    if(frame & ADS7044_FRAME_ZERO_MASK)     return false;   // padding bits should be 0

    const uint16_t code = (frame >> ADS7044_CODE_SHIFT) & ADS7044_CODE_MASK;
    *code_out = (int16_t)((code ^ ADS7044_SIGN_BIT) - ADS7044_SIGN_BIT);    // sign extend 12 -> 16
    return true;
}
#endif

bool MeasureBatteryVoltage(float *volts_out){
    if(volts_out == NULL) return false;

#if USE_HV_BOARD
    int16_t code;
    if(!ReadHvCode(&code)) return false;

    *volts_out = (code * HV_ADC2VOLTS_SCALE) + HV_ADC2VOLTS_OFFSET;
    return true;
#else
    const StatusReg_t *status = GetStatusRegData();
    if(status == NULL) return false;

    float SC[NUMBER_OF_SLAVE_BOARDS_TOTAL];
    getStatus_SC(SC);
    const float sum = arr_f_sum(SC, NUMBER_OF_SLAVE_BOARDS_TOTAL);
    *volts_out = sum;
    return true;
#endif
}

uint16_t CompressVoltage(const float volts){
    if(isnan(volts)) return 0;

    const float scaled = roundf(volts * VOLT2SAVED_SCALE);
    if(scaled <= 0.0f)              return 0;
    if(scaled >= (float)UINT16_MAX) return UINT16_MAX;
    return (uint16_t)scaled;
}

float GetBatteryVoltage(void){
    return Pack_GetVolt();
}

uint16_t GetBatteryVoltage_u16(void){
    return CompressVoltage(Pack_GetVolt());
}
