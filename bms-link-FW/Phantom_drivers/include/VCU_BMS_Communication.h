/*
 * VCU_BMS_Communication.h
 *
 *  Created on: Aug 25, 2026
 *      Author: tanjo
 */

#ifndef PHANTOM_DRIVERS_INCLUDE_VCU_BMS_COMMUNICATION_H_
#define PHANTOM_DRIVERS_INCLUDE_VCU_BMS_COMMUNICATION_H_


//----------------------------------------------------------------------------------------------------

typedef struct{
    uint8_t start: 1;
    uint8_t chargeNdischarge: 1;
    uint8_t CLEAR_FAULTS: 1;
    uint8_t has_fault;

}VCU2BMS_t;
typedef union {
    VCU2BMS_t f;
    uint8_t buf[sizeof(VCU2BMS_t)];
}VCU2BMS_buf_t;
//----------------------------------------------------------------------------------------------------

typedef struct{
    uint16_t BatteryVolts;
    uint16_t BatterySOC;
    uint16_t BatteryCurrent;
}BMS2VCU_t;

typedef union {
    BMS2VCU_t f;
    uint8_t buf[sizeof(BMS2VCU_t)];
}BMS2VCU_buf_t;
//----------------------------------------------------------------------------------------------------


#endif /* PHANTOM_DRIVERS_INCLUDE_VCU_BMS_COMMUNICATION_H_ */
