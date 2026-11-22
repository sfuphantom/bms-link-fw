/*
 * BMS_Tasks.c
 */
//----------------------------------------------------------------------------------------------------
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#include "rti.h"
#include "spi.h"
#include "can.h"
#include "het.h"
#include "ecap.h"
#include "sys_vim.h"
#include "sys_core.h"
#include "system.h"
#include "sci.h"
#include "gio.h"

#include "SlaveCommunation_Functions.h"
#include "BMS_Data.h"
#include "Charger.h"
#include "Fans.h"
#include "SPI_helpers.h"

#include "PhantomAlgorithms.h"
#include "PhantomTimers.h"

#include "Fault_handler.h"
#include "BMS_Tasks.h"
#include "BMS_Routines.h"


//----------------------------------------------------------------------------------------------------
// Task table
//----------------------------------------------------------------------------------------------------
#define TASK_NO_FAULT   NUM_BMS_FAULTS      // for a task that should never raise a fault by itself

typedef struct {
    RoutineFunc_t   run;
    TaskID_t        id;             // must match the row number, checked in initTasks()
    BMS_Faults      fault;          // raised when the task fails ROUTINES_NUMBER_OF_FAILS_ALLOWED times in a row
    bool            enable;         // initial value is chosen in the table
    uint8_t         errorCount;     // fails in a row
    uint32_t        errorTotal;
    uint8_t         step;           // handed to the routine, where it exited last time (0 = start)
    RoutineResult_t lastResult;
    Periodic_t      timing;         // timing.period is the one to change
    TimingStats_t   stats;
} Task_t;

#define TASK(id, fn, period, fault, enable)     { (fn), (id), (fault), (enable), 0, 0, 0, ROUTINE_OK, { (period), 0 }, {0} }
// To add a task: write a RoutineResult_t f(void) in BMS_Routines.c, add an ID in BMS_Tasks.h, add its period, add a row here.
// Rows run in this order, one routine at a time, and the row number must equal the ID.
static Task_t BMSTask[] = {
    //   id                      routine                      period                                   fault                       enable
    TASK(TASK_CELL_VOLTAGE,      MeasureCellVoltage_Routine,  CELL_VOLTAGE_MEASURE_TASK_PERIOD_TICK,   BAD_SLAVE_CONNECTION_FLAG,  true),
    TASK(TASK_BALANCE,           SetBalanceRate_Routine,      CELL_BALANCER_CONTROL_TASK_PERIOD_TICK,  BAD_SLAVE_CONNECTION_FLAG,  true),
    TASK(TASK_SLAVE_FLAGS,       CheckSlaveFlags_Routine,     SLAVE_FLAG_CHECK_TASK_PERIOD_TICK,       BAD_SLAVE_CONNECTION_FLAG,  true),

    TASK(TASK_PACK_VOLTAGE,      MeasurePackVoltage_Routine,  PACK_VOLTAGE_TASK_PERIOD_TICK,           BAD_HV_VOLT_FLAG,           true),
    TASK(TASK_IMD,               MonitorIMD_Routine,          IMD_MONITOR_TASK_PERIOD_TICK,            IMD_FAULT_FLAG,             true),
    TASK(TASK_CELL_TEMP,         MonitorCellTemp_Routine,     CELL_TEMP_MONITOR_TASK_PERIOD_TICK,      TASK_NO_FAULT,              true),
    TASK(TASK_SOC,               EstimateSOC_Routine,         SOC_ESTIMATE_TASK_PERIOD_TICK,           TASK_NO_FAULT,              false),
    TASK(TASK_RESISTANCE,        MeasureResistance_Routine,   RESISTANCE_MEASURE_TASK_PERIOD_TICK,     TASK_NO_FAULT,              false),
    TASK(TASK_CHARGER_CONTROL,   SendChargerControls_Routine, SEND_CHARGER_CONTROL_TASK_PERIOD_TICK,   CHARGER_COMMS_FAULT,        true),
    TASK(TASK_KEEP_SLAVES_AWAKE, KeepSlavesAwake_Routine,     KEEP_SLAVES_AWAKE_TASK_PERIOD_TICK,      BAD_SLAVE_CONNECTION_FLAG,  true),
#if BMS_DEBUG_TASK_ENABLE
    TASK(TASK_DEBUG,             Debug_Routine,               DEBUG_TASK_PERIOD_TICK,                  TASK_NO_FAULT,              false),
#endif
};

// Compile error if the table and the enum get out of step (array size would be -1)
typedef char BMSTask_table_size_check[(ARRAY_LEN(BMSTask) == TASK_COUNT) ? 1 : -1];

#warning IMD_FAULT_FLAG and IMD_FAULT both exist in BMS_Faults. Is that on purpose? Check which one the IMD task should raise.

//----------------------------------------------------------------------------------------------------
// Helpers
//----------------------------------------------------------------------------------------------------
static Task_t * getTask(const TaskID_t id)
{
    if ((unsigned)id >= (unsigned)TASK_COUNT) {
        return NULL;
    }
    return &BMSTask[id];
}

//----------------------------------------------------------------------------------------------------
// Running tasks. Works on any table, so a second group only needs its own table and wrapper.
//----------------------------------------------------------------------------------------------------
static bool isDue(Task_t * const task)
{
    if (task->lastResult == ROUTINE_WAITING) { return true; }   // non-blocking routine, keep calling it
    if (task->timing.period == 0)         { return true; }      // handled here so periodic_due() never sees 0
    return periodic_due(&task->timing);
}

