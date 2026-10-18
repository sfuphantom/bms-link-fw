/*
 * Fault_handler.c
 */
#include <string.h>
#include "het.h"
#include "GIO_helpers.h"

#include "Fault_handler.h"
#include "PhantomHelpers.h"
#include "Charger.h"
#include "Fans.h"
#include "Phantom_Can.h"

#define FAULT_FAN_DUTY      99
#define FAULT_CAN_BYTES     8

// Map these to interrupt disable / enable if faults are raised from interrupts AND the main loop.
#ifndef FAULT_LOCK
#define FAULT_LOCK()        ((void)0)
#define FAULT_UNLOCK()      ((void)0)
#endif

// compile error if the fault data grows past what one CAN frame holds
typedef char FaultData_must_fit_8_bytes[(sizeof(BMSFaultsData_t) <= FAULT_CAN_BYTES) ? 1 : -1];

static BMSFaultsData_t Faults = {0u, 0u, Normal_Condition, ISOLATION_UNKNOWN, FIRST_FAULT_NONE, 0u};
static bool handling = false;

//----------------------------------------------------------------------------------------------------
// Extra routines, one per fault. A fault without one only gets Fault_Handler().
typedef void (*FaultRoutine_t)(void);

static void FansMax(void){
    SetAllFansDuty(FAULT_FAN_DUTY);
}
static void OverVoltageRoutine(void){
    FansMax();
}
static void UnderVoltageRoutine(void){
    // nothing extra yet, kept separate from over voltage on purpose
}

static const FaultRoutine_t BMSFaultRoutines[NUM_BMS_FAULTS] = {
    [CELL_OVERVOLT_FLAG]  = OverVoltageRoutine,
    [CELL_UNDERVOLT_FLAG] = UnderVoltageRoutine,
    [OVERTEMP_FLAG]       = FansMax,
};

//----------------------------------------------------------------------------------------------------
// Fault data <-> bytes. Always big endian on the wire, so a different processor changes nothing.
static void Pack(const BMSFaultsData_t* const f, uint8_t* const out){
    write_u16_BE(&out[0], f->Slave_Faults);
    write_u16_BE(&out[2], f->BMS_Faults);
    out[4] = f->IMDState;
    out[5] = f->IsolationState;
    out[6] = f->FirstFault;
    out[7] = f->TripCount;
}
static void Unpack(const uint8_t* const in, BMSFaultsData_t* const f){
    f->Slave_Faults   = read_u16_BE(&in[0]);
    f->BMS_Faults     = read_u16_BE(&in[2]);
    f->IMDState       = in[4];
    f->IsolationState = in[5];
    f->FirstFault     = in[6];
    f->TripCount      = in[7];
}

bool Fault_SendCAN(void){
    const BMSFaultsData_t copy = Fault_Get();
    uint8_t buf[FAULT_CAN_BYTES];
    Pack(&copy, buf);
    return can_transmit_data(BMS2ALL_FAULT, buf, sizeof(buf));
}
bool Fault_ReceiveCAN(BMSFaultsData_t* const data){
    uint8_t buf[FAULT_CAN_BYTES];
    const bool ok = can_receive_data(BMS2ALL_FAULT, buf, sizeof(buf));
    if (ok){
        Unpack(buf, data);
    }
    return ok;
}

//----------------------------------------------------------------------------------------------------
bool Fault_AnyBMS(void){
    return Faults.BMS_Faults != 0u;
}
bool Fault_AnySlave(void){
    return Faults.Slave_Faults != 0u;
}
bool Fault_Any(void){
    return Fault_AnyBMS() || Fault_AnySlave();
}
bool Fault_IsBMSSet(const BMS_Faults fault){
    return (fault < NUM_BMS_FAULTS) && BIT_IS_SET(Faults.BMS_Faults, fault);
}
bool Fault_IsSlaveSet(const Slave_Faults fault){
    return (fault < NUM_SLAVE_FAULTS) && BIT_IS_SET(Faults.Slave_Faults, fault);
}
uint16_t Fault_GetBMSFlags(void){
    return Faults.BMS_Faults;
}
uint16_t Fault_GetSlaveFlags(void){
    return Faults.Slave_Faults;
}
BMSFaultsData_t Fault_Get(void){
    FAULT_LOCK();
    const BMSFaultsData_t copy = Faults;
    FAULT_UNLOCK();
    return copy;
}

