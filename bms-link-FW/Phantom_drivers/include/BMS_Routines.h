/*
 * BMS_Routines.h
 *
 * A routine is one attempt at a job. It returns what happened, the task layer (BMS_Tasks.c)
 * decides what to do about it (retry, count, raise the fault).
 */
#ifndef PHANTOM_DRIVERS_INCLUDE_BMS_ROUTINES_H_
#define PHANTOM_DRIVERS_INCLUDE_BMS_ROUTINES_H_

#include <stdint.h>
#include <stdbool.h>

// 1 = temperatures/ref come from the analog GPIO pins, so the 2nd reference is not measured
#ifndef USE_ANILOG_GPIO
#define USE_ANILOG_GPIO     0
#endif

// 0 removes the debug routine, its task ID and its table row completely
#ifndef BMS_DEBUG_TASK_ENABLE
#define BMS_DEBUG_TASK_ENABLE   true
#endif

#define ROUTINES_NUMBER_OF_FAILS_ALLOWED (2)
//---------------------------------------------------------------------------------------------------------
// Keep every failure at or after ROUTINE_FAIL_TIMEOUT, ROUTINE_IS_FAIL() depends on the order.
typedef enum {
    ROUTINE_OK = 0,             // worked
    ROUTINE_BUSY,               // non-blocking wait in progress, call again (the task layer ignores the period)
    ROUTINE_SKIPPED,            // nothing to do this time, not an error
    ROUTINE_FAIL_TIMEOUT,       // waited too long
    ROUTINE_FAIL_COMM,          // SPI / CAN / slave did not answer correctly
    ROUTINE_FAIL_BAD_VALUE,     // answered, but the value or the start conditions are wrong
    ROUTINE_FAIL_INTERRUPT,     // an interrupt got in the way
    ROUTINE_FAIL_FAULT          // a fault is active, not counted as a task error
} RoutineResult_t;

#define ROUTINE_IS_FAIL(result)     ((result) >= ROUTINE_FAIL_TIMEOUT)

typedef RoutineResult_t (*RoutineFunc_t)(void);

//---------------------------------------------------------------------------------------------------------
RoutineResult_t MeasureCellVoltage_Routine(void);
RoutineResult_t SetBalanceRate_Routine(void);
RoutineResult_t CheckSlaveFlags_Routine(void);
RoutineResult_t MeasurePackVoltage_Routine(void);
RoutineResult_t MonitorIMD_Routine(void);
RoutineResult_t MonitorCellTemp_Routine(void);      // not written, see the warning in the .c
RoutineResult_t SendChargerControls_Routine(void);
RoutineResult_t KeepSlavesAwake_Routine(void);
RoutineResult_t FanControl_Routine(void);

RoutineResult_t Debug_Routine(void);                // toggles GIO_DEBUG_PIN, put your own probes here

#endif /* PHANTOM_DRIVERS_INCLUDE_BMS_ROUTINES_H_ */
