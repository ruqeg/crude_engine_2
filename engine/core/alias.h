#pragma once

#include <stdint.h>
#include <stdbool.h>

/************************************************
 *
 * Compiler Macros
 * 
 ***********************************************/
#if defined(_MSC_VER)
#if defined(__cplusplus)
#define CRUDE_EXPORT extern "C" __declspec(dllexport)
#define CRUDE_IMPORT extern "C" __declspec(dllimport)
#else
#define CRUDE_EXPORT __declspec(dllexport)
#define CRUDE_IMPORT __declspec(dllimport)
#endif
#endif

#if __cplusplus >= 201703L
#define CRUDE_ALIGNED_DATA(x)   alignas(x)
#define CRUDE_ALIGNED_STRUCT(x) struct alignas(x)
#elif defined(__GNUC__)
#define CRUDE_ALIGNED_DATA(x)   __attribute__ ((aligned(x)))
#define CRUDE_ALIGNED_STRUCT(x) struct __attribute__ ((aligned(x)))
#else
#define CRUDE_ALIGNED(x)   __declspec(align(x))
#define CRUDE_ALIGNED_STRUCT(x) __declspec(align(x)) struct
#endif

#ifdef crude_engine_EXPORTS
#define CRUDE_API CRUDE_EXPORT
#else
#define CRUDE_API CRUDE_IMPORT
#endif

#define CRUDE_DEBUG_BREAK       __debugbreak()
#define CRUDE_INLINE            inline

/************************************************
 *
 * Types
 * 
 ***********************************************/
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

/************************************************
 *
 * Utils Macros
 * 
 ***********************************************/
#define CAST( t, exp ) ( ( t ) ( exp ) )
#define ARRAY_SIZE( arr ) ( sizeof( arr ) / sizeof( arr[ 0 ] ) )