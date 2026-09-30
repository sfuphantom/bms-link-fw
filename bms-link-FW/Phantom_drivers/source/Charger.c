/* Charger.c
 *
 *  NOTE ON FIXES (see Charger.h for the matching macro-level fixes):
 *   1. Charger_BuildMessageXX() used to cast the wire buffer straight to a
 *      ChargerCmdMsgNN_t* and write the struct fields directly. The CAN
 *      spec's multi-byte fields are Big-Endian ("High Byte" first); a plain
 *      struct overlay writes them in the host's native endianness, which is
 *      little-endian on essentially every likely target. Every multi-byte
 *      field sent to the charger would have been byte-swapped. Rewritten to
 *      pack each byte explicitly, independent of host endianness.
 *   2. The header declared Charger_SetTargetVoltage/Current, Charger_EnableOutput,
 *      Charger_GetTargetVoltage/Current, and Charger_IsOutputEnabled, but this
 *      file only ever defined differently-named functions
 *      (Charger_SetAllowedMaxVoltage/Current, Charger_EnableControl) - any
 *      caller using the declared API would fail to link. Renamed the
 *      implementations to match the header, and added the missing getters.
 *      While renaming, also fixed Charger_EnableControl()'s body: it set
 *      s_chargerControl.disable = enable ? 1 : 0 (inverted), so the old
 *      Charger_DisableOutput() (which called it with false) actually set
 *      disable = 0, i.e. enabled the output. Now enable ? 0 : 1, matching
 *      the field name and every read site (control byte: 0=enable, 1=disable).
 *   3. Charger_UpdateStateMachine() treated ANY nonzero s_chargerStatus.status_flags
 *      bit as an immediate fault, including CHARGER_STATUS_STARTING - which the
 *      charger legitimately asserts while powering up - so a normal start
 *      sequence could fault-shutdown itself. Now masked with
 *      CHARGER_STATUS_FAULT_MASK (excludes STARTING). The magic numbers
 *      0x01/0x02/0x04 for the "flags" argument were also replaced with the
 *      named CHARGER_CTRL_FLAG_* bits added to Charger.h.
 *   4. CHARGER_STATE_RE_CHARGING unconditionally transitioned to
 *      CHARGER_STATE_DONE_CHARGING regardless of its own balance check (both
 *      branches of the if/else reached the same line), so a recharge cycle
 *      never actually resumed charging. Fixed so failing the balance check
 *      keeps it in RE_CHARGING (where Charger_UpdateOutputsFromState() now
 *      commands real, non-zero output for that state - previously it
 *      commanded 0V/0A/disabled, so even a working transition wouldn't have
 *      charged anything).
 *   5. CHARGER_STATE_DONE_CHARGING had no logic at all (comment only) for
 *      detecting the pack sagging from self-discharge and resuming charging -
 *      nothing in the whole file ever transitioned into RE_CHARGING. Added
 *      a check against CHARGER_RECHARGE_HYSTERESIS_V.
 *   6. Charger_UpdateOutputsFromState() (formerly file-local, un-prototyped
 *      "changeChargerControl") had no case for CHARGER_STATE_OVERHEATED_CELL,
 *      silently falling through to `default`. Same end result today, but
 *      it's a trap for the next person who changes `default` - added an
 *      explicit case.
 *   7. CV_CHARGING commanded CHARGER_TRICKLE_CURRENT_A as its current limit,
 *      which does not match "constant voltage, current tapers" (comment in
 *      the original dead code) - clamping the limit to trickle from the
 *      moment CV begins would prevent the charger's own CV loop from ever
 *      supplying more than trickle current. CV_CHARGING now commands the
 *      full CC current as a ceiling (the charger's own CV regulation still
 *      tapers actual current down); the trickle limit is now used for
 *      CHARGER_STATE_EOC_CHARGING instead, matching its own comment ("keep
 *      CV for a short period to ensure saturation") rather than disabling
 *      output outright.
 */
#include "Charger.h"
#include <string.h>
#include <math.h>

/*----------------------------------------------------------------------------------------------------
 * Private data
 *----------------------------------------------------------------------------------------------------*/
static ChargerState_t   s_chargerState = CHARGER_STATE_NOT_CHARGING;
static ChargerStatus_t  s_chargerStatus;          /* last received charger status */
//static ChargerStatus_t  s_chargerTarget;          /* target values to send */

/* Battery data cache (set by application) */


