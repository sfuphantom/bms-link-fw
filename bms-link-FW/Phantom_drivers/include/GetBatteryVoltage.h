/*
 * GetBatteryVoltage.h
 *
 *  Created on: Sep 13, 2026
 *      Author: tanjo
 */

#ifndef PHANTOM_DRIVERS_INCLUDE_GETBATTERYVOLTAGE_H_
#define PHANTOM_DRIVERS_INCLUDE_GETBATTERYVOLTAGE_H_

#include <stdint.h>
#include <stdbool.h>

// 0 = sum of slave-board SC voltages (current hardware)
// 1 = HV board, ADS7044 over SPI (future). Can be set from the build flags.
#ifndef USE_HV_BOARD
#define USE_HV_BOARD            0
#endif

// Compressed format: unsigned, 0.01 V per count (max 655.35 V)
#define VOLT2SAVED_SCALE        (100.0f)

// Fault limits, in compressed units (0.01 V): 410 V and 260 V
#define HV_OVER_VOLT_FLAG       41000
#define HV_UNDER_VOLT_FLAG      26000

#if USE_HV_BOARD
// ADS7044 frame (16 clocks, MSB first): [15:14]=0  [13:2]=12-bit twos-complement code  [1:0]=0
#define HV_ADQ_TIME_ns          200
#define ADS7044_CODE_SHIFT      2u
#define ADS7044_CODE_MASK       0x0FFFu
#define ADS7044_SIGN_BIT        0x0800u
#define ADS7044_FRAME_ZERO_MASK 0xC003u     // bits that must read 0 in a valid frame

// volts = code * SCALE + OFFSET
#define HV_ADC2VOLTS_SCALE      3.3f        // TODO: placeholder, volts per count is not known yet
#define HV_ADC2VOLTS_OFFSET     3.3f        // TODO: placeholder, not known yet
#endif

// Raw measurement. Returns false (and leaves *volts_out untouched) if the reading is invalid.
bool MeasureBatteryVoltage(float *volts_out);

// Last stored pack voltage
float GetBatteryVoltage(void);          // volts
uint16_t GetBatteryVoltage_u16(void);   // compressed, for transmission

// float volts -> compressed (rounded, saturated to 0..65535, NaN -> 0)
uint16_t CompressVoltage(float volts);

#endif /* PHANTOM_DRIVERS_INCLUDE_GETBATTERYVOLTAGE_H_ */
