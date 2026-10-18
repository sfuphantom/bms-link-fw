/*
 * GetBatteryVoltage.c
 *
 *  Created on: Sep 14, 2026
 *      Author: tanjo
 */


#if USE_HV_BOARD
#include "spi.h"
#include "spi_helpers.h"
#else

#include "SlaveCommunation_Hardware.h"
#include "SlaveCommunation_Functions.h"

#endif

#include "GetBatteryVoltage.h"
#include "FullBattery_Hardware.h"
#include "BatteryData.h"
#include "PhantomHelpers.h"

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
///////////////////////////////////////////////////////////////////////////
#if USE_HV_BOARD
uint16_t Get_HV_Data_Raw(){
    setCS(LOW, HV_CS_PIN_ID);

    uint16_t Data =   SPI_SR2Link_WORD_FAST(SPI_DUMMY_DATA_WORD);

    uint16_t Data2 =  SPI_SR2Link_WORD_FAST(SPI_DUMMY_DATA_WORD);

    setCS(HIGH, HV_CS_PIN_ID);
    return Data;
}

float HV_ADC2VOLTS(const uint16_t HV_ADC){
    const uint16_t HV_DataShifted = HV_ADC>>ADS7044_CODE_SHIFT;
    const float Volts = (HV_ADC * HV_ADC2VOLTS_SCALE) + HV_ADC2VOLTS_OFFSET;

    return Volts;
}
#endif

float MeasureBatteryVoltage(){
    int i;

#if USE_HV_BOARD
#warning there might be a hardware problem, check if it is fix and remove this warning
    const uint16_t HV_DataRaw = Get_HV_Data_Raw();
    const float BatVolt = HV_ADC2VOLTS(HV_DataRaw);
#else
    float BatVolt = 0;
    const StatusReg_t* StatusReg = GetStatusRegData();

    for(i=0;i<NUMBER_OF_SLAVE_BOARDS_TOTAL;i++){
        BatVolt += Slave_SC_ADC2Volt(StatusReg[i].SC);
    }

#endif
    return BatVolt;
}
bool MeasureAndSetVoltage(){
    int i;

#if USE_HV_BOARD
#warning there might be a hardware problem, check if it is fix and remove this warning
    const uint16_t HV_DataRaw = Get_HV_Data_Raw();
    const float BatVolt = HV_ADC2VOLTS(HV_DataRaw);
#else

    float boardSC[NUMBER_OF_SLAVE_BOARDS_TOTAL];
    getStatus_SC(boardSC);

    const float BatVolt = arr_f_sum(boardSC, NUMBER_OF_SLAVE_BOARDS_TOTAL);
#endif
    Pack_SetVolt(BatVolt);

#warning this should use HV board, change that, currently hardware problem
#if USE_HV_BOARD
    return HV_DataRaw != SPI_DUMMY_DATA_WORD;
#else
    return true;
#endif

}
float GetBatteryVoltage(){
    const float BatVolt = Pack_GetVolt();
    return BatVolt;
}
