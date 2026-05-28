#include "spi.h"
#include <stdint.h>
#include <stdbool.h>
#include "ltc6811_commands.h"
#include "SlaveCommunication.h"
#include "SlaveCommunication_Services.h"
#include "PhantomHelpers.h"


struct ConfigReg_A ConfigRegData_A[NUMBER_OF_SLAVE_BOARDS];
struct StatusReg_A StatusRegData_A[NUMBER_OF_SLAVE_BOARDS];
struct StatusReg_B StatusRegData_B[NUMBER_OF_SLAVE_BOARDS];

void wakeup_idle(){ //Number of ICs in the system
    int i;
    for (i=0; i<NUMBER_OF_SLAVE_BOARDS; i++){
        setCS(LOW);
        SPI_SR2Link_BYTE(SPI_DUMMY_DATA_BYTE);
        setCS(HIGH);
    }
}
void wakeup_sleep() {
    setCS(LOW);
    int i;
    for (i = 0; i < NUMBER_OF_SLAVE_BOARDS; i++) {
        setCS(LOW);
        SPI_SR2Link_BYTE(SPI_DUMMY_DATA_BYTE);
        delay_ms_us(0, tWAKE_us);
        setCS(HIGH);
        delay_ms_us(0, 10);
    }
}
//---------------------------------------------------------------------------------------------------------
void SetValueConfigReg_A(uint16_t* data, ConfigReg_A_Values Value2Change){
     int i;
     for(i=0;i<NUMBER_OF_SLAVE_BOARDS;i++){
         struct ConfigReg_A* current_Slaves_ConfigReg_A = &ConfigRegData_A[i];

         switch (Value2Change){
             case adcopt:   current_Slaves_ConfigReg_A->adcopt = data[i] & 0x0001; break;
             case DTEN:     current_Slaves_ConfigReg_A->DTEN   = data[i] & 0x0001; break;
             case refon:    current_Slaves_ConfigReg_A->refon  = data[i] & 0x0001; break;
             case gpio:     current_Slaves_ConfigReg_A->gpio   = data[i] & 0x001F; break;
             case VUV:      current_Slaves_ConfigReg_A->VUV    = data[i] & 0x0FFF; break;
             case VOV:      current_Slaves_ConfigReg_A->VOV    = data[i] & 0x0FFF; break;
             case DCC:      current_Slaves_ConfigReg_A->DCC    = data[i] & 0x0FFF; break;
             case dcto:     current_Slaves_ConfigReg_A->dcto   = data[i] & 0x000F; break;
             default:       break;
         }
     }
 }
void GetValueConfigReg_A(uint16_t* data, ConfigReg_A_Values Value2Get){
    int i;
    for(i=0;i<NUMBER_OF_SLAVE_BOARDS;i++){
         struct ConfigReg_A* current_Slaves_ConfigReg_A = &ConfigRegData_A[i];

         switch (Value2Get){
             case adcopt:   data[i] =  current_Slaves_ConfigReg_A->adcopt; break;
             case DTEN:     data[i] =  current_Slaves_ConfigReg_A->DTEN  ; break;
             case refon:    data[i] =  current_Slaves_ConfigReg_A->refon ; break;
             case gpio:     data[i] =  current_Slaves_ConfigReg_A->gpio  ; break;
             case VUV:      data[i] =  current_Slaves_ConfigReg_A->VUV   ; break;
             case VOV:      data[i] =  current_Slaves_ConfigReg_A->VOV   ; break;
             case DCC:      data[i] =  current_Slaves_ConfigReg_A->DCC   ; break;
             case dcto:     data[i] =  current_Slaves_ConfigReg_A->dcto  ; break;
             default:       data[i] =  0; break;
         }
    }
 }
 void SetAllConfigReg_A(bool* adcopt, bool* DTEN, bool* refon, uint8_t* gpio,  uint16_t* VUV, uint16_t* VOV, uint16_t* DCC, uint8_t* dcto){
     int i;
     for(i=0;i<NUMBER_OF_SLAVE_BOARDS;i++){
         struct ConfigReg_A* current_Slaves_ConfigReg_A = &ConfigRegData_A[i];

         current_Slaves_ConfigReg_A->adcopt    = adcopt [i];
         current_Slaves_ConfigReg_A->DTEN      = DTEN   [i];
         current_Slaves_ConfigReg_A->refon     = refon  [i];
         current_Slaves_ConfigReg_A->gpio      = gpio   [i] & 0x1F;
         current_Slaves_ConfigReg_A->DCC       = DCC    [i] & 0x0FFF;
         current_Slaves_ConfigReg_A->dcto      = dcto   [i] & 0xF;
         current_Slaves_ConfigReg_A->VUV       = VUV    [i] & 0x0FFF;
         current_Slaves_ConfigReg_A->VOV       = VOV    [i] & 0x0FFF;
     }
 }
 void Get_STAT_A(uint16_t* data, StatusReg_A_Values Value2Get){
    int i;
    for(i=0;i<NUMBER_OF_SLAVE_BOARDS;i++){
         struct StatusReg_A* current_Slaves_STAT_A = &StatusRegData_A[i];

         switch (Value2Get){
             case SC:     data[i] =  current_Slaves_STAT_A->SC;   break;
             case ITMP:   data[i] =  current_Slaves_STAT_A->ITMP; break;
             case VA:     data[i] =  current_Slaves_STAT_A->VA;   break;
             default:     data[i] =  0; break;
         }
    }
 }
  void Get_STAT_B(uint16_t* data, StatusReg_B_Values Value2Get){
    int i;
    for(i=0;i<NUMBER_OF_SLAVE_BOARDS;i++){
         struct StatusReg_B* current_Slaves_STAT_B = &StatusRegData_B[i];
         switch (Value2Get){
             case VD:           data[i] =  current_Slaves_STAT_B->VD;           break;
             case OV_flags:     data[i] =  current_Slaves_STAT_B->OV_flags;     break;
             case UV_flags:     data[i] =  current_Slaves_STAT_B->UV_flags;     break;
             case THSD:         data[i] =  current_Slaves_STAT_B->THSD;         break;
             case MUXFAIL:      data[i] =  current_Slaves_STAT_B->MUXFAIL;      break;
             case REV:          data[i] =  current_Slaves_STAT_B->REV;          break;
             default:           data[i] =  0; break;
         }
    }
 }