static void taskFailed(Task_t * const task)
{
    if (task->errorTotal < UINT32_MAX) { task->errorTotal++; }
    if (task->errorCount < UINT8_MAX)  { task->errorCount++; }

    // == so the fault is raised once per streak, not on every later fail
    if (ROUTINES_NUMBER_OF_FAILS_ALLOWED == task->errorCount  && task->fault < NUM_BMS_FAULTS) {
        Fault_RaiseBMS(task->fault);
    }
}

static void recordResult(Task_t * const task)
{
    const RoutineResult_t result = task->lastResult;

    if (result == ROUTINE_OK) {
        task->errorCount = 0;
    }
    else if (ROUTINE_IS_FAIL(result) && result != ROUTINE_FAIL_FAULT) {
        taskFailed(task);
    }
    // INCOMPLETE, WAITING, the SKIPPED_* and FAIL_FAULT do not change the error count

    // Still in the middle of something: keep step. Anything else ends the sequence, so the next run starts at 0.
    if (result != ROUTINE_WAITING && result != ROUTINE_INCOMPLETE) {
        task->step = 0;
    }
}

static void runTasks(Task_t * const tasks, const size_t count)
{
    size_t i;
    for (i = 0; i < count; i++) {
        Task_t * const task = &tasks[i];
        ticks_t tic;

        if (!task->enable || !isDue(task)) {
            continue;
        }

        tic = timer_tic_tick();
        task->lastResult = task->run(&task->step);
        timingStats_add(&task->stats, timer_toc_tick(tic));

        recordResult(task);
    }
}

//----------------------------------------------------------------------------------------------------
// The manager: turns tasks on/off depending on the BMS state. Add rules here.
//----------------------------------------------------------------------------------------------------
static void updateBMSTaskEnables(void)
{
    // Charger was turned off by the fault handler, do not let this task turn it back on.
//    BMSTask_SetEnable(TASK_CHARGER_CONTROL, CHARGER_TASK_ON && !Fault_Any());
}

void Do_BMS_Tasks(void)
{
    updateBMSTaskEnables();
    runTasks(BMSTask, ARRAY_LEN(BMSTask));
}

//----------------------------------------------------------------------------------------------------
// Read / change a task
//----------------------------------------------------------------------------------------------------
bool BMSTask_IsEnabled(const TaskID_t id)
{
    const Task_t * const task = getTask(id);
    return (task != NULL) && task->enable;
}

ticks_t BMSTask_GetPeriod(const TaskID_t id)
{
    const Task_t * const task = getTask(id);
    return (task != NULL) ? task->timing.period : 0;
}

RoutineResult_t BMSTask_GetLastResult(const TaskID_t id)
{
    const Task_t * const task = getTask(id);
    return (task != NULL) ? task->lastResult : ROUTINE_FAIL_BAD_VALUE;      // bad id
}

uint8_t BMSTask_GetStep(const TaskID_t id)
{
    const Task_t * const task = getTask(id);
    return (task != NULL) ? task->step : 0;
}

uint8_t BMSTask_GetErrorCount(const TaskID_t id)
{
    const Task_t * const task = getTask(id);
    return (task != NULL) ? task->errorCount : 0;
}

uint32_t BMSTask_GetErrorTotal(const TaskID_t id)
{
    const Task_t * const task = getTask(id);
    return (task != NULL) ? task->errorTotal : 0;
}

TimingStats_t BMSTask_GetStats(const TaskID_t id)
{
    static const TimingStats_t empty = {0};
    const Task_t * const task = getTask(id);
    return (task != NULL) ? task->stats : empty;
}

void BMSTask_SetEnable(const TaskID_t id, const bool enable)
{
    Task_t * const task = getTask(id);
    if (task == NULL) { return; }

    if (enable && !task->enable) {
        // starting fresh: forget a half finished sequence and restart the period
        task->step = 0;
        task->lastResult = ROUTINE_OK;
        periodic_init(&task->timing, task->timing.period);
    }
    task->enable = enable;
}

void BMSTask_SetPeriod(const TaskID_t id, const ticks_t period)
{
    Task_t * const task = getTask(id);
    if (task != NULL) { task->timing.period = period; }
}

void BMSTask_ResetStats(const TaskID_t id)
{
    Task_t * const task = getTask(id);
    if (task != NULL) { timingStats_reset(&task->stats); }
}

//----------------------------------------------------------------------------------------------------
static void initTasks(void)
{
    size_t i;
    for (i = 0; i < ARRAY_LEN(BMSTask); i++) {
        Task_t * const task = &BMSTask[i];

        if (task->id != (TaskID_t)i) {
            // Table rows are not in the same order as the enum, so BMSTask_Get*(id) would read the wrong task.
            Fault_RaiseBMS(DEBUG_FLAG);
        }

        task->errorCount = 0;
        task->errorTotal = 0;
        task->step = 0;
        task->lastResult = ROUTINE_OK;
        periodic_init(&task->timing, task->timing.period);
        timingStats_reset(&task->stats);
    }
}

//----------------------------------------------------------------------------------------------------
// Init
//----------------------------------------------------------------------------------------------------
static void enableAllInterrupts(void)
{
    vimInit();
    _enable_IRQ();
    _enable_interrupt_();
}

void restart_BMS_system(void)
{
    Fans_Init();
    Fault_ClearAll();
    initLink();
    initTasks();
}

void init_BMS_system(void)
{
    systemInit();

#warning Interrupts are on before the peripherals below are initialized (same as the old code). Move it to the end if an early interrupt can touch uninitialized data.
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

    BatteryData_Init();
    Charger_Init();
    Fault_Init();           // needs CAN and the charger to be initialized first

    restart_BMS_system();
}
