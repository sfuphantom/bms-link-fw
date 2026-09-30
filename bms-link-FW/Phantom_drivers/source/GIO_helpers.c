/*
 * GIO_helpers.c
 *
 *  Created on: Jul 24, 2026
 *      Author: tanjo
 */

#include "gio.h"
#include "GIO_helpers.h"
#include "PhantomHelpers.h"
#include "PhantomTimers.h"

//static uint8_t gio_past_level;
//--------------------------------------------------------------------------
bool gioGetBitHelper(const uint8_t bit){
//     const typeof(gio_past_level) bitMask = 1U << bit;

     const bool NewLevel = (bool)gioGetBit(GIO_PORT_A, bit);

     return NewLevel;
//     bool LastLevel;
//     gio_past_level = GetAndInsertBit(gio_past_level, bit, NewLevel, &LastLevel);
//
//     const uint8_t NewState = (uint8_t)NewLevel | ((uint8_t)(NewLevel ^ LastLevel)<<1U);
//
//     return (Gio_State_t)NewState;
}

void gioSetBitHelper(const uint8_t bit, const bool NewState){
//     const typeof(gio_past_level) bitMask = 1U << bit;

     gioSetBit(GIO_PORT_A, bit, NewState);
}
bool gioToggleBitHelper(const uint8_t bit){

    gioToggleBit(GIO_PORT_A, bit);

    return gioGetBitHelper(bit);
}
gioPulse(const uint8_t bit, uint32_t time_us, const bool Val){
    gioSetBitHelper(bit, Val);
    delay_ms_us(0, time_us);
    gioSetBitHelper(bit, !Val);
}


#pragma WEAK(Debug_GIO_Notification)
void Debug_GIO_Notification(){  }

#pragma WEAK(IMD_FAULT_GIO_Notification)
void IMD_FAULT_GIO_Notification(){  }
//void BMS_FAULT_GIO_Notification(){
//
//}
//void START_CHARGING_GIO_Notification(){
//
//}
void gioNotification(gioPORT_t *port, uint32 bit){
    if(port != gioPORTA){return;}

    switch(bit){
        case GIO_DEGUBING_BIT1      : Debug_GIO_Notification(); break;
        case GIO_IMD_FAULT_BIT      : IMD_FAULT_GIO_Notification(); break;
//        case GIO_BMS_FAULT_BIT      : BMS_FAULT_GIO_Notification(); break;
//        case GIO_START_CHARGING_BIT : START_CHARGING_GIO_Notification(); break;
        default: break;
    }
}
//----------------------------------------------------------------------------------------------------
