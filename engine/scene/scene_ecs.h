#pragma once

#include <engine/core/ecs.h>
#include <engine/platform/platform.h>
#include <engine/scene/components_serialization.h>

#include <engine/scene/scene_resources.h>

ecs_entity_t const crude_ecs_on_pre_physics_update = EcsPostLoad;
ecs_entity_t const crude_ecs_on_engine_update = EcsPreUpdate;
ecs_entity_t const crude_ecs_on_game_update = EcsOnUpdate;
ecs_entity_t const crude_ecs_on_editor_update = EcsPostUpdate;
ecs_entity_t const crude_ecs_on_post_physics_update = EcsPreStore;

#define CRUDE_ECS_GAME_STAGE_ENABLE( world, value ) crude_entity_enable( world, crude_ecs_on_game_update, value )
#define CRUDE_ECS_EDITOR_STAGE_ENABLE( world, value ) crude_entity_enable( world, crude_ecs_on_editor_update, value )
#define CRUDE_ECS_GAME_STAGE_IS_ENABLED( world ) crude_entity_is_enable( world, crude_ecs_on_game_update )
#define CRUDE_ECS_EDITOR_STAGE_IS_ENABLED( world ) crude_entity_is_enable( world, crude_ecs_on_editor_update )

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
CRUDE_API ECS_COMPONENT_DECLARE( crude_ray );

CRUDE_API CRUDE_PARSE_JSON_TO_COMPONENT_FUNC_DECLARATION( crude_camera );
CRUDE_API CRUDE_PARSE_COMPONENT_TO_JSON_FUNC_DECLARATION( crude_camera );
CRUDE_API CRUDE_PARSE_JSON_TO_COMPONENT_FUNC_DECLARATION( crude_transform );
CRUDE_API CRUDE_PARSE_COMPONENT_TO_JSON_FUNC_DECLARATION( crude_transform );
CRUDE_API CRUDE_PARSE_JSON_TO_COMPONENT_FUNC_DECLARATION( crude_gltf );
CRUDE_API CRUDE_PARSE_COMPONENT_TO_JSON_FUNC_DECLARATION( crude_gltf );
CRUDE_API CRUDE_PARSE_JSON_TO_COMPONENT_FUNC_DECLARATION( crude_light );
CRUDE_API CRUDE_PARSE_COMPONENT_TO_JSON_FUNC_DECLARATION( crude_light );
CRUDE_API CRUDE_PARSE_JSON_TO_COMPONENT_FUNC_DECLARATION( crude_ray );
CRUDE_API CRUDE_PARSE_COMPONENT_TO_JSON_FUNC_DECLARATION( crude_ray );

CRUDE_API CRUDE_COMPONENT_STRING_DECLARE( crude_camera );
CRUDE_API CRUDE_COMPONENT_STRING_DECLARE( crude_transform );
CRUDE_API CRUDE_COMPONENT_STRING_DECLARE( crude_gltf );
CRUDE_API CRUDE_COMPONENT_STRING_DECLARE( crude_light );
CRUDE_API CRUDE_COMPONENT_STRING_DECLARE( crude_node_external );
CRUDE_API CRUDE_COMPONENT_STRING_DECLARE( crude_ray );

CRUDE_API CRUDE_PARSE_COMPONENT_TO_IMGUI_FUNC_DECLARATION( crude_camera );
CRUDE_API CRUDE_PARSE_COMPONENT_TO_IMGUI_FUNC_DECLARATION( crude_transform );
CRUDE_API CRUDE_PARSE_COMPONENT_TO_IMGUI_FUNC_DECLARATION( crude_gltf );
CRUDE_API CRUDE_PARSE_COMPONENT_TO_IMGUI_FUNC_DECLARATION( crude_light );
CRUDE_API CRUDE_PARSE_COMPONENT_TO_IMGUI_FUNC_DECLARATION( crude_node_external );
CRUDE_API CRUDE_PARSE_COMPONENT_TO_IMGUI_FUNC_DECLARATION( crude_ray );

CRUDE_API void
crude_scene_components_import
(
  _In_ crude_ecs                                          *world,
  _In_ crude_components_serialization_manager             *manager
);