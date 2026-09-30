/*
 * SendDataSerial.h
 *
 *  Created on: Jul 31, 2026
 *      Author: tanjo
 */

#ifndef PHANTOM_DRIVERS_INCLUDE_SENDDATASERIAL_H_
#define PHANTOM_DRIVERS_INCLUDE_SENDDATASERIAL_H_

#include "SlaveCommunation_Hardware.h"
#include "BatteryCell_Hardware.h"
#include "FullBattery_Hardware.h"

#include "BatteryData.h"
#include "PhantomTimers.h"


#define USE_UART_N_SPI  TRUE

#if USE_UART_N_SPI
//#include "sci_helpers.h"
#include "sci.h"
#include "reg_sci.h"

#define USE_USB_N_PINS TRUE
#if USE_USB_N_PINS
    #define UART_REG        sciREG
#else
    #define UART_REG        scilinREG
#endif

#else

#endif


#define SOH_BYTE 0x01
#define EOT_BYTE 0x04
#define MAX_BUF_SIZE 1024

//---------------------------------------------------------------------------------------------------------
#define CELL_VOLTAGE_MSG_LEN            (sizeof(uint16_t) * NUMBER_OF_CELLS_SERIES)
#define CELL_RESISTANCE_MSG_LEN         (sizeof(uint16_t) * NUMBER_OF_CELLS_SERIES)
#define CELL_TEMP_MSG_LEN               (sizeof(uint16_t) * NUMBER_OF_THERMISTORS_TOTAL)
#define CELL_PWM_MSG_LEN                (sizeof(uint8_t) * NUMBER_OF_CELLS_SERIES/2)

#define IMD_ECAP_DATA_MSG_LEN           (sizeof(ecapIMDData_t))
#define SLAVE_STATUE_MSG_LEN            (sizeof(uint16_t)*NUMBER_OF_SLAVE_BOARDS_TOTAL*3)
#define FAN_MSG_LEN                     (sizeof(uint8_t)*NUMBER_OF_FANS_TOTAL)



//---------------------------------------------------------------------------------------------------------
//#define PERIODIC_MSG_ID_START_IDX SEND_CELL_VOLT
//#define PERIODIC_MSG_ID_END_IDX   SEND_BALC_PWM
//#define NUMBER_OF_PERIODIC_MSG (PERIODIC_MSG_ID_END_IDX - PERIODIC_MSG_ID_START_IDX+1)
//---------------------------------------------------------------------------------------------------------
typedef enum {
                SEND_DEBUG_MSG_DATA,
                SEND_LOG_MSG_DATA,
                SEND_ERROR_MSG_DATA,
                SEND_FAULT_DATA,
                SEND_CANS_LOG,

                SEND_CELL_VOLT,
                SEND_IMD_DATA,
                SEND_CHARGER_DATA,
                SEND_HV_DATA,
                SEND_SOC_DATA,
                SEND_CELL_RES,
                SEND_CELL_TEMP,
                SEND_SLAVE_STATE,
                SEND_FAN_DATA,
                SEND_PWM_DATA,

//                SEND_BATTERY_DATA_SERIAL,
//                SEND_CHARGER_DATA,
            }msg_ID_t;

// ---------------------------------------------------------------------------------------------------
void StartMsg(const msg_ID_t msg_ID);
void endMsg();
void SendDataSerial(const void * data, const uint16_t data_len);

void SendMsgSerial_DL(const msg_ID_t msg_ID, const void * data, const uint16_t data_len);
// ---------------------------------------------------------------------------------------------------
void SendCellVoltage_Serial();
void SendCellTemp_Serial();
void SendFanData_Serial();
void SendIMDData_Serial();
void SendCellRes_Serial();
void SendCellPWM_Serial();
void SendSlaveState_Serial();
void SendChargerData_Serial();
void Send_BMSFaultsData_Serial();
//---------------------------------------------------------------------------------------------------------
void SendPeriodic(const msg_ID_t msg_ID);




#endif /* PHANTOM_DRIVERS_INCLUDE_SENDDATASERIAL_H_ */
