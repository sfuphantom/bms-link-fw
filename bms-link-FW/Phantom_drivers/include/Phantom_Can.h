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
////#define BMS2VCU_FAULT_MSG_LEN_BITS          48
////#define BMS2VCU_BATTERY_VOLT_MSG_LEN_BITS   16
//#define VCU2BMS_DATA_MSG_LEN_BITS           32
//
//#define BMS2CHARGER_DATA_MSG_LEN_BITS       40
//#define CHARGER2BMS_DATA_MSG_LEN_BITS       40
////----------------------------------------------------------------------------------------
//#ifdef BMS2VCU_FAULT_MSG_LEN_BITS
//    #define BMS2VCU_FAULT_MSG_LEN_BYTES          Bit2Bytes_Ceil(BMS2VCU_FAULT_MSG_LEN_BITS)
//#else
//    #define BMS2VCU_FAULT_MSG_LEN_BYTES          6
//#endif
//
//#ifdef BMS2VCU_BATTERY_VOLT_MSG_LEN_BITS
//    #define BMS2VCU_BATTERY_VOLT_MSG_LEN_BYTES   Bit2Bytes_Ceil(BMS2VCU_BATTERY_VOLT_MSG_LEN_BITS)
//#else
//    #define BMS2VCU_BATTERY_VOLT_MSG_LEN_BYTES   6
//#endif
//
//#ifdef VCU2BMS_DATA_MSG_LEN_BITS
//    #define VCU2BMS_DATA_MSG_LEN_BYTES           Bit2Bytes_Ceil(VCU2BMS_DATA_MSG_LEN_BITS)
//#else
//    #define BMS2VCU_FAULT_MSG_LEN_BYTES          4
//#endif
//
//#ifdef BMS2CHARGER_DATA_MSG_LEN_BITS
//    #define BMS2CHARGER_DATA_MSG_LEN_BYTES       Bit2Bytes_Ceil(BMS2CHARGER_DATA_MSG_LEN_BITS)
//#else
//    #define BMS2VCU_FAULT_MSG_LEN_BYTES          5
//#endif
//
//#ifdef CHARGER2BMS_DATA_MSG_LEN_BITS
//    #define CHARGER2BMS_DATA_MSG_LEN_BYTES       Bit2Bytes_Ceil(CHARGER2BMS_DATA_MSG_LEN_BITS)
//
//#else
//    #define CHARGER2BMS_DATA_MSG_LEN_BYTES          5
//#endif

//----------------------------------------------------------------------------------------
uint32_t can_transmit_data(const phantomCanMsgBox msgbox, const void * const data, const uint8_t len);
uint32_t can_receive_data(const phantomCanMsgBox msgbox, void * const data, const uint8_t len);
//---------------------------------------------------------


#endif /* PHANTOM_DRIVERS_INCLUDE_PHANTOM_CAN_H_ */
