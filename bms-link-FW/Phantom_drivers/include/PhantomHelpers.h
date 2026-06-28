#ifndef HELPERS_H
#define HELPERS_H

#include <stdint.h>
#include <stdbool.h>




#define NIBBLE2BYTES 2
#define  WORDS2BYTES 2
#define DWORDS2BYTES 4
#define QWORDS2BYTES 8

//#define NAN_FLOAT    (0.0/0.0)
// -----------------------------------------
typedef enum {LittleEndian, BigEndian} Endianness;
typedef enum {  nibble  = 4 ,
                byte    = 8 ,
                word    = 16,
                dWord   = 32,
                qWord   = 64 } dataType;
// -----------------------------------------
#define MINUS1(Bits) ((1U<<(Bits))-1)
// -----------------------------------------

void delay_ms_us(const uint32_t ms, const uint32_t us);
boolean rtiTimerExpired(const uint32 id, const uint32 ms, const uint32 us);
uint32_t getNow_tick();
uint32_t getNow_us();
uint32_t getNow_ms();
uint32_t timer_tic_tick();
uint32_t timer_toc_us(const uint32_t tic);
// -----------------------------------------
void word2byte(const uint16_t word, uint8_t *byteLow, uint8_t *byteHigh);
uint16_t word2byte_BigEndian(const uint8_t byteLow, const uint8_t byteHigh);
uint16_t word2byte_LittleEndian(const uint8_t byte1, const uint8_t byte2);
// -----------------------------------------
void bytes2words_arr(const uint8_t *bytes, uint16_t *words, uint16_t NumberOfWords, Endianness Endian);
void words2bytes_arr(const uint16_t *words, uint8_t *bytes, uint16_t NumberOfWords, Endianness Endian);

uint16_t swap_word_bytes(const uint16_t input);
void     swap_word_bytes_arr(const uint16_t *input, uint16_t *output, uint16_t len);
// -----------------------------------------
uint16 round16(const uint16_t word, uint8_t bit2Round);

// -----------------------------------------
void array16_minAndIdx(const uint16_t* arr, uint8_t len, uint16_t* min, uint8_t* idx);
void array16_maxAndIdx(const uint16_t* arr, uint8_t len, uint16_t* max, uint8_t* idx);
uint16_t array16_min(const uint16_t* arr, uint8_t len);
uint16_t array16_max(const uint16_t* arr, uint8_t len);
uint32_t array16_sum(const uint16_t* arr, uint8_t len);
uint32_t array16_avg(const uint16_t* arr, uint8_t len);

bool array8_eq_all(const uint8_t* arr1, const uint8_t* arr2, uint8_t len);
bool array16_eq_all(const uint16_t* arr1, const uint16_t* arr2, uint8_t len);
bool array16_eq_element(const uint16_t* arr1, const uint16_t* arr2, bool *out, uint8_t len);
bool array16_less_element(const uint16_t* arr1, const uint16_t* arr2, bool *out, uint8_t len);
bool array16_greater_element(const uint16_t* arr1, const uint16_t* arr2, bool *out, uint8_t len);
bool array16_eq_any(const uint16_t* arr1, const uint16_t val, uint8_t len);
bool array16_less_any(const uint16_t* arr1, const uint16_t val, uint8_t len);
bool array16_greater_any(const uint16_t* arr1, const uint16_t val, uint8_t len);
bool array16_eq_every(const uint16_t* arr1, uint16_t val, uint8_t len);
bool array16_less_every(const uint16_t* arr1, uint16_t val, uint8_t len);
bool array16_greater_every(const uint16_t* arr1, uint16_t val, uint8_t len);

uint32 InvertAndInsertBit(uint32 Num, const uint8_t bit, bool * const new_val);
uint32 GetAndInsertBit(uint32 Num, const uint8_t bit, const bool New_val, bool *const last_val);


#endif /* HELPERS_H */
