/** @file sci_helper.c
*   @brief UART/SCI Helper Function Implementations
*
*   Convenience wrappers around the TI HALCoGen SCI driver (sci.h).
*   All functions operate on the global handle UART_REG.
*
*   Multi-byte values (uint16/uint32/float32) are sent MSB-first (big
*   endian on the wire). Adjust the byte order in the pack/unpack helpers
*   below if your receiver expects little endian instead.
*/

#include <stdint.h>
#include <stdbool.h>
#include "sci.h"
#include "sci_helpers.h"
#include <string.h>



/* ==========================================================================
 *  Internal helpers
 * ========================================================================== */

/* Reinterpret a float32 as 4 bytes without violating strict aliasing */
typedef union
{
    float32 f;
    uint32  u;
    uint8   b[4];
} sci_float_bytes_u;

typedef union
{
    uint32 u;
    uint8  b[4];
} sci_u32_bytes_u;

typedef union
{
    uint16 u;
    uint8  b[2];
} sci_u16_bytes_u;

/* ==========================================================================
 *  SEND FUNCTIONS
 * ========================================================================== */

void UART_sendByte(uint8 data)
{
    sciSendByte(UART_REG, data);
}

void UART_sendArray(uint8 *data, uint32 length)
{
    sciSend(UART_REG, length, data);
}

void UART_sendUint16(uint16 data)
{
    sci_u16_bytes_u v;
    v.u = data;
    /* MSB first */
    UART_sendByte(v.b[1]);
    UART_sendByte(v.b[0]);
}

void UART_sendUint32(uint32 data)
{
    sci_u32_bytes_u v;
    v.u = data;
    /* MSB first */
    UART_sendByte(v.b[3]);
    UART_sendByte(v.b[2]);
    UART_sendByte(v.b[1]);
    UART_sendByte(v.b[0]);
}

void UART_sendUint16Array(uint16 *data, uint32 length)
{
    uint32 i;
    for (i = 0U; i < length; i++)
    {
        UART_sendUint16(data[i]);
    }
}

void UART_sendUint32Array(uint32 *data, uint32 length)
{
    uint32 i;
    for (i = 0U; i < length; i++)
    {
        UART_sendUint32(data[i]);
    }
}

void UART_sendFloat(float32 data)
{
    sci_float_bytes_u v;
    v.f = data;
    UART_sendUint32(v.u);
}

void UART_sendFloatArray(float32 *data, uint32 length)
{
    uint32 i;
    for (i = 0U; i < length; i++)
    {
        UART_sendFloat(data[i]);
    }
}

void UART_sendString(char *str)
{
    UART_sendArray((uint8 *)str, (uint32)strlen(str));
}

void UART_sendStringLine(char *str)
{
    UART_sendString(str);
    UART_sendEndChar();
}

void UART_sendEndChar(void)
{
    UART_sendByte(UART_DEFAULT_END_CHAR);
}

void UART_sendChar(uint8 endChar)
{
    UART_sendByte(endChar);
}

void UART_sendStartCharID(sciPacketID_t id)
{
    UART_sendByte(UART_DEFAULT_START_CHAR);
    UART_sendByte((uint8)id);
}

/* ==========================================================================
 *  RECEIVE ("RESERVE") FUNCTIONS
 * ========================================================================== */

uint8 UART_receiveByte(void)
{
    return (uint8)sciReceiveByte(UART_REG);
}

void UART_receiveArray(uint8 *data, uint32 length)
{
    sciReceive(UART_REG, length, data);
}

uint16 UART_receiveUint16(void)
{
    sci_u16_bytes_u v;
    v.b[1] = UART_receiveByte();  /* MSB first, matches UART_sendUint16 */
    v.b[0] = UART_receiveByte();
    return v.u;
}

uint32 UART_receiveUint32(void)
{
    sci_u32_bytes_u v;
    v.b[3] = UART_receiveByte();  /* MSB first, matches UART_sendUint32 */
    v.b[2] = UART_receiveByte();
    v.b[1] = UART_receiveByte();
    v.b[0] = UART_receiveByte();
    return v.u;
}

void UART_receiveUint16Array(uint16 *data, uint32 length)
{
    uint32 i;
    for (i = 0U; i < length; i++)
    {
        data[i] = UART_receiveUint16();
    }
}

void UART_receiveUint32Array(uint32 *data, uint32 length)
{
    uint32 i;
    for (i = 0U; i < length; i++)
    {
        data[i] = UART_receiveUint32();
    }
}

float32 UART_receiveFloat(void)
{
    sci_float_bytes_u v;
    v.u = UART_receiveUint32();
    return v.f;
}

void UART_receiveFloatArray(float32 *data, uint32 length)
{
    uint32 i;
    for (i = 0U; i < length; i++)
    {
        data[i] = UART_receiveFloat();
    }
}

uint32 UART_receiveString(char *buffer, uint32 maxLength)
{
    return UART_receiveUntilChar(buffer, maxLength, UART_DEFAULT_END_CHAR);
}

sciPacketID_t UART_receiveStartCharID(void)
{
    uint8 c;

    /* discard bytes until the start char shows up */
    do
    {
        c = UART_receiveByte();
    } while (c != UART_DEFAULT_START_CHAR);

    return (sciPacketID_t)UART_receiveByte();
}

uint32 UART_receiveUntilChar(char *buffer, uint32 maxLength, uint8 endChar)
{
    uint32 count = 0U;
    uint8 c;

    /* leave room for the null terminator */
    while (count < (maxLength - 1U))
    {
        c = UART_receiveByte();

        if (c == endChar)
        {
            break;
        }

        buffer[count] = (char)c;
        count++;
    }

    buffer[count] = '\0';
    return count;
}
