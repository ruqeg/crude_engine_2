#pragma once

#include <stdint.h>
#include <stdbool.h>

#if defined(_MSC_VER)
#if defined(__cplusplus)
#define CRUDE_EXPORT extern "C" __declspec(dllexport)
#define CRUDE_IMPORT extern "C" __declspec(dllimport)
#else
#define CRUDE_EXPORT __declspec(dllexport)
#define CRUDE_IMPORT __declspec(dllimport)
#endif
#endif

#ifdef crude_engine_EXPORTS
#define CRUDE_API CRUDE_EXPORT
#else
#define CRUDE_API CRUDE_IMPORT
#endif

#define CRUDE_DEBUG_BREAK   __debugbreak()
#define CRUDE_INLINE        inline

#define CAST( t, exp )      ( ( t ) ( exp ) )
#define ARRAY_SIZE( arr )   ( sizeof( arr ) / sizeof( arr[ 0 ] ) )

#define MAX( a, b) ( a ) > ( b ) ? ( a ) : ( b )
#define MIN( a, b) ( a ) < ( b ) ? ( a ) : ( b )

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