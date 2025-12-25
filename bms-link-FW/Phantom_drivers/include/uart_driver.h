#ifndef UART_DRIVER_H_
#define UART_DRIVER_H_

#include <stdint.h>
#include "sci.h"   // HalCoGen SCI driver API

void UART_init(void);
void UART_sendByte(uint8_t byte);
void UART_sendString(const char *str);

#endif /* UART_DRIVER_H_ */
