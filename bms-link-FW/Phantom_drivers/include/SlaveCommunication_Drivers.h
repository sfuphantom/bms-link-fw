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
#include "PhantomHelpers.h"

/////////////////////////////////////////////////////////////////
#define NUMBER_OF_FAILS_ALLOWED         2
/////////////////////////////////////////////////////////////////
#define NUMBER_OF_CELLS_SLAVES             (CELLS_PER_SLAVE_BOARD     * NUMBER_OF_SLAVE_BOARDS_TOTAL)
#define NUMBER_OF_AUCILIARY_SLAVES         (AUCILIARY_PER_SLAVE_BOARD * NUMBER_OF_SLAVE_BOARDS_TOTAL)
#define NUMBER_OF_GPIOS_SLAVES             (GPIOS_PER_SLAVE_BOARD     * NUMBER_OF_SLAVE_BOARDS_TOTAL)
#define NUMBER_OF_REF_2ND_SLAVES           (REF_2ND_PER_SLAVE_BOARD   * NUMBER_OF_SLAVE_BOARDS_TOTAL)
/////////////////////////////////////////////////////////////////

//#define CELL_IN_SERIES               NUMBER_OF_CELLS
#define NUMBER_OF_CONFIG_WORDS_SLAVES      (NUMBER_OF_REG_WORDS_PER_CMD * NUMBER_OF_CONFIG_REG_GROUPS_PER_BOARD)
#define NUMBER_OF_STAT_WORDS_SLAVES        (NUMBER_OF_REG_WORDS_PER_CMD * NUMBER_OF_STAT_REG_GROUPS_PER_BOARD)
//#define SPI_DUMMY_CMD  0xFFFF

#define MAX_POLLS_TIMEOUT 2000
#define NUMBER_OF_GARBAGE_BYTES     (Bit2Bytes_Ceil(NUMBER_OF_SLAVE_BOARDS_TOTAL))
//------------------------------------------------------------------------
#define MAX_INTERNAL_DIE_TEMPERATURE_FLAG  Temp2SLAVE_ADC(120.0f)
#define MIN_INTERNAL_DIE_TEMPERATURE_FLAG  Temp2SLAVE_ADC(-40.0f)

#define MAX_ANALOG_POWER_SUPPLY_VOLTAGE_FLAG  VOLTS2SLAVE_ADC(5.5f)//55000
#define MIN_ANALOG_POWER_SUPPLY_VOLTAGE_FLAG  VOLTS2SLAVE_ADC(4.5f)//45000
#define MAX_DIGITAL_POWER_SUPPLY_VOLTAGE_FLAG VOLTS2SLAVE_ADC(3.6f)//36000
#define MIN_DIGITAL_POWER_SUPPLY_VOLTAGE_FLAG VOLTS2SLAVE_ADC(2.7f)//27000
#define MAX_2ND_REFERENCE_VOLTAGE_FLAG        VOLTS2SLAVE_ADC(3.01f)//30100
#define MIN_2ND_REFERENCE_VOLTAGE_FLAG        VOLTS2SLAVE_ADC(2.99f)//29900

#define OVER_VOLTAGE_FLAG       VOLTS2SLAVE_ADC(CELL_VOLT_100_FULL_F)
#define UNDER_VOLTAGE_FLAG      VOLTS2SLAVE_ADC(CELL_VOLT_0_FULL_F)
#define OVER_VOLTAGE_CONFIG     (OVER_VOLTAGE_FLAG>>4)
#define UNDER_VOLTAGE_CONFIG    ((UNDER_VOLTAGE_FLAG>>4)-1)
//----------------------------------------------------------------------------------------
//uint16_t pec15Table[256];
#define PEC_INIT_VALUE 0x0010
#define PEC_CHARACTERISTIC_POLYNOMIAL 0x4599

void init_PEC15_Table();
 uint16_t pec15_calc(uint8_t len, const uint16_t *data);
 //---------------------------------------------------------------------------------------------------------
 void wakeup_idle();
 void wakeup_sleep();
 //---------------------------------------------------------------------------------------------------------
// uint32 SPI_SendAndRecevie_Links(uint16_t* Tx, uint16_t* Rx, uint32 MsgSize);
// uint32 SPI_Recevie_Links(uint16_t* Rx, uint32 MsgSize);
// uint32 SPI_Send2Links(uint16_t* Tx, uint32 MsgSize);

 void SendCmdAndPec2Slave(const uint16_t cmd);
 void SendCMD2Slave_alone(const uint16_t cmd);
#define CUSTOM_POLL_WAIT FALSE
#if CUSTOM_POLL_WAIT
// uint32_t SendCMD2Slave_pollAndWait(const uint16_t cmd, const uint32_t wait_periods_us);
#else
uint32_t SendCMD2Slave_pollAndWait(const uint16_t cmd);
#endif
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
