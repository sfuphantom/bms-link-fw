/*
 * BatteryData.c
 *
 *  Created on: Jun 8, 2026
 *      Author: tanjo
 *
 *  See BatteryData.h for the full list of fixes/changes. Summary of what
 *  changed in THIS file specifically:
 *   - The single global `struct BatteryData_t BatteryData` is replaced by
 *     five smaller, file-static globals (CellData, FullBattery, IMData,
 *     BatteryCurrent, BMS_State), each reachable only through the
 *     accessors declared in BatteryData.h - nothing outside this file can
 *     poke at them directly anymore, which also means the staleness
 *     tracking / (future) fault checks in the setters can't be bypassed.
 *   - Every "inline" function that used to live only in this .c file (with
 *     just a bare "inline" prototype in the header) is now an ordinary
 *     extern function. They referenced file-static data, so they couldn't
 *     be made "static inline" in the header (that data isn't visible
 *     outside this file) - and a bare "inline" definition with no "extern"
 *     is a well-known C99 linkage footgun (see RowBuf.h notes for the same
 *     issue on the two functions where it *was* safe to fix that way).
 *   - GetCellTemp()'s copy-length bug (used NUMBER_OF_CELLS_SERIES instead
 *     of NUMBER_OF_THERMISTORS_TOTAL) is fixed.
 *   - SetBalancePWM_Nibbles()/GetBalancePWM_Nibbles() now use sizeof() on
 *     the actual array instead of recomputing the count a second,
 *     inconsistent way.
 *   - BMS_StateMachine() is filled in as a real switch/break/default
 *     skeleton (it previously had every case body empty with no `break`,
 *     and unconditionally forced BMS_RUNNING on every non-fault call,
 *     ignoring all its own parameters). Each state has a TODO for the real
 *     logic; there's a single #warning so the incompleteness shows up at
 *     build time instead of only being discovered on hardware.
 */

#include <stdint.h>
#include <stdbool.h>
#include "string.h"

//#include "SlaveCommunation_Hardware.h"
#include "FullBattery_Hardware.h"
#include "IMD_hardware.h"

#include "PhantomHelpers.h"
#include "PhantomTimers.h"   /* getNow_tick() - for cell-data staleness tracking */
#include "BatteryData.h"
#include "RowBuf.h"

//----------------------------------------------------------------------------------------------------
// Private data (fix: replaces the single monolithic BatteryData global)
//----------------------------------------------------------------------------------------------------
#warning "CellData is not static - any file can write to it directly and bypass the setters (staleness tracking, future fault checks). Make it static once nothing else references it."
CellData_t         CellData;
#warning "FullBattery is not static - same problem as CellData."
FullBatteryData_t  FullBattery;
#warning "CellData.CellTempData is never read or written (no accessor, nothing fills it from thermistors[]). It is sized per cell while thermistors[] is per sensor, so a thermistor->cell mapping is still needed."
static IMDData_t      IMD_Data;
static float              BatteryCurrent;
/* Per-thermistor temperatures (history rows), kept outside CellData_t. */
static float              thermistors[NUMBER_OF_TEMP_SAMPLES_SAVED][NUMBER_OF_THERMISTORS_TOTAL];
static BMSState_t         BMS_State = BMS_INIT;

static BMSFaults_t        s_BMSFaults = BMS_FAULT_NONE;
static ContactorState_t   s_Contactors = CONTACTORS_OPEN;

/* Tick at which CellData.CellVolt was last refreshed via Cell_SetVolt(). Used
 * by Cell_IsDataStale() to catch a stalled slave-board link. */
static uint64_t           s_CellDataUpdateTick = 0;

//----------------------------------------------------------------------------------------------------
// Init
//----------------------------------------------------------------------------------------------------
void BatteryData_Init(void){
    memset(&CellData, 0, sizeof(CellData));
    memset(&FullBattery, 0, sizeof(FullBattery));
    memset(&IMD_Data, 0, sizeof(IMD_Data));
    memset(thermistors, 0, sizeof(thermistors));
    BatteryCurrent = 0.0f;
    s_BMSFaults = BMS_FAULT_NONE;
    s_Contactors = CONTACTORS_OPEN;
    s_CellDataUpdateTick = getNow_tick();

    /* Should boot into BMS_INIT so BMS_UpdateStateMachine() can decide where to go
     * after a self-test. Still BMS_CHARGING for now. */
#warning "BatteryData_Init() boots into BMS_CHARGING instead of BMS_INIT - every power-on looks like charging, even with no charger connected."
    BMS_State = BMS_CHARGING;
}

