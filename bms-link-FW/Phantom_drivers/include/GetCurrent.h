/*
 * GetCurrent.h
 *
 *  Created on: Sep 13, 2026
 *      Author: tanjo
 */

#ifndef PHANTOM_DRIVERS_INCLUDE_GETCURRENT_H_
#define PHANTOM_DRIVERS_INCLUDE_GETCURRENT_H_

#include <stdint.h>
#include <stdbool.h>

// 0 = estimate from charger / inverter / fan currents (current hardware)
// 1 = real pack current sensor (future). Can be set from the build flags.
#ifndef USE_CURRENT_SENSOR
#define USE_CURRENT_SENSOR      0
#endif

// Compressed format: signed, 0.002 A per count (range about +/-65.5 A)
#define CURRENT2SAVED_SCALE     (500.0f)

#if USE_CURRENT_SENSOR
float ReadCurrentSensor_A(void);        // TODO: write this (driver for the real sensor)
#endif

// Raw measurement, amps. Positive = charging, negative = discharging.
float MeasureCurrent(void);

// Last stored pack current
float GetCurrent_f(void);               // amps
int16_t GetCurrent_i16(void);           // compressed, for transmission

// float amps -> compressed (rounded, saturated to int16, NaN -> 0)
int16_t CompressCurrent(float amps);

#endif /* PHANTOM_DRIVERS_INCLUDE_GETCURRENT_H_ */
