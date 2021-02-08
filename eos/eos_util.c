#include <float.h>
#include <math.h>

#include "eos_util.h"
#include "eos_log.h"

EosEndianness system_endianness(void)
{
    union {
        I32 i;
        I8 c[4];
    } bint = {0x01020304};

    return (bint.c[0] == 1) ? EOS_BIG_ENDIAN : EOS_LITTLE_ENDIAN;
}

void byte_swap_U16(U16 *bytes){
    U16 value;
    value = bytes[0];
    value = (value >> 8) | (value << 8);
    bytes[0] = value;
}

void byte_swap_U32(U32 *bytes) {
    U32 value;
    value = bytes[0];
    value = ((value>> 24) & 0x000000ff) |
            ((value<<  8) & 0x00ff0000) |
            ((value>>  8) & 0x0000ff00) |
            ((value<< 24) & 0xff000000);
    bytes[0] = value;
}

void correct_endianness_U16(EosEndianness current, EosEndianness desired,
                            U16 *bytes) {
    if (current != desired) {
        byte_swap_U16(bytes);
    }
}

void correct_endianness_U32(EosEndianness current, EosEndianness desired,
                            U32 *bytes) {
    if (current != desired) {
        byte_swap_U32(bytes);
    }
}

void* byte_offset(void* ptr, U64 nbytes) {
    return (void*)((U8*)ptr + nbytes);
}

const void* const_byte_offset(const void* ptr, U64 nbytes) {
    return (const void*)((const U8*)ptr + nbytes);
}

I32 eos_round(F32 value)
{
    /* it's ok if round does not comply with IEEE754 standard;
     the tests should allow +/-1 difference when the tested functions use round */
    return (I32)(value + (value >= 0 ? 0.5f : -0.5f));
}

I32 eos_ceil(F32 value)
{
    I32 i;
    F32 diff;
    i = eos_round(value);
    diff = (F32)(i - value);
    return i + (diff < 0);
}

I32 eos_floor(F32 value)
{
    I32 i;
    F32 diff;
    i = eos_round(value);
    diff = (F32)(value - i);
    return i - (diff < 0);
}

inline I32 eos_imax(I32 a, I32 b) {
    if (a > b) {
        return a;
    } else {
        return b;
    }
}

inline I32 eos_imin(I32 a, I32 b) {
    if (a < b) {
        return a;
    } else {
        return b;
    }
}

inline U32 eos_umin(U32 a, U32 b) {
    if (a < b) {
        return a;
    } else {
        return b;
    }
}

inline I64 eos_lmax(I64 a, I64 b) {
    if (a > b) {
        return a;
    } else {
        return b;
    }
}

inline I64 eos_lmin(I64 a, I64 b) {
    if (a < b) {
        return a;
    } else {
        return b;
    }
}

F64 eos_norm_sqeuclidean(U32 n, F64 *x) {
    U32 i;
    F64 sum = 0;
    for (i = 0; i < n; i++) {
        sum += x[i]*x[i];
    }
    return sum;
}

F64 eos_norm_infinity(U32 n, F64 *x) {
    U32 i;
    F64 max = 0;
    F64 val;
    for (i = 0; i < n; i++) {
        val = fabs(x[i]);
        if (val > max) {
            max = val;
        }
    }
    return max;
}

F64 eos_dsum(U32 n, F64 *x) {
    F64 sum = 0;
    U32 i;
    for (i = 0; i < n; i++) {
        sum += x[i];
    }
    return sum;
}
