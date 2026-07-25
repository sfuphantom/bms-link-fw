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
#define CELL_VOLTAGE_CONTROL_TASK_PERIOD_MS     50
#define CELL_VOLTAGE_CONTROL_TASK_PERIOD_US     0

#define CELL_TEMP_MONITOR_TASK_PERIOD_SEC       0
#define CELL_TEMP_MONITOR_TASK_PERIOD_MS        100
#define CELL_TEMP_MONITOR_TASK_PERIOD_US        0

#define SLAVE_FLAG_CHECK_TASK_PERIOD_SEC        0
#define SLAVE_FLAG_CHECK_TASK_PERIOD_MS         100
#define SLAVE_FLAG_CHECK_TASK_PERIOD_US         0

#define BATTERY_SOC_MONITOR_TASK_PERIOD_SEC     0
#define BATTERY_SOC_MONITOR_TASK_PERIOD_MS      200
#define BATTERY_SOC_MONITOR_TASK_PERIOD_US      0

#define HV_VOLTAGE_MONITOR_TASK_PERIOD_SEC      0
#define HV_VOLTAGE_MONITOR_TASK_PERIOD_MS       200
#define HV_VOLTAGE_MONITOR_TASK_PERIOD_US       0

//---------------------------------------------------------------------------------------------------------
typedef struct {
    void (*RoutineFunction)(void);
    uint32_t Period;
    uint32_t LastDone;

    void (*TimeFailFunction)(void);
    bool (*AllowFunction)(void);

}Task_t;
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
void Do_BMS_Tasks();

#endif /* PHANTOM_DRIVERS_INCLUDE_BMS_TASKS_H_ */
