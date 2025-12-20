#include "uart_driver.h"
#include <string.h>
/* Pick the SCI instance you configured in HalCoGen.
 * Most common is sciREG1. If your pinmux uses SCI2, change to sciREG2.
 */
#define UART_SCI_REG sciREG1

void UART_init(void)
{
    // HalCoGen function for configuring clock
    sciInit();
}

void UART_sendByte(uint8_t byte)
{
    sciSendByte(sciREG, byte);
}

void UART_sendString(const char *str)
{
    if (str == 0) return;

    while (*str != '\0')
    {
        uint8_t b = (uint8_t)(*str++);
        UART_sendByte(b);
    }
}



