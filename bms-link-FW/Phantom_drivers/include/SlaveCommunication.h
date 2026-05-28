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
#define NUMBER_OF_SLAVE_BOARDS      2
#define SLAVES_IN_SERIES 1
#define SLAVES_IN_PARALLEL (NUMBER_OF_SLAVE_BOARDS/SLAVES_IN_SERIES)

#define BYTES_PER_REG_GROUP 6
#define WORDS_PER_REG_GROUP (BYTES_PER_REG_GROUP / 2)

#define NUMBER_OF_CELL_VOLTAGE_REG_GROUPS_PER_BOARD 4
#define NUMBER_OF_GPIO_VOLTAGE_REG_GROUPS_PER_BOARD 2
#define NUMBER_OF_CONFIG_REG_GROUPS_PER_BOARD 1

#define CELL_BALANCE_THESHOLD_VOLTS_ADC 1000
#define CELL_BALANCE_HYSTERSIS_BAND_VOLTS_ADC 50
#define CELL_BALANCE_TRIGGER_HIGH_VOLTS_ADC (CELL_BALANCE_THESHOLD_VOLTS_ADC - CELL_BALANCE_HYSTERSIS_BAND_VOLTS_ADC)
#define CELL_BALANCE_TRIGGER_LOW_VOLTS_ADC (CELL_BALANCE_THESHOLD_VOLTS_ADC + CELL_BALANCE_HYSTERSIS_BAND_VOLTS_ADC)


#define CELL_BALANCE_THESHOLD_VOLTS_ADC 1000
#define CELL_BALANCE_THESHOLD_VOLTS_ADC 1000

#define VOLTS_ADC_DRAINED_PER_PULSE 100

/////////////////////////////////////////////////////////////////

#define CELL_IN_SERIES (SLAVES_IN_SERIES * CELLS_PER_SLAVE_BOARD)

#define CELLS_PER_SLAVE_BOARD       (NUMBER_OF_CELL_VOLTAGE_REG_GROUPS_PER_BOARD * WORDS_PER_REG_GROUP)
#define GPIOS_PER_SLAVE_BOARD       (NUMBER_OF_GPIO_VOLTAGE_REG_GROUPS_PER_BOARD * WORDS_PER_REG_GROUP)

#define NUMBER_OF_CELLS             (CELLS_PER_SLAVE_BOARD * NUMBER_OF_SLAVE_BOARDS)
#define NUMBER_OF_GPIOS             (GPIOS_PER_SLAVE_BOARD * NUMBER_OF_SLAVE_BOARDS)

#define NUMBER_OF_REG_BYTES_PER_CMD (BYTES_PER_REG_GROUP * NUMBER_OF_SLAVE_BOARDS)
#define NUMBER_OF_REG_WORDS_PER_CMD (WORDS_PER_REG_GROUP * NUMBER_OF_SLAVE_BOARDS)
#define NUMBER_OF_REG_NIBBLES_PER_CMD NUMBER_OF_REG_BYTES_PER_CMD*2

/////////////////////////////////////////////////
//SPI

#define REG_FOR_SPI     spiREG3
#define CS_PIN_ID       0x00U
#define CS_PIN_MASK     (1U << CS_PIN_ID)//0xFEU//0b11111110 << Active low
#define CS_HOLD_MASK    0x10000000U
#define SPI_WDEL        FALSE

#define SPI_CONFIG0_WORD (((uint32)SPI_WDEL<<26U) | ((uint32)SPI_FMT_0 << 24U) | (uint32)CS_PIN_MASK<<16)

//////////////////////////////////////////////////////////////////
typedef enum  {LOW, HIGH} CS_Level;
typedef enum  {Read, WriteSame, Write} WR_RegGroups;
//////////////////////////////////////////////////////////////////
//#define SPI_CONFIG1_WORD (((uint32)SPI_WDEL<<26U) | ((uint32)SPI_FMT_1 << 24U) | (uint32)CS_PIN_MASK<<16)
//////////////////////////////////////////////////////////////////
// Poll ADC
#define PADC_GARBAGE_CLOCK_CYCLES_BYTES (((NUMBER_OF_SLAVE_BOARDS-1)>>3)+1)
//////////////////////////////////////////////////////////////////
// ADC specs
#define ADC2MICRO_VOLTS      100
#define ADC2VOLTS            ((float)(ADC2MICRO_VOLTS) * 1e-6f)
//#define ADC2VOLTS            (ADC2MICRO_VOLTS / 1000000.0f)
#define ADC_OFFSET_VOLTS                (ADC2VOLTS * 0.00f)
#define ADC_RESOLUTION_BIT              14
//#define ADC_RESOLUTION_BIT_MASK         (~((1<<(16-ADC_RESOLUTION_BIT))-1))//0xFFFC//(~MINUS1(16-ADC_RESOLUTION_BIT))//
#define ADC_MRCRO_VOLT_NOISE            250
#define ADC_MAX_VOLT    0.0f
#define ADC_MIN_VOLT    5.0f
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

#define MINUS1_32 0xFFFF FFFF FFFF FFFF



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
 uint32 SPI_SendAndRecevie_Links(uint16_t* Tx, uint16_t* Rx, uint32 MsgSize);
 uint32 SPI_Recevie_Links(uint16_t* Rx, uint32 MsgSize);
 uint32 SPI_Send2Links(uint16_t* Tx, uint32 MsgSize);

 uint32 SendCmdAndPec2Slave(const uint16_t cmd);
 bool SendCMD2Slave_and_Poll(const uint16_t cmd);

 uint32 WriteRegGroup(const uint16_t cmd, uint16_t *data);
 uint32 ReadRegGroup(const uint16_t cmd, uint16_t *data);
 void ReadMultiRegGroups(const uint16_t *cmds, uint8_t NumOfCmds, uint16_t *data);

 //---------------------------------------------------------------------------------------------------------

 void GetBalanceNibbles(uint16* Volts, uint8_t* BalanceNibbles);
 void GetBalanceDCC(uint16* Volts, uint16_t* DCC);

 //---------------------------------------------------------------------------------------------------------
 uint32 WriteBytes2RegGroup(const uint16_t cmd, uint8_t* Bytes);
 uint32 ReadBytesFromRegGroup(const uint16_t cmd, uint8_t* Bytes);
 uint32 WriteNibbles2RegGroup(const uint16_t cmd, uint8_t* Nibbles);
 uint32 ReadNibbles2RegGroup(const uint16_t cmd, uint8_t* Nibbles);

#endif /*SLAVECOMMUNICATION_DRIVERS_H*/
