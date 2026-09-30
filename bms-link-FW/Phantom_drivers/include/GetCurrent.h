/*
 * Current.h
 *
 *  Created on: Sep 13, 2026
 *      Author: tanjo
 */

#ifndef PHANTOM_DRIVERS_INCLUDE_GETCURRENT_H_
#define PHANTOM_DRIVERS_INCLUDE_GETCURRENT_H_


#include <stdint.h>
#include <stdbool.h>

#define CURRENT2SAVED_SCALE (500.0f)

uint64_t GetLastMeasure_tick();
void MeasureAndSaveCurrent();
float GetCurrent_f();


#endif /* PHANTOM_DRIVERS_INCLUDE_GETCURRENT_H_ */
