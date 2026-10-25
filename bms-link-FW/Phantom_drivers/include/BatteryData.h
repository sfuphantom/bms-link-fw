/*
 * BatteryData.h
 *
 *  Created on: Jun 8, 2026
 *      Author: tanjo
 *
 *  NOTE ON FIXES / CHANGES (see BatteryData.c for the matching .c-side notes):
 *   1. MISSING DECLARATIONS: about half of BatteryData.c's public functions
 *      (setBMS_State, BMS_StateMachine, every getFullBatteryData_/
 *      setFullBatteryData_ variant, every GetAvg/Max/MinCell*_SlaveADC/_float) were
 *      never declared here despite having external linkage in the .c file.
 *      Any other file calling them relies on an implicit function
 *      declaration, which is not legal C99/C11 (most modern compilers
 *      reject it outright; the rest silently assume an `int` return type,
 *      which is wrong for several of these and is undefined behaviour).
 *      Every public function now has a matching prototype here.
 *   2. NAME MISMATCH: this file declared `GetBatteryDataPrt()`, but
 *      BatteryData.c defined `GetBatteryData()` (no "Prt"). Any caller using
 *      the declared name would fail to link. Both are gone now anyway - see
 *      #3 - but flagging the class of bug since it's the second time it's
 *      shown up (Charger.c/h had the same issue).
 *   3. SPLIT THE MONOLITHIC GLOBAL (requested): `struct BatteryData_t
 *      BatteryData` is gone, replaced by five smaller, module-private
 *      globals in BatteryData.c - CellData, FullBattery, EcapIMD,
 *      BatteryCurrent, BMS_State - each only reachable through this header's
 *      accessor functions. If any other file was reading `BatteryData.xxx`
 *      directly (rather than through an accessor), it will need updating -
 *      I can't see other files from here, so please grep for `BatteryData.`
 *      before deleting the old struct definition below... it's already
 *      deleted; grep your build for `GetBatteryData(` / `BatteryData.` to be
 *      sure nothing else referenced it.
 *   4. REMOVED DUPLICATE/CONFLICTING "current" FIELDS: the old struct had
 *      BOTH a top-level `int16_t Current` AND a `FullBatteryData_t.Current`
 *      (`uint16_t`) - two sources of truth for the same quantity, one of
 *      them unsigned. An EV pack's current absolutely needs a sign
 *      (charging vs. discharging); the unsigned copy silently threw that
 *      away. Consolidated into a single, signed `BatteryCurrent` global
 *      (matches the "current" you asked to keep separate), and removed
 *      Current from FullBatteryData_t entirely.
 *      This is also the same underlying quantity that Charger.c's
 *      `battery2Charger_t.charge_current_A` needs signed - if that field is
 *      ever fed from here, make sure the ADC->Amps scaling for
 *      GetBatteryCurrentVal() actually produces a signed physical value and
 *      not a raw always-positive ADC code (I don't have the current-sensor
 *      scaling constants to verify which one this is).
 *   5. REMOVED DEAD/UNIMPLEMENTED API: Slave_Volt2ADC / Slave_ADC2Volt /
 *      Slave_ADC2Celcius / Slave_ADC2Volt_arr were declared here but every
 *      body in BatteryData.c was commented out - calling any of them would
 *      have been a straight link error. They also referenced constants
 *      (ADC_OFFSET_VOLTS, ITMP_MILLI_VOLTS_2_CELCIUS, ...) that aren't
 *      defined in any file I have, and look superseded by the SLAVE_ADC2VOLTS
 *      macro already used elsewhere in this file. Removed; if you still need
 *      a real Slave_ADC2Celcius, it needs to be reimplemented against
 *      whatever SlaveCommunation_Hardware.h actually defines.
 *   6. REMOVED SetChargingStatus(): it and GetChargingStatus() were a second,
 *      independent "is charging" flag that could drift out of sync with
 *      BMS_State (which already has a BMS_CHARGING value). Kept
 *      GetChargingStatus() as a read-only wrapper derived from BMS_State
 *      instead of a separately-settable flag, so there's one source of
 *      truth.
 *   7. GetCellTemp() in the .c file copied NUMBER_OF_CELLS_SERIES (6)
 *      elements instead of NUMBER_OF_THERMISTORS_TOTAL (8) - a copy/paste
 *      bug (every sibling function - SetCellTemp, GetCellTempWritePrt,
 *      GetCellTempReadPrt - correctly used the thermistor count). Fixed;
 *      always silently dropped the last 2 thermistor readings before.
 *   8. SetBalancePWM_Nibbles()/GetBalancePWM_Nibbles() computed the copy
 *      length as `NUMBER_OF_CELLS_SERIES/2`, while the array itself is sized
 *      `(NUMBER_OF_CELLS_SERIES+1)/2`. These integer-divide to the same
 *      value for an EVEN cell count (today's 6), so it happened to work, but
 *      an odd series count would silently drop the last nibble's byte.
 *      Fixed to use sizeof() on the actual array so it can't drift again.
 *   9. `initBatteryData()` set the boot state straight to BMS_CHARGING
 *      unconditionally - meaning every power-on looked like "actively
 *      charging" regardless of whether a charger was even connected. Now
 *      boots into BMS_INIT (see the state list below).
 *  10. STATE LIST: renamed/added/removed a few states (all fair game per
 *      your note) -
 *        BMS_START          -> BMS_INIT (power-on self-test, before
 *                               anything is trusted)
 *        (new)              -> BMS_PRECHARGE (close the precharge relay and
 *                               let bus voltage ramp before slamming the
 *                               main contactor closed - very commonly a
 *                               missing state that bites people later)
 *        BMS_RUNNING +
 *        BMS_DISCHARGING    -> merged into just BMS_RUNNING (they had no
 *                               distinct meaning here - flag if you actually
 *                               intended two different things)
 *        BMS_DONE_DISCHARGING -> BMS_LOW_VOLTAGE_CUTOFF (clearer meaning:
 *                               discharge stopped because the pack hit its
 *                               cutoff, not because something finished
 *                               normally)
 *      BMS_IDLE, BMS_CHARGING, BMS_DONE_CHARGING, BMS_SLEEPING, BMS_FAULT
 *      kept as-is.
 *  11. ADDED (see BMS-fault/contactor/staleness sections below): a fault
 *      bitmask (BMSFaults_t) instead of the single opaque `hasFault` bool,
 *      a contactor-state struct (main + precharge - there was nowhere to
 *      even record that today), cell-data staleness tracking (there was no
 *      way to detect a slave-board comms timeout before), pack SOC
 *      aggregation from cell data, per-cell min/max *index* getters (not
 *      just the value), and per-cell balance-PWM get/set so callers don't
 *      have to hand-roll the nibble packing.
 *  12. NOT ADDED, BUT WORTH CONSIDERING LATER: a persistent Ah/cycle counter
 *      for SOH tracking (needs non-volatile storage, out of scope here), and
 *      a real isolation-resistance conversion for ecapIMDData_t (duty/freq
 *      -> ohms is sensor-model-specific and I don't have that datasheet).
 *  13. FLOAT CONVERSION: current, all CellData_t / FullBatteryData_t values and
 *      ecapIMDData_t are now float and are expected to hold PHYSICAL units
 *      (V, degC, ohms, ...), not raw slave-ADC codes - convert before calling
 *      the Set functions. Per-thermistor temperatures live in their own
 *      file-static global (thermistors[] in BatteryData.c); CellData_t also has
 *      its own, currently unused, per-cell CellTempData. ContactorState_t is
 *      now an enum. The *_SlaveADC getters were removed (no ADC code to return
 *      any more); the old *_float getters replace them.
 *  14. RENAMES / REMOVALS: every public function now follows a Module_Verb
 *      scheme (BatteryData_, BMS_, Pack_, Cell_, Thermistor_, IMD_). Notes 1-13
 *      above still use the OLD names. GetChargingStatus() was removed - use
 *      BMS_IsCharging(). Search for "#warning" in BatteryData.c for open issues.
 */

