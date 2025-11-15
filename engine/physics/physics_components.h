#pragma once

#include <physics/physics_resource.h>

/************************************************
 *
 * ECS Components Declaration
 * 
 ***********************************************/
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

/************************************************
 *
 * Functions Declaratin
 * 
 ***********************************************/
CRUDE_ECS_MODULE_IMPORT_DECL( crude_physics_components );