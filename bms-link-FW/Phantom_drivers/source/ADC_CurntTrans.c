#include <stdint.h>

#include "ADC_CurntTrans.h"
#include "adc.h"
#include "spi.h"  // SPI driver functions
#include "sys_common.h"
#include <math.h>

CurrentTrans_Vals_t currentValsStruct;

// Converts ADC's buffer value from digital reading to voltage
uint32_t convert_ADCVal_toVolt(uint16_t digiVal)
{
    float voltage_V;
    uint16_t adcMaxCounts;

#if ADC_DATARES == 10
    adcMaxCounts = ADC_MAX_10BIT_COUNTS;
#else
    adcMaxCounts = ADC_MAX_12BIT_COUNTS;
#endif

    if (digiVal > adcMaxCounts)
    {
        digiVal = adcMaxCounts;
    }

    voltage_V = ((float)digiVal / (float)adcMaxCounts) *
                (ADREFHI_V - ADREFLO_V) + ADREFLO_V;

    return (uint32_t)((voltage_V * 1000.0f) + 0.5f);
}

static uint32_t adc_mV_to_transducer_mV(uint32_t adc_mV)
{
    uint64_t numerator;

    numerator = (uint64_t)adc_mV * (uint64_t)(VDIVIDER_R1 + VDIVIDER_R2);

    return (uint32_t)(numerator / VDIVIDER_R2);
}

// Converts Voltage to Current Reading (for HTFS-200P Current Transducer)
uint32_t convertVoltageToCurrent(float voltage)
{
    float vRefFromTransducer = (voltage * (VDIVIDER_R1 + VDIVIDER_R2)) / VDIVIDER_R2;
    float current = (TRANSDUCER_CURRENT_RANGE / 1.25) * (vRefFromTransducer - UREF);
    return current;
}

void setCurrentFloat(float current_A)
{
    currentValsStruct.current_A = current_A;
}

void setCurrentValid(bool valid)
{
    currentValsStruct.valid = valid;
}

uint32_t getCurrentUint32()
{
    /*
     * Your requested getter has an input argument, but it should not.
     * Better prototype:
     *
     *      uint32_t getCurrent(void);
     *
     * This ignores the argument to preserve your requested declaration.
     */

    if (currentValsStruct.current_A < 0.0f)
    {
        return 0U;
    }

    return (uint32_t)(currentValsStruct.current_A + 0.5f);
}

void setCurntTransADCVal(uint16_t raw_adc)
{
    currentValsStruct.raw_adc = raw_adc;
}

uint16_t getCurntTransADCVal(void)
{
    return currentValsStruct.raw_adc;
}

void setCurntTransADCVoltage(uint32_t voltage)
{
    /*
     * Stored in millivolts.
     */
    currentValsStruct.adc_voltage_V = voltage;
}

uint32_t getCurntTransADCVoltage(void)
{
    /*
     * Returned in millivolts.
     */
    return currentValsStruct.adc_voltage_V;
}

void setCurrentTransducerVoltage(uint32_t trans_Voltage)
{
    /*
     * Stored in millivolts.
     */
    currentValsStruct.transducer_voltage_V = trans_Voltage;
}

uint32_t getCurrentTransducerVoltage(void)
{
    /*
     * Returned in millivolts.
     */
    return currentValsStruct.transducer_voltage_V;
}

uint8_t getCurrentTransFaults(void)
{
    return currentValsStruct.faults;
}

void setCurntTransFaults(uint8_t faults)
{
    currentValsStruct.faults = faults;
}

uint8_t getCurntTransFaults(void)
{
    return currentValsStruct.faults;
}

void setCurntTransWarnings(uint8_t warnings)
{
    currentValsStruct.warnings = warnings;
}

uint8_t getCurntTransWarnings(void)
{
    return currentValsStruct.warnings;
}

uint8_t scanForCurrentTransWarnings(void)
{
    uint8_t warnings = 0U;
    float current_A = currentValsStruct.current_A;

    if ((current_A > OC_WARN_A) &&
        (current_A <= OC_FAULT_A))
    {
        warnings |= CURNT_WARNING_BIT(WARNING_OVERCURRENT);
    }

    /*
     * If a timeout fault is active, also expose the timeout warning.
     */
    if ((getCurntTransFaults() &
         CURNT_FAULT_BIT(FAULT_CURRENT_SENSOR_TIMEOUT)) != 0U)
    {
        warnings |= CURNT_WARNING_BIT(WARNING_TIMEOUT);
    }

    return warnings;
}