static struct {
    uint16_t actual_Ah;
    uint16_t pack_voltage_V;
    uint16_t charge_current_A;
    uint8_t soc;
    uint8_t  temp_max;
    uint8_t  temp_min;
    uint16_t cell_max_V;
    uint16_t cell_min_V;
    uint16_t cell_avg_V;
} s_battery;

typedef struct {
    uint16_t MaxOutVolts;
    int16_t  MaxOutAmps; /* Signed: positive=charging, negative=discharging */
    bool  disable;
}s_chargerControl_t;
static s_chargerControl_t s_chargerControl = {0,0,1};

/* Timers for state machine */
//static uint32_t s_inhibit_timer_ms = 0;
//static bool     s_charge_started = false;

static uint64_t start_inhibit_timer_tick = 0;

/*----------------------------------------------------------------------------------------------------
 * Internal helpers
 *----------------------------------------------------------------------------------------------------*/


/* Convert float to CAN unit with saturation */

static uint16_t BatteryAmps2ChargerAmps(const float A) {
    if (A < 0.0f) return 0;
    return CHARGER_CURRENT_TO_CAN(A);
}
static uint16_t BatteryVolts2ChargerVolts(const float V) {
    if (V < 0.0f) return 0;
    return CHARGER_BATTERY_VOLTAGE_TO_CAN(V);
}
static uint16_t CellV2ChargerVM(const float V) {
    if (V < 0.0f) return 0;
    return CHARGER_CELL_VOLT_TO_CAN(V);
}
static uint16_t Capacity2ChargerCapacity(const float Ah) {
    if (Ah < 0.0f) return 0;
    return CHARGER_CAPACITY_TO_CAN(Ah);
}
static uint8_t Temp2ChargerTemp(const float Temp){
    return CHARGER_TEMP_TO_CAN(Temp);
}

/*----------------------------------------------------------------------------------------------------
 * CAN message builders (BMSâ†’Charger)
 *
 * NOTE: the GT CAN spec's multi-byte fields are Big-Endian ("High Byte"
 * first). These are packed byte-by-byte on purpose (see fix #1) rather than
 * via the ChargerCmdMsgNN_t struct overlays, which remain purely for
 * documenting field layout/order and must NOT be pointer-cast onto a raw
 * CAN buffer on a little-endian target.
 *----------------------------------------------------------------------------------------------------*/
static void PutBE16(uint8_t *dst, uint16_t v) {
    dst[0] = (uint8_t)(v >> 8);
    dst[1] = (uint8_t)(v & 0xFFu);
}

void Charger_BuildMessage10(uint8_t *buffer) {
    PutBE16(&buffer[0], s_chargerControl.MaxOutVolts);
    PutBE16(&buffer[2], (uint16_t)s_chargerControl.MaxOutAmps);
    buffer[4] = (uint8_t)s_chargerControl.disable;  /* control: 0 = enable, 1 = disable */
    buffer[5] = 0;  /* discharge_A - not used */
    buffer[6] = 0;  /* reserved */
    buffer[7] = (uint8_t)CHARGER_MSG_PAGE_1;
}

void Charger_BuildMessage11(uint8_t *buffer) {
    PutBE16(&buffer[0], CHARGER_BATTERY_NOMINAL_CAPACITY_AH);
    PutBE16(&buffer[2], s_battery.actual_Ah);
    PutBE16(&buffer[4], SINGLE_CELL_MAX_PROTECTION_V);
    buffer[6] = (uint8_t)(NUMBER_OF_CELLS_SERIES & 0xFF);
    buffer[7] = (uint8_t)CHARGER_MSG_PAGE_2;
}

void Charger_BuildMessage12(uint8_t *buffer) {
    uint8_t state = 0;
    if (s_battery.cell_max_V > SINGLE_CELL_MAX_PROTECTION_V) state |= 0x01;
    if (s_battery.cell_min_V < SINGLE_CELL_MIN_PROTECTION_V) state |= 0x02;

    PutBE16(&buffer[0], s_battery.cell_max_V);
    PutBE16(&buffer[2], s_battery.cell_min_V);
    PutBE16(&buffer[4], SINGLE_CELL_MIN_PROTECTION_V);
    buffer[6] = state;
    buffer[7] = (uint8_t)CHARGER_MSG_PAGE_3;
}

