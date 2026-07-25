///*
// * hv_charger.c
// *
// *  Created on: Jul 2, 2026
// *      Author: tanjo
// */
//
//#include "spi.h"
//#include "spi_drivers.h"
//
//#include "BatteryData.h"
//#include "HV_Data.h"
//#include "IMD.h"
//#include "Phantom_Can.h"
//#include "Fault_handler.h"
//
//
//
//uint16_t Get_HV_Data_Raw(){
//    setCS(LOW, 5);
//    uint16_t Data = SPI_SR2Link_WORD(SPI_DUMMY_DATA_WORD);
//    setCS(HIGH, 5);
//    return Data;
//}
//
//bool HV_Data_Routine(){
//    const uint16_t HV_DataRaw = Get_HV_Data_Raw();
//
//    if(HV_DataRaw == SPI_DUMMY_DATA_WORD){return false;}
//
//    const uint16_t HV_Volts = HV_DataRaw;
//
//    Set_HV_Voltage(HV_Volts);
//
//    bool can_vaild = transmit_BMS2VCU_Data(HV_Volts, getIMDResistance());
//
////    if(HV_Volts > 410 || HV_Volts < 260){
////        SetBMSFault_bool_HIGH(BAD_HV_VOLT_FLAG);
////    }
//
//    return true;
//}

/*
 * hv_charger.c
 *
 *  Created on: Jul 2, 2026
 *      Author: tanjo
 */

#include "spi.h"
#include "spi_helpers.h"

#include "BatteryData.h"
#include "HV_Data.h"
#include "IMD.h"
#include "Phantom_Can.h"
#include "Fault_handler.h"


//  bool transmit_BMS2VCU_Data(HV_Volts, IMD_Data);

/* ------------------------------------------------------------------------
 * ADS7044 raw-word -> voltage conversion
 *
 * SPI frame (see ADS7044 datasheet, Fig. 1 / Fig. 35):
 *   bit1..bit2   = 0, 0                 (leading zeros)
 *   bit3..bit14  = D11 .. D0            (12-bit twos-complement code)
 *   bit15..bit16 = 0, 0                 (SDO stays low after 14th SCLK)
 *
 * So on a 16-clock transfer, the shift register (MSB-first) layout is:
 *   [15:14] = 00, [13:2] = 12-bit code, [1:0] = 00
 * ------------------------------------------------------------------------ */
uint16_t Get_HV_Data_Raw(){
    setCS(LOW, HV_CS_PIN_ID);

    uint16_t Data =   SPI_SR2Link_WORD_FAST(SPI_DUMMY_DATA_WORD);

    uint16_t Data2 =  SPI_SR2Link_WORD_FAST(SPI_DUMMY_DATA_WORD);

    setCS(HIGH, HV_CS_PIN_ID);
    return Data;
}

float HV_ADC2VOLTS(const uint16_t HV_ADC){
    const float Volts = (HV_ADC * HV_ADC2VOLTS_SCALE) + HV_ADC2VOLTS_OFFSET;

    return Volts;
}
