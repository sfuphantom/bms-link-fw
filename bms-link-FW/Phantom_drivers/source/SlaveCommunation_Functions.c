/*
 * SlaveCommunation_Functions.c
 *
 *  Created on: Jun 7, 2026
 *      Author: tanjo
 */


#include <stdint.h>
#include <stdbool.h>
#include "string.h"
#include "ltc6811_commands.h"
#include "SlaveCommunation_Hardware.h"
#include "spi.h"
#include "spi_helpers.h"
#include "SlaveCommunication_Drivers.h"
#include "SlaveCommunation_Functions.h"
#include "PhantomHelpers.h"
#include "PhantomTimers.h"
#include "Fault_handler.h"

static ConfigReg_t ConfigRegWriteData[NUMBER_OF_SLAVE_BOARDS_TOTAL];
//ConfigReg ConfigRegReadData[NUMBER_OF_SLAVE_BOARDS];
static StatusReg_t StatusRegData[NUMBER_OF_SLAVE_BOARDS_TOTAL];
static uint16_t SlaveStatusFaults = 0u;

//---------------------------------------------------------------------------------------------------------
static float Slave_ADC2Volt(const uint16_t ADCVaule){
    const float Volts = SLAVE_ADC2VOLTS(ADCVaule);
    return Volts;
}
static void ADC2Volts_arr(const uint16_t ADCVaule_in[], float Volts_out[], const size_t NumberOfElements){
    int i;
    for(i=0;i<NumberOfElements;i++){
        Volts_out[i] = Slave_ADC2Volt(ADCVaule_in[i]);
    }
}
float Slave_ADC2Celcius(const uint16_t ADC){
    return SLAVE_ADC2TEMP(ADC);
//    float Volts = Slave_ADC2Volt(ADC);
//    float Kelvin = Volts / ITMP_MILLI_VOLTS_2_CELCIUS * 1000;
//    float Celcius = Kelvin - ITMP_KELVIN_2_CELCIUS;
//    return Celcius;
}
float Slave_SC_ADC2Volt(const uint16_t ADC_Word){
    return Slave_ADC2Volt(ADC_Word) * 20;
}
//---------------------------------------------------------------------------------------------------------
const StatusReg_t* GetStatusRegData(){
    return StatusRegData;
}
const ConfigReg_t* GetConfigRegData(){
    return ConfigRegWriteData;
}
//uint16_t GetSlaveStatusFaults(){
//    return SlaveStatusFaults;
//}
//---------------------------------------------------------------------------------------------------------
void Config_Struct2Words(uint16_t* data){
    int i;
    uint16_t data16[WORDS_PER_REG_GROUP];
    uint8_t data8[BYTES_PER_REG_GROUP];
    for(i = 0; i < NUMBER_OF_SLAVE_BOARDS_TOTAL; i++){
        ConfigReg_t* current_Reg = &ConfigRegWriteData[i];

        // CFGR0: GPIO[4:0] in bits[7:3], REFON in bit2, DTEN in bit1, ADCOPT in bit0
        data8[0] = ((current_Reg->gpio   & 0x1F) << 3U)
                 | ((current_Reg->refon  & 0x01) << 2U)
                 | (0U                           << 1U)   // DTEN read-only, write 0
                 | ((current_Reg->adcopt & 0x01) << 0U);

        // CFGR1: VUV[7:0]
        data8[1] =  (current_Reg->VUV & 0xFF);

        // CFGR2: VOV[3:0] in bits[7:4], VUV[11:8] in bits[3:0]
        data8[2] = ((current_Reg->VOV & 0x0F) << 4U)
                 | ((current_Reg->VUV >> 8U)  & 0x0F);

        // CFGR3: VOV[11:4]
        data8[3] =  (current_Reg->VOV >> 4U) & 0xFF;

        // CFGR4: DCC[8:1]
        data8[4] =  (current_Reg->DCC & 0xFF);

        // CFGR5: DCTO[3:0] in bits[7:4], DCC[12:9] in bits[3:0]
        data8[5] = ((current_Reg->dcto & 0x0F) << 4U)
                 | ((current_Reg->DCC  >> 8U)  & 0x0F);

        bytes_to_u16_BE(data8, data16, WORDS_PER_REG_GROUP);
        memcpy(data, data16, WORDS_PER_REG_GROUP * sizeof(uint16_t));
        data += WORDS_PER_REG_GROUP;
    }
}
void Config_Words2Struct(const uint16_t* data){
    int i, idx;
    uint8_t byteLow, byteHigh;
    uint8_t vuv_lo;

    for(i = 0, idx = 0; i < NUMBER_OF_SLAVE_BOARDS_TOTAL; i++){
        ConfigReg_t* current_Reg = &ConfigRegWriteData[i];

        // word[0] = CFGR1(high byte) | CFGR0(low byte)
        // CFGR0 = GPIO5..GPIO1 | REFON | DTEN | ADCOPT
        // CFGR1 = VUV[7:0]
        word2bytes(data[idx++], &byteLow, &byteHigh);
        current_Reg->adcopt = (byteLow >> 0U) & 0x01;
        current_Reg->DTEN   = (byteLow >> 1U) & 0x01;
        current_Reg->refon  = (byteLow >> 2U) & 0x01;
        current_Reg->gpio   = (byteLow >> 3U) & 0x1F;
        vuv_lo = byteHigh;   // save CFGR1 = VUV[7:0] for later

        // word[1] = CFGR3(high byte) | CFGR2(low byte)
        // CFGR2 = VOV[3:0] in bits[7:4], VUV[11:8] in bits[3:0]
        // CFGR3 = VOV[11:4]
        word2bytes(data[idx++], &byteLow, &byteHigh);
        current_Reg->VUV = ((uint16_t)(byteLow & 0x0F) << 8U)   // VUV[11:8]
                         |  (uint16_t)vuv_lo;                    // VUV[7:0]
        current_Reg->VOV = ((uint16_t)byteHigh          << 4U)   // VOV[11:4]
                         | ((uint16_t)(byteLow >> 4U) & 0x0F);   // VOV[3:0]

        // word[2] = CFGR5(high byte) | CFGR4(low byte)
        // CFGR4 = DCC[8:1]
        // CFGR5 = DCTO[3:0] in bits[7:4], DCC[12:9] in bits[3:0]
        word2bytes(data[idx++], &byteLow, &byteHigh);
        current_Reg->DCC  = ((uint16_t)(byteHigh & 0x0F) << 8U)  // DCC[12:9]
                          |  (uint16_t)byteLow;                   // DCC[8:1]
        current_Reg->dcto = (byteHigh >> 4U) & 0x0F;             // DCTO[3:0]
    }
}

