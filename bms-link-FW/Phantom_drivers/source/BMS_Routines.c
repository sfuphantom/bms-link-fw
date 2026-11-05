/*
 * BMS_Routines.c
 */
// #include "spi.h"
#include <stdint.h>
#include <stdbool.h>

#include "SPI_helpers.h"
#include "SlaveCommunication_Drivers.h"
#include "SlaveCommunation_Functions.h"
#include "SlaveCommunation_Hardware.h"
#include "BMS_Routines.h"

#include "Can_Helpers.h"
#include "PhantomAlgorithms.h"

#include "BMS_Data.h"
#include "Charger.h"
#include "GetBatteryVoltage.h"
#include "Fault_handler.h"
#include "Balance.h"
#include "IMD.h"
#include "GIO_helpers.h"

//----------------------------------------------------------------------------------------------------
// Helpers: turn a driver answer into a RoutineResult_t
//----------------------------------------------------------------------------------------------------
// For drivers that only say true/false, the caller picks what a false means.
static RoutineResult_t fromBool(const bool ok, const RoutineResult_t failType)
{
    return ok ? ROUTINE_OK : failType;
}

#warning SlaveStatus_t is defined in a header I was not given. Only OK, PEC, MISMATCH, ASLEEP, NO_DATA and LAYOUT are mapped, any other code (an SPI timeout?) becomes FAIL_COMM. Add the rest.
static RoutineResult_t fromSlave(const SlaveStatus_t status)
{
    switch (status) {
    case SLAVE_OK:
        return ROUTINE_OK;
    case SLAVE_ERR_NO_DATA:     // the chip answered but the ADC value is not there
    case SLAVE_ERR_LAYOUT:      // setup problem, not a wire problem
        return ROUTINE_FAIL_BAD_VALUE;
    case SLAVE_ERR_PEC:         // corrupted on the wire
    case SLAVE_ERR_MISMATCH:    // wrote it, read back something else
    case SLAVE_ERR_ASLEEP:
    default:
        return ROUTINE_FAIL_COMM;
    }
}

//----------------------------------------------------------------------------------------------------
RoutineResult_t MeasureCellVoltage_Routine(void)
{
    float * const cellVolt = Cell_GetVoltWritePtr();

#warning Cell over/under voltage is not checked anywhere here (was a TODO in the old code). Who raises CELL_OVERVOLT_FLAG / CELL_UNDERVOLT_FLAG?
    return fromSlave(MeasureCellVoltage(cellVolt, NUMBER_OF_CELLS_SERIES));
}

//----------------------------------------------------------------------------------------------------
RoutineResult_t SetBalanceRate_Routine(void)
{
    // Balance only while charging and with no fault. Otherwise make sure every DCC bit is off.
    if (BMS_GetState() == BMS_CHARGING && !Fault_Any()) {
        float * const balancePWMs = Cell_GetBalancePWMsWritePtr();
        const float * const allCellVolts = Cell_GetVoltReadPtr(0);
        const float minCellVolt = Cell_GetMinVolt();
        uint16_t balanceDCC[NUMBER_OF_SLAVE_BOARDS_TOTAL];
        float slaveBoardTemp[NUMBER_OF_SLAVE_BOARDS_TOTAL];

        getStatus_BoardTempC(slaveBoardTemp);
        Balance_Update(allCellVolts, minCellVolt, slaveBoardTemp, balancePWMs, balanceDCC);
        SetConfig_DCC(balanceDCC);
    }
    else {
        SetAllConfig_DCC(0);
    }

    return fromSlave(Write_CFGR());
}

//----------------------------------------------------------------------------------------------------
RoutineResult_t CheckSlaveFlags_Routine(void)
{
    uint32_t flags = 0;
    SlaveStatus_t status = SLAVE_OK;

#if !USE_ANILOG_GPIO
    status = MeasureRef2ndVoltage();
#endif
    if (status == SLAVE_OK) { status = ReadStatAndGetFlags(&flags); }
    if (status != SLAVE_OK) { return fromSlave(status); }       // a failed read must not look like "no faults"

    Fault_RaiseSlaveMask((uint16_t)flags);      // flags is made from 16 bit per-board masks, nothing is lost
    return ROUTINE_OK;
}

//----------------------------------------------------------------------------------------------------
RoutineResult_t MeasurePackVoltage_Routine(void)
{
    float packVoltage;
    const bool returnVal = MeasureBatteryVoltage(&packVoltage);
    Pack_SetVolt(packVoltage);
    return fromBool(returnVal, ROUTINE_FAIL_COMM);
}

//----------------------------------------------------------------------------------------------------
RoutineResult_t MonitorIMD_Routine(void)
{
    IMDData_t imd;
    if (readIMDSignal(&imd.freq, &imd.duty)){
        return ROUTINE_FAIL_BAD_VALUE;
    }

    imd.resistance = IMD_CalcResistance(imd.duty);
    imd.isolation  = IMD_CalcIsolation(imd.duty);
    imd.state      = IMD_CalcState(imd.duty, imd.freq);
    IMD_SetData(imd);

    set_IMD_fault_state(imd.isolation, imd.state);
    if (IMD_HasFault(imd.duty, imd.freq, imd.resistance, imd.isolation, imd.state)) {
        Fault_RaiseIMD();
    }    return ROUTINE_OK;
}

//----------------------------------------------------------------------------------------------------
RoutineResult_t MonitorCellTemp_Routine(void)
{
#warning MonitorCellTemp_Routine is empty (it had no return at all before). Returns SKIPPED so it can never raise anything.
    return ROUTINE_SKIPPED;
}

//----------------------------------------------------------------------------------------------------
RoutineResult_t SendChargerControls_Routine(void)
{
    battery2Charger_t battery2Charger;
#warning do not be 0: add values. State flag is hardcoded to 0, so the charger state machine is not getting real input.
    const uint8_t chargerStateFlag = 0;

#warning need actual_Ah function and calc
    battery2Charger.actual_Ah = 0;
    battery2Charger.soc = Pack_GetSOC();

    battery2Charger.cell_avg_V = Cell_GetAvgVolt();
    battery2Charger.cell_max_V = Cell_GetMaxVolt();
    battery2Charger.cell_min_V = Cell_GetMinVolt();

    battery2Charger.temp_max = Thermistor_GetMaxTemp();
    battery2Charger.temp_min = Thermistor_GetMinTemp();

    battery2Charger.charge_current_A = Pack_GetCurrent();
    battery2Charger.pack_voltage_V = Pack_GetVolt();

    Charger_UpdateBatteryData(&battery2Charger);
    Charger_UpdateStateMachine(chargerStateFlag);
    Charger_UpdateOutputsFromState();
    Charger_SendAllCommands();

    return ROUTINE_OK;
}
//----------------------------------------------------------------------------------------------------
RoutineResult_t ControlFans_Routine(void){
    return ROUTINE_OK;
}
//----------------------------------------------------------------------------------------------------
RoutineResult_t KeepSlavesAwake_Routine(void)
{
    return fromSlave(KeepAlive());
}

//----------------------------------------------------------------------------------------------------
#if BMS_DEBUG_TASK_ENABLE
RoutineResult_t Debug_Routine(void)
{
#warning GIO_DEBUG_PIN (A2) also has the Debug_GIO_Notification interrupt in Fault_handler.h. If A2 is an input/interrupt there, toggling it will not show on a scope (or may trigger that callback). Use a free output pin if so.
    gio_toggle(GIO_DEBUG_PIN);      // scope this: one toggle per run, so a period of 0 shows the loop time
    return ROUTINE_OK;
}
#endif
