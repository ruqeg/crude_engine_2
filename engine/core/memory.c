#include <tlsf.h>
#include <core/memory.h>
#include <core/assert.h>
#include <stdlib.h>

typedef struct memory_statistics
{
  sizet   allocated_bytes;
  sizet   total_bytes;
  uint32  allocation_count;
} memory_statistics;

void exit_walker( void* ptr, size_t size, int used, void* user )
{
  if ( !used ) return;

  memory_statistics* stats = CRUDE_CAST( memory_statistics*, user );
  stats->allocated_bytes += size;
  ++stats->allocation_count;
  CRUDE_LOG_WARNING( CRUDE_CHANNEL_MEMORY, "Found active allocation %p, %llu", ptr, size );
}

void crude_heap_initialize( crude_heap* heap, sizet size )
{
  heap->memory = malloc( size );
  heap->size = size;
  heap->tlsf_handle = tlsf_create_with_pool( heap->memory, size );

  CRUDE_LOG_INFO( CRUDE_CHANNEL_MEMORY, "Heap allocator of size %llu created", size );
}

void crude_heap_deinitialize( crude_heap* heap )
{

  memory_statistics stats;
  pool_t pool = tlsf_get_pool( heap->tlsf_handle );
  tlsf_walk_pool( pool, exit_walker, ( void* )&stats );
  
  if ( stats.allocated_bytes )
  {
    CRUDE_LOG_ERROR( CRUDE_CHANNEL_MEMORY, "Heap allocator shutdown. Allocated memory detected. Allocated %llu, total %llu", stats.allocated_bytes, stats.total_bytes );
  }
  else
  {
    CRUDE_LOG_INFO( CRUDE_CHANNEL_MEMORY, "Heap allocator shutdown - all memory free" );
  }
  
  CRUDE_ASSERTM( stats.allocated_bytes == 0, "Allocations still present. Check your code!" );

  tlsf_destroy( heap->tlsf_handle );
  free( heap->memory );
}

void* crude_heap_allocate( crude_heap* heap, sizet size, sizet alignment )
{
  void* allocated_memory = alignment == 1 ? tlsf_malloc( heap->tlsf_handle, size ) : tlsf_memalign( heap->tlsf_handle, alignment, size );
  sizet actual_size = tlsf_block_size( allocated_memory );
  return allocated_memory;
}

void crude_heap_deallocate( crude_heap* heap, void* pointer )
{
  tlsf_free( heap->tlsf_handle, pointer );
}