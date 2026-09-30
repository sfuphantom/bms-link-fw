/*
 * VCU_BMS_Communication.h
 *
 *  Created on: Aug 25, 2026
 *      Author: tanjo
 */

#ifndef PHANTOM_DRIVERS_INCLUDE_VCU_BMS_COMMUNICATION_H_
#define PHANTOM_DRIVERS_INCLUDE_VCU_BMS_COMMUNICATION_H_


//----------------------------------------------------------------------------------------------------

typedef enum {CLAER_FAULTS, CHARGE, START, } VCU_FLAGS_bits;

typedef struct{
    uint8_t VCU_FLAGS;
}VCU2BMS_t;

//----------------------------------------------------------------------------------------------------

typedef struct{
    uint16_t BatteryVolts;
    uint16_t BatterySOC;
    uint16_t BatteryCurrent;
}BMS2VCU_t;

//----------------------------------------------------------------------------------------------------


#endif /* PHANTOM_DRIVERS_INCLUDE_VCU_BMS_COMMUNICATION_H_ */