//---------------------------------------------------------------------------------------------------------
//#define BYTE_LENGTH 8;
uint32 Write_CFGR_A(){
          int i, idx;
          uint8_t Config_Bytes[NUMBER_OF_REG_BYTES_PER_CMD] = {0};

          for(i=0, idx=0;i<NUMBER_OF_SLAVE_BOARDS;i++){
              struct ConfigReg_A* current_Slaves_ConfigReg_A = &ConfigRegData_A[i];
              Config_Bytes[idx++] = (current_Slaves_ConfigReg_A->gpio<<3U) | (current_Slaves_ConfigReg_A->refon<<2) | (current_Slaves_ConfigReg_A->DTEN<<1) | (current_Slaves_ConfigReg_A->adcopt<<0U);
              Config_Bytes[idx++] = (current_Slaves_ConfigReg_A->VUV<<0U);
              Config_Bytes[idx++] = (current_Slaves_ConfigReg_A->VOV<<4U)  | (current_Slaves_ConfigReg_A->VUV>>8U);
              Config_Bytes[idx++] = (current_Slaves_ConfigReg_A->VOV>>4U);
              Config_Bytes[idx++] = (current_Slaves_ConfigReg_A->DCC<<0U);
              Config_Bytes[idx++] = (current_Slaves_ConfigReg_A->dcto<<4U)| (current_Slaves_ConfigReg_A->DCC>>8U);
          }

          uint16_t PecEQ = WriteBytes2RegGroup(LTC6811_WRCFGA, Config_Bytes);

     return PecEQ;
 }
 uint32 Read_CFGR_A(){
     uint8_t raw[BYTES_PER_REG_GROUP * NUMBER_OF_SLAVE_BOARDS];
     uint16_t PecEq = ReadBytesFromRegGroup(LTC6811_RDCFGA, raw);
     int i, idx;
     for(i=0, idx=0;i<NUMBER_OF_SLAVE_BOARDS;i++){
         struct ConfigReg_A* current_Slaves_ConfigReg_A = &ConfigRegData_A[i];

         current_Slaves_ConfigReg_A->adcopt    = ((raw[idx]>>0U) & 0x01);
         current_Slaves_ConfigReg_A->DTEN      = ((raw[idx]>>1U) & 0x01);
         current_Slaves_ConfigReg_A->refon     = ((raw[idx]>>2U) & 0x01);
         current_Slaves_ConfigReg_A->gpio      = ((raw[idx]>>3U) & 0x1F);
         idx++;

         current_Slaves_ConfigReg_A->VUV       =  (((raw[idx+1]) & 0x0F) << 8U) | (raw[idx]);
         idx++;

         current_Slaves_ConfigReg_A->VOV       =  ((raw[idx+1]) << 4U) | (((raw[idx]>>4U) & 0x0F));
         idx++;

         current_Slaves_ConfigReg_A->DCC       = ((raw[idx]>>0U) & 0xFF) << 0U;
         idx++;

         current_Slaves_ConfigReg_A->DCC      |= ((raw[idx]>>0U) & 0x0F) << 8U;
         current_Slaves_ConfigReg_A->dcto      = ((raw[idx]>>4U) & 0x0F) << 0U;
         idx++;
     }
    return PecEq;
 }

 uint32_t Read_STATA() {
     uint16_t raw[WORDS_PER_REG_GROUP * NUMBER_OF_SLAVE_BOARDS];
     uint16_t PecEq = ReadRegGroup(LTC6811_RDSTATA, raw);

     int i, idx;
     for (i = 0, idx = 0; i < NUMBER_OF_SLAVE_BOARDS; i++) {
         struct StatusReg_A* current_Slaves_STAT_A = &StatusRegData_A[i];
         current_Slaves_STAT_A->SC   = raw[idx++];
         current_Slaves_STAT_A->ITMP = raw[idx++];
         current_Slaves_STAT_A->VA   = raw[idx++];
     }
     return PecEq;
 }

 uint32_t Read_STATB() {
     uint8_t raw[BYTES_PER_REG_GROUP * NUMBER_OF_SLAVE_BOARDS];
     uint16_t PecEq = ReadBytesFromRegGroup(LTC6811_RDSTATB, raw);

     int i, idx;

     const uint8_t CellPreByte_OV_UV_Flags = 4;
     const uint8_t OV_UV_Flags_Bytes = 3;

     for (i = 0, idx = 0; i < NUMBER_OF_SLAVE_BOARDS; i++) {
         struct StatusReg_B* current_Slaves_STAT_B = &StatusRegData_B[i];
         /* VD: bytes 0-1 */
         current_Slaves_STAT_B->VD = ((uint16_t)raw[idx + 1] << 8) | raw[idx];
         idx += 2;

         /* OV/UV flags packed in bytes 2-4.
          * Each byte: [CnOV | CnUV | C(n-1)OV | C(n-1)UV | ...]
          * STBR2: C4OV C4UV C3OV C3UV C2OV C2UV C1OV C1UV
          * STBR3: C8OV C8UV C7OV C7UV C6OV C6UV C5OV C5UV
          * STBR4: C12OV C12UV C11OV C11UV C10OV C10UV C9OV C9UV
          */
         current_Slaves_STAT_B->OV_flags = 0;
         current_Slaves_STAT_B->UV_flags = 0;
         int byte_idx, cell;
         for (byte_idx = 0; byte_idx < OV_UV_Flags_Bytes; byte_idx++) {
             uint8_t byte = raw[idx];

             for (cell = 0; cell < CellPreByte_OV_UV_Flags; cell++) {
                 uint8_t cell_num = byte_idx * CellPreByte_OV_UV_Flags + cell; /* 0-based, cell 0 = C1 */

                 uint8_t cellFlags = (byte >> (cell*2)) & 0x3;
                 uint8_t cellOVFlag = (cellFlags >> 0) & 0x1;
                 uint8_t cellUVFlag = (cellFlags >> 1) & 0x1;

                 current_Slaves_STAT_B->OV_flags |= cellOVFlag<<cell_num;
                 current_Slaves_STAT_B->UV_flags |= cellUVFlag<<cell_num;
             }

             idx++;
         }

         /* STBR5: REV[3:0] | RSVD | RSVD | MUXFAIL | THSD */
         uint8_t stbr5 = raw[idx];
         current_Slaves_STAT_B->THSD    = (stbr5 >> 0) & 0x01;
         current_Slaves_STAT_B->MUXFAIL = (stbr5 >> 1) & 0x01;
         current_Slaves_STAT_B->REV     = (stbr5 >> 4) & 0x0F;
     }
     return PecEq;
 }
 //---------------------------------------------------------------------------------------------------------
 uint32 MeasureCellsCmd(uint8_t MD  , // DC mode: 0=Fast, 1=Normal, 2=Filtered
                        bool DCP    , // Discharge Permit
                        uint8_t CHG ) // Cell Selection for ADC Conversion
 {

     uint16 CHG_bits     = CHG & 0x07;
     uint16 DCP_bits     = DCP ? 0x0010 : 0x0000;
     uint16_t MD_bits    = (MD & 0x03) << 7;
     uint16_t cmd = LTC6811_ADCV | MD_bits | DCP_bits | CHG_bits;

     return SendCMD2Slave_and_Poll(cmd);
 }

 uint32 MeasureGPIOCmd(uint8_t MD,     // ADC mode: 0=Fast, 1=Normal, 2=Filtered
                       uint8_t CHG)    //  GPIO Selection for ADC Conversion
 {
     uint16 CHG_bits     = CHG & 0x07;
     uint16_t MD_bits    = (MD & 0x03) << 7;
     uint16_t cmd = LTC6811_ADAX | MD_bits | CHG_bits;

     return SendCMD2Slave_and_Poll(cmd);
 }
 //---------------------------------------------------------------------------------------------------------
