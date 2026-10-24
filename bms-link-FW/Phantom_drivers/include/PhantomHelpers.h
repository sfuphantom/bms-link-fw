#ifndef PHANTOM_HELPERS_H
#define PHANTOM_HELPERS_H

/* Needs C81 or newer. */

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>

// ---------------------------------------------------------------------------------------
// Macros (generic, or they change the caller's variable, so they can't be functions)
// ---------------------------------------------------------------------------------------
#define ARRAY_LEN(arr)         (sizeof(arr) / sizeof((arr)[0]))     // real arrays only, not pointers
#define MEMBER_SIZE(type, m)   (sizeof(((type *)0)->m))
#define CONTAINER_OF(ptr, type, member) ((type *)((char *)(ptr) - offsetof(type, member)))

#define MIN_VAL(a, b)          (((a) < (b)) ? (a) : (b))
#define MAX_VAL(a, b)          (((a) > (b)) ? (a) : (b))
#define ABS_VAL(a)             (((a) < 0) ? -(a) : (a))
#define SWAP(type, a, b)       do { type tmp_ = (a); (a) = (b); (b) = tmp_; } while (0)

// constants only: bit/bits must be smaller than the width of unsigned long
#define BIT(bit)               (1UL << (bit))
#define MASK_LOW_BITS(bits)    ((1UL << (bits)) - 1UL)
#define BITS_TO_BYTES(bits)    (((bits) + 7) / 8)
#define ALIGN_UP(x, a)         (((x) + ((a) - 1)) & ~((a) - 1))     // a must be a power of 2

#define BIT_SET(x, bit)        ((x) |=  BIT(bit))
#define BIT_CLEAR(x, bit)      ((x) &= ~BIT(bit))
#define BIT_TOGGLE(x, bit)     ((x) ^=  BIT(bit))
#define BIT_IS_SET(x, bit)     (((x) & BIT(bit)) != 0)

#define ZERO_STRUCT(x)         memset(&(x), 0, sizeof(x))
#define UNUSED(x)              ((void)(x))
#define KB(n)                  ((n) * 1024UL)
#define MB(n)                  ((n) * 1024UL * 1024UL)

#define NOT_FOUND              SIZE_MAX     // returned by the arr_*_find functions

// ---------------------------------------------------------------------------------------
// Comparing single values. Every type is passed as double (floats and 32 bit ints fit exactly),
// so one function works for all of them. They also fit CompareFunc, which the arrays use.
// ---------------------------------------------------------------------------------------
typedef bool (*CompareFunc)(double a, double b);

bool isEqual(double a, double b);
bool isNotEqual(double a, double b);
bool isGreater(double a, double b);
bool isGreaterEq(double a, double b);   // same as "not less"
bool isLess(double a, double b);
bool isLessEq(double a, double b);      // same as "not greater"

// O = bound not included (open), C = bound included (closed). Example: CO is [lo, hi). lo must be <= hi.
bool isBetween_OO(double value, double lo, double hi);
bool isBetween_CC(double value, double lo, double hi);
bool isBetween_CO(double value, double lo, double hi);
bool isBetween_OC(double value, double lo, double hi);
bool isNotBetween_OO(double value, double lo, double hi);
bool isNotBetween_CC(double value, double lo, double hi);
bool isNotBetween_CO(double value, double lo, double hi);
bool isNotBetween_OC(double value, double lo, double hi);

double clampValue(double value, double lo, double hi);      // lo and hi can be in any order

// ---------------------------------------------------------------------------------------
// Float only
// ---------------------------------------------------------------------------------------
bool  isClose_f(float a, float b, float tolerance);
float lerp_f(float a, float b, float t);
float mapRange_f(float x, float inLo, float inHi, float outLo, float outHi);
float roundDecimals_f(float value, uint8_t decimals);

float tempCtoF(float c);
float tempFtoC(float f);
float tempCtoK(float c);
float tempKtoC(float k);

// ---------------------------------------------------------------------------------------
// Arrays
// Rules for all of them:
//  - "out" can be the same array as the input
//  - find/count/any/all take a compare function and a value: arr_u8_find(arr, len, isGreater, 5)
//  - min/max return false if the array is empty, idx may be NULL
//  - avg is NAN if the array is empty
//  - integer math rounds and saturates (250 + 10 stays 255)
// ---------------------------------------------------------------------------------------

