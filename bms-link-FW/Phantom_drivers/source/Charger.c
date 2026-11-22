/*
 * Charger.c
 *
 *  See Charger.h for usage. Everything that is not part of the API is static.
 *
 *  Units: all state is kept as SI floats (V, A, degC, Ah). Values are only
 *  converted to CAN counts inside the Build/Receive functions below.
 */

#include "Charger.h"
#include "Charger_Hardware.h"
#include "PhantomTimers.h"
#include <string.h>

/*----------------------------------------------------------------------------------------------------
 * CAN encoding (private). Multi-byte fields are big-endian (high byte first).
 *----------------------------------------------------------------------------------------------------*/
#define COUNTS_PER_PACK_VOLT    (10.0f)     /* 0.1 V/bit  */
#define COUNTS_PER_AMP          (10.0f)     /* 0.1 A/bit  */
#define COUNTS_PER_AH           (10.0f)     /* 0.1 Ah/bit */
#define COUNTS_PER_CELL_VOLT    (1000.0f)   /* 1 mV/bit   */
#define TEMP_OFFSET_C           (100.0f)    /* byte = degC + 100 */

/* Current fields: bit 15 = direction (0 = charging, 1 = discharging), bits 0..14 = magnitude.
 * This is the reading of the spec in Charger.md ("my reading") - verify on the real unit. */
#warning "Current direction bit (bit 15) is only a reading of the spec - verify on the real charger"
#define CURRENT_DIRECTION_BIT   (0x8000U)
#define CURRENT_MAGNITUDE_MASK  (0x7FFFU)

/* Pages of the BMS -> charger message (CAN data byte 8). Page 5 only for > 255 cells. */
typedef enum {
    CHARGER_MSG_PAGE_1 = 1U,    /* Msg 10: voltage / current / control    */
    CHARGER_MSG_PAGE_2 = 2U,    /* Msg 11: capacity, cell OVP, cell count */
    CHARGER_MSG_PAGE_3 = 3U,    /* Msg 12: cell max/min, cell UVP, state  */
    CHARGER_MSG_PAGE_4 = 4U,    /* Msg 13: pack V/I, SOC, temperatures    */
    CHARGER_MSG_PAGE_5 = 5U     /* Msg 14: 16-bit cell count              */
} CHARGER_MSG_PAGE_t;

/* Pages sent every cycle: page 5 (16-bit cell count) only when 8 bits are not enough */
#if NUMBER_OF_CELLS_SERIES > 0xFF
#define CHARGER_PAGE_COUNT      (5U)
#else
#define CHARGER_PAGE_COUNT      (4U)
#endif
#if CHARGER_PAGE_COUNT > CAN_MAX_FRAMES
#error "CHARGER_PAGE_COUNT is larger than CAN_MAX_FRAMES"
#endif

/* Message 11, byte 7: cell count, 0 = invalid -> use page 5 */
#define CELL_COUNT_BYTE         ((NUMBER_OF_CELLS_SERIES > 0xFF) ? 0U : (uint8_t)NUMBER_OF_CELLS_SERIES)

/* Message 12, byte 7: battery state bits */
#define BATTERY_STATE_OV        (1U << 0)
#define BATTERY_STATE_UV        (1U << 1)

/*----------------------------------------------------------------------------------------------------
 * Private data
 *----------------------------------------------------------------------------------------------------*/
static ChargerState_t    s_state = CHARGER_STATE_NOT_CHARGING;
static uint64_t          s_stateEntryTick = 0U;       /* when s_state was entered, for the timers */
static battery2Charger_t s_battery;                   /* last data from the application           */
static float             s_maxCurrent = 0.0f;         /* charge current the BMS allows now (A)    */

/* What we ask the charger for. Starts disabled. */
static struct {
    float volts;
    float amps;
    bool  enabled;
} s_control = { 0.0f, 0.0f, false };

/* Last charger broadcast, raw. Written from the CAN interrupt. */
#warning "s_status is written from the CAN interrupt and read in the main loop with no lock: fields can be from two different messages"
static volatile struct {
    uint16_t voltage_raw;
    uint16_t current_raw;
    uint8_t  flags;
} s_status;

/*----------------------------------------------------------------------------------------------------
 * Helpers
 *----------------------------------------------------------------------------------------------------*/
static float ClampF(const float x, const float lo, const float hi)
{
    if (!(x > lo)) return lo;           /* also catches NaN */
    if (x > hi)    return hi;
    return x;
}

