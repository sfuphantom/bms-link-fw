#include <math.h>
#include "strings.h"
#include "PhantomHelpers.h"

// Integer array math is done in double, then rounded and limited to the type's range.
// NaN becomes 0. Used for the integer math and the float -> integer conversions.
static double roundClamp(const double value, const double lo, const double hi){
    if (isnan(value)) return 0.0;

    const double rounded = round(value);
    if (rounded < lo) return lo;
    if (rounded > hi) return hi;
    return rounded;
}

// =======================================================================================
// Single values
// =======================================================================================
bool isEqual(const double a, const double b)     { return a == b; }
bool isNotEqual(const double a, const double b)  { return a != b; }
bool isGreater(const double a, const double b)   { return a >  b; }
bool isGreaterEq(const double a, const double b) { return a >= b; }
bool isLess(const double a, const double b)      { return a <  b; }
bool isLessEq(const double a, const double b)    { return a <= b; }

bool isBetween_OO(const double value, const double lo, const double hi){ return value >  lo && value <  hi; }
bool isBetween_CC(const double value, const double lo, const double hi){ return value >= lo && value <= hi; }
bool isBetween_CO(const double value, const double lo, const double hi){ return value >= lo && value <  hi; }
bool isBetween_OC(const double value, const double lo, const double hi){ return value >  lo && value <= hi; }

bool isNotBetween_OO(const double value, const double lo, const double hi){ return !isBetween_OO(value, lo, hi); }
bool isNotBetween_CC(const double value, const double lo, const double hi){ return !isBetween_CC(value, lo, hi); }
bool isNotBetween_CO(const double value, const double lo, const double hi){ return !isBetween_CO(value, lo, hi); }
bool isNotBetween_OC(const double value, const double lo, const double hi){ return !isBetween_OC(value, lo, hi); }

double clampValue(const double value, const double lo, const double hi){
    const double low  = MIN_VAL(lo, hi);
    const double high = MAX_VAL(lo, hi);
    return MIN_VAL(MAX_VAL(value, low), high);
}

// =======================================================================================
// Float only
// =======================================================================================
bool isClose_f(const float a, const float b, const float tolerance){
    return fabsf(a - b) <= tolerance;
}

float lerp_f(const float a, const float b, const float t){
    return a + (b - a) * t;
}

float mapRange_f(const float x, const float inLo, const float inHi, const float outLo, const float outHi){
    if (inHi == inLo) return outLo;
    return outLo + (x - inLo) * (outHi - outLo) / (inHi - inLo);
}

float roundDecimals_f(const float value, const uint8_t decimals){
    const float scale = powf(10.0f, (float)decimals);
    return roundf(value * scale) / scale;
}

float tempCtoF(const float c){ return c * 9.0f / 5.0f + 32.0f; }
float tempFtoC(const float f){ return (f - 32.0f) * 5.0f / 9.0f; }
float tempCtoK(const float c){ return c + 273.15f; }
float tempKtoC(const float k){ return k - 273.15f; }

// =======================================================================================
// float arrays
// =======================================================================================
void arr_f_fill(float *arr, const size_t len, const float value){
    if (arr == NULL) return;
    size_t i;
    for (i = 0; i < len; i++) arr[i] = value;
}

bool arr_f_equal(const float *a, const float *b, const size_t len){
    if (a == NULL || b == NULL) return false;
    size_t i;
    for (i = 0; i < len; i++){
        if (a[i] != b[i]) return false;
    }
    return true;
}

bool arr_f_min(const float *arr, const size_t len, float *min, size_t *idx){
    if (arr == NULL || len == 0) return false;
    size_t best = 0;
    size_t i;
    for (i = 1; i < len; i++){
        if (arr[i] < arr[best]) best = i;
    }
    if (min != NULL) *min = arr[best];
    if (idx != NULL) *idx = best;
    return true;
}

bool arr_f_max(const float *arr, const size_t len, float *max, size_t *idx){
    if (arr == NULL || len == 0) return false;
    size_t best = 0;
    size_t i;
    for (i = 1; i < len; i++){
        if (arr[i] > arr[best]) best = i;
    }
    if (max != NULL) *max = arr[best];
    if (idx != NULL) *idx = best;
    return true;
}

double arr_f_sum(const float *arr, const size_t len){
    double sum = 0.0;
    if (arr == NULL) return sum;
    size_t i;
    for (i = 0; i < len; i++) sum += arr[i];
    return sum;
}

double arr_f_avg(const float *arr, const size_t len){
    if (arr == NULL || len == 0) return NAN;
    return arr_f_sum(arr, len) / (double)len;
}

// population variance
double arr_f_variance(const float *arr, const size_t len){
    const double mean = arr_f_avg(arr, len);
    if (isnan(mean)) return NAN;

    double acc = 0.0;
    size_t i;
    for (i = 0; i < len; i++){
        const double diff = arr[i] - mean;
        acc += diff * diff;
    }
    return acc / (double)len;
}

double arr_f_stddev(const float *arr, const size_t len){
    return sqrt(arr_f_variance(arr, len));
}

double arr_f_dot(const float *a, const float *b, const size_t len){
    if (a == NULL || b == NULL) return NAN;
    double acc = 0.0;
    size_t i;
    for (i = 0; i < len; i++) acc += (double)a[i] * b[i];
    return acc;
}

size_t arr_f_find(const float *arr, const size_t len, const CompareFunc cmp, const double value){
    if (arr == NULL || cmp == NULL) return NOT_FOUND;
    size_t i;
    for (i = 0; i < len; i++){
        if (cmp(arr[i], value)) return i;
    }
    return NOT_FOUND;
}

size_t arr_f_findClose(const float *arr, const size_t len, const float value, const float tolerance){
    if (arr == NULL) return NOT_FOUND;
    size_t i;
    for (i = 0; i < len; i++){
        if (isClose_f(arr[i], value, tolerance)) return i;
    }
    return NOT_FOUND;
}

size_t arr_f_count(const float *arr, const size_t len, const CompareFunc cmp, const double value){
    size_t count = 0;
    if (arr == NULL || cmp == NULL) return count;
    size_t i;
    for (i = 0; i < len; i++){
        if (cmp(arr[i], value)) count++;
    }
    return count;
}

bool arr_f_any(const float *arr, const size_t len, const CompareFunc cmp, const double value){
    return arr_f_find(arr, len, cmp, value) != NOT_FOUND;
}

