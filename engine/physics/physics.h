#pragma once

#include <engine/core/ecs.h>
#include <engine/core/math.h>
#include <engine/core/resource_pool.h>
#include <engine/physics/physics_resource.h>

typedef struct crude_physics_raycast_result
{
  crude_raycast_result                                     raycast_result;
  crude_entity                                             node;
} crude_physics_raycast_result;

typedef struct crude_physics_creation
{
  crude_heap_allocator                                    *allocator;
} crude_physics_creation;

typedef struct crude_physics
{
  crude_heap_allocator                                    *allocator;
  crude_physics_character_body_handle                     *character_bodies;
  crude_physics_static_body_handle                        *static_bodies;
  crude_resource_pool                                      character_bodies_resource_pool;
  crude_resource_pool                                      static_bodies_resource_pool;
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
crude_physics_update
(
  _In_ crude_physics                                      *physics,
  _In_ float64                                             delta_time
);

CRUDE_API crude_physics_character_body_handle
crude_physics_create_character_body
(
  _In_ crude_physics                                      *physics,
  _In_ crude_entity                                        node
);

CRUDE_API crude_physics_static_body_handle
crude_physics_create_static_body
(
  _In_ crude_physics                                      *physics,
  _In_ crude_entity                                        node
);

CRUDE_API void
crude_physics_destroy_character_body
(
  _In_ crude_physics                                      *physics,
  _In_ crude_physics_character_body_handle                 handle
);

CRUDE_API void
crude_physics_destroy_static_body
(
  _In_ crude_physics                                      *physics,
  _In_ crude_physics_static_body_handle                    handle
);

CRUDE_API crude_physics_character_body*
crude_physics_access_character_body
(
  _In_ crude_physics                                      *physics,
  _In_ crude_physics_character_body_handle                 handle
);

CRUDE_API crude_physics_static_body*
crude_physics_access_static_body
(
  _In_ crude_physics                                      *physics,
  _In_ crude_physics_static_body_handle                    handle
);

CRUDE_API XMVECTOR
crude_physics_character_body_get_velocity
(
  _In_ crude_physics                                      *physics,
  _In_ crude_physics_character_body_handle                 handle
);

CRUDE_API void
crude_physics_character_body_set_velocity
(
  _In_ crude_physics                                      *physics,
  _In_ crude_physics_character_body_handle                 handle,
  _In_ XMVECTOR                                            velocity
);

CRUDE_API bool
crude_physics_character_body_on_floor
(
  _In_ crude_physics                                      *physics,
  _In_ crude_physics_character_body_handle                 handle
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