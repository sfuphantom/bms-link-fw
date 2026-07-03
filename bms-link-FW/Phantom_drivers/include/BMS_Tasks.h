/*
 * BMS_Tasks.h
 *
 *  Created on: Jun 19, 2026
 *      Author: tanjo
 */

#ifndef PHANTOM_DRIVERS_INCLUDE_BMS_TASKS_H_
#define PHANTOM_DRIVERS_INCLUDE_BMS_TASKS_H_
//---------------------------------------------------------------------------------------------------------
#define CELL_VOLTAGE_CONTROL_TASK_PERIOD_SEC    0
#define CELL_VOLTAGE_CONTROL_TASK_PERIOD_MS     1000
#define CELL_VOLTAGE_CONTROL_TASK_PERIOD_US     0

#define CELL_TEMP_MONITOR_TASK_PERIOD_SEC       0
#define CELL_TEMP_MONITOR_TASK_PERIOD_MS        1000
#define CELL_TEMP_MONITOR_TASK_PERIOD_US        0

#define SLAVE_FLAG_CHECK_TASK_PERIOD_SEC        0
#define SLAVE_FLAG_CHECK_TASK_PERIOD_MS         1000
#define SLAVE_FLAG_CHECK_TASK_PERIOD_US         0

#define BATTERY_SOC_MONITOR_TASK_PERIOD_SEC     0
#define BATTERY_SOC_MONITOR_TASK_PERIOD_MS      2000
#define BATTERY_SOC_MONITOR_TASK_PERIOD_US      0

//---------------------------------------------------------------------------------------------------------
struct Task_t{
    void (*RoutineFunction)(void);
    uint32_t Period;
    uint32_t LastDone;
};
//---------------------------------------------------------------------------------------------------------

enum{CellVoltageControl_ID, MonitorCellTemp_ID, SlaveFlagsCheck_ID, MonitorBatterySOC_ID, CommunacateWithOthers_ID}Task_ID;
//---------------------------------------------------------------------------------------------------------
void init_BMS_system();
void DoNothing();
//---------------------------------------------------------------------------------------------------------
bool CellVoltageControlTask();
bool MonitorCellTempTask();
bool SlaveFlagsCheckTasks();
//---------------------------------------------------------------------------------------------------------


#endif /* PHANTOM_DRIVERS_INCLUDE_BMS_TASKS_H_ */
