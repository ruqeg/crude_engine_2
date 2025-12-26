#pragma once

#if CRUDE_DEVELOP

#include <engine/core/math.h>
#include <engine/core/ecs.h>
#include <engine/scene/components_serialization.h>

/**********************************************************
 *
 *                 Components
 *
 *********************************************************/
typedef struct crude_debug_collision
{
  char const                                              *absolute_filepath;
  bool                                                     visible;
} crude_debug_collision;

typedef struct crude_debug_gltf
{
  char const                                              *absolute_filepath;
  bool                                                     visible;
} crude_debug_gltf;

CRUDE_API ECS_COMPONENT_DECLARE( crude_debug_collision );
CRUDE_API ECS_COMPONENT_DECLARE( crude_debug_gltf );

CRUDE_API CRUDE_COMPONENT_STRING_DECLARE( crude_debug_collision );
CRUDE_API CRUDE_COMPONENT_STRING_DECLARE( crude_debug_gltf );

CRUDE_API CRUDE_PARSE_COMPONENT_TO_IMGUI_FUNC_DECLARATION( crude_debug_collision );
CRUDE_API CRUDE_PARSE_COMPONENT_TO_IMGUI_FUNC_DECLARATION( crude_debug_gltf );

CRUDE_API void
crude_scene_debug_components_import
(
  _In_ crude_ecs                                          *world
);

#endif