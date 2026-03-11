#pragma once

#include <engine/core/math.h>
#include <engine/core/ecs.h>
#include <engine/scene/scene_config.h>
#include <engine/graphics/model_renderer_resources.h>

typedef struct crude_node_external
{
  char                                                     node_relative_filepath[ CRUDE_NODE_RELATIVE_FILEPATH_LENGTH_MAX ];
} crude_node_external;

typedef struct crude_node_runtime
{
} crude_node_runtime;

typedef struct crude_transform
{
  XMFLOAT3                                                 translation;
  XMFLOAT4                                                 rotation;
  XMFLOAT3                                                 scale;
} crude_transform;

typedef struct crude_camera
{
  float32                                                  fov_radians;
  float32                                                  near_z;
  float32                                                  far_z;
  float32                                                  aspect_ratio;
} crude_camera;

typedef struct crude_light
{
  float32                                                  radius;
  XMFLOAT3                                                 color;
  float32                                                  intensity;
} crude_light;

typedef struct crude_gltf
{
  crude_gfx_model_renderer_resources_instance              model_renderer_resources_instance;
  bool                                                     hidden;
} crude_gltf;


CRUDE_API XMMATRIX
crude_camera_view_to_clip
(
  _In_ crude_camera const                                 *camera
);

CRUDE_API crude_transform
crude_transform_empty
(
);

CRUDE_API XMMATRIX
crude_transform_node_to_world
(
  _In_ crude_ecs                                          *world,
  _In_ crude_entity                                        node,
  _In_opt_ crude_transform const                          *transform
);

CRUDE_API XMMATRIX
crude_transform_node_to_parent
(
  _In_ crude_transform const                              *transform
);

CRUDE_API XMMATRIX
crude_transform_parent_to_world
(
  _In_ crude_ecs                                          *world,
  _In_ crude_entity                                        node
);