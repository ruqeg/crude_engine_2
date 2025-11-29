#include <tlsf.h>
#include <stdlib.h>
#include <string.h>

#include <engine/core/profiler.h>
#include <engine/core/assert.h>

#include <engine/core/memory.h>

typedef struct memory_statistics
{
  sizet   allocated_bytes;
  sizet   total_bytes;
  uint32  allocation_count;
} memory_statistics;

static void
exit_walker
(
  _In_ void                                               *ptr,
  _In_ size_t                                              size,
  _In_ int                                                 used,
  _In_ void                                               *user
)
{
  memory_statistics                                       *stats;

  if ( !used )
  {
    return;
  }

  stats = CRUDE_REINTERPRET_CAST( memory_statistics*, user );
  stats->allocated_bytes += size;
  ++stats->allocation_count;
  CRUDE_LOG_WARNING( CRUDE_CHANNEL_MEMORY, "Found active allocation %p, %llu", ptr, size );
}

/*****************************************
 *
 * Heap Allocator
 * 
 ******************************************/
void
crude_heap_allocator_initialize
(
  _In_ crude_heap_allocator                               *allocator,
  _In_ sizet                                               capacity,
  _In_ char const                                         *name
)
{
  allocator->memory = malloc( capacity );
  allocator->capacity = capacity;
  allocator->occupied = 0.f;
  allocator->name = name;
  allocator->tlsf_handle = tlsf_create_with_pool( allocator->memory, capacity );
  CRUDE_LOG_INFO( CRUDE_CHANNEL_MEMORY, "Heap allocator \"%s\" of capacity %llu created", name, capacity );
}

void
crude_heap_allocator_deinitialize
(
  _In_ crude_heap_allocator                               *allocator
)
{
  pool_t                                                   pool;
  memory_statistics                                        stats;
  
  
  stats = CRUDE_COMPOUNT( memory_statistics, {
    .allocated_bytes = 0,
    .total_bytes = 0,
    .allocation_count = 0
  } );
  
  pool = tlsf_get_pool( allocator->tlsf_handle );
  tlsf_walk_pool( pool, exit_walker, &stats );
  
  if ( stats.allocated_bytes )
  {
    CRUDE_LOG_ERROR( CRUDE_CHANNEL_MEMORY, "Heap allocator \"%s\" shutdown. Allocated memory detected. Allocated %llu, total %llu", allocator->name, stats.allocated_bytes, stats.total_bytes );
  }
  else
  {
    CRUDE_LOG_INFO( CRUDE_CHANNEL_MEMORY, "Heap allocator \"%s\" shutdown - all memory free", allocator->name );
  }
  
  CRUDE_ASSERTM( CRUDE_CHANNEL_MEMORY, stats.allocated_bytes == 0, "Allocations still present. Check your code!" );

  tlsf_destroy( allocator->tlsf_handle );
  free( allocator->memory );
}

void*
crude_heap_allocator_allocate
(
  _In_ crude_heap_allocator                               *allocator,
  _In_ sizet                                               size
)
{
  void                                                    *allocated_memory;
  sizet                                                    actual_size;

  allocated_memory = tlsf_malloc( allocator->tlsf_handle, size );
  CRUDE_ASSERTM( CRUDE_CHANNEL_MEMORY, allocated_memory, "Failed to allocate %i bytes in \"%s\" allocator!", size, allocator->name ? allocator->name : "unknown allocator" );
  actual_size = tlsf_block_size( allocated_memory );
  
  CRUDE_PROFILER_ALLOC_NAME( allocated_memory, actual_size, allocator->name );
  allocator->occupied += actual_size;

  return allocated_memory;
}

void*
crude_heap_allocator_allocate_align
(
  _In_ crude_heap_allocator                               *allocator,
  _In_ sizet                                               size,
  _In_ sizet                                               alignment
)
{
  void                                                    *allocated_memory;
  sizet                                                    actual_size;

  allocated_memory = alignment == 1 ? tlsf_malloc( allocator->tlsf_handle, size ) : tlsf_memalign( allocator->tlsf_handle, alignment, size );
  actual_size = tlsf_block_size( allocated_memory );
  CRUDE_PROFILER_ALLOC_NAME( allocated_memory, actual_size, allocator->name );
  allocator->occupied += actual_size;
  return allocated_memory;
}

