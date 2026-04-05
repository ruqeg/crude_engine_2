#pragma once

#include <engine/core/math.h>
#include <engine/core/ecs.h>
#include <engine/scene/scene_config.h>
#include <engine/graphics/model_renderer_resources.h>

typedef enum crude_node_external_type
{
  CRUDE_NODE_EXTERNAL_TYPE_REFERENCE = 0,
  CRUDE_NODE_EXTERNAL_TYPE_COPY = 1,
  CRUDE_NODE_EXTERNAL_TYPE_COUNT = 2
} crude_node_external_type;

typedef struct crude_node_external
{
  char                                                     node_relative_filepath[ CRUDE_NODE_RELATIVE_FILEPATH_LENGTH_MAX ];
  crude_node_external_type                                 type;
} crude_node_external;

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
#if CRUDE_DEVELOP
  int                                                      debug_animation_instance_index;
#endif
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

CRUDE_API crude_gltf
crude_gltf_empty
(
);

CRUDE_API crude_camera
crude_camera_empty
(
);

CRUDE_API crude_node_external
crude_node_external_empty
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

CRUDE_API crude_transform
crude_transform_lerp
(
  _In_ crude_transform                                    *transform1,
  _In_ crude_transform                                    *transform2,
  _In_ float32                                             t
);

CRUDE_API crude_entity
crude_node_copy_hierarchy
(
  _In_ crude_ecs                                          *world,
  _In_ crude_entity                                        node,
  _In_ char const                                         *name,
  _In_ crude_entity                                        parent,
  _In_ bool                                                copy_value,
  _In_ bool                                                enabled
);