void Stat_Words2Struct(uint16_t* data){
    uint32_t UO_Flags;
    uint8_t byteLow, byteHigh;
    uint8_t flag;
    int cell, i, idx;

    for(i = 0, idx = 0; i < NUMBER_OF_SLAVE_BOARDS_TOTAL; i++){
        StatusReg_t* current_Reg = &StatusRegData[i];

        // STATA:
        // word[0] = SC   (sum of all cells raw ADC value, multiply by 20*100uV)
        // word[1] = ITMP (die temperature raw ADC value)
        // word[2] = VA   (analog supply VREG raw ADC value)
        current_Reg->SC   = data[idx++];
        current_Reg->ITMP = data[idx++];
        current_Reg->VA   = data[idx++];

        // STATB:
        // word[3] = VD   (digital supply VREGD raw ADC value)
        // word[4] = STBR3(high byte) | STBR2(low byte)
        // word[5] = STBR5(high byte) | STBR4(low byte)
        //
        // STBR2: C4OV|C4UV|C3OV|C3UV|C2OV|C2UV|C1OV|C1UV
        // STBR3: C8OV|C8UV|C7OV|C7UV|C6OV|C6UV|C5OV|C5UV
        // STBR4: C12OV|C12UV|C11OV|C11UV|C10OV|C10UV|C9OV|C9UV
        // each cell pair: bit(2*cell+0)=CxUV, bit(2*cell+1)=CxOV
        current_Reg->VD = data[idx++];

        UO_Flags  = (uint32_t)data[idx++];                        // STBR3|STBR2
        UO_Flags |= (uint32_t)(data[idx] & 0x00FF) << 16U;       // STBR4 low byte

        // STBR5: REV[3:0] | RSVD | RSVD | MUXFAIL | THSD
        word2bytes(data[idx++], &byteLow, &byteHigh);
        // byteLow  = STBR4 (already consumed above)
        // byteHigh = STBR5

        current_Reg->OV_flags = 0;
        current_Reg->UV_flags = 0;

        for(cell = 0; cell < CELLS_PER_SLAVE_BOARD; cell++){
            flag = (UO_Flags >> (2*cell + 0)) & 0x1;   // UV flag for this cell
            current_Reg->UV_flags |= (uint16_t)flag << cell;

            flag = (UO_Flags >> (2*cell + 1)) & 0x1;   // OV flag for this cell
            current_Reg->OV_flags |= (uint16_t)flag << cell;
        }

        // STBR5 bit layout: REV[3:0] in bits[7:4], RSVD in bits[3:2],
        //                   MUXFAIL in bit[1], THSD in bit[0]
        current_Reg->THSD    = (byteHigh >> 0U) & 0x01;
        current_Reg->MUXFAIL = (byteHigh >> 1U) & 0x01;
        current_Reg->REV     = (byteHigh >> 4U) & 0x0F;
    }
}

