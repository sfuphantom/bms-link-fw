/*
Author: James Floe
*/

//#def USE_RTOS //define if using RTOS

#ifndef ADC_CURNTTRANS_H
#define ADC_CURNTTRANS_H

#include "reg_adc.h"
#include <stdint.h>
#include <stdbool.h>

#define ADC_MAX_10BIT_COUNTS 1023U
#define ADC_MAX_12BIT_COUNTS 4095U

#define ADREFHI_V 5
#define ADREFLO_V 0
#define ADC_DATARES 12

#define TRANSDUCER_VREF 5
#define UREF 2.5
#define TRANSDUCER_CURRENT_RANGE 200

#define VDIVIDER_R2 3300
#define VDIVIDER_R1 1690

#define ADC_MODULE adcREG1
#define ADC_GROUP 1
#define ADC_WAIT_CONV_COMPLETE 0xff

#define OC_WARN_A        95.0f
#define OC_FAULT_A       100.0f
#define OC_SEVERE_A      120.0f
#define SENSOR_MAX_A     150.0f

#define ADC_SHORT_GND_mV     100U
#define ADC_SHORT_VREF_mV    4800U

#define CURRENT_ADC_MIN_VALID_mV  500U
#define CURRENT_ADC_MAX_VALID_mV  2800U
#define TRANSDUCER_MIN_VALID_mV   750U
#define TRANSDUCER_MAX_VALID_mV   4250U
#define CURRENT_MAX_VALID_A       210.0f
#define CURRENT_MAX_STEP_A_PER_SAMPLE   100.0f

#define CURRENT_SENSOR_INVALID_FAULT_MASK \
    (CURNT_FAULT_BIT(FAULT_CURRENT_SENSOR_OPEN)         | \
     CURNT_FAULT_BIT(FAULT_CURRENT_SENSOR_SHORT_TO_GND) | \
     CURNT_FAULT_BIT(FAULT_CURRENT_SENSOR_SHORT_TO_VREF)| \
     CURNT_FAULT_BIT(FAULT_CURRENT_SENSOR_TIMEOUT)      | \
     CURNT_FAULT_BIT(FAULT_CURRENT_SENSOR_IMPLAUSIBLE))

typedef struct
{
    uint16_t raw_adc;
    uint32_t adc_voltage_V;
    uint32_t transducer_voltage_V;
    float current_A;
    uint8_t faults;
    uint8_t warnings;
    bool valid;
} CurrentTrans_Vals_t;

typedef enum {FAULT_OVERCURRENT, FAULT_OVERCURRENT_SEVERE, FAULT_CURRENT_SENSOR_OPEN, FAULT_CURRENT_SENSOR_SHORT_TO_GND, FAULT_CURRENT_SENSOR_SHORT_TO_VREF, FAULT_CURRENT_SENSOR_TIMEOUT, FAULT_CURRENT_SENSOR_IMPLAUSIBLE}CurrentTrans_Faults;
typedef enum {WARNING_OVERCURRENT, WARNING_TIMEOUT}CurrentTrans_Warnings;

#define CURNT_FAULT_BIT(CurrentTrans_Faults)  ((uint8_t)(1U << (uint8_t)(CurrentTrans_Faults)))
#define CURNT_WARNING_BIT(CurrentTrans_Warnings) ((uint8_t)(1U << (uint8_t)(CurrentTrans_Warnings)))

// Function Declarations
uint32_t convert_ADCVal_toVolt(uint16_t digiVal);
uint32_t convertVoltageToCurrent(float voltage);
void get_currentOnce(adcBASE_t *adc, uint32_t group);

uint8_t scanForCurrentTransFaults();
void setCurrentFloat(float current);
uint32_t getCurrentUint32();
void setCurntTransADCVal(uint16_t raw_adc);
uint16_t getCurntTransADCVal();
void setCurntTransADCVoltage(uint32_t voltage);
uint32_t getCurntTransADCVoltage();
void setCurrentTransducerVoltage(uint32_t trans_Voltage);
uint32_t getCurrentTransducerVoltage();
uint8_t getCurrentTransFaults();
void setCurntTransFaults(uint8_t faults);
uint8_t getCurntTransFaults();
void setCurntTransWarnings(uint8_t warnings);
uint8_t getCurntTransWarnings();
void setCurrentValid(bool valid);

#endif
