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
#include "spi.h"
#include "SlaveCommunication_Drivers.h"
//------------------------------------------------------------------------
//////////////////////////////////////////////////////////////////
#define CELLS_PER_SLAVE_BOARD          12
#define AUCILIARY_PER_SLAVE_BOARD       6
#define CELL_IN_PARALLEL                5

#define GPIOS_PER_SLAVE_BOARD           (AUCILIARY_PER_SLAVE_BOARD-1)
#define REF_2ND_PER_SLAVE_BOARD         1

/////////////////////////////////////////////////////////////////
#define NUMBER_OF_CONFIG_REG_GROUPS_PER_BOARD           1
#define NUMBER_OF_STAT_REG_GROUPS_PER_BOARD             2

#define NUMBER_OF_CELL_VOLTAGE_REG_GROUPS_PER_BOARD     (CELLS_PER_SLAVE_BOARD/WORDS_PER_REG_GROUP)
#define NUMBER_OF_GPIO_VOLTAGE_REG_GROUPS_PER_BOARD     (AUCILIARY_PER_SLAVE_BOARD/WORDS_PER_REG_GROUP)
/////////////////////////////////////////////////////////////////

#define NUMBER_OF_CELLS             (CELLS_PER_SLAVE_BOARD     * NUMBER_OF_SLAVE_BOARDS)
#define NUMBER_OF_AUCILIARY         (AUCILIARY_PER_SLAVE_BOARD * NUMBER_OF_SLAVE_BOARDS)
#define NUMBER_OF_GPIOS             (GPIOS_PER_SLAVE_BOARD     * NUMBER_OF_SLAVE_BOARDS)
#define NUMBER_OF_REF_2ND           (REF_2ND_PER_SLAVE_BOARD   * NUMBER_OF_SLAVE_BOARDS)

//#define CELL_IN_SERIES               NUMBER_OF_CELLS
#define NUMBER_OF_CONFIG_WORDS      (NUMBER_OF_REG_WORDS_PER_CMD * NUMBER_OF_CONFIG_REG_GROUPS_PER_BOARD)
#define NUMBER_OF_STAT_WORDS        (NUMBER_OF_REG_WORDS_PER_CMD * NUMBER_OF_STAT_REG_GROUPS_PER_BOARD)
/////////////////////////////////////////////////////////////////
#define ADC_MEASURE_MODE 2 //0=Fast, 1=Normal, 2=Filtered
#define ADC_Measure_Discharge_Permit TRUE

#define SLAVE_CONVERSATION_TIMEOUT 20
#define NUMBER_OF_GARBAGE_BYTES ((NUMBER_OF_SLAVE_BOARDS-7)/8)
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

#define ITMP_MILLI_VOLTS_2_CELCIUS 7.5f
#define ITMP_KELVIN_2_CELCIUS  273
/////////////////////////////////////////////////////////////////
#define CELL_BALANCE_THESHOLD_VOLTS_ADC 1000 //0.1V
#define CELL_BALANCE_HYSTERSIS_BAND_VOLTS_ADC 500 // 0.05
#define CELL_BALANCE_TRIGGER_HIGH_VOLTS_ADC (CELL_BALANCE_THESHOLD_VOLTS_ADC + CELL_BALANCE_HYSTERSIS_BAND_VOLTS_ADC)
#define CELL_BALANCE_TRIGGER_LOW_VOLTS_ADC (CELL_BALANCE_THESHOLD_VOLTS_ADC - CELL_BALANCE_HYSTERSIS_BAND_VOLTS_ADC)

#define VOLTS_ADC_DRAINED_PER_PULSE 100
//------------------------------------------------------------------------
#define MAX_INTERNAL_DIE_TEMPERATURE_FLAG  55000
#define MIN_INTERNAL_DIE_TEMPERATURE_FLAG  55000

#define MAX_ANALOG_POWER_SUPPLY_VOLTAGE_FLAG  55000
#define MIN_ANALOG_POWER_SUPPLY_VOLTAGE_FLAG  45000
#define MAX_DIGITAL_POWER_SUPPLY_VOLTAGE_FLAG 27000
#define MIN_DIGITAL_POWER_SUPPLY_VOLTAGE_FLAG 36000
#define MAX_2ND_REFERENCE_VOLTAGE_FLAG        29900
#define MIN_2ND_REFERENCE_VOLTAGE_FLAG        30100

#define OVER_VOLTAGE_FLAG       41000
#define UNDER_VOLTAGE_FLAG      33000
#define OVER_VOLTAGE_CONFIG     (OVER_VOLTAGE_FLAG>>4)
#define UNDER_VOLTAGE_CONFIG    ((UNDER_VOLTAGE_FLAG>>4)-1)


#define MAX_CELL_CHARGING_VOLTAGE_TARGET 42000

