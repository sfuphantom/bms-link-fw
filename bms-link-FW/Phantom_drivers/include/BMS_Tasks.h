/*
 * BMS_Tasks.h
 *
 * Task periods live here, the task table itself is in BMS_Tasks.c.
 * Period 0 means "every pass of the main loop".
 */
#ifndef PHANTOM_DRIVERS_INCLUDE_BMS_TASKS_H_
#define PHANTOM_DRIVERS_INCLUDE_BMS_TASKS_H_

#include <stdbool.h>
#include "PhantomTimers.h"

//                                                         MIN, SEC, MS, US, TICK
#warning The first argument of MAKE_TICK is MINUTES: these 2 mean 2 minutes between cell voltage reads. Was that the intent?
#define CELL_VOLTAGE_CONTROL_TASK_PERIOD_TICK       MAKE_TICK(2,  0, 0, 0, 0)
#define CELL_BALANCER_CONTROL_TASK_PERIOD_TICK      MAKE_TICK(2,  0, 0, 0, 0)
#define SLAVE_FLAG_CHECK_TASK_PERIOD_TICK           MAKE_TICK(2,  0, 0, 0, 0)
#define CELL_RESISTANCE_MONITOR_TASK_PERIOD_TICK    MAKE_TICK(50, 0, 0, 0, 0)
#define PACK_VOLTAGE_TASK_PERIOD_TICK               MAKE_TICK(0,  0, 0, 0, 0)
#warning SEND_CHARGER_CONTROL period is 0 (every loop), that sends CAN as fast as the loop runs. Set a real period.
#define SEND_CHARGER_CONTROL_TASK_PERIOD_TICK       MAKE_TICK(0,  0, 0, 0, 0)

//---------------------------------------------------------------------------------------------------------
void init_BMS_system(void);
void restart_BMS_system(void);      // clears faults, restarts fans, slave link and task timers
void Do_BMS_Tasks(void);            // call from the main loop

#endif /* PHANTOM_DRIVERS_INCLUDE_BMS_TASKS_H_ */