bool arr_f_all(const float *arr, const size_t len, const CompareFunc cmp, const double value){
    if (arr == NULL || cmp == NULL) return false;
    return arr_f_count(arr, len, cmp, value) == len;
}

bool arr_f_anyNaN(const float *arr, const size_t len){
    if (arr == NULL) return false;
    size_t i;
    for (i = 0; i < len; i++){
        if (isnan(arr[i])) return true;
    }
    return false;
}

// out[i] = cmp(a[i], b[i]). Returns true if every result is true
bool arr_f_compare(const float *a, const float *b, const CompareFunc cmp, bool *out, const size_t len){
    if (a == NULL || b == NULL || cmp == NULL || out == NULL) return false;
    bool allTrue = true;
    size_t i;
    for (i = 0; i < len; i++){
        out[i] = cmp(a[i], b[i]);
        allTrue = allTrue && out[i];
    }
    return allTrue;
}

void arr_f_clamp(const float *in, float *out, const size_t len, const float lo, const float hi){
    if (in == NULL || out == NULL) return;
    size_t i;
    for (i = 0; i < len; i++) out[i] = (float)clampValue(in[i], lo, hi);
}

void arr_f_addScalar(const float *in, float *out, const size_t len, const float value){
    if (in == NULL || out == NULL) return;
    size_t i;
    for (i = 0; i < len; i++) out[i] = in[i] + value;
}

void arr_f_mulScalar(const float *in, float *out, const size_t len, const float value){
    if (in == NULL || out == NULL) return;
    size_t i;
    for (i = 0; i < len; i++) out[i] = in[i] * value;
}

void arr_f_add(const float *a, const float *b, float *out, const size_t len){
    if (a == NULL || b == NULL || out == NULL) return;
    size_t i;
    for (i = 0; i < len; i++) out[i] = a[i] + b[i];
}

void arr_f_sub(const float *a, const float *b, float *out, const size_t len){
    if (a == NULL || b == NULL || out == NULL) return;
    size_t i;
    for (i = 0; i < len; i++) out[i] = a[i] - b[i];
}

void arr_f_mul(const float *a, const float *b, float *out, const size_t len){
    if (a == NULL || b == NULL || out == NULL) return;
    size_t i;
    for (i = 0; i < len; i++) out[i] = a[i] * b[i];
}

void arr_f_div(const float *a, const float *b, float *out, const size_t len){
    if (a == NULL || b == NULL || out == NULL) return;
    size_t i;
    for (i = 0; i < len; i++) out[i] = a[i] / b[i];      // divide by 0 gives inf or NaN
}

void arr_f_abs(const float *in, float *out, const size_t len){
    if (in == NULL || out == NULL) return;
    size_t i;
    for (i = 0; i < len; i++) out[i] = fabsf(in[i]);
}

void arr_f_map(const float *in, float *out, const size_t len, float (*func)(float)){
    if (in == NULL || out == NULL || func == NULL) return;
    size_t i;
    for (i = 0; i < len; i++) out[i] = func(in[i]);
}

void arr_f_roundDecimals(const float *in, float *out, const size_t len, const uint8_t decimals){
    if (in == NULL || out == NULL) return;
    size_t i;
    for (i = 0; i < len; i++) out[i] = roundDecimals_f(in[i], decimals);
}

// =======================================================================================
// uint8_t arrays
// =======================================================================================
void arr_u8_fill(uint8_t *arr, const size_t len, const uint8_t value){
    if (arr == NULL) return;
    size_t i;
    for (i = 0; i < len; i++) arr[i] = value;
}

bool arr_u8_equal(const uint8_t *a, const uint8_t *b, const size_t len){
    if (a == NULL || b == NULL) return false;
    size_t i;
    for (i = 0; i < len; i++){
        if (a[i] != b[i]) return false;
    }
    return true;
}

bool arr_u8_min(const uint8_t *arr, const size_t len, uint8_t *min, size_t *idx){
    if (arr == NULL || len == 0) return false;
    size_t best = 0;
    size_t i;
    for (i = 1; i < len; i++){
        if (arr[i] < arr[best]) best = i;
    }
    if (min != NULL) *min = arr[best];
    if (idx != NULL) *idx = best;
    return true;
}

bool arr_u8_max(const uint8_t *arr, const size_t len, uint8_t *max, size_t *idx){
    if (arr == NULL || len == 0) return false;
    size_t best = 0;
    size_t i;
    for (i = 1; i < len; i++){
        if (arr[i] > arr[best]) best = i;
    }
    if (max != NULL) *max = arr[best];
    if (idx != NULL) *idx = best;
    return true;
}

uint64_t arr_u8_sum(const uint8_t *arr, const size_t len){
    uint64_t sum = 0;
    if (arr == NULL) return sum;
    size_t i;
    for (i = 0; i < len; i++) sum += arr[i];
    return sum;
}

double arr_u8_avg(const uint8_t *arr, const size_t len){
    if (arr == NULL || len == 0) return NAN;
    return (double)arr_u8_sum(arr, len) / (double)len;
}

size_t arr_u8_find(const uint8_t *arr, const size_t len, const CompareFunc cmp, const double value){
    if (arr == NULL || cmp == NULL) return NOT_FOUND;
    size_t i;
    for (i = 0; i < len; i++){
        if (cmp(arr[i], value)) return i;
    }
    return NOT_FOUND;
}

size_t arr_u8_count(const uint8_t *arr, const size_t len, const CompareFunc cmp, const double value){
    size_t count = 0;
    if (arr == NULL || cmp == NULL) return count;
    size_t i;
    for (i = 0; i < len; i++){
        if (cmp(arr[i], value)) count++;
    }
    return count;
}

bool arr_u8_any(const uint8_t *arr, const size_t len, const CompareFunc cmp, const double value){
    return arr_u8_find(arr, len, cmp, value) != NOT_FOUND;
}

bool arr_u8_all(const uint8_t *arr, const size_t len, const CompareFunc cmp, const double value){
    if (arr == NULL || cmp == NULL) return false;
    return arr_u8_count(arr, len, cmp, value) == len;
}

bool arr_u8_compare(const uint8_t *a, const uint8_t *b, const CompareFunc cmp, bool *out, const size_t len){
    if (a == NULL || b == NULL || cmp == NULL || out == NULL) return false;
    bool allTrue = true;
    size_t i;
    for (i = 0; i < len; i++){
        out[i] = cmp(a[i], b[i]);
        allTrue = allTrue && out[i];
    }
    return allTrue;
}

