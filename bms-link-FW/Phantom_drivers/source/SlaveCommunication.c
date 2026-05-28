/*
Author: Tanjosh Sidhu
*/


#include "spi.h"
#include <stdint.h>
#include <stdbool.h>
#include "ltc6811_commands.h"
#include "SlaveCommunication.h"
#include "PhantomHelpers.h"

//spiDAT1_t *SPI_LinkConfigData;
spiBASE_t* Slave_SPI_REG = REG_FOR_SPI;
static uint16_t SubDataWords[NUMBER_OF_REG_WORDS_PER_CMD];

//struct ConfigReg_A *ConfigRegData_A_ptr = &ConfigRegData_A;

CS_Level Current_CS_Level = HIGH;

uint16 Slave_Volt2ADC(float ADC_Volt){
    uint16 ADC_Value = (ADC_Volt - ADC_OFFSET_VOLTS)/ADC2VOLTS;
    return ADC_Value;
}
float Slave_ADC2Volt(uint16_t ADC_Word){
    uint16 ADC_Round = round16(ADC_Word, 16-ADC_RESOLUTION_BIT);
    float Volt = ADC_Round * ADC2VOLTS + ADC_OFFSET_VOLTS;
    return Volt;
}

void Slave_ADC2Volt_arr(uint16_t* ADC_Words, float* Volts, uint16_t len){
    int i;
    for(i=0; i<len; i++)
        Volts[i] = Slave_ADC2Volt(ADC_Words[i]);
}

//----------------------------------------------------------------------------------------


uint16_t pec15Table[256];
//#define PEC_INIT_VALUE 0x0010
//#define PEC_CHARACTERISTIC_POLYNOMIAL 0x4599

void init_PEC15_Table(){
    int i, bit;
    uint16_t remainder;
    for (i = 0; i < 256; i++){
        remainder = i << 7;
        for (bit = 8; bit > 0; --bit){
            if (remainder & 0x4000)
                remainder = (remainder << 1) ^ PEC_CHARACTERISTIC_POLYNOMIAL;
            else
                remainder = (remainder << 1);

        }
        pec15Table[i] = remainder & 0xFFFF;
    }
}
 uint16_t pec15_calc(uint8_t len, const uint16_t *data){
     if (data == NULL)
         return SPI_DUMMY_DATA_WORD;
     uint8_t address, byte;

     uint16_t remainder = PEC_INIT_VALUE;//PEC seed, 16

     int i, j;
     uint8_t jShift;
     for (i = 0; i < len; i++){
         for(j=1; j>=0; j--){
             jShift = 8*j;
             byte = (uint8_t) (data[i] >> jShift);
             address = ((remainder >> 7) ^ byte) & 0xff;//calculate PEC table address
             remainder = (remainder << 8 ) ^ pec15Table[address];
         }
     }
     return remainder<<1;//The CRC15 has a 0 in the LSB so the final value must be multiplied by 2
 }

 uint16_t calc_cmd_pec15(uint16_t cmd){
//     uint8_t cmd8[2] = {cmd>>8,cmd};
     return pec15_calc(1, &cmd);
 }