bool Start_S_CTRL_Pulsing(){
    return SendCMD2Slave_and_Poll(LTC6811_STSCTRL);
}
uint32 Write_S_CTRL(uint8* nibbles){
    return WriteBytes2RegGroup(LTC6811_WRSCTRL, nibbles);
}
uint32 Read_S_CTRL(uint8* nibbles){
    return WriteBytes2RegGroup(LTC6811_RDSCTRL, nibbles);
}
uint32 Write_PWM(uint8* nibbles){
    return WriteBytes2RegGroup(LTC6811_WRPWM, nibbles);
}
uint32 Read_PWM(uint8* nibbles){
    return WriteBytes2RegGroup(LTC6811_RDPWM, nibbles);
}
 //---------------------------------------------------------------------------------------------------------
 bool isConvComplete(void) {
     uint8_t status = 0;
     setCS(LOW);
     SendCmdAndPec2Slave(LTC6811_PLADC);
     status = SPI_SR2Link_BYTE(SPI_DUMMY_DATA_BYTE);
     setCS(HIGH);
     // If any bit is high, conversion is done (SDO is open-drain, driven low only when busy)
     return (status != 0x00);
 }
 bool waitConvComplete(){
     int i;
     bool status;
     uint32_t BytesWaiting = 0;


     setCS(LOW);
     SendCmdAndPec2Slave(LTC6811_PLADC);

     for (i = 0; i < NUMBER_OF_SLAVE_BOARDS; i++){
         status = FALSE;
         while (!status && BytesWaiting < SLAVE_CONVERSATION_TIMEOUT){

             status = SPI_SR2Link_BYTE(SPI_DUMMY_DATA_BYTE) & 0x0001;
             BytesWaiting++;

             delay_ms_us(0,500);
         }
     }
     setCS(HIGH);


     SPI_SR2Link_QWORD(SPI_DUMMY_DATA_QWORD);
     bool TimedOut = BytesWaiting >= SLAVE_CONVERSATION_TIMEOUT;
     return TimedOut;
 }
 bool waitConvComplete_ADC_Cells(){
     return waitConvComplete();
 }
 bool waitConvComplete_ADC_GPIO(){
     return waitConvComplete();
 }
 bool waitConvComplete_Cell_Bal(){
     return waitConvComplete();
 }
 //---------------------------------------------------------------------------------------------------------
 bool GetVoltageReadings(uint16_t* data){
     const uint16_t cmds[NUMBER_OF_CELL_VOLTAGE_REG_GROUPS_PER_BOARD] = {LTC6811_RDCVA, LTC6811_RDCVB, LTC6811_RDCVC, LTC6811_RDCVD};

     ReadMultiRegGroups(cmds, NUMBER_OF_CELL_VOLTAGE_REG_GROUPS_PER_BOARD, data);

     return TRUE;
 }
 bool GetGPIOReadings_Analog(uint16_t* data){
      uint16_t cmds[NUMBER_OF_GPIO_VOLTAGE_REG_GROUPS_PER_BOARD] = {LTC6811_RDAUXA,LTC6811_RDAUXB};

      ReadMultiRegGroups(cmds, NUMBER_OF_GPIO_VOLTAGE_REG_GROUPS_PER_BOARD, data);

      return TRUE;
  }
 bool GetGPIOReadings_Digital(uint8_t *gpio_data)
 {
     Read_CFGR_A();

     int i;
     for(i=0;i<NUMBER_OF_SLAVE_BOARDS; i++){
         struct ConfigReg_A* current_Slaves_ConfigReg_A = &ConfigRegData_A[i];

         gpio_data[i] = current_Slaves_ConfigReg_A->gpio;
     }

     return TRUE;
 }
