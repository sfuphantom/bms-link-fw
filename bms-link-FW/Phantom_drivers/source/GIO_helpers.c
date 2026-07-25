/*
 * GIO_helpers.c
 *
 *  Created on: Jul 24, 2026
 *      Author: tanjo
 */

#include "gio.h"
#include "GIO_helpers.h"
#include "PhantomHelpers.h"



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
//     if(NewLevel){
//         hetREG1->DOUT |= (1 << bit);   // set high
//     }
//     else {
//         hetREG1->DOUT &= ~(1 << bit);   // set high
//
//     }

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
