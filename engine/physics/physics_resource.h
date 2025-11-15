#pragma once

#include <core/ecs.h>
#include <core/math.h>
#include <core/octree.h>

typedef uint32 crude_physics_resource_index;

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
} crude_physics_character_body;

typedef struct crude_physics_static_body
{
  crude_entity                                             node;
} crude_physics_static_body;

typedef struct crude_physics_collision_shape
{
  crude_physics_collision_shape_type                       type;
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
      crude_octree                                        *octree;
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

CRUDE_API char const*
crude_physics_collision_shape_get_debug_model_filename
(
  _In_ crude_physics_collision_shape_type                  type
);