/* value * countsPerUnit, rounded, saturated to 0..65535 */
static uint16_t Encode16(const float value, const float countsPerUnit)
{
    const float counts = (value * countsPerUnit) + 0.5f;
    if (!(counts > 0.0f))      return 0U;
    if (counts >= 65535.0f)    return 0xFFFFU;
    return (uint16_t)counts;
}

static uint16_t EncodeCurrent(const float amps)
{
    const bool discharging = (amps < 0.0f);
    uint16_t   magnitude   = Encode16(discharging ? -amps : amps, COUNTS_PER_AMP);
    if (magnitude > CURRENT_MAGNITUDE_MASK) magnitude = CURRENT_MAGNITUDE_MASK;
    return discharging ? (uint16_t)(magnitude | CURRENT_DIRECTION_BIT) : magnitude;
}

static int16_t DecodeCurrentDeci(const uint16_t raw)
{
    const int16_t magnitude = (int16_t)(raw & CURRENT_MAGNITUDE_MASK);
    return (raw & CURRENT_DIRECTION_BIT) ? (int16_t)(-magnitude) : magnitude;
}

static uint8_t EncodeTemp(const float degC)
{
    return (uint8_t)ClampF(degC + TEMP_OFFSET_C + 0.5f, 0.0f, 255.0f);
}

static void PutBE16(uint8_t * const dst, const uint16_t v)
{
    dst[0] = (uint8_t)(v >> 8);
    dst[1] = (uint8_t)(v & 0xFFU);
}

static uint16_t GetBE16(const uint8_t * const src)
{
    return (uint16_t)(((uint16_t)src[0] << 8) | src[1]);
}

static void SetState(const ChargerState_t next)
{
    if (next != s_state) {
        s_state = next;
        s_stateEntryTick = getNow_tick();
    }
}

#warning "ticks_make() argument order assumed (first = minutes) - check PhantomTimers.h, see CHARGER_TICKS_MINUTES"
static bool StateTimeElapsed(const uint32_t minutes)
{
    return hasTimeElapsed(s_stateEntryTick, CHARGER_TICKS_MINUTES(minutes));
}

/*----------------------------------------------------------------------------------------------------
 * CAN message builders (BMS -> charger). buffer is already zeroed by Charger_BuildMessage().
 *----------------------------------------------------------------------------------------------------*/
static void Charger_BuildMessage10(uint8_t * const buffer)
{
    PutBE16(&buffer[0], Encode16(s_control.volts, COUNTS_PER_PACK_VOLT));
    PutBE16(&buffer[2], Encode16(s_control.amps,  COUNTS_PER_AMP));
    buffer[4] = (uint8_t)(s_control.enabled ? 0U : 1U);       /* control: 0 = charge, 1 = battery protection / close output */
    /* [5] max discharge current (10 A/bit): not used */
    buffer[7] = (uint8_t)CHARGER_MSG_PAGE_1;
}

static void Charger_BuildMessage11(uint8_t * const buffer)
{
    PutBE16(&buffer[0], Encode16(CHARGER_NOMINAL_CAPACITY_AH, COUNTS_PER_AH));
    PutBE16(&buffer[2], Encode16(s_battery.actual_Ah,         COUNTS_PER_AH));
    PutBE16(&buffer[4], Encode16(CHARGER_CELL_OVP_V,          COUNTS_PER_CELL_VOLT));
    buffer[6] = CELL_COUNT_BYTE;
    buffer[7] = (uint8_t)CHARGER_MSG_PAGE_2;
}

static void Charger_BuildMessage12(uint8_t * const buffer)
{
    uint8_t state = 0U;
    if (s_battery.cell_max_V > CHARGER_CELL_OVP_V) state |= BATTERY_STATE_OV;
    if (s_battery.cell_min_V < CHARGER_CELL_UVP_V) state |= BATTERY_STATE_UV;

    PutBE16(&buffer[0], Encode16(s_battery.cell_max_V, COUNTS_PER_CELL_VOLT));
    PutBE16(&buffer[2], Encode16(s_battery.cell_min_V, COUNTS_PER_CELL_VOLT));
    PutBE16(&buffer[4], Encode16(CHARGER_CELL_UVP_V,   COUNTS_PER_CELL_VOLT));
    buffer[6] = state;
    buffer[7] = (uint8_t)CHARGER_MSG_PAGE_3;
}

