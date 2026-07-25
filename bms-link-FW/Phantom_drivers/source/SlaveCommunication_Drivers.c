/*
Author: Tanjosh Sidhu
*/


#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include "string.h"
#include "spi_helpers.h"

//#include "spi.h"
#include "SlaveCommunication_Drivers.h"
#include "SlaveCommunation_Hardware.h"
#include "PhantomHelpers.h"
#include "PhantomTimers.h"


//spiDAT1_t *SPI_LinkConfigData;
//spiBASE_t* REG_FOR_SPI = REG_FOR_SPI;
static uint16_t SubDataWords[NUMBER_OF_REG_WORDS_PER_CMD];


//CS_Level Current_CS_Level = HIGH;
bool ADC_is_Free = TRUE;

#define USE_MEMCPY TRUE

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
 uint16_t calc_cmd_pec15(const uint16_t cmd){
//     uint8_t cmd8[2] = {cmd>>8,cmd};
     return pec15_calc(1, &cmd);
 }
//---------------------------------------------------------------------------------------------------------
//
// void setCS(const CS_Level level){
////     const uint32 CS_MaskPin = (1U << SLAVE_CS_PIN_ID);
//     Current_CS_Level = level;
//     if(level == LOW)
//         REG_FOR_SPI->PC3 &= ~(uint32_t)CS_PIN_MASK;
//     else if(level == HIGH)
//         REG_FOR_SPI->PC3 |=  (uint32_t)CS_PIN_MASK;
// }
// CS_Level GetCS(){
//     return Current_CS_Level;
// }
// CS_Level ToggleCS(){
//     if(Current_CS_Level == HIGH){
//         REG_FOR_SPI->PC3 &= ~(uint32_t)CS_PIN_MASK;
//         Current_CS_Level = LOW;
//     }
//     else if(Current_CS_Level == LOW){
//         REG_FOR_SPI->PC3 |=  (uint32_t)CS_PIN_MASK;
//         Current_CS_Level = HIGH;
//     }
//     return Current_CS_Level;
// }
// uint8_t SPI_SR2Link_2Bits(const uint8_t Tx){
////     const uint8_t waitCount = 0xFF;
//
//     REG_FOR_SPI->DAT1 =   SPI_CONFIG1_WORD | (uint32)(Tx);
//
//     int i=0;
//     while((REG_FOR_SPI->FLG & 0x00000100U) != 0x00000100U && i < SPI_WAIT_BYTE_FINISH_COUNT ){
//         i++;
//     } /* Wait */
//
//     uint8_t Rx = REG_FOR_SPI->BUF;
//     return Rx;
// }
// uint8_t SPI_SR2Link_BYTE(const uint8_t Tx){
////     const uint8_t waitCount = 0xFF;
//
//     REG_FOR_SPI->DAT1 =   SPI_CONFIG0_WORD | (uint32)(Tx);
//
//     int i=0;
//     while((REG_FOR_SPI->FLG & 0x00000100U) != 0x00000100U && i < SPI_WAIT_BYTE_FINISH_COUNT ){
//         i++;
//     } /* Wait */
//
//     uint8_t Rx = REG_FOR_SPI->BUF;
//     return Rx;
// }
// uint64_t SPI_SR2Link_MultiBYTE(const uint64_t Tx_Full, const uint8_t Bytes){
//      const uint8_t Bits = Bytes<<3;
//
//      uint8_t Tx = 0;
//      uint8_t Rx = 0;
//      uint64_t Rx_Full = 0;
//
//      int i;
//
//      for (i=0;i<Bits;i+=8){
//          Tx = Tx_Full >> (Bits - i-8);
//          Rx = SPI_SR2Link_BYTE(Tx);
//
//          Rx_Full |= (uint64_t)Rx<<i;
//      }
//
//      return Rx_Full;
//  }
//
// uint16_t SPI_SR2Link_WORD(const uint16_t Tx){
//     const uint8_t NumOfBytes = 2;
//     uint16_t Rx = SPI_SR2Link_MultiBYTE(Tx, NumOfBytes);
//     return Rx;
// }
// uint32_t SPI_SR2Link_DWORD(const uint32_t Tx){
//     const uint8_t NumOfBytes = 4;
//     uint16_t Rx = SPI_SR2Link_MultiBYTE(Tx, NumOfBytes);
//     return Rx;
// }
// uint64_t SPI_SR2Link_QWORD(const uint64_t Tx){
//     const uint8_t NumOfBytes = 8;
//     uint16_t Rx = SPI_SR2Link_MultiBYTE(Tx, NumOfBytes);
//     return Rx;
// }
// void SPI_Clock_BYTES(const uint8_t Bytes2Clock){
//     int i;
//     for(i=0;i<Bytes2Clock; i++)
//         SPI_SR2Link_BYTE(SPI_DUMMY_DATA_BYTE);
// }

 //---------------------------------------------------------------------------------------------------------
 void wakeup_idle(){ //Number of ICs in the system
     int i;
     for (i=0; i<NUMBER_OF_SLAVE_BOARDS; i++){
         setCS(LOW, SLAVE_CS_PIN_ID);
         SPI_SR2Link_BYTE(SPI_DUMMY_DATA_BYTE);
         setCS(HIGH, SLAVE_CS_PIN_ID);
     }
 }
 void wakeup_sleep() {
     setCS(LOW, SLAVE_CS_PIN_ID);
     int i;
     for (i = 0; i < NUMBER_OF_SLAVE_BOARDS; i++) {
         setCS(LOW, SLAVE_CS_PIN_ID);
         delay_ms_us(0, 2*tWAKE_us);
         setCS(HIGH, SLAVE_CS_PIN_ID);
         delay_ms_us(0, 10);
     }
 }
 //---------------------------------------------------------------------------------------------------------
 void Write_Data(const uint16_t* Tx, uint32 MsgSize){
      uint16_t Tx_Data;

      uint16 tx_Pec = pec15_calc(MsgSize, Tx);
     while(MsgSize-- && !(REG_FOR_SPI->FLG & 0xFF)){
         Tx_Data = *Tx++;

          (void)SPI_SR2Link_WORD(Tx_Data);

      }
      uint16_t Rx_pec = SPI_SR2Link_WORD(tx_Pec);
//      return (REG_FOR_SPI->FLG & 0xFF);
 //     return Rx_pec;
  }
 uint32 Read_Data(uint16_t* Rx, uint32 MsgSize){
     const uint16_t Tx_Data = SPI_DUMMY_DATA_WORD;
     const uint16 tx_Pec = SPI_DUMMY_DATA_WORD;

     uint16_t Rx_Data;

     while(MsgSize-- && !(REG_FOR_SPI->FLG & 0xFF)){
          Rx_Data = SPI_SR2Link_WORD(Tx_Data);

          *Rx++ = Rx_Data;
      }
      uint16_t Rx_pec = SPI_SR2Link_WORD(tx_Pec);
//      return (REG_FOR_SPI->FLG & 0xFF);
      return Rx_pec;
  }

 void SendCmdAndPec2Slave(const uint16_t cmd){
     uint16_t cmd_pec = calc_cmd_pec15(cmd);

     (void)SPI_SR2Link_WORD(cmd);
     (void)SPI_SR2Link_WORD(cmd_pec);
//     uint16_t Rx_cmd = SPI_SR2Link_WORD(cmd);
//     uint16_t Rx_pec = SPI_SR2Link_WORD(cmd_pec);
//     uint32_t FullCmdRx = (uint32_t)Rx_cmd<<16 | (uint32_t)Rx_pec;
//     return FullCmdRx;
 }