//----------------------------------------------------------------------------------------------------
void Fault_Handler(void){
    if (handling){      // something in here raised a fault again, the line is already low
        return;
    }
    handling = true;

    gioSetBitHelper(GIO_BMS_FAULT_BIT, false);    // first, nothing before it can stop it
    Charger_FaultShutdown();
    Fault_SendCAN();

    handling = false;
}

//----------------------------------------------------------------------------------------------------
// Sets the bits and remembers which fault came first
static void Record(uint16_t* const flags, const uint16_t mask, const uint8_t source){
    FAULT_LOCK();
    if (!Fault_Any()){
        Faults.FirstFault = source;
    }
    if (((*flags & mask) != mask) && (Faults.TripCount < UINT8_MAX)){
        Faults.TripCount++;
    }
    *flags |= mask;
    FAULT_UNLOCK();
}

static uint8_t LowestBit(const uint16_t mask){
    uint8_t bit = 0u;
    while (((mask >> bit) & 1u) == 0u){
        bit++;
    }
    return bit;     // mask must not be 0
}

void Fault_RaiseBMS(const BMS_Faults fault){
    if (fault >= NUM_BMS_FAULTS){
        return;
    }
    Record(&Faults.BMS_Faults, (uint16_t)BIT(fault), (uint8_t)fault);
    Fault_Handler();

    if (BMSFaultRoutines[fault] != NULL){
        BMSFaultRoutines[fault]();
    }
}

void Fault_RaiseSlaveMask(const uint16_t mask){
    const uint16_t valid = mask & (uint16_t)MASK_LOW_BITS(NUM_SLAVE_FAULTS);
    if (valid == 0u){
        return;
    }
    Record(&Faults.Slave_Faults, valid, FIRST_FAULT_SLAVE_BIT | LowestBit(valid));
    Fault_Handler();

    if (BIT_IS_SET(valid, BAD_TEMP_flags)){
        FansMax();
    }
}
void Fault_RaiseSlave(const Slave_Faults fault){
    if (fault < NUM_SLAVE_FAULTS){
        Fault_RaiseSlaveMask((uint16_t)BIT(fault));
    }
}

void Fault_RaiseIMD(const IMDStateEnum state, const uint8_t isolation){
    FAULT_LOCK();
    Faults.IMDState       = (uint8_t)state;
    Faults.IsolationState = isolation;
    FAULT_UNLOCK();

    Fault_RaiseBMS(IMD_FAULT_FLAG);
}

//----------------------------------------------------------------------------------------------------
static void ReleaseLineIfClear(void){
    if (!Fault_Any()){
        gioSetBitHelper(GIO_BMS_FAULT_BIT, true);
    }
}

void Fault_ClearBMS(const BMS_Faults fault){
    if (fault >= NUM_BMS_FAULTS){
        return;
    }
    FAULT_LOCK();
    Faults.BMS_Faults &= (uint16_t)~BIT(fault);
    FAULT_UNLOCK();
    ReleaseLineIfClear();
}
void Fault_ClearSlave(const uint16_t mask){
    FAULT_LOCK();
    Faults.Slave_Faults &= (uint16_t)~mask;
    FAULT_UNLOCK();
    ReleaseLineIfClear();
}
void Fault_ClearAll(void){
    FAULT_LOCK();
    Faults.Slave_Faults   = 0u;
    Faults.BMS_Faults     = 0u;
    Faults.IMDState       = Normal_Condition;
    Faults.IsolationState = ISOLATION_UNKNOWN;
    Faults.FirstFault     = FIRST_FAULT_NONE;
    Faults.TripCount      = 0u;
    FAULT_UNLOCK();
    ReleaseLineIfClear();
}

//----------------------------------------------------------------------------------------------------
void Debug_GIO_Notification(void){
    Fault_RaiseBMS(DEBUG_FLAG);
}
void IMD_FAULT_GIO_Notification(void){
    Fault_RaiseIMD(Undefined_Fault, ISOLATION_UNKNOWN);
}

void Fault_Init(void){
    Fault_ClearAll();

    const bool imdPinHigh = gioGetBitHelper(GIO_IMD_FAULT_BIT) & 1;     // IMD pulls it low on a fault
    if (!imdPinHigh){
        IMD_FAULT_GIO_Notification();
    }
}
