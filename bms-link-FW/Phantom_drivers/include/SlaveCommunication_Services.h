
#include <stdint.h>
#include <stdbool.h>
#include "spi.h"
#include "SlaveCommunication.h"


//////////////////////////////////////////////////////////////////
#define ADC_Measure_Mode 2 //0=Fast, 1=Normal, 2=Filtered
#define ADC_Measure_Discharge_Permit TRUE

#define SLAVE_CONVERSATION_TIMEOUT 20

//////////////////////////////////////////////////////////////////
struct SlaveBatteryCellData_struct {
  uint16_t Temp[NUMBER_OF_CELLS];
  uint16_t Volt[NUMBER_OF_CELLS];
  // uint16_t Cells_to_discharge[NUMBER_OF_SLAVE_BOARDS];
};

struct ConfigReg_A{
    bool     adcopt; // ADC Mode Option Bit
    bool     DTEN;   // The Discharge Timer Enable (READ ONLY)
    bool     refon;  // References Powered Up
    uint8_t  gpio;   // The GPIO bits
    uint16_t VUV;    // Undervoltage Comparison Voltage
    uint16_t VOV;    // Overvoltage Comparison Voltage
    uint16_t DCC;    //  Discharge Cell x
    uint8_t  dcto;   //  Discharge Time Out Value
};
struct StatusReg_A {
    uint16_t SC;    /* Sum of All Cells raw ADC value  */
    uint16_t ITMP;  /* Internal die temperature raw    */
    uint16_t VA;    /* Analog supply (VREG) raw ADC    */
};

/* Status Register Group B: Digital supply, OV/UV flags, diagnostics */
struct StatusReg_B {
    uint16_t VD;        /* Digital supply (VREGD) raw ADC  */
    uint16_t OV_flags;  /* Overvoltage  flags, cells 1-12  */
    uint16_t UV_flags;  /* Undervoltage flags, cells 1-12  */
    bool     THSD;      /* Thermal shutdown occurred        */
    bool     MUXFAIL;   /* MUX self-test failed             */
    uint8_t  REV;       /* Device revision code             */
};
//////////////////////////////////////////////////////////////////

typedef enum  {adcopt, DTEN, refon, gpio, DCC, dcto, VUV, VOV}ConfigReg_A_Values;
typedef enum  {SC, ITMP, VA}StatusReg_A_Values;
typedef enum  {VD, OV_flags, UV_flags, THSD, MUXFAIL, REV}StatusReg_B_Values;

//////////////////////////////////////////////////////////////////
void wakeup_idle();
void wakeup_sleep();
//---------------------------------------------------------------------------------------------------------

void SetValueConfigReg_A(uint16_t* data, ConfigReg_A_Values Value2Change);
void GetValueConfigReg_A(uint16_t* data, ConfigReg_A_Values Value2Get);
 void SetAllConfigReg_A(bool* adcopt, bool* DTEN, bool* refon, uint8_t* gpio,  uint16_t* VUV, uint16_t* VOV, uint16_t* DCC, uint8_t* dcto);
//---------------------------------------------------------------------------------------------------------

 uint32 Write_CFGR_A();
 uint32 Read_CFGR_A();
 uint32_t Read_STATA();
 uint32_t Read_STATB();
 //---------------------------------------------------------------------------------------------------------
 uint32 MeasureCellsCmd(uint8_t MD, //ADC Mode
                        bool DCP,   //Discharge Permit
                        uint8_t CHG //  GPIO Selection for ADC Conversion
                        );

 uint32 MeasureGPIOCmd(uint8_t MD,     // ADC mode: 0=Fast, 1=Normal, 2=Filtered
                       uint8_t CHG    //  GPIO Selection for ADC Conversion
                      );


 bool isConvComplete(void);
 bool waitConvComplete();
 bool waitConvComplete_ADC_Cells();
 bool waitConvComplete_ADC_GPIO();
 bool waitConvComplete_Cell_Bal();
 //---------------------------------------------------------------------------------------------------------
 bool GetVoltageReadings(uint16_t* data);
 bool GetGPIOReadings_Analog(uint16_t* data);
 bool GetGPIOReadings_Digital(uint8_t *gpio_data);
 //---------------------------------------------------------------------------------------------------------
bool Start_S_CTRL_Pulsing();
uint32 Write_S_CTRL(uint8* S_CTRL_nibbles);
uint32 Read_S_CTRL(uint8* S_CTRL_nibbles);
uint32 Write_PWM(uint8* PWM_nibbles);
uint32 Read_PWM(uint8* PWM_nibbles);

 void ClearSlaveRegs();

 void initLink();

 //---------------------------------------------------------------------------------------------------------
 bool MeasureCellVoltageRoutine(uint16_t* VoltDataOut);

#define UseAnilog FLASE
#if UseAnilog
 bool MeasureGPIOVoltageRoutine(uint16_t* GPIODataOut);
#else
 bool MeasureGPIOVoltageRoutine(uint8_t* GPIODataOut);
#endif

 bool BalanceCellsRoutine(uint16_t* VoltInData);