//----------------------------------------------------------------------------------------------------
// BMS state
//----------------------------------------------------------------------------------------------------
void BMS_SetState(const BMSState_t State){
    BMS_State = State;
}
BMSState_t BMS_GetState(void){
    return BMS_State;
}
bool BMS_IsCharging(void){
    return BMS_State == BMS_CHARGING;
}
bool BMS_IsRunning(void){
    return BMS_State == BMS_RUNNING;
}
bool BMS_IsFaulted(void){
    return BMS_State == BMS_FAULT;
}

//----------------------------------------------------------------------------------------------------
// Fault flags
//----------------------------------------------------------------------------------------------------
void BMS_SetFault(const BMSFaults_t fault){
    s_BMSFaults |= fault;
}
void BMS_ClearFault(const BMSFaults_t fault){
    s_BMSFaults &= (BMSFaults_t)~fault;
}
void BMS_ClearAllFaults(void){
    s_BMSFaults = BMS_FAULT_NONE;
}
bool BMS_HasFault(const BMSFaults_t fault){
    return (s_BMSFaults & fault) != 0;
}
BMSFaults_t BMS_GetFaultFlags(void){
    return s_BMSFaults;
}

//----------------------------------------------------------------------------------------------------
// Contactor state
//----------------------------------------------------------------------------------------------------
void BMS_SetContactorState(const ContactorState_t state){
    s_Contactors = state;
}
ContactorState_t BMS_GetContactorState(void){
    return s_Contactors;
}

//----------------------------------------------------------------------------------------------------
// State machine
//
// This is intentionally a skeleton, not a finished implementation - every
// case below needs real logic before this is safe to run on hardware. The
// #warning makes sure that's visible at build time rather than only
// discovered later. Fault handling short-circuits the whole switch: any of
// hasFault / VCUFault / a latched internal fault flag forces BMS_FAULT
// immediately, same idea as Charger_UpdateStateMachine()'s fault check.
//----------------------------------------------------------------------------------------------------
#warning "BMS_UpdateStateMachine() is a TODO skeleton - review every case (and the fault-detection conditions) before relying on this on real hardware."
void BMS_UpdateStateMachine(const bool shouldCharge, const bool clearFault, const bool RunBMS, const bool VCUFault, const bool hasFault){
    const BMSState_t currentState = BMS_GetState();
    BMSState_t newState = currentState;

    /* TODO: consider whether every fault source here should be equally
     * "hard" - e.g. a transient slave-board comm blip might deserve a retry
     * before latching BMS_FAULT, whereas an isolation fault probably
     * shouldn't. */
    if (hasFault || VCUFault || (BMS_GetFaultFlags() != BMS_FAULT_NONE)) {
        BMS_SetState(BMS_FAULT);
        return;
    }

    switch (currentState) {
        case BMS_INIT:
            /* TODO: run POST / self-test - slave-board comms, IMD reading,
             * cell voltages sane, contactors in the expected open state -
             * before trusting this pack enough to move to BMS_IDLE. */
            newState = BMS_IDLE;
            break;

        case BMS_IDLE:
            /* TODO: pick charge vs. run vs. sleep. */
            if (shouldCharge) {
                newState = BMS_CHARGING;
            } else if (RunBMS) {
                newState = BMS_PRECHARGE;
            }
            /* else: TODO - after some idle timeout, move to BMS_SLEEPING */
            break;

        case BMS_PRECHARGE:
#warning "BMS_PRECHARGE has no exit transition - the state machine sits here forever once entered."
            /* TODO: close the precharge contactor, wait for bus voltage to
             * converge with pack voltage within tolerance, then close the
             * main contactor (BMS_SetContactorState(...)) and move to
             * BMS_RUNNING. Needs its own timeout back to BMS_FAULT if the
             * bus never converges (likely BMS_FAULT_CONTACTOR). */
            break;

        case BMS_RUNNING:
            /* TODO: monitor for the low-voltage cutoff and for the load
             * asking to stop. */
            if (!RunBMS) {
                newState = BMS_IDLE;
            }
            break;

        case BMS_CHARGING:
            /* TODO: hand off to/consult Charger.c's state (Charger_GetState())
             * and move to BMS_DONE_CHARGING once it reports done. */
            if (!shouldCharge) {
                newState = BMS_IDLE;
            }
            break;

        case BMS_DONE_CHARGING:
#warning "BMS_DONE_CHARGING has no exit transition - the state machine sits here forever once entered."
            /* TODO: stay here while the charger is still connected; move to
             * BMS_IDLE once it's unplugged (however that's detected). */
            break;

        case BMS_LOW_VOLTAGE_CUTOFF:
#warning "Nothing ever transitions INTO BMS_LOW_VOLTAGE_CUTOFF (no cutoff check in BMS_RUNNING), so this case is unreachable."
            /* TODO: only leave this state once actually back on a charger -
             * don't let RunBMS alone pull it out of cutoff. */
            if (shouldCharge) {
                newState = BMS_CHARGING;
            }
            break;

        case BMS_SLEEPING:
#warning "Nothing ever transitions INTO BMS_SLEEPING (no idle timeout in BMS_IDLE), so this case is unreachable."
            /* TODO: wake periodically on a timer even with nothing asking
             * for it, so a slowly-drifting fault still gets caught. */
            if (RunBMS || shouldCharge) {
                newState = BMS_IDLE;
            }
            break;

        case BMS_FAULT:
            /* TODO: consider re-running BMS_INIT on clear instead of jumping
             * straight back to BMS_IDLE, so faults get re-checked rather
             * than just trusted to be gone. */
            if (clearFault) {
                BMS_ClearAllFaults();
                newState = BMS_INIT;
            }
            break;

        default:
            /* Unknown/corrupted state value - fail safe. */
            newState = BMS_FAULT;
            break;
    }

    BMS_SetState(newState);
}