void arr_u8_clamp(const uint8_t *in, uint8_t *out, const size_t len, const uint8_t lo, const uint8_t hi){
    if (in == NULL || out == NULL) return;
    size_t i;
    for (i = 0; i < len; i++) out[i] = (uint8_t)clampValue(in[i], lo, hi);
}

void arr_u8_addScalar(const uint8_t *in, uint8_t *out, const size_t len, const double value){
    if (in == NULL || out == NULL) return;
    size_t i;
    for (i = 0; i < len; i++) out[i] = (uint8_t)roundClamp(in[i] + value, 0, UINT8_MAX);
}

void arr_u8_mulScalar(const uint8_t *in, uint8_t *out, const size_t len, const double value){
    if (in == NULL || out == NULL) return;
    size_t i;
    for (i = 0; i < len; i++) out[i] = (uint8_t)roundClamp(in[i] * value, 0, UINT8_MAX);
}

void arr_u8_add(const uint8_t *a, const uint8_t *b, uint8_t *out, const size_t len){
    if (a == NULL || b == NULL || out == NULL) return;
    size_t i;
    for (i = 0; i < len; i++) out[i] = (uint8_t)roundClamp((double)a[i] + b[i], 0, UINT8_MAX);
}

void arr_u8_sub(const uint8_t *a, const uint8_t *b, uint8_t *out, const size_t len){
    if (a == NULL || b == NULL || out == NULL) return;
    size_t i;
    for (i = 0; i < len; i++) out[i] = (uint8_t)roundClamp((double)a[i] - b[i], 0, UINT8_MAX);
}

void arr_u8_mul(const uint8_t *a, const uint8_t *b, uint8_t *out, const size_t len){
    if (a == NULL || b == NULL || out == NULL) return;
    size_t i;
    for (i = 0; i < len; i++) out[i] = (uint8_t)roundClamp((double)a[i] * b[i], 0, UINT8_MAX);
}

void arr_u8_and(const uint8_t *a, const uint8_t *b, uint8_t *out, const size_t len){
    if (a == NULL || b == NULL || out == NULL) return;
    size_t i;
    for (i = 0; i < len; i++) out[i] = (uint8_t)(a[i] & b[i]);
}

void arr_u8_or(const uint8_t *a, const uint8_t *b, uint8_t *out, const size_t len){
    if (a == NULL || b == NULL || out == NULL) return;
    size_t i;
    for (i = 0; i < len; i++) out[i] = (uint8_t)(a[i] | b[i]);
}

void arr_u8_xor(const uint8_t *a, const uint8_t *b, uint8_t *out, const size_t len){
    if (a == NULL || b == NULL || out == NULL) return;
    size_t i;
    for (i = 0; i < len; i++) out[i] = (uint8_t)(a[i] ^ b[i]);
}

void arr_u8_not(const uint8_t *in, uint8_t *out, const size_t len){
    if (in == NULL || out == NULL) return;
    size_t i;
    for (i = 0; i < len; i++) out[i] = (uint8_t)(~in[i]);
}

// =======================================================================================
// uint16_t arrays
// =======================================================================================
void arr_u16_fill(uint16_t *arr, const size_t len, const uint16_t value){
    if (arr == NULL) return;
    size_t i;
    for (i = 0; i < len; i++) arr[i] = value;
}

bool arr_u16_equal(const uint16_t *a, const uint16_t *b, const size_t len){
    if (a == NULL || b == NULL) return false;
    size_t i;
    for (i = 0; i < len; i++){
        if (a[i] != b[i]) return false;
    }
    return true;
}

bool arr_u16_min(const uint16_t *arr, const size_t len, uint16_t *min, size_t *idx){
    if (arr == NULL || len == 0) return false;
    size_t best = 0;
    size_t i;
    for (i = 1; i < len; i++){
        if (arr[i] < arr[best]) best = i;
    }
    if (min != NULL) *min = arr[best];
    if (idx != NULL) *idx = best;
    return true;
}

bool arr_u16_max(const uint16_t *arr, const size_t len, uint16_t *max, size_t *idx){
    if (arr == NULL || len == 0) return false;
    size_t best = 0;
    size_t i;
    for (i = 1; i < len; i++){
        if (arr[i] > arr[best]) best = i;
    }
    if (max != NULL) *max = arr[best];
    if (idx != NULL) *idx = best;
    return true;
}

uint64_t arr_u16_sum(const uint16_t *arr, const size_t len){
    uint64_t sum = 0;
    if (arr == NULL) return sum;
    size_t i;
    for (i = 0; i < len; i++) sum += arr[i];
    return sum;
}

double arr_u16_avg(const uint16_t *arr, const size_t len){
    if (arr == NULL || len == 0) return NAN;
    return (double)arr_u16_sum(arr, len) / (double)len;
}

size_t arr_u16_find(const uint16_t *arr, const size_t len, const CompareFunc cmp, const double value){
    if (arr == NULL || cmp == NULL) return NOT_FOUND;
    size_t i;
    for (i = 0; i < len; i++){
        if (cmp(arr[i], value)) return i;
    }
    return NOT_FOUND;
}

size_t arr_u16_count(const uint16_t *arr, const size_t len, const CompareFunc cmp, const double value){
    size_t count = 0;
    if (arr == NULL || cmp == NULL) return count;
    size_t i;
    for (i = 0; i < len; i++){
        if (cmp(arr[i], value)) count++;
    }
    return count;
}

bool arr_u16_any(const uint16_t *arr, const size_t len, const CompareFunc cmp, const double value){
    return arr_u16_find(arr, len, cmp, value) != NOT_FOUND;
}

bool arr_u16_all(const uint16_t *arr, const size_t len, const CompareFunc cmp, const double value){
    if (arr == NULL || cmp == NULL) return false;
    return arr_u16_count(arr, len, cmp, value) == len;
}

bool arr_u16_compare(const uint16_t *a, const uint16_t *b, const CompareFunc cmp, bool *out, const size_t len){
    if (a == NULL || b == NULL || cmp == NULL || out == NULL) return false;
    bool allTrue = true;
    size_t i;
    for (i = 0; i < len; i++){
        out[i] = cmp(a[i], b[i]);
        allTrue = allTrue && out[i];
    }
    return allTrue;
}

void arr_u16_clamp(const uint16_t *in, uint16_t *out, const size_t len, const uint16_t lo, const uint16_t hi){
    if (in == NULL || out == NULL) return;
    size_t i;
    for (i = 0; i < len; i++) out[i] = (uint16_t)clampValue(in[i], lo, hi);
}

