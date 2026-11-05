/*
 * BMS_Tasks.h
 *
 * Task periods live here, the task table itself is in BMS_Tasks.c.
 * Period 0 means "every pass of the main loop".
 */
#ifndef PHANTOM_DRIVERS_INCLUDE_BMS_TASKS_H_
#define PHANTOM_DRIVERS_INCLUDE_BMS_TASKS_H_

#include <stdint.h>
#include <stdbool.h>
#include "PhantomTimers.h"
#include "BMS_Routines.h"

//                                                              MIN, SEC, MS, US, TICK
#define CELL_VOLTAGE_MEASURE_TASK_PERIOD_TICK       (MAKE_TICK(0,  0,  50, 0, 0))
#define CELL_BALANCER_CONTROL_TASK_PERIOD_TICK      (MAKE_TICK(0,  0, 100, 0, 0))
#define SLAVE_FLAG_CHECK_TASK_PERIOD_TICK           (MAKE_TICK(0,  0,  50, 0, 0))
#define PACK_VOLTAGE_TASK_PERIOD_TICK               (MAKE_TICK(0,  0,  10, 0, 0))
#define SEND_CHARGER_CONTROL_TASK_PERIOD_TICK       (MAKE_TICK(0,  0, 100, 0, 0))
#warning The next 3 periods are guesses, pick real ones.
#define IMD_MONITOR_TASK_PERIOD_TICK                (MAKE_TICK(0,  0, 100, 0, 0))
#define CELL_TEMP_MONITOR_TASK_PERIOD_TICK          (MAKE_TICK(0,  0, 500, 0, 0))
#define KEEP_SLAVES_AWAKE_TASK_PERIOD_TICK          (MAKE_TICK(0,  0, 500, 0, 0))
#define DEBUG_TASK_PERIOD_TICK                      0       // every loop
#warning PhantomTimers.h: MS_TO_TICKS multiplies by 1000000U, it should be 1000U (1 ms = 1000 us). Not used here, but it is wrong.

//---------------------------------------------------------------------------------------------------------
// The order is the run order AND the index into the table in BMS_Tasks.c. Keep both the same.
typedef enum {
    TASK_CELL_VOLTAGE,
    TASK_BALANCE,
    TASK_SLAVE_FLAGS,
    TASK_PACK_VOLTAGE,
    TASK_IMD,
    TASK_CELL_TEMP,
    TASK_CHARGER_CONTROL,
    TASK_KEEP_SLAVES_AWAKE,
#if BMS_DEBUG_TASK_ENABLE
    TASK_DEBUG,                 // keep it last so it runs after everything else
#endif
    TASK_COUNT
} TaskID_t;

//---------------------------------------------------------------------------------------------------------
void init_BMS_system(void);
void restart_BMS_system(void);      // clears faults, restarts fans, slave link and task counters/timers
void Do_BMS_Tasks(void);            // call from the main loop

// Read
bool            BMSTask_IsEnabled(const TaskID_t id);
ticks_t         BMSTask_GetPeriod(const TaskID_t id);
RoutineResult_t BMSTask_GetLastResult(const TaskID_t id);
uint8_t         BMSTask_GetErrorCount(const TaskID_t id);     // fails in a row, back to 0 on the next OK
uint32_t        BMSTask_GetErrorTotal(const TaskID_t id);     // all fails since the last restart
TimingStats_t   BMSTask_GetStats(const TaskID_t id);          // run time in ticks, count = number of runs

// Change (a bad id is ignored)
void BMSTask_SetEnable(const TaskID_t id, const bool enable);
void BMSTask_SetPeriod(const TaskID_t id, const ticks_t period);
void BMSTask_ResetStats(const TaskID_t id);

#endif /* PHANTOM_DRIVERS_INCLUDE_BMS_TASKS_H_ */
