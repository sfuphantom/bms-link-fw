#include <stdint.h>
#include <stdbool.h>
#include "rti.h"
#include "PhantomHelpers.h"

#include <math.h>



//----------------------------------------------------------------------------------------
//void splitArrayElements(void* arr_in, void* arr_out, uint16_t in_len, Endianness Endian, dataType in_dType, dataType out_dType){
//    const uint8_t TIMES_MORE_BITS = 2;
//    const uint8_t BITS2SHIFT = 16/TIMES_MORE_BITS;
//
//    int i, j;
//
//    for (i = 0; i < in_len; i++){
//        for(j=0; j<TIMES_MORE_BITS; j++){
//            uint8_t shiftIdx = (Endian == BigEndian) ? TIMES_MORE_BITS - j - 1 : j;
//            shiftIdx *= BITS2SHIFT;
//            arr_in[TIMES_MORE_BITS * i + j]     = (uint8_t)(arr_out[i] >> shiftIdx);
//        }
//    }
//}
//void combineArrayElements(uint8_t *bytes, uint16_t *words, uint16_t NumberOfWords, Endianness Endian){
//    const uint8_t TIMES_MORE_BITS = 2;
//    const uint8_t BITS2SHIFT = 16/TIMES_MORE_BITS;
//    int i, j;
//
//    for (i = 0; i < NumberOfWords; i++){
//        words[i] = 0;
//        for(j=0; j<TIMES_MORE_BITS; j++){
//            uint8_t shiftIdx = (Endian == BigEndian) ? TIMES_MORE_BITS - j - 1 : j;
//            shiftIdx *= BITS2SHIFT;
//            words[i] |= (uint16_t)(bytes[TIMES_MORE_BITS * i + j]<<shiftIdx);
//        }
//    }
//}

void word2byte(const uint16_t word, uint8_t *byteLow, uint8_t *byteHigh){
    *byteLow  = (word>>0U) & 0xFF;
    *byteHigh = (word>>8U) & 0xFF;
}

uint16_t bytes2word_BigEndian(const uint8_t byte1, const uint8_t byte2){
    uint16_t word = 0;
    word = byte1;
    word <<= 8;
    word |= byte2;
    return word;
}
uint16_t bytes2word_LittleEndian(const uint8_t byte1, const uint8_t byte2){
    uint16_t word = 0;
    word = byte2;
    word <<= 8;
    word |= byte1;

    return word;
}
//----------------------------------------------------------------------------------------
uint16_t swap_word_bytes(const uint16_t input){
    uint16_t FirstByte = (input >> 8) & 0xFF;
    uint16_t LastByte  = (input << 8);
    uint16_t output = FirstByte | LastByte;
    return output;
}

void ExtractByteFromWord(const uint16_t *words, uint8_t *bytes, uint16_t NumberOfWords, bool frontNback){

    const uint8_t bits2Extract = 8;
    const uint8_t mask = (1U<< bits2Extract)-1;
    const uint8_t shiftConst = frontNback ? 8:0;

    uint8_t theByte;
    int i;

    for (i = 0; i < NumberOfWords; i++){
            theByte     = (words[i] >> shiftConst) & mask;
            bytes[i]    = theByte;
    }
}


uint32 InvertAndInsertBit(uint32 Num, const uint8_t bit, bool * const new_val){
    const uint32_t bitMask = 1U << bit;

    *new_val = (0 == (Num & bitMask));

    Num ^= bitMask;
    return Num;
}
uint32 GetAndInsertBit(uint32 Num, const uint8_t bit, const bool New_val, bool *const last_val){
    const uint32_t bitMask = 1U << bit;

    *last_val = (0 != (Num & bitMask));

    if(New_val){
        Num |= bitMask;
    }
    else{
        Num &= ~bitMask;
    }
    return Num;
}

//----------------------------------------------------------------------------------------
 void swap_word_bytes_arr(const uint16_t *input, uint16_t *output, uint16_t len){
    uint16_t i;
    for (i = 0; i < len; i++)
        output[i] = swap_word_bytes(input[i]);
 }
 void words2bytes_arr(const uint16_t *words, uint8_t *bytes, uint16_t NumberOfWords, const Endianness Endian){
     const uint8_t TIMES_MORE_BITS = 2;
     int i;
     uint8_t High, Low;

     for (i = 0; i < NumberOfWords; i++){
         word2byte(words[i], &Low, &High);

         if(Endian == BigEndian){
             bytes[TIMES_MORE_BITS*i  ] = Low;
             bytes[TIMES_MORE_BITS*i+1] = High;
         }
         else{
             bytes[TIMES_MORE_BITS*i  ] = High;
             bytes[TIMES_MORE_BITS*i+1] = Low;
         }
     }
 }
 void bytes2words_arr(const uint8_t *bytes, uint16_t *words, uint16_t NumberOfWords, const Endianness Endian){
     const uint8_t TIMES_MORE_BITS = 2;
     int i;
     uint8_t High, Low;

     for (i = 0; i < NumberOfWords; i++){
         High = bytes[TIMES_MORE_BITS*i+1];
         Low  = bytes[TIMES_MORE_BITS*i  ];

         if(Endian == BigEndian){
             words[i] = bytes2word_BigEndian(Low, High);
         }
         else{
             words[i] = bytes2word_LittleEndian(Low, High);
         }
     }
 }