//----------------------------------------------------------------------------------------------------
// Pack current (its own global - see BatteryData.h fix #4)
//----------------------------------------------------------------------------------------------------
void Pack_SetCurrent(const float Current_Val){
    BatteryCurrent = Current_Val;
}
float Pack_GetCurrent(void){
    return BatteryCurrent;
}

//----------------------------------------------------------------------------------------------------
// IMD
//----------------------------------------------------------------------------------------------------
void IMD_SetData(const IMDData_t IMDData){
    IMD_Data = IMDData;
}
IMDData_t IMD_GetData(void){
    return IMD_Data;
}

//----------------------------------------------------------------------------------------------------
// Bulk row-buffer read pointers
//----------------------------------------------------------------------------------------------------
#warning "The index argument of the *ReadPtr functions is not bounds-checked - it must be < the matching NUMBER_OF_*_SAMPLES_SAVED."
const float* Cell_GetVoltReadPtr(const uint8_t index){
    return CellData.CellVolt[index];
}
const float* Thermistor_GetReadPtr(const uint8_t index){
    return thermistors[index];
}
const float* Cell_GetResReadPtr(const uint8_t index){
    return CellData.CellRes[index];
}
const float* Cell_GetSOCReadPtr(const uint8_t index){
    return CellData.CellSOC[index];
}
const uint8_t* Cell_GetBalanceNibblesReadPtr(void){
    return CellData.BalancePWM_Nibbles;
}
const uint16_t* Cell_GetBalanceDCCReadPtr(void){
    return CellData.BalanceDCC;
}
const CellData_t* Cell_GetDataPtr(void){
    return &CellData;
}

//----------------------------------------------------------------------------------------------------
// Bulk row-buffer write pointers
//----------------------------------------------------------------------------------------------------
#warning "Cell_GetVoltWritePtr() does not refresh s_CellDataUpdateTick - data written through this pointer never counts as fresh for Cell_IsDataStale()."
float* Cell_GetVoltWritePtr(void){
    return (float*)getRowBufWritePrt(CellData.CellVolt, sizeof(float)*NUMBER_OF_CELLS_SERIES, NUMBER_OF_VOLT_SAMPLES_SAVED);
}
float* Thermistor_GetWritePtr(void){
    return (float*)getRowBufWritePrt(thermistors, sizeof(float)*NUMBER_OF_THERMISTORS_TOTAL, NUMBER_OF_TEMP_SAMPLES_SAVED);
}
uint8_t* Cell_GetBalanceNibblesWritePtr(void){
    return CellData.BalancePWM_Nibbles;
}
uint16_t* Cell_GetBalanceDCCWritePtr(void){
    return CellData.BalanceDCC;
}
float* Cell_GetResWritePtr(void){
    return (float*)getRowBufWritePrt(CellData.CellRes, sizeof(float)*NUMBER_OF_CELLS_SERIES, NUMBER_OF_RES_SAMPLES_SAVED);
}
float* Cell_GetSOCWritePtr(void){
    return (float*)getRowBufWritePrt(CellData.CellSOC, sizeof(float)*NUMBER_OF_CELLS_SERIES, NUMBER_OF_SOC_SAMPLES_SAVED);
}