void*
crude_heap_allocator_reallocate
(
  _In_ crude_heap_allocator                               *allocator,
  _In_ void                                               *pointer,
  _In_ sizet                                               size
)
{
  void                                                    *allocated_memory;
  sizet                                                    actual_size;

  if ( pointer )
  {
    actual_size = tlsf_block_size( pointer );
    allocator->occupied -= actual_size;
    CRUDE_PROFILER_FREE_NAME( pointer, allocator->name );
  }
  allocated_memory = tlsf_realloc( allocator->tlsf_handle, pointer, size );
  actual_size = tlsf_block_size( allocated_memory );
  allocator->occupied += actual_size;
  CRUDE_PROFILER_ALLOC_NAME( allocated_memory, actual_size, allocator->name );
  return allocated_memory;
}

void
crude_heap_allocator_deallocate
(
  _In_ crude_heap_allocator                               *allocator,
  _In_ void                                               *pointer
)
{
  tlsf_free( allocator->tlsf_handle, pointer );
  CRUDE_PROFILER_FREE_NAME( pointer, allocator->name );
}

/*****************************************
 *
 * Stack Allocator
 * 
 ******************************************/
void
crude_stack_allocator_initialize
(
  _In_ crude_stack_allocator                              *allocator,
  _In_ sizet                                               capacity,
  _In_ char const                                         *name
)
{
  allocator->memory = malloc( capacity );
  allocator->capacity = capacity;
  allocator->occupied = 0u;
  allocator->name = name;
  CRUDE_LOG_INFO( CRUDE_CHANNEL_MEMORY, "Stack allocator of capacity %llu created", capacity );
}

void
crude_stack_allocator_deinitialize
(
  _In_ crude_stack_allocator                              *allocator
)
{
  CRUDE_ASSERTM( CRUDE_CHANNEL_MEMORY, allocator->occupied == 0u, "Stack allocator \"%s\" shutdown. Allocated memory detected. Allocated %llu, total %llu", allocator->name, allocator->occupied, allocator->capacity );
  free( allocator->memory );
}

CRUDE_API void*
crude_stack_allocator_allocate
( 
  _In_ crude_stack_allocator                              *allocator,
  _In_ sizet                                               size
)
{
  void                                                    *memory_block;
  
  if ( allocator->occupied + size > allocator->capacity )
  {
    CRUDE_ABORT( CRUDE_CHANNEL_MEMORY, "New memory block is too big for current stack allocator! %i occupied, %i requested size, %i capacity", allocator->occupied, size, allocator->capacity );
  }
  memory_block = ( int8* )( allocator->memory ) + allocator->occupied;
  allocator->occupied += size;
  return memory_block;
}

size_t
crude_stack_allocator_get_marker
(
  _In_ crude_stack_allocator                              *allocator
)
{
  return allocator->occupied;
}

void
crude_stack_allocator_free_marker
(
  _In_ crude_stack_allocator                              *allocator,
  _In_ size_t                                              marker
)
{
  sizet difference = marker - allocator->occupied;
  if ( difference > 0u )
  {
    allocator->occupied = marker;
  }
}

/*****************************************
 *
 * Linear Allocator
 * 
 ******************************************/
void
crude_linear_allocator_initialize
(
  _In_ crude_linear_allocator                             *allocator,
  _In_ sizet                                               capacity,
  _In_ char const                                         *name
)
{
  allocator->memory = malloc( capacity );
  allocator->capacity = capacity;
  allocator->occupied = 0u;
  allocator->name = name;
  CRUDE_LOG_INFO( CRUDE_CHANNEL_MEMORY, "Linear allocator of capacity %llu created", capacity );
}

void
crude_linear_allocator_deinitialize
(
  _In_ crude_linear_allocator                             *allocator
)
{
  free( allocator->memory );
}

void*
crude_linear_allocator_allocate
( 
  _In_ crude_linear_allocator                             *allocator,
  _In_ sizet                                               size
)
{
  void                                                    *memory_block;
  
  if ( allocator->occupied + size > allocator->capacity )
  {
    CRUDE_ABORT( CRUDE_CHANNEL_MEMORY, "New memory block is too big for current linear allocator! %i occupied, %i requested size, %i capacity", allocator->occupied, size, allocator->capacity );
  }
  memory_block = ( int8* )( allocator->memory ) + allocator->occupied;
  allocator->occupied += size;
  return memory_block;
}

void
crude_linear_allocator_clear
(
  _In_ crude_linear_allocator                             *allocator
)
{
  allocator->occupied = 0u;
}

