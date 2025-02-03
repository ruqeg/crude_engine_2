#include <tlsf.h>
#include <core/memory.h>
#include <core/assert.h>
#include <core/utils.h>
#include <stdlib.h>

typedef struct memory_statistics
{
  sizet   allocated_bytes;
  sizet   total_bytes;
  uint32  allocation_count;
} memory_statistics;

static void exit_walker( void* ptr, size_t size, int used, void* user )
{
  if ( !used ) return;

  memory_statistics* stats = CAST( memory_statistics*, user );
  stats->allocated_bytes += size;
  ++stats->allocation_count;
  CRUDE_LOG_WARNING( CRUDE_CHANNEL_MEMORY, "Found active allocation %p, %llu", ptr, size );
}

void crude_heap_allocator_initialize( crude_heap_allocator* allocator, sizet capacity )
{
  allocator->memory = malloc( capacity );
  allocator->capacity = capacity;
  allocator->tlsf_handle = tlsf_create_with_pool( allocator->memory, capacity );
  CRUDE_LOG_INFO( CRUDE_CHANNEL_MEMORY, "Heap allocator of capacity %llu created", capacity );
}

void crude_heap_allocator_deinitialize( crude_heap_allocator* allocator )
{
  memory_statistics stats = {
    .allocated_bytes = 0,
    .total_bytes = 0,
    .allocation_count = 0};

  pool_t pool = tlsf_get_pool( allocator->tlsf_handle );
  tlsf_walk_pool( pool, exit_walker, ( void* )&stats );
  
  if ( stats.allocated_bytes )
  {
    CRUDE_LOG_ERROR( CRUDE_CHANNEL_MEMORY, "Heap allocator shutdown. Allocated memory detected. Allocated %llu, total %llu", stats.allocated_bytes, stats.total_bytes );
  }
  else
  {
    CRUDE_LOG_INFO( CRUDE_CHANNEL_MEMORY, "Heap allocator shutdown - all memory free" );
  }
  
  CRUDE_ASSERTM( CRUDE_CHANNEL_MEMORY, stats.allocated_bytes == 0, "Allocations still present. Check your code!" );

  tlsf_destroy( allocator->tlsf_handle );
  free( allocator->memory );
}

void* crude_heap_allocator_allocate( crude_heap_allocator* allocator, sizet size, sizet alignment )
{
  void* allocated_memory = alignment == 1 ? tlsf_malloc( allocator->tlsf_handle, size ) : tlsf_memalign( allocator->tlsf_handle, alignment, size );
  sizet actual_size = tlsf_block_size( allocated_memory );
  return allocated_memory;
}

void crude_heap_allocator_deallocate( crude_heap_allocator* allocator, void* pointer )
{
  tlsf_free( allocator->tlsf_handle, pointer );
}

void crude_stack_allocator_initialize( crude_stack_allocator* allocator, sizet capacity )
{
  allocator->memory = malloc( capacity );
  allocator->capacity = capacity;
  allocator->occupied = 0u;
  CRUDE_LOG_INFO( CRUDE_CHANNEL_MEMORY, "Stack allocator of capacity %llu created", capacity );
}

void crude_stack_allocator_deinitialize( crude_stack_allocator* allocator )
{
  free( allocator->memory );
}

void* crude_stack_allocator_allocate( crude_stack_allocator* allocator, sizet size, sizet alignment )
{
  if ( allocator->occupied + size > allocator->capacity )
  {
    CRUDE_ABORT( CRUDE_CHANNEL_MEMORY, "New memory block is too big for current stack allocator!" );
  }
  void* memory_block = CAST( int8*, allocator->memory ) + allocator->occupied;
  allocator->occupied += size;
  return memory_block;
}

void crude_stack_allocator_deallocate( crude_stack_allocator* allocator, void* pointer )
{
  CRUDE_ASSERTM( CRUDE_CHANNEL_MEMORY, pointer >= allocator->memory, "New memory block is too big for current stack allocator!" );
  CRUDE_ASSERTM( CRUDE_CHANNEL_MEMORY, pointer < CAST( int8*, allocator->memory ) + allocator->capacity, "Out of bound free on stack allocator!" );
  CRUDE_ASSERTM( CRUDE_CHANNEL_MEMORY, pointer < CAST( int8*, allocator->memory ) + allocator->occupied, "Out of bound free on stack allocator!" );
  allocator->occupied = CAST( int8*, pointer ) - CAST( int8*, allocator->memory );
}