uint8_t scanForCurrentTransFaults(void)
{
    uint8_t faults = 0U;

    uint32_t adc_mV = getCurntTransADCVoltage();
    uint32_t transducer_mV = getCurrentTransducerVoltage();
    float current_A = (float)getCurrentUint32();

    // Direct Low Fault
    if (adc_mV <= ADC_SHORT_GND_mV)
    {
        faults |= CURNT_FAULT_BIT(FAULT_CURRENT_SENSOR_SHORT_TO_GND);
    }

    if ((adc_mV > ADC_SHORT_GND_mV) &&
        (adc_mV < CURRENT_ADC_MIN_VALID_mV))
    {
        faults |= CURNT_FAULT_BIT(FAULT_CURRENT_SENSOR_OPEN);
    }

    /*
     * ADC pin directly shorted high.
     */
    if (adc_mV >= ADC_SHORT_VREF_mV)
    {
        faults |= CURNT_FAULT_BIT(FAULT_CURRENT_SENSOR_SHORT_TO_VREF);
    }

    /*
     * Signal too high for the expected current transducer range.
     * Because of your divider, a transducer-side short to 5 V may only show
     * up as about 3.3 V at the ADC pin, not 5 V.
     */
    if ((adc_mV > CURRENT_ADC_MAX_VALID_mV) ||
        (transducer_mV > TRANSDUCER_MAX_VALID_mV))
    {
        faults |= CURNT_FAULT_BIT(FAULT_CURRENT_SENSOR_SHORT_TO_VREF);
    }

    /*
     * Signal too low for expected transducer range.
     */
    if ((adc_mV < CURRENT_ADC_MIN_VALID_mV) ||
        (transducer_mV < TRANSDUCER_MIN_VALID_mV))
    {
        faults |= CURNT_FAULT_BIT(FAULT_CURRENT_SENSOR_OPEN);
    }

    /*
     * Overcurrent checks.
     */
    if (current_A > OC_FAULT_A)
    {
        faults |= CURNT_FAULT_BIT(FAULT_OVERCURRENT);
    }

    if (current_A > OC_SEVERE_A)
    {
        faults |= CURNT_FAULT_BIT(FAULT_OVERCURRENT_SEVERE);
    }

    /*
     * Implausible current checks.
    */
    if (!isfinite(current_A))
    {
        faults |= CURNT_FAULT_BIT(FAULT_CURRENT_SENSOR_IMPLAUSIBLE);
    }

    if (fabsf(current_A) > CURRENT_MAX_VALID_A)
    {
        faults |= CURNT_FAULT_BIT(FAULT_CURRENT_SENSOR_IMPLAUSIBLE);
    }

    return faults;
}

#ifndef USE_RTOS

void get_currentOnce(adcBASE_t *adc, uint32_t group)
{
    adcData_t digitalVal[1];
    uint32_t adc_mV;
    uint32_t transducer_mV;
    float adc_voltage_V;
    float current_A;
    uint8_t faults;
    uint8_t warnings;
    uint32_t timeout = ADC_WAIT_CONV_COMPLETE;

    adcStartConversion(adc, group);

    while (!adcIsConversionComplete(adc, group))
    {
        if (timeout == 0U)
        {
            adcStopConversion(adc, group);

            setCurntTransFaults(CURNT_FAULT_BIT(FAULT_CURRENT_SENSOR_TIMEOUT));
            setCurntTransWarnings(CURNT_WARNING_BIT(WARNING_TIMEOUT));
            setCurrentValid(false);

            return;
        }

        timeout--;
    }

    adcStopConversion(adc, group);
    adcGetData(adc, group, &digitalVal[0]);

    setCurntTransADCVal(digitalVal[0].value);

    adc_mV = convert_ADCVal_toVolt(digitalVal[0].value);
    setCurntTransADCVoltage(adc_mV);

    transducer_mV = adc_mV_to_transducer_mV(adc_mV);
    setCurrentTransducerVoltage(transducer_mV);

    adc_voltage_V = (float)adc_mV / 1000.0f;
    current_A = convertVoltageToCurrent(adc_voltage_V);
    setCurrentFloat(current_A);

    faults = scanForCurrentTransFaults();
    setCurntTransFaults(faults);

    warnings = scanForCurrentTransWarnings();
    setCurntTransWarnings(warnings);

    if ((faults & CURRENT_SENSOR_INVALID_FAULT_MASK) != 0U)
    {
        setCurrentValid(false);
    }
    else
    {
        setCurrentValid(true);
    }
}

#endif

/* =========================
 * RTOS-friendly version
 * ========================= */

#ifdef USE_RTOS

void get_currentOnce(adcBASE_t *adc, uint32_t group)
{
    adcData_t digitalVal[1];
    uint32_t adc_mV;
    uint32_t transducer_mV;
    float adc_voltage_V;
    float current_A;
    uint8_t faults;
    uint8_t warnings;

    TickType_t startTick;

    adcStartConversion(adc, group);

    startTick = xTaskGetTickCount();

    while (!adcIsConversionComplete(adc, group))
    {
        if ((xTaskGetTickCount() - startTick) >= CURRENT_ADC_TIMEOUT_TICKS)
        {
            adcStopConversion(adc, group);

            setCurntTransFaults(CURNT_FAULT_BIT(FAULT_CURRENT_SENSOR_TIMEOUT));
            setCurntTransWarnings(CURNT_WARNING_BIT(WARNING_TIMEOUT));
            setCurrentValid(false);

            return;
        }

        /*
         * Lets another ready task run instead of burning CPU.
         * For very fast ADC conversions, taskYIELD() is usually less disruptive
         * than vTaskDelay(1), because vTaskDelay(1) can delay by a whole tick.
         */
        taskYIELD();
    }

    adcStopConversion(adc, group);
    adcGetData(adc, group, &digitalVal[0]);

    setCurntTransADCVal(digitalVal[0].value);

    adc_mV = convert_ADCVal_toVolt(digitalVal[0].value);
    setCurntTransADCVoltage(adc_mV);

    transducer_mV = adc_mV_to_transducer_mV(adc_mV);
    setCurrentTransducerVoltage(transducer_mV);

    adc_voltage_V = (float)adc_mV / 1000.0f;
    current_A = convertVoltageToCurrent(adc_voltage_V);
    setCurrentFloat(current_A);

    faults = scanForCurrentTransFaults();
    setCurntTransFaults(faults);

    warnings = scanForCurrentTransWarnings();
    setCurntTransWarnings(warnings);

    if ((faults & CURRENT_SENSOR_INVALID_FAULT_MASK) != 0U)
    {
        setCurrentValid(false);
    }
    else
    {
        setCurrentValid(true);
    }
}

#endif
