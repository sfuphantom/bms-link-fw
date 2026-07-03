/*
 * Phantom_Can_Config.h
 *
 *  Created on: Jun 30, 2026
 *      Author: tanjo
 */

#ifndef PHANTOM_HARDWARE_PHANTOM_CAN_CONFIG_H_
#define PHANTOM_HARDWARE_PHANTOM_CAN_CONFIG_H_

#include "can.h"
#include "stdint.h"

enum {
        BMS2VCU_BMS_FAULT    = canMESSAGE_BOX1,
        BMS2VCU_IMD_FAULT    = canMESSAGE_BOX2,
        BMS2VCU_BATTERY_VOLT = canMESSAGE_BOX3,

        }canMESSAGE;



#endif /* PHANTOM_HARDWARE_PHANTOM_CAN_CONFIG_H_ */