//----------------------------------------------------------------------------------------
//uint32_t divCeil_u32(uint32_t num,  uint32_t dem){
//    return (num-dem+1)/dem;
//}
//uint32_t Bit2Bytes_Ceil(uint32_t Bits){
//    return divCeil_u32(Bits,8);
//}

//----------------------------------------------------------------------------------------

uint16_t round16(uint16_t input, uint8_t bit2Round){
    if (bit2Round == 0) return input;

    uint16_t half = (uint16_t)(1 << (bit2Round - 1));
    uint16_t mask = (uint16_t)(0xFFFFU << bit2Round);
    return (uint16_t)((input + half) & mask);
}
//float roundFloat(float input, uint8_t Rounder){
//    if (Rounder == 0) return input;
//
//    const uint8_t MultiOffset = 10
//    roundf(input)
//}
//----------------------------------------------------------------------------------------

void array16_minAndIdx(const uint16_t* arr, uint8_t len, uint16_t* min, uint8_t* idx){
    int i;
    *min = arr[0];
    *idx = 0;
    for(i=1; i<len; i++){
        if(*min > arr[i]){
            *min = arr[i];
            *idx = i;
        }
    }
}
void array16_maxAndIdx(const uint16_t* arr, uint8_t len, uint16_t* max, uint8_t* idx){
    int i;
    *max = arr[0];
    *idx = 0;
    for(i=1; i<len; i++){
        if(*max < arr[i]){
            *max = arr[i];
            *idx = i;
        }
    }
}

uint16_t array16_min(const uint16_t* arr, uint8_t len){
    int i;
    uint16_t min = arr[0];

    for(i=1; i<len; i++){
        if(min > arr[i]){
            min = arr[i];
        }
    }
    return min;
}
uint16_t array16_max(const uint16_t* arr, uint8_t len){
    int i;
    uint16_t max = arr[0];

    for(i=1; i<len; i++){
        if(max < arr[i]){
            max = arr[i];
        }
    }
    return max;
}

uint32_t array16_sum(const uint16_t* arr, uint8_t len){
    int i;
    uint32_t sum = 0;

    for(i=0; i<len; i++)
        sum += arr[i];

    return sum;
}
uint32_t array16_avg(const uint16_t* arr, uint8_t len){

    uint32_t sum =  array16_sum(arr, len);
    uint32_t avg = sum / len;

    return avg;
}
bool array8_eq_all(const uint8_t* arr1, const uint8_t* arr2, uint8_t len){
    int i;
    for(i=0; i<len; i++){
        if(arr1[i] != arr2[i]){
            return FALSE;
        }
    }

    return TRUE;
}
bool array16_eq_all(const uint16_t* arr1, const uint16_t* arr2, uint8_t len){
    int i;
    for(i=0; i<len; i++){
        if(arr1[i] != arr2[i]){
            return FALSE;
        }
    }

    return TRUE;
}
bool array16_eq_element(const uint16_t* arr1, const uint16_t* arr2, bool *out, uint8_t len){
    int i;
    bool eqAll = TRUE;
    for(i=0; i<len; i++){
        out[i] = (arr1[i] == arr2[i]);
        eqAll &= out[i];
    }

    return eqAll;
}
bool array16_less_element(const uint16_t* arr1, const uint16_t* arr2, bool *out, uint8_t len){
    int i;
    bool lessAll = TRUE;
    for(i=0; i<len; i++){
        out[i] = (arr1[i] < arr2[i]);
        lessAll &= out[i];
    }

    return lessAll;
}
bool array16_greater_element(const uint16_t* arr1, const uint16_t* arr2, bool *out, uint8_t len){
    int i;
    bool greaterAll = TRUE;
    for(i=0; i<len; i++){
        out[i] = (arr1[i] > arr2[i]);
        greaterAll &= out[i];
    }

    return greaterAll;
}
bool array16_eq_any(const uint16_t* arr1, uint16_t val, uint8_t len){
    int i;
    for(i=0; i<len; i++){
        if(arr1[i] == val){
            return TRUE;
        }
    }

    return FALSE;
}
bool array16_eq_every(const uint16_t* arr1, uint16_t val, uint8_t len){
    int i;
    for(i=0; i<len; i++){
        if(arr1[i] != val){
            return FALSE;
        }
    }

    return TRUE;
}
bool array16_less_any(const uint16_t* arr1, uint16_t val, uint8_t len){
    int i;
    for(i=0; i<len; i++){
        if(arr1[i] < val){
            return TRUE;
        }
    }

    return FALSE;
}
bool array16_greater_any(const uint16_t* arr1, uint16_t val, uint8_t len){
    int i;
    for(i=0; i<len; i++){
        if(arr1[i] > val){
            return TRUE;
        }
    }

    return FALSE;
}
//----------------------------------------------------------------------------------------
void initArray8(uint8_t* arr, const uint8_t val, uint8_t len){
    int i;
    for(i=0;i<len;i++){
        arr[i] = val;
    }
}

