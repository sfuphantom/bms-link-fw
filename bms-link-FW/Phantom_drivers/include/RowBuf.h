/*
 * RowBuf.h
 *
 *  Created on: Sep 1, 2026
 *      Author: tanjo
 */

#ifndef PHANTOM_DRIVERS_INCLUDE_ROWBUF_H_
#define PHANTOM_DRIVERS_INCLUDE_ROWBUF_H_

 void shiftRowBufElements(void* const Buf, const size_t RowSize, const size_t QueueSize);
 void prePendRowIntoBuf(void* const Buf, const void* row, const size_t RowSize, const size_t QueueSize);
 inline void peekRowBuf(const void* const Buf, void* const row, const size_t RowSize);
 inline void initRowBuf(void* const Buf, const uint32_t initVal, const size_t RowSize, const size_t QueueSize);


#endif /* PHANTOM_DRIVERS_INCLUDE_ROWBUF_H_ */
