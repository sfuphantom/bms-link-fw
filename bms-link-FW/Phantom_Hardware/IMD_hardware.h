/*
 * IMD_hardware.h
 *
 *  Created on: Oct 25, 2026
 *      Author: tanjo
 */

#ifndef PHANTOM_HARDWARE_IMD_HARDWARE_H_
#define PHANTOM_HARDWARE_IMD_HARDWARE_H_

#include "hal_stdtypes.h"

//--------------------------------------------------------------------------
// IMD message names are taken from the IMD datasheet
typedef enum {
    Normal_Condition,
    Short_Circuit,
    Undervoltage_Condition,
    Speed_Start_Measurement_Good,
    Speed_Start_Measurement_Bad,
    Device_Error,
    Connection_Fault_Earth,
    Bad_Info,
    Undefined_Fault
} IMDStateEnum;

#define ISOLATION_UNKNOWN   0xFFu
//---------------------------------------------------------------------------------------------------------

#endif /* PHANTOM_HARDWARE_IMD_HARDWARE_H_ */
