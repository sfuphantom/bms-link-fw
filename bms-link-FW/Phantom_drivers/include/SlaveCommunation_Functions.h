/*
 * SlaveCommunation_Functions.h
 *
 *  Created on: Jun 7, 2026
 *      Author: tanjo
 */

#ifndef PHANTOM_DRIVERS_INCLUDE_SLAVECOMMUNATION_FUNCTIONS_H_
#define PHANTOM_DRIVERS_INCLUDE_SLAVECOMMUNATION_FUNCTIONS_H_
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "string.h"
#include "ltc6811_commands.h"
#include "SlaveCommunation_Hardware.h"
#include "FullBattery_Hardware.h"
#include "BatteryCell_Hardware.h"
#include "SlaveCommunication_Drivers.h"

//------------------------------------------------------------------------
#define DUMMY_CMD LTC6811_PLADC
#define ADC_NO_DATA 0xFFFFU                 // what a register holds until it has been converted
//------------------------------------------------------------------------
//Fault Flag Data
#define MAX_INTERNAL_DIE_TEMPERATURE_FLAG  Temp2SLAVE_ADC(120.0f)
#define MIN_INTERNAL_DIE_TEMPERATURE_FLAG  Temp2SLAVE_ADC(-40.0f)

#define MAX_ANALOG_POWER_SUPPLY_VOLTAGE_FLAG  VOLTS2SLAVE_ADC(5.5f)//55000
#define MIN_ANALOG_POWER_SUPPLY_VOLTAGE_FLAG  VOLTS2SLAVE_ADC(4.5f)//45000
#define MAX_DIGITAL_POWER_SUPPLY_VOLTAGE_FLAG VOLTS2SLAVE_ADC(3.6f)//36000
#define MIN_DIGITAL_POWER_SUPPLY_VOLTAGE_FLAG VOLTS2SLAVE_ADC(2.7f)//27000
#define MAX_2ND_REFERENCE_VOLTAGE_FLAG        VOLTS2SLAVE_ADC(3.01f)//30100
#define MIN_2ND_REFERENCE_VOLTAGE_FLAG        VOLTS2SLAVE_ADC(2.99f)//29900
//------------------------------------------------------------------------
// config Data
#define OVER_VOLTAGE_FLAG       VOLTS2SLAVE_ADC(CELL_VOLT_100_FULL_F)
#define UNDER_VOLTAGE_FLAG      VOLTS2SLAVE_ADC(CELL_VOLT_0_FULL_F)
#define OVER_VOLTAGE_CONFIG     (OVER_VOLTAGE_FLAG>>4)
#define UNDER_VOLTAGE_CONFIG    ((UNDER_VOLTAGE_FLAG>>4)-1)
//---------------------------------------------------------------------------------------------------------
// Registers. Each one is a union of the bytes (as they go over the SPI) and a view of the same bytes.
// Every member of the view is one byte or less, so nothing depends on the endianness. Values that
// are split over bytes (VUV, VOV, DCC) or 16 bit (SC, ITMP, ...) are read with the functions below.
//
// Bit fields are filled from the top bit on a big endian compiler, from the bottom bit on a little
// endian one. If your compiler does it differently set BITFIELDS_MSB_FIRST yourself; initLink checks it.
//---------------------------------------------------------------------------------------------------------
#ifndef BITFIELDS_MSB_FIRST
  #if defined(__BIG_ENDIAN__) || defined(__big_endian__) || \
      (defined(__BYTE_ORDER__) && defined(__ORDER_BIG_ENDIAN__) && (__BYTE_ORDER__ == __ORDER_BIG_ENDIAN__))
    #define BITFIELDS_MSB_FIRST 1
  #else
    #define BITFIELDS_MSB_FIRST 0
  #endif
#endif

typedef struct {                // CFGR0
#if BITFIELDS_MSB_FIRST
    uint8_t gpio    : 5;        // GPIO5..1: write 1 = pull-down off, read = pin level
    uint8_t refon   : 1;        // references stay on
    uint8_t DTEN    : 1;        // read only
    uint8_t adcopt  : 1;
#else
    uint8_t adcopt  : 1;
    uint8_t DTEN    : 1;
    uint8_t refon   : 1;
    uint8_t gpio    : 5;
#endif
} Cfgr0_t;