// ---- float ----
void     arr_f_fill(float *arr, size_t len, float value);
bool     arr_f_equal(const float *a, const float *b, size_t len);
bool     arr_f_min(const float *arr, size_t len, float *min, size_t *idx);
bool     arr_f_max(const float *arr, size_t len, float *max, size_t *idx);
double   arr_f_sum(const float *arr, size_t len);
double   arr_f_avg(const float *arr, size_t len);
double   arr_f_variance(const float *arr, size_t len);
double   arr_f_stddev(const float *arr, size_t len);
double   arr_f_dot(const float *a, const float *b, size_t len);
size_t   arr_f_find(const float *arr, size_t len, CompareFunc cmp, double value);
size_t   arr_f_findClose(const float *arr, size_t len, float value, float tolerance);
size_t   arr_f_count(const float *arr, size_t len, CompareFunc cmp, double value);
bool     arr_f_any(const float *arr, size_t len, CompareFunc cmp, double value);
bool     arr_f_all(const float *arr, size_t len, CompareFunc cmp, double value);
bool     arr_f_anyNaN(const float *arr, size_t len);
bool     arr_f_compare(const float *a, const float *b, CompareFunc cmp, bool *out, size_t len);
void     arr_f_clamp(const float *in, float *out, size_t len, float lo, float hi);
void     arr_f_addScalar(const float *in, float *out, size_t len, float value);
void     arr_f_mulScalar(const float *in, float *out, size_t len, float value);
void     arr_f_add(const float *a, const float *b, float *out, size_t len);
void     arr_f_sub(const float *a, const float *b, float *out, size_t len);
void     arr_f_mul(const float *a, const float *b, float *out, size_t len);
void     arr_f_div(const float *a, const float *b, float *out, size_t len);
void     arr_f_abs(const float *in, float *out, size_t len);
void     arr_f_map(const float *in, float *out, size_t len, float (*func)(float));    // e.g. sqrtf
void     arr_f_roundDecimals(const float *in, float *out, size_t len, uint8_t decimals);

// ---- uint8_t ----
void     arr_u8_fill(uint8_t *arr, size_t len, uint8_t value);
bool     arr_u8_equal(const uint8_t *a, const uint8_t *b, size_t len);
bool     arr_u8_min(const uint8_t *arr, size_t len, uint8_t *min, size_t *idx);
bool     arr_u8_max(const uint8_t *arr, size_t len, uint8_t *max, size_t *idx);
uint64_t arr_u8_sum(const uint8_t *arr, size_t len);
double   arr_u8_avg(const uint8_t *arr, size_t len);
size_t   arr_u8_find(const uint8_t *arr, size_t len, CompareFunc cmp, double value);
size_t   arr_u8_count(const uint8_t *arr, size_t len, CompareFunc cmp, double value);
bool     arr_u8_any(const uint8_t *arr, size_t len, CompareFunc cmp, double value);
bool     arr_u8_all(const uint8_t *arr, size_t len, CompareFunc cmp, double value);
bool     arr_u8_compare(const uint8_t *a, const uint8_t *b, CompareFunc cmp, bool *out, size_t len);
void     arr_u8_clamp(const uint8_t *in, uint8_t *out, size_t len, uint8_t lo, uint8_t hi);
void     arr_u8_addScalar(const uint8_t *in, uint8_t *out, size_t len, double value);
void     arr_u8_mulScalar(const uint8_t *in, uint8_t *out, size_t len, double value);
void     arr_u8_add(const uint8_t *a, const uint8_t *b, uint8_t *out, size_t len);
void     arr_u8_sub(const uint8_t *a, const uint8_t *b, uint8_t *out, size_t len);
void     arr_u8_mul(const uint8_t *a, const uint8_t *b, uint8_t *out, size_t len);
void     arr_u8_and(const uint8_t *a, const uint8_t *b, uint8_t *out, size_t len);
void     arr_u8_or (const uint8_t *a, const uint8_t *b, uint8_t *out, size_t len);
void     arr_u8_xor(const uint8_t *a, const uint8_t *b, uint8_t *out, size_t len);
void     arr_u8_not(const uint8_t *in, uint8_t *out, size_t len);

