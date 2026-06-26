/*
 * Fault_handler.c
 *
 *  Created on: Jun 23, 2026
 *      Author: tanjo
 */


#include "gio.h"
#include "can.h"
#include "sci.h"

#include "Fault_handler.h"


//#include "batteryData.h"
//#include "charger.h"

static uint8_t gio_past_level;
//--------------------------------------------------------------------------
Gio_State_t gioGetBitHelper(const uint8_t bit){
//     const typeof(gio_past_level) bitMask = 1U << bit;

     const bool NewLevel = (bool)gioGetBit(GIO_PORT_A, bit);

     bool LastLevel;
     gio_past_level = GetAndInsertBit(gio_past_level, bit, NewLevel, &LastLevel);

     const uint8_t NewState = (uint8_t)NewLevel | ((uint8_t)(NewLevel ^ LastLevel)<<1U);

     return (Gio_State_t)NewState;
}

Gio_State_t gioSetBitHelper(const uint8_t bit, const Gio_State_t NewState){
//     const typeof(gio_past_level) bitMask = 1U << bit;

     const bool NewLevel = (bool)NewState & 1U;
     gioSetBit(GIO_PORT_A, bit, NewLevel);

     bool LastLevel;

     gio_past_level = GetAndInsertBit(gio_past_level, bit, NewLevel, &LastLevel);

     const uint8_t State = (uint8_t)NewLevel | ((uint8_t)(NewLevel ^ LastLevel)<<1U);

     return (Gio_State_t)State;
}
Gio_State_t gioToggleBitHelper(const uint8_t bit){
//     const typeof(gio_past_level) bitMask = 1U << bit;

    gioToggleBit(GIO_PORT_A, bit);

    bool NewLevel;
    gio_past_level = InvertAndInsertBit(gio_past_level, bit, &NewLevel);

     const uint8_t State = (uint8_t)NewLevel | (1U)<<1U;

     return (Gio_State_t)State;
}
//-------------------------------------------------------------------

//-------------------------------------------------------------------
void IMD_Fault_Handler(){

}
void Slave_Fault_Handler(){

}
void BMS_Fault_Handler(){

}
//-------------------------------------------------------------------
void init_BMS_Faults();
void reset_BMS_GIO();
