#pragma once

#include <core/ecs.h>
#include <core/math.h>
#include <core/resource_pool.h>
#include <physics/physics_resource.h>

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
  _In_ crude_physics_character_body_handle                   handle
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
  _In_ crude_physics_character_body_handle                   handle
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

CRUDE_API float32
crude_physics_cast_ray
(
  _In_ crude_physics                                      *physics,
  _In_ XMVECTOR                                            ray_origin,
  _In_ XMVECTOR                                            ray_direction
);

/* We want to access physics in systems without extra nonsense, maybe rework in the future if it will be broken */
/* btw, first time i use instance in this engine... a bit sad because of this to be honest */
CRUDE_API void
crude_physics_instance_allocate
(
  _In_ crude_allocator_container                           allocator_container
);

CRUDE_API void
crude_physics_instance_deallocate
(
  _In_ crude_allocator_container                           allocator_container
);

CRUDE_API crude_physics*
crude_physics_instance
(
);