//---------------------------------------------------------------------------------------------------------
void Write_CFGR(){
      uint16_t WriteConfig[NUMBER_OF_CONFIG_WORDS_SLAVES];

      const uint16_t cmds_W[NUMBER_OF_CONFIG_REG_GROUPS_PER_BOARD] = {LTC6811_WRCFGA};

      Config_Struct2Words(WriteConfig);

      WriteMultiRegGroups(cmds_W, NUMBER_OF_CONFIG_REG_GROUPS_PER_BOARD, WriteConfig);
      return;
 }
 bool Read_CFGR(){
     uint16_t raw[NUMBER_OF_CONFIG_WORDS_SLAVES];
     uint16_t cmds[NUMBER_OF_CONFIG_REG_GROUPS_PER_BOARD] = {LTC6811_RDCFGA};

     bool PecEq = ReadMultiRegGroups(cmds, NUMBER_OF_CONFIG_REG_GROUPS_PER_BOARD, raw);

     if(PecEq){
         Config_Words2Struct(raw);
         return true;
     }

    return false ;
 }

 bool WriteThenRead_CFGR(){
       bool PecEq, RW_EQ;
       uint16_t WriteConfig[NUMBER_OF_CONFIG_WORDS_SLAVES];
       uint16_t ReadConfig [NUMBER_OF_CONFIG_WORDS_SLAVES];

       uint16_t* WriteDataPrt = WriteConfig;
       uint16_t* ReadDataPrt  = ReadConfig;

       const uint16_t cmds_W[NUMBER_OF_CONFIG_REG_GROUPS_PER_BOARD] = {LTC6811_WRCFGA};
       const uint16_t cmds_R[NUMBER_OF_CONFIG_REG_GROUPS_PER_BOARD] = {LTC6811_RDCFGA};

       Config_Struct2Words(WriteConfig);

       int i,j;


       for(i=0; i<NUMBER_OF_FAILS_ALLOWED; i++){
           WriteMultiRegGroups(cmds_W, NUMBER_OF_CONFIG_REG_GROUPS_PER_BOARD, WriteConfig);
           PecEq = ReadMultiRegGroups(cmds_R, NUMBER_OF_CONFIG_REG_GROUPS_PER_BOARD, ReadConfig);


           for(j=0; j<NUMBER_OF_CONFIG_WORDS_SLAVES; j++){
               WriteDataPrt[0]  &= ~(0x0002);
               ReadDataPrt[0]   &= ~(0x0002);

               WriteDataPrt     += NUMBER_OF_CONFIG_WORDS_SLAVES;
               ReadDataPrt      += NUMBER_OF_CONFIG_WORDS_SLAVES;
           }

           RW_EQ = arr_u16_equal(WriteConfig, ReadConfig, NUMBER_OF_CONFIG_WORDS_SLAVES);
           if(RW_EQ && PecEq){
               return true;
           }
       }
       return false;
  }
 bool Read_STAT(){
     uint16_t raw[NUMBER_OF_STAT_WORDS_SLAVES];
     uint16_t cmds[NUMBER_OF_STAT_REG_GROUPS_PER_BOARD] = {LTC6811_RDSTATA, LTC6811_RDSTATB};

     bool PecEq = ReadMultiRegGroups(cmds, NUMBER_OF_STAT_REG_GROUPS_PER_BOARD, raw);

     Stat_Words2Struct(raw);
     int i;
     bool AllValid = TRUE;
     for(i=0; i<NUMBER_OF_STAT_WORDS_SLAVES; i+=WORDS_PER_REG_GROUP){
         AllValid &= !arr_u16_all(&raw[i], SPI_DUMMY_DATA_WORD, isEqual, WORDS_PER_REG_GROUP);
     }
     return (PecEq & AllValid);
 }