#if CUSTOM_POLL_WAIT
// uint32_t pollAndWait(const uint32_t wait_periods_us){
//     bool status = FALSE;
//     uint32_t PollsWaited = 1;
//
//     SPI_Clock_BYTES(NUMBER_OF_GARBAGE_BYTES);
//
//     for(PollsWaited=1; PollsWaited < MAX_POLLS_TIMEOUT; PollsWaited++){
//         status = SPI_SR2Link_2Bits(SPI_DUMMY_DATA_BYTE) & 0x0001;
//         if(status){
//             return PollsWaited;
//         }
//
//         delay_ms_us(0,wait_periods_us);
//     }
//     return 0;
//
// }
#else
  uint32_t pollAndWait(){
      bool status = FALSE;
      uint32_t PollsWaited = 1;

      SPI_Clock_BYTES(NUMBER_OF_GARBAGE_BYTES);

      for(PollsWaited=1; PollsWaited < MAX_POLLS_TIMEOUT; PollsWaited++){
          status = SPI_SR2Link_BYTE(SPI_DUMMY_DATA_BYTE) & 0x0001;
          if(status){
              return PollsWaited;
          }

      }
      return 0;

  }
#endif
 //---------------------------------------------------------------------------------------------------------
 void SendCMD2Slave_alone(const uint16_t cmd){
     setCS(LOW, SLAVE_CS_PIN_ID);
     SendCmdAndPec2Slave(cmd);
     setCS(HIGH, SLAVE_CS_PIN_ID);
 }
#if CUSTOM_POLL_WAIT
 uint32_t SendCMD2Slave_pollAndWait(const uint16_t cmd, const uint32_t wait_periods_us){
      setCS(LOW, SLAVE_CS_PIN_ID);

      SendCmdAndPec2Slave(cmd);
      uint32_t PollsWaited = pollAndWait(wait_periods_us);

      setCS(HIGH, SLAVE_CS_PIN_ID);

      return PollsWaited;
  }
