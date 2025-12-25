#pragma once

#include <engine/physics/physics.h>
#include <engine/scene/components_serialization.h>

/**********************************************************
 *
 *                 Components
 *
 *********************************************************/
CRUDE_API ECS_COMPONENT_DECLARE( crude_physics_static_body_handle );
CRUDE_API ECS_COMPONENT_DECLARE( crude_physics_character_body_handle );
CRUDE_API ECS_COMPONENT_DECLARE( crude_physics_collision_shape );

CRUDE_API CRUDE_PARSE_JSON_TO_COMPONENT_FUNC_DECLARATION( crude_physics_static_body_handle );
CRUDE_API CRUDE_PARSE_COMPONENT_TO_JSON_FUNC_DECLARATION( crude_physics_static_body_handle );
CRUDE_API CRUDE_PARSE_JSON_TO_COMPONENT_FUNC_DECLARATION( crude_physics_character_body_handle );
CRUDE_API CRUDE_PARSE_COMPONENT_TO_JSON_FUNC_DECLARATION( crude_physics_character_body_handle );
CRUDE_API CRUDE_PARSE_JSON_TO_COMPONENT_FUNC_DECLARATION( crude_physics_collision_shape );
CRUDE_API CRUDE_PARSE_COMPONENT_TO_JSON_FUNC_DECLARATION( crude_physics_collision_shape );

CRUDE_API CRUDE_COMPONENT_STRING_DECLARE( crude_physics_static_body_handle );
CRUDE_API CRUDE_COMPONENT_STRING_DECLARE( crude_physics_character_body_handle );
CRUDE_API CRUDE_COMPONENT_STRING_DECLARE( crude_physics_collision_shape );

CRUDE_PARSE_COMPONENT_TO_IMGUI_FUNC_DECLARATION( crude_physics_static_body_handle );
CRUDE_PARSE_COMPONENT_TO_IMGUI_FUNC_DECLARATION( crude_physics_character_body_handle );
CRUDE_PARSE_COMPONENT_TO_IMGUI_FUNC_DECLARATION( crude_physics_collision_shape );

CRUDE_API void
crude_physics_components_import
(
  _In_ crude_ecs                                          *world
);

/**********************************************************
 *
 *                 System
 *
 *********************************************************/
typedef struct crude_physics_system_context
{
  crude_physics                                           *physics;
} crude_physics_system_context;

CRUDE_API void
crude_physics_system_import
(
  _In_ crude_ecs                                          *world,
  _In_ crude_physics_system_context                       *ctx
);