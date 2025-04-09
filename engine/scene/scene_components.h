#pragma once

#include <flecs.h>

#include <core/math.h>
#include <scene/entity.h>

typedef struct crude_transform
{
  crude_float3                 translation;
  crude_float4                 rotation;
  crude_float3                 scale;
} crude_transform;

typedef struct crude_camera
{
  float32                      fov_radians;
  float32                      near_z;
  float32                      far_z;
  float32                      aspect_ratio;
} crude_camera;

CRUDE_API ECS_COMPONENT_DECLARE( crude_transform );
CRUDE_API ECS_COMPONENT_DECLARE( crude_camera );

CRUDE_API crude_matrix
crude_camera_view_to_clip
(
  _In_ crude_camera           *camera
);

CRUDE_API crude_matrix
crude_transform_node_to_world
(
  _In_ crude_entity            node,
  _In_ crude_transform        *transform
);

CRUDE_API void
crude_scene_componentsImport
(
  _In_ ecs_world_t            *world
);