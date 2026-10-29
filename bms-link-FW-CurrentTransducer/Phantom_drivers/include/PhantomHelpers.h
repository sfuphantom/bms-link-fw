#ifndef HELPERS_H
#define HELPERS_H

#include <stdint.h>
#include <stdbool.h>


typedef enum {LittleEndian, BigEndian} Endianness;
typedef enum {  nibble  = 4 ,
                byte    = 8 ,
                word    = 16,
                dWord   = 32,
                qWord   = 64 } dataType;
// -----------------------------------------
#define MINUS1(Bits) ((1U<<(Bits))-1)

void delay_ms_us(uint32_t ms, uint32_t us);

void bytes2words(uint8_t *bytes, uint16_t *words, uint16_t NumberOfWords, Endianness Endian);
void words2bytes(uint16_t *words, uint8_t *bytes, uint16_t NumberOfWords, Endianness Endian);

void words2nibbles(uint16_t *words, uint8_t *nibbles, uint16_t NumberOfWords, Endianness Endian);
void nibbles2words(uint8_t *nibbles, uint16_t *words, uint16_t NumberOfWords, Endianness Endian);

uint16_t swap_word_bytes(uint16_t input);
void     swap_word_bytes_arr(uint16_t *input, uint16_t *output, uint16_t len);

uint16 round16(uint16_t word, uint8_t bit2Round);

void array16_minAndIdx(uint16_t* arr, uint8_t len, uint16_t* min, uint8_t* idx);
void array16_maxAndIdx(uint16_t* arr, uint8_t len, uint16_t* max, uint8_t* idx);
uint16 array16_avg(uint16_t* arr, uint8_t len);


#endif /* HELPERS_H */
