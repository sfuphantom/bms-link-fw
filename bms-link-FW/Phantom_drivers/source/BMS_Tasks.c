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
#include "SlaveCommunation_Functions.h"
#include "BMS_Routines.h"
#include "BatteryData.h"
#include "Charger.h"

#include "PhantomHelpers.h"
#include "PhantomTimers.h"

#include "Fault_handler.h"
#include "BMS_Tasks.h"
#include "Fans.h"
#include "spi_helpers.h"
#include "HV_data.h"
#include "sci.h"
#include "gio.h"



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
    canInit();
    gioInit();
    hetInit();
    ecapInit();
    sciInit();


    rtiInit();
    rtiStartCounter(rtiCOUNTER_BLOCK0);

    init_fans();

    init_BMS_Faults();

    initLink();

    initBatteryData();
    initCharger();
}
//----------------------------------------------------------------------------------------------------
void DoNothing(){
    //Nothing
}
bool returnTrue(){
    return TRUE;
}
//----------------------------------------------------------------------------------------------------
//void TaskSuperLoop(struct Task_t AllTasks[], uint8_t NumOfTasks, void (*ElseFunction)(void)){
//    int i;
//    uint32_t now = getNow_us();
//    struct Task_t currentTask;
//
//    for(i=0; i<NumOfTasks; i++){
//        currentTask = AllTasks[i];
//        if(now - currentTask.LastDone > currentTask.Period){
//            currentTask.RoutineFunction();
//            currentTask.LastDone = now;
//        }
//        else{
//            ElseFunction();
//        }
//    }
//}

void TaskSuperLoop(Task_t AllTasks[], uint8_t NumOfTasks){
    int i;
    uint32_t now = 0;
    Task_t currentTask;

    for(i=0; i<NumOfTasks; i++){
        currentTask = AllTasks[i];

        if(currentTask.AllowFunction()){
            continue;
        }

        now = getNow_us();
        if(now - currentTask.LastDone > currentTask.Period){
            currentTask.LastDone = now;
            currentTask.RoutineFunction();
        }
        else{
            currentTask.TimeFailFunction();
        }
    }
}
//----------------------------------------------------------------------------------------------------
Task_t SlaveComunationSubTask[] =   {
//                                     {CellVoltageControlRoutine ,CELL_VOLTAGE_CONTROL_TASK_PERIOD_MS,0, keepAwake, SPI_busy},
//                                     {MonitorCellTempRoutine    ,CELL_VOLTAGE_CONTROL_TASK_PERIOD_MS,0, keepAwake, SPI_busy},
//                                     {SlaveFlagsRoutine         ,CELL_VOLTAGE_CONTROL_TASK_PERIOD_MS,0, keepAwake, SPI_busy},
                                     {HV_DataRoutine            ,CELL_VOLTAGE_CONTROL_TASK_PERIOD_MS,0, DoNothing, SPI_busy}
                                    };

const uint8_t NUMBER_OF_TASKS = sizeof(SlaveComunationSubTask)/sizeof(Task_t);

void Do_BMS_Tasks(){
    TaskSuperLoop(SlaveComunationSubTask, NUMBER_OF_TASKS);
}

//struct Task_t AllTask[] =   {
//                               {SlaveComunation_Task,0,0},
//                            };

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

