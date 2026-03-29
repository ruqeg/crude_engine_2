#pragma once

#include <engine/core/ecs.h>
#include <engine/platform/platform.h>
#include <engine/scene/components_serialization.h>

#include <engine/scene/scene_resources.h>

ecs_entity_t const crude_ecs_on_engine_update = EcsOnUpdate;
ecs_entity_t const crude_ecs_on_game_update = EcsPostUpdate;

/**********************************************************
 *
 *                 Components
 *
 *********************************************************/
CRUDE_API ECS_COMPONENT_DECLARE( crude_transform );
CRUDE_API ECS_COMPONENT_DECLARE( crude_camera );
CRUDE_API ECS_COMPONENT_DECLARE( crude_gltf );
CRUDE_API ECS_COMPONENT_DECLARE( crude_light );
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
CRUDE_API CRUDE_COMPONENT_STRING_DECLARE( crude_node_runtime );
CRUDE_API CRUDE_COMPONENT_STRING_DECLARE( crude_node_external );

CRUDE_API CRUDE_PARSE_COMPONENT_TO_IMGUI_FUNC_DECLARATION( crude_camera );
CRUDE_API CRUDE_PARSE_COMPONENT_TO_IMGUI_FUNC_DECLARATION( crude_transform );
CRUDE_API CRUDE_PARSE_COMPONENT_TO_IMGUI_FUNC_DECLARATION( crude_gltf );
CRUDE_API CRUDE_PARSE_COMPONENT_TO_IMGUI_FUNC_DECLARATION( crude_light );
CRUDE_API CRUDE_PARSE_COMPONENT_TO_IMGUI_FUNC_DECLARATION( crude_node_external );

CRUDE_API void
crude_scene_components_import
(
  _In_ crude_ecs                                          *world,
  _In_ crude_components_serialization_manager             *manager
);