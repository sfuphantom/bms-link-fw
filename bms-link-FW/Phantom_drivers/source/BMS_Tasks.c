/*
 * BMS_Tasks.c
 */
//----------------------------------------------------------------------------------------------------
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
#include "BMS_Routines.h"
#include "BatteryData.h"
#include "Charger.h"
#include "Fans.h"
#include "spi_helpers.h"

#include "PhantomHelpers.h"
#include "PhantomTimers.h"

#include "Fault_handler.h"
#include "BMS_Tasks.h"

//----------------------------------------------------------------------------------------------------
// Task table
//----------------------------------------------------------------------------------------------------
typedef struct {
//    const char   *name;         // shows up in the debugger
//    task_id_t      task_id;
    VoidFunc      run;
    bool        (*allowed)(void);   // NULL = always allowed
    Periodic_t    timing;       // period 0 = every loop
    TimingStats_t stats;        // how long run() takes, in ticks (check max against the other periods)
} Task_t;

//#define TASK(fn, period, allow)     { #fn, (fn), (allow), { (period), 0 }, {0} }
#define TASK(fn, period, allow)     {(fn), (allow), { (period), 0 }, {0} }

// After a fault the fault handler turned the charger off. Do not let the state machine turn it back on.
static bool allowIfNoFault(void)
{
    return !Fault_Any();
}

// To add a task: write a void(void) routine in BMS_Routines.c, add its period in BMS_Tasks.h, add a row here.
// Rows run in this order, one routine at a time.
static Task_t BMSTask[] = {
    TASK(CellVoltageControlRoutine,  CELL_VOLTAGE_CONTROL_TASK_PERIOD_TICK,  is_SPI_free),
    TASK(SetCellBalancersRoutine,    CELL_BALANCER_CONTROL_TASK_PERIOD_TICK, is_SPI_free),
    TASK(SlaveFlagsRoutine,          SLAVE_FLAG_CHECK_TASK_PERIOD_TICK,      is_SPI_free),
    TASK(SendChargerControlsRoutine, SEND_CHARGER_CONTROL_TASK_PERIOD_TICK,  allowIfNoFault),

    // Written but not scheduled (they were not scheduled in the old code either). Uncomment to use:
    // TASK(MeasureCellResistanceRoutine, CELL_RESISTANCE_MONITOR_TASK_PERIOD_TICK, is_SPI_free),
    // TASK(MeasureBatteryVoltageRoutine, PACK_VOLTAGE_TASK_PERIOD_TICK,            is_SPI_free),
    // TASK(MonitorCellTempRoutine,       <add a period>,                           is_SPI_free),
};
#warning MeasureBatteryVoltageRoutine is not scheduled. Pack_GetVolt() (used by the charger) only updates if something else calls it.

//----------------------------------------------------------------------------------------------------
static void initTasks(void)
{
    size_t i;
    for (i = 0; i < ARRAY_LEN(BMSTask); i++) {
        periodic_init(&BMSTask[i].timing, BMSTask[i].timing.period);
        timingStats_reset(&BMSTask[i].stats);
    }
}

void Do_BMS_Tasks(void)
{
    size_t i;
    for (i = 0; i < ARRAY_LEN(BMSTask); i++) {
        Task_t * const task = &BMSTask[i];

        if (task->allowed != NULL && !task->allowed()) {
            continue;
        }
        // period 0 is handled here so periodic_due() never sees it
        if (task->timing.period != 0 && !periodic_due(&task->timing)) {
            continue;
        }
        timeFunction_stats(task->run, &task->stats);
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

    // WARNING: interrupts are on before the peripherals/modules below are initialized (same as the old code).
    // Consider moving this to the end if an early interrupt can touch uninitialized data.
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
    Fault_Init();           // needs CAN and the charger to be initialized first (moved after Charger_Init)

    restart_BMS_system();
}
