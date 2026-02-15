#pragma once

#include <stdint.h>
#include <stdbool.h>
#include <type_traits>

/************************************************
 *
 * Constants
 * 
 ***********************************************/
#define CRUDE_ASYNCHRONOUS_LOADERS_MAX_COUNT 1
#define CRUDE_ASYNCHRONOUS_LOADER_MANAGER_ACTIVE_THREAD 1
#define CRUDE_ENGINE_TASK_SHEDULER_PINNED_TASK_ACTIVE_THREAD 4
#define CRUDE_ENGINE_ENVIRONMENT_INITIAL "main.crude_environment"

#define CRUDE_SHADER_DEBUG_MODE_NONE 0

#define CRUDE_PHYSICS_MAX_BODIES_COUNT 1024
#define CRUDE_PHYSICS_STEP_DELTA_TIME 0.008 
  
#define CRUDE_SCENE_STRING_LINEAR_ALLOCATOR_SIZE 5000
#define CRUDE_SCENE_OCTREE_RESOURCE_POOl_SIZE 32

/************************************************
 *
 * Compiler Macros
 * 
 ***********************************************/
//#if defined(_MSC_VER)
//#if defined(__cplusplus)
//#define CRUDE_EXPORT extern "C" __declspec( dllexport )
//#define CRUDE_IMPORT extern "C" __declspec( dllimport )
//#else
//#define CRUDE_EXPORT __declspec( dllexport )
//#define CRUDE_IMPORT __declspec( dllimport )
//#endif
//#endif
#define CRUDE_EXPORT extern
#define CRUDE_IMPORT extern

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

#ifndef __cplusplus
#define CRUDE_STATIC_ASSERT( x, m ) _Static_assert( x, m );
#else
#define CRUDE_STATIC_ASSERT( x, m ) static_assert( x, m );
#endif

#define CRUDE_DEBUG_BREAK       __debugbreak()
#define CRUDE_INLINE            inline

#ifndef __cplusplus
#define CRUDE_CAST( t, v ) ( ( t )( v ) )
#define CRUDE_STATIC_CAST( t, v ) ( ( t ) ( v ) )
#define CRUDE_REINTERPRET_CAST( t, v ) ( ( t ) ( v ) )
#else
#define CRUDE_CAST( t, v ) ( ( t )( v ) )
#define CRUDE_STATIC_CAST( t, v ) ( static_cast<t>( v ) )
#define CRUDE_REINTERPRET_CAST( t, v ) ( reinterpret_cast<t>( v ) )
#endif

#ifndef __cplusplus
#define CRUDE_COMPOUNT( t, ... ) ( ( t ) ##__VA_ARGS__ )
#define CRUDE_COMPOUNT_EMPTY( t ) (( t ) { 0 } )
#else
#define CRUDE_COMPOUNT( t, ... ) ( t __VA_ARGS__ )
#define CRUDE_COMPOUNT_EMPTY( t ) ( t {} )
#endif

#ifndef __cplusplus
#define CRUDE_TYPE( v ) typeof( v )
#else
#define CRUDE_TYPE( v ) std::remove_reference_t<decltype( v )>
#endif


#ifdef __cplusplus
#define CRUDE_OFFSETOF( s, m ) ( (::size_t )&reinterpret_cast< char const volatile& >( ( ( ( s* )0 )->m ) ) )
#else
#define CRUDE_OFFSETOF( s, m ) ( ( size_t ) &( ( ( s* )0 )->m ) )
#endif

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
#define CRUDE_COUNTOF( a ) ( sizeof( a ) / sizeof( a[ 0 ] ) )
#define CRUDE_SWAP( a, b ) { decltype( a ) temp = a; a = b; b = temp; }