static void Charger_BuildMessage13(uint8_t * const buffer)
{
    PutBE16(&buffer[0], Encode16(s_battery.pack_voltage_V, COUNTS_PER_PACK_VOLT));
    PutBE16(&buffer[2], EncodeCurrent(s_battery.charge_current_A));
    buffer[4] = (s_battery.soc > 100U) ? 100U : s_battery.soc;
    buffer[5] = EncodeTemp(s_battery.temp_max);
    buffer[6] = EncodeTemp(s_battery.temp_min);
    buffer[7] = (uint8_t)CHARGER_MSG_PAGE_4;
}

static void Charger_BuildMessage14(uint8_t * const buffer)
{
    PutBE16(&buffer[0], (uint16_t)NUMBER_OF_CELLS_SERIES);
    buffer[7] = (uint8_t)CHARGER_MSG_PAGE_5;
}

static void Charger_BuildMessage(const CHARGER_MSG_PAGE_t page, uint8_t buffer[CAN_MAX_DLC])
{
    memset(buffer, 0, CAN_MAX_DLC);
    switch (page) {
        case CHARGER_MSG_PAGE_1: Charger_BuildMessage10(buffer); break;
        case CHARGER_MSG_PAGE_2: Charger_BuildMessage11(buffer); break;
        case CHARGER_MSG_PAGE_3: Charger_BuildMessage12(buffer); break;
        case CHARGER_MSG_PAGE_4: Charger_BuildMessage13(buffer); break;
        case CHARGER_MSG_PAGE_5: Charger_BuildMessage14(buffer); break;
        default: break;
    }
}

/* All pages go out as ONE multi-frame message. Single can_send() calls back to back
 * would fail with CAN_ERR_BUSY: the box still holds the previous frame. */
#warning "Pages 1..5 are sent on BMS2CHARGER_DATA. A plain on-board charger may only understand Message 1 (ID 0x1806E5F4, no page byte) - verify"
#warning "can_send_multi frame count must match the BMS2CHARGER_DATA entry in Can_helpers.c (4 or 5 frames); it also blocks up to CAN_TX_TIMEOUT_MS per frame"
static CanStatus_t Charger_SendAllCommands(void)
{
    uint8_t data[CHARGER_PAGE_COUNT * CAN_MAX_DLC];

    uint8_t i;
    for (i = 0U; i < CHARGER_PAGE_COUNT; i++) {
        Charger_BuildMessage((CHARGER_MSG_PAGE_t)(i + 1U), &data[i * CAN_MAX_DLC]);
    }
    return can_send_multi(BMS2CHARGER_DATA, data, sizeof(data));
}

/*----------------------------------------------------------------------------------------------------
 * Receive (charger -> broadcast, Message 2)
 *----------------------------------------------------------------------------------------------------*/
CanStatus_t Charger_ReceiveStatus(void)
{
    uint8_t frame[CAN_MAX_DLC];
    const CanStatus_t status = can_receive(CHARGER2BMS_DATA, frame, sizeof(frame));

    /* OVERRUN: the data is valid, an older message was lost */
    if ((status == CAN_OK) || (status == CAN_ERR_OVERRUN)) {
        s_status.voltage_raw = GetBE16(&frame[0]);
        s_status.current_raw = GetBE16(&frame[2]);
        s_status.flags       = frame[4];
    }
    return status;
}

/* Called from the CAN interrupt when a full message arrived */
void CHARGER2BMS_DATA_FullRoutine(void)
{
    (void)Charger_ReceiveStatus();
}

/*----------------------------------------------------------------------------------------------------
 * Manual control
 *----------------------------------------------------------------------------------------------------*/
void Charger_SetTargetVoltage(const float voltage_V)
{
    s_control.volts = ClampF(voltage_V, 0.0f, CHARGER_OUTPUT_VOLTAGE_MAX_V);
}

void Charger_SetTargetCurrent(const float current_A)
{
    s_control.amps = ClampF(current_A, 0.0f, CHARGER_OUTPUT_MAX_CURRENT_A);
}

void Charger_EnableOutput(const bool enable)
{
    s_control.enabled = enable;
}

void Charger_DisableOutput(void)
{
    Charger_EnableOutput(false);
}

static void SetOutput(const float volts, const float amps, const bool enable)
{
    Charger_SetTargetVoltage(volts);
    Charger_SetTargetCurrent(amps);
    Charger_EnableOutput(enable);
}

/* Output off: 0 V, 0 A, control = close output. Does not touch the state. */
static void Charger_OutputOff(void)
{
    SetOutput(0.0f, 0.0f, false);
}