//---------------------------------------------------------------------------------------------------------
 void ClearSlaveRegs(){
    #define NUMBER_OF_CLEAR_CMDS 3
     uint16_t All_Cear_CMDs[NUMBER_OF_CLEAR_CMDS] = {LTC6811_CLRCELL, LTC6811_CLRAUX, LTC6811_CLRSTAT};

     int i;
     for (i=0;i<NUMBER_OF_CLEAR_CMDS;i++){
         setCS(LOW);
         SendCmdAndPec2Slave(All_Cear_CMDs[i]);
         setCS(HIGH);
     }
 }

 void initLink(){
     init_PEC15_Table();
     //------------------------------------------------
     wakeup_sleep();
     //------------------------------------------------
     const bool adcopt = TRUE;
     const bool DTEN   = TRUE;
     const bool refon  = TRUE;
     const uint8_t gpio = 0b00000;
     const uint16_t VUV = ((Slave_Volt2ADC(3.3)>>4)-1) & 0x0FFF;
     const uint16_t VOV = ((Slave_Volt2ADC(5.0)>>4)  ) & 0x0FFF;
     const uint16_t DCC = 0x000;//0b0000111111111110;//0xFFF;
     const uint8_t dcto = 0x0;
     int i;
     for(i=0;i<NUMBER_OF_SLAVE_BOARDS;i++){
         struct ConfigReg_A* current_Slaves_ConfigReg_A = &ConfigRegData_A[i];

         current_Slaves_ConfigReg_A->adcopt    = adcopt;
         current_Slaves_ConfigReg_A->DTEN      = DTEN  ;
         current_Slaves_ConfigReg_A->refon     = refon ;
         current_Slaves_ConfigReg_A->gpio      = gpio  ;
         current_Slaves_ConfigReg_A->DCC       = DCC   ;
         current_Slaves_ConfigReg_A->dcto      = dcto  ;
         current_Slaves_ConfigReg_A->VUV       = VUV   ;
         current_Slaves_ConfigReg_A->VOV       = VOV   ;
     }
//     SetAllConfigReg_A(adcopt, DTEN, refon, gpio, VUV, VOV, DCC, dcto);
     ClearSlaveRegs();
     Write_CFGR_A();
 }

 //---------------------------------------------------------------------------------------------------------

 bool MeasureCellVoltageRoutine(uint16_t* VoltDataOut){

     MeasureCellsCmd(ADC_Measure_Mode, ADC_Measure_Discharge_Permit, 0);
     uint16_t TimedOut = waitConvComplete_ADC_Cells();

//     GetVoltageReadings(VoltDataOut);
     wakeup_sleep();
     GetVoltageReadings(VoltDataOut);

     return TimedOut;
 }
