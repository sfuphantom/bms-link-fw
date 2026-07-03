/*
 * spi_drivers.h
 *
 *  Created on: Jul 2, 2026
 *      Author: tanjo
 */

#ifndef PHANTOM_DRIVERS_INCLUDE_SPI_DRIVERS_H_
#define PHANTOM_DRIVERS_INCLUDE_SPI_DRIVERS_H_

#include "spi.h"


#define SPI_WAIT_BYTE_FINISH_COUNT 0xFF

#define REG_FOR_SPI     spiREG3

#define CS_PIN_MASK(CS_PIN_ID)     (1U << CS_PIN_ID)//0xFEU//0b11111110 << Active low
//#define CS_HOLD_MASK    0x10000000U
#define SPI_WDEL        FALSE

#define SPI_CONFIG0_WORD (((uint32)SPI_WDEL<<26U) | ((uint32)SPI_FMT_0 << 24U) | (uint32)1U<<16)
#define SPI_CONFIG1_WORD (((uint32)SPI_WDEL<<26U) | ((uint32)SPI_FMT_1 << 24U) | (uint32)1U<<16)
#define SPI_CONFIG2_WORD (((uint32)SPI_WDEL<<26U) | ((uint32)SPI_FMT_2 << 24U) | (uint32)0<<16)
#define SPI_CONFIG3_WORD (((uint32)SPI_WDEL<<26U) | ((uint32)SPI_FMT_3 << 24U) | (uint32)0<<16)


#define SPI_DUMMY_DATA_BYTE 0xFFU
#define SPI_DUMMY_DATA_WORD 0xFFFFU
#define SPI_DUMMY_DATA_DWORD 0xFFFFFFFFU
#define SPI_DUMMY_DATA_QWORD 0xFFFFFFFFFFFFFFFFU
//---------------------------------------------------------------------------------------------------------
typedef enum  {LOW, HIGH} CS_Level;
//---------------------------------------------------------------------------------------------------------

void setCS(const CS_Level level, const uint8_t CS);
 CS_Level GetCS(const uint8_t CS);
 CS_Level ToggleCS(const uint8_t CS);

 uint8_t SPI_SR2Link_2Bits(const uint8_t Tx);
 uint8_t SPI_SR2Link_BYTE(const uint8_t Tx);
 uint64_t SPI_SR2Link_MultiBYTE(const uint64_t Tx_Full, const uint8_t Bytes);
 uint16_t SPI_SR2Link_WORD(const uint16_t Tx);
 uint32_t SPI_SR2Link_DWORD(const uint32_t Tx);
 uint64_t SPI_SR2Link_QWORD(const uint64_t Tx);
 void SPI_Clock_BYTES(const uint8_t Bytes2Clock);
 //---------------------------------------------------------------------------------------------------------


#endif /* PHANTOM_DRIVERS_INCLUDE_SPI_DRIVERS_H_ */