void Charger_FaultShutdown(void)
{
    Charger_OutputOff();
    SetState(CHARGER_STATE_CHARGER_FAULT);
}

/*----------------------------------------------------------------------------------------------------
 * Getters
 *----------------------------------------------------------------------------------------------------*/
uint16_t Charger_GetOutputVoltage16(void) { return s_status.voltage_raw; }
int16_t  Charger_GetOutputCurrent16(void) { return DecodeCurrentDeci(s_status.current_raw); }

float Charger_GetOutputVoltage(void) { return (float)Charger_GetOutputVoltage16() / COUNTS_PER_PACK_VOLT; }
float Charger_GetOutputCurrent(void) { return (float)Charger_GetOutputCurrent16() / COUNTS_PER_AMP; }
float Charger_GetTargetVoltage(void) { return s_control.volts; }
float Charger_GetTargetCurrent(void) { return s_control.amps; }
bool  Charger_IsOutputEnabled(void)  { return s_control.enabled; }

uint8_t        Charger_GetStatusFlags(void) { return s_status.flags; }
ChargerState_t Charger_GetState(void)       { return s_state; }

bool Charger_IsCharging(void)
{
    return (s_state == CHARGER_STATE_START_CHARGING)   ||
           (s_state == CHARGER_STATE_TRICKLE_CHARGING) ||
           (s_state == CHARGER_STATE_CC_CHARGING)      ||
           (s_state == CHARGER_STATE_CV_CHARGING)      ||
           (s_state == CHARGER_STATE_EOC_CHARGING)     ||
           (s_state == CHARGER_STATE_RE_CHARGING)      ||
           (s_state == CHARGER_STATE_UNBALANCED_CELLS);
}

bool Charger_IsChargeComplete(void)
{
    return (s_state == CHARGER_STATE_DONE_CHARGING);
}

/*----------------------------------------------------------------------------------------------------
 * Initialisation
 *----------------------------------------------------------------------------------------------------*/
void Charger_Init(void)
{
    memset(&s_battery, 0, sizeof(s_battery));
    s_status.voltage_raw = 0U;
    s_status.current_raw = 0U;
    s_status.flags       = 0U;
    s_maxCurrent = 0.0f;
    Charger_OutputOff();
    s_state = CHARGER_STATE_NOT_CHARGING;
    s_stateEntryTick = getNow_tick();
}

/*----------------------------------------------------------------------------------------------------
 * Step 1: battery data
 *----------------------------------------------------------------------------------------------------*/
static bool Charger_UpdateBatteryData(const battery2Charger_t * const battery, const float Max_current)
{
    if (battery == NULL) return false;
    s_battery = *battery;
    s_maxCurrent = (Max_current > 0.0f) ? Max_current : 0.0f;   /* negative / NaN -> 0 = no charging */
    return true;
}

/*----------------------------------------------------------------------------------------------------
 * Step 2: state machine
 *----------------------------------------------------------------------------------------------------*/
static bool IsProtectionState(const ChargerState_t state)
{
    return (state == CHARGER_STATE_OVERHEATED_CELL) || (state == CHARGER_STATE_COLD_CELL) ||
           (state == CHARGER_STATE_OVERCHARGED_CELL) || (state == CHARGER_STATE_UNBALANCED_CELLS);
}

/* Cell protections, highest priority first. Each one uses a set threshold to enter and
 * a (lower) clear threshold to stay, so they cannot chatter. A protection that is
 * already active is never overridden by one with lower priority.
 * Returns true if it changed or holds the state this cycle. */
static bool ApplyProtections(void)
{
    const float delta = s_battery.cell_max_V - s_battery.cell_min_V;

    const bool hot  = (s_state == CHARGER_STATE_OVERHEATED_CELL)
                    ? (s_battery.temp_max   >= CHARGER_OVERTEMP_CLEAR_C)
                    : (s_battery.temp_max   >  CHARGER_OVERTEMP_SET_C);
    const bool cold = (s_state == CHARGER_STATE_COLD_CELL)
                    ? (s_battery.temp_min   <= CHARGER_UNDERTEMP_CLEAR_C)
                    : (s_battery.temp_min   <  CHARGER_UNDERTEMP_SET_C);
    const bool over = (s_state == CHARGER_STATE_OVERCHARGED_CELL)
                    ? (s_battery.cell_max_V >= CHARGER_OVERCHARGE_CLEAR_V)
                    : (s_battery.cell_max_V >  CHARGER_OVERCHARGE_SET_V);
    const bool unb  = (s_state == CHARGER_STATE_UNBALANCED_CELLS)
                    ? (delta                >= CHARGER_UNBALANCE_CLEAR_DELTA_V)
                    : (delta                >  CHARGER_UNBALANCE_SET_DELTA_V);

    if      (hot)  SetState(CHARGER_STATE_OVERHEATED_CELL);
    else if (cold) SetState(CHARGER_STATE_COLD_CELL);
    else if (over) SetState(CHARGER_STATE_OVERCHARGED_CELL);
    else if (unb)  SetState(CHARGER_STATE_UNBALANCED_CELLS);
    else if (IsProtectionState(s_state)) SetState(CHARGER_STATE_START_CHARGING);   /* everything cleared */
    else return false;

    return true;
}

