/*
 * BatteryData.c
 *
 *  Created on: Jun 8, 2026
 *      Author: tanjo
 *
 *  See BatteryData.h for the full list of fixes/changes. Summary of what
 *  changed in THIS file specifically:
 *   - The single global `struct BatteryData_t BatteryData` is replaced by
 *     five smaller, file-static globals (CellData, FullBattery, EcapIMD,
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

#include "SlaveCommunation_Hardware.h"
//#include "BatteryCell_Hardware.h"
#include "FullBattery_Hardware.h"

#include "PhantomHelpers.h"
#include "PhantomTimers.h"   /* getNow_tick() - for cell-data staleness tracking */
#include "BatteryData.h"
#include "RowBuf.h"

//----------------------------------------------------------------------------------------------------
// Private data (fix: replaces the single monolithic BatteryData global)
//----------------------------------------------------------------------------------------------------
CellData_t         CellData;
FullBatteryData_t  FullBattery;
static ecapIMDData_t      EcapIMD;
static int16_t            BatteryCurrent;
static BMSState_t         BMS_State = BMS_INIT;

static BMSFaults_t        s_BMSFaults = BMS_FAULT_NONE;
static ContactorState_t   s_Contactors = { false, false };

/* Tick at which CellData.CellVolt was last refreshed via SetCellVolt(). Used
 * by BatteryData_IsCellDataStale() to catch a stalled slave-board link. */
static uint64_t           s_CellDataUpdateTick = 0;

//----------------------------------------------------------------------------------------------------
// Init
//----------------------------------------------------------------------------------------------------
void initBatteryData(void){
    memset(&CellData, 0, sizeof(CellData));
    memset(&FullBattery, 0, sizeof(FullBattery));
    memset(&EcapIMD, 0, sizeof(EcapIMD));
    BatteryCurrent = 0;
    s_BMSFaults = BMS_FAULT_NONE;
    s_Contactors.MainContactorClosed = false;
    s_Contactors.PrechargeContactorClosed = false;
    s_CellDataUpdateTick = getNow_tick();

    /* Fixed: used to jump straight to BMS_CHARGING on every boot regardless
     * of whether a charger was even connected. BMS_INIT lets
     * BMS_StateMachine() decide where to actually go from a self-test. */
#warning Change to BMS_INIT
    BMS_State = BMS_CHARGING;
}