//---------------------------------------------------------------------------------------------
 void ReadConfig_DCC(uint16_t* DCC){
    int i;
    for(i=0; i<NUMBER_OF_SLAVE_BOARDS_TOTAL; i++){
         ConfigReg_t* current_Reg = &ConfigRegWriteData[i];

         DCC[i] = current_Reg->DCC;
    }
}
void SetConfig_DCC(const uint16_t* DCC){
    int i;
    for(i=0; i<NUMBER_OF_SLAVE_BOARDS_TOTAL; i++){
         ConfigReg_t* current_Reg = &ConfigRegWriteData[i];

          current_Reg->DCC = DCC[i] & 0xFFF;
    }
}
void SetAllConfig_DCC(const uint16_t DCC){
    int i;
    for(i=0; i<NUMBER_OF_SLAVE_BOARDS_TOTAL; i++){
         ConfigReg_t* current_Reg = &ConfigRegWriteData[i];
         current_Reg->DCC = DCC & 0xFFF;
    }
}
void SetAllHigh_DCC(const uint16_t DCC){
    int i;
    for(i=0; i<NUMBER_OF_SLAVE_BOARDS_TOTAL; i++){
         ConfigReg_t* current_Reg = &ConfigRegWriteData[i];
         current_Reg->DCC |= DCC & 0xFFF;
    }
}
void ReadConfig_gpio(uint8_t* gpio){
    int i;
    for(i=0; i<NUMBER_OF_SLAVE_BOARDS_TOTAL; i++){
         ConfigReg_t* current_Reg = &ConfigRegWriteData[i];

         gpio[i] = current_Reg->gpio;
    }
}
bool ReadConfig_gpio_allZero(){
    int i;
    bool allZero = TRUE;
    for(i=0; i<NUMBER_OF_SLAVE_BOARDS_TOTAL; i++){
         ConfigReg_t* current_Reg = &ConfigRegWriteData[i];

         allZero &= !(current_Reg->gpio);
    }
    return allZero;
}
void SetConfig_gpio(const uint8_t* gpio){
    int i;
    for(i=0; i<NUMBER_OF_SLAVE_BOARDS_TOTAL; i++){
         ConfigReg_t* current_Reg = &ConfigRegWriteData[i];

          current_Reg->gpio = gpio[i];
    }
}
//---------------------------------------------------------------------------------------------------------
void getStatus_SC(float * SC){
    const StatusReg_t* current_Slave = GetStatusRegData();
    int i;
    for(i=0;i<NUMBER_OF_SLAVE_BOARDS_TOTAL; i++, current_Slave++){
        SC[i] = Slave_SC_ADC2Volt(current_Slave->SC);
    }
}
void getStatus_BoardTempC(float * BoardTempC){
    const StatusReg_t* current_Slave = GetStatusRegData();
    int i;
    for(i=0;i<NUMBER_OF_SLAVE_BOARDS_TOTAL; i++, current_Slave++){
        BoardTempC[i] = Slave_ADC2Celcius(current_Slave->ITMP);
    }
}
void getStatus_RefVolt2nd(float * RefVolt2nd){
    const StatusReg_t* current_Slave = GetStatusRegData();
    int i;
    for(i=0;i<NUMBER_OF_SLAVE_BOARDS_TOTAL; i++, current_Slave++){
        RefVolt2nd[i] = Slave_ADC2Volt(current_Slave->RefVolt2nd);
    }
}
//---------------------------------------------------------------------------------------------------------
static void slitAUX_GIO_Ref2nd(const uint16_t * AUXData, uint16_t * GPIO_Data, uint16 * RefVolt2nd){
    uint8_t i,j;
    uint8_t idx = 0;
    for(i=0;i<NUMBER_OF_SLAVE_BOARDS_TOTAL; i++){
        for(j=0;j<GPIOS_PER_SLAVE_BOARD; j++){
            *GPIO_Data = AUXData[idx++];
            GPIO_Data++;
        }

        *RefVolt2nd = AUXData[idx++];
        RefVolt2nd++;
    }
}
static void setStatus_RefVolt2nd_16(const uint16 * RefVolt2nd){
    int i;
    for(i=0;i<NUMBER_OF_SLAVE_BOARDS_TOTAL; i++){
        StatusRegData[i].RefVolt2nd = RefVolt2nd[i];
    }
}
//---------------------------------------------------------------------------------------------------------
uint32_t checkStatFlags(){
     bool flag;
     uint32_t Flags = 0;

     const StatusReg_t* current_Slave = GetStatusRegData();
     int i;
     for(i=0;i<NUMBER_OF_SLAVE_BOARDS_TOTAL; i++, current_Slave++){


         flag = current_Slave->OV_flags == 0;
         Flags |= (uint32_t)flag<<BAD_OV_flags;
         flag = current_Slave->UV_flags == 0;
         Flags |= (uint32_t)flag<<BAD_UV_flags;
         flag = !current_Slave->THSD;
         Flags |= (uint32_t)flag<<BAD_THSD;
         flag = !current_Slave->MUXFAIL;
         Flags |= (uint32_t)flag<<BAD_MUXFAIL;

#warning Change all compamairaon with either inline function or macro, for readabilty

         flag = current_Slave->ITMP > MAX_INTERNAL_DIE_TEMPERATURE_FLAG;
         Flags |= (uint32_t)flag<<BAD_ITMP;
         flag = current_Slave->ITMP < MIN_INTERNAL_DIE_TEMPERATURE_FLAG;
         Flags |= (uint32_t)flag<<BAD_ITMP;
         flag = current_Slave->VA > MAX_ANALOG_POWER_SUPPLY_VOLTAGE_FLAG;
         Flags |= (uint32_t)flag<<BAD_VA;
         flag = current_Slave->VA < MIN_ANALOG_POWER_SUPPLY_VOLTAGE_FLAG;
         Flags |= (uint32_t)flag<<BAD_VA;
         flag = current_Slave->VD > MAX_DIGITAL_POWER_SUPPLY_VOLTAGE_FLAG;
         Flags |= (uint32_t)flag<<BAD_VD;
         flag = current_Slave->VD < MIN_DIGITAL_POWER_SUPPLY_VOLTAGE_FLAG;
         Flags |= (uint32_t)flag<<BAD_VD;
         flag = (current_Slave->RefVolt2nd > MAX_2ND_REFERENCE_VOLTAGE_FLAG) | (current_Slave->RefVolt2nd < MIN_2ND_REFERENCE_VOLTAGE_FLAG);
         Flags |= (uint32_t)flag<<BAD_REF2ND;
     }



     return Flags;
}

