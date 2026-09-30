/*
 * Phantom_Can.h
 *
 *  Created on: Jul 1, 2026
 *      Author: tanjo
 */

#ifndef PHANTOM_DRIVERS_INCLUDE_PHANTOM_CAN_H_
#define PHANTOM_DRIVERS_INCLUDE_PHANTOM_CAN_H_

#include "can.h"
#include "reg_can.h"


//----------------------------------------------------------------------------------------
#define CAN_NODE_REG canREG1
#define DEFULT_CAN_MSG_SIZE_BYTE 8

//----------------------------------------------------------------------------------------

typedef enum {
        BMS2ALL_FAULT        = canMESSAGE_BOX1,
        BMS2VCU_DATA         = canMESSAGE_BOX3,
        VCU2BMS_DATA         = canMESSAGE_BOX4,

        BMS2CHARGER_DATA     = canMESSAGE_BOX6,
        CHARGER2BMS_DATA     = canMESSAGE_BOX7,
        }phantomCanMsgBox;

//----------------------------------------------------------------------------------------

extern void BMS2ALL_FAULT_FullRoutine();
extern void BMS2VCU_Data_FullRoutine();
extern void VCU2BMS_DATA_FullRoutine();
extern void BMS2CHARGER_DATA_FullRoutine();
extern void CHARGER2BMS_DATA_FullRoutine();

//----------------------------------------------------------------------------------------
bool can_transmit_data(const phantomCanMsgBox msgbox, const void * const data, const uint8_t len);
bool can_receive_data(const phantomCanMsgBox msgbox, void * const data, const uint8_t len);
//---------------------------------------------------------


#endif /* PHANTOM_DRIVERS_INCLUDE_PHANTOM_CAN_H_ */