//---------------------------------------------------------------------------------------------------------

 void setCS(CS_Level level){
//     const uint32 CS_MaskPin = (1U << CS_PIN_ID);
     if(level == LOW)
         Slave_SPI_REG->PC3 &= ~(uint32_t)CS_PIN_MASK;
     else if(level == HIGH)
         Slave_SPI_REG->PC3 |=  (uint32_t)CS_PIN_MASK;
     Current_CS_Level = level;
 }
 CS_Level GetCS(){
     return Current_CS_Level;
 }

 uint8_t SPI_SR2Link_BYTE(uint8_t Tx){
     const uint8_t waitCount = 0xFF;

     Slave_SPI_REG->DAT1 =   SPI_CONFIG0_WORD | (uint32)(Tx);

     int i=0;
     while((Slave_SPI_REG->FLG & 0x00000100U) != 0x00000100U && i < waitCount){
         i++;
     } /* Wait */

     uint8_t Rx = Slave_SPI_REG->BUF;
     return Rx;
 }
 uint64_t SPI_SR2Link_MultiBYTE(uint64_t Tx_Full, uint8_t Bytes){
      const uint8_t Bits = Bytes*8;

      uint8_t Tx = 0;
      uint8_t Rx = 0;
      uint64_t Rx_Full = 0;

      int i;

      for (i=0;i<Bits;i+=8){
          Tx = Tx_Full >> (Bits - i-8);
          Rx = SPI_SR2Link_BYTE(Tx);

          Rx_Full |= (uint64_t)Rx<<i;
      }

      return Rx_Full;
  }

 uint16_t SPI_SR2Link_WORD(uint16_t Tx){
     const uint8_t NumOfBytes = 2;
     uint16_t Rx = SPI_SR2Link_MultiBYTE(Tx, NumOfBytes);
     return Rx;
 }
 uint32_t SPI_SR2Link_DWORD(uint32_t Tx){
     const uint8_t NumOfBytes = 4;
     uint16_t Rx = SPI_SR2Link_MultiBYTE(Tx, NumOfBytes);
     return Rx;
 }
 uint64_t SPI_SR2Link_QWORD(uint64_t Tx){
     const uint8_t NumOfBytes = 8;
     uint16_t Rx = SPI_SR2Link_MultiBYTE(Tx, NumOfBytes);
     return Rx;
 }

 void SPI_Clock_BYTES(uint8_t Bytes2Clock){
     int i;
     for(i=0;i<Bytes2Clock; i++)
         SPI_SR2Link_BYTE(SPI_DUMMY_DATA_BYTE);
 }

 //---------------------------------------------------------------------------------------------------------
 uint32 WR_Data(uint16_t* Tx, uint16_t* Rx, uint32 MsgSize){
     uint16_t Tx_Data = SPI_DUMMY_DATA_WORD;
     uint16_t Rx_Data;

     uint16 tx_Pec = pec15_calc(MsgSize, Tx);
    while(MsgSize-- && !(Slave_SPI_REG->FLG & 0xFF)){
        if(Tx)
            Tx_Data = *Tx++;

         Rx_Data = SPI_SR2Link_WORD(Tx_Data);

         if(Rx)
             *Rx++ = Rx_Data;
     }
//     uint16_t Rx_pec = SPI_SR2Link_WORD(tx_Pec);
     return (Slave_SPI_REG->FLG & 0xFF);
//     return Rx_pec;
 }
 uint32 SPI_SendAndRecevie_Links(uint16_t* Tx, uint16_t* Rx, uint32 MsgSize){
     uint16_t Tx_Data = SPI_DUMMY_DATA_WORD;
     uint16_t Rx_Data;

     uint16 tx_Pec = pec15_calc(MsgSize, Tx);
    while(MsgSize-- && !(Slave_SPI_REG->FLG & 0xFF)){
        if(Tx)
            Tx_Data = *Tx++;

         Rx_Data = SPI_SR2Link_WORD(Tx_Data);

         if(Rx)
             *Rx++ = Rx_Data;
     }
     uint16_t Rx_pec = SPI_SR2Link_WORD(tx_Pec);
//     return (Slave_SPI_REG->FLG & 0xFF);
     return Rx_pec;
 }
 uint32 SPI_Recevie_Links(uint16_t* Rx, uint32 MsgSize){
     return SPI_SendAndRecevie_Links(NULL, Rx, MsgSize);
 }
 uint32 SPI_Send2Links(uint16_t* Tx, uint32 MsgSize){
     return SPI_SendAndRecevie_Links(Tx, NULL, MsgSize);
 }
 uint32 SendCmdAndPec2Slave(uint16_t cmd){
     uint16_t cmd_pec = calc_cmd_pec15(cmd);

     uint16_t Rx_cmd = SPI_SR2Link_WORD(cmd);
     uint16_t Rx_pec = SPI_SR2Link_WORD(cmd_pec);
     uint32_t FullCmdRx = (uint32_t)Rx_cmd<<16 | (uint32_t)Rx_pec;
     return FullCmdRx;
 }

 bool SendCMD2Slave_and_Poll(const uint16_t cmd){
     setCS(LOW);
     SendCmdAndPec2Slave(cmd);

     SPI_Clock_BYTES(PADC_GARBAGE_CLOCK_CYCLES_BYTES);

     bool status = SPI_SR2Link_BYTE(SPI_DUMMY_DATA_BYTE);
     setCS(HIGH);
     return (status != 0x00);
 }

 uint16_t ReadWriteRegGroup(const uint16_t cmd, uint16_t* data, WR_RegGroups Mode){
     uint16_t Pec_Equal = (1U<<NUMBER_OF_SLAVE_BOARDS)-1;

     int i, j;
     uint16_t Rx_Pec_Mesg = SPI_DUMMY_DATA_WORD;
     uint16_t Rx_Pec_Calc = SPI_DUMMY_DATA_WORD;
     uint16_t tx_Pec      = SPI_DUMMY_DATA_WORD;

     uint16 *data_Tx = NULL;
     uint16 *data_Rx = NULL;

     uint16_t ReadOneReg[WORDS_PER_REG_GROUP];


     setCS(LOW);
     uint32_t cmdRx = SendCmdAndPec2Slave(cmd);

     for(i=0; i<NUMBER_OF_SLAVE_BOARDS; i++){
         switch (Mode){
             case Read:
                 data_Tx = NULL;
                 data_Rx = data + i*WORDS_PER_REG_GROUP;
                 tx_Pec = SPI_DUMMY_DATA_WORD;
                 break;
             case WriteSame:
                 data_Tx = data;
                 data_Rx = NULL;
                 tx_Pec = pec15_calc(WORDS_PER_REG_GROUP, data_Tx);
                 break;
             case Write:
                 data_Tx = data + NUMBER_OF_REG_WORDS_PER_CMD - (i+1)*WORDS_PER_REG_GROUP;
                 data_Rx = NULL;
                 tx_Pec = pec15_calc(WORDS_PER_REG_GROUP, data_Tx);
                 break;
             default:
                 data_Tx = NULL;
                 data_Rx = NULL;
                 tx_Pec = SPI_DUMMY_DATA_WORD;
                 break;
         }

         WR_Data(data_Tx, ReadOneReg, WORDS_PER_REG_GROUP);
         Rx_Pec_Mesg = SPI_SR2Link_WORD(tx_Pec);

         Rx_Pec_Calc = pec15_calc(WORDS_PER_REG_GROUP,  ReadOneReg);


         if (data_Rx){
             for(j=0; j < WORDS_PER_REG_GROUP; j++)
                 data_Rx[j] = ReadOneReg[j];
         }

         if(Rx_Pec_Mesg != Rx_Pec_Calc)
             Pec_Equal &= ~(1<<i);
     }

     setCS(HIGH);
     return Pec_Equal;
 }
 uint32 WriteSameRegGroup(const uint16_t cmd, uint16_t *data){
//     swap_word_bytes_arr(data, data, WORDS_PER_REG_GROUP);
     return ReadWriteRegGroup(cmd, data, WriteSame);
 }
 uint32 WriteRegGroup(const uint16_t cmd, uint16_t *data){
//     swap_word_bytes_arr(data, data, WORDS_PER_REG_GROUP);
     return ReadWriteRegGroup(cmd, data, Write);
 }
 uint32 ReadRegGroup(const uint16_t cmd, uint16_t *data){
     uint16_t data_Pec = ReadWriteRegGroup(cmd, data, Read);
     return data_Pec;
  }

 void ReadMultiRegGroups(const uint16_t *cmds, uint8_t NumOfCmds, uint16_t *data){
     int i=0, j=0, k=0, idx0=0, idx1=0;
     uint16_t CurrentWord;
     uint16_t PecEq = 1;

     for(i=0;i<NumOfCmds;i++){
         PecEq = ReadRegGroup(cmds[i], SubDataWords);

         for(j=0;j < NUMBER_OF_REG_WORDS_PER_CMD;j+=WORDS_PER_REG_GROUP){
             idx0 = WORDS_PER_REG_GROUP * i + NumOfCmds*j;
             idx1 = j;

//             memcpy(data[idx0,], SubDataWords[idx1], WORDS_PER_REG_GROUP);
             for(k=0;k<WORDS_PER_REG_GROUP; k++){
                 CurrentWord  = SubDataWords[idx1+k];
                 data[idx0+k] = CurrentWord;
             }
         }
     }
  }
 //---------------------------------------------------------------------------------------------------------
 uint32 WriteBytes2RegGroup(const uint16_t cmd, uint8_t* Bytes){
//     uint16_t Words[NUMBER_OF_REG_WORDS_PER_CMD] = {0};
     bytes2words(Bytes , SubDataWords  , NUMBER_OF_REG_WORDS_PER_CMD, BigEndian);

     return WriteRegGroup(cmd, SubDataWords);
 }
 uint32 ReadBytesFromRegGroup(const uint16_t cmd, uint8_t* Bytes){
//     uint16_t Words[NUMBER_OF_REG_WORDS_PER_CMD] = {0};

     uint16_t PecEq = ReadRegGroup(cmd, SubDataWords);
     words2bytes(SubDataWords, Bytes, NUMBER_OF_REG_WORDS_PER_CMD, BigEndian);
     return PecEq;
 }

 uint32 WriteNibbles2RegGroup(const uint16_t cmd, uint8_t* Nibbles){
//     uint16_t Words[NUMBER_OF_REG_WORDS_PER_CMD] = {0};
     nibbles2words(Nibbles , SubDataWords  , NUMBER_OF_REG_WORDS_PER_CMD, BigEndian);

     return WriteRegGroup(cmd, SubDataWords);
 }
 uint32 ReadNibbles2RegGroup(const uint16_t cmd, uint8_t* Nibbles){
//     uint16_t Words[NUMBER_OF_REG_WORDS_PER_CMD] = {0};

     uint16_t PecEq = ReadRegGroup(cmd, SubDataWords);
     words2nibbles(SubDataWords, Nibbles, NUMBER_OF_REG_WORDS_PER_CMD, BigEndian);
     return PecEq;
 }

 //---------------------------------------------------------------------------------------------------------