void Charger_BuildMessage13(uint8_t *buffer) {
    PutBE16(&buffer[0], s_battery.pack_voltage_V);
    PutBE16(&buffer[2], s_battery.charge_current_A);
    buffer[4] = s_battery.soc;
    buffer[5] = s_battery.temp_max;
    buffer[6] = s_battery.temp_min;
    buffer[7] = (uint8_t)CHARGER_MSG_PAGE_4;
}

void Charger_BuildMessage14(uint8_t *buffer) {
    PutBE16(&buffer[0], (uint16_t)NUMBER_OF_CELLS_SERIES);
    memset(&buffer[2], 0, 5);
    buffer[7] = (uint8_t)CHARGER_MSG_PAGE_5;
}

///*----------------------------------------------------------------------------------------------------
// * CAN message builders (BMS→Charger)
// *----------------------------------------------------------------------------------------------------*/
//void Charger_BuildMessage10(uint8_t *buffer) {
//    ChargerCmdMsg10_t *msg = (ChargerCmdMsg10_t *)buffer;
//    msg->voltage_dV = s_chargerControl.MaxOutVolts;
//    msg->current_dA = s_chargerControl.MaxOutAmps;
//    /* control: 0 = enable, 1 = disable */
//    msg->control = (uint8_t)s_chargerControl.disable;
//    msg->discharge_A = 0; /* not used */
//    memset(msg->reserved, 0, sizeof(msg->reserved));
//    msg->page = CHARGER_MSG_PAGE_1;
//}
//
//void Charger_BuildMessage11(uint8_t *buffer) {
//    ChargerCmdMsg11_t *msg = (ChargerCmdMsg11_t *)buffer;
//    msg->nominal_Ah = CHARGER_BATTERY_NOMINAL_CAPACITY_AH;
//    msg->actual_Ah  = s_battery.actual_Ah;//(uint16_t)(s_battery.actual_Ah / 0.1f);
//    msg->cell_ovp_mV = SINGLE_CELL_MAX_PROTECTION_V;
//    msg->battery_num = (uint8_t)(NUMBER_OF_CELLS_SERIES & 0xFF);
//    msg->page = CHARGER_MSG_PAGE_2;
//}
//
//void Charger_BuildMessage12(uint8_t *buffer) {
//    ChargerCmdMsg12_t *msg = (ChargerCmdMsg12_t *)buffer;
//    msg->cell_max_mV = s_battery.cell_max_V;
//    msg->cell_min_mV = s_battery.cell_min_V;
//    msg->cell_uvp_mV = SINGLE_CELL_MIN_PROTECTION_V;
//    uint8_t state = 0;
//    if (s_battery.cell_max_V > SINGLE_CELL_MAX_PROTECTION_V) state |= 0x01;
//    if (s_battery.cell_min_V < SINGLE_CELL_MIN_PROTECTION_V) state |= 0x02;
//    msg->battery_state = state;
//    msg->page = CHARGER_MSG_PAGE_3;
//}
//
//void Charger_BuildMessage13(uint8_t *buffer) {
//    ChargerCmdMsg13_t *msg = (ChargerCmdMsg13_t *)buffer;
//    msg->pack_voltage_dV = s_battery.pack_voltage_V;
//    msg->charge_current_dA = s_battery.charge_current_A;
//    msg->soc = s_battery.soc;
//    msg->temp_max = s_battery.temp_max;
//    msg->temp_min = s_battery.temp_min;
//    msg->page = CHARGER_MSG_PAGE_4;
//}
//
//void Charger_BuildMessage14(uint8_t *buffer) {
//    ChargerCmdMsg14_t *msg = (ChargerCmdMsg14_t *)buffer;
//    msg->battery_num = NUMBER_OF_CELLS_SERIES;
//    memset(msg->reserved, 0, sizeof(msg->reserved));
//    msg->page = CHARGER_MSG_PAGE_5;
//}


void Charger_BuildMessage(const CHARGER_MSG_PAGE_t page, uint8_t *buffer){
    switch(page){
        case CHARGER_MSG_PAGE_1: Charger_BuildMessage10(buffer); return;
        case CHARGER_MSG_PAGE_2: Charger_BuildMessage11(buffer); return;
        case CHARGER_MSG_PAGE_3: Charger_BuildMessage12(buffer); return;
        case CHARGER_MSG_PAGE_4: Charger_BuildMessage13(buffer); return;
        case CHARGER_MSG_PAGE_5: Charger_BuildMessage14(buffer); return;
    }
}

