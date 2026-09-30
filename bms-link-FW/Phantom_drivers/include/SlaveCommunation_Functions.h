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
#include "ltc6811_commands.h"
#include "SlaveCommunation_Hardware.h"
#include "FullBattery_Hardware.h"
#include "BatteryCell_Hardware.h"

#include "BatteryData.h"


//------------------------------------------------------------------------
#define DUMMY_CMD LTC6811_PLADC
////////////////////////////////////////////////////////////////////

 //---------------------------------------------------------------------------------------------------------
#define POLL_PERIOD_STAT_US 1000
#define POLL_PERIOD_AUX_US 1000
#define POLL_PERIOD_CELL_VOLTS_US 1000
#define POLL_PERIOD_CELL_BAL_US 1000

//------------------------------------------------------------------------
typedef struct {
    bool     adcopt; // ADC Mode Option Bit
    bool     DTEN;   // The Discharge Timer Enable (READ ONLY)
    bool     refon;  // References Powered Up
    uint8_t  gpio;   // The GPIO bits
    uint16_t VUV;    // Undervoltage Comparison Voltage
    uint16_t VOV;    // Overvoltage Comparison Voltage
    uint16_t DCC;    //  Discharge Cell x
    uint8_t  dcto;   //  Discharge Time Out Value
}ConfigReg;
typedef struct {
    uint16_t SC;    /* Sum of All Cells raw ADC value  */
    uint16_t ITMP;  /* Internal die temperature raw    */
    uint16_t VA;    /* Analog supply (VREG) raw ADC    */

    uint16_t VD;        /* Digital supply (VREGD) raw ADC  */
    uint16_t OV_flags;  /* Overvoltage  flags, cells 1-12  */
    uint16_t UV_flags;  /* Undervoltage flags, cells 1-12  */
    bool     THSD;      /* Thermal shutdown occurred        */
    bool     MUXFAIL;   /* MUX self-test failed             */
    uint8_t  REV;       /* Device revision code             */

    uint16_t RefVolt2nd;
}StatusReg_t;

//////////////////////////////////////////////////////////////////

//typedef enum  {adcopt, DTEN, refon, gpio, DCC, dcto, VUV, VOV}ConfigReg_Values;
//typedef enum  {SC, ITMP, VA, VD, OV_flags, UV_flags, THSD, MUXFAIL, REV}StatusReg_Values;

//---------------------------------------------------------------------------------------------------------
//void SetValueConfigReg(uint16_t* data, ConfigReg_Values Value2Change);
//void GetValueConfigReg(uint16_t* data, ConfigReg_Values Value2Get);
// void SetAllConfigReg(bool* adcopt, bool* DTEN, bool* refon, uint8_t* gpio,  uint16_t* VUV, uint16_t* VOV, uint16_t* DCC, uint8_t* dcto);
//void GetValueStatusReg(uint16_t* data, StatusReg_Values Value2Get);
 //---------------------------------------------------------------------------------------------------------
StatusReg_t* GetStatusRegData();

void Write_CFGR();
bool Read_CFGR();
bool Read_STAT();
uint32_t checkStatFlags();
//--------------------------------------------------------------------------------------------------------
void ClearCellsCMD();
void ClearAUXCMD();
void ClearStatCMD();
void ClearSCtrlCMD();
void ClearSlaveRegs();

//---------------------------------------------------------------------------------------------------------
bool MeasureCellsCmd(const uint8_t MD,     //ADC Mode
                        const bool DCP,     //Discharge Permit
                        const uint8_t CHG   //Cell Selection for ADC Conversion
                        );
 bool MeasureCellsCmd_All_NoDis(const uint8_t MD);    //ADC Mode
 bool MeasureCellsCmd_Dis(const uint8_t MD,   //ADC Mode
                          const uint8_t CHG   //GPIO Selection for ADC Conversion
                          );

 bool MeasureAUXCmd(const uint8_t MD,       // ADC mode: 0=Fast, 1=Normal, 2=Filtered
                    const uint8_t CHG       // Cell Selection for ADC Conversion
                    );
 bool MeasureAUXCmd_All(const uint8_t MD);  // ADC mode: 0=Fast, 1=Normal, 2=Filtered

 bool MeasureSTATCmd(const uint8_t MD,      // ADC mode: 0=Fast, 1=Normal, 2=Filtered
                     const uint8_t CHST     //  Status Group Selection
                     );
 bool MeasureSTATCmd_All(const uint8_t MD); // ADC mode: 0=Fast, 1=Normal, 2=Filtered
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
void Write_S_CTRL(const uint8_t* S_CTRL_nibbles);
bool Read_S_CTRL(uint8_t* S_CTRL_nibbles);
void Write_PWM(const uint8_t* PWM_nibbles);
bool Read_PWM(uint8_t* PWM_nibbles);
bool WriteThenRead_PWM(const uint8_t* nibbles);
bool SetAllPWM_Regs(uint8_t nibble);
//---------------------------------------------------------------------------------------------------------
void ReadConfig_DCC(uint16_t* DCC);
void SetConfig_DCC(const uint16_t* DCC);
void SetAllConfig_DCC(const uint16_t DCC);
void ReadConfig_gpio(uint8_t* gpio);
bool ReadConfig_gpio_allZero();
void SetAllConfig_DCC(const uint16_t DCC);
void SetAllHigh_DCC(const uint16_t DCC);
void SetConfig_gpio(const uint8_t* gpio);
//---------------------------------------------------------------------------------------------------------
void initConfig();
//---------------------------------------------------------------------------------------------------------
void SendDummyCMD();
void waitDummyCMD(const uint32_t WaitPeriod_ms, const uint32_t WaitPeriod_us, uint16_t WaitSends);
//---------------------------------------------------------------------------------------------------------
void initLink();
//---------------------------------------------------------------------------------------------------------
bool MeasureCellVoltage(uint16_t* CellVolt);
bool MeasureRef2ndVoltage();
 bool MeasureAUXVoltage(uint16_t* GPIO_DataOut);
 bool ReadStatAndGetFlags();


#endif /* PHANTOM_DRIVERS_INCLUDE_SLAVECOMMUNATION_FUNCTIONS_H_ */