// void wakeup_idle(){ //Number of ICs in the system
//     int i;
//     for (i=0; i<NUMBER_OF_SLAVE_BOARDS; i++){
//         setCS(LOW);
//         SPI_SR2Link_BYTE(SPI_DUMMY_DATA_BYTE);
//         setCS(HIGH);
//     }
// }
// void wakeup_sleep() {
//     setCS(LOW);
//     int i;
//     for (i = 0; i < NUMBER_OF_SLAVE_BOARDS; i++) {
//         setCS(LOW);
//         SPI_SR2Link_BYTE(SPI_DUMMY_DATA_BYTE);
//         delay_ms_us(0, tWAKE_us);
//         setCS(HIGH);
//         delay_ms_us(0, 10);
//     }
// }
// //---------------------------------------------------------------------------------------------------------

// parallel and series
 uint8_t checkCellBalance_Enable(uint16* Volts){
     int i;
     uint16_t min, avg;
//     uint8_t idx;
     uint8_t Par2Bal = 0;

     for(i=0;i<SLAVES_IN_PARALLEL;i++){
         array16_minAndIdx(&Volts[i * CELL_IN_SERIES], CELL_IN_SERIES, &min, NULL);
         avg = array16_avg(Volts, CELL_IN_SERIES);

         if(CELL_BALANCE_THESHOLD_VOLTS_ADC < (avg-min))
             Par2Bal |= 1U<i;

     }
     return Par2Bal;
 }

 void GetBalanceNibbles(uint16* Volts, uint8_t* BalanceNibbles){
     const uint8_t Nibbles2Bytes = 2;
     int i, j, k;
     uint16_t min, avg, Vdiff, minAvgDiff;
     uint8_t nibble;
     bool Balance;

     static uint8_t Hysteresis = 0;


     for(i=0;i<SLAVES_IN_PARALLEL;i++){

         array16_minAndIdx(&Volts[i * CELL_IN_SERIES], CELL_IN_SERIES, &min, NULL);
         avg = array16_avg(Volts, CELL_IN_SERIES);

         minAvgDiff = avg-min;
         if (CELL_BALANCE_TRIGGER_HIGH_VOLTS_ADC > minAvgDiff ){
             Balance = TRUE;
             Hysteresis |= 1U<<i;
         }
         else if(CELL_BALANCE_TRIGGER_LOW_VOLTS_ADC < minAvgDiff){
             Balance = FALSE;
             Hysteresis &= (~1U)<<i;
         }
         else{
             Balance = (Hysteresis>>i) & 0x1;
         }


         for(j=0;j<CELL_IN_SERIES/2; j++){
             BalanceNibbles[i * CELL_IN_SERIES + j] = 0;

             if(!Balance)
                 continue;

             for(k=0;k<Nibbles2Bytes;k++){
                 Vdiff = Volts[i * CELL_IN_SERIES + j*Nibbles2Bytes + k] - min;

                 if(CELL_BALANCE_THESHOLD_VOLTS_ADC > Vdiff)
                     continue;

                 nibble = 0xF;//0x1//((Vdiff + VOLTS_ADC_DRAINED_PER_PULSE/2)/VOLTS_ADC_DRAINED_PER_PULSE) & 0xF
                 BalanceNibbles[i * CELL_IN_SERIES + j] = nibble<<(k*4);
             }
         }
     }
 }

 void GetBalanceDCC(uint16* Volts, uint16_t* DCC){
      int i, j, k;
      uint16_t min, avg, Vdiff;
      bool Balance;

      for(i=0;i<SLAVES_IN_PARALLEL;i++){
          array16_minAndIdx(&Volts[i * CELL_IN_SERIES], CELL_IN_SERIES, &min, NULL);
          avg = array16_avg(Volts, CELL_IN_SERIES);

          Balance = CELL_BALANCE_THESHOLD_VOLTS_ADC < (avg-min);

          for(j=0;j<SLAVES_IN_SERIES; j++){
              *DCC = 0;
              if(!Balance)
                   continue;

              for (k=0;k<CELLS_PER_SLAVE_BOARD; k++){
                  Vdiff = Volts[i * CELL_IN_SERIES + j] - min;

                  if(CELL_BALANCE_THESHOLD_VOLTS_ADC < Vdiff)
                      *DCC |= 1U<<k;
              }
              DCC++;
          }
      }
  }
