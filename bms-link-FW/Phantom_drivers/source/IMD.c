/** @file sys_main.c 
*   @brief Application main file
*   @date 21-May-2020
* */
#include "IMD.h"
#include "Fault_handler.h"
#include "BatteryData.h"

#define SERIAL_SEND

// Global Variables
//static const float FREQ_RUNNING_COUNTER = 10000000.00; // 10Mhz
//static const uint32_t UINT32_MAX_ = 4294967295;
//static uint32_t last_time1, time1, time2;
//float period, time_on, frequency, duty_cycle, IMD_resistance;

// SCI variables
//unsigned int NumberofCharsFreq, freq_value, NumberofCharsDuty, duty_value;
//unsigned char freq_data[8];
//unsigned char duty_data[8];
//const float f_HCLK = 160.00;


//IMDData_t IMDData = {Normal_Condition};
//ecapIMDData_t ecapIMDData;

//IMDStateEnum currentState;
//IsolationStateEnum isolationState;

/*
* Fn: initalizeIMD
* Purpose: Initializes all functions required to make reading the IMD work
*/
void initalizeIMD(){
//    IMDData.IMDState        = Normal_Condition;
//    IMDData.IsolationState  = Normal;

//    ecapIMDData.duty        = 0;
//    ecapIMDData.freq        = 0;
//    ecapIMDData.resistance  = 0;

}

/*
* Fn: updateIMDState
* Purpose: Message Mapping for IMD Operation State
* To-do: Need to find what the tolerance is for frequency when testing (i.e. is it +- 5hz?)
*/
//void updateIMDState(unsigned int freq_value, unsigned int duty_value){
//
//
//    IMDStateEnum currentState;
//
//    if (freq_value <=5) currentState = Short_Circuit; //0Hz
//    else if (freq_value > 5 && freq_value <=15) currentState = Normal_Condition; //10Hz, PWM is between 5-95%
//    else if (freq_value > 15 && freq_value <=25) currentState = Undervoltage_Condition; //20Hz, PWM is between 5-95%
//    else if (freq_value > 25 && freq_value <=35) //30Hz, PWM is between 5-10% (good) or 90-95% (bad)
//    {
//        if(duty_value >= 5 && duty_value <= 10) currentState = Speed_Start_Measurement_Good;
//        else if(duty_value >= 90 && duty_value <= 95) currentState = Speed_Start_Measurement_Bad;
//        else currentState = Undefined_Fault;
//    }
//    else if (freq_value > 35 && freq_value <=45 && duty_value >= 47 && duty_value <= 53) currentState = Device_Error; //40Hz, PWM is between 47.5-52.5%
//    else if (freq_value > 45 && freq_value <=55 && duty_value >= 47 && duty_value <= 53) currentState = Connection_Fault_Earth; //50Hz, PWM is between 47.5-52.5%
//    else currentState = Undefined_Fault; //Freq range outside known values
//
////    return currentState;
////    IMDData.IMDState = currentState;
//}


/*
*   Fn: updateIsolationState
*   Purpose: Message Mapping for HV/LV Isolation State
*   Duty Cycle Mapping. If Duty Cycle is anything other than Normal, it means either the isolation is dropping
*   or that something is wrong with the IMD
*   ex. If IMD State is Normal or in UnderVoltage Condition and the IsolationState, that's fine
*   ex. But if IMD State is Normal and Duty Cycle is anything other than Normal, there is an isolation problem
*/
//void updateIsolationState(unsigned int duty_value){
//
//      IsolationStateEnum isolationState;
//
//      if (duty_value >= 5 && duty_value <10) isolationState = Normal; //PWM is between 5-10%
//      else if (duty_value >= 10 && duty_value <30) isolationState = Normal_75; //PWM is between 10-30%
//      else if (duty_value >= 30 && duty_value <60) isolationState = Normal_50; //PWM is between 30-60%
//      else if (duty_value >= 60 && duty_value <90) isolationState = Normal_25; //PWM is between 60-90%
//      else if (duty_value >= 90 && duty_value <=95) isolationState = Isolation_Failure; //PWM is between 90-95%
//      else isolationState = Unknown;
//
//      IMDData.IsolationState = isolationState;
//}

/*
* Fn: updateIMDData
* Purpose: Updates the message statuses for what is inside the IMDData Struct
*/

//void updateIMDDataLocal(float64 freq, float64 duty)    {//same as above, just without global variable
//        // adding this 0.5 and then typecasting to an int (truncating all decimals)
//        // basically acts as rounding the float to the nearest integer
//
//        unsigned int freq_value = (unsigned int) (freq + 0.5);
//        unsigned int duty_value = (unsigned int) (duty*100.0 + 0.5);
//
//        updateIMDState(freq_value,duty_value);
//        updateIsolationState(duty_value);
//    }


