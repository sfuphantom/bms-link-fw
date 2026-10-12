/*
 * RowBuf.h
 *
 *  Created on: Sep 1, 2026
 *      Author: tanjo
 *
 *  NOTE ON FIXES:
 *   1. This header used size_t and uint32_t without including anything that
 *      defines them. It only compiled before because every .c file that
 *      happened to include it also happened to include <string.h>/<stdint.h>
 *      first. Any new file that includes only RowBuf.h would fail to build.
 *      Added the includes it actually needs.
 *   2. peekRowBuf()/initRowBuf() were declared "inline" (no "static") here,
 *      with their only body living in RowBuf.c, also marked plain "inline"
 *      (no "extern"). Per C99, a plain inline *definition* with no "extern"
 *      anywhere is not guaranteed to produce an external-linkage copy for
 *      the linker to find - whether another .c file can successfully call
 *      them is compiler/flag dependent (classic C99 footgun; GNU89 "inline"
 *      semantics are the opposite of C99's and easy to mix up). Since
 *      neither function touches any private state, they're now genuine
 *      "static inline" functions defined entirely in this header, which is
 *      always correct regardless of compiler/dialect.
 *      shiftRowBufElements()/prePendRowIntoBuf() do real work and stay as
 *      ordinary extern functions, declared here and defined once in RowBuf.c.
 *   3. Added getRowBufWritePrt() - a declared, correctly-typed replacement
 *      for RowBuf.c's old "SendWritePrt", which was never prototyped here
 *      (so nothing outside RowBuf.c could safely call it), had an unused
 *      `index` parameter, and returned `const void*` for what's meant to be
 *      a writable slot. BatteryData.c's GetXxxWritePrt() helpers now use
 *      this instead of repeating the shift-then-return-pointer boilerplate.
 *   4. initRowBuf()'s fill value was typed `uint32_t`, which reads as "fills
 *      with a 32-bit pattern" but memset() only ever fills with a single
 *      repeated byte (only the low 8 bits of the argument are used). Changed
 *      the parameter to `int` to match memset()'s actual signature/behavior
 *      and avoid the misleading implication.
 *
 *  PRECONDITION (not enforced - flagging for awareness): QueueSize must be
 *  >= 1. shiftRowBufElements() computes (QueueSize-1) on an unsigned type;
 *  QueueSize==0 would underflow to SIZE_MAX and walk off into unrelated
 *  memory. Every current caller passes a compile-time constant >= 1, so this
 *  is low risk today, but there's no guard if that ever changes.
 */

#ifndef PHANTOM_DRIVERS_INCLUDE_ROWBUF_H_
#define PHANTOM_DRIVERS_INCLUDE_ROWBUF_H_

#include <stddef.h>
#include <stdint.h>
#include <string.h>

/* Shifts every row in Buf up by one slot (row QueueSize-2 -> QueueSize-1,
 * ..., row 0 -> row 1), leaving row 0 stale/duplicated and ready to be
 * overwritten - e.g. by prePendRowIntoBuf() or getRowBufWritePrt(). */
void shiftRowBufElements(void* const Buf, const size_t RowSize, const size_t QueueSize);

/* Shifts Buf (as above) then copies `row` into the now-free row 0 slot. */
void prePendRowIntoBuf(void* const Buf, const void* row, const size_t RowSize, const size_t QueueSize);

/* Shifts Buf (as above) and returns a pointer to row 0, for callers that want
 * to write the new row's data directly into the buffer rather than copying
 * it in via prePendRowIntoBuf(). */
void* getRowBufWritePrt(void* const Buf, const size_t RowSize, const size_t QueueSize);

/* Pure helpers with no dependency on any module's private state - safe as
 * true static-inline (fix #2 above). */
static inline void peekRowBuf(const void* const Buf, void* const row, const size_t RowSize){
    memcpy(row, Buf, RowSize);
}
static inline void initRowBuf(void* const Buf, const int initVal, const size_t RowSize, const size_t QueueSize){
    memset(Buf, initVal, RowSize * QueueSize);
}

#endif /* PHANTOM_DRIVERS_INCLUDE_ROWBUF_H_ */
