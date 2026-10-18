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
#include "SlaveCommunation_Hardware.h"

 //---------------------------------------------------------------------------------------------------------
 #define USE_ANILOG_GPIO        FALSE

 //---------------------------------------------------------------------------------------------------------
 void MeasureCellVoltageSubRoutine();
 void MeasureGPIOVoltageSubRoutine();
 void BalanceCellsSubRoutine();
 void ReadStatAndGetFlagsSubRoutine();
 //---------------------------------------------------------------------------------------------------------
 void CellVoltageControlRoutine();
 void SetCellBalancersRoutine();
 void SlaveFlagsRoutine();
 void MonitorCellTempRoutine();
 void MeasureBatteryVoltageRoutine();
 void MeasureBatteryCurrentRoutine();
 void MeasureCellResistanceRoutine();
 void keepSlavesAwakeRoutine();
 void SendChargerControlsRoutine();

 void SendDataRoutine_Serial();
 //---------------------------------------------------------------------------------------------------------

#endif /* PHANTOM_DRIVERS_INCLUDE_SLAVECOMMUNATION_TASKANDROUTINES_H_ */