//---------------------------------------------------------------------------------------------------------
 void ClearCellsCMD(){
     SendCMD2Slave_alone(LTC6811_CLRCELL);
 }
 void ClearAUXCMD(){
     SendCMD2Slave_alone(LTC6811_CLRAUX);
 }
 void ClearStatCMD(){
     SendCMD2Slave_alone(LTC6811_CLRSTAT);
 }
 void ClearSCtrlCMD(){
     SendCMD2Slave_alone(LTC6811_CLRSCTRL);
 }

 void ClearSlaveRegs(){
     ClearCellsCMD();
     ClearAUXCMD();
     ClearStatCMD();
     ClearSCtrlCMD();
 }
 //---------------------------------------------------------------------------------------------------------
// bool CheckSTATCmd(const uint8_t MD,     // ADC mode: 0=Fast, 1=Normal, 2=Filtered
//                   const uint8_t ST)    //  Self Test Mode Selection
// {
//
//     uint16_t MD_bits     = ((uint16_t)MD & 0x03) << 7;
//     uint16_t ST_bits     = ((uint16_t)ST & 0x03) << 5;
//     uint16_t cmd = LTC6811_STATST| MD_bits | ST_bits;
//
//     return SendCMD2Slave_pollAndWait(cmd);
////     return SendCMD2Slave_pollAndWait(cmd, POLL_PERIOD_STAT_US);
//
// }
 bool MeasureSTATCmd(const uint8_t MD,      // ADC mode: 0=Fast, 1=Normal, 2=Filtered
                     const uint8_t CHST)    //  Status Group Selection
 {

     uint16_t MD_bits     = ((uint16_t)MD & 0x03) << 7;
     uint16_t ST_bits     = CHST & 0x7;
     uint16_t cmd = LTC6811_ADSTAT| MD_bits | ST_bits;

     return SendCMD2Slave_pollAndWait(cmd);
 }

 bool MeasureSTATCmd_All(const uint8_t MD)     // ADC mode: 0=Fast, 1=Normal, 2=Filtered
 {
     ClearStatCMD();
     return MeasureSTATCmd(MD, 0);
//     const uint16_t MD_bits     = ((uint16_t)MD & 0x03) << 7;
//     const uint16_t ST_bits     = 0;
//     const uint16_t cmd = LTC6811_ADSTAT| MD_bits | ST_bits;
//
//     ClearStatCMD();
//     return SendCMD2Slave_pollAndWait(cmd);
//     return SendCMD2Slave_pollAndWait(cmd, POLL_PERIOD_STAT_US);
 }

 bool MeasureCellsCmd(const uint8_t MD  , // DC mode: 0=Fast, 1=Normal, 2=Filtered
                      const bool DCP    , // Discharge Permit
                      const uint8_t CHG ) // Cell Selection for ADC Conversion
 {

     const uint16 CHG_bits     = CHG & 0x07;
     const uint16 DCP_bits     = DCP ? 0x0010 : 0x0000;
     const uint16_t MD_bits    = ((uint16_t)MD & 0x03) << 7;
     const uint16_t cmd = LTC6811_ADCV | MD_bits | DCP_bits | CHG_bits;

//     SendCMD2Slave_alone(cmd);
     return SendCMD2Slave_pollAndWait(cmd);
 }

 bool MeasureCellsCmd_All_NoDis(const uint8_t MD) // DC mode: 0=Fast, 1=Normal, 2=Filtered
 {
     ClearCellsCMD();
     return MeasureCellsCmd(MD, FALSE, 0);
//     const uint16 CHG_bits     = 0;
//     const uint16 DCP_bits     = 0x0000;
//     const uint16_t MD_bits    = ((uint16_t)MD & 0x03) << 7;
//     const uint16_t cmd = LTC6811_ADCV | MD_bits | DCP_bits | CHG_bits;
//
//     ClearCellsCMD();
//     return SendCMD2Slave_pollAndWait(cmd);
//     return SendCMD2Slave_pollAndWait(cmd, POLL_PERIOD_CELL_VOLTS_US);
 }
 bool MeasureCellsCmd_Dis(const uint8_t MD  , // DC mode: 0=Fast, 1=Normal, 2=Filtered
                          const uint8_t CHG ) // Cell Selection for ADC Conversion
 {

     return MeasureCellsCmd(MD, TRUE, CHG);
 }

 bool MeasureAUXCmd(const uint8_t MD,     // ADC mode: 0=Fast, 1=Normal, 2=Filtered
                    const uint8_t CHG)    //  GPIO Selection for ADC Conversion
 {
     uint16 CHG_bits     = CHG & 0x07;
     uint16_t MD_bits    = ((uint16_t)MD & 0x03) << 7;
     uint16_t cmd = LTC6811_ADAX | MD_bits | CHG_bits;

//     SendCMD2Slave_alone(cmd);
     return SendCMD2Slave_pollAndWait(cmd);
 }
 bool MeasureAUXCmd_All(const uint8_t MD)     // ADC mode: 0=Fast, 1=Normal, 2=Filtered
 {
     ClearAUXCMD();
     return MeasureAUXCmd(MD, 0);
//     const uint16 CHG_bits     = 0;
//     const uint16_t MD_bits    = ((uint16_t)MD & 0x03) << 7;
//     const uint16_t cmd = LTC6811_ADAX | MD_bits | CHG_bits;
//
//     ClearAUXCMD();
//     return SendCMD2Slave_pollAndWait(cmd);
//     return SendCMD2Slave_pollAndWait(cmd, POLL_PERIOD_AUX_US);
 }
 bool Start_S_CTRL_Pulsing(){
     return SendCMD2Slave_pollAndWait(LTC6811_STSCTRL);
//     return SendCMD2Slave_pollAndWait(LTC6811_STSCTRL, POLL_PERIOD_CELL_BAL_US);
 }
 //---------------------------------------------------------------------------------------------------------
