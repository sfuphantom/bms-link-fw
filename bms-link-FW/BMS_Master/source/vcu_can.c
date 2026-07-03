/*
 * vcu_can.c
 *
 *  Created on: Apr 24, 2026
 *      Author: joeyw
 */

#include "vcu_can.h"
#include <string.h>

static void transmit_data(uint32_t msgbox, uint8_t val){
    uint8_t buf[8];
    uint8_t fill = (val != 0) ? 0xFF : 0x00;
    memset(buf, fill, sizeof(buf));
    canTransmit(VCU_CAN_NODE, msgbox, buf);
}

void VCU_TransmitStatus(const BMSStatusFlags_t *f){
    transmit_data(BMS_FAULT, f->bms_fault);
    transmit_data(BMS_IMD, f->imd_fault);
    transmit_data(BMS_HV, f->hv_active);
}

