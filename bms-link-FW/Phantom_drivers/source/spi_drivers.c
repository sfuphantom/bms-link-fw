/*
 * spi_drivers.c
 *
 *  Created on: Jul 2, 2026
 *      Author: tanjo
 */

#include "spi.h"
#include "spi_drivers.h"


//---------------------------------------------------------------------------------------------------------
 CS_Level Current_CS_Level = HIGH;
//---------------------------------------------------------------------------------------------------------

 void setCS(const CS_Level level, const uint8_t CS){
//     const uint32 CS_MaskPin = (1U << CS_PIN_ID);
     Current_CS_Level = level;
     if(level == LOW)
         REG_FOR_SPI->PC3 &= ~(uint32_t)CS_PIN_MASK(CS);
     else if(level == HIGH)
         REG_FOR_SPI->PC3 |=  (uint32_t)CS_PIN_MASK(CS);
 }
 CS_Level GetCS(const uint8_t CS){
     return Current_CS_Level;
 }
 CS_Level ToggleCS(const uint8_t CS){
     if(Current_CS_Level == HIGH){
         REG_FOR_SPI->PC3 &= ~(uint32_t)CS_PIN_MASK(CS);
         Current_CS_Level = LOW;
     }
     else if(Current_CS_Level == LOW){
         REG_FOR_SPI->PC3 |=  (uint32_t)CS_PIN_MASK(CS);
         Current_CS_Level = HIGH;
     }
     return Current_CS_Level;
 }
 uint8_t SPI_SR2Link_2Bits(const uint8_t Tx){
//     const uint8_t waitCount = 0xFF;

     REG_FOR_SPI->DAT1 =   SPI_CONFIG1_WORD | (uint32)(Tx);

     int i=0;
     while((REG_FOR_SPI->FLG & 0x00000100U) != 0x00000100U && i < SPI_WAIT_BYTE_FINISH_COUNT ){
         i++;
     } /* Wait */

     uint8_t Rx = REG_FOR_SPI->BUF;
     return Rx;
 }
 uint8_t SPI_SR2Link_BYTE(const uint8_t Tx){
//     const uint8_t waitCount = 0xFF;

     REG_FOR_SPI->DAT1 =   SPI_CONFIG0_WORD | (uint32)(Tx);

     int i=0;
     while((REG_FOR_SPI->FLG & 0x00000100U) != 0x00000100U && i < SPI_WAIT_BYTE_FINISH_COUNT ){
         i++;
     } /* Wait */

     uint8_t Rx = REG_FOR_SPI->BUF;
     return Rx;
 }
 uint16_t SPI_SR2Link_14Bit(const uint16_t Tx){
//     const uint8_t waitCount = 0xFF;

     REG_FOR_SPI->DAT1 =   SPI_CONFIG2_WORD | (uint32)(Tx);

     int i=0;
     while((REG_FOR_SPI->FLG & 0x00000100U) != 0x00000100U && i < SPI_WAIT_BYTE_FINISH_COUNT ){
         i++;
     } /* Wait */

     uint16_t Rx = REG_FOR_SPI->BUF;
     return Rx;
 }
 uint64_t SPI_SR2Link_MultiBYTE(const uint64_t Tx_Full, const uint8_t Bytes){
      const uint8_t Bits = Bytes<<3;

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

 uint16_t SPI_SR2Link_WORD(const uint16_t Tx){
     const uint8_t NumOfBytes = 2;
     uint16_t Rx = SPI_SR2Link_MultiBYTE(Tx, NumOfBytes);
     return Rx;
 }
 uint32_t SPI_SR2Link_DWORD(const uint32_t Tx){
     const uint8_t NumOfBytes = 4;
     uint16_t Rx = SPI_SR2Link_MultiBYTE(Tx, NumOfBytes);
     return Rx;
 }
 uint64_t SPI_SR2Link_QWORD(const uint64_t Tx){
     const uint8_t NumOfBytes = 8;
     uint16_t Rx = SPI_SR2Link_MultiBYTE(Tx, NumOfBytes);
     return Rx;
 }
 void SPI_Clock_BYTES(const uint8_t Bytes2Clock){
     int i;
     for(i=0;i<Bytes2Clock; i++)
         SPI_SR2Link_BYTE(SPI_DUMMY_DATA_BYTE);
 }

 //---------------------------------------------------------------------------------------------------------
