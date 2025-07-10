#pragma once

#include <core/math.h>
#include <core/ecs.h>
#include <scene/scene.h>

typedef struct crude_transform
{
  XMFLOAT3                                             translation;
  XMFLOAT4                                             rotation;
  XMFLOAT3                                             scale;
} crude_transform;

typedef struct crude_camera
{
  float32                                                  fov_radians;
  float32                                                  near_z;
  float32                                                  far_z;
  float32                                                  aspect_ratio;
} crude_camera;

typedef struct crude_gltf
{
  char                                                    *path;
  char const                                              *resources_path;
} crude_gltf;

typedef struct crude_scene_handle
{
  void                                                    *value;
} crude_scene_handle;

CRUDE_API ECS_COMPONENT_DECLARE( crude_transform );
CRUDE_API ECS_COMPONENT_DECLARE( crude_camera );
CRUDE_API ECS_COMPONENT_DECLARE( crude_scene );
CRUDE_API ECS_COMPONENT_DECLARE( crude_scene_creation );
CRUDE_API ECS_COMPONENT_DECLARE( crude_scene_handle );
CRUDE_API ECS_COMPONENT_DECLARE( crude_gltf );

CRUDE_API XMMATRIX
crude_camera_view_to_clip
(
  _In_ crude_camera const                                 *camera
);

CRUDE_API XMMATRIX
crude_transform_node_to_world
(
  _In_ crude_entity                                        node,
  _In_opt_ crude_transform const                          *transform
);

CRUDE_API XMMATRIX
crude_transform_node_to_parent
(
  _In_ crude_transform const                              *transform
);

CRUDE_ECS_MODULE_IMPORT_DECL( crude_scene_components );