//----------------------------------------------------------------------------------------------------
// BMS state
//----------------------------------------------------------------------------------------------------
void setBMS_State(const BMSState_t State){
    BMS_State = State;
}
BMSState_t getBMS_State(void){
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

/* Fix: was a wrapper around a second, independent "Charging" bool that could
 * drift out of sync with BMS_State. Now derived, so there's one source of
 * truth. */
bool GetChargingStatus(void){
    return BMS_IsCharging();
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
void SetContactorState(const ContactorState_t state){
    s_Contactors = state;
}
ContactorState_t GetContactorState(void){
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
#warning "BMS_StateMachine() is a TODO skeleton - review every case (and the fault-detection conditions) before relying on this on real hardware."
void BMS_StateMachine(const bool shouldCharge, const bool clearFault, const bool RunBMS, const bool VCUFault, const bool hasFault){
    const BMSState_t currentState = getBMS_State();
    BMSState_t newState = currentState;

    /* TODO: consider whether every fault source here should be equally
     * "hard" - e.g. a transient slave-board comm blip might deserve a retry
     * before latching BMS_FAULT, whereas an isolation fault probably
     * shouldn't. */
    if (hasFault || VCUFault || (BMS_GetFaultFlags() != BMS_FAULT_NONE)) {
        setBMS_State(BMS_FAULT);
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
            /* TODO: close the precharge contactor, wait for bus voltage to
             * converge with pack voltage within tolerance, then close the
             * main contactor (SetContactorState(...)) and move to
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
            /* TODO: stay here while the charger is still connected; move to
             * BMS_IDLE once it's unplugged (however that's detected). */
            break;

        case BMS_LOW_VOLTAGE_CUTOFF:
            /* TODO: only leave this state once actually back on a charger -
             * don't let RunBMS alone pull it out of cutoff. */
            if (shouldCharge) {
                newState = BMS_CHARGING;
            }
            break;

        case BMS_SLEEPING:
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

    setBMS_State(newState);
}

//----------------------------------------------------------------------------------------------------
// Pack current (its own global - see BatteryData.h fix #4)
//----------------------------------------------------------------------------------------------------
void SetBatteryCurrentVal(const int16_t Current_Val){
    BatteryCurrent = Current_Val;
}
int16_t GetBatteryCurrentVal(void){
    return BatteryCurrent;
}

//----------------------------------------------------------------------------------------------------
// IMD / ecap
//----------------------------------------------------------------------------------------------------
void SetEcapIMDData(const ecapIMDData_t ecapIMDData){
    EcapIMD = ecapIMDData;
}
ecapIMDData_t GetEcapIMDData(void){
    return EcapIMD;
}

//----------------------------------------------------------------------------------------------------
// Bulk row-buffer read pointers
//----------------------------------------------------------------------------------------------------
const uint16_t* GetCellVoltReadPrt(const uint8_t index){
    return CellData.CellVolt[index];
}
const uint16_t* GetCellTempReadPrt(const uint8_t index){
    return CellData.CellTemp[index];
}
const uint16_t* GetCellResReadPrt(const uint8_t index){
    return CellData.CellRes[index];
}
const uint16_t* GetCellSOCReadPrt(const uint8_t index){
    return CellData.CellSOC[index];
}
const uint8_t* GetBalancePWM_NibblesReadPrt(void){
    return CellData.BalancePWM_Nibbles;
}
const CellData_t* GetCellDataPrt(void){
    return &CellData;
}

//----------------------------------------------------------------------------------------------------
// Bulk row-buffer write pointers
//----------------------------------------------------------------------------------------------------
uint16_t* GetCellVoltWritePrt(void){
    return (uint16_t*)getRowBufWritePrt(CellData.CellVolt, sizeof(uint16_t)*NUMBER_OF_CELLS_SERIES, NUMBER_OF_VOLT_SAMPLES_SAVED);
}
uint16_t* GetCellTempWritePrt(void){
    return (uint16_t*)getRowBufWritePrt(CellData.CellTemp, sizeof(uint16_t)*NUMBER_OF_THERMISTORS_TOTAL, NUMBER_OF_TEMP_SAMPLES_SAVED);
}
uint8_t* GetBalancePWM_NibblesWritePrt(void){
    return CellData.BalancePWM_Nibbles;
}
uint16_t* GetCellResWritePrt(void){
    return (uint16_t*)getRowBufWritePrt(CellData.CellRes, sizeof(uint16_t)*NUMBER_OF_CELLS_SERIES, NUMBER_OF_RES_SAMPLES_SAVED);
}
uint16_t* GetCellSOCWritePrt(void){
    return (uint16_t*)getRowBufWritePrt(CellData.CellSOC, sizeof(uint16_t)*NUMBER_OF_CELLS_SERIES, NUMBER_OF_SOC_SAMPLES_SAVED);
}

//----------------------------------------------------------------------------------------------------
// Set (prepend a fresh row)
//----------------------------------------------------------------------------------------------------
void SetCellVolt(const uint16_t* CellVolt){
    prePendRowIntoBuf(CellData.CellVolt, CellVolt, sizeof(uint16_t)*NUMBER_OF_CELLS_SERIES, NUMBER_OF_VOLT_SAMPLES_SAVED);
    s_CellDataUpdateTick = getNow_tick();
    /* TODO: check the new row against OV/UV thresholds and BMS_SetFault(...)
     * accordingly (BMS_FAULT_CELL_OVERVOLTAGE / BMS_FAULT_CELL_UNDERVOLTAGE). */
}
void SetCellTemp(const uint16_t* CellTemp){
    prePendRowIntoBuf(CellData.CellTemp, CellTemp, sizeof(uint16_t)*NUMBER_OF_THERMISTORS_TOTAL, NUMBER_OF_TEMP_SAMPLES_SAVED);
    /* TODO: check the new row against OT/UT thresholds and BMS_SetFault(...). */
}
void SetCellRes(const uint16_t *CellRes){
    prePendRowIntoBuf(CellData.CellRes, CellRes, sizeof(uint16_t)*NUMBER_OF_CELLS_SERIES, NUMBER_OF_RES_SAMPLES_SAVED);
}
void SetCellSOC(const uint16_t *CellSOC){
    prePendRowIntoBuf(CellData.CellSOC, CellSOC, sizeof(uint16_t)*NUMBER_OF_CELLS_SERIES, NUMBER_OF_SOC_SAMPLES_SAVED);
}

//----------------------------------------------------------------------------------------------------
// Get (peek the most recent row)
//----------------------------------------------------------------------------------------------------
void GetCellVolt(uint16_t* const CellVolt){
    peekRowBuf(CellData.CellVolt, CellVolt, sizeof(uint16_t)*NUMBER_OF_CELLS_SERIES);
}
void GetCellTemp(uint16_t* const CellTemp){
    /* Fixed (see header fix #7): used to copy NUMBER_OF_CELLS_SERIES
     * elements here, silently dropping the last 2 of 8 thermistor readings. */
    peekRowBuf(CellData.CellTemp, CellTemp, sizeof(uint16_t)*NUMBER_OF_THERMISTORS_TOTAL);
}
void GetCellRes(uint16_t* const CellRes){
    peekRowBuf(CellData.CellRes, CellRes, sizeof(uint16_t)*NUMBER_OF_CELLS_SERIES);
}
void GetCellSOC(uint16_t* const CellSOC){
    peekRowBuf(CellData.CellSOC, CellSOC, sizeof(uint16_t)*NUMBER_OF_CELLS_SERIES);
}

//----------------------------------------------------------------------------------------------------
void SetBalancePWM_Nibbles(const uint8_t* BalancePWM_Nibbles){
    /* Fixed (see header fix #8): was NUMBER_OF_CELLS_SERIES/2, which can
     * disagree with the array's own (NUMBER_OF_CELLS_SERIES+1)/2 sizing for
     * an odd cell count. sizeof() can't drift out of sync with the array. */
    memcpy(CellData.BalancePWM_Nibbles, BalancePWM_Nibbles, sizeof(CellData.BalancePWM_Nibbles));
}
void GetBalancePWM_Nibbles(uint8_t* const BalancePWM_Nibbles){
    memcpy(BalancePWM_Nibbles, CellData.BalancePWM_Nibbles, sizeof(CellData.BalancePWM_Nibbles));
}
void BatteryData_SetCellBalancePWM(const uint8_t cellIndex, const uint8_t dutyNibble){
    const uint8_t byteIdx = cellIndex / 2;
    uint8_t b = CellData.BalancePWM_Nibbles[byteIdx];
    if ((cellIndex % 2u) == 0u) {
        b = (uint8_t)((b & 0xF0u) | (dutyNibble & 0x0Fu));
    } else {
        b = (uint8_t)((b & 0x0Fu) | ((dutyNibble & 0x0Fu) << 4));
    }
    CellData.BalancePWM_Nibbles[byteIdx] = b;
}
uint8_t BatteryData_GetCellBalancePWM(const uint8_t cellIndex){
    const uint8_t byteIdx = cellIndex / 2;
    const uint8_t b = CellData.BalancePWM_Nibbles[byteIdx];
    return ((cellIndex % 2u) == 0u) ? (uint8_t)(b & 0x0Fu) : (uint8_t)((b >> 4) & 0x0Fu);
}

//----------------------------------------------------------------------------------------------------
// Aggregate cell stats
//----------------------------------------------------------------------------------------------------
uint16_t GetAvgCellVolt_SlaveADC(void){
    return array16_avg(GetCellVoltReadPrt(0), NUMBER_OF_CELLS_SERIES);
}
uint16_t GetMaxCellVolt_SlaveADC(void){
    return array16_max(GetCellVoltReadPrt(0), NUMBER_OF_CELLS_SERIES);
}
uint16_t GetMinCellVolt_SlaveADC(void){
    return array16_min(GetCellVoltReadPrt(0), NUMBER_OF_CELLS_SERIES);
}
uint8_t GetMaxCellVolt_Index(void){
    const uint16_t* cells = GetCellVoltReadPrt(0);
    uint8_t maxIdx = 0;
    uint8_t i;
    for (i = 1; i < NUMBER_OF_CELLS_SERIES; i++) {
        if (cells[i] > cells[maxIdx]) maxIdx = i;
    }
    return maxIdx;
}
uint8_t GetMinCellVolt_Index(void){
    const uint16_t* cells = GetCellVoltReadPrt(0);
    uint8_t minIdx = 0;
    uint8_t i;
    for (i = 1; i < NUMBER_OF_CELLS_SERIES; i++) {
        if (cells[i] < cells[minIdx]) minIdx = i;
    }
    return minIdx;
}
float GetAvgCellVolt_float(void){
    return SLAVE_ADC2VOLTS(GetAvgCellVolt_SlaveADC());
}
float GetMaxCellVolt_float(void){
    return SLAVE_ADC2VOLTS(GetMaxCellVolt_SlaveADC());
}
float GetMinCellVolt_float(void){
    return SLAVE_ADC2VOLTS(GetMinCellVolt_SlaveADC());
}
//----------------------------------------------------------------------------------------------------
uint16_t GetAvgCellTemp_SlaveADC(void){
    return array16_avg(GetCellTempReadPrt(0), NUMBER_OF_THERMISTORS_TOTAL);
}
uint16_t GetMaxCellTemp_SlaveADC(void){
    return array16_max(GetCellTempReadPrt(0), NUMBER_OF_THERMISTORS_TOTAL);
}
uint16_t GetMinCellTemp_SlaveADC(void){
    return array16_min(GetCellTempReadPrt(0), NUMBER_OF_THERMISTORS_TOTAL);
}
uint8_t GetMaxCellTemp_Index(void){
    const uint16_t* temps = GetCellTempReadPrt(0);
    uint8_t maxIdx = 0;
    uint8_t i;
    for (i = 1; i < NUMBER_OF_THERMISTORS_TOTAL; i++) {
        if (temps[i] > temps[maxIdx]) maxIdx = i;
    }
    return maxIdx;
}
uint8_t GetMinCellTemp_Index(void){
    const uint16_t* temps = GetCellTempReadPrt(0);
    uint8_t minIdx = 0;
    uint8_t i;
    for (i = 1; i < NUMBER_OF_THERMISTORS_TOTAL; i++) {
        if (temps[i] < temps[minIdx]) minIdx = i;
    }
    return minIdx;
}
#warning "GetAvg/Max/MinCellTemp_float() are still stubs (return 0) - need a real thermistor ADC->Celsius curve; the old Slave_ADC2Celcius this might have used was removed as dead/unimplemented code (see BatteryData.h fix #5)."
float GetAvgCellTemp_float(void){
    return 0.0f;
}
float GetMaxCellTemp_float(void){
    return 0.0f;
}
float GetMinCellTemp_float(void){
    return 0.0f;
}

void BatteryData_UpdatePackSOCFromCells(void){
    FullBattery.SOC = array16_avg(GetCellSOCReadPrt(0), NUMBER_OF_CELLS_SERIES);
}

uint64_t BatteryData_GetCellDataAgeTicks(void){
    return getNow_tick() - s_CellDataUpdateTick;
}
bool BatteryData_IsCellDataStale(const uint64_t maxAgeTicks){
    return BatteryData_GetCellDataAgeTicks() > maxAgeTicks;
}

//----------------------------------------------------------------------------------------------------
// Pack-level (full battery) data
//----------------------------------------------------------------------------------------------------
const FullBatteryData_t* getFullBatteryData(void){
    return &FullBattery;
}
uint16_t getFullBatteryData_Volts(void){
    return FullBattery.Volt;
}
uint16_t getFullBatteryData_SOC(void){
    return FullBattery.SOC;
}
uint16_t getFullBatteryData_Res(void){
    return FullBattery.Res;
}
void setFullBatteryData_Volts(const uint16_t Volt){
    FullBattery.Volt = Volt;
}
void setFullBatteryData_SOC(const uint16_t SOC){
    FullBattery.SOC = SOC;
}
void setFullBatteryData_Res(const uint16_t Res){
    FullBattery.Res = Res;
}
void setFullBatteryData(const FullBatteryData_t * FullBatteryDataIn){
    memcpy(&FullBattery, FullBatteryDataIn, sizeof(FullBatteryData_t));
}
void setFullBatteryData_split(const uint16_t Volt, const uint16_t SOC, const uint16_t Res){
    setFullBatteryData_Volts(Volt);
    setFullBatteryData_SOC(SOC);
    setFullBatteryData_Res(Res);
}
