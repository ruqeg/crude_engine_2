#pragma once

#include <core/math.h>
#include <core/ecs.h>
#include <scene/scene.h>

typedef struct crude_transform
{
  crude_float3                                             translation;
  crude_float4                                             rotation;
  crude_float3                                             scale;
} crude_transform;

typedef struct crude_camera
{
  float32                                                  fov_radians;
  float32                                                  near_z;
  float32                                                  far_z;
  float32                                                  aspect_ratio;
} crude_camera;

typedef struct crude_scene_handle
{
  void                                                    *value;
} crude_scene_handle;

CRUDE_API CRUDE_ECS_COMPONENT_DECLARE( crude_transform );
CRUDE_API CRUDE_ECS_COMPONENT_DECLARE( crude_camera );
CRUDE_API CRUDE_ECS_COMPONENT_DECLARE( crude_scene );
CRUDE_API CRUDE_ECS_COMPONENT_DECLARE( crude_scene_creation );
CRUDE_API CRUDE_ECS_COMPONENT_DECLARE( crude_scene_handle );

CRUDE_API crude_matrix
crude_camera_view_to_clip
(
  _In_ crude_camera                                       *camera
);

CRUDE_API crude_matrix
crude_transform_node_to_world
(
  _In_ crude_entity                                        node,
  _In_ crude_transform                                    *transform
);

CRUDE_ECS_MODULE_IMPORT_DECL( crude_scene_components );