void Write_S_CTRL(const uint8* nibbles){
    WriteBytes2RegGroup(LTC6811_WRSCTRL, nibbles);
}
bool Read_S_CTRL(uint8* nibbles){
    return ReadBytesFromRegGroup(LTC6811_RDSCTRL, nibbles);
}
void Write_PWM(const uint8* nibbles){
    WriteBytes2RegGroup(LTC6811_WRPWM, nibbles);
}
bool Read_PWM(uint8* nibbles){
    return ReadBytesFromRegGroup(LTC6811_RDPWM, nibbles);
}
bool WriteThenRead_PWM(const uint8* nibbles){
    return WriteThenReadRegGroup_Bytes(LTC6811_WRPWM, LTC6811_RDPWM, nibbles);
}
bool SetAllPWM_Regs(uint8_t nibble){
    nibble &= 0xF;
    nibble |= nibble<<4;
    uint8_t nibbles[NUMBER_OF_CELLS_SLAVES/2];
    memset(nibbles, nibble, NUMBER_OF_CELLS_SLAVES/2);
    return WriteThenRead_PWM(nibbles);
}
 //---------------------------------------------------------------------------------------------------------
 bool GetVoltageReadings(uint16_t* data){
     const uint16_t cmds[NUMBER_OF_CELL_VOLTAGE_REG_GROUPS_PER_BOARD] = {LTC6811_RDCVA, LTC6811_RDCVB, LTC6811_RDCVC, LTC6811_RDCVD};

     bool PecEq = ReadMultiRegGroups(cmds, NUMBER_OF_CELL_VOLTAGE_REG_GROUPS_PER_BOARD, data);

     return PecEq;
 }
 bool GetGPIOReadings_Analog(uint16_t* data){
      uint16_t cmds[NUMBER_OF_GPIO_VOLTAGE_REG_GROUPS_PER_BOARD] = {LTC6811_RDAUXA,LTC6811_RDAUXB};

      bool PecEq = ReadMultiRegGroups(cmds, NUMBER_OF_GPIO_VOLTAGE_REG_GROUPS_PER_BOARD, data);

      return PecEq;
  }
