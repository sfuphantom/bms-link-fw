/*
 * LogHelpers.c
 */

#include "LogHelpers.h"
#include "PhantomTimers.h"

#if SERIAL_TRANSPORT == SERIAL_TRANSPORT_SCI
#include "sci.h"
#include "reg_sci.h"

#define USE_USB_N_PINS 1
#if USE_USB_N_PINS
    #define UART_REG        sciREG
#else
    #define UART_REG        scilinREG
#endif
#endif

#define FRAME_HEADER_LEN    4           // SOH, id, length (2)
#define TICK_LEN            8
#define CRC_INIT            0xFFFFU

// A frame is only 'atomic' if nothing else can start one halfway. Put the IRQ disable / enable here
// if anything logs from an ISR. They only wrap the busy flag, not the (slow) UART send.
#ifndef SERIAL_CRITICAL_ENTER
#warning SERIAL_CRITICAL_ENTER / EXIT are empty, fill them in if logs can be sent from an ISR
#define SERIAL_CRITICAL_ENTER()     ((void)0)
#define SERIAL_CRITICAL_EXIT()      ((void)0)
#endif

static volatile bool txBusy = false;
static uint16_t      frameCrc;
static uint32_t      framesDropped;

//------------------------------------------------------------------------------------
// Transport
//------------------------------------------------------------------------------------
// sciSend has to be in polling mode (no TX interrupt), the data is on the stack.
void Serial_Write(const uint8_t * const data, const uint16_t len){
#if SERIAL_TRANSPORT == SERIAL_TRANSPORT_SCI
    sciSend(UART_REG, len, (uint8_t *)data);        // sciSend does not change the data, it just has no const
#else
    SerialPort_Write(data, len);
#endif
}

bool Serial_ReadByte(uint8_t * const byte){
#if SERIAL_TRANSPORT == SERIAL_TRANSPORT_SCI
    if (!sciIsRxReady(UART_REG)) return false;
    *byte = (uint8_t)sciReceiveByte(UART_REG);
    return true;
#else
    return SerialPort_ReadByte(byte);
#endif
}

//------------------------------------------------------------------------------------
// Frames
//------------------------------------------------------------------------------------
// CRC-16/CCITT, poly 0x1021, init CRC_INIT. Adds to frameCrc, so it can continue where it stopped.
static void Crc_Add(const uint8_t * data, uint16_t len){
#if SERIAL_USE_CRC
    uint8_t bit;

    while (len--) {
        frameCrc ^= (uint16_t)((uint16_t)(*data++) << 8);
        for (bit = 0; bit < 8; bit++) {
            frameCrc = (frameCrc & 0x8000U) ? (uint16_t)((frameCrc << 1) ^ 0x1021U) : (uint16_t)(frameCrc << 1);
        }
    }
#else
    (void)data;
    (void)len;
#endif
}

static bool Tx_Lock(void){
    bool idle;

    SERIAL_CRITICAL_ENTER();
    idle   = !txBusy;
    txBusy = true;
    SERIAL_CRITICAL_EXIT();
    return idle;
}

bool Serial_FrameStart(const msg_ID_t id, const uint16_t bodyLen){
    uint8_t hdr[FRAME_HEADER_LEN + TICK_LEN];
    const uint64_t tick = getNow_tick();

    if (!Tx_Lock()) {
        framesDropped++;
        return false;
    }

    hdr[0] = SOH_BYTE;
    hdr[1] = (uint8_t)id;
    write_u16_BE(&hdr[2], (uint16_t)(TICK_LEN + bodyLen));
    write_u32_BE(&hdr[4], (uint32_t)(tick >> 32));
    write_u32_BE(&hdr[8], (uint32_t)tick);
    Serial_Write(hdr, sizeof(hdr));

    frameCrc = CRC_INIT;
    Crc_Add(&hdr[1], sizeof(hdr) - 1);      // everything but SOH
    return true;
}

void Serial_FrameSend(const uint8_t * const data, const uint16_t len){
    if (len == 0) return;

    Serial_Write(data, len);
    Crc_Add(data, len);
}

void Serial_FrameEnd(void){
    uint8_t tail[3];
    uint8_t n = 0;

#if SERIAL_USE_CRC
    write_u16_BE(tail, frameCrc);
    n = 2;
#endif
    tail[n++] = EOT_BYTE;
    Serial_Write(tail, n);
    txBusy = false;
}

uint32_t Serial_GetDroppedFrames(void){
    return framesDropped;
}