void arr_u16_addScalar(const uint16_t *in, uint16_t *out, const size_t len, const double value){
    if (in == NULL || out == NULL) return;
    size_t i;
    for (i = 0; i < len; i++) out[i] = (uint16_t)roundClamp(in[i] + value, 0, UINT16_MAX);
}

void arr_u16_mulScalar(const uint16_t *in, uint16_t *out, const size_t len, const double value){
    if (in == NULL || out == NULL) return;
    size_t i;
    for (i = 0; i < len; i++) out[i] = (uint16_t)roundClamp(in[i] * value, 0, UINT16_MAX);
}

void arr_u16_add(const uint16_t *a, const uint16_t *b, uint16_t *out, const size_t len){
    if (a == NULL || b == NULL || out == NULL) return;
    size_t i;
    for (i = 0; i < len; i++) out[i] = (uint16_t)roundClamp((double)a[i] + b[i], 0, UINT16_MAX);
}

void arr_u16_sub(const uint16_t *a, const uint16_t *b, uint16_t *out, const size_t len){
    if (a == NULL || b == NULL || out == NULL) return;
    size_t i;
    for (i = 0; i < len; i++) out[i] = (uint16_t)roundClamp((double)a[i] - b[i], 0, UINT16_MAX);
}

void arr_u16_mul(const uint16_t *a, const uint16_t *b, uint16_t *out, const size_t len){
    if (a == NULL || b == NULL || out == NULL) return;
    size_t i;
    for (i = 0; i < len; i++) out[i] = (uint16_t)roundClamp((double)a[i] * b[i], 0, UINT16_MAX);
}

void arr_u16_and(const uint16_t *a, const uint16_t *b, uint16_t *out, const size_t len){
    if (a == NULL || b == NULL || out == NULL) return;
    size_t i;
    for (i = 0; i < len; i++) out[i] = (uint16_t)(a[i] & b[i]);
}

void arr_u16_or(const uint16_t *a, const uint16_t *b, uint16_t *out, const size_t len){
    if (a == NULL || b == NULL || out == NULL) return;
    size_t i;
    for (i = 0; i < len; i++) out[i] = (uint16_t)(a[i] | b[i]);
}

void arr_u16_xor(const uint16_t *a, const uint16_t *b, uint16_t *out, const size_t len){
    if (a == NULL || b == NULL || out == NULL) return;
    size_t i;
    for (i = 0; i < len; i++) out[i] = (uint16_t)(a[i] ^ b[i]);
}

void arr_u16_not(const uint16_t *in, uint16_t *out, const size_t len){
    if (in == NULL || out == NULL) return;
    size_t i;
    for (i = 0; i < len; i++) out[i] = (uint16_t)(~in[i]);
}

// =======================================================================================
// uint32_t arrays
// =======================================================================================
void arr_u32_fill(uint32_t *arr, const size_t len, const uint32_t value){
    if (arr == NULL) return;
    size_t i;
    for (i = 0; i < len; i++) arr[i] = value;
}

bool arr_u32_equal(const uint32_t *a, const uint32_t *b, const size_t len){
    if (a == NULL || b == NULL) return false;
    size_t i;
    for (i = 0; i < len; i++){
        if (a[i] != b[i]) return false;
    }
    return true;
}

bool arr_u32_min(const uint32_t *arr, const size_t len, uint32_t *min, size_t *idx){
    if (arr == NULL || len == 0) return false;
    size_t best = 0;
    size_t i;
    for (i = 1; i < len; i++){
        if (arr[i] < arr[best]) best = i;
    }
    if (min != NULL) *min = arr[best];
    if (idx != NULL) *idx = best;
    return true;
}

bool arr_u32_max(const uint32_t *arr, const size_t len, uint32_t *max, size_t *idx){
    if (arr == NULL || len == 0) return false;
    size_t best = 0;
    size_t i;
    for (i = 1; i < len; i++){
        if (arr[i] > arr[best]) best = i;
    }
    if (max != NULL) *max = arr[best];
    if (idx != NULL) *idx = best;
    return true;
}

uint64_t arr_u32_sum(const uint32_t *arr, const size_t len){
    uint64_t sum = 0;
    if (arr == NULL) return sum;
    size_t i;
    for (i = 0; i < len; i++) sum += arr[i];
    return sum;
}

double arr_u32_avg(const uint32_t *arr, const size_t len){
    if (arr == NULL || len == 0) return NAN;
    return (double)arr_u32_sum(arr, len) / (double)len;
}

size_t arr_u32_find(const uint32_t *arr, const size_t len, const CompareFunc cmp, const double value){
    if (arr == NULL || cmp == NULL) return NOT_FOUND;
    size_t i;
    for (i = 0; i < len; i++){
        if (cmp(arr[i], value)) return i;
    }
    return NOT_FOUND;
}

size_t arr_u32_count(const uint32_t *arr, const size_t len, const CompareFunc cmp, const double value){
    size_t count = 0;
    if (arr == NULL || cmp == NULL) return count;
    size_t i;
    for (i = 0; i < len; i++){
        if (cmp(arr[i], value)) count++;
    }
    return count;
}

bool arr_u32_any(const uint32_t *arr, const size_t len, const CompareFunc cmp, const double value){
    return arr_u32_find(arr, len, cmp, value) != NOT_FOUND;
}

bool arr_u32_all(const uint32_t *arr, const size_t len, const CompareFunc cmp, const double value){
    if (arr == NULL || cmp == NULL) return false;
    return arr_u32_count(arr, len, cmp, value) == len;
}

bool arr_u32_compare(const uint32_t *a, const uint32_t *b, const CompareFunc cmp, bool *out, const size_t len){
    if (a == NULL || b == NULL || cmp == NULL || out == NULL) return false;
    bool allTrue = true;
    size_t i;
    for (i = 0; i < len; i++){
        out[i] = cmp(a[i], b[i]);
        allTrue = allTrue && out[i];
    }
    return allTrue;
}

void arr_u32_clamp(const uint32_t *in, uint32_t *out, const size_t len, const uint32_t lo, const uint32_t hi){
    if (in == NULL || out == NULL) return;
    size_t i;
    for (i = 0; i < len; i++) out[i] = (uint32_t)clampValue(in[i], lo, hi);
}

void arr_u32_addScalar(const uint32_t *in, uint32_t *out, const size_t len, const double value){
    if (in == NULL || out == NULL) return;
    size_t i;
    for (i = 0; i < len; i++) out[i] = (uint32_t)roundClamp(in[i] + value, 0, UINT32_MAX);
}