//------------------------------------------------------------------------
struct ConfigReg{
    bool     adcopt; // ADC Mode Option Bit
    bool     DTEN;   // The Discharge Timer Enable (READ ONLY)
    bool     refon;  // References Powered Up
    uint8_t  gpio;   // The GPIO bits
    uint16_t VUV;    // Undervoltage Comparison Voltage
    uint16_t VOV;    // Overvoltage Comparison Voltage
    uint16_t DCC;    //  Discharge Cell x
    uint8_t  dcto;   //  Discharge Time Out Value
};
struct StatusReg {
    uint16_t SC;    /* Sum of All Cells raw ADC value  */
    uint16_t ITMP;  /* Internal die temperature raw    */
    uint16_t VA;    /* Analog supply (VREG) raw ADC    */

    uint16_t VD;        /* Digital supply (VREGD) raw ADC  */
    uint16_t OV_flags;  /* Overvoltage  flags, cells 1-12  */
    uint16_t UV_flags;  /* Undervoltage flags, cells 1-12  */
    bool     THSD;      /* Thermal shutdown occurred        */
    bool     MUXFAIL;   /* MUX self-test failed             */
    uint8_t  REV;       /* Device revision code             */
};

//////////////////////////////////////////////////////////////////

//typedef enum  {adcopt, DTEN, refon, gpio, DCC, dcto, VUV, VOV}ConfigReg_Values;
//typedef enum  {SC, ITMP, VA, VD, OV_flags, UV_flags, THSD, MUXFAIL, REV}StatusReg_Values;

//---------------------------------------------------------------------------------------------------------
//void SetValueConfigReg(uint16_t* data, ConfigReg_Values Value2Change);
//void GetValueConfigReg(uint16_t* data, ConfigReg_Values Value2Get);
// void SetAllConfigReg(bool* adcopt, bool* DTEN, bool* refon, uint8_t* gpio,  uint16_t* VUV, uint16_t* VOV, uint16_t* DCC, uint8_t* dcto);
//void GetValueStatusReg(uint16_t* data, StatusReg_Values Value2Get);
 //---------------------------------------------------------------------------------------------------------

uint32_t Write_CFGR();
void Read_CFGR();
void Read_STAT();
//--------------------------------------------------------------------------------------------------------
void ClearCellsCMD();
void ClearAUXCMD();
void ClearStatCMD();
//---------------------------------------------------------------------------------------------------------
 uint32 MeasureCellsCmd(const uint8_t MD, //ADC Mode
                        const bool DCP,   //Discharge Permit
                        const uint8_t CHG //  GPIO Selection for ADC Conversion
                        );

 uint32 MeasureAUXCmd(const uint8_t MD,     // ADC mode: 0=Fast, 1=Normal, 2=Filtered
                      const uint8_t CHG    //  GPIO Selection for ADC Conversion
                      );
 uint32 MeasureSTATCmd(const uint8_t MD,     // ADC mode: 0=Fast, 1=Normal, 2=Filtered
                       const uint8_t CHST    //  Status Group Selection
                     );
 //---------------------------------------------------------------------------------------------------------

 bool isConvComplete();
 bool isADCFree();
 bool waitADCFree();

 uint32_t waitConvComplete(const uint32_t wait_periods_us);
 uint32_t waitConvComplete_ADC_Cells();
 uint32_t waitConvComplete_ADC_GPIO();
 uint32_t waitConvComplete_ADC_STAT();
 uint32_t waitConvComplete_Cell_Bal();
 uint32_t waitConvComplete_ADC_STAT();

 uint32_t waitSPIFree();
 //---------------------------------------------------------------------------------------------------------
 bool GetVoltageReadings(uint16_t* data);
 bool GetGPIOReadings_Analog(uint16_t* data);
 bool GetGPIOReadings_Digital(uint8_t *gpio_data);
 //---------------------------------------------------------------------------------------------------------
bool Start_S_CTRL_Pulsing();
bool Write_S_CTRL(uint8* S_CTRL_nibbles);
bool Read_S_CTRL(uint8* S_CTRL_nibbles);
bool Write_PWM(uint8* PWM_nibbles);
bool Read_PWM(uint8* PWM_nibbles);
//---------------------------------------------------------------------------------------------------------
bool GetBalanceNibbles(const uint16* Volts, uint8_t* BalanceNibbles);
bool GetBalanceDCC(const uint16* Volts, uint16_t* DCC);
//---------------------------------------------------------------------------------------------------------
void ReadConfig_DCC(uint16_t* DCC);
void SetConfig_DCC(const uint16_t* DCC);
void ReadConfig_gpio(uint8_t* gpio);
bool ReadConfig_gpio_allZero();
void SetConfig_gpio(const uint8_t* gpio);
//---------------------------------------------------------------------------------------------------------
uint32_t checkStatFlags();
//---------------------------------------------------------------------------------------------------------
void initConfig();
void ClearSlaveRegs();

#endif /* PHANTOM_DRIVERS_INCLUDE_SLAVECOMMUNATION_FUNCTIONS_H_ */
