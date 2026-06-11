/*
 * SlaveCommunation_TaskAndRoutines.h
 *
 *  Created on: Jun 5, 2026
 *      Author: tanjo
 */

#ifndef PHANTOM_DRIVERS_INCLUDE_SLAVECOMMUNATION_TASKANDROUTINES_H
#define PHANTOM_DRIVERS_INCLUDE_SLAVECOMMUNATION_TASKANDROUTINES_H
#include <stdint.h>
#include <stdbool.h>
#include "spi.h"
#include "SlaveCommunication_Drivers.h"
#include "SlaveCommunation_Functions.h"


#define NUMBER_OF_CLEAR_CMDS 3
////////////////////////////////////////////////////////////////////
//struct BatteryData_struct {
//  bool Charging;
//
//  uint16_t CellTemp[NUMBER_OF_GPIOS];
//  uint16_t CellVolt[NUMBER_OF_CELLS];
//
//  uint16_t RefVolt2nd[NUMBER_OF_REF_2ND];
//
//  uint32_t TotalSumVolt;
//  uint16_t Avg_SOC;
//
//  uint8_t IMD_Fault;
//  uint32_t statusFaults;
//};
 //---------------------------------------------------------------------------------------------------------
 void ClearSlaveRegs();
 void initLink();
 //---------------------------------------------------------------------------------------------------------
 bool MeasureCellVoltageRoutine(uint16_t* VoltDataOut);

 bool MeasureGPIOVoltageRoutine(uint16_t* GPIO_DataOut, uint16_t* Ref2nd_DataOut);

 bool BalanceCellsRoutine(const uint16_t* VoltInData);

 uint32_t ReadStatAndGetFlagsRoutine();
 void CellVoltageControlTask();
 void SlaveFlagsTask();
 void MonitorCellTempTask();

#endif /* PHANTOM_DRIVERS_INCLUDE_SLAVECOMMUNATION_TASKANDROUTINES_H_ */
