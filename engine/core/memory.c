#include <tlsf.h>
#include <core/memory.h>
#include <core/assert.h>
#include <stdlib.h>

#define CRUDE_VALIDATE_ALLOCATOR( allocator ) CRUDE_ASSERTM( allocator, CRUDE_CHANNEL_MEMORY, "Invalid allocator!" );
#define CRUDE_GET_ALLOCATOR_TYPE( allocator ) CRUDE_CAST( crude_allocator_common_data*, allocator )->type

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

void crude_heap_allocator_initialize( crude_heap_allocator* allocator, sizet size )
{
  allocator->memory = malloc( size );
  allocator->size = size;
  allocator->tlsf_handle = tlsf_create_with_pool( allocator->memory, size );

  CRUDE_LOG_INFO( CRUDE_CHANNEL_MEMORY, "Heap allocator of size %llu created", size );
}

void crude_heap_allocator_deinitialize( crude_heap_allocator* allocator )
{

  memory_statistics stats;
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
  
  CRUDE_ASSERTM( stats.allocated_bytes == 0, "Allocations still present. Check your code!" );

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

void* crude_allocate( void* allocator, sizet size, sizet alignment )
{
  CRUDE_VALIDATE_ALLOCATOR( allocator );
  switch ( CRUDE_GET_ALLOCATOR_TYPE( allocator ) )
  {
  case CRUDE_ALLOCATOR_TYPE_HEAP:
    return crude_heap_allocator_allocate( CRUDE_CAST( crude_heap_allocator*, allocator ), size, alignment );
  }
  CRUDE_ABORT( CRUDE_CHANNEL_MEMORY, "Unsupported allocator!" );
  return NULL;
}

void crude_deallocate( void* allocator, void* pointer )
{
  CRUDE_VALIDATE_ALLOCATOR( allocator );
  switch ( CRUDE_GET_ALLOCATOR_TYPE( allocator ) )
  {
  case CRUDE_ALLOCATOR_TYPE_HEAP:
    crude_heap_allocator_deallocate( CRUDE_CAST( crude_heap_allocator*, allocator ), pointer );
  }
  CRUDE_ABORT( CRUDE_CHANNEL_MEMORY, "Unsupported allocator!" );
}
