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
void words2bytes(const uint16_t *words, uint8_t *bytes, uint16_t NumberOfWords, Endianness Endian){
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
void bytes2words(const uint8_t *bytes, uint16_t *words, uint16_t NumberOfWords, Endianness Endian){
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
void ExtendByte2Word(const uint8_t *bytes, uint16_t *words, uint16_t NumberOfWords, bool frontNback){

    const uint8_t shiftConst = frontNback ? 8:0;

    uint16_t theByte;
    int i;

    for (i = 0; i < NumberOfWords; i++){
            theByte     = bytes[i];
            words[i]    = theByte<<shiftConst;
    }
}
//----------------------------------------------------------------------------------------
uint16_t swap_word_bytes(const uint16_t input){
    uint16_t FirstByte = (input >> 8) & 0xFF;
    uint16_t LastByte  = (input << 8);
    uint16_t output = FirstByte | LastByte;
    return output;
}
 void swap_word_bytes_arr(const uint16_t *input, uint16_t *output, uint16_t len){
    uint16_t i;
    for (i = 0; i < len; i++)
        output[i] = swap_word_bytes(input[i]);
 }

//----------------------------------------------------------------------------------------
#define RTI_CLOCK_MEG_HZ    10U  // 10 MHz
#define RTI_CLOCK_HZ        (RTI_CLOCK_MEG_HZ*1000000UL)  // 10 MHz
#define RTI_US_2_TICKS      (RTI_CLOCK_MEG_HZ)
#define RTI_MS_2_TICKS      (RTI_US_2_TICKS * 1000UL)
#define RTI_SEC_2_TICKS     (RTI_MS_2_TICKS * 1000UL)


#define RTI_MAX_TIMERS 8

#define USE_RTI_DELAY TRUE
 void delay_ms_us(const uint32_t ms, const uint32_t us){
    #if USE_RTI_DELAY
        const uint64 total_us = (uint64)ms * 1000UL + us;
        const uint64 total_ticks = total_us * RTI_US_2_TICKS;

        /* Start counter block 0 if it is not already running */
        if ((rtiREG1->GCTRL & (1U << rtiCOUNTER_BLOCK0)) == 0U)
        {
            rtiStartCounter(rtiCOUNTER_BLOCK0);
        }

        const uint32 start = rtiREG1->CNT[rtiCOUNTER_BLOCK0].FRCx;
        /* Wait until the required number of ticks has elapsed */
//        rtiBASE_t * rtiRegDelay = rtiREG1;
        const uint32_t * timer = &rtiREG1->CNT[rtiCOUNTER_BLOCK0].FRCx;
        while ((rtiREG1->CNT[rtiCOUNTER_BLOCK0].FRCx - start) < (uint32)total_ticks)
        {
            /* Busy wait */
        }
        rtiStopCounter(rtiCOUNTER_BLOCK0);
    #else
        volatile uint32_t i, j;
        // Approximate loops per ms, tune by measurement.
        const uint32_t loops_per_us = 11;
        const uint32_t loops_per_ms = loops_per_us * 1000;

        for (i = 0; i < ms; i++) {
            for (j = 0; j < loops_per_ms; j++);
        }
        for (i = 0; i < us; i++) {
            for (j = 0; j < loops_per_us; j++);
        }
    #endif
}
/* Static storage for the start ticks of each software timer */
static uint32  rti_timer_last[RTI_MAX_TIMERS];
static boolean rti_timer_valid[RTI_MAX_TIMERS];

void rtiTimerStart(const uint8_t id)
{
    if (id >= RTI_MAX_TIMERS) return;

    /* Ensure counter block 0 is running */
    if ((rtiREG1->GCTRL & (1U << rtiCOUNTER_BLOCK0)) == 0U) {
        rtiStartCounter(rtiCOUNTER_BLOCK0);
    }

    rti_timer_last[id] = rtiREG1->CNT[rtiCOUNTER_BLOCK0].FRCx;
    rti_timer_valid[id] = true;
}

boolean rtiTimerExpired(const uint32 id, const uint32 ms, const uint32 us)
{
    if (id >= RTI_MAX_TIMERS) return false;

    /* First call: initialise the timer and return false */
    if (!rti_timer_valid[id]) {
        rtiTimerStart(id);
        return true;
    }

    /* Make sure counter block 0 is running */
    if ((rtiREG1->GCTRL & (1U << rtiCOUNTER_BLOCK0)) == 0U) {
        rtiStartCounter(rtiCOUNTER_BLOCK0);
    }

    uint32 now      = rtiREG1->CNT[rtiCOUNTER_BLOCK0].FRCx;
    uint32 last     = rti_timer_last[id];
    uint32 elapsed  = now - last;   /* wraps safely with unsigned arithmetic */

    /* Convert ms+us to RTI ticks */
    uint64 required_ticks = ((uint64)ms * 1000UL + us) * RTI_US_2_TICKS;

    if (elapsed >= required_ticks) {
        /* Reload the start time to the current moment (periodic behaviour) */
        rti_timer_last[id] = now;
        return true;
    }
    return false;
}

uint32_t timer_tic_tick(){
    const uint32_t now = rtiREG1->CNT[rtiCOUNTER_BLOCK0].FRCx;
    return now;
}
uint32_t timer_toc_us(const uint32_t tic){
    const uint32_t now = rtiREG1->CNT[rtiCOUNTER_BLOCK0].FRCx;
    uint32_t diff = (now-tic)/RTI_US_2_TICKS;
    return diff;
}
//----------------------------------------------------------------------------------------

uint16 round16(uint16_t input, uint8_t bit2Round){
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