// bool GetGPIOReadings_Digital(uint8_t *gpio_data)
// {
//     Read_CFGR();
//
//     int i;
//     for(i=0;i<NUMBER_OF_SLAVE_BOARDS_TOTAL; i++){
//         ConfigReg_t* current_Slaves_ConfigReg = &ConfigRegData[i];
//
//         gpio_data[i] = current_Slaves_ConfigReg->gpio;
//     }
//
//     return TRUE;
// }
//---------------------------------------------------------------------------------------------------------

 void initConfig(){
     const bool     adcopt  = FALSE;
     const bool     DTEN    = true;
     const bool     refon   = TRUE;
     const uint8_t  gpio    = 0b11111;
     const uint16_t DCC     = 0x000;//0b0000111111111110;//0xFFF;
     const uint8_t  dcto    = 0x10;

     int i;
     for(i = 0; i < NUMBER_OF_SLAVE_BOARDS_TOTAL; i++){
         ConfigReg_t* current_Reg = &ConfigRegWriteData[i];

         current_Reg->adcopt    = adcopt;
         current_Reg->DTEN      = DTEN  ;
         current_Reg->refon     = refon ;
         current_Reg->gpio      = gpio  ;
         current_Reg->DCC       = DCC   ;
         current_Reg->dcto      = dcto  ;
         current_Reg->VUV       = UNDER_VOLTAGE_CONFIG;
         current_Reg->VOV       = OVER_VOLTAGE_CONFIG;
     }

     Write_CFGR();
  }
