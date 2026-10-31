/*
 * BMS_Routines.h
 *
 * Every routine is one attempt: true = it worked, false = it failed.
 * Retries, error counting and raising faults are done by the task layer (BMS_Tasks.c).
 */
#ifndef PHANTOM_DRIVERS_INCLUDE_BMS_ROUTINES_H_
#define PHANTOM_DRIVERS_INCLUDE_BMS_ROUTINES_H_

#include <stdint.h>
#include <stdbool.h>

// 1 = temperatures/ref come from the analog GPIO pins, so the 2nd reference is not measured
#ifndef USE_ANILOG_GPIO
#define USE_ANILOG_GPIO     0
#endif

//---------------------------------------------------------------------------------------------------------
bool MeasureCellVoltage_Routine(void);
bool SetBalanceRate_Routine(void);
bool CheckSlaveFlags_Routine(void);
bool MeasurePackVoltage_Routine(void);
bool MonitorIMD_Routine(void);
bool SendChargerControls_Routine(void);
bool MonitorCellTemp_Routine(void);         // empty, see the warning in the .c
bool KeepSlavesAwake_Routine(void);

#endif /* PHANTOM_DRIVERS_INCLUDE_BMS_ROUTINES_H_ */
