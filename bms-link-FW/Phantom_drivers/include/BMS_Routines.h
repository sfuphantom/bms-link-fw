/*
 * BMS_Routines.h
 *
 * A routine is one attempt at a job. It returns what happened, the task layer (BMS_Tasks.c)
 * decides what to do about it (retry, count, raise the fault).
 *
 * step: owned by the task, starts at 0. A routine that needs several calls (non-blocking wait) writes
 * where it exited into *step and picks up from there on the next call. The task sets it back to 0
 * after any result except ROUTINE_WAITING and ROUTINE_INCOMPLETE. Routines that do not need it: (void)step;
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
#define BMS_DEBUG_TASK_ENABLE   1
#endif

// Fails in a row before the task raises its fault
#ifndef ROUTINES_NUMBER_OF_FAILS_ALLOWED
#define ROUTINES_NUMBER_OF_FAILS_ALLOWED    3
#endif

//---------------------------------------------------------------------------------------------------------
// Keep the groups in this order: ROUTINE_IS_FAIL() is everything from ROUTINE_FAIL_TIMEOUT on.
typedef enum {
    //-----------------------------------------------------------------------------------------------------
    // routine ok / still going return values >>
    ROUTINE_OK = 0,             // worked, finished
    ROUTINE_INCOMPLETE,         // not finished, not an error. Run again at the normal period (step keeps the position)
    ROUTINE_WAITING,            // non-blocking wait in progress. Run again on the next pass, the period is ignored

    //-----------------------------------------------------------------------------------------------------
    // routine skipped return values (not an error, try again at the next period) >>
    ROUTINE_SKIPPED,            // nothing to do
    ROUTINE_SKIPPED_BUSY,       // port / ADC / SPI / CAN in use by someone else
    ROUTINE_SKIPPED_INPUT,      // the input is bad right now, try again later

    //-----------------------------------------------------------------------------------------------------
    // routine fail return values >>
    ROUTINE_FAIL_TIMEOUT,       // waited too long
    ROUTINE_FAIL_COMM,          // SPI / CAN / slave did not answer correctly
    ROUTINE_FAIL_BAD_VALUE,     // answered, but the value or the start conditions are wrong
    ROUTINE_FAIL_INPUT,         // something about the input caused an error / fault / shutdown condition
    ROUTINE_FAIL_INTERRUPT,     // an interrupt got in the way
    ROUTINE_FAIL_FAULT          // a fault is already active, not counted as a task error
} RoutineResult_t;

#define ROUTINE_IS_FAIL(result)     ((result) >= ROUTINE_FAIL_TIMEOUT)

typedef RoutineResult_t (*RoutineFunc_t)(uint8_t * const step);

//---------------------------------------------------------------------------------------------------------
RoutineResult_t MeasureCellVoltage_Routine(uint8_t * const step);
RoutineResult_t SetBalanceRate_Routine(uint8_t * const step);
RoutineResult_t CheckSlaveFlags_Routine(uint8_t * const step);
RoutineResult_t MeasurePackVoltage_Routine(uint8_t * const step);
RoutineResult_t MonitorIMD_Routine(uint8_t * const step);
RoutineResult_t MonitorCellTemp_Routine(uint8_t * const step);      // placeholder
RoutineResult_t EstimateSOC_Routine(uint8_t * const step);          // placeholder
RoutineResult_t MeasureResistance_Routine(uint8_t * const step);    // placeholder, pseudo code in the .c
RoutineResult_t SendChargerControls_Routine(uint8_t * const step);
RoutineResult_t ControlFans_Routine(uint8_t * const step);          // placeholder, no task row yet
RoutineResult_t KeepSlavesAwake_Routine(uint8_t * const step);
#if BMS_DEBUG_TASK_ENABLE
RoutineResult_t Debug_Routine(uint8_t * const step);                // toggles GIO_DEBUG_PIN, put your own probes here
#endif

#endif /* PHANTOM_DRIVERS_INCLUDE_BMS_ROUTINES_H_ */
