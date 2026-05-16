/*
Author: James Floe
*/

#include "reg_adc.h"

#ifndef ADC_CURNTTRANS_H
#define ADC_CURNTTRANS_H

#define ADREFHI_V 5
#define ADREFLO_V 0
#define ADC_DATARES 10

#define TRANSDUCER_VREF 5
#define UREF TRANSDUCER_VREF/2
#define TRANSDUCER_CURRENT_RANGE 200

#define VDIVIDER_R2 3300
#define VDIVIDER_R1 1690

#define ADC_MODULE adcREG1
#define ADC_GROUP 1

// Function Declarations
float convert_ADCVal_toVolt(uint16_t digiVal);
float convertVoltageToCurrent(float voltage);
uint32_t get_currentOnce(adcBASE_t *adc, uint32_t group);
#endif
