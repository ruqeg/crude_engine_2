#include <tlsf.h>
#include <stdlib.h>

#include <core/profiler.h>
#include <core/assert.h>

#include <core/memory.h>

typedef struct memory_statistics
{
  sizet   allocated_bytes;
  sizet   total_bytes;
  uint32  allocation_count;
} memory_statistics;

static void
exit_walker
(
  _In_ void    *ptr,
  _In_ size_t   size,
  _In_ int      used,
  _In_ void    *user
)
{
  if ( !used ) return;

  memory_statistics* stats = CAST( memory_statistics*, user );
  stats->allocated_bytes += size;
  ++stats->allocation_count;
  CRUDE_LOG_WARNING( CRUDE_CHANNEL_MEMORY, "Found active allocation %p, %llu", ptr, size );
}

void
crude_initialize_heap_allocator
(
  _In_ crude_heap_allocator  *allocator,
  _In_ sizet                  capacity,
  char const                  *name
)
{
  allocator->memory = malloc( capacity );
  allocator->capacity = capacity;
  allocator->name = name;
  allocator->tlsf_handle = tlsf_create_with_pool( allocator->memory, capacity );
  CRUDE_LOG_INFO( CRUDE_CHANNEL_MEMORY, "Heap allocator \"%s\" of capacity %llu created", name, capacity );
}

void
crude_deinitialize_heap_allocator
(
  _In_ crude_heap_allocator *allocator
)
{
  memory_statistics stats = {
    .allocated_bytes = 0,
    .total_bytes = 0,
    .allocation_count = 0};

  pool_t pool = tlsf_get_pool( allocator->tlsf_handle );
  tlsf_walk_pool( pool, exit_walker, CAST( void*, &stats ) );
  
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
crude_heap_allocate
(
  _In_ crude_heap_allocator *allocator,
  _In_ sizet                 size
)
{
  
  void* allocated_memory = tlsf_malloc( allocator->tlsf_handle, size );
  sizet actual_size = tlsf_block_size( allocated_memory );
  CRUDE_PROFILER_ALLOC_NAME( allocated_memory, actual_size, allocator->name );
  return allocated_memory;
}

CRUDE_API void*
crude_heap_allocate_align
(
  _In_ crude_heap_allocator   *allocator,
  _In_ sizet                   size,
  _In_ sizet                   alignment
)
{
  void* allocated_memory = alignment == 1 ? tlsf_malloc( allocator->tlsf_handle, size ) : tlsf_memalign( allocator->tlsf_handle, alignment, size );
  sizet actual_size = tlsf_block_size( allocated_memory );
  CRUDE_PROFILER_ALLOC_NAME( allocated_memory, actual_size, allocator->name );
  return allocated_memory;
}

void*
crude_heap_reallocate
(
  _In_ crude_heap_allocator *allocator,
  _In_ void                 *pointer,
  _In_ sizet                 size
)
{
  if ( pointer )
  {
    CRUDE_PROFILER_FREE_NAME( pointer, allocator->name );
  }
  void* allocated_memory = tlsf_realloc( allocator->tlsf_handle, pointer, size );
  sizet actual_size = tlsf_block_size( allocated_memory );
  CRUDE_PROFILER_ALLOC_NAME( allocated_memory, actual_size, allocator->name );
  return allocated_memory;
}

void
crude_heap_deallocate
(
  _In_ crude_heap_allocator *allocator,
  _In_ void                 *pointer
)
{
  tlsf_free( allocator->tlsf_handle, pointer );
  CRUDE_PROFILER_FREE_NAME( pointer, allocator->name );
}

void
crude_initialize_stack_allocator
(
  _In_ crude_stack_allocator *allocator,
  _In_ sizet                  capacity
)
{
  allocator->memory = malloc( capacity );
  allocator->capacity = capacity;
  allocator->occupied = 0u;
  CRUDE_LOG_INFO( CRUDE_CHANNEL_MEMORY, "Stack allocator of capacity %llu created", capacity );
}

void
crude_deinitialize_stack_allocator
(
  _In_ crude_stack_allocator *allocator
)
{
  free( allocator->memory );
}

void*
crude_stack_allocate
(
  _In_ crude_stack_allocator *allocator,
  _In_ sizet                  size,
  _In_ sizet                  alignment
)
{
  if ( allocator->occupied + size > allocator->capacity )
  {
    CRUDE_ABORT( CRUDE_CHANNEL_MEMORY, "New memory block is too big for current stack allocator!" );
  }
  void* memory_block = CAST( int8*, allocator->memory ) + allocator->occupied;
  allocator->occupied += size;
  return memory_block;
}

void
crude_stack_deallocate
(
  _In_ crude_stack_allocator  *allocator,
  _In_ void                   *pointer
)
{
  CRUDE_ASSERTM( CRUDE_CHANNEL_MEMORY, pointer >= allocator->memory, "New memory block is too big for current stack allocator!" );
  CRUDE_ASSERTM( CRUDE_CHANNEL_MEMORY, pointer < CAST( int8*, allocator->memory ) + allocator->capacity, "Out of bound free on stack allocator!" );
  CRUDE_ASSERTM( CRUDE_CHANNEL_MEMORY, pointer < CAST( int8*, allocator->memory ) + allocator->occupied, "Out of bound free on stack allocator!" );
  allocator->occupied = CAST( int8*, pointer ) - CAST( int8*, allocator->memory );
}

sizet
crude_memory_align
(
  _In_ sizet                   size,
  _In_ sizet                   alignment
)
{
  const sizet alignment_mask = alignment - 1;
  return ( size + alignment_mask ) & ~alignment_mask;
}

// Bro I'm done with this shit
static void* crude_heap_allocate_raw( void *ctx, sizet size ) { return crude_heap_allocate( ctx, size ); }
static void crude_heap_deallocate_raw( void *ctx, void *pointer ) { crude_heap_deallocate( ctx, pointer ); }
static void* crude_heap_reallocate_raw( void *ctx, void *pointer, sizet size ) { return crude_heap_reallocate( ctx, pointer, size ); }
static void* crude_heap_allocate_align_raw( void *ctx, sizet size, sizet alignment ) { return crude_heap_allocate_align( ctx, size, alignment ); }

CRUDE_API crude_allocator 
crude_pack_heap_allocator
(
  _In_ crude_heap_allocator                               *heap_allocator
)
{
  crude_allocator allocator = {
    .allocate = crude_heap_allocate_raw,
    .reallocate = crude_heap_reallocate_raw,
    .deallocate = crude_heap_deallocate_raw,
    .allocate_align = crude_heap_allocate_align_raw,
    .ctx = heap_allocator,
  };
  return allocator;
}