// ---- uint16_t ----
void     arr_u16_fill(uint16_t *arr, size_t len, uint16_t value);
bool     arr_u16_equal(const uint16_t *a, const uint16_t *b, size_t len);
bool     arr_u16_min(const uint16_t *arr, size_t len, uint16_t *min, size_t *idx);
bool     arr_u16_max(const uint16_t *arr, size_t len, uint16_t *max, size_t *idx);
uint64_t arr_u16_sum(const uint16_t *arr, size_t len);
double   arr_u16_avg(const uint16_t *arr, size_t len);
size_t   arr_u16_find(const uint16_t *arr, size_t len, CompareFunc cmp, double value);
size_t   arr_u16_count(const uint16_t *arr, size_t len, CompareFunc cmp, double value);
bool     arr_u16_any(const uint16_t *arr, size_t len, CompareFunc cmp, double value);
bool     arr_u16_all(const uint16_t *arr, size_t len, CompareFunc cmp, double value);
bool     arr_u16_compare(const uint16_t *a, const uint16_t *b, CompareFunc cmp, bool *out, size_t len);
void     arr_u16_clamp(const uint16_t *in, uint16_t *out, size_t len, uint16_t lo, uint16_t hi);
void     arr_u16_addScalar(const uint16_t *in, uint16_t *out, size_t len, double value);
void     arr_u16_mulScalar(const uint16_t *in, uint16_t *out, size_t len, double value);
void     arr_u16_add(const uint16_t *a, const uint16_t *b, uint16_t *out, size_t len);
void     arr_u16_sub(const uint16_t *a, const uint16_t *b, uint16_t *out, size_t len);
void     arr_u16_mul(const uint16_t *a, const uint16_t *b, uint16_t *out, size_t len);
void     arr_u16_and(const uint16_t *a, const uint16_t *b, uint16_t *out, size_t len);
void     arr_u16_or (const uint16_t *a, const uint16_t *b, uint16_t *out, size_t len);
void     arr_u16_xor(const uint16_t *a, const uint16_t *b, uint16_t *out, size_t len);
void     arr_u16_not(const uint16_t *in, uint16_t *out, size_t len);

// ---- uint32_t ----
void     arr_u32_fill(uint32_t *arr, size_t len, uint32_t value);
bool     arr_u32_equal(const uint32_t *a, const uint32_t *b, size_t len);
bool     arr_u32_min(const uint32_t *arr, size_t len, uint32_t *min, size_t *idx);
bool     arr_u32_max(const uint32_t *arr, size_t len, uint32_t *max, size_t *idx);
uint64_t arr_u32_sum(const uint32_t *arr, size_t len);
double   arr_u32_avg(const uint32_t *arr, size_t len);
size_t   arr_u32_find(const uint32_t *arr, size_t len, CompareFunc cmp, double value);
size_t   arr_u32_count(const uint32_t *arr, size_t len, CompareFunc cmp, double value);
bool     arr_u32_any(const uint32_t *arr, size_t len, CompareFunc cmp, double value);
bool     arr_u32_all(const uint32_t *arr, size_t len, CompareFunc cmp, double value);
bool     arr_u32_compare(const uint32_t *a, const uint32_t *b, CompareFunc cmp, bool *out, size_t len);
void     arr_u32_clamp(const uint32_t *in, uint32_t *out, size_t len, uint32_t lo, uint32_t hi);
void     arr_u32_addScalar(const uint32_t *in, uint32_t *out, size_t len, double value);
void     arr_u32_mulScalar(const uint32_t *in, uint32_t *out, size_t len, double value);
void     arr_u32_add(const uint32_t *a, const uint32_t *b, uint32_t *out, size_t len);
void     arr_u32_sub(const uint32_t *a, const uint32_t *b, uint32_t *out, size_t len);
void     arr_u32_mul(const uint32_t *a, const uint32_t *b, uint32_t *out, size_t len);
void     arr_u32_and(const uint32_t *a, const uint32_t *b, uint32_t *out, size_t len);
void     arr_u32_or (const uint32_t *a, const uint32_t *b, uint32_t *out, size_t len);
void     arr_u32_xor(const uint32_t *a, const uint32_t *b, uint32_t *out, size_t len);
void     arr_u32_not(const uint32_t *in, uint32_t *out, size_t len);

