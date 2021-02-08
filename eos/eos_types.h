#ifndef JPL_EOS_TYPES
#define JPL_EOS_TYPES

#include <stdint.h>

#include "eos_types_pub.h"

typedef char CHAR;
typedef uint8_t U8;
typedef int8_t I8;
typedef uint16_t U16;
typedef int16_t I16;
typedef uint32_t U32;
typedef int32_t I32;
typedef uint64_t U64;
typedef int64_t I64;
typedef float F32;
typedef double F64;

typedef enum {
    EOS_LITTLE_ENDIAN,
    EOS_BIG_ENDIAN,
} EosEndianness;

typedef struct {
    void *ptr;
    U64 size;
} EosMemoryBuffer;

/* Relevant for E-THEMIS or MISE single-pixel detections */
typedef struct {
    EosPixelDetection* data;
    U32 capacity;
    U32 size;
} EosDetectionHeap;

#endif