IMDStateEnum CalcIMDState(const ecapIMDData_t ecapIMDData){

    const uint8_t freq = ecapIMDData.freq;
    const uint8_t duty = (ecapIMDData.duty+1)>>2;

    IMDStateEnum currentState;

    if (freq <=5) currentState = Short_Circuit; //0Hz
    else if (freq > 5 && freq <=15) currentState = Normal_Condition; //10Hz, PWM is between 5-95%
    else if (freq > 15 && freq <=25) currentState = Undervoltage_Condition; //20Hz, PWM is between 5-95%
    else if (freq > 25 && freq <=35) //30Hz, PWM is between 5-10% (good) or 90-95% (bad)
    {
        if(duty >= 5 && duty <= 10) currentState = Speed_Start_Measurement_Good;
        else if(duty >= 90 && duty <= 95) currentState = Speed_Start_Measurement_Bad;
        else currentState = Undefined_Fault;
    }
    else if (freq > 35 && freq <=45 && duty >= 47 && duty <= 53) currentState = Device_Error; //40Hz, PWM is between 47.5-52.5%
    else if (freq > 45 && freq <=55 && duty >= 47 && duty <= 53) currentState = Connection_Fault_Earth; //50Hz, PWM is between 47.5-52.5%
    else currentState = Undefined_Fault; //Freq range outside known values

    return currentState;
}
IsolationStateEnum CalcIsolationState(const uint8_t duty_value){

    const uint8_t duty = (duty_value+1)>>2;

      IsolationStateEnum isolationState;

      if (duty >= 5 && duty <10) isolationState = Normal; //PWM is between 5-10%
      else if (duty >= 10 && duty <30) isolationState = Normal_75; //PWM is between 10-30%
      else if (duty >= 30 && duty <60) isolationState = Normal_50; //PWM is between 30-60%
      else if (duty >= 60 && duty <90) isolationState = Normal_25; //PWM is between 60-90%
      else if (duty >= 90 && duty <=95) isolationState = Isolation_Failure; //PWM is between 90-95%
      else isolationState = Unknown;

      return isolationState;
}
IMDData_t CalcIMDData(const ecapIMDData_t ecapIMDData)    {//same as above, just without global variable
        // adding this 0.5 and then typecasting to an int (truncating all decimals)
        // basically acts as rounding the float to the nearest integer

//        unsigned int freq_value = (unsigned int) (freq + 0.5);
//        unsigned int duty_value = (unsigned int) (duty*100.0 + 0.5);

        IMDData_t IMDData;

        IMDData.IMDState = CalcIMDState(ecapIMDData);
        IMDData.IsolationState = CalcIsolationState(ecapIMDData.duty);

        return IMDData;
    }


/*
* Fn: getIMDData
* Purpose: Getter function for reading IMDData
* Returns: IMDData Struct
*/
//IMDData_t getIMDData(){
//    return IMDData;
//}

//ecapIMDData_t getecapIMDData(){
//    return ecapIMDData;
//}
//TODO: Check if this is set to 500 Ohms or higher according to EV.7.6.3 in the 2024 V1 rulebook


uint32 CalcIMDResistance(const uint8_t duty){//same as above, just without global variable, outputs uint
    // Equation from data sheet correlating duty cycle with resistance
   const float duty_val = duty>>2;
   float IMD_resistance = 90.0*1200.0/(duty_val - 5.0)-1200.0;
   uint32 IMD_R_Uint;
   if (IMD_resistance < 0)
       IMD_R_Uint = (uint32)(-IMD_resistance);
   else
       IMD_R_Uint = (uint32)(IMD_resistance);

   return IMD_R_Uint;
}
//---------------------------------------------------------------------------------------------------------

//uint32_t getIMDResistance(){
//    return ecapIMDData.resistance;
//}
void Sendfault_IMD(const IMDData_t * const IMDdata){
    SetIMDFaults(IMDdata);
}

//---------------------------------------------------------------------------------------------------------
void ecapNotification(ecapBASE_t *ecap, uint16 flags)
{
/*  enter user code between the USER CODE BEGIN and USER CODE END. */
/* USER CODE BEGIN (51) */

    // mibspi3miso (X1, pin 5 on the right) <- input

    if (ecap != IMD_ECAP_REG) {return;} // change this is there is more then 1 ecap
    if (flags & VALID_FLAG_MASK != VALID_FLAG_VAL) {return;} // double check flags

    const uint32 C1 = ecapGetCAP1(ecap);
    const uint32 C2 = ecapGetCAP2(ecap);
    const uint32 C3 = ecapGetCAP3(ecap);

    if (!(C2>C1 && C3>C2)){ return; }//if event 1 is >= event 2 or event 2 >= event 3,
    //leave the function, the reading are invaded and it would cause the program to go to dabort if you don't

    const uint32_t period_scale = (C3 - C1);
    const float freq_scale      = 1.0f/period_scale;
    const float duty            = freq_scale * (C2 - C1);
    const float freq            = freq_scale * ecap_sec2counts;

//    updateIMDData()

//    ecapIMDData.resistance  = CalcIMDResistance(duty);

    ecapIMDData_t ecapIMDData;

    ecapIMDData.duty = (duty * 200);
    ecapIMDData.freq = (freq);

    IMDData_t IMDData = CalcIMDData(ecapIMDData);
    Sendfault_IMD(&IMDData);

    //see what is wrong in helcogen, I should not need these functions
//    ecapResetCAP(ecap);


/* USER CODE END */
}
