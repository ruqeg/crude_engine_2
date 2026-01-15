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

#define CRUDE_CORE_HASHMAP_INITIAL_CAPACITY 16
#define CRUDE_CORE_PROCESS_LOGGER_BUFFER_SIZE 256
#define CRUDE_CORE_PROCESS_OUTPUT_BUFFER_SIZE 1024

#define CRUDE_SHADER_DEBUG_MODE_NONE 0

#define CRUDE_GRAPHICS_CONSTANT_MESHLET_MAX_VERTICES 128
#define CRUDE_GRAPHICS_CONSTANT_MESHLET_MAX_TRIANGLES 124
#define CRUDE_GRAPHICS_CONSTANT_MESHLET_CONE_WEIGHT 0.0
#define CRUDE_GRAPHICS_TEXTURE_UPDATE_COMMANDS_THREAD_ID 1
#define CRUDE_GRAPHICS_ASYNCHRONOUS_LOADER_UPLOAD_REQUESTS_LIMIT 1024
#define CRUDE_GRAPHICS_ASYNCHRONOUS_LOADER_FILE_LOAD_REQUESTS_LIMIT 1024
#define CRUDE_GRAPHICS_FRAME_DESCRIPTOR_POOL_ELEMENTES_COUNT 1024
#define CRUDE_GRAPHICS_DEPTH_AND_STENCIL_CLEAR_COLOR_INDEX 8
#define CRUDE_GRAPHICS_SWAPCHAIN_BARRIER_COMMAND_BUFFER_THREAD_ID 0
#define CRUDE_GRAPHICS_TEXTURE_COPY_FROM_BUFFER_THREAD_ID 0
#define CRUDE_GRAPHICS_MAX_SWAPCHAIN_IMAGES 3
#define CRUDE_GRAPHICS_MAX_IMAGE_OUTPUTS 8
#define CRUDE_GRAPHICS_MAX_DESCRIPTOR_SET_LAYOUTS 8
#define CRUDE_GRAPHICS_MAX_SHADER_STAGES 5
#define CRUDE_GRAPHICS_MAX_DESCRIPTORS_PER_SET 64
#define CRUDE_GRAPHICS_UBO_ALIGNMENT 256
#define CRUDE_GRAPHICS_MAX_SET_COUNT 32
#define CRUDE_GRAPHICS_MAX_BINDLESS_RESOURCES 1024
#define CRUDE_GRAPHICS_DEPTH_PYRAMID_PASS_MAX_LEVELS 16
#define CRUDE_GRAPHICS_TETRAHEDRON_SHADOWMAP_SIZE 8192
#define CRUDE_GRAPHICS_RENDER_GRAPH_MAX_RESOURCES_COUNT 1024
#define CRUDE_GRAPHICS_RENDER_GRAPH_MAX_NODES_COUNT 1024
#define CRUDE_GRAPHICS_SCENE_RENDERER_MAX_DEBUG_LINES 640000
#define CRUDE_GRAPHICS_SCENE_RENDERER_MAX_DEBUG_CUBES 60000
#define CRUDE_GRAPHICS_SCENE_RENDERER_MESH_INSTANCES_BUFFER_CAPACITY 200
#define CRUDE_GRAPHICS_SCENE_RENDERER_COLLISION_MESH_INSTANCES_BUFFER_CAPACITY 200
#define CRUDE_GRAPHICS_DEVICE_HEAP_ALLOCATOR_MIN_BLOCK_SIZE 16
#define CRUDE_GRAPHICS_DEVICE_HEAP_ALLOCATOR_DEFAULT_ALIGN 16
#define CRUDE_GRAPHICS_DEVICE_HEAP_ALLOCATOR_DEFAULT_ALLOCATION_STRATEGY 1
#define CRUDE_GRAPHICS_PRESENT_TEXTURE_NAME "final"

#define CRUDE_PHYSICS_MAX_BODIES_COUNT 1024
#define CRUDE_PHYSICS_STEP_DELTA_TIME 0.008 
  
#define CRUDE_SCENE_STRING_LINEAR_ALLOCATOR_SIZE 5000
#define CRUDE_SCENE_OCTREE_RESOURCE_POOl_SIZE 32

/************************************************
 *
 * Compiler Macros
 * 
 ***********************************************/
#if defined(_MSC_VER)
#if defined(__cplusplus)
#define CRUDE_EXPORT extern "C" __declspec( dllexport )
#define CRUDE_IMPORT extern "C" __declspec( dllimport )
#else
#define CRUDE_EXPORT __declspec( dllexport )
#define CRUDE_IMPORT __declspec( dllimport )
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
#define CRUDE_COMPOUNT( t, ... ) ( t ##__VA_ARGS__ )
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