typedef struct {                // CFGR2
#if BITFIELDS_MSB_FIRST
    uint8_t vov_lo  : 4;        // VOV[3:0]
    uint8_t vuv_hi  : 4;        // VUV[11:8]
#else
    uint8_t vuv_hi  : 4;
    uint8_t vov_lo  : 4;
#endif
} Cfgr2_t;

typedef struct {                // CFGR5
#if BITFIELDS_MSB_FIRST
    uint8_t dcto    : 4;        // discharge time out, read = time left
    uint8_t dcc_hi  : 4;        // DCC12..9
#else
    uint8_t dcc_hi  : 4;
    uint8_t dcto    : 4;
#endif
} Cfgr5_t;

typedef struct {                // STBR5
#if BITFIELDS_MSB_FIRST
    uint8_t REV     : 4;
    uint8_t rsvd    : 2;
    uint8_t MUXFAIL : 1;
    uint8_t THSD    : 1;
#else
    uint8_t THSD    : 1;
    uint8_t MUXFAIL : 1;
    uint8_t rsvd    : 2;
    uint8_t REV     : 4;
#endif
} Stbr5_t;

// Configuration register group A of one chip
typedef union {
    uint8_t bytes[BYTES_PER_REG_GROUP];
    struct {
        Cfgr0_t cfgr0;
        uint8_t vuv_lo;         // CFGR1
        Cfgr2_t cfgr2;
        uint8_t vov_hi;         // CFGR3: VOV[11:4]
        uint8_t dcc_lo;         // CFGR4: DCC8..1
        Cfgr5_t cfgr5;
    } f;
} ConfigReg_t;

// Status register groups A and B of one chip
typedef union {
    uint8_t bytes[NUMBER_OF_STAT_REG_GROUPS_PER_BOARD * BYTES_PER_REG_GROUP];
    struct {
        uint8_t sc[2];          // little endian words
        uint8_t itmp[2];
        uint8_t va[2];
        uint8_t vd[2];
        uint8_t uvov[3];        // C1UV C1OV C2UV C2OV ... C12OV, first cell in the lowest bit
        Stbr5_t flags;
    } f;
} StatusReg_t;

uint16_t Config_getVUV(const ConfigReg_t * const c);
uint16_t Config_getVOV(const ConfigReg_t * const c);
uint16_t Config_getDCC(const ConfigReg_t * const c);
void     Config_setVUV(ConfigReg_t * const c, const uint16_t value);
void     Config_setVOV(ConfigReg_t * const c, const uint16_t value);
void     Config_setDCC(ConfigReg_t * const c, const uint16_t value);

uint16_t Status_SC(const StatusReg_t * const s);
uint16_t Status_ITMP(const StatusReg_t * const s);
uint16_t Status_VA(const StatusReg_t * const s);
uint16_t Status_VD(const StatusReg_t * const s);
uint16_t Status_OV_flags(const StatusReg_t * const s);      // bit n = cell n+1
uint16_t Status_UV_flags(const StatusReg_t * const s);

const StatusReg_t* GetStatusRegData(void);                  // one per chip
const ConfigReg_t* GetConfigRegData(void);
//---------------------------------------------------------------------------------------------------------
SlaveStatus_t initLink(void);
SlaveStatus_t initConfig(void);
SlaveStatus_t wakeup_sleep_and_SetConfig(void);             // always wakes the chips
SlaveStatus_t wakeIfAsleep(void);                           // only if checkAsleep says so
SlaveStatus_t KeepAlive(void);                              // a valid command, resets the 2 s watchdog
SlaveStatus_t waitDummyCMD(const uint32_t WaitPeriod_ms, const uint32_t WaitPeriod_us, uint16_t WaitSends);
//---------------------------------------------------------------------------------------------------------
// Config. Read_CFGR replaces the config in memory with what the chips hold.
SlaveStatus_t Write_CFGR(void);
SlaveStatus_t Read_CFGR(void);
SlaveStatus_t WriteThenRead_CFGR(void);                     // SLAVE_ERR_MISMATCH if the chips do not hold what was written
SlaveStatus_t checkAsleep(uint32_t * const asleepBoards);   // SLAVE_ERR_ASLEEP, bit n of the mask = chip n; mask may be NULL

