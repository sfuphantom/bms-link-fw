/*
 * vcu_can.h
 *
 *  Created on: Apr 24, 2026
 *      Author: joeyw
 */

#ifndef BMS_MASTER_INCLUDE_VCU_CAN_H_
#define BMS_MASTER_INCLUDE_VCU_CAN_H_

#include "can.h"
#include "stdint.h"

#define VCU_CAN_NODE canREG2

#define BMS_FAULT canMESSAGE_BOX1
#define BMS_IMD canMESSAGE_BOX2
#define BMS_HV canMESSAGE_BOX3

typedef struct{
    uint8_t bms_fault;
    uint8_t imd_fault;
    uint8_t hv_active;
} BMSStatusFlags_t;

void VCU_TransmitStatus(const BMSStatusFlags_t *f);

#endif /* BMS_MASTER_INCLUDE_VCU_CAN_H_ */
