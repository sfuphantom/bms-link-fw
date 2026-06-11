/*
Author: Tanjosh Sidhu
*/

//#include "iso_spi_driver.h"

#ifndef SLAVECOMMUNICATION_DRIVERS_H
#define SLAVECOMMUNICATION_DRIVERS_H
#include <stdint.h>
#include <stdbool.h>
#include "spi.h"
#include "PhantomHelpers.h"

/////////////////////////////////////////////////////////////////
#define NUMBER_OF_FAILS_ALLOWED         2
/////////////////////////////////////////////////////////////////
#define NUMBER_OF_SLAVE_BOARDS          1

#define BYTES_PER_REG_GROUP             6
#define WORDS_PER_REG_GROUP             (BYTES_PER_REG_GROUP / 2)

#define NUMBER_OF_REG_BYTES_PER_CMD     (BYTES_PER_REG_GROUP * NUMBER_OF_SLAVE_BOARDS)
#define NUMBER_OF_REG_WORDS_PER_CMD     (WORDS_PER_REG_GROUP * NUMBER_OF_SLAVE_BOARDS)
//#define NUMBER_OF_REG_NIBBLES_PER_CMD   (NUMBER_OF_REG_BYTES_PER_CMD*2)
/////////////////////////////////////////////////
//SPI
#define SPI_WAIT_BYTE_FINISH_COUNT 0xFF

#define REG_FOR_SPI     spiREG3
#define CS_PIN_ID       0x00U
#define CS_PIN_MASK     (1U << CS_PIN_ID)//0xFEU//0b11111110 << Active low
#define CS_HOLD_MASK    0x10000000U
#define SPI_WDEL        FALSE

#define SPI_CONFIG0_WORD (((uint32)SPI_WDEL<<26U) | ((uint32)SPI_FMT_0 << 24U) | (uint32)CS_PIN_MASK<<16)
#define SPI_CONFIG1_WORD (((uint32)SPI_WDEL<<26U) | ((uint32)SPI_FMT_1 << 24U) | (uint32)CS_PIN_MASK<<16)

//////////////////////////////////////////////////////////////////
typedef enum  {LOW, HIGH} CS_Level;
typedef enum  {Read, Write} WR_RegGroups;
//////////////////////////////////////////////////////////////////
#define tWAKE_us  400
#define tCYCLE_us 3325
#define tSLEEP_ms 2200
#define tREFUP_us 4400
#define tIDEL_us  4300
#define fADC_kHz  3300
//////////////////////////////////////////////////////////////////
#define SPI_DUMMY_DATA_BYTE 0xFFU
#define SPI_DUMMY_DATA_WORD 0xFFFFU
#define SPI_DUMMY_DATA_DWORD 0xFFFFFFFFU
#define SPI_DUMMY_DATA_QWORD 0xFFFFFFFFFFFFFFFFU
//#define SPI_DUMMY_DATA 0xFFFF
#define SPI_DUMMY_CMD  0xFFFF

//#define MINUS1_32 0xFFFF FFFF FFFF FFFF

//////////////////////////////////////////////////////////////////
uint16 Slave_Volt2ADC(float ADC_Volt);
float Slave_ADC2Volt(uint16_t ADC_Word);
void Slave_ADC2Volt_arr(uint16_t* ADC_Words, float* Volts, uint16_t len);
//----------------------------------------------------------------------------------------
//uint16_t pec15Table[256];
#define PEC_INIT_VALUE 0x0010
#define PEC_CHARACTERISTIC_POLYNOMIAL 0x4599

void init_PEC15_Table();
 uint16_t pec15_calc(uint8_t len, const uint16_t *data);
//---------------------------------------------------------------------------------------------------------

 void setCS(CS_Level level);
 CS_Level GetCS();

 uint8_t SPI_SR2Link_BYTE(uint8_t Tx);
 uint64_t SPI_SR2Link_MultiBYTE(uint64_t Tx_Full, uint8_t Bytes);
 uint16_t SPI_SR2Link_WORD(uint16_t Tx);
 uint32_t SPI_SR2Link_DWORD(uint32_t Tx);
 uint64_t SPI_SR2Link_QWORD(uint64_t Tx);
 void SPI_Clock_BYTES(uint8_t Bytes2Clock);
 //---------------------------------------------------------------------------------------------------------
 void wakeup_idle();
 void wakeup_sleep();
 //---------------------------------------------------------------------------------------------------------
 uint32 SPI_SendAndRecevie_Links(uint16_t* Tx, uint16_t* Rx, uint32 MsgSize);
 uint32 SPI_Recevie_Links(uint16_t* Rx, uint32 MsgSize);
 uint32 SPI_Send2Links(uint16_t* Tx, uint32 MsgSize);

 uint32 SendCmdAndPec2Slave(const uint16_t cmd);
 bool SendCMD2Slave_alone(const uint16_t cmd);

 void WriteRegGroup(const uint16_t cmd, uint16_t *data);
 bool ReadRegGroup(const uint16_t cmd, uint16_t *data);
 void ReadMultiRegGroups(const uint16_t *cmds, uint8_t NumOfCmds, uint16_t *data);

 void SendClearThenMeasureCMD(const uint16_t cmd_clear, const uint16_t cmd_Measure);
 bool SendClearCheckThenMeasureCMD(const uint16_t cmd_clear, const uint16_t cmd_check, const uint16_t cmd_Measure);

 //---------------------------------------------------------------------------------------------------------
 bool WriteBytes2RegGroup(const uint16_t cmd, uint8_t* Bytes);
 bool ReadBytesFromRegGroup(const uint16_t cmd, uint8_t* Bytes);
// uint32 WriteNibbles2RegGroup(const uint16_t cmd, uint8_t* Nibbles);
// uint32 ReadNibbles2RegGroup(const uint16_t cmd, uint8_t* Nibbles);

#endif /*SLAVECOMMUNICATION_DRIVERS_H*/
