#pragma once

#include <engine/core/ecs.h>
#include <engine/core/math.h>
#include <engine/core/octree.h>
#include <engine/scene/collisions_resources_manager.h>

typedef uint32 crude_physics_resource_index;

typedef void (*crude_physics_collision_callback_fun)( void *ctx );

typedef struct crude_physics_collision_callback_container
{
  crude_physics_collision_callback_fun                     fun;
  void                                                    *ctx;
} crude_physics_collision_callback_container;

typedef enum crude_physics_collision_shape_type
{
  CRUDE_PHYSICS_COLLISION_SHAPE_TYPE_BOX,
  CRUDE_PHYSICS_COLLISION_SHAPE_TYPE_SPHERE,
  CRUDE_PHYSICS_COLLISION_SHAPE_TYPE_MESH
} crude_physics_collision_shape_type;

typedef struct crude_physics_static_body_handle
{
  crude_physics_resource_index                             index;
} crude_physics_static_body_handle;

typedef struct crude_physics_character_body_handle
{
  crude_physics_resource_index                             index;
} crude_physics_character_body_handle;

typedef struct crude_physics_character_body
{
  crude_entity                                             node;
  XMFLOAT3                                                 velocity;
  bool                                                     on_floor;
  uint32                                                   mask;
  crude_physics_collision_callback_container               callback_container;
} crude_physics_character_body;

typedef struct crude_physics_static_body
{
  crude_entity                                             node;
  uint32                                                   layer;
} crude_physics_static_body;

typedef struct crude_physics_collision_shape
{
  crude_physics_collision_shape_type                       type;
  //char const                                              *debug_model_filename;
  union
  {
    struct
    {
      XMFLOAT3                                             half_extent;
    } box;
    struct
    {
      float32                                              radius;
    } sphere;
    struct
    {
      char const                                          *model_filename;
      crude_octree_handle                                  octree_handle;
    } mesh;
  };
} crude_physics_collision_shape;

CRUDE_API char const*
crude_physics_collision_shape_type_to_string
(
  _In_ crude_physics_collision_shape_type                  type
);

CRUDE_API crude_physics_collision_shape_type
crude_physics_collision_shape_string_to_type
(
  _In_ char const*                                         type_str
);

CRUDE_API crude_physics_collision_callback_container
crude_physics_collision_callback_container_empty
(
);

CRUDE_API void
crude_physics_collision_callback_container_fun
(
  _In_ crude_physics_collision_callback_container          container
);