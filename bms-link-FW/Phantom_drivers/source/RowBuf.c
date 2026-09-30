/*
 * RowBuf.c
 *
 *  Created on: Sep 1, 2026
 *      Author: tanjo
 */



#include <stdint.h>
#include <stdbool.h>
#include "string.h"
#include "RowBuf.h"


void shiftRowBufElements(void* const Buf, const size_t RowSize, const size_t QueueSize){
    int i;
    uint8_t* Buf8 = (uint8_t*)Buf + (RowSize * (QueueSize-1));
    for(i=QueueSize-1; i>0;i--){
        memcpy(Buf8, Buf8 - RowSize, RowSize);
        Buf8 -= RowSize;
    }
}
 void prePendRowIntoBuf(void* const Buf, const void* row, const size_t RowSize, const size_t QueueSize){
     // [row0][row1][row2][row3][row4][row5] >> [newRow][row0][row1][row2][row3][row4]

     shiftRowBufElements(Buf, RowSize, QueueSize);

     memcpy(Buf, row, RowSize);
 }
 inline void peekRowBuf(const void* const Buf, void* const row, const size_t RowSize){
     memcpy(row, Buf, RowSize);
 }

 const void* SendWritePrt(const uint8_t index, void* Buf, const size_t RowSize, const size_t QueueSize){
  shiftRowBufElements(Buf, RowSize, QueueSize);
  return Buf;
 }

 inline void initRowBuf(void* const Buf, const uint32_t initVal, const size_t RowSize, const size_t QueueSize){

     memset(Buf, initVal, RowSize * QueueSize);

//     int i;
//     uint8_t* Queue8 = (uint8_t*)Buf;
//     for(i=QueueSize-1; i>0;i--){
//         memset(Queue8, initVal, RowSize);
//         Queue8 += RowSize;
//     }
}
