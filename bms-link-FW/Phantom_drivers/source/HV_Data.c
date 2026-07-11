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
#include "spi_drivers.h"

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

#define ADS7044_CODE_SHIFT     2u
#define ADS7044_CODE_MASK      0x0FFFu
#define ADS7044_SIGN_BIT       0x0800u
#define ADS7044_SIGN_EXTEND    0xF000u

/* TODO: set to the actual AVDD/reference supplied to the ADS7044 (V) */
#define HV_ADC_AVDD_VOLTS      3.3f

/* TODO: set to the actual sense-divider ratio: (Rtop + Rbottom) / Rbottom */
#define HV_DIVIDER_RATIO       100.0f

/**
 * @brief Extract the 12-bit twos-complement ADC code from a raw 16-bit
 *        SPI word read from the ADS7044.
 */
static int16_t ADS7044_ExtractCode(uint16_t rawWord)
{
    uint16_t code = (rawWord >> ADS7044_CODE_SHIFT) & ADS7044_CODE_MASK;

    /* sign-extend the 12-bit twos-complement value to 16 bits */
//    if (code & ADS7044_SIGN_BIT)
//    {
//        code |= ADS7044_SIGN_EXTEND;
//    }

    return (int16_t)code;
}

/**
 * @brief Convert a signed 12-bit ADS7044 code to the voltage present at
 *        the ADC's AINP/AINM inputs.
 *        1 LSB = FSR / 2^12, where FSR = 2 x AVDD (datasheet section 8.3.3).
 */
static float ADS7044_CodeToADCVoltage(int16_t code)
{
    const float lsb = (2.0f * HV_ADC_AVDD_VOLTS) / 4096.0f;
    return (float)code * lsb;
}

/**
 * @brief Convert an ADS7044 input voltage back to the actual HV bus
 *        voltage, undoing the external sense-resistor divider.
 */
static float ADS7044_ADCVoltageToHVVoltage(float adcVolts)
{
    return adcVolts * HV_DIVIDER_RATIO;
}

/**
 * @brief Convert a raw 16-bit SPI word from the ADS7044 directly into
 *        the HV bus voltage, in volts.
 */
float Get_HV_Voltage_Volts(uint16_t rawWord)
{
    const int16_t code    = ADS7044_ExtractCode(rawWord);
    const float   adcVolt = ADS7044_CodeToADCVoltage(code);
    return ADS7044_ADCVoltageToHVVoltage(adcVolt);
}

/**
 * @brief Convert a raw 16-bit SPI word into millivolts (as an integer),
 *        useful if downstream code (CAN messages, fault checks) expects
 *        an integer type instead of float.
 */
uint16_t Get_HV_Voltage_mV(uint16_t rawWord)
{
    const float volts = Get_HV_Voltage_Volts(rawWord);
    return (uint16_t)(volts * 1000.0f);
}

uint16_t Get_HV_Data_Raw(){
    setCS(LOW, 0);
    uint16_t Data =  SPI_SR2Link_14Bit(SPI_DUMMY_DATA_WORD);
    setCS(HIGH, 0);
    return Data;
}

bool HV_Data_Routine(){
    const uint16_t HV_DataRaw = Get_HV_Data_Raw();

    if(HV_DataRaw == SPI_DUMMY_DATA_WORD){return false;}

    const float HV_Volts = Get_HV_Voltage_Volts(HV_DataRaw);

    Set_HV_Voltage(HV_Volts);


    bool can_vaild = transmit_BMS2VCU_Data(HV_Volts, getIMDResistance());

//    if(HV_Volts > 410.0f || HV_Volts < 260.0f){
//        SetBMSFault_bool_HIGH(BAD_HV_VOLT_FLAG);
//    }

    return true;
}
