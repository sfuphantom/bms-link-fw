/*
 * IMD.c
 */
#include <math.h>
#include "IMD.h"
#include "BatteryData.h"
#include "Fault_handler.h"
#include "PhantomHelpers.h"

// Frequency picks the state (10 Hz steps, bounds are the middle between them). The duty only
// matters for the 30/40/50 Hz states. Duty limits are the datasheet ones with some tolerance.
IMDStateEnum IMD_CalcState(const float duty, const float freq){
    if (freq < 5.0f)  return Short_Circuit;             //  0 Hz
    if (freq < 15.0f) return Normal_Condition;          // 10 Hz
    if (freq < 25.0f) return Undervoltage_Condition;    // 20 Hz

    if (freq < 35.0f){                                  // 30 Hz
        if (isBetween_CC(duty,  5.0f, 10.5f)) return Speed_Start_Measurement_Good;
        if (isBetween_CC(duty, 90.0f, 95.5f)) return Speed_Start_Measurement_Bad;
        return Undefined_Fault;
    }
    if (freq < 55.0f){                                  // 40 Hz and 50 Hz
        if (!isBetween_CC(duty, 47.0f, 53.5f)) return Undefined_Fault;
        return (freq < 45.0f) ? Device_Error : Connection_Fault_Earth;
    }
    return Undefined_Fault;
}

uint8_t IMD_CalcIsolation(const float duty){
    if (duty < 5.0f || duty > 95.0f){
        return ISOLATION_UNKNOWN;
    }
    return (uint8_t)roundf(100.53763f - 1.05287f * duty);   // fit of the datasheet curve
}

float IMD_CalcResistance(const float duty){
    if (duty <= 5.0f){
        return IMD_RESISTANCE_MAX_KOHM;
    }
    if (duty >= 95.0f){
        return 0.0f;
    }
    const float kohm = 90.0f * 1200.0f / (duty - 5.0f) - 1200.0f;   // datasheet equation
    return fminf(kohm, IMD_RESISTANCE_MAX_KOHM);
}

IMDData_t IMD_CalcData(const float duty, const float freq){
    IMDData_t data;
    data.duty       = duty;
    data.freq       = freq;
    data.resistance = IMD_CalcResistance(duty);
    data.isolation  = IMD_CalcIsolation(duty);
    return data;
}

//---------------------------------------------------------------------------------------------------------
bool IMD_HasFault(const IMDData_t* const data, const IMDStateEnum state){
    // only these two states are OK, change here if undervoltage should not be a fault
    if (state != Normal_Condition && state != Speed_Start_Measurement_Good){
        return true;
    }
    return  data->isolation  < MIN_IMD_ISOLATION_PERCENT ||
            data->isolation  > MAX_IMD_ISOLATION_PERCENT ||     // also catches ISOLATION_UNKNOWN
            data->duty       > MAX_IMD_DUTY_PERCENT      ||
            data->resistance < MIN_IMD_RESISTANCE_KOHM;
}

//---------------------------------------------------------------------------------------------------------
void ecapNotification(ecapBASE_t *ecap, uint16 flags)
{
    // mibspi3miso (X1, pin 5 on the right) <- input
    if (ecap != IMD_ECAP_REG) {return;}                             // change this if there is more than 1 ecap
    if ((flags & VALID_FLAG_MASK) != VALID_FLAG_VAL) {return;}      // all 3 events must have happened

    const uint32 C1 = ecapGetCAP1(ecap);
    const uint32 C2 = ecapGetCAP2(ecap);
    const uint32 C3 = ecapGetCAP3(ecap);

    // unsigned subtraction also works if the counter wrapped
    const uint32 high   = C2 - C1;
    const uint32 period = C3 - C1;
    if (high == 0u || high >= period) {return;}     // bad capture, would divide by 0 or give duty > 100%

    const float duty = 100.0f * (float)high / (float)period;
    const float freq = ECAP_COUNTS_PER_SEC / (float)period;

    const IMDData_t    data  = IMD_CalcData(duty, freq);
    const IMDStateEnum state = IMD_CalcState(duty, freq);

    IMD_SetData(data);

    if (IMD_HasFault(&data, state)){
        Fault_RaiseIMD(state, (uint8_t)data.isolation);
    }
}
