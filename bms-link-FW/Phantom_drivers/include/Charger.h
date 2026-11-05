/*
 * Charger.h
 *
 *  BMS side of the HK-MF 3.3 kW charger: state machine + CAN messages.
 *
 *  Normal use (call every 1 s - the charger closes its output after 5 s of silence):
 *
 *      Charger_Init();
 *      ...
 *      Charger_Update(&battery2Charger, CHARGER_CTRL_FLAG_CHARGE_ENABLE, Max_current);
 *
 *  Charger_Update() runs, in order: take battery data -> state machine -> state to
 *  setpoints -> send all CAN messages. Those four steps are private to Charger.c.
 *  Max_current = the most charge current (A) the BMS allows right now.
 *
 *  Layout, same as the BatteryCell files:
 *    Charger.h               this file: types and API
 *    Charger_Hardware.h      limits, targets, thresholds, timers (only Charger.c includes it)
 *    Charger_DataSheetSpec.h raw spec numbers               (only Charger_Hardware.h includes it)
 *
 *  All values in this API are SI floats (V, A, degC, Ah). CAN scaling and
 *  byte order (big-endian) are private to Charger.c.
 */

#ifndef PHANTOM_DRIVERS_INCLUDE_CHARGER_H_
#define PHANTOM_DRIVERS_INCLUDE_CHARGER_H_

#include <stdint.h>
#include <stdbool.h>
#include "Can_helpers.h"

/*----------------------------------------------------------------------------------------------------
 * Charger status bits (Message 2, byte 5). 0 = normal for every bit.
 *----------------------------------------------------------------------------------------------------*/
#define CHARGER_STATUS_HW_FAIL          (1U << 0)
#define CHARGER_STATUS_OVERTEMP         (1U << 1)
#define CHARGER_STATUS_INPUT_FAULT      (1U << 2)
#define CHARGER_STATUS_STARTING         (1U << 3)   /* 1 = charger did not start (no battery seen / reverse polarity). Not a fault by itself. */
#define CHARGER_STATUS_COMM_TIMEOUT     (1U << 4)

/* Bits that force a fault shutdown. STARTING is left out on purpose: it is set
 * whenever the pack is not connected yet, so faulting on it would block every start. */
#define CHARGER_STATUS_FAULT_MASK       (CHARGER_STATUS_HW_FAIL | CHARGER_STATUS_OVERTEMP | \
                                         CHARGER_STATUS_INPUT_FAULT | CHARGER_STATUS_COMM_TIMEOUT)

/*----------------------------------------------------------------------------------------------------
 * Control flags for Charger_Update() / Charger_UpdateStateMachine()
 *----------------------------------------------------------------------------------------------------*/
#define CHARGER_CTRL_FLAG_EXTERNAL_FAULT    (1U << 0)   /* system fault: force shutdown                       */
#define CHARGER_CTRL_FLAG_FAULT_RESET       (1U << 1)   /* leave CHARGER_STATE_CHARGER_FAULT (if no fault left) */
#define CHARGER_CTRL_FLAG_CHARGE_ENABLE     (1U << 2)   /* permission to charge                               */

/*----------------------------------------------------------------------------------------------------
 * State machine
 *----------------------------------------------------------------------------------------------------*/
typedef enum {
    CHARGER_STATE_NOT_CHARGING,
    CHARGER_STATE_START_CHARGING,
    CHARGER_STATE_TRICKLE_CHARGING,
    CHARGER_STATE_CC_CHARGING,
    CHARGER_STATE_CV_CHARGING,
    CHARGER_STATE_EOC_CHARGING,
    CHARGER_STATE_INHIBIT_CHARGING,
    CHARGER_STATE_RE_CHARGING,
    CHARGER_STATE_DONE_CHARGING,
    CHARGER_STATE_UNBALANCED_CELLS,
    CHARGER_STATE_OVERCHARGED_CELL,
    CHARGER_STATE_OVERHEATED_CELL,
    CHARGER_STATE_COLD_CELL,            /* new: cell too cold to charge */
    CHARGER_STATE_CHARGER_FAULT
} ChargerState_t;

/*----------------------------------------------------------------------------------------------------
 * Battery data the application hands to the charger
 *----------------------------------------------------------------------------------------------------*/
typedef struct {
    float   actual_Ah;
    float   pack_voltage_V;
    float   charge_current_A;   /* positive = charging, negative = discharging */
    uint8_t soc;                /* 0..100 % */
    float   temp_max;           /* degC */
    float   temp_min;           /* degC */
    float   cell_max_V;
    float   cell_min_V;
    float   cell_avg_V;
} battery2Charger_t;

/*----------------------------------------------------------------------------------------------------
 * Main API
 *----------------------------------------------------------------------------------------------------*/
void Charger_Init(void);

/* One full cycle. Call every 1 s.
 *   battery     = latest battery data. NULL shuts the charger down.
 *   flags       = CHARGER_CTRL_FLAG_*
 *   Max_current = charge current (A) the BMS allows. The charger is never asked for more.
 *                 0 (or less / NaN) = no charging.
 * Returns the CAN send status. */
CanStatus_t Charger_Update(const battery2Charger_t * const battery, const uint8_t flags, const float Max_current);

/* Read Message 2 (charger status) from its CAN box. CHARGER2BMS_DATA_FullRoutine() (called from the
 * CAN interrupt) already does this. CAN_OK or CAN_ERR_OVERRUN = status updated. */
CanStatus_t Charger_ReceiveStatus(void);

/*----------------------------------------------------------------------------------------------------
 * Manual control. Values are clamped to the charger model's limits.
 * The state machine overwrites these every cycle.
 *----------------------------------------------------------------------------------------------------*/
void Charger_SetTargetVoltage(const float voltage_V);
void Charger_SetTargetCurrent(const float current_A);
void Charger_EnableOutput(const bool enable);
void Charger_DisableOutput(void);
void Charger_FaultShutdown(void);               /* output off, 0 V / 0 A, state = FAULT */

/*----------------------------------------------------------------------------------------------------
 * Getters
 *----------------------------------------------------------------------------------------------------*/
float          Charger_GetTargetVoltage(void);
float          Charger_GetTargetCurrent(void);
bool           Charger_IsOutputEnabled(void);
float          Charger_GetOutputVoltage(void);      /* from the charger's broadcast */
float          Charger_GetOutputCurrent(void);
uint16_t       Charger_GetOutputVoltage16(void);    /* raw, 0.1 V/bit */
int16_t        Charger_GetOutputCurrent16(void);    /* signed, 0.1 A/bit */
uint8_t        Charger_GetStatusFlags(void);
ChargerState_t Charger_GetState(void);
bool           Charger_IsCharging(void);            /* the output is being asked to charge */
bool           Charger_IsChargeComplete(void);

#endif /* PHANTOM_DRIVERS_INCLUDE_CHARGER_H_ */
