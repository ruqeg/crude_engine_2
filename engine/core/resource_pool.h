#pragma once

#include <core/memory.h>

#define CRUDE_RESOURCE_INVALID_HANDLE 0xffffffff

typedef struct crude_resource_pool
{
  crude_allocator        allocator;
  uint8                 *memory;
  uint32                *free_indices;
  uint32                 free_indices_head;
  uint32                 pool_size;
  uint32                 resource_size;
  uint32                 used_indices;
} crude_resource_pool;

typedef uint32 crude_resource_handle;

CRUDE_API void crude_initialize_resource_pool( _In_ crude_resource_pool *resource_pool, _In_ crude_allocator allocator, _In_ uint32 pool_size, _In_ uint32 resource_size );
CRUDE_API void crude_deinitialize_resource_pool( _In_ crude_resource_pool *resource_pool );
CRUDE_API crude_resource_handle crude_resource_pool_obtain( _In_ crude_resource_pool *resource_pool );
CRUDE_API void crude_resource_pool_release( _In_ crude_resource_pool *resource_pool, _In_ crude_resource_handle handle );
CRUDE_API void crude_resource_pool_free_all( _In_ crude_resource_pool *resource_pool );
CRUDE_API void* crude_resource_pool_access( _In_ crude_resource_pool *resource_pool, _In_ crude_resource_handle handle );