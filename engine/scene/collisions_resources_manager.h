#pragma once

#include <engine/core/ecs.h>
#include <engine/core/resource_pool.h>
#include <engine/core/octree.h>
#include <engine/core/memory.h>
#include <engine/core/string.h>

typedef uint32 crude_octree_resource_index;

typedef struct crude_octree_handle
{
  crude_octree_resource_index                              index;
} crude_octree_handle;

typedef struct crude_collisions_resources_manager
{
  crude_heap_allocator                                    *allocator;
  crude_heap_allocator                                    *cgltf_temporary_allocator;
  crude_resource_pool                                      octree_resource_pool;
  struct { uint64 key; crude_octree_handle value; }       *name_hashed_to_octree_resource_hadle;
} crude_collisions_resources_manager;

CRUDE_API void
crude_collisions_resources_manager_initialize
(
  _In_ crude_collisions_resources_manager                 *manager,
  _In_ crude_heap_allocator                               *allocator,
  _In_ crude_heap_allocator                               *cgltf_temporary_allocator
);

CRUDE_API void
crude_collisions_resources_manager_deinitialize
(
  _In_ crude_collisions_resources_manager                 *manager
);

CRUDE_API crude_octree_handle
crude_collisions_resources_manager_get_octree_handle
(
  _In_ crude_collisions_resources_manager                 *manager,
  _In_ char const                                         *filepath
);

crude_octree*
crude_collisions_resources_manager_access_octree
(
  _In_ crude_collisions_resources_manager                 *manager,
  _In_ crude_octree_handle                                 handle
);

CRUDE_API void
crude_collisions_resources_manager_instance_allocate
(
  _In_ crude_allocator_container                           allocator_container
);

CRUDE_API void
crude_collisions_resources_manager_instance_deallocate
(
  _In_ crude_allocator_container                           allocator_container
);

CRUDE_API crude_collisions_resources_manager*
crude_collisions_resources_manager_instance
(
);