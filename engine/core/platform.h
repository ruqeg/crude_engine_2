#pragma once

#include <stdint.h>

#define CRUDE_DEBUG_BREAK   __debugbreak();
#define CRUDE_INLINE        inline

#define CRUDE_CAST(t, exp)  ((t) (exp))

typedef int8_t        int8;
typedef int16_t       int16;
typedef int64_t       int64;
typedef int32_t       int32;

typedef uint8_t       uint8;
typedef uint16_t      uint16;
typedef uint64_t      uint64;
typedef uint32_t      uint32;

typedef float         float32;
typedef double        float64;

typedef size_t        sizet;

typedef const char*   cstring;