#if UseAnilog
 bool MeasureGPIOVoltageRoutine(uint16_t* GPIODataOut){
     MeasureGPIOCmd(ADC_Measure_Mode, 0);
     bool TimedOut = waitConvComplete_ADC_GPIO();

     GetGPIOReadings_Analog(GPIODataOut);

     return TimedOut;
 }
#else
 bool MeasureGPIOVoltageRoutine(uint8_t* GPIODataOut){
     GetGPIOReadings_Digital(GPIODataOut);
     return FALSE;
 }
#endif
 bool BalanceCellsRoutine(uint16_t* VoltInData){
    #define S_CRTL FALSE
    #if S_CRTL
         uint8_t S_CLTR_nibbles[NUMBER_OF_CELLS/2];

         GetBalanceNibbles(VoltInData, S_CLTR_nibbles);
         Write_S_CTRL(S_CLTR_nibbles);

         Start_S_CTRL_Pulsing();

         bool TimedOut = waitConvComplete();



         int i;
         for(i=0;i<NUMBER_OF_CELLS/2;i++){
             S_CLTR_nibbles[i]=0;
         }

         Write_S_CTRL(S_CLTR_nibbles);

         return TimedOut;
    #else
         uint16_t DCC_Val[NUMBER_OF_SLAVE_BOARDS]={0};

         GetBalanceDCC(VoltInData, DCC_Val);

         SetValueConfigReg_A(DCC_Val, DCC);
         Write_CFGR_A();
         delay_ms_us(25,0);

         int i;
         for(i=0;i<NUMBER_OF_SLAVE_BOARDS/2;i++){
             DCC_Val[i]=0;
         }
         SetValueConfigReg_A(DCC_Val, DCC);
         Write_CFGR_A();
    #endif

 }

 void ReadStatRoutine(){

     Read_STATA();
     Read_STATB();


 }


