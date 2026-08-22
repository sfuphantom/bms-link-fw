/** @file sys_main.c 
*   @brief Application main file
*   @date 21-May-2020
* */
#include "IMD.h"
#include "Fault_handler.h"
#include "BatteryData.h"
#include "SendDataSerial.h"

/*
* Fn: initalizeIMD
* Purpose: Initializes all functions required to make reading the IMD work
*/
void initalizeIMD(){

}


inline IMDStateEnum CalcIMDState(const ecapIMDData_t ecapIMDData){

//    const uint8_t freq = (ecapIMDData.freq+2)>>2;
    const uint8_t duty = ecapIMDData.duty;

    switch(ecapIMDData.freq){
        case   0 ...  20: return Short_Circuit;
        case  21 ...  60: return Normal_Condition;
        case  61 ... 100: return Undervoltage_Condition;

        case 101 ... 140: {
            return   (duty >= 10  && duty <= 21)  ? Speed_Start_Measurement_Good : //if
                    ((duty >= 180 && duty <= 191) ? Speed_Start_Measurement_Bad  : Undefined_Fault);  //elif : else
        }
        case 141 ... 180: return (duty >= 94 && duty <= 107) ? Device_Error : Undefined_Fault;
        case 181 ... 220: return (duty >= 94 && duty <= 107) ? Connection_Fault_Earth : Undefined_Fault;
        default: return Undefined_Fault;
    }

}

//IMDStateEnum CalcIMDState(const ecapIMDData_t ecapIMDData){
//
//    const uint8_t freq = (ecapIMDData.freq+2)>>2;
//    const uint8_t duty = (ecapIMDData.duty+1)>>1;
//
//    IMDStateEnum currentState;
//
//    if (freq <=5) currentState = Short_Circuit; //0Hz
//    else if (freq > 5 && freq <=15) currentState = Normal_Condition; //10Hz, PWM is between 5-95%
//    else if (freq > 15 && freq <=25) currentState = Undervoltage_Condition; //20Hz, PWM is between 5-95%
//    else if (freq > 25 && freq <=35) //30Hz, PWM is between 5-10% (good) or 90-95% (bad)
//    {
//        if(duty >= 5 && duty <= 10) currentState = Speed_Start_Measurement_Good;
//        else if(duty >= 90 && duty <= 95) currentState = Speed_Start_Measurement_Bad;
//        else currentState = Undefined_Fault;
//    }
//    else if (freq > 35 && freq <=45 && duty >= 47 && duty <= 53) currentState = Device_Error; //40Hz, PWM is between 47.5-52.5%
//    else if (freq > 45 && freq <=55 && duty >= 47 && duty <= 53) currentState = Connection_Fault_Earth; //50Hz, PWM is between 47.5-52.5%
//    else currentState = Undefined_Fault; //Freq range outside known values
//
//    return currentState;
//}
inline uint8_t CalcIsolationState(const uint8_t duty_value_2){
    // y=-1.05287 * (duty) + 100.53763; << from desmos
    //y=101 - ((duty_value_2+1)>>1); <<close enough

    switch(duty_value_2){ // note this is 2 times the duty cycle, that is why the numbers are doubled
        case  10 ...  19:   return  100;
        case  20 ...  59:   return   75;
        case  60 ... 119:   return   50;
        case 120 ... 179:   return   25;
        case 180 ... 190:   return    0;
        default:            return 0xFF;
    }

}
//IsolationStateEnum CalcIsolationState(const uint8_t duty_value){
//
//    const uint8_t duty = (duty_value+1)>>1;
//
//      IsolationStateEnum isolationState;
//
//      if (duty >= 5 && duty <10) isolationState = Normal; //PWM is between 5-10%
//      else if (duty >= 10 && duty <30) isolationState = Normal_75; //PWM is between 10-30%
//      else if (duty >= 30 && duty <60) isolationState = Normal_50; //PWM is between 30-60%
//      else if (duty >= 60 && duty <90) isolationState = Normal_25; //PWM is between 60-90%
//      else if (duty >= 90 && duty <=95) isolationState = Isolation_Failure; //PWM is between 90-95%
//      else isolationState = Unknown;
//
//      return isolationState;
//}
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
void Sendfault_IMD(const IMDData_t * const IMDdata){
    SetIMDFaults(IMDdata);
}

//---------------------------------------------------------------------------------------------------------
void ReportEcapIMDData(const ecapIMDData_t ecapIMDData){

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


    const uint8_t duty8 = (uint8_t)(200*duty);
    const uint8_t freq8 = (uint8_t)(freq*4);
    const ecapIMDData_t ecapIMDData = {duty8, freq8};

    IMDData_t IMDData = CalcIMDData(ecapIMDData);

    SetEcapIMDData(ecapIMDData);
    ReportEcapIMDData(ecapIMDData);
    Sendfault_IMD(&IMDData);


    SendPeriodic(SEND_IMD_DATA);


    //see what is wrong in helcogen, I should not need these functions
//    ecapResetCAP(ecap);


/* USER CODE END */
}
