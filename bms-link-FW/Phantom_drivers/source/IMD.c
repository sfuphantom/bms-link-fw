/*
 * IMD.c
 */
#include <math.h>
#include "IMD.h"
#include "PhantomAlgorithms.h"
#include "PhantomTimers.h"
#include "IMD_hardware.h"


bool readIMDSignal(float * freq, float * duty){
    const uint32 C1 = ecapGetCAP1(IMD_ECAP_REG);
    const uint32 C2 = ecapGetCAP2(IMD_ECAP_REG);
    const uint32 C3 = ecapGetCAP3(IMD_ECAP_REG);

    // unsigned subtraction also works if the counter wrapped
    const uint32 high   = C2 - C1;
    const uint32 period = C3 - C1;

    if (high == 0u || high >= period){
        return false;
    };

    *duty = 100.0f * (float)high / (float)period;
    *freq = ECAP_COUNTS_PER_SEC  / (float)period;

    return true;
}
//---------------------------------------------------------------------------------------------
void IMD_init(){
    (void)readIMDSignal(NULL, NULL);
}

// Frequency picks the state (10 Hz steps, bounds are the middle between them). The duty only
// matters for the 30/40/50 Hz states. Duty windows are the datasheet ones plus IMD_DUTY_TOL_PERCENT.
IMDStateEnum IMD_CalcState(const float duty, const float freq){
    if (freq < IMD_FREQ_UP_SHORT_HZ)  return Short_Circuit;             //  0 Hz
    if (freq < IMD_FREQ_UP_NORMAL_HZ) return Normal_Condition;          // 10 Hz
    if (freq < IMD_FREQ_UP_UNDERV_HZ) return Undervoltage_Condition;    // 20 Hz

    if (freq < IMD_FREQ_UP_SST_HZ){                                     // 30 Hz
        if (isBetween_CC(duty, IMD_DUTY_SST_GOOD_MIN, IMD_DUTY_SST_GOOD_MAX)) return Speed_Start_Measurement_Good;
        if (isBetween_CC(duty, IMD_DUTY_SST_BAD_MIN,  IMD_DUTY_SST_BAD_MAX))  return Speed_Start_Measurement_Bad;
        return Undefined_Fault;
    }
    if (freq < IMD_FREQ_UP_EARTH_HZ){                                   // 40 Hz and 50 Hz
        if (!isBetween_CC(duty, IMD_DUTY_ERR_MIN, IMD_DUTY_ERR_MAX)) return Undefined_Fault;
        return (freq < IMD_FREQ_UP_DEVICE_ERR_HZ) ? Device_Error : Connection_Fault_Earth;
    }
    return Undefined_Fault;
}

uint8_t IMD_CalcIsolation(const float duty){
    if (duty < IMD_DS_PWM_DUTY_MIN_PERCENT || duty > IMD_DS_PWM_DUTY_MAX_PERCENT){
        return ISOLATION_UNKNOWN;
    }
    return (uint8_t)roundf(IMD_ISOLATION_FIT_OFFSET - IMD_ISOLATION_FIT_SLOPE * duty);   // not a datasheet value
}

float IMD_CalcResistance(const float duty){
    if (duty <= IMD_DS_PWM_DUTY_MIN_PERCENT){
        return IMD_RESISTANCE_MAX_kOHM;
    }
    if (duty >= IMD_DS_PWM_DUTY_MAX_PERCENT){
        return 0.0f;
    }
    return fminf(IMD_DUTY_TO_RES_kOHM(duty), IMD_RESISTANCE_MAX_kOHM);   // datasheet equation
}

//IMDData_t IMD_CalcData(const float duty, const float freq){
//    IMDData_t data;
//    data.duty       = duty;
//    data.freq       = freq;
//    data.resistance = IMD_CalcResistance(duty);
//    data.isolation  = IMD_CalcIsolation(duty);
//    return data;
//}

//---------------------------------------------------------------------------------------------------------
bool IMD_HasFault(const float duty, const float freq, const float resistance, const float isolation, const IMDStateEnum state){
    // only these two states are OK, change here if undervoltage should not be a fault
    if (state != Normal_Condition && state != Speed_Start_Measurement_Good){
        return true;
    }
    return  isolation  < MIN_IMD_ISOLATION_PERCENT ||
            isolation  > MAX_IMD_ISOLATION_PERCENT ||     // also catches ISOLATION_UNKNOWN
            duty       > MAX_IMD_DUTY_PERCENT      ||
            resistance < IMD_FAULT_RESISTANCE_kOHM;    // MIN_IMD_RESISTANCE_kOHM (per volt) x max TS voltage
}

//---------------------------------------------------------------------------------------------------------