/*----------------------------------------------------------------------------------------------------
 * Send all BMSâ†’charger messages
 *----------------------------------------------------------------------------------------------------*/
void Charger_SendAllCommands(void) {
    uint8_t buffer[8];

    CHARGER_MSG_PAGE_t pageIdx = CHARGER_MSG_PAGE_1;

    for(pageIdx=CHARGER_MSG_PAGE_1; pageIdx <= NUMBER_OF_CHARGER_MSG_PAGE; pageIdx++){
        Charger_BuildMessage( pageIdx, buffer);
        (void)can_transmit_data(BMS2CHARGER_DATA, buffer, 8);
    }
}

/*----------------------------------------------------------------------------------------------------
 * Receive charger status (broadcast)
 *----------------------------------------------------------------------------------------------------*/
bool Charger_ReceiveStatus(const ChargerStatusBroadcast_t *status) {
    if (status == NULL) return false;
    s_chargerStatus.output_voltage_dV = status->output_voltage_dV;
    s_chargerStatus.output_current_dA = status->output_current_dA;
    s_chargerStatus.status_flags      = status->status_flags;
    return true;
}

/*----------------------------------------------------------------------------------------------------
 * Apply target settings (send the command message)
 *----------------------------------------------------------------------------------------------------*/
bool Charger_ApplyTargetSettings(void) {
    /* We only send Message 10 (voltage, current, control) via the legacy CAN ID.
     * The full set of messages is sent by Charger_SendAllCommands().
     * Here we use the original can_transmit_data with BMS2CHARGER_DATA (which likely is same ID).
     * For compatibility, we call Charger_SendAllCommands() which sends all.
     */
    Charger_SendAllCommands();
    return true;
}

/*----------------------------------------------------------------------------------------------------
 * Setters for target (used by state machine)
 * NOTE (fix #2): renamed to match the API actually declared in Charger.h -
 * the header/implementation names had diverged, which would have failed to
 * link for any external caller using the declared names.
 *----------------------------------------------------------------------------------------------------*/
void Charger_SetTargetVoltage(float voltage_V) {
    s_chargerControl.MaxOutVolts = BatteryVolts2ChargerVolts(voltage_V);
}
void Charger_SetTargetCurrent(float current_A) {
    s_chargerControl.MaxOutAmps = BatteryAmps2ChargerAmps(current_A);
}
void Charger_EnableOutput(bool enable) {
    s_chargerControl.disable = enable ? 0 : 1;
}
void Charger_DisableOutput(void) {
    Charger_EnableOutput(false);
}

/*----------------------------------------------------------------------------------------------------
 * Getters
 *----------------------------------------------------------------------------------------------------*/
uint16_t Charger_GetOutputVoltage16(void) {
    return s_chargerStatus.output_voltage_dV;
}
int16_t Charger_GetOutputCurrent16(void) {
    return s_chargerStatus.output_current_dA;
}

float Charger_GetOutputVoltage(void) {
    return CHARGER_CAN_TO_BATTERY_VOLTAGE(Charger_GetOutputVoltage16());
}
float Charger_GetOutputCurrent(void) {
    return CHARGER_CAN_TO_CURRENT(Charger_GetOutputCurrent16());
}
float Charger_GetTargetVoltage(void) {
    return CHARGER_CAN_TO_BATTERY_VOLTAGE(s_chargerControl.MaxOutVolts);
}
float Charger_GetTargetCurrent(void) {
    return CHARGER_CAN_TO_CURRENT(s_chargerControl.MaxOutAmps);
}
bool Charger_IsOutputEnabled(void) {
    return !s_chargerControl.disable;
}
uint8_t Charger_GetStatusFlags(void) {
    return s_chargerStatus.status_flags;
}
ChargerState_t Charger_GetState(void) {
    return s_chargerState;
}
bool Charger_IsCharging(void) {
    return (s_chargerState == CHARGER_STATE_TRICKLE_CHARGING ||
            s_chargerState == CHARGER_STATE_CC_CHARGING ||
            s_chargerState == CHARGER_STATE_CV_CHARGING);
}
bool Charger_IsChargeComplete(void) {
    return (s_chargerState == CHARGER_STATE_DONE_CHARGING);
}

/*----------------------------------------------------------------------------------------------------
 * Fault shutdown
 *----------------------------------------------------------------------------------------------------*/
