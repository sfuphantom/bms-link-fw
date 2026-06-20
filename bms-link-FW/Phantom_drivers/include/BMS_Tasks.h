/*
 * BMS_Tasks.h
 *
 *  Created on: Jun 19, 2026
 *      Author: tanjo
 */

#ifndef PHANTOM_DRIVERS_INCLUDE_BMS_TASKS_H_
#define PHANTOM_DRIVERS_INCLUDE_BMS_TASKS_H_
//---------------------------------------------------------------------------------------------------------
#define CELL_VOLTAGE_CONTROL_TASK_PERIOD_MS 2000
#define CELL_TEMP_CONTROL_TASK_PERIOD_MS 2000
#define SLAVE_FLAG_CHECK_TASK_PERIOD_MS 2000
//---------------------------------------------------------------------------------------------------------
enum{CellVoltageControl_ID, MonitorCellTemp_ID, SlaveFlagsCheck_ID, MonitorBatterySOC_ID, CommunacateWithOthers_ID}Task_ID;
//---------------------------------------------------------------------------------------------------------
void init_BMS_system();
//---------------------------------------------------------------------------------------------------------
void CellVoltageControlTask();
void MonitorCellTempTask();
void SlaveFlagsCheckTasks();
//---------------------------------------------------------------------------------------------------------





#endif /* PHANTOM_DRIVERS_INCLUDE_BMS_TASKS_H_ */