#warning "Charger status is never checked for age: if Message 2 stops arriving, the old status flags and current stay in use"
#warning "CHARGER_STATUS_STARTING (charger did not start / reverse polarity) is not monitored: a wiring fault would just look like 'charging'"
static void Charger_UpdateStateMachine(const uint8_t flags)
{
    /* 1. Faults come first, and stay until a reset is requested and the fault is gone */
    if ((flags & CHARGER_CTRL_FLAG_EXTERNAL_FAULT) || (s_status.flags & CHARGER_STATUS_FAULT_MASK)) {
        Charger_FaultShutdown();
        return;
    }
    if (s_state == CHARGER_STATE_CHARGER_FAULT) {
        if (flags & CHARGER_CTRL_FLAG_FAULT_RESET) SetState(CHARGER_STATE_NOT_CHARGING);
        return;
    }

    /* 2. No permission to charge: not enabled, or the BMS allows no current */
    if (!(flags & CHARGER_CTRL_FLAG_CHARGE_ENABLE) || !(s_maxCurrent > 0.0f)) {
        SetState(CHARGER_STATE_NOT_CHARGING);
        return;
    }

    /* 3. Cell protections */
    if (ApplyProtections()) return;

    /* 4. Normal charge cycle */
    const float cellMin = s_battery.cell_min_V;
    const float cellMax = s_battery.cell_max_V;
    const bool  allCellsAtTarget = (cellMin >= (CHARGER_CELL_TARGET_V - CHARGER_CELL_TARGET_WINDOW_V));

    switch (s_state) {
        case CHARGER_STATE_NOT_CHARGING:
            SetState(CHARGER_STATE_START_CHARGING);
            break;

        case CHARGER_STATE_START_CHARGING:
            SetState((cellMin < CHARGER_TRICKLE_CELL_VOLT) ? CHARGER_STATE_TRICKLE_CHARGING
                                                           : CHARGER_STATE_CC_CHARGING);
            break;

        case CHARGER_STATE_TRICKLE_CHARGING:
            if (cellMin > CHARGER_TRICKLE_CELL_VOLT) SetState(CHARGER_STATE_CC_CHARGING);
            break;

        case CHARGER_STATE_CC_CHARGING:
            /* CV when the pack is at target, or when the fullest cell is (cells can be uneven) */
#warning "CHARGER_CV_ENTRY_MARGIN_V may be inside the charger's +/-1% CV accuracy; if CC never ends, raise it"
            if ((s_battery.pack_voltage_V >= CHARGER_CV_ENTRY_VOLTAGE_V) || (cellMax >= CHARGER_CELL_TARGET_V)) {
                SetState(CHARGER_STATE_CV_CHARGING);
            }
            break;

        case CHARGER_STATE_CV_CHARGING:
            /* Current has tapered off and SOC is high -> end of charge */
#warning "EOC uses the BMS SOC against a target-based threshold, and the charger reports current in 0.1 A steps (EOC current <= 0.1 A never triggers)"
            if ((Charger_GetOutputCurrent() < CHARGER_EOC_CURRENT_AMPS) &&
                ((float)s_battery.soc >= CHARGER_EOC_SOC_PERCENT)) {
                SetState(CHARGER_STATE_EOC_CHARGING);
            }
            break;

        case CHARGER_STATE_EOC_CHARGING:
#warning "CHARGER_EOC_HOLD_TIME_MIN (30 min) is a placeholder"
            /* Hold at target with trickle current until every cell is at target.
             * If that does not happen in time, cool down and try again. */
            if (allCellsAtTarget)                              SetState(CHARGER_STATE_DONE_CHARGING);
            else if (StateTimeElapsed(CHARGER_EOC_HOLD_TIME_MIN)) SetState(CHARGER_STATE_INHIBIT_CHARGING);
            break;

        case CHARGER_STATE_INHIBIT_CHARGING:
            if (StateTimeElapsed(CHARGER_INHIBIT_TIME_MIN)) SetState(CHARGER_STATE_NOT_CHARGING);
            break;

        case CHARGER_STATE_DONE_CHARGING:
            /* Pack sagged (self-discharge): top it up. Needs cellMax below target too,
             * otherwise RE_CHARGING would end at once and the two states would flip-flop. */
            if ((cellMin < (CHARGER_CELL_TARGET_V - CHARGER_CELL_TARGET_WINDOW_V - CHARGER_RECHARGE_HYST_V)) &&
                (cellMax < CHARGER_CELL_TARGET_V)) {
                SetState(CHARGER_STATE_RE_CHARGING);
            }
            break;

        case CHARGER_STATE_RE_CHARGING:
            if (allCellsAtTarget || (cellMax >= CHARGER_CELL_TARGET_V)) SetState(CHARGER_STATE_DONE_CHARGING);
            else if (StateTimeElapsed(CHARGER_EOC_HOLD_TIME_MIN))       SetState(CHARGER_STATE_INHIBIT_CHARGING);
            break;

        default:    /* protection states are handled above; anything else is invalid */
            SetState(CHARGER_STATE_NOT_CHARGING);
            break;
    }
}