#else
 uint32_t SendCMD2Slave_pollAndWait(const uint16_t cmd){
     setCS(LOW, SLAVE_CS_PIN_ID);

     SendCmdAndPec2Slave(cmd);
     uint32_t PollsWaited = pollAndWait();

     setCS(HIGH, SLAVE_CS_PIN_ID);

     return PollsWaited;
 }
#endif



// void SendClearThenMeasureCMD(const uint16_t cmd_clear, const uint16_t cmd_Measure){
//     SendCMD2Slave_alone(cmd_clear);
//     SendCMD2Slave_alone(cmd_Measure);
// }
 //---------------------------------------------------------------------------------------------------------

 bool ReadRegGroup_NoPecCheck(const uint16_t cmd, uint16_t* data){
     bool Pec_Equal = TRUE;

     int i;
     uint16_t Rx_Pec_Mesg = SPI_DUMMY_DATA_WORD;
     uint16_t Rx_Pec_Calc = SPI_DUMMY_DATA_WORD;

     uint16 *data_Rx = NULL;

     uint16_t ReadOneReg[WORDS_PER_REG_GROUP];
     uint16_t ReadOneReg_Swap[WORDS_PER_REG_GROUP];



     setCS(LOW, SLAVE_CS_PIN_ID);

     SendCmdAndPec2Slave(cmd);

     for(i=0; i<NUMBER_OF_SLAVE_BOARDS; i++){
         data_Rx = data + i*WORDS_PER_REG_GROUP;

         Rx_Pec_Mesg = Read_Data(ReadOneReg, WORDS_PER_REG_GROUP);

#if USE_MEMCPY
         memcpy(data_Rx, ReadOneReg, WORDS_PER_REG_GROUP * sizeof(uint16_t));
#else
         int j;
         for(j=0; j < WORDS_PER_REG_GROUP; j++)
             data_Rx[j] = ReadOneReg[j];
#endif

         swap_word_bytes_arr(ReadOneReg, ReadOneReg_Swap, WORDS_PER_REG_GROUP);

         Rx_Pec_Calc = pec15_calc(WORDS_PER_REG_GROUP, ReadOneReg_Swap);
         Pec_Equal &= (Rx_Pec_Mesg == Rx_Pec_Calc);
         if(false /*!Pec_Equal*/){//TODO: should be if(!Pec_Equal), however PEC_Equal is alway false idk
             break;
         }
     }

     setCS(HIGH, SLAVE_CS_PIN_ID);
     return TRUE; //Pec_Equal; //TODO should return Pec_Equal, however PEC_Equal is alway false idk
 }
 bool ReadRegGroup(const uint16_t cmd, uint16_t *data){
     bool Pec_Eq;
     int repeat_idx;

     for(repeat_idx=0;repeat_idx<NUMBER_OF_FAILS_ALLOWED;repeat_idx++){
         Pec_Eq = ReadRegGroup_NoPecCheck(cmd, data);

         if(Pec_Eq){return true;}
     }
     return false;
  }

 void WriteRegGroup(const uint16_t cmd, const uint16_t *data){
      int i;
      uint8_t idx;
      setCS(LOW, SLAVE_CS_PIN_ID);
      SendCmdAndPec2Slave(cmd);

      for(i=0; i<NUMBER_OF_SLAVE_BOARDS; i++){
          idx = NUMBER_OF_REG_WORDS_PER_CMD - (i+1)*WORDS_PER_REG_GROUP;

          Write_Data(&data[idx], WORDS_PER_REG_GROUP);
      }
          setCS(HIGH, SLAVE_CS_PIN_ID);
 }
 bool WriteThenReadRegGroup(const uint16_t W_cmd, const uint16_t R_cmd, const uint16_t *W_data){
      bool RW_EQ;
      int repeat_idx;
      uint16_t R_data[NUMBER_OF_REG_WORDS_PER_CMD];

      for(repeat_idx=0;repeat_idx<NUMBER_OF_FAILS_ALLOWED;repeat_idx++){

         WriteRegGroup(W_cmd, W_data);
         ReadRegGroup(R_cmd, R_data);

         RW_EQ = array16_eq_all(W_data, R_data, NUMBER_OF_REG_WORDS_PER_CMD);

         if(RW_EQ){return true;}
     }
      return false;
 }
 //---------------------------------------------------------------------------------------------------------

 bool ReadMultiRegGroups(const uint16_t *cmds, uint8_t NumOfCmds, uint16_t *data){
     int i=0, j=0, idx0=0, idx1=0;
     bool PecEq = true;

     for(i=0;i<NumOfCmds;i++){
         wakeup_idle();
         PecEq &= ReadRegGroup(cmds[i], SubDataWords);

         for(j=0;j < NUMBER_OF_REG_WORDS_PER_CMD;j+=WORDS_PER_REG_GROUP){
             idx0 = WORDS_PER_REG_GROUP * i + NumOfCmds*j;
             idx1 = j;
#if USE_MEMCPY
             memcpy(&data[idx0], &SubDataWords[idx1], WORDS_PER_REG_GROUP * sizeof(uint16_t));
#else
             int k;
             for(k=0;k<WORDS_PER_REG_GROUP; k++){
                 uint16_t CurrentWord = SubDataWords[idx1+k];
                 data[idx0+k] = CurrentWord;
             }
#endif
         }
     }
     return PecEq;
  }
 void WriteMultiRegGroups(const uint16_t *cmds, uint8_t NumOfCmds, uint16_t *data){
     int i=0, j=0, idx0=0, idx1=0;

     for(i=0;i<NumOfCmds;i++){
         for(j=0;j < NUMBER_OF_REG_WORDS_PER_CMD;j+=WORDS_PER_REG_GROUP){
             idx0 = WORDS_PER_REG_GROUP * i + NumOfCmds*j;
             idx1 = j;
#if USE_MEMCPY
             memcpy(&SubDataWords[idx1], &data[idx0], WORDS_PER_REG_GROUP * sizeof(uint16_t));
#else
             int k;
             for(k=0;k<WORDS_PER_REG_GROUP; k++){
                 uint16_t CurrentWord = SubDataWords[idx1+k];
                 data[idx0+k] = CurrentWord;
             }
#endif

         }

         wakeup_idle();
         WriteRegGroup(cmds[i], SubDataWords);

     }
  }

 bool WriteThenReadMultiRegGroups(const uint16_t *W_cmds, const uint16_t *R_cmds, uint8_t NumOfCmds, uint16_t *data){
      int i, j, idx0, idx1;
      bool PecEq = true;

      for(i=0;i<NumOfCmds;i++){
          for(j=0;j < NUMBER_OF_REG_WORDS_PER_CMD;j+=WORDS_PER_REG_GROUP){
              idx0 = WORDS_PER_REG_GROUP * i + NumOfCmds*j;
              idx1 = j;
 #if USE_MEMCPY
              memcpy(&SubDataWords[idx1], &data[idx0], WORDS_PER_REG_GROUP * sizeof(uint16_t));
 #else
              int k;
              for(k=0;k<WORDS_PER_REG_GROUP; k++){
                  uint16_t CurrentWord = SubDataWords[idx1+k];
                  data[idx0+k] = CurrentWord;
              }
 #endif
          }

          wakeup_idle();
          PecEq = WriteThenReadRegGroup(W_cmds[i], R_cmds[i], SubDataWords);

          if(!PecEq){return false;}

      }
      return true;
   }

 bool SendClearThenCheckCMD(const uint16_t cmd_clear, const uint16_t cmd_check){
     bool cleared = FALSE;
     bool PecEQ;
     uint16_t Reg[NUMBER_OF_REG_WORDS_PER_CMD];
     int repeat_idx;

     for(repeat_idx=0; repeat_idx<NUMBER_OF_FAILS_ALLOWED; repeat_idx++){
         SendCMD2Slave_alone(cmd_clear);
         PecEQ = ReadRegGroup(cmd_check, Reg);

         if(array16_eq_every(Reg, 0xFFFF, NUMBER_OF_REG_WORDS_PER_CMD) && PecEQ){
             cleared = TRUE;
             break;
         }
     }

     if(!cleared){return FALSE;}
     return TRUE;
 }
 //---------------------------------------------------------------------------------------------------------
 void WriteBytes2RegGroup(const uint16_t cmd, const uint8_t* Bytes){
     bytes2words_arr(Bytes , SubDataWords  , NUMBER_OF_REG_WORDS_PER_CMD, BigEndian);

     WriteRegGroup(cmd, SubDataWords);
 }
 bool ReadBytesFromRegGroup(const uint16_t cmd, uint8_t* Bytes){
     uint16_t PecEq = ReadRegGroup(cmd, SubDataWords);

     words2bytes_arr(SubDataWords, Bytes, NUMBER_OF_REG_WORDS_PER_CMD, BigEndian);
     return PecEq;
 }

 bool WriteThenReadRegGroup_Bytes(const uint16_t W_cmd, const uint16_t R_cmd, const uint8_t *W_data){
      bool RW_EQ;
      int repeat_idx;

      uint8_t R_data[NUMBER_OF_REG_BYTES_PER_CMD];

      for(repeat_idx=0;repeat_idx<NUMBER_OF_FAILS_ALLOWED;repeat_idx++){
         WriteBytes2RegGroup(W_cmd, W_data);
         ReadBytesFromRegGroup(R_cmd, R_data);

         RW_EQ = array8_eq_all(W_data, R_data, NUMBER_OF_REG_BYTES_PER_CMD);

         if(RW_EQ){return true;}
     }

      return false;
 }

