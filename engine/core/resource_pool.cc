#include <string.h>

#include <core/assert.h>

#include <core/resource_pool.h>

void
crude_resource_pool_initialize
(
  _In_ crude_resource_pool                                *resource_pool,
  _In_ crude_allocator_container                           allocator,
  _In_ uint32                                              pool_size,
  _In_ uint32                                              resource_size
)
{
  resource_pool->allocator = allocator;
  resource_pool->pool_size = pool_size;
  resource_pool->resource_size = resource_size;

  uint64 allocation_size = pool_size * ( resource_size + sizeof( uint32 ) );
  resource_pool->memory = CRUDE_REINTERPRET_CAST( uint8*, CRUDE_ALLOCATE( allocator, allocation_size ) );
  memset( resource_pool->memory, 0, allocation_size );
  
  resource_pool->free_indices = CRUDE_REINTERPRET_CAST( uint32*, resource_pool->memory + pool_size * resource_size );
  resource_pool->free_indices_head = 0;
  
  for ( uint32 i = 0; i < pool_size; ++i )
  {
    resource_pool->free_indices[ i ] = i;
  }
  
  resource_pool->used_indices = 0;
}

void
crude_resource_pool_deinitialize
(
  _In_ crude_resource_pool                                *resource_pool
)
{
  CRUDE_ASSERT( resource_pool );

  if ( resource_pool->free_indices_head != 0 )
  {
    CRUDE_LOG_ERROR( CRUDE_CHANNEL_CORE, "Resource pool has unfreed resources" );
  
    for ( uint32 i = 0; i < resource_pool->free_indices_head; ++i )
    {
      CRUDE_LOG_ERROR( CRUDE_CHANNEL_CORE, "\tResource %u", resource_pool->free_indices[i] );
    }
  }

  CRUDE_ASSERT( ( resource_pool->free_indices_head == 0 ) && ( resource_pool->used_indices == 0 ) );

  CRUDE_DEALLOCATE(resource_pool->allocator, resource_pool->memory );
}

uint32
crude_resource_pool_obtain_resource
(
  _In_ crude_resource_pool                                *resource_pool
)
{
  if ( resource_pool->free_indices_head < resource_pool->pool_size )
  {
    uint32 free_index = resource_pool->free_indices[ resource_pool->free_indices_head++ ];
    ++resource_pool->used_indices;
    return free_index;
  }
  CRUDE_ASSERT( false );
  return CRUDE_RESOURCE_INDEX_INVALID;
}

void
crude_resource_pool_release_resource
(
  _In_ crude_resource_pool                                *resource_pool,
  _In_ uint32                                              handle
)
{
  resource_pool->free_indices[ --resource_pool->free_indices_head ] = handle;
  --resource_pool->used_indices;
}

void
crude_resource_pool_free_all_resource
(
  _In_ crude_resource_pool                                *resource_pool
)
{
  resource_pool->free_indices_head = 0u;
  resource_pool->used_indices = 0u;

  for ( uint32 i = 0; i < resource_pool->pool_size; ++i )
  {
    resource_pool->free_indices[ i ] = i;
  }
}

void*
crude_resource_pool_access_resource
(
  _In_ crude_resource_pool                                *resource_pool,
  _In_ uint32                                              handle
)
{
  if ( handle != CRUDE_RESOURCE_INDEX_INVALID )
  {
    return &resource_pool->memory[ handle * resource_pool->resource_size ];
  }
  return NULL;
}