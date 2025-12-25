#pragma once

#include <engine/physics/physics_resources_manager.h>

typedef struct crude_physics_raycast_result
{
  crude_raycast_result                                     raycast_result;
  crude_entity                                             node;
  uint64                                                   body_layer;
} crude_physics_raycast_result;

typedef struct crude_physics_creation
{
  crude_ecs                                               *world;
  crude_physics_resources_manager                         *manager;
  crude_collisions_resources_manager                      *collision_manager;
} crude_physics_creation;

typedef struct crude_physics
{
  crude_ecs                                               *world;
  ecs_query_t                                             *static_body_handle_query;
  crude_physics_resources_manager                         *manager;
  crude_collisions_resources_manager                      *collision_manager;
  bool                                                     simulation_enabled;
} crude_physics;

CRUDE_API void
crude_physics_initialize
(
  _In_ crude_physics                                      *physics,
  _In_ crude_physics_creation const                       *creation
);

CRUDE_API void
crude_physics_deinitialize
(
  _In_ crude_physics                                      *physics
);

CRUDE_API void
crude_physics_enable_simulation
(
  _In_ crude_physics                                      *physics,
  _In_ bool                                                enable
);

CRUDE_API bool
crude_physics_cast_ray
(
  _In_ crude_physics                                      *physics,
  _In_ XMVECTOR                                            ray_origin,
  _In_ XMVECTOR                                            ray_direction,
  _In_ uint32                                              mask,
  _Out_opt_ crude_physics_raycast_result                  *result
);