/*----------------------------------------------------------------------------------------------------
 * Step 3: state -> charger setpoints
 *----------------------------------------------------------------------------------------------------*/
/* Less current when the coldest cell is near the lower charge limit */
static float ColdDerated(const float amps)
{
    return (s_battery.temp_min < CHARGER_COLD_DERATE_BELOW_C) ? (amps * CHARGER_COLD_DERATE_FACTOR) : amps;
}

/* Never ask for more than the BMS allows */
static float BmsLimited(const float amps)
{
    return (amps > s_maxCurrent) ? s_maxCurrent : amps;
}

#warning "Whole pack is assumed to be in range of the charger model (CHARGER_OUTPUT_VOLTAGE_MIN_V..MAX_V): the charger shuts off outside it, and nothing here checks it"
static void Charger_UpdateOutputsFromState(void)
{
    const float v = CHARGER_TARGET_VOLTAGE_V;

    switch (s_state) {
        case CHARGER_STATE_START_CHARGING:
        case CHARGER_STATE_CC_CHARGING:
        case CHARGER_STATE_CV_CHARGING:     /* current limit stays at CC; the charger's own CV loop tapers it */
            SetOutput(v, BmsLimited(ColdDerated(CHARGER_CC_CURRENT_AMPS)), true);
            break;
        case CHARGER_STATE_TRICKLE_CHARGING:
        case CHARGER_STATE_EOC_CHARGING:
            SetOutput(v, BmsLimited(CHARGER_TRICKLE_CURRENT_AMPS), true);
            break;
        case CHARGER_STATE_RE_CHARGING:
            SetOutput(v, BmsLimited(ColdDerated(CHARGER_RECHARGE_CURRENT_AMPS)), true);
            break;
        case CHARGER_STATE_UNBALANCED_CELLS:
            SetOutput(v, BmsLimited(CHARGER_UNBALANCED_CURRENT_AMPS), true);
            break;

        case CHARGER_STATE_NOT_CHARGING:
        case CHARGER_STATE_INHIBIT_CHARGING:
        case CHARGER_STATE_DONE_CHARGING:
        case CHARGER_STATE_OVERCHARGED_CELL:
        case CHARGER_STATE_OVERHEATED_CELL:
        case CHARGER_STATE_COLD_CELL:
        case CHARGER_STATE_CHARGER_FAULT:
        default:
            Charger_OutputOff();
            break;
    }
}

/*----------------------------------------------------------------------------------------------------
 * One full cycle (call every 1 s)
 *----------------------------------------------------------------------------------------------------*/
CanStatus_t Charger_Update(const battery2Charger_t * const battery, const uint8_t flags, const float Max_current)
{
    if (Charger_UpdateBatteryData(battery, Max_current)) {
        Charger_UpdateStateMachine(flags);
        Charger_UpdateOutputsFromState();
    } else {
        Charger_FaultShutdown();            /* no battery data: never keep charging blind */
    }
    return Charger_SendAllCommands();
}

#warning "Untested with a real charger"
