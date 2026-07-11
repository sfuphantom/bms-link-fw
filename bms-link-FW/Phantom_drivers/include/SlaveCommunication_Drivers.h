/*
Author: Tanjosh Sidhu
*/

//#include "iso_spi_driver.h"

#ifndef SLAVECOMMUNICATION_DRIVERS_H
#define SLAVECOMMUNICATION_DRIVERS_H
#include <stdint.h>
#include <stdbool.h>
//#include "spi.h"
#include "SlaveCommunation_Hardware.h"
//#include "PhantomHelpers.h"

/////////////////////////////////////////////////////////////////
#define NUMBER_OF_FAILS_ALLOWED         2
/////////////////////////////////////////////////////////////////
typedef enum  {LOW, HIGH} CS_Level;
//////////////////////////////////////////////////////////////////
#define SPI_DUMMY_DATA_BYTE 0xFFU
#define SPI_DUMMY_DATA_WORD 0xFFFFU
#define SPI_DUMMY_DATA_DWORD 0xFFFFFFFFU
#define SPI_DUMMY_DATA_QWORD 0xFFFFFFFFFFFFFFFFU
//#define SPI_DUMMY_CMD  0xFFFF

#define MAX_POLLS_TIMEOUT 20
#define NUMBER_OF_GARBAGE_BYTES ((NUMBER_OF_SLAVE_BOARDS-7)/8)
//----------------------------------------------------------------------------------------
//uint16_t pec15Table[256];
#define PEC_INIT_VALUE 0x0010
#define PEC_CHARACTERISTIC_POLYNOMIAL 0x4599

void init_PEC15_Table();
 uint16_t pec15_calc(uint8_t len, const uint16_t *data);
//---------------------------------------------------------------------------------------------------------

 void setCS(const CS_Level level);
 CS_Level GetCS();
 CS_Level ToggleCS();

 uint8_t SPI_SR2Link_2Bits(const uint8_t Tx);
 uint8_t SPI_SR2Link_BYTE(const uint8_t Tx);
 uint64_t SPI_SR2Link_MultiBYTE(const uint64_t Tx_Full, const uint8_t Bytes);
 uint16_t SPI_SR2Link_WORD(const uint16_t Tx);
 uint32_t SPI_SR2Link_DWORD(const uint32_t Tx);
 uint64_t SPI_SR2Link_QWORD(const uint64_t Tx);
 void SPI_Clock_BYTES(const uint8_t Bytes2Clock);
 //---------------------------------------------------------------------------------------------------------
 void wakeup_idle();
 void wakeup_sleep();
 //---------------------------------------------------------------------------------------------------------
// uint32 SPI_SendAndRecevie_Links(uint16_t* Tx, uint16_t* Rx, uint32 MsgSize);
// uint32 SPI_Recevie_Links(uint16_t* Rx, uint32 MsgSize);
// uint32 SPI_Send2Links(uint16_t* Tx, uint32 MsgSize);

 uint32 SendCmdAndPec2Slave(const uint16_t cmd);
 void SendCMD2Slave_alone(const uint16_t cmd);
 uint32_t SendCMD2Slave_pollAndWait(const uint16_t cmd, const uint32_t wait_periods_us);
 //---------------------------------------------------------------------------------------------------------
 void WriteRegGroup(const uint16_t cmd, const uint16_t *data);
 bool ReadRegGroup(const uint16_t cmd, uint16_t *data);
 bool WriteThenReadRegGroup(const uint16_t W_cmd, const uint16_t R_cmd, const uint16_t *W_data);

 bool ReadMultiRegGroups(const uint16_t *cmds, uint8_t NumOfCmds, uint16_t *data);
 void WriteMultiRegGroups(const uint16_t *cmds, uint8_t NumOfCmds, uint16_t *data);
 bool WriteThenReadMultiRegGroups(const uint16_t *W_cmds, const uint16_t *R_cmds, uint8_t NumOfCmds, uint16_t *data);
 //---------------------------------------------------------------------------------------------------------
 void SendClearThenMeasureCMD(const uint16_t cmd_clear, const uint16_t cmd_Measure);
 bool SendClearThenCheckCMD(const uint16_t cmd_clear, const uint16_t cmd_check);
 //---------------------------------------------------------------------------------------------------------
 void WriteBytes2RegGroup(const uint16_t cmd, const uint8_t* Bytes);
 bool ReadBytesFromRegGroup(const uint16_t cmd, uint8_t* Bytes);
 bool WriteThenReadRegGroup_Bytes(const uint16_t W_cmd, const uint16_t R_cmd, const uint8_t *W_data);
// uint32 WriteNibbles2RegGroup(const uint16_t cmd, uint8_t* Nibbles);
// uint32 ReadNibbles2RegGroup(const uint16_t cmd, uint8_t* Nibbles);

#endif /*SLAVECOMMUNICATION_DRIVERS_H*/