void arr_u32_mulScalar(const uint32_t *in, uint32_t *out, const size_t len, const double value){
    if (in == NULL || out == NULL) return;
    size_t i;
    for (i = 0; i < len; i++) out[i] = (uint32_t)roundClamp(in[i] * value, 0, UINT32_MAX);
}

void arr_u32_add(const uint32_t *a, const uint32_t *b, uint32_t *out, const size_t len){
    if (a == NULL || b == NULL || out == NULL) return;
    size_t i;
    for (i = 0; i < len; i++) out[i] = (uint32_t)roundClamp((double)a[i] + b[i], 0, UINT32_MAX);
}

void arr_u32_sub(const uint32_t *a, const uint32_t *b, uint32_t *out, const size_t len){
    if (a == NULL || b == NULL || out == NULL) return;
    size_t i;
    for (i = 0; i < len; i++) out[i] = (uint32_t)roundClamp((double)a[i] - b[i], 0, UINT32_MAX);
}

void arr_u32_mul(const uint32_t *a, const uint32_t *b, uint32_t *out, const size_t len){
    if (a == NULL || b == NULL || out == NULL) return;
    size_t i;
    for (i = 0; i < len; i++) out[i] = (uint32_t)roundClamp((double)a[i] * b[i], 0, UINT32_MAX);
}

void arr_u32_and(const uint32_t *a, const uint32_t *b, uint32_t *out, const size_t len){
    if (a == NULL || b == NULL || out == NULL) return;
    size_t i;
    for (i = 0; i < len; i++) out[i] = a[i] & b[i];
}

void arr_u32_or(const uint32_t *a, const uint32_t *b, uint32_t *out, const size_t len){
    if (a == NULL || b == NULL || out == NULL) return;
    size_t i;
    for (i = 0; i < len; i++) out[i] = a[i] | b[i];
}

void arr_u32_xor(const uint32_t *a, const uint32_t *b, uint32_t *out, const size_t len){
    if (a == NULL || b == NULL || out == NULL) return;
    size_t i;
    for (i = 0; i < len; i++) out[i] = a[i] ^ b[i];
}

void arr_u32_not(const uint32_t *in, uint32_t *out, const size_t len){
    if (in == NULL || out == NULL) return;
    size_t i;
    for (i = 0; i < len; i++) out[i] = ~in[i];
}

// =======================================================================================
// int32_t arrays
// =======================================================================================
void arr_i32_fill(int32_t *arr, const size_t len, const int32_t value){
    if (arr == NULL) return;
    size_t i;
    for (i = 0; i < len; i++) arr[i] = value;
}

bool arr_i32_equal(const int32_t *a, const int32_t *b, const size_t len){
    if (a == NULL || b == NULL) return false;
    size_t i;
    for (i = 0; i < len; i++){
        if (a[i] != b[i]) return false;
    }
    return true;
}

bool arr_i32_min(const int32_t *arr, const size_t len, int32_t *min, size_t *idx){
    if (arr == NULL || len == 0) return false;
    size_t best = 0;
    size_t i;
    for (i = 1; i < len; i++){
        if (arr[i] < arr[best]) best = i;
    }
    if (min != NULL) *min = arr[best];
    if (idx != NULL) *idx = best;
    return true;
}

bool arr_i32_max(const int32_t *arr, const size_t len, int32_t *max, size_t *idx){
    if (arr == NULL || len == 0) return false;
    size_t best = 0;
    size_t i;
    for (i = 1; i < len; i++){
        if (arr[i] > arr[best]) best = i;
    }
    if (max != NULL) *max = arr[best];
    if (idx != NULL) *idx = best;
    return true;
}

int64_t arr_i32_sum(const int32_t *arr, const size_t len){
    int64_t sum = 0;
    if (arr == NULL) return sum;
    size_t i;
    for (i = 0; i < len; i++) sum += arr[i];
    return sum;
}

double arr_i32_avg(const int32_t *arr, const size_t len){
    if (arr == NULL || len == 0) return NAN;
    return (double)arr_i32_sum(arr, len) / (double)len;
}

size_t arr_i32_find(const int32_t *arr, const size_t len, const CompareFunc cmp, const double value){
    if (arr == NULL || cmp == NULL) return NOT_FOUND;
    size_t i;
    for (i = 0; i < len; i++){
        if (cmp(arr[i], value)) return i;
    }
    return NOT_FOUND;
}

size_t arr_i32_count(const int32_t *arr, const size_t len, const CompareFunc cmp, const double value){
    size_t count = 0;
    if (arr == NULL || cmp == NULL) return count;
    size_t i;
    for (i = 0; i < len; i++){
        if (cmp(arr[i], value)) count++;
    }
    return count;
}

bool arr_i32_any(const int32_t *arr, const size_t len, const CompareFunc cmp, const double value){
    return arr_i32_find(arr, len, cmp, value) != NOT_FOUND;
}

bool arr_i32_all(const int32_t *arr, const size_t len, const CompareFunc cmp, const double value){
    if (arr == NULL || cmp == NULL) return false;
    return arr_i32_count(arr, len, cmp, value) == len;
}

bool arr_i32_compare(const int32_t *a, const int32_t *b, const CompareFunc cmp, bool *out, const size_t len){
    if (a == NULL || b == NULL || cmp == NULL || out == NULL) return false;
    bool allTrue = true;
    size_t i;
    for (i = 0; i < len; i++){
        out[i] = cmp(a[i], b[i]);
        allTrue = allTrue && out[i];
    }
    return allTrue;
}

void arr_i32_clamp(const int32_t *in, int32_t *out, const size_t len, const int32_t lo, const int32_t hi){
    if (in == NULL || out == NULL) return;
    size_t i;
    for (i = 0; i < len; i++) out[i] = (int32_t)clampValue(in[i], lo, hi);
}

void arr_i32_addScalar(const int32_t *in, int32_t *out, const size_t len, const double value){
    if (in == NULL || out == NULL) return;
    size_t i;
    for (i = 0; i < len; i++) out[i] = (int32_t)roundClamp(in[i] + value, INT32_MIN, INT32_MAX);
}

void arr_i32_mulScalar(const int32_t *in, int32_t *out, const size_t len, const double value){
    if (in == NULL || out == NULL) return;
    size_t i;
    for (i = 0; i < len; i++) out[i] = (int32_t)roundClamp(in[i] * value, INT32_MIN, INT32_MAX);
}

