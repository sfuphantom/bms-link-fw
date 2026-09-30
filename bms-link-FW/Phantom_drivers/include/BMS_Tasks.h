/*
 * BMS_Tasks.h
 *
 *  Created on: Jun 19, 2026
 *      Author: tanjo
 */

#ifndef PHANTOM_DRIVERS_INCLUDE_BMS_TASKS_H_
#define PHANTOM_DRIVERS_INCLUDE_BMS_TASKS_H_
//---------------------------------------------------------------------------------------------------------
#include "PhantomTimers.h"

#define CELL_VOLTAGE_CONTROL_TASK_PERIOD_TICK         MIN_SEC_MS_US_TICK_2_TICK(0,0,50,0,0)
#define CELL_TEMP_MONITOR_TASK_PERIOD_TICK            MIN_SEC_MS_US_TICK_2_TICK(0,0,50,0,0)
#define SLAVE_FLAG_CHECK_TASK_PERIOD_TICK             MIN_SEC_MS_US_TICK_2_TICK(0,0,100,0,0)
#define FULL_BATTERY_MONITOR_TASK_PERIOD_TICK         MIN_SEC_MS_US_TICK_2_TICK(0,0,200,0,0)
#define CELL_RESISTANCE_MONITOR_TASK_PERIOD_TICK      MIN_SEC_MS_US_TICK_2_TICK(1,0,0,0,0)
#define SEND_CHARGER_CONTROL_TASK_PERIOD_TICK         MIN_SEC_MS_US_TICK_2_TICK(0,0,500,0,0)
#define KEEP_SLAVES_AWAKE_TASK_PERIOD_TICK            MIN_SEC_MS_US_TICK_2_TICK(0,0,500,0,0)

#define SEND_DATA_SERIAL_TASK_PERIOD_TICK             MIN_SEC_MS_US_TICK_2_TICK(0,30,0,0,0)
//---------------------------------------------------------------------------------------------------------
typedef struct {
    void (*RoutineFunction)(void);
    uint64_t Period;
    uint64_t LastDone;

//    void (*TimeFailFunction)(void);
    bool (*AllowFunction)(void);

}Task_t;
//---------------------------------------------------------------------------------------------------------

enum{CellVoltageControl_ID, MonitorCellTemp_ID, SlaveFlagsCheck_ID, MonitorBatterySOC_ID, CommunacateWithOthers_ID}Task_ID;
//---------------------------------------------------------------------------------------------------------
void init_BMS_system();
void restart_BMS_system();
void DoNothing();
//---------------------------------------------------------------------------------------------------------
bool CellVoltageControlTask();
bool MonitorCellTempTask();
bool SlaveFlagsCheckTasks();
//---------------------------------------------------------------------------------------------------------
void Do_BMS_Tasks();


#endif /* PHANTOM_DRIVERS_INCLUDE_BMS_TASKS_H_ */
