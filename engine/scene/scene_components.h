#pragma once

#include <core/math.h>
#include <core/ecs.h>
#include <scene/scene.h>

typedef struct crude_node_external
{
  char const                                              *path;
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
  char                                                    *path;
  char                                                    *original_path;
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
CRUDE_API ECS_COMPONENT_DECLARE( crude_light );
CRUDE_API ECS_COMPONENT_DECLARE( crude_collision_shape );
CRUDE_API ECS_COMPONENT_DECLARE( crude_node_external );

CRUDE_API CRUDE_PARSE_JSON_TO_COMPONENT_FUNC_DECLARATION( crude_camera );
CRUDE_API CRUDE_PARSE_COMPONENT_TO_JSON_FUNC_DECLARATION( crude_camera );
CRUDE_API CRUDE_PARSE_JSON_TO_COMPONENT_FUNC_DECLARATION( crude_transform );
CRUDE_API CRUDE_PARSE_COMPONENT_TO_JSON_FUNC_DECLARATION( crude_transform );
CRUDE_API CRUDE_PARSE_JSON_TO_COMPONENT_FUNC_DECLARATION( crude_gltf );
CRUDE_API CRUDE_PARSE_COMPONENT_TO_JSON_FUNC_DECLARATION( crude_gltf );
CRUDE_API CRUDE_PARSE_JSON_TO_COMPONENT_FUNC_DECLARATION( crude_light );
CRUDE_API CRUDE_PARSE_COMPONENT_TO_JSON_FUNC_DECLARATION( crude_light );

CRUDE_API CRUDE_COMPONENT_STRING_DECLARE( crude_camera );
CRUDE_API CRUDE_COMPONENT_STRING_DECLARE( crude_transform );
CRUDE_API CRUDE_COMPONENT_STRING_DECLARE( crude_gltf );
CRUDE_API CRUDE_COMPONENT_STRING_DECLARE( crude_light );

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
  _In_ crude_entity                                        node,
  _In_opt_ crude_transform const                          *transform
);

CRUDE_API XMMATRIX
crude_transform_node_to_parent
(
  _In_ crude_transform const                              *transform
);

CRUDE_ECS_MODULE_IMPORT_DECL( crude_scene_components );