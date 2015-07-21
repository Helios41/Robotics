#ifndef DEFINITIONS_H_
#define DEFINITIONS_H_

#ifdef _WIN32
#define _USING_V110_SDK71_
#define inline _inline
#endif

#include <stdint.h>

typedef int8_t int8;
typedef int8_t s8;
typedef uint8_t uint8;
typedef uint8_t u8;

typedef int16_t int16;
typedef int16_t s16;
typedef uint16_t uint16;
typedef uint16_t u16;

typedef int32_t int32;
typedef int32_t s32;
typedef uint32_t uint32;
typedef uint32_t u32;

typedef int64_t int64;
typedef int64_t s64;
typedef uint64_t uint64;
typedef uint64_t u64;

typedef float real32;
typedef float r32;
typedef double real64;
typedef double r64;

typedef uint32 bool32;
typedef uint32 b32;

#define true 1
#define false 0

#endif