//---------------------------------------------------------------------------------------------------------
void SendDummyCMD(){
    SendCMD2Slave_alone(DUMMY_CMD);
}
void waitDummyCMD(const uint32_t WaitPeriod_ms, const uint32_t WaitPeriod_us, uint16_t WaitSends){
    for(;WaitSends!=0;WaitSends--){
        SendCMD2Slave_alone(DUMMY_CMD);
        delay_ms_us(WaitPeriod_ms,WaitPeriod_us);
    }
}
//---------------------------------------------------------------------------------------------------------
void initLink(){
    init_PEC15_Table();
    //------------------------------------------------
    wakeup_sleep();

    ClearSlaveRegs();

    SetAllPWM_Regs(0x0);
    initConfig();
}
void wakeup_sleep_and_SetConfig(){
    wakeup_sleep();
    Write_CFGR();
}
//---------------------------------------------------------------------------------------------------------
bool MeasureCellVoltage_16(uint16_t* CellVolt){
   bool cmdDone = MeasureCellsCmd_All_NoDis(ADC_MEASURE_MODE);
   bool PecEQ = GetVoltageReadings(CellVolt);

   bool AllValid = !arr_u16_any(CellVolt, SPI_DUMMY_DATA_WORD, isEqual, NUMBER_OF_CELLS_SLAVES);

   return cmdDone && AllValid && PecEQ;
}

bool MeasureRef2ndVoltage() {
    ClearAUXCMD();
    MeasureAUXCmd(ADC_MEASURE_MODE, 6);

    uint16_t AUXDataOut[NUMBER_OF_AUCILIARY_SLAVES];
    uint16_t GPIO_data_buf[NUMBER_OF_GPIOS_SLAVES];
    uint16_t RefVolt2nd_Data[NUMBER_OF_REF_2ND_SLAVES];

    GetGPIOReadings_Analog(AUXDataOut);
    ///////////////////////////////////////////////////////////
    slitAUX_GIO_Ref2nd(AUXDataOut, GPIO_data_buf, RefVolt2nd_Data);
    setStatus_RefVolt2nd_16(RefVolt2nd_Data);
    ///////////////////////////////////////////////////////////
    bool AllValid = !arr_u16_any(AUXDataOut, SPI_DUMMY_DATA_WORD, isEqual, NUMBER_OF_AUCILIARY_SLAVES);

    return AllValid;
 }

 bool MeasureAUXVoltage16(uint16_t* GPIO_Data){
     uint32_t cmdDone = MeasureAUXCmd_All(ADC_MEASURE_MODE);

     uint16_t AUXDataOut[NUMBER_OF_AUCILIARY_SLAVES];
     uint16_t RefVolt2nd_Data[NUMBER_OF_REF_2ND_SLAVES];

     bool PecEq = GetGPIOReadings_Analog(AUXDataOut);

     bool AllValid = !arr_u16_any(AUXDataOut, SPI_DUMMY_DATA_WORD, isEqual, NUMBER_OF_AUCILIARY_SLAVES);

     if(!((cmdDone != 0) && AllValid && PecEq)){
         return false;
     }
     ///////////////////////////////////////////////////////////
     slitAUX_GIO_Ref2nd(AUXDataOut, GPIO_Data, RefVolt2nd_Data);
     setStatus_RefVolt2nd_16(RefVolt2nd_Data);
     return true;
 }
 bool ReadStatAndGetFlags(){
      const uint32_t cmdDone = MeasureSTATCmd_All(ADC_MEASURE_MODE);

      const bool Stat_Valid = Read_STAT();

      const bool Valid = (cmdDone != 0) && Stat_Valid;
      if(!Valid){
          return false;
      }

     return true;
  }

 //---------------------------------------------------------------------------------------------------------
 bool MeasureCellVoltage(float* CellVolt, const size_t NumberOfRealValues){
     uint16_t CellVolt16[NUMBER_OF_CELLS_SLAVES];
     const bool vaild = MeasureCellVoltage_16(CellVolt16);

     ADC2Volts_arr(CellVolt16, CellVolt, NumberOfRealValues);
     return vaild;
 }
 bool MeasureAUXVoltage(float* GPIO_Data, const size_t NumberOfRealValues){
     uint16_t GPIO_Data16[NUMBER_OF_GPIOS_SLAVES];
     const bool vaild = MeasureAUXVoltage16(GPIO_Data16);

     ADC2Volts_arr(GPIO_Data16, GPIO_Data, NumberOfRealValues);
     return vaild;
 }
 //---------------------------------------------------------------------------------------------------------
