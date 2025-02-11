#include <string.h>

#include <core/assert.h>

#include <core/resource_pool.h>

crude_resource_pool crude_resource_pool_create( crude_allocator allocator, uint32 pool_size, uint32 resource_size )
{
  crude_resource_pool resource_pool;

  resource_pool.allocator = allocator;
  resource_pool.pool_size = pool_size;
  resource_pool.resource_size = resource_size;

  uint64 allocation_size = pool_size * ( resource_size + sizeof( uint32 ) );
  resource_pool.memory = ( uint8* )allocator.allocate( allocation_size, 1 );
  memset( resource_pool.memory, 0, allocation_size );
  
  resource_pool.free_indices = ( uint32* )( resource_pool.memory + pool_size * resource_size );
  resource_pool.free_indices_head = 0;
  
  for ( uint32 i = 0; i < pool_size; ++i )
  {
    resource_pool.free_indices[i] = i;
  }
  
  resource_pool.used_indices = 0;

  return resource_pool;
}

void crude_resource_pool_destroy( crude_resource_pool *resource_pool )
{
  CRUDE_ASSERT( resource_pool && ( resource_pool->free_indices_head != 0 ) && ( resource_pool->used_indices == 0 ) );
  resource_pool->allocator.deallocate( resource_pool->memory );
}

crude_resource_handle crude_resource_pool_obtain_resource( crude_resource_pool *resource_pool )
{
  if ( resource_pool->free_indices_head < resource_pool->pool_size )
  {
    uint32 free_index = resource_pool->free_indices[resource_pool->free_indices_head++];
    ++resource_pool->used_indices;
    return free_index;
  }
  CRUDE_ASSERT( false );
  return CRUDE_RESOURCE_INVALID_HANDLE;
}

void crude_resource_pool_release_resource( crude_resource_pool *resource_pool, crude_resource_handle handle )
{
  resource_pool->free_indices[--resource_pool->free_indices_head] = handle;
  --resource_pool->used_indices;
}

void crude_resource_pool_free_all_resources( crude_resource_pool *resource_pool )
{
  resource_pool->free_indices_head = 0u;
  resource_pool->used_indices = 0u;

  for ( uint32 i = 0; i < resource_pool->pool_size; ++i )
  {
    resource_pool->free_indices[i] = i;
  }
}

void* crude_resource_pool_access_resource( crude_resource_pool *resource_pool, crude_resource_handle handle )
{
  if ( handle != CRUDE_RESOURCE_INVALID_HANDLE )
  {
    return &resource_pool->memory[handle * resource_pool->resource_size];
  }
  return NULL;
}