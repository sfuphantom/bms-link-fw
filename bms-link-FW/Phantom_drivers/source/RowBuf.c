/*
 * RowBuf.c
 *
 *  Created on: Sep 1, 2026
 *      Author: tanjo
 *
 *  See RowBuf.h for the fix notes (inline linkage, missing includes, the
 *  new getRowBufWritePrt() replacing the old dead/un-prototyped SendWritePrt).
 */

#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include "RowBuf.h"


void shiftRowBufElements(void* const Buf, const size_t RowSize, const size_t QueueSize){
    size_t i;
    uint8_t* Buf8 = (uint8_t*)Buf + (RowSize * (QueueSize-1));
    for(i=QueueSize-1; i>0; i--){
        memcpy(Buf8, Buf8 - RowSize, RowSize);
        Buf8 -= RowSize;
    }
}

void prePendRowIntoBuf(void* const Buf, const void* row, const size_t RowSize, const size_t QueueSize){
    // [row0][row1][row2][row3][row4][row5] >> [newRow][row0][row1][row2][row3][row4]
    shiftRowBufElements(Buf, RowSize, QueueSize);
    memcpy(Buf, row, RowSize);
}

void* getRowBufWritePrt(void* const Buf, const size_t RowSize, const size_t QueueSize){
    shiftRowBufElements(Buf, RowSize, QueueSize);
    return Buf;
}
