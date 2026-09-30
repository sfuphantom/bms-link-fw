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
#include "sci.h"
#include "gio.h"



//----------------------------------------------------------------------------------------------------
void enableAllInterrupts(){
    vimInit();
    _enable_IRQ();
    _enable_interrupt_();
}
//----------------------------------------------------------------------------------------------------
void restart_BMS_system(){
    init_fans();
    ClearAllFaults();
    initLink();
}
//----------------------------------------------------------------------------------------------------

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
    initPhantomTimers();
    rtiStartCounter(rtiCOUNTER_BLOCK0);

    init_BMS_Faults();
    initBatteryData();
    Charger_Init();

    restart_BMS_system();

}
//----------------------------------------------------------------------------------------------------

void DoNothing(){
    //Nothing
}
bool returnTrue(){
    return TRUE;
}
bool returnFalse(){
    return FALSE;
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
//    uint32_t now = 0;
    Task_t currentTask;

    for(i=0; i<NumOfTasks; i++){
        //if(AnyFaults()){
//                break;
//        }

        currentTask = AllTasks[i];

        if(! currentTask.AllowFunction()){
            continue;
        }

//        now = getNow_tick();
//        if(hasTimeElapsed(currentTask.LastDone, currentTask.Period)){
        if(!hasPeriodExpired_rti(currentTask.Period, &currentTask.LastDone)){
//        if(now - currentTask.LastDone < currentTask.Period){
            continue;
        }
        currentTask.RoutineFunction();
//        currentTask.LastDone = getNow_tick();

    }
}
//----------------------------------------------------------------------------------------------------
Task_t BMSTask[] =   {
                         {CellVoltageControlRoutine     ,CELL_VOLTAGE_CONTROL_TASK_PERIOD_TICK, 0, is_SPI_free},
//                                     {MonitorCellTempRoutine    ,CELL_VOLTAGE_CONTROL_TASK_PERIOD_US, 0, is_SPI_free},
                         {SlaveFlagsRoutine             ,SLAVE_FLAG_CHECK_TASK_PERIOD_TICK, 0, is_SPI_free},
//                                     {MeasureCellResistanceRoutine  ,CELL_RESISTANCE_MONITOR_TASK_PERIOD_TICK, 0, is_SPI_free},
//                                     {MonitorFullBatteryDataRoutine ,FULL_BATTERY_MONITOR_TASK_PERIOD_TICK, 0, is_SPI_free},
                         {keepSlavesAwakeRoutine        ,KEEP_SLAVES_AWAKE_TASK_PERIOD_TICK, 0, is_SPI_free},
                         {SendChargerControlsRoutine    ,SEND_CHARGER_CONTROL_TASK_PERIOD_TICK, 0, returnTrue},
                         {SendDataRoutine_Serial        ,SEND_DATA_SERIAL_TASK_PERIOD_TICK,     0, returnTrue},
                        };

const uint8_t NUMBER_OF_TASKS = sizeof(BMSTask)/sizeof(Task_t);

void Do_BMS_Tasks(){
    TaskSuperLoop(BMSTask, NUMBER_OF_TASKS);
}

// //----------------------------------------------------------------------------------------------------
//