// ---- int32_t ----
void     arr_i32_fill(int32_t *arr, size_t len, int32_t value);
bool     arr_i32_equal(const int32_t *a, const int32_t *b, size_t len);
bool     arr_i32_min(const int32_t *arr, size_t len, int32_t *min, size_t *idx);
bool     arr_i32_max(const int32_t *arr, size_t len, int32_t *max, size_t *idx);
int64_t  arr_i32_sum(const int32_t *arr, size_t len);
double   arr_i32_avg(const int32_t *arr, size_t len);
size_t   arr_i32_find(const int32_t *arr, size_t len, CompareFunc cmp, double value);
size_t   arr_i32_count(const int32_t *arr, size_t len, CompareFunc cmp, double value);
bool     arr_i32_any(const int32_t *arr, size_t len, CompareFunc cmp, double value);
bool     arr_i32_all(const int32_t *arr, size_t len, CompareFunc cmp, double value);
bool     arr_i32_compare(const int32_t *a, const int32_t *b, CompareFunc cmp, bool *out, size_t len);
void     arr_i32_clamp(const int32_t *in, int32_t *out, size_t len, int32_t lo, int32_t hi);
void     arr_i32_addScalar(const int32_t *in, int32_t *out, size_t len, double value);
void     arr_i32_mulScalar(const int32_t *in, int32_t *out, size_t len, double value);
void     arr_i32_add(const int32_t *a, const int32_t *b, int32_t *out, size_t len);
void     arr_i32_sub(const int32_t *a, const int32_t *b, int32_t *out, size_t len);
void     arr_i32_mul(const int32_t *a, const int32_t *b, int32_t *out, size_t len);
void     arr_i32_abs(const int32_t *in, int32_t *out, size_t len);

// ---- bool ----
void     arr_bool_fill(bool *arr, size_t len, bool value);
size_t   arr_bool_countTrue(const bool *arr, size_t len);
bool     arr_bool_all(const bool *arr, size_t len);
bool     arr_bool_any(const bool *arr, size_t len);
void     arr_bool_and(const bool *a, const bool *b, bool *out, size_t len);
void     arr_bool_or (const bool *a, const bool *b, bool *out, size_t len);
void     arr_bool_xor(const bool *a, const bool *b, bool *out, size_t len);
void     arr_bool_not(const bool *in, bool *out, size_t len);

// ---- float <-> integer. float to integer rounds to nearest, saturates, NaN becomes 0 ----
void arr_f_to_u8 (const float *in, uint8_t  *out, size_t len);
void arr_f_to_u16(const float *in, uint16_t *out, size_t len);
void arr_f_to_u32(const float *in, uint32_t *out, size_t len);
void arr_f_to_i8 (const float *in, int8_t   *out, size_t len);
void arr_f_to_i16(const float *in, int16_t  *out, size_t len);
void arr_f_to_i32(const float *in, int32_t  *out, size_t len);

void arr_u8_to_f (const uint8_t  *in, float *out, size_t len);
void arr_u16_to_f(const uint16_t *in, float *out, size_t len);
void arr_u32_to_f(const uint32_t *in, float *out, size_t len);
void arr_i8_to_f (const int8_t   *in, float *out, size_t len);
void arr_i16_to_f(const int16_t  *in, float *out, size_t len);
void arr_i32_to_f(const int32_t  *in, float *out, size_t len);

// ---------------------------------------------------------------------------------------
// Bits and integer math
// ---------------------------------------------------------------------------------------
bool     bitRead32(uint32_t num, uint8_t bit);
uint32_t bitWrite32(uint32_t num, uint8_t bit, bool value, bool *prevValue);    // prevValue may be NULL
uint32_t bitToggle32(uint32_t num, uint8_t bit, bool *newValue);                // newValue may be NULL
uint32_t lowBitsMask32(uint8_t bits);
uint8_t  bitCount32(uint32_t num);
bool     isPowerOfTwo32(uint32_t num);
uint32_t divCeil_u32(uint32_t num, uint32_t den);
uint32_t bitsToBytes(uint32_t bits);

// round to the nearest multiple of 2^bits, saturates instead of wrapping
uint16_t roundToBits16(uint16_t input, uint8_t bits);
uint32_t roundToBits32(uint32_t input, uint8_t bits);

