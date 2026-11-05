/*
 * IMD.h
 *
 *  Created on: May 21, 2020
 *      Author: Sumreen Rattan
 */
#ifndef SOURCE_IMD_H_
#define SOURCE_IMD_H_

#include "sys_common.h"
#include "ecap.h"
#include "system.h"
#include "IMD_hardware.h"       // IMD limits, windows and datasheet values live here
#include "PhantomTimers.h"

//---------------------------------------------------------------------------------------------------------
#define IMD_ECAP_REG            ecapREG6
#define ECAP_COUNTS_PER_SEC     ((float)(VCLK4_FREQ * 1E6))
#define VALID_FLAG_MASK         0x00FF
#define VALID_FLAG_VAL          0x000E
//---------------------------------------------------------------------------------------------------------
bool readIMDSignal(float * freq, float * duty);
void IMD_init();
//---------------------------------------------------------------------------------------------------------
IMDStateEnum IMD_CalcState(const float duty, const float freq);
uint8_t      IMD_CalcIsolation(const float duty);   // percent, ISOLATION_UNKNOWN if duty is out of range
float        IMD_CalcResistance(const float duty);  // kOhm
bool         IMD_HasFault(const float duty, const float freq, const float resistance, const float isolation, const IMDStateEnum state);
//---------------------------------------------------------------------------------------------------------
#endif /* SOURCE_IMD_H_ */
