#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include "SlaveCommunation_Functions.h"
#include "spi.h"
#include "sci.h"
#include "adc.h"
#include "SlaveCommunication_Drivers.h"
#include "BatteryData.h"
#include "ADC_CurntTrans.h"

void SCI_SendCellVoltages()
{
    char sciBuf[256];
    uint32_t len = 0;
    uint32_t i;
    float VoltCells[NUMBER_OF_CELLS];


    Slave_ADC2Volt_arr(GetCellVoltReadPrt(), VoltCells, (uint16_t)NUMBER_OF_CELLS);

    float AvgCellVolt = (float)GetAvgCellVolt();

    len += snprintf(&sciBuf[len],
                    sizeof(sciBuf) - len,
                    "Cells: ");

    for(i = 0; i < NUMBER_OF_CELLS; i++)
    {
        len += snprintf(&sciBuf[len],
                        sizeof(sciBuf) - len,
                        "C%lu=%.4fV ",
                        (unsigned long)(i + 1),
                        VoltCells[i]);
    }

    len += snprintf(&sciBuf[len],
                    sizeof(sciBuf) - len,
                    "Avg=%.4fV\r\n",
                    AvgCellVolt);

    sciSend(sciREG, len, (uint8 *)sciBuf);
}
void SCI_Print(char *msg)
{
    sciSend(sciREG,
            strlen(msg),
            (uint8 *)msg);
}
void SCI_SendGPIOVoltages(uint16_t *GPIOVolts,
                          uint32_t NumGPIOs)
{
    char sciBuf[256];
    uint32_t len = 0;
    uint32_t i;

    len += snprintf(&sciBuf[len],
                    sizeof(sciBuf) - len,
                    "GPIOs: ");

    for(i = 0; i < NumGPIOs; i++)
    {
        len += snprintf(&sciBuf[len],
                        sizeof(sciBuf) - len,
                        "G%lu=%u ",
                        (unsigned long)(i + 1),
                        GPIOVolts[i]);
    }

    len += snprintf(&sciBuf[len],
                    sizeof(sciBuf) - len,
                    "\r\n");

    sciSend(sciREG, len, (uint8 *)sciBuf);
}
void SCI_SendFaultsAndWarnings(void)
{
    char sciBuf[512];
    uint32_t len = 0;
    uint8_t i;

    uint8_t  imdFault = GetAllIMDFaults();
    uint32_t slaveFaults = GetAllSlaveFaults();
    uint8_t  currentTransFaults = getCurntTransFaults();
    uint8_t  currentTransWarnings = getCurntTransWarnings();

    const char *slaveFaultNames[] =
    {
        "Temp_HIGH",
        "OV_flags",
        "UV_flags",
        "THSD",
        "MUXFAIL",
        "ITMP_HIGH",
        "ITMP_LOW",
        "VA_HIGH",
        "VA_LOW",
        "VD_HIGH",
        "VD_LOW"
    };

    const char *currentTransFaultNames[] =
    {
        "FAULT_OVERCURRENT",
        "FAULT_OVERCURRENT_SEVERE",
        "FAULT_CURRENT_SENSOR_OPEN",
        "FAULT_CURRENT_SENSOR_SHORT_TO_GND",
        "FAULT_CURRENT_SENSOR_SHORT_TO_VREF",
        "FAULT_CURRENT_SENSOR_TIMEOUT",
        "FAULT_CURRENT_SENSOR_IMPLAUSIBLE"
    };

    const char *currentTransWarningNames[] =
    {
        "WARNING_OVERCURRENT",
        "WARNING_TIMEOUT"
    };

    len += snprintf(&sciBuf[len],
                    sizeof(sciBuf) - len,
                    "IMD_Fault=0x%02X ",
                    imdFault);

    if (imdFault != 0U)
    {
        len += snprintf(&sciBuf[len],
                        sizeof(sciBuf) - len,
                        "IMD_FAULT_ACTIVE");
    }
    else
    {
        len += snprintf(&sciBuf[len],
                        sizeof(sciBuf) - len,
                        "None");
    }

    len += snprintf(&sciBuf[len],
                    sizeof(sciBuf) - len,
                    "\r\nSlave_Faults=0x%08lX ",
                    (unsigned long)slaveFaults);

    for (i = 0; i < 11; i++)
    {
        if (slaveFaults & (1UL << i))
        {
            len += snprintf(&sciBuf[len],
                            sizeof(sciBuf) - len,
                            "%s ",
                            slaveFaultNames[i]);
        }
    }

    if (slaveFaults == 0U)
    {
        len += snprintf(&sciBuf[len],
                        sizeof(sciBuf) - len,
                        "None");
    }

    len += snprintf(&sciBuf[len],
                    sizeof(sciBuf) - len,
                    "\r\nCurrentTrans_Faults=0x%02X ",
                    currentTransFaults);

    for (i = 0; i < 7; i++)
    {
        if (currentTransFaults & (1U << i))
        {
            len += snprintf(&sciBuf[len],
                            sizeof(sciBuf) - len,
                            "%s ",
                            currentTransFaultNames[i]);
        }
    }

    if (currentTransFaults == 0U)
    {
        len += snprintf(&sciBuf[len],
                        sizeof(sciBuf) - len,
                        "None");
    }

    len += snprintf(&sciBuf[len],
                    sizeof(sciBuf) - len,
                    "\r\nCurrentTrans_Warnings=0x%02X ",
                    currentTransWarnings);

    for (i = 0; i < 2; i++)
    {
        if (currentTransWarnings & (1U << i))
        {
            len += snprintf(&sciBuf[len],
                            sizeof(sciBuf) - len,
                            "%s ",
                            currentTransWarningNames[i]);
        }
    }

    if (currentTransWarnings == 0U)
    {
        len += snprintf(&sciBuf[len],
                        sizeof(sciBuf) - len,
                        "None");
    }

    len += snprintf(&sciBuf[len],
                    sizeof(sciBuf) - len,
                    "\r\n");

    sciSend(sciREG, len, (uint8 *)sciBuf);
}

void SCI_PrintArray(const uint16_t *data, uint16_t count)
{
    char buf[16];
    uint16_t i;

    if (data == NULL)
    {
        SCI_Print("NULL array\r\n");
        return;
    }

    for (i = 0; i < count; i++)
    {
        snprintf(buf, sizeof(buf), "0x%04X", data[i]);
        SCI_Print(buf);

        if (i < (count - 1U))
        {
            SCI_Print(", ");
        }
    }

    SCI_Print("\r\n");
}
