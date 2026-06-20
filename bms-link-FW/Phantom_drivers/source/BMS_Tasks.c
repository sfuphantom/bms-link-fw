/*
 * BMS_Tasks.c
 *
 *  Created on: Jun 19, 2026
 *      Author: tanjo
 */
//----------------------------------------------------------------------------------------------------
#include "rti.h"
#include "spi.h"
#include "sys_common.h"
#include "system.h"

#include "SlaveCommunication_Routines.h"

#include "BatteryData.h"
#include "Charger.h"

#include "PhantomHelpers.h"

#include "BMS_Tasks.h"

//----------------------------------------------------------------------------------------------------
void init_BMS_system(){
    _enable_IRQ();
    spiInit();
    rtiInit();
    initLink();

    initBatteryData();

}
//----------------------------------------------------------------------------------------------------
 void CellVoltageControlTask(){
//     const uint32_t CS_pollWaitings = waitSPIFree(1000);
     if(!rtiTimerExpired(CellVoltageControl_ID, CELL_VOLTAGE_CONTROL_TASK_PERIOD_MS, 0)){
         keepAwake();
         return;
     }

     CellVoltageControlRoutine();
 }
 void MonitorCellTempTask(){
//     const uint32_t CS_pollWaitings = waitSPIFree(1000);
     if(!rtiTimerExpired(MonitorCellTemp_ID, CELL_TEMP_CONTROL_TASK_PERIOD_MS, 0)){
         keepAwake();
         return;
     }

     MonitorCellTempRoutine();
 }
 void SlaveFlagsCheckTasks(){
 //     const uint32_t CS_pollWaitings = waitSPIFree(1000);
      if(!rtiTimerExpired(SlaveFlagsCheck_ID, SLAVE_FLAG_CHECK_TASK_PERIOD_MS, 0)){
          keepAwake();
          return;
      }
      SlaveFlagsRoutine();
  }
 //----------------------------------------------------------------------------------------------------

