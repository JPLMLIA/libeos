#ifndef JPL_EOS_UTIL
#define JPL_EOS_UTIL

#include "eos_types.h"

#define PI 3.1415926535897932384626433832795
#define SQRT2 1.41421356237f
#define ATAN2_P1 57.283626556396484f
#define ATAN2_P3 -18.667446136474609f
#define ATAN2_P5 8.914000511169434f
#define ATAN2_P7 -2.539724588394165f

EosEndianness system_endianness(void);
void byte_swap_U16(U16 *bytes);
void byte_swap_U32(U32 *bytes);
void byte_swap_F32(F32 *bytes);
void correct_endianness_U16(EosEndianness current, EosEndianness desired,
                            U16 *bytes);
void correct_endianness_U32(EosEndianness current, EosEndianness desired,
                            U32 *bytes);
void correct_endianness_F32(EosEndianness current, EosEndianness desired,
                            F32 *bytes);

const void* const_byte_offset(const void* ptr, U64 nbytes);
void* byte_offset(void* ptr, U64 nbytes);

I32 eos_round(F32 value);
I32 eos_ceil(F32 value);
I32 eos_floor(F32 value);
I32 eos_imax(I32 a, I32 b);
I32 eos_imin(I32 a, I32 b);
I64 eos_lmax(I64 a, I64 b);
I64 eos_lmin(I64 a, I64 b);
F64 eos_norm_sqeuclidean(U32 n, F64 *x);
F64 eos_norm_infinity(U32 n, F64 *x);
F64 eos_dsum(U32 n, F64 *x);
U32 eos_umin(U32 a, U32 b);
U32 eos_umax(U32 a, U32 b);
U32 eos_uabs_diff(U32 a, U32 b);
F64 eos_hypot(F64 n, F64 x);

#endif
