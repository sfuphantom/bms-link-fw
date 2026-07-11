/*
 * BMS_Tasks.c
 *
 *  Created on: Jun 19, 2026
 *      Author: tanjo
 */
//----------------------------------------------------------------------------------------------------
#include "rti.h"
#include "spi.h"
#include "can.h"
#include "het.h"
#include "ecap.h"
#include "sys_vim.h"
#include "sys_core.h"


//#include "sys_common.h"
#include "system.h"

#include "SlaveCommunication_Routines.h"
#include "BatteryData.h"
#include "Charger.h"
#include "PhantomHelpers.h"
#include "Fault_handler.h"
#include "BMS_Tasks.h"
#include "Fans.h"

//----------------------------------------------------------------------------------------------------
void enableAllInterrupts(){
    vimInit();
    _enable_IRQ();
    _enable_interrupt_();
}

void init_BMS_system(){
    systemInit();

    enableAllInterrupts();

    spiInit();
    rtiInit();
    canInit();
    gioInit();
    hetInit();
    ecapInit();

    init_fans();

    init_BMS_Faults();

    initLink();

    initBatteryData();
    initCharger();
}
//----------------------------------------------------------------------------------------------------
void DoNothing(){

}
//----------------------------------------------------------------------------------------------------
void TaskSuperLoop(struct Task_t AllTasks[], uint8_t NumOfTasks, void (*ElseFunction)(void)){
    int i;
    uint32_t now = getNow_us();
    struct Task_t currentTask;

    for(i=0; i<NumOfTasks; i++){
        currentTask = AllTasks[i];
        if(now - currentTask.LastDone > currentTask.Period){
            currentTask.RoutineFunction();
            currentTask.LastDone = now;
        }
        else{
            ElseFunction();
        }
    }
}
//----------------------------------------------------------------------------------------------------
struct Task_t SlaveComunationSubTask[] =    {
                                             {CellVoltageControlRoutine,CELL_VOLTAGE_CONTROL_TASK_PERIOD_MS,0},
                                             {MonitorCellTempRoutine,CELL_VOLTAGE_CONTROL_TASK_PERIOD_MS,0},
                                             {SlaveFlagsRoutine,CELL_VOLTAGE_CONTROL_TASK_PERIOD_MS,0},
                                            };
void SlaveComunation_Task(){
    TaskSuperLoop(SlaveComunationSubTask, 3, keepAwake);
}

struct Task_t AllTask[] =   {
                               {SlaveComunation_Task,0,0},
                            };

//----------------------------------------------------------------------------------------------------

 bool CellVoltageControlTask(){
//     const uint32_t CS_pollWaitings = waitSPIFree(1000);
     if(!rtiTimerExpired(CellVoltageControl_ID, CELL_VOLTAGE_CONTROL_TASK_PERIOD_MS, 0)){
         keepAwake();
         return false;
     }

     CellVoltageControlRoutine();
     return true;
 }
 bool MonitorCellTempTask(){
//     const uint32_t CS_pollWaitings = waitSPIFree(1000);
     if(!rtiTimerExpired(MonitorCellTemp_ID, CELL_TEMP_MONITOR_TASK_PERIOD_MS, 0)){
         keepAwake();
         return false;
     }

     MonitorCellTempRoutine();
     return true;
 }
 bool SlaveFlagsCheckTasks(){
 //     const uint32_t CS_pollWaitings = waitSPIFree(1000);
      if(!rtiTimerExpired(SlaveFlagsCheck_ID, SLAVE_FLAG_CHECK_TASK_PERIOD_MS, 0)){
          keepAwake();
          return false;
      }
      SlaveFlagsRoutine();
      return true;
  }
 //----------------------------------------------------------------------------------------------------

