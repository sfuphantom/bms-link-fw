/*
 * IMD.h
 *
 *  Created on: Apr 25, 2026
 *      Author: joeyw
 */

#ifndef BMS_MASTER_INCLUDE_IMD_H_
#define BMS_MASTER_INCLUDE_IMD_H_

#include "het.h"
#include "gio.h"
#include "sys_vim.h"
#include "rti.h"
#include "sci.h"
#include "stdlib.h" // contains ltoa
#include "sys_common.h"
#include "ecap.h"
#include "etpwm.h"

extern float frequency;
extern float duty_cycle;
extern float period;

// IMD State Enum
// message names are taken from IMD Datasheet
typedef enum{
    Short_Circuit,
    Normal_Condition,
    Undervoltage_Condition,
    Speed_Start_Measurement_Good, // related to start-up
    Speed_Start_Measurement_Bad, // related to start-up
    Device_Error,
    Connection_Fault_Earth,
    Bad_Info,
    Undefined_Fault
} IMDStateEnum;

// Isolation State Enum
typedef enum{
    Normal, // When Duty Cycle 5-10%
    Normal_75, // Usually when Duty Cycle 10-30%
    Normal_50, // Usually when Duty Cycle 30-60%
    Normal_25, // Usually when Duty Cycle 60-90%
    Isolation_Failure, //When Duty Cycle is 90-95%
    Unknown // Duty Cycle is outside of range (5-95%)
}IsolationStateEnum;

// IMDData Struct which contains the status of the IMD and Isolation
typedef struct{
    IMDStateEnum IMDState;
    IsolationStateEnum IsolationState;
}IMDData_t;


void initalizeIMD();
void updateIsolationState(unsigned int duty_value);
void updateIMDData();
void updateIMDState(unsigned int freq_value, unsigned int duty_value);
IMDData_t getIMDData();
void checkIMDState(); // to implement

#endif /* BMS_MASTER_INCLUDE_IMD_H_ */