#ifndef PHANTOM_DRIVERS_INCLUDE_BATTERYDATA_H_
#define PHANTOM_DRIVERS_INCLUDE_BATTERYDATA_H_
#include <stdint.h>
#include <stdbool.h>
#include "strings.h"
#include "FullBattery_Hardware.h"
#include "IMD_hardware.h"
//////////////////////////////////////////////////////////////
/* Currently unused anywhere in this module - if this is meant to be a
 * global "compile the driver out" switch, it needs to actually gate
 * something (e.g. #if KILL_DRIVER around the bodies in BatteryData.c).
 * Left as-is but fixed to a real boolean instead of an undefined TRUE. */
#define KILL_DRIVER false
/////////////////////////////////////////////////////////////////
#define NUMBER_OF_VOLT_SAMPLES_SAVED 1
#define NUMBER_OF_TEMP_SAMPLES_SAVED 1
#define NUMBER_OF_RES_SAMPLES_SAVED 1
#define NUMBER_OF_SOC_SAMPLES_SAVED 1
//////////////////////////////////////////////////////////////

//#define PROTO_CODE TRUE

/* See fix #10 above for what changed from the original list. */
typedef enum {
    BMS_INIT = 0,           /* power-on self-test / hardware init, before anything is trusted */
    BMS_IDLE,               /* pack healthy, contactors open, waiting for a charge or run request */
    BMS_PRECHARGE,          /* precharge relay closed, ramping bus voltage before closing the main contactor */
    BMS_RUNNING,            /* main contactor closed, powering the load (discharging) */
    BMS_CHARGING,           /* charger connected and actively charging */
    BMS_DONE_CHARGING,      /* charge complete/balanced; charger still connected but idle */
    BMS_LOW_VOLTAGE_CUTOFF, /* discharge stopped because SOC/voltage hit the cutoff - needs a charge */
    BMS_SLEEPING,           /* low-power sleep; wake periodically to check pack health */
    BMS_FAULT               /* latched fault - outputs disabled until cleared */
} BMSState_t;