/*****************************************
 *
 * Utils
 * 
 ******************************************/
sizet
crude_memory_align
(
  _In_ sizet                                               size,
  _In_ sizet                                               alignment
)
{
  sizet alignment_mask = alignment - 1;
  return ( size + alignment_mask ) & ~alignment_mask;
}

void
crude_memory_copy
(
  _Out_ void                                              *dst,
  _In_ void const                                         *src,
  _In_ sizet                                               size
)
{
  memcpy( dst, src, size );
}

void
crude_memory_set
(
  _Out_ void                                              *dst,
  _In_ int32                                               val,
  _In_ sizet                                               size
)
{
  memset( dst, val, size );
}

/*****************************************
 *
 * Common Allocator Interface
 * 
 ******************************************/
// I'm done with this shit
static void* crude_heap_allocate_raw( void *ctx, sizet size ) { return crude_heap_allocator_allocate( CRUDE_REINTERPRET_CAST( crude_heap_allocator*, ctx ), size ); }
static void crude_heap_deallocate_raw( void *ctx, void *pointer ) { crude_heap_allocator_deallocate( CRUDE_REINTERPRET_CAST( crude_heap_allocator*, ctx ), pointer ); }
static void* crude_heap_allocate_align_raw( void *ctx, sizet size, sizet alignment ) { return crude_heap_allocator_allocate_align( CRUDE_REINTERPRET_CAST( crude_heap_allocator*, ctx ), size, alignment ); }
static void* crude_stack_allocate_raw( void *ctx, sizet size ) { return crude_stack_allocator_allocate( CRUDE_REINTERPRET_CAST( crude_stack_allocator*, ctx ), size ); }
static void crude_stack_deallocate_raw( void *ctx, void *pointer ) {}
static void* crude_stack_allocate_align_raw( void *ctx, sizet size, sizet alignment ) { CRUDE_ABORT( CRUDE_CHANNEL_CORE, "TODO crude_stack_allocate_align_raw" ); return NULL; }
static void* crude_linear_allocate_raw( void *ctx, sizet size ) { return crude_linear_allocator_allocate( CRUDE_REINTERPRET_CAST( crude_linear_allocator*, ctx ), size ); }
static void crude_linear_deallocate_raw( void *ctx, void *pointer ) {}
static void* crude_linear_allocate_align_raw( void *ctx, sizet size, sizet alignment ) { CRUDE_ABORT( CRUDE_CHANNEL_CORE, "TODO crude_linear_allocate_align_raw" ); return NULL; }

crude_allocator_container 
crude_heap_allocator_pack
(
  _In_ crude_heap_allocator                               *heap_allocator
)
{
  crude_allocator_container allocator = {
    .allocate = crude_heap_allocate_raw,
    .deallocate = crude_heap_deallocate_raw,
    .allocate_align = crude_heap_allocate_align_raw,
    .ctx = heap_allocator,
  };
  return allocator;
}

crude_allocator_container 
crude_stack_allocator_pack
(
  _In_ crude_stack_allocator                              *stack_allocator
)
{
  crude_allocator_container allocator = {
    .allocate = crude_stack_allocate_raw,
    .deallocate = crude_stack_deallocate_raw,
    .allocate_align = crude_stack_allocate_align_raw,
    .ctx = stack_allocator,
  };
  return allocator;
}

crude_allocator_container 
crude_linear_allocator_pack
(
  _In_ crude_linear_allocator                             *linear_allocator
)
{
  crude_allocator_container allocator = {
    .allocate = crude_linear_allocate_raw,
    .deallocate = crude_linear_deallocate_raw,
    .allocate_align = crude_linear_allocate_align_raw,
    .ctx = linear_allocator,
  };
  return allocator;
}

crude_allocator_type
crude_allocator_container_get_type
(
  _In_ crude_allocator_container                           allocator_container
)
{
  if ( allocator_container.allocate == crude_heap_allocate_raw )
  {
    return CRUDE_ALLOCATOR_TYPE_HEAP;
  }
  else if ( allocator_container.allocate == crude_stack_allocate_raw )
  {
    return CRUDE_ALLOCATOR_TYPE_STACK;
  }
  else if ( allocator_container.allocate == crude_linear_allocate_raw )
  {
    return CRUDE_ALLOCATOR_TYPE_LINEAR;
  }
  return CRUDE_ALLOCATOR_TYPE_UNKNOWN;
}