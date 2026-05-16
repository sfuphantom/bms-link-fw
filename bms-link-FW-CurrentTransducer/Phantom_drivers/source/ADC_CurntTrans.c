#include <stdint.h>

#include "ADC_CurntTrans.h"
#include "adc.h"
#include "spi.h"  // SPI driver functions
#include "sys_common.h"

// Converts ADC's buffer value from digital reading to voltage
float convert_ADCVal_toVolt(uint16_t digiVal)
{
    float v;
    #if ADC_DATARES == 10
        v = ((float)digiVal/1023)*(ADREFHI_V - ADREFLO_V) + ADREFLO_V;
    #else
        v = ((float)digiVal/4095)*(ADREFHI_V - ADREFLO_V) + ADREFLO_V;
    #endif

    return v;
}

// Converts Voltage to Current Reading (for HTFS-200P Current Transducer)
float convertVoltageToCurrent(float voltage)
{
    float vRefFromTransducer = (voltage * (VDIVIDER_R1 + VDIVIDER_R2)) / VDIVIDER_R2;
    float current = (TRANSDUCER_CURRENT_RANGE / 1.25) * (vRefFromTransducer - UREF);
    return current;
}

// Performs single ADC conversion and returns the properly calculated current value
uint32_t get_currentOnce(adcBASE_t *adc, uint32_t group)
{
    adcData_t digitalVal[1];

    adcStartConversion(adc, group);
    while(!adcIsConversionComplete(adc, group)) {}
    adcStopConversion(adc, group);
    adcGetData(adc, group, &digitalVal[0]);

    float voltage = convert_ADCVal_toVolt(digitalVal->value);
    float current = convertVoltageToCurrent(voltage);

    return (uint32_t)current;
}