void arr_i32_add(const int32_t *a, const int32_t *b, int32_t *out, const size_t len){
    if (a == NULL || b == NULL || out == NULL) return;
    size_t i;
    for (i = 0; i < len; i++) out[i] = (int32_t)roundClamp((double)a[i] + b[i], INT32_MIN, INT32_MAX);
}

void arr_i32_sub(const int32_t *a, const int32_t *b, int32_t *out, const size_t len){
    if (a == NULL || b == NULL || out == NULL) return;
    size_t i;
    for (i = 0; i < len; i++) out[i] = (int32_t)roundClamp((double)a[i] - b[i], INT32_MIN, INT32_MAX);
}

void arr_i32_mul(const int32_t *a, const int32_t *b, int32_t *out, const size_t len){
    if (a == NULL || b == NULL || out == NULL) return;
    size_t i;
    for (i = 0; i < len; i++) out[i] = (int32_t)roundClamp((double)a[i] * b[i], INT32_MIN, INT32_MAX);
}

// INT32_MIN becomes INT32_MAX
void arr_i32_abs(const int32_t *in, int32_t *out, const size_t len){
    if (in == NULL || out == NULL) return;
    size_t i;
    for (i = 0; i < len; i++) out[i] = (int32_t)roundClamp(fabs((double)in[i]), 0, INT32_MAX);
}

// =======================================================================================
// bool arrays
// =======================================================================================
void arr_bool_fill(bool *arr, const size_t len, const bool value){
    if (arr == NULL) return;
    size_t i;
    for (i = 0; i < len; i++) arr[i] = value;
}

size_t arr_bool_countTrue(const bool *arr, const size_t len){
    size_t count = 0;
    if (arr == NULL) return count;
    size_t i;
    for (i = 0; i < len; i++){
        if (arr[i]) count++;
    }
    return count;
}

bool arr_bool_all(const bool *arr, const size_t len){
    if (arr == NULL) return false;
    return arr_bool_countTrue(arr, len) == len;
}

bool arr_bool_any(const bool *arr, const size_t len){
    return arr_bool_countTrue(arr, len) > 0;
}

void arr_bool_and(const bool *a, const bool *b, bool *out, const size_t len){
    if (a == NULL || b == NULL || out == NULL) return;
    size_t i;
    for (i = 0; i < len; i++) out[i] = a[i] && b[i];
}

void arr_bool_or(const bool *a, const bool *b, bool *out, const size_t len){
    if (a == NULL || b == NULL || out == NULL) return;
    size_t i;
    for (i = 0; i < len; i++) out[i] = a[i] || b[i];
}

void arr_bool_xor(const bool *a, const bool *b, bool *out, const size_t len){
    if (a == NULL || b == NULL || out == NULL) return;
    size_t i;
    for (i = 0; i < len; i++) out[i] = (a[i] != b[i]);
}

void arr_bool_not(const bool *in, bool *out, const size_t len){
    if (in == NULL || out == NULL) return;
    size_t i;
    for (i = 0; i < len; i++) out[i] = !in[i];
}

// =======================================================================================
// float <-> integer arrays
// =======================================================================================
void arr_f_to_u8(const float *in, uint8_t *out, const size_t len){
    if (in == NULL || out == NULL) return;
    size_t i;
    for (i = 0; i < len; i++) out[i] = (uint8_t)roundClamp(in[i], 0, UINT8_MAX);
}

void arr_f_to_u16(const float *in, uint16_t *out, const size_t len){
    if (in == NULL || out == NULL) return;
    size_t i;
    for (i = 0; i < len; i++) out[i] = (uint16_t)roundClamp(in[i], 0, UINT16_MAX);
}

void arr_f_to_u32(const float *in, uint32_t *out, const size_t len){
    if (in == NULL || out == NULL) return;
    size_t i;
    for (i = 0; i < len; i++) out[i] = (uint32_t)roundClamp(in[i], 0, UINT32_MAX);
}

void arr_f_to_i8(const float *in, int8_t *out, const size_t len){
    if (in == NULL || out == NULL) return;
    size_t i;
    for (i = 0; i < len; i++) out[i] = (int8_t)roundClamp(in[i], INT8_MIN, INT8_MAX);
}

void arr_f_to_i16(const float *in, int16_t *out, const size_t len){
    if (in == NULL || out == NULL) return;
    size_t i;
    for (i = 0; i < len; i++) out[i] = (int16_t)roundClamp(in[i], INT16_MIN, INT16_MAX);
}

void arr_f_to_i32(const float *in, int32_t *out, const size_t len){
    if (in == NULL || out == NULL) return;
    size_t i;
    for (i = 0; i < len; i++) out[i] = (int32_t)roundClamp(in[i], INT32_MIN, INT32_MAX);
}

void arr_u8_to_f(const uint8_t *in, float *out, const size_t len){
    if (in == NULL || out == NULL) return;
    size_t i;
    for (i = 0; i < len; i++) out[i] = (float)in[i];
}

void arr_u16_to_f(const uint16_t *in, float *out, const size_t len){
    if (in == NULL || out == NULL) return;
    size_t i;
    for (i = 0; i < len; i++) out[i] = (float)in[i];
}

void arr_u32_to_f(const uint32_t *in, float *out, const size_t len){
    if (in == NULL || out == NULL) return;
    size_t i;
    for (i = 0; i < len; i++) out[i] = (float)in[i];
}

void arr_i8_to_f(const int8_t *in, float *out, const size_t len){
    if (in == NULL || out == NULL) return;
    size_t i;
    for (i = 0; i < len; i++) out[i] = (float)in[i];
}

void arr_i16_to_f(const int16_t *in, float *out, const size_t len){
    if (in == NULL || out == NULL) return;
    size_t i;
    for (i = 0; i < len; i++) out[i] = (float)in[i];
}

void arr_i32_to_f(const int32_t *in, float *out, const size_t len){
    if (in == NULL || out == NULL) return;
    size_t i;
    for (i = 0; i < len; i++) out[i] = (float)in[i];
}

// =======================================================================================
// Bits and integer math
// =======================================================================================
bool bitRead32(const uint32_t num, const uint8_t bit){
    if (bit >= 32) return false;
    return ((num >> bit) & 1U) != 0;
}

uint32_t bitWrite32(const uint32_t num, const uint8_t bit, const bool value, bool *prevValue){
    if (prevValue != NULL) *prevValue = bitRead32(num, bit);
    if (bit >= 32) return num;

    const uint32_t mask = (uint32_t)1 << bit;
    return value ? (num | mask) : (num & ~mask);
}