void Charger_FaultShutdown(void) {
    s_chargerControl.MaxOutVolts = 0;
    s_chargerControl.MaxOutAmps = 0;
    s_chargerControl.disable = 1;   /* disable */
//    (void)Charger_ApplyTargetSettings();
    s_chargerState = CHARGER_STATE_CHARGER_FAULT;
}

/*----------------------------------------------------------------------------------------------------
 * Initialisation
 *----------------------------------------------------------------------------------------------------*/
void Charger_Init(void) {
    memset(&s_chargerStatus, 0, sizeof(s_chargerStatus));
//    memset(&s_chargerTarget, 0, sizeof(s_chargerTarget));
    memset(&s_battery, 0, sizeof(s_battery));
    s_chargerState = CHARGER_STATE_NOT_CHARGING;
    start_inhibit_timer_tick = 0;
//    s_charge_started = false;

}

/*----------------------------------------------------------------------------------------------------
 * Legacy functions (kept for compatibility)
 *----------------------------------------------------------------------------------------------------*/
//bool Charger_SendCmd(const ChargerStatus_t* const data) {
//    /* Convert to target and send all messages */
//    if (data) {
//        s_chargerControl. = data->output_voltage_dV;
//        s_chargerTarget.output_current_dA = data->output_current_dA;
//        s_chargerTarget.status_flags = data->status_flags;
//    }
//    return Charger_ApplyTargetSettings();
//}

bool Charger_GetStatus(ChargerStatus_t* const data) {
    if (data) {
        data->output_voltage_dV = s_chargerStatus.output_voltage_dV;
        data->output_current_dA = s_chargerStatus.output_current_dA;
        data->status_flags = s_chargerStatus.status_flags;
        return true;
    }
    return false;
}

/* The following functions are already implemented; we keep them as they are */
//uint16_t GetChargerControlVoltage(void) { return s_chargerControl.MaxOutVolts; }
//uint16_t GetChargerControlCurrent(void) { return s_chargerControl.MaxOutAmps; }
//uint16_t GetChargerOutputVoltage(void) { return s_chargerStatus.output_voltage_dV; }
//uint16_t GetChargerOutputCurrent(void) { return s_chargerStatus.output_current_dA; }
//bool isChargerEnabled(void) { return !s_chargerControl.disable; }
//uint8_t GetChargerStatus(void) { return s_chargerStatus.status_flags; }

void changeChargeState(const uint16_t Volt, const uint16_t current, const bool enable) {
    s_chargerControl.MaxOutVolts  = Volt;
    s_chargerControl.MaxOutAmps = current;
    s_chargerControl.disable = enable ? 0 : 1;
}
void SetChargerVoltage(const uint16_t Volt) { s_chargerControl.MaxOutVolts = Volt; }
void SetChargerCurrent(const uint16_t current) { s_chargerControl.MaxOutAmps = current; }
void SetChargerLimits(const uint16_t Volt, const uint16_t current) {
    s_chargerControl.MaxOutVolts = Volt;
    s_chargerControl.MaxOutAmps = current;
}
void TurnChargerOn(void) { s_chargerControl.disable = 0; }
void TurnChargerOff(void) { s_chargerControl.disable = 1; }

bool ShutDownCharger_Fault(void) {
    Charger_FaultShutdown();
    return true;
}

//void initCharger(void) { Charger_Init(); }

void CHARGER2BMS_DATA_FullRoutine(void) {
    /* This function is intended to be called when a CAN frame with charger status arrives.
     * We assume the frame is already received and we call Charger_ReceiveStatus().
     */
    /* Placeholder: in real implementation, we would read from CAN buffer and call Charger_ReceiveStatus */
}

bool SetChargerSettings(void) {
    return Charger_ApplyTargetSettings();
}

/*----------------------------------------------------------------------------------------------------
 * Current calculation based on SOC (example)
 *----------------------------------------------------------------------------------------------------*/
//void CalcNewCurrentSetting(const float avg_SOC, const float max_SOC) {
//    float current_A;
//    const float target_SOC = 0.95f; /* 95% */
//    if (avg_SOC < target_SOC) {
//        /* Taper current linearly from max to trickle as SOC approaches target */
//        float fraction = (target_SOC - avg_SOC) / target_SOC;
//        current_A = CHARGER_CONSTANT_CURRENT_A * fraction + CHARGER_TRICKLE_CURRENT_A * (1.0f - fraction);
//    } else {
//        current_A = CHARGER_TRICKLE_CURRENT_A;
//    }
//    current_A = clamp(current_A, CHARGER_TRICKLE_CURRENT_A, CHARGER_CONSTANT_CURRENT_A);
//    Charger_SetTargetCurrent(current_A);
//}

