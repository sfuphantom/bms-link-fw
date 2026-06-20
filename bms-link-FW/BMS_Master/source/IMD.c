/*
 * IMD.c
 *
 *  Created on: Apr 25, 2026
 *      Author: joeyw
 */

#include "IMD.h"

unsigned int NumberofCharsFreq, freq_value, NumberofCharsDuty, duty_value;
float period, time_on, frequency, duty_cycle, IMD_resistance;
static const float FREQ_RUNNING_COUNTER = 10000000.00; // 10Mhz

IsolationStateEnum isolationState;
IMDData_t IMDData = {Normal_Condition};
IMDStateEnum currentState;

void updateIMDState(unsigned int freq_value, unsigned int duty_value);

void initalizeIMD(){
  /*
    hetInit();    //Initialized in phantomSystemInit()
//    gioInit();    //Initialized in phantomSystemInit()
//    rtiInit();    //Previously commented
//    sciInit();    //Previously commented; Initialized in phantomSystemInit()
//    rtiResetCounter(rtiCOUNTER_BLOCK1);   //Previously commented

//    _enable_IRQ();    //Previously commented; Initialized in phantomSystemInit()

    edgeEnableNotification(hetREG1, edge0);
    edgeEnableNotification(hetREG1, edge1);
    edgeEnableNotification(hetREG1, edge2);
    edgeEnableNotification(hetREG1, edge3);
    edgeEnableNotification(hetREG1, edge4);
    edgeEnableNotification(hetREG1, edge5);
    edgeEnableNotification(hetREG1, edge6);
    edgeEnableNotification(hetREG1, edge7);
    //gioEnableNotification(gioPORTA,5);
    //gioEnableNotification(gioPORTA,6);
//    rtiStartCounter(rtiCOUNTER_BLOCK1); // cant read register without this (RTI doesnt start?)    //Previously commented
*/

    etpwmSetClkDiv(etpwmREG1, ClkDiv_by_1, HspClkDiv_by_1);

    /* Set the time period as 1000 ns (Divider value = (1000ns * 90MHz) - 1 = 89)*/
    etpwmSetTimebasePeriod(etpwmREG1, 89);

    /* Configure Compare A value as half the time period */
    etpwmSetCmpA(etpwmREG1, 45);

    /* Configure mthe module to set PWMA value as 1 when CTR=0 and as 0 when CTR=CmpA  */
    etpwmActionQualConfig_t configPWMA;
    configPWMA.CtrEqZero_Action = ActionQual_Set;
    configPWMA.CtrEqCmpAUp_Action = ActionQual_Clear;
    configPWMA.CtrEqPeriod_Action = ActionQual_Disabled;
    configPWMA.CtrEqCmpADown_Action = ActionQual_Disabled;
    configPWMA.CtrEqCmpBUp_Action = ActionQual_Disabled;
    configPWMA.CtrEqCmpBDown_Action = ActionQual_Disabled;
    etpwmSetActionQualPwmA(etpwmREG1, configPWMA);

    /* Start counter in CountUp mode */
    etpwmSetCount(etpwmREG1, 0);
    etpwmSetCounterMode(etpwmREG1, CounterMode_Up);
    etpwmStartTBCLK();

    /* Configure ECAP1 */
    /* Configure Event 1 to Capture the rising edge */
    ecapSetCaptureEvent1(ecapREG1, RISING_EDGE, RESET_DISABLE);

    /* Configure Event 2 to Capture the falling edge */
    ecapSetCaptureEvent2(ecapREG1, FALLING_EDGE, RESET_DISABLE);

    /* Configure Event 3 to Capture the rising edge with reset counter enable */
    ecapSetCaptureEvent3(ecapREG1, RISING_EDGE, RESET_ENABLE);

    /* Set Capure mode as Continuous and Wrap event as CAP3  */
    ecapSetCaptureMode(ecapREG1, CONTINUOUS, CAPTURE_EVENT3);

    /* Start counter */
    ecapStartCounter(ecapREG1);

    /* Enable Loading on Capture */
    ecapEnableCapture(ecapREG1);

    /* Enable Interrupt for CAP3 event */
    ecapEnableInterrupt(ecapREG1, ecapInt_CEVT3);

}

void updateIsolationState(unsigned int duty_value){
    if (duty_value >= 5 && duty_value <10) isolationState = Normal; //PWM is between 5-10%
    else if (duty_value >= 10 && duty_value <30) isolationState = Normal_75; //PWM is between 10-30%
    else if (duty_value >= 30 && duty_value <60) isolationState = Normal_50; //PWM is between 30-60%
    else if (duty_value >= 60 && duty_value <90) isolationState = Normal_25; //PWM is between 60-90%
    else if (duty_value >= 90 && duty_value <=95) isolationState = Isolation_Failure; //PWM is between 90-95%
    else isolationState = Unknown;

    IMDData.IsolationState = isolationState;
}

void updateIMDData(){
    // adding this 0.5 and then typecasting to an int (truncating all decimals)
    // basically acts as rounding the float to the nearest integer
    freq_value = (unsigned int) (frequency + 0.5);
    duty_value = (unsigned int) (duty_cycle*100.0 + 0.5);

    updateIMDState(freq_value,duty_value);
    updateIsolationState(duty_value);
}

void updateIMDState(unsigned int freq_value, unsigned int duty_value){
    if (freq_value <=5) currentState = Short_Circuit; //0Hz
    else if (freq_value > 5 && freq_value <=15) currentState = Normal_Condition; //10Hz, PWM is between 5-95%
    else if (freq_value > 15 && freq_value <=25) currentState = Undervoltage_Condition; //20Hz, PWM is between 5-95%
    else if (freq_value > 25 && freq_value <=35) //30Hz, PWM is between 5-10% (good) or 90-95% (bad)
    {
        if(duty_value >= 5 && duty_value <= 10) currentState = Speed_Start_Measurement_Good;
        else if(duty_value >= 90 && duty_value <= 95) currentState = Speed_Start_Measurement_Bad;
        else currentState = Undefined_Fault;
    }
    else if (freq_value > 35 && freq_value <=45 && duty_value >= 47 && duty_value <= 53) currentState = Device_Error; //40Hz, PWM is between 47.5-52.5%
    else if (freq_value > 45 && freq_value <=55 && duty_value >= 47 && duty_value <= 53) currentState = Connection_Fault_Earth; //50Hz, PWM is between 47.5-52.5%
    else currentState = Undefined_Fault; //Freq range outside known values

    IMDData.IMDState = currentState;
}

IMDData_t getIMDData(){
    return IMDData;
}