/* Fault bitmask (fix #11) - lets multiple concurrent fault causes be tracked
 * and cleared independently instead of one opaque bool. Extend as needed. */
typedef uint16_t BMSFaults_t;
#define BMS_FAULT_NONE                (0U)
#define BMS_FAULT_CELL_OVERVOLTAGE    (1U << 0)
#define BMS_FAULT_CELL_UNDERVOLTAGE   (1U << 1)
#define BMS_FAULT_OVERTEMP            (1U << 2)
#define BMS_FAULT_UNDERTEMP           (1U << 3)
#define BMS_FAULT_CELL_IMBALANCE      (1U << 4)
#define BMS_FAULT_SLAVE_COMM_TIMEOUT  (1U << 5)
#define BMS_FAULT_ISOLATION           (1U << 6)  /* from IMD/ecap data - see fix #12, no conversion yet */
#define BMS_FAULT_CONTACTOR           (1U << 7)  /* weld/failed-to-close/failed-to-open detect */
#define BMS_FAULT_EXTERNAL            (1U << 8)  /* VCU or other upstream system requested a fault */

//----------------------------------------------------------------------------------------------------
typedef struct {
    float duty;
    float freq;
    float isolation;
    float resistance;
    IMDStateEnum state;
} IMDData_t;

typedef struct {
    float Volt;
    float SOC;
    float Res;
    /* Current removed - see fix #4. Use Pack_GetCurrent()/Pack_SetCurrent(). */
} FullBatteryData_t;

typedef struct {
    float CellVolt[NUMBER_OF_VOLT_SAMPLES_SAVED][NUMBER_OF_CELLS_SERIES];
    float CellTempData[NUMBER_OF_TEMP_SAMPLES_SAVED][NUMBER_OF_CELLS_SERIES];  /* per cell (not per thermistor) - currently unused */
    float CellRes[NUMBER_OF_RES_SAMPLES_SAVED][NUMBER_OF_CELLS_SERIES];
    float CellSOC[NUMBER_OF_SOC_SAMPLES_SAVED][NUMBER_OF_CELLS_SERIES];
    uint8_t  BalancePWM_Nibbles[NUMBER_OF_CELLS_SERIES_PAIRS];
    uint16_t BalanceDCC[NUMBER_OF_SLAVE_BOARDS_TOTAL];
} CellData_t;

/* New (fix #11): there was nowhere to even record contactor state before. */
typedef enum {
    CONTACTORS_OPEN = 0,    /* main and precharge both open */
    CONTACTORS_PRECHARGE,   /* precharge closed, main open - bus voltage ramping */
    CONTACTORS_CLOSED       /* main closed, precharge open - normal running */
} ContactorState_t;

//----------------------------------------------------------------------------------------------------
/* ---- Init ---- */
void BatteryData_Init(void);

//----------------------------------------------------------------------------------------------------
/* ---- BMS state machine ---- */
void        BMS_SetState(const BMSState_t State);
BMSState_t  BMS_GetState(void);
bool        BMS_IsCharging(void);
bool        BMS_IsRunning(void);
bool        BMS_IsFaulted(void);
/* Skeleton only - see the #warning and per-case TODOs in BatteryData.c. */
void BMS_UpdateStateMachine(const bool shouldCharge, const bool clearFault, const bool RunBMS, const bool VCUFault, const bool hasFault);

/* ---- Fault flags (fix #11) ---- */
void        BMS_SetFault(const BMSFaults_t fault);
void        BMS_ClearFault(const BMSFaults_t fault);
void        BMS_ClearAllFaults(void);
bool        BMS_HasFault(const BMSFaults_t fault);
BMSFaults_t BMS_GetFaultFlags(void);

/* ---- Contactor state (fix #11) ---- */
void             BMS_SetContactorState(const ContactorState_t state);
ContactorState_t BMS_GetContactorState(void);

