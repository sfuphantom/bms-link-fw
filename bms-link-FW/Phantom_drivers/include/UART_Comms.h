//Author: James Floe
#ifndef UART_COMMS_H
#define UART_COMMS_H
#include <stdint.h>

void SCI_SendCellVoltages();
void SCI_Print(char *msg);
void SCI_SendGPIOVoltages(uint16_t *GPIOVolts, uint32_t NumGPIOs);
void SCI_SendFaultsAndWarnings(void);
void SCI_PrintArray(uint16_t *data, uint16_t count);

#endif
