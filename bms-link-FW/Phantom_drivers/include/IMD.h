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

#include "BatteryData.h"
#include "Fault_handler.h"

//---------------------------------------------------------------------------------------------------------
#define IMD_ECAP_REG            ecapREG6
#define ECAP_COUNTS_PER_SEC     ((float)(VCLK4_FREQ * 1E6))
#define VALID_FLAG_MASK         0x00FF
#define VALID_FLAG_VAL          0x000E

//---------------------------------------------------------------------------------------------------------
// Fault limits
// TODO: check these against EV.7.6.3 in the 2024 V1 rulebook (500 ohm/V)
#define MIN_IMD_RESISTANCE_KOHM     1E3f    // datasheet equation gives kOhm
#define MIN_IMD_ISOLATION_PERCENT   75.0f
#define MAX_IMD_ISOLATION_PERCENT   100.0f
#define MAX_IMD_DUTY_PERCENT        50.0f
#define IMD_RESISTANCE_MAX_KOHM     1E6f    // "infinite" resistance (duty <= 5%) is capped to this

//---------------------------------------------------------------------------------------------------------
IMDStateEnum IMD_CalcState(const float duty, const float freq);
uint8_t      IMD_CalcIsolation(const float duty);   // percent, ISOLATION_UNKNOWN if duty is out of range
float        IMD_CalcResistance(const float duty);  // kOhm
IMDData_t    IMD_CalcData(const float duty, const float freq);
bool         IMD_HasFault(const IMDData_t* const data, const IMDStateEnum state);

#endif /* SOURCE_IMD_H_ */