// ---------------------------------------------------------------------------------------
// Endianness. The _BE and _LE functions give the same result on any processor.
// ---------------------------------------------------------------------------------------
bool     isBigEndianHost(void);

uint16_t swapBytes16(uint16_t value);
uint32_t swapBytes32(uint32_t value);
void     swapBytes16_arr(const uint16_t *in, uint16_t *out, size_t len);
void     swapBytes32_arr(const uint32_t *in, uint32_t *out, size_t len);

// swap only if the processor is not already that endianness (they also work the other way around)
uint16_t hostToBE16(uint16_t value);
uint16_t hostToLE16(uint16_t value);
uint32_t hostToBE32(uint32_t value);
uint32_t hostToLE32(uint32_t value);

void     word2bytes(uint16_t word, uint8_t *byteLow, uint8_t *byteHigh);
uint16_t bytes2word_BE(uint8_t first, uint8_t second);      // first = byte that comes first in memory
uint16_t bytes2word_LE(uint8_t first, uint8_t second);

// one value from / to a byte buffer
uint16_t read_u16_BE(const uint8_t *p);
uint16_t read_u16_LE(const uint8_t *p);
uint32_t read_u32_BE(const uint8_t *p);
uint32_t read_u32_LE(const uint8_t *p);
float    read_f_BE(const uint8_t *p);
float    read_f_LE(const uint8_t *p);

void     write_u16_BE(uint8_t *p, uint16_t value);
void     write_u16_LE(uint8_t *p, uint16_t value);
void     write_u32_BE(uint8_t *p, uint32_t value);
void     write_u32_LE(uint8_t *p, uint32_t value);
void     write_f_BE(uint8_t *p, float value);
void     write_f_LE(uint8_t *p, float value);

// whole arrays. count = number of words/floats, not bytes
void bytes_to_u16_BE(const uint8_t *bytes, uint16_t *out, size_t count);
void bytes_to_u16_LE(const uint8_t *bytes, uint16_t *out, size_t count);
void bytes_to_u32_BE(const uint8_t *bytes, uint32_t *out, size_t count);
void bytes_to_u32_LE(const uint8_t *bytes, uint32_t *out, size_t count);
void bytes_to_f_BE  (const uint8_t *bytes, float    *out, size_t count);
void bytes_to_f_LE  (const uint8_t *bytes, float    *out, size_t count);

void u16_to_bytes_BE(const uint16_t *in, uint8_t *bytes, size_t count);
void u16_to_bytes_LE(const uint16_t *in, uint8_t *bytes, size_t count);
void u32_to_bytes_BE(const uint32_t *in, uint8_t *bytes, size_t count);
void u32_to_bytes_LE(const uint32_t *in, uint8_t *bytes, size_t count);
void f_to_bytes_BE  (const float    *in, uint8_t *bytes, size_t count);
void f_to_bytes_LE  (const float    *in, uint8_t *bytes, size_t count);

void words_highBytes(const uint16_t *words, uint8_t *bytes, size_t count);
void words_lowBytes (const uint16_t *words, uint8_t *bytes, size_t count);

// ---------------------------------------------------------------------------------------
// Pointers and raw memory (elemSize = sizeof one element)
// ---------------------------------------------------------------------------------------
bool ptrIsAligned(const void *p, size_t alignment);
bool ptrRangesOverlap(const void *a, size_t aSize, const void *b, size_t bSize);

void memSwap(void *a, void *b, size_t size);                    // does nothing if the ranges overlap
bool memIsFilled(const void *p, uint8_t value, size_t size);    // every byte equals value

void arr_swap(void *arr, size_t elemSize, size_t len, size_t i, size_t j);
void arr_reverse(void *arr, size_t elemSize, size_t len);
void arr_rotateLeft(void *arr, size_t elemSize, size_t len, size_t shift);

// ---------------------------------------------------------------------------------------
// Checksums
// ---------------------------------------------------------------------------------------
uint8_t  checksum_sum8(const uint8_t *data, size_t len);
uint8_t  crc8(const uint8_t *data, size_t len);                 // poly 0x07, init 0x00
uint16_t crc16_ccitt(const uint8_t *data, size_t len);          // poly 0x1021, init 0xFFFF

#endif /* PHANTOM_HELPERS_H */