/*----------------------------------------------------------------------------------------------------
 * State machine update
 *----------------------------------------------------------------------------------------------------*/
void Charger_UpdateBatteryData(const battery2Charger_t * battery2Charger){
    /* Read the battery data (assumed already updated by application) */

    s_battery.pack_voltage_V    = BatteryVolts2ChargerVolts(battery2Charger->pack_voltage_V);
    s_battery.charge_current_A  = BatteryAmps2ChargerAmps(battery2Charger->charge_current_A);

    s_battery.cell_max_V        = CellV2ChargerVM(battery2Charger->cell_max_V);
    s_battery.cell_min_V        = CellV2ChargerVM(battery2Charger->cell_min_V);
    s_battery.cell_avg_V        = CellV2ChargerVM(battery2Charger->cell_avg_V);

    s_battery.soc               = (uint8_t)battery2Charger->soc;
    s_battery.temp_max          = Temp2ChargerTemp(battery2Charger->temp_max);
    s_battery.temp_min          = Temp2ChargerTemp(battery2Charger->temp_min);

    s_battery.actual_Ah         = Capacity2ChargerCapacity(battery2Charger->actual_Ah);
}

///* Set initial target */
//Charger_SetTargetVoltage(CHARGER_TARGET_VOLTAGE_V);
//Charger_SetTargetCurrent(CHARGER_CONSTANT_CURRENT_A);
//Charger_EnableOutput(true);
//(void)Charger_ApplyTargetSettings();

