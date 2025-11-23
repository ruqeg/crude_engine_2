#pragma once

#include <engine/core/memory.h>

#define CRUDE_RESOURCE_INDEX_INVALID 0xffffffff

typedef struct crude_resource_pool
{
  crude_allocator_container                                allocator;
  uint8                                                   *memory;
  uint32                                                  *free_indices;
  uint32                                                   free_indices_head;
  uint32                                                   pool_size;
  uint32                                                   resource_size;
  uint32                                                   used_indices;
} crude_resource_pool;

CRUDE_API void
crude_resource_pool_initialize
(
  _In_ crude_resource_pool                                *resource_pool,
  _In_ crude_allocator_container                           allocator,
  _In_ uint32                                              pool_size,
  _In_ uint32                                              resource_size
);

CRUDE_API void
crude_resource_pool_deinitialize
( 
  _In_ crude_resource_pool                                *resource_pool
);

CRUDE_API uint32
crude_resource_pool_obtain_resource
(
  _In_ crude_resource_pool                                *resource_pool
);

CRUDE_API void
crude_resource_pool_release_resource
(
  _In_ crude_resource_pool                                *resource_pool,
  _In_ uint32                                              index
);

CRUDE_API void
crude_resource_pool_free_all_resource
(
  _In_ crude_resource_pool                                *resource_pool
);

CRUDE_API void*
crude_resource_pool_access_resource
(
  _In_ crude_resource_pool                                *resource_pool,
  _In_ uint32                                              index
);

#define CRUDE_RESOURCE_HANDLE_IS_INVALID( handle ) ( ( handle ).index == CRUDE_RESOURCE_INDEX_INVALID )
#define CRUDE_RESOURCE_HANDLE_IS_VALID( handle ) ( ( handle ).index != CRUDE_RESOURCE_INDEX_INVALID )