uint32_t bitToggle32(const uint32_t num, const uint8_t bit, bool *newValue){
    if (bit >= 32){
        if (newValue != NULL) *newValue = false;
        return num;
    }
    const uint32_t result = num ^ ((uint32_t)1 << bit);
    if (newValue != NULL) *newValue = bitRead32(result, bit);
    return result;
}

uint32_t lowBitsMask32(const uint8_t bits){
    if (bits >= 32) return UINT32_MAX;
    return ((uint32_t)1 << bits) - 1U;
}

uint8_t bitCount32(const uint32_t num){
    uint32_t rest = num;
    uint8_t count = 0;
    while (rest != 0){
        rest &= rest - 1U;      // clears the lowest set bit
        count++;
    }
    return count;
}

bool isPowerOfTwo32(const uint32_t num){
    return num != 0 && (num & (num - 1U)) == 0;
}

uint32_t divCeil_u32(const uint32_t num, const uint32_t den){
    if (den == 0) return 0;
    return num / den + ((num % den) != 0);
}

uint32_t bitsToBytes(const uint32_t bits){
    return divCeil_u32(bits, 8);
}

uint32_t roundToBits32(const uint32_t input, const uint8_t bits){
    if (bits == 0)  return input;
    if (bits >= 32) return 0;

    const uint32_t half   = (uint32_t)1 << (bits - 1);
    const uint32_t mask   = ~lowBitsMask32(bits);
    const uint32_t maxVal = UINT32_MAX & mask;      // biggest value that can be returned

    if (input > UINT32_MAX - half) return maxVal;
    return (input + half) & mask;
}

uint16_t roundToBits16(const uint16_t input, const uint8_t bits){
    const uint32_t rounded = roundToBits32(input, bits);
    const uint32_t maxVal  = UINT16_MAX & ~lowBitsMask32(bits);
    return (uint16_t)(rounded < maxVal ? rounded : maxVal);
}

// =======================================================================================
// Endianness
// =======================================================================================
typedef union { float value; uint32_t bits; } FloatBits;

bool isBigEndianHost(void){
    const union { uint16_t word; uint8_t bytes[2]; } probe = { 0x0102 };
    return probe.bytes[0] == 0x01;
}

uint16_t swapBytes16(const uint16_t value){
    return (uint16_t)((value >> 8) | (value << 8));
}

uint32_t swapBytes32(const uint32_t value){
    return ((uint32_t)swapBytes16((uint16_t)value) << 16) | swapBytes16((uint16_t)(value >> 16));
}

void swapBytes16_arr(const uint16_t *in, uint16_t *out, const size_t len){
    if (in == NULL || out == NULL) return;
    size_t i;
    for (i = 0; i < len; i++) out[i] = swapBytes16(in[i]);
}

void swapBytes32_arr(const uint32_t *in, uint32_t *out, const size_t len){
    if (in == NULL || out == NULL) return;
    size_t i;
    for (i = 0; i < len; i++) out[i] = swapBytes32(in[i]);
}

uint16_t hostToBE16(const uint16_t value){ return isBigEndianHost() ? value : swapBytes16(value); }
uint16_t hostToLE16(const uint16_t value){ return isBigEndianHost() ? swapBytes16(value) : value; }
uint32_t hostToBE32(const uint32_t value){ return isBigEndianHost() ? value : swapBytes32(value); }
uint32_t hostToLE32(const uint32_t value){ return isBigEndianHost() ? swapBytes32(value) : value; }

void word2bytes(const uint16_t word, uint8_t *byteLow, uint8_t *byteHigh){
    *byteLow  = (uint8_t)(word & 0xFF);
    *byteHigh = (uint8_t)((word >> 8) & 0xFF);
}

uint16_t bytes2word_BE(const uint8_t first, const uint8_t second){
    return (uint16_t)(((uint16_t)first << 8) | second);
}

uint16_t bytes2word_LE(const uint8_t first, const uint8_t second){
    return (uint16_t)(((uint16_t)second << 8) | first);
}

// ---- one value from / to a byte buffer ----
uint16_t read_u16_BE(const uint8_t *p){ return bytes2word_BE(p[0], p[1]); }
uint16_t read_u16_LE(const uint8_t *p){ return bytes2word_LE(p[0], p[1]); }

uint32_t read_u32_BE(const uint8_t *p){
    return ((uint32_t)read_u16_BE(p) << 16) | read_u16_BE(p + 2);
}

uint32_t read_u32_LE(const uint8_t *p){
    return ((uint32_t)read_u16_LE(p + 2) << 16) | read_u16_LE(p);
}

float read_f_BE(const uint8_t *p){
    FloatBits conv;
    conv.bits = read_u32_BE(p);
    return conv.value;
}

float read_f_LE(const uint8_t *p){
    FloatBits conv;
    conv.bits = read_u32_LE(p);
    return conv.value;
}

void write_u16_BE(uint8_t *p, const uint16_t value){ word2bytes(value, &p[1], &p[0]); }
void write_u16_LE(uint8_t *p, const uint16_t value){ word2bytes(value, &p[0], &p[1]); }

void write_u32_BE(uint8_t *p, const uint32_t value){
    write_u16_BE(p,     (uint16_t)(value >> 16));
    write_u16_BE(p + 2, (uint16_t)value);
}

void write_u32_LE(uint8_t *p, const uint32_t value){
    write_u16_LE(p,     (uint16_t)value);
    write_u16_LE(p + 2, (uint16_t)(value >> 16));
}

void write_f_BE(uint8_t *p, const float value){
    FloatBits conv;
    conv.value = value;
    write_u32_BE(p, conv.bits);
}

void write_f_LE(uint8_t *p, const float value){
    FloatBits conv;
    conv.value = value;
    write_u32_LE(p, conv.bits);
}

// ---- whole arrays ----
void bytes_to_u16_BE(const uint8_t *bytes, uint16_t *out, const size_t count){
    if (bytes == NULL || out == NULL) return;
    size_t i;
    for (i = 0; i < count; i++) out[i] = read_u16_BE(bytes + i * sizeof(uint16_t));
}

void bytes_to_u16_LE(const uint8_t *bytes, uint16_t *out, const size_t count){
    if (bytes == NULL || out == NULL) return;
    size_t i;
    for (i = 0; i < count; i++) out[i] = read_u16_LE(bytes + i * sizeof(uint16_t));
}