//----------------------------------------------------------------------------------------------------
/* ---- Pack current (its own global, per request - fix #4) ---- */
void  Pack_SetCurrent(const float Current_Val);
float Pack_GetCurrent(void);
//----------------------------------------------------------------------------------------------------
/* ---- Bulk row-buffer read pointers (index 0 = most recent sample) ---- */
const float*    Cell_GetVoltReadPtr(const uint8_t index);
const float*    Thermistor_GetReadPtr(const uint8_t index);
const float*    Cell_GetResReadPtr(const uint8_t index);
const float*    Cell_GetSOCReadPtr(const uint8_t index);
const uint8_t*  Cell_GetBalanceNibblesReadPtr(void);
const uint16_t*  Cell_GetBalanceDCCReadPtr(void);
const CellData_t* Cell_GetDataPtr(void);
//----------------------------------------------------------------------------------------------------
/* ---- Bulk row-buffer write pointers (shifts history, returns row 0) ---- */
float*    Cell_GetVoltWritePtr(void);
float*    Thermistor_GetWritePtr(void);
uint8_t*  Cell_GetBalanceNibblesWritePtr(void);
uint16_t* Cell_GetBalanceDCCWritePtr(void);
float*    Cell_GetResWritePtr(void);
float*    Cell_GetSOCWritePtr(void);
//----------------------------------------------------------------------------------------------------
void Cell_SetVolt(const float* CellVolt);
void Thermistor_Set(const float* temps);
void Cell_SetRes(const float *CellRes);
void Cell_SetSOC(const float *CellSOC);
void Cell_GetVolt(float* const CellVolt);
void Thermistor_Get(float* const temps);
void Cell_GetRes(float* const CellRes);
void Cell_GetSOC(float* const CellSOC);
void Cell_SetBalanceNibbles(const uint8_t* BalancePWM_Nibbles);
void Cell_GetBalanceNibbles(uint8_t* const BalancePWM_Nibbles);
/* Nice to have (fix #11): per-cell balance duty without hand-rolling the
 * nibble packing. cellIndex must be < NUMBER_OF_CELLS_SERIES (unchecked,
 * consistent with the rest of this file's indexed accessors). */
void    Cell_SetBalancePWM(const uint8_t cellIndex, const uint8_t dutyNibble);
uint8_t Cell_GetBalancePWM(const uint8_t cellIndex);
//----------------------------------------------------------------------------------------------------
/* ---- Aggregate cell stats (fix #1: now actually declared) ---- */
size_t  Cell_GetMaxVoltIndex(void);   /* new (fix #11): which cell, not just the value */
size_t  Cell_GetMinVoltIndex(void);
float    Cell_GetAvgVolt(void);
float    Cell_GetMaxVolt(void);
float    Cell_GetMinVolt(void);

size_t  Thermistor_GetMaxTempIndex(void);
size_t  Thermistor_GetMinTempIndex(void);
float Thermistor_GetAvgTemp(void);
float Thermistor_GetMaxTemp(void);
float Thermistor_GetMinTemp(void);

/* New (fix #11): averages CellSOC into FullBattery.SOC. Call after a fresh
 * Cell_SetSOC() if you want the pack-level SOC to track the cells. */
void Pack_UpdateSOCFromCells(void);

/* New (fix #11): lets a watchdog/state-machine detect a stalled slave-board
 * link instead of silently charging/discharging on stale cell data forever. */
uint64_t Cell_GetDataAgeTicks(void);
bool     Cell_IsDataStale(const uint64_t maxAgeTicks);

//----------------------------------------------------------------------------------------------------
/* ---- Pack-level (full battery) data ---- */
const FullBatteryData_t* Pack_GetData(void);
float Pack_GetVolt(void);
float Pack_GetSOC(void);
float Pack_GetRes(void);
void Pack_SetVolt(const float Volt);
void Pack_SetSOC(const float SOC);
void Pack_SetRes(const float Res);
void Pack_SetData(const FullBatteryData_t * FullBatteryData);
/* NOTE: signature changed - Current is no longer part of this struct (fix
 * #4), so it's no longer a parameter here either. Use Pack_SetCurrent()
 * for current. Update any existing callers accordingly. */
void Pack_SetAll(const float Volt, const float SOC, const float Res);

//----------------------------------------------------------------------------------------------------
/* ---- IMD /  ---- */
void      IMD_SetData(const IMDData_t ecapIMDData);
IMDData_t IMD_GetData(void);

#endif /* PHANTOM_DRIVERS_INCLUDE_BATTERYDATA_H_ */