void Charger_UpdateStateMachine(const uint8_t flags){
    /* Fix #3: only a genuine fault bit (mask excludes CHARGER_STATUS_STARTING,
     * which the charger sets during a normal power-up) or an explicit
     * external fault request now forces a shutdown. */
    if((flags & CHARGER_CTRL_FLAG_EXTERNAL_FAULT) || (s_chargerStatus.status_flags & CHARGER_STATUS_FAULT_MASK)){
        Charger_FaultShutdown();
        return;
    }
    if(!(flags & CHARGER_CTRL_FLAG_CHARGE_ENABLE)){
        s_chargerState = CHARGER_STATE_NOT_CHARGING;
        return;
    }

    if (s_battery.temp_max > CELL_MAX_CHARGING_OVERHEATED_TRIGGER) {
        s_chargerState = CHARGER_STATE_OVERHEATED_CELL;
        return;
    }
    if (s_battery.cell_max_V > CELL_MAX_CHARGING_OVERSHOOT_TRIGGER) {
        s_chargerState = CHARGER_STATE_OVERCHARGED_CELL;
        return;
    }
    if ((s_battery.cell_max_V - s_battery.cell_min_V) > CELL_CHARGING_MAX_UNBALANCE_DELTA_TRIGGER){
        s_chargerState = CHARGER_STATE_UNBALANCED_CELLS;
        return;
    }

    /* State transitions */
    switch (s_chargerState) {
        case CHARGER_STATE_CHARGER_FAULT:
            if(flags & CHARGER_CTRL_FLAG_FAULT_RESET){
                s_chargerState = CHARGER_STATE_START_CHARGING;
            }
            return;
        case CHARGER_STATE_NOT_CHARGING:
            /* Wait for external start command â€“ we assume it's set by application.
             * For this example, we use a simple condition: if SOC < 95% and voltage within range.
             */
            s_chargerState = CHARGER_STATE_START_CHARGING;
            return;

        case CHARGER_STATE_START_CHARGING:
            /* If battery voltage is very low, use trickle */
            if(s_battery.cell_min_V < CHARGER_TRICKLE_CHARGING_STATE_TRIGGER){
                s_chargerState = CHARGER_STATE_TRICKLE_CHARGING;
                return;
            }

            s_chargerState = CHARGER_STATE_CC_CHARGING;
            return;

        case CHARGER_STATE_TRICKLE_CHARGING:
            if (s_battery.cell_min_V > CHARGER_TRICKLE_CHARGING_STATE_TRIGGER) {
                s_chargerState = CHARGER_STATE_CC_CHARGING;
            }
            else {
                s_chargerState = CHARGER_STATE_TRICKLE_CHARGING;
            }
            return;

        case CHARGER_STATE_CC_CHARGING:
//            /* Constant current phase: maintain max current until voltage approaches target */
//            Charger_SetTargetVoltage(CHARGER_TARGET_VOLTAGE_V);
//            /* Use SOCâ€‘based current tapering if desired (or simply keep max) */
//            Charger_SetTargetCurrent(CHARGER_CONSTANT_CURRENT_A);
//            Charger_EnableOutput(true);
//            (void)Charger_ApplyTargetSettings();

            /* Transition to CV when voltage reaches target (within tolerance) */
            if (s_battery.pack_voltage_V >= CV_CHARGING_TRIGGER) {
                s_chargerState = CHARGER_STATE_CV_CHARGING;
            }
            return;

        case CHARGER_STATE_CV_CHARGING:
//            /* Constant voltage: voltage fixed at target, current tapers */
//            Charger_SetTargetVoltage(CHARGER_TARGET_VOLTAGE_V);
//            /* Compute current based on SOC or voltage error */
//            {
//                float error = CHARGER_TARGET_VOLTAGE_V - pack_V;
//                float current = CHARGER_CONSTANT_CURRENT_A * (error / 1.0f);
//                current = clamp(current, CHARGER_TRICKLE_CURRENT_A, CHARGER_CONSTANT_CURRENT_A);
//                Charger_SetTargetCurrent(current);
//            }
//            Charger_EnableOutput(true);
//            (void)Charger_ApplyTargetSettings();

            /* End of charge when current drops below threshold and SOC high */
            if (s_chargerStatus.output_current_dA < CHARGER_EOC_CURRENT_TRIGGER  && s_battery.soc >= CHARGER_EOC_SOC_TRIGGER) {
                s_chargerState = CHARGER_STATE_EOC_CHARGING;
            }

            return;

        case CHARGER_STATE_EOC_CHARGING:
//            /* Charging complete, but keep CV for a short period to ensure saturation */
//            Charger_SetTargetVoltage(CHARGER_TARGET_VOLTAGE_V);
//            Charger_SetTargetCurrent(CHARGER_TRICKLE_CURRENT_A);
//            Charger_EnableOutput(true);
//            (void)Charger_ApplyTargetSettings();

            /* After a timeout or when current is very low, go to DONE */
            if (s_battery.cell_min_V < SINGLE_CELL_CHARGING_TARGET_V) {
                s_chargerState = CHARGER_STATE_INHIBIT_CHARGING;

                start_inhibit_timer_tick = getNow_tick();

//                Charger_DisableOutput();
//                (void)Charger_ApplyTargetSettings();
            }
            return;

        case CHARGER_STATE_INHIBIT_CHARGING:
            /* Placeholder: implement a cool to down timer */
            if(hasTimeElapsed_rti(start_inhibit_timer_tick, STATE_INHIBIT_CHARGING_TIME)){/* 1 min */
                s_chargerState = CHARGER_STATE_NOT_CHARGING;
            }
            return;

        case CHARGER_STATE_DONE_CHARGING:
            /* Fix #5: this used to be a no-op (comment only) - nothing in the
             * file ever transitioned into RE_CHARGING. Resume charging if the
             * pack has sagged meaningfully below the charged target
             * (self-discharge over time). */
            if (s_battery.cell_min_V < (SINGLE_CELL_CHARGING_TARGET_V - CHARGER_RECHARGE_HYSTERESIS_V)) {
                s_chargerState = CHARGER_STATE_RE_CHARGING;
            }
            return;

        case CHARGER_STATE_RE_CHARGING:
            /* Fix #4: both branches used to fall through to DONE_CHARGING
             * regardless of the balance check below, so a recharge cycle
             * never actually resumed charging. Also fixed the threshold to
             * use SINGLE_CELL_CHARGING_TARGET_V (cell-scale) instead of the
             * pack-scale CHARGER_TARGET_VOLTAGE that was being compared
             * against a cell-scale reading. Stay here (charging, per
             * Charger_UpdateOutputsFromState()) until back within the
             * balanced target window. */
            if (s_battery.cell_max_V < (SINGLE_CELL_CHARGING_TARGET_V + SINGLE_CELL_TARGET_MAX_DELTA_V) &&
                s_battery.cell_min_V > (SINGLE_CELL_CHARGING_TARGET_V - SINGLE_CELL_TARGET_MAX_DELTA_V)
                ) {
                s_chargerState = CHARGER_STATE_DONE_CHARGING;
            }
            return;

        case CHARGER_STATE_UNBALANCED_CELLS:
            /* Detect cell imbalance: if maxâ€‘min > threshold, reduce current or pause */
            if ((s_battery.cell_max_V - s_battery.cell_min_V) < CELL_CHARGING_MIN_UNBALANCE_DELTA_TRIGGER) {
                s_chargerState = CHARGER_STATE_START_CHARGING;
            }
            return;


        case CHARGER_STATE_OVERCHARGED_CELL:
            /* One or more cells exceeded OVP â€“ disable charger and wait */
//            Charger_DisableOutput();
//            (void)Charger_ApplyTargetSettings();
            if (s_battery.cell_max_V < CELL_MIN_CHARGING_OVERSHOOT_TIGGER) {
                s_chargerState = CHARGER_STATE_START_CHARGING;
            }
            return;

        case CHARGER_STATE_OVERHEATED_CELL:
            if (s_battery.temp_max < CELL_MIN_CHARGING_OVERHEATED_TIGGER) {
                s_chargerState = CHARGER_STATE_START_CHARGING;
            }
            return;

        default:
            s_chargerState = CHARGER_STATE_NOT_CHARGING;
            return;
    }
}