void ReadConfig_DCC(uint16_t * const DCC);
void SetConfig_DCC(const uint16_t * const DCC);
void SetAllConfig_DCC(const uint16_t DCC);
void ReadConfig_gpio(uint8_t * const gpio);
bool ReadConfig_gpio_allZero(void);
void SetConfig_gpio(const uint8_t * const gpio);
//---------------------------------------------------------------------------------------------------------
// Status and faults
SlaveStatus_t Read_STAT(void);
SlaveStatus_t ReadStatAndGetFlags(uint32_t * const faults); // faults: bit n = Slave_Faults n, all chips together
uint32_t checkStatFlags(void);                              // all chips together
void     checkStatFlags_perBoard(uint16_t * const faults);  // one entry per chip

void getStatus_SC(float * const SC);
void getStatus_BoardTempC(float * const BoardTempC);
void getStatus_RefVolt2nd(float * const RefVolt2nd);
//--------------------------------------------------------------------------------------------------------
SlaveStatus_t ClearCellsCMD(void);
SlaveStatus_t ClearAUXCMD(void);
SlaveStatus_t ClearStatCMD(void);
SlaveStatus_t ClearSCtrlCMD(void);
SlaveStatus_t ClearSlaveRegs(void);

// MD: ADC mode 0=Fast, 1=Normal, 2=Filtered. Each of them waits until the ADC is done.
SlaveStatus_t MeasureCellsCmd(const uint8_t MD, const bool DCP, const uint8_t CHG);
SlaveStatus_t MeasureCellsCmd_All_NoDis(const uint8_t MD);
SlaveStatus_t MeasureAUXCmd(const uint8_t MD, const uint8_t CHG);
SlaveStatus_t MeasureAUXCmd_All(const uint8_t MD);
SlaveStatus_t MeasureSTATCmd(const uint8_t MD, const uint8_t CHST);
SlaveStatus_t MeasureSTATCmd_All(const uint8_t MD);
//---------------------------------------------------------------------------------------------------------
SlaveStatus_t GetVoltageReadings(uint16_t * const data);
SlaveStatus_t GetGPIOReadings_Analog(uint16_t * const data);
SlaveStatus_t GetCellReadings_Slave(const uint8_t board, uint16_t * const cells);   // one chip, CELLS_PER_SLAVE_BOARD values
//---------------------------------------------------------------------------------------------------------
SlaveStatus_t Start_S_CTRL_Pulsing(void);
SlaveStatus_t Write_S_CTRL(const uint8_t * const nibbles);
SlaveStatus_t Read_S_CTRL(uint8_t * const nibbles);
SlaveStatus_t Write_PWM(const uint8_t * const nibbles);
SlaveStatus_t Read_PWM(uint8_t * const nibbles);
SlaveStatus_t WriteThenRead_PWM(const uint8_t * const nibbles);
SlaveStatus_t SetAllPWM_Regs(const uint8_t nibble);
//---------------------------------------------------------------------------------------------------------
SlaveStatus_t MeasureCellVoltage_16(uint16_t * const CellVolt);
SlaveStatus_t MeasureAUXVoltage16(uint16_t * const GPIO_Data);
SlaveStatus_t MeasureRef2ndVoltage(void);

SlaveStatus_t MeasureCellVoltage(float * const CellVolt, const size_t NumberOfRealValues);
SlaveStatus_t MeasureAUXVoltage(float * const GPIO_Data, const size_t NumberOfRealValues);

float Slave_ADC2Celsius(const uint16_t ADC);
float Slave_SC_ADC2Volt(const uint16_t ADC_Word);

#endif /* PHANTOM_DRIVERS_INCLUDE_SLAVECOMMUNATION_FUNCTIONS_H_ */