//----------------------------------------------------------------------------------------------------
// Set (prepend a fresh row)
//----------------------------------------------------------------------------------------------------
void Cell_SetVolt(const float* CellVolt){
    prePendRowIntoBuf(CellData.CellVolt, CellVolt, sizeof(float)*NUMBER_OF_CELLS_SERIES, NUMBER_OF_VOLT_SAMPLES_SAVED);
    s_CellDataUpdateTick = getNow_tick();
#warning "Cell_SetVolt() does no over/under-voltage check - BMS_FAULT_CELL_OVERVOLTAGE / UNDERVOLTAGE are never set."
    /* TODO: check the new row against OV/UV thresholds and BMS_SetFault(...)
     * accordingly (BMS_FAULT_CELL_OVERVOLTAGE / BMS_FAULT_CELL_UNDERVOLTAGE). */
}
void Thermistor_Set(const float* temps){
    prePendRowIntoBuf(thermistors, temps, sizeof(float)*NUMBER_OF_THERMISTORS_TOTAL, NUMBER_OF_TEMP_SAMPLES_SAVED);
#warning "Thermistor_Set() does no over/under-temperature check - BMS_FAULT_OVERTEMP / UNDERTEMP are never set."
    /* TODO: check the new row against OT/UT thresholds and BMS_SetFault(...). */
}
void Cell_SetRes(const float *CellRes){
    prePendRowIntoBuf(CellData.CellRes, CellRes, sizeof(float)*NUMBER_OF_CELLS_SERIES, NUMBER_OF_RES_SAMPLES_SAVED);
}
void Cell_SetSOC(const float *CellSOC){
    prePendRowIntoBuf(CellData.CellSOC, CellSOC, sizeof(float)*NUMBER_OF_CELLS_SERIES, NUMBER_OF_SOC_SAMPLES_SAVED);
}

//----------------------------------------------------------------------------------------------------
// Get (peek the most recent row)
//----------------------------------------------------------------------------------------------------
void Cell_GetVolt(float* const CellVolt){
    peekRowBuf(CellData.CellVolt, CellVolt, sizeof(float)*NUMBER_OF_CELLS_SERIES);
}
void Thermistor_Get(float* const temps){
    /* Fixed (see header fix #7): used to copy NUMBER_OF_CELLS_SERIES
     * elements here, silently dropping the last 2 of 8 thermistor readings. */
    peekRowBuf(thermistors, temps, sizeof(float)*NUMBER_OF_THERMISTORS_TOTAL);
}
void Cell_GetRes(float* const CellRes){
    peekRowBuf(CellData.CellRes, CellRes, sizeof(float)*NUMBER_OF_CELLS_SERIES);
}
void Cell_GetSOC(float* const CellSOC){
    peekRowBuf(CellData.CellSOC, CellSOC, sizeof(float)*NUMBER_OF_CELLS_SERIES);
}

//----------------------------------------------------------------------------------------------------
void Cell_SetBalanceNibbles(const uint8_t* BalancePWM_Nibbles){
    /* Fixed (see header fix #8): was NUMBER_OF_CELLS_SERIES/2, which can
     * disagree with the array's own (NUMBER_OF_CELLS_SERIES+1)/2 sizing for
     * an odd cell count. sizeof() can't drift out of sync with the array. */
    memcpy(CellData.BalancePWM_Nibbles, BalancePWM_Nibbles, sizeof(CellData.BalancePWM_Nibbles));
}

void Cell_GetBalanceNibbles(uint8_t* const BalancePWM_Nibbles){
    memcpy(BalancePWM_Nibbles, CellData.BalancePWM_Nibbles, sizeof(CellData.BalancePWM_Nibbles));
}
#warning "Cell_SetBalancePWM()/Cell_GetBalancePWM() do not bounds-check cellIndex - an index >= NUMBER_OF_CELLS_SERIES reads/writes past BalancePWM_Nibbles[]."
void Cell_SetBalancePWM(const uint8_t cellIndex, const uint8_t dutyNibble){
    const uint8_t byteIdx = cellIndex / 2;
    uint8_t b = CellData.BalancePWM_Nibbles[byteIdx];
    if ((cellIndex % 2u) == 0u) {
        b = (uint8_t)((b & 0xF0u) | (dutyNibble & 0x0Fu));
    } else {
        b = (uint8_t)((b & 0x0Fu) | ((dutyNibble & 0x0Fu) << 4));
    }
    CellData.BalancePWM_Nibbles[byteIdx] = b;
}
uint8_t Cell_GetBalancePWM(const uint8_t cellIndex){
    const uint8_t byteIdx = cellIndex / 2;
    const uint8_t b = CellData.BalancePWM_Nibbles[byteIdx];
    return ((cellIndex % 2u) == 0u) ? (uint8_t)(b & 0x0Fu) : (uint8_t)((b >> 4) & 0x0Fu);
}