void bytes_to_u32_BE(const uint8_t *bytes, uint32_t *out, const size_t count){
    if (bytes == NULL || out == NULL) return;
    size_t i;
    for (i = 0; i < count; i++) out[i] = read_u32_BE(bytes + i * sizeof(uint32_t));
}

void bytes_to_u32_LE(const uint8_t *bytes, uint32_t *out, const size_t count){
    if (bytes == NULL || out == NULL) return;
    size_t i;
    for (i = 0; i < count; i++) out[i] = read_u32_LE(bytes + i * sizeof(uint32_t));
}

void bytes_to_f_BE(const uint8_t *bytes, float *out, const size_t count){
    if (bytes == NULL || out == NULL) return;
    size_t i;
    for (i = 0; i < count; i++) out[i] = read_f_BE(bytes + i * sizeof(float));
}

void bytes_to_f_LE(const uint8_t *bytes, float *out, const size_t count){
    if (bytes == NULL || out == NULL) return;
    size_t i;
    for (i = 0; i < count; i++) out[i] = read_f_LE(bytes + i * sizeof(float));
}

void u16_to_bytes_BE(const uint16_t *in, uint8_t *bytes, const size_t count){
    if (in == NULL || bytes == NULL) return;
    size_t i;
    for (i = 0; i < count; i++) write_u16_BE(bytes + i * sizeof(uint16_t), in[i]);
}

void u16_to_bytes_LE(const uint16_t *in, uint8_t *bytes, const size_t count){
    if (in == NULL || bytes == NULL) return;
    size_t i;
    for (i = 0; i < count; i++) write_u16_LE(bytes + i * sizeof(uint16_t), in[i]);
}

void u32_to_bytes_BE(const uint32_t *in, uint8_t *bytes, const size_t count){
    if (in == NULL || bytes == NULL) return;
    size_t i;
    for (i = 0; i < count; i++) write_u32_BE(bytes + i * sizeof(uint32_t), in[i]);
}

void u32_to_bytes_LE(const uint32_t *in, uint8_t *bytes, const size_t count){
    if (in == NULL || bytes == NULL) return;
    size_t i;
    for (i = 0; i < count; i++) write_u32_LE(bytes + i * sizeof(uint32_t), in[i]);
}

void f_to_bytes_BE(const float *in, uint8_t *bytes, const size_t count){
    if (in == NULL || bytes == NULL) return;
    size_t i;
    for (i = 0; i < count; i++) write_f_BE(bytes + i * sizeof(float), in[i]);
}

void f_to_bytes_LE(const float *in, uint8_t *bytes, const size_t count){
    if (in == NULL || bytes == NULL) return;
    size_t i;
    for (i = 0; i < count; i++) write_f_LE(bytes + i * sizeof(float), in[i]);
}

void words_highBytes(const uint16_t *words, uint8_t *bytes, const size_t count){
    if (words == NULL || bytes == NULL) return;
    size_t i;
    for (i = 0; i < count; i++) bytes[i] = (uint8_t)(words[i] >> 8);
}

void words_lowBytes(const uint16_t *words, uint8_t *bytes, const size_t count){
    if (words == NULL || bytes == NULL) return;
    size_t i;
    for (i = 0; i < count; i++) bytes[i] = (uint8_t)(words[i] & 0xFF);
}

// =======================================================================================
// Pointers and raw memory
// =======================================================================================
bool ptrIsAligned(const void *p, const size_t alignment){
    if (alignment == 0) return false;
    return ((uintptr_t)p % alignment) == 0;
}

bool ptrRangesOverlap(const void *a, const size_t aSize, const void *b, const size_t bSize){
    if (aSize == 0 || bSize == 0) return false;

    const uintptr_t aStart = (uintptr_t)a;
    const uintptr_t bStart = (uintptr_t)b;
    return (aStart < bStart + bSize) && (bStart < aStart + aSize);
}

void memSwap(void *a, void *b, const size_t size){
    if (a == NULL || b == NULL || ptrRangesOverlap(a, size, b, size)) return;

    uint8_t *byteA = a;
    uint8_t *byteB = b;
    size_t i;
    for (i = 0; i < size; i++){
        SWAP(uint8_t, byteA[i], byteB[i]);
    }
}

bool memIsFilled(const void *p, const uint8_t value, const size_t size){
    if (p == NULL) return false;

    const uint8_t *bytes = p;
    size_t i;
    for (i = 0; i < size; i++){
        if (bytes[i] != value) return false;
    }
    return true;
}

void arr_swap(void *arr, const size_t elemSize, const size_t len, const size_t i, const size_t j){
    if (arr == NULL || i >= len || j >= len) return;

    uint8_t *bytes = arr;
    memSwap(bytes + i * elemSize, bytes + j * elemSize, elemSize);
}

void arr_reverse(void *arr, const size_t elemSize, const size_t len){
    if (arr == NULL || len < 2) return;
    size_t i;
    for (i = 0; i < len / 2; i++){
        arr_swap(arr, elemSize, len, i, len - 1 - i);
    }
}

// three reverses: first part, the rest, then everything
void arr_rotateLeft(void *arr, const size_t elemSize, const size_t len, const size_t shift){
    if (arr == NULL || len < 2) return;

    const size_t k = shift % len;
    uint8_t *bytes = arr;
    arr_reverse(bytes,                elemSize, k);
    arr_reverse(bytes + k * elemSize, elemSize, len - k);
    arr_reverse(bytes,                elemSize, len);
}

// =======================================================================================
// Checksums
// =======================================================================================
uint8_t checksum_sum8(const uint8_t *data, const size_t len){
    return (uint8_t)arr_u8_sum(data, len);
}

uint8_t crc8(const uint8_t *data, const size_t len){
    uint8_t crc = 0x00;
    if (data == NULL) return crc;
    size_t i;
    for (i = 0; i < len; i++){
        crc ^= data[i];
        uint8_t bit;
        for (bit = 0; bit < 8; bit++){
            crc = (crc & 0x80) ? (uint8_t)((crc << 1) ^ 0x07) : (uint8_t)(crc << 1);
        }
    }
    return crc;
}

uint16_t crc16_ccitt(const uint8_t *data, const size_t len){
    uint16_t crc = 0xFFFF;
    if (data == NULL) return crc;
    size_t i;
    for (i = 0; i < len; i++){
        crc ^= (uint16_t)((uint16_t)data[i] << 8);
        size_t bit;
        for (bit = 0; bit < 8; bit++){
            crc = (crc & 0x8000) ? (uint16_t)((crc << 1) ^ 0x1021) : (uint16_t)(crc << 1);
        }
    }
    return crc;
}
