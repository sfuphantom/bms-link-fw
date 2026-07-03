///*
// * charger_can.h
// *
// *  Created on: Feb 28, 2026
// *      Author: joeyw
// */
//
//#ifndef BMS_MASTER_INCLUDE_CHARGER_CAN_H_
//#define BMS_MASTER_INCLUDE_CHARGER_CAN_H_
//
//#include "can.h"
//#include "stdint.h"
//
//#define CHARGER_TX_BOX canMESSAGE_BOX1
//#define CHARGER_RX_BOX canMESSAGE_BOX2
//
//#define CHARGER_FLAG_HW_FAULT (1U << 0)
//#define CHARGER_FLAG_OVERTEMP (1U << 1)
//#define CHARGER_FLAG_INPUT_FAULT (1U << 2)
//#define CHARGER_FLAG_NO_BATTERY (1U << 3)
//#define CHARGER_FLAG_COMM_TIMEOUT (1U << 4)
//
//typedef struct {
//    uint16 max_voltage_dV;
//    uint16 max_current_dA;
//    uint8 charge_enable;
//} ChargerCmd_t;
//
//typedef struct {
//    uint16 output_voltage_dV;
//    uint16 output_current_dA;
//    uint8 status_flags;
//} ChargerStatus_t;
//
//uint32 Charger_SendCmd(const ChargerCmd_t *cmd);
//uint32 Charger_GetStatus(ChargerStatus_t *status);
//
//void Charger_Update(ChargerCmd_t *cmd, ChargerStatus_t *status);
//
//#endif
//
//
