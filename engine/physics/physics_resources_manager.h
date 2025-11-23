#pragma once

#include <engine/core/ecs.h>
#include <engine/core/math.h>
#include <engine/core/resource_pool.h>
#include <engine/physics/physics_resource.h>

typedef struct crude_physics_resources_manager_creation
{
  crude_heap_allocator                                    *allocator;
} crude_physics_resources_manager_creation;

typedef struct crude_physics_resources_manager
{
  crude_heap_allocator                                    *allocator;
  crude_physics_character_body_handle                     *character_bodies;
  crude_physics_static_body_handle                        *static_bodies;
  crude_resource_pool                                      character_bodies_resource_pool;
  crude_resource_pool                                      static_bodies_resource_pool;
  bool                                                     simulation_enabled;
} crude_physics_resources_manager;

CRUDE_API void
crude_physics_resources_manager_initialize
(
  _In_ crude_physics_resources_manager                    *manager,
  _In_ crude_physics_resources_manager_creation const     *creation
);

CRUDE_API void
crude_physics_resources_manager_deinitialize
(
  _In_ crude_physics_resources_manager                    *manager
);

CRUDE_API crude_physics_character_body_handle
crude_physics_resources_manager_create_character_body
(
  _In_ crude_physics_resources_manager                    *manager,
  _In_ crude_entity                                        node
);

CRUDE_API crude_physics_static_body_handle
crude_physics_resources_manager_create_static_body
(
  _In_ crude_physics_resources_manager                    *manager,
  _In_ crude_entity                                        node
);

CRUDE_API void
crude_physics_resources_manager_destroy_character_body
(
  _In_ crude_physics_resources_manager                    *manager,
  _In_ crude_physics_character_body_handle                 handle
);

CRUDE_API void
crude_physics_resources_manager_destroy_static_body
(
  _In_ crude_physics_resources_manager                    *manager,
  _In_ crude_physics_static_body_handle                    handle
);

CRUDE_API crude_physics_character_body*
crude_physics_resources_manager_access_character_body
(
  _In_ crude_physics_resources_manager                    *manager,
  _In_ crude_physics_character_body_handle                 handle
);

CRUDE_API crude_physics_static_body*
crude_physics_resources_manager_access_static_body
(
  _In_ crude_physics_resources_manager                    *manager,
  _In_ crude_physics_static_body_handle                    handle
);