/* Fix #6/#7: renamed from the file-local, un-prototyped "changeChargerControl"
 * (now declared in Charger.h so it can actually be called from outside this
 * file - see Charger.h fix #7). CV_CHARGING now commands the full CC current
 * as a ceiling rather than clamping to trickle (fix #7); EOC_CHARGING now
 * keeps output enabled at a trickle current instead of disabling outright,
 * matching its own "keep CV for a short period" comment; RE_CHARGING now
 * commands real (reduced) charging current instead of 0V/0A/disabled;
 * OVERHEATED_CELL got its own explicit case instead of relying on `default`. */
void Charger_UpdateOutputsFromState(void){
    switch(s_chargerState){
//        case CHARGER_STATE_NOT_CHARGING:        changeChargeState(0, 0, false); break;
        case CHARGER_STATE_START_CHARGING:      changeChargeState(CHARGER_CONSTANT_CURRENT_VOLTAGE, CHARGER_CONSTANT_CURRENT_A, true); break;
        case CHARGER_STATE_TRICKLE_CHARGING:    changeChargeState(CHARGER_CONSTANT_CURRENT_VOLTAGE, CHARGER_TRICKLE_CURRENT_A,true); break;
        case CHARGER_STATE_CC_CHARGING:         changeChargeState(CHARGER_CONSTANT_CURRENT_VOLTAGE, CHARGER_CONSTANT_CURRENT_A,true); break;
        case CHARGER_STATE_CV_CHARGING:         changeChargeState(CHARGER_TARGET_VOLTAGE, CHARGER_CONSTANT_CURRENT_A,true); break;
        case CHARGER_STATE_EOC_CHARGING:        changeChargeState(CHARGER_TARGET_VOLTAGE, CHARGER_TRICKLE_CURRENT_A, true); break;
//        case CHARGER_STATE_DONE_CHARGING:       changeChargeState(0,0,false); break;
        case CHARGER_STATE_RE_CHARGING:         changeChargeState(CHARGER_TARGET_VOLTAGE, CHARGER_RECHARGE_CURRENT_A, true); break;
//        case CHARGER_STATE_INHIBIT_CHARGING:    changeChargeState(0, 0, false); break;
        case CHARGER_STATE_UNBALANCED_CELLS:    changeChargeState(CHARGER_TARGET_VOLTAGE,CHARGER_UNBALANCED_CURRENT_A,true); break;
//        case CHARGER_STATE_OVERCHARGED_CELL:    changeChargeState(0,0,false); break;
//        case CHARGER_STATE_OVERHEATED_CELL:     changeChargeState(0,0,false); break;
//        case CHARGER_STATE_CHARGER_FAULT:       changeChargeState(0, 0, false); break;

        case CHARGER_STATE_NOT_CHARGING:
        case CHARGER_STATE_DONE_CHARGING:
        case CHARGER_STATE_INHIBIT_CHARGING:
        case CHARGER_STATE_OVERCHARGED_CELL:
        case CHARGER_STATE_OVERHEATED_CELL:
        case CHARGER_STATE_CHARGER_FAULT:   changeChargeState(0, 0, false); break;

        default: changeChargeState(0, 0, false); break;
    }
}

#warning untested, test with charger
