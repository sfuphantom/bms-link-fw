/*
 * BMS_gio.c
 *
 *  Created on: Jun 22, 2026
 *      Author: tanjo
 */

#include "gio.h"
#include "BMS_gio.h"


void GIO_inturupt_Notification(gioPORT_t *port, uint32 bit){

    if(gioPORTA == port){
        switch(bit){
        case START_CHARGING_BIT:

            break;
        }
    }
    else if(gioPORTB == port){

    }
    else{
        /* error*/
    }
}

//---------------------------------------------------------
Gio_State_t read_START_CHARGING_GIO(){
    static bool lastLevel = FALSE;

    const bool NewLevel = (bool)gioGetBit(gioPORTA, START_CHARGING_BIT);

    uint8_t NewState = ((uint8_t)(NewLevel != lastLevel))<<1 | (uint8_t)NewLevel;
    return (Gio_State_t) NewState;
}
//---------------------------------------------------------
void IMD_FALUT_BIT_GIO(Gio_State_t NewState){
    const uint8_t value = (uint8_t)NewState & 1U;

    gioSetBit(gioPORTA, IMD_FALUT_BIT, value);
}
void BMS_FALUT_BIT_GIO(Gio_State_t NewState){
    const uint8_t value = (uint8_t)NewState & 1U;

    gioSetBit(gioPORTA, BMS_FALUT_BIT, value);
}

//---------------------------------------------------------

void reset_BMS_GIO(){
    IMD_FALUT_BIT_GIO(GIO_LOW);
    BMS_FALUT_BIT_GIO(GIO_LOW);
}
void init_BMS_GIO(){
    gioInit();
    reset_BMS_GIO();
}