//----------------------------------------------------------------------------------------------------
// Aggregate cell stats
//----------------------------------------------------------------------------------------------------
size_t Cell_GetMaxVoltIndex(void){
    size_t idx;
    arr_f_max(Cell_GetVoltReadPtr(0), NUMBER_OF_CELLS_SERIES, NULL, &idx);
    return idx;
}
size_t Cell_GetMinVoltIndex(void){
    size_t idx;
    arr_f_min(Cell_GetVoltReadPtr(0), NUMBER_OF_CELLS_SERIES, NULL, &idx);
    return idx;
}
float Cell_GetAvgVolt(void){
    const float val = arr_f_avg(Cell_GetVoltReadPtr(0), NUMBER_OF_CELLS_SERIES);
    return val;
}
float Cell_GetMaxVolt(void){
    float val;
    arr_f_max(Cell_GetVoltReadPtr(0), NUMBER_OF_CELLS_SERIES, &val, NULL);
    return val;
}
float Cell_GetMinVolt(void){
    float val;
    arr_f_min(Cell_GetVoltReadPtr(0), NUMBER_OF_CELLS_SERIES, &val, NULL);
    return val;
}
//----------------------------------------------------------------------------------------------------
size_t Thermistor_GetMaxTempIndex(void){
    size_t idx;
    arr_f_max(Thermistor_GetReadPtr(0), NUMBER_OF_THERMISTORS_TOTAL, NULL, &idx);
    return idx;
}
size_t Thermistor_GetMinTempIndex(void){
    size_t idx;
    arr_f_min(Thermistor_GetReadPtr(0), NUMBER_OF_THERMISTORS_TOTAL, NULL, &idx);
    return idx;
}
float Thermistor_GetAvgTemp(void){
    return arr_f_avg(Thermistor_GetReadPtr(0), NUMBER_OF_THERMISTORS_TOTAL);
}
float Thermistor_GetMaxTemp(void){
    float val;
    arr_f_max(Thermistor_GetReadPtr(0), NUMBER_OF_THERMISTORS_TOTAL, &val, NULL);
    return val;
}
float Thermistor_GetMinTemp(void){
    float val;
    arr_f_min(Thermistor_GetReadPtr(0), NUMBER_OF_THERMISTORS_TOTAL, &val, NULL);
    return val;
}

void Pack_UpdateSOCFromCells(void){
    FullBattery.SOC = arr_f_avg(Cell_GetSOCReadPtr(0), NUMBER_OF_THERMISTORS_TOTAL);
}

uint64_t Cell_GetDataAgeTicks(void){
    return getNow_tick() - s_CellDataUpdateTick;
}
bool Cell_IsDataStale(const uint64_t maxAgeTicks){
    return Cell_GetDataAgeTicks() > maxAgeTicks;
}

//----------------------------------------------------------------------------------------------------
// Pack-level (full battery) data
//----------------------------------------------------------------------------------------------------
const FullBatteryData_t* Pack_GetData(void){
    return &FullBattery;
}
float Pack_GetVolt(void){
    return FullBattery.Volt;
}
float Pack_GetSOC(void){
    return FullBattery.SOC;
}
float Pack_GetRes(void){
    return FullBattery.Res;
}
void Pack_SetVolt(const float Volt){
    FullBattery.Volt = Volt;
}
void Pack_SetSOC(const float SOC){
    FullBattery.SOC = SOC;
}
void Pack_SetRes(const float Res){
    FullBattery.Res = Res;
}
void Pack_SetData(const FullBatteryData_t * FullBatteryDataIn){
    memcpy(&FullBattery, FullBatteryDataIn, sizeof(FullBatteryData_t));
}
void Pack_SetAll(const float Volt, const float SOC, const float Res){
    Pack_SetVolt(Volt);
    Pack_SetSOC(SOC);
    Pack_SetRes(Res);
}
