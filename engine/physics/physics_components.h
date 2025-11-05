#pragma once

#include <core/math.h>
#include <scene/scene_components.h>

typedef struct crude_physics_static_body
{
  uint32                                                   static_body_index;
} crude_physics_static_body;

typedef struct crude_physics_dynamic_body
{
  uint32                                                   dynamic_body_index;
} crude_physics_dynamic_body;

/************************************************
 *
 * ECS Components Declaration
 * 
 ***********************************************/
CRUDE_API extern ECS_COMPONENT_DECLARE( crude_physics_static_body );
CRUDE_API extern ECS_COMPONENT_DECLARE( crude_physics_dynamic_body );

CRUDE_API ECS_COMPONENT_DECLARE( crude_physics_static_body );
CRUDE_API ECS_COMPONENT_DECLARE( crude_physics_dynamic_body );

CRUDE_API CRUDE_PARSE_JSON_TO_COMPONENT_FUNC_DECLARATION( crude_physics_static_body );
CRUDE_API CRUDE_PARSE_COMPONENT_TO_JSON_FUNC_DECLARATION( crude_physics_static_body );
CRUDE_API CRUDE_PARSE_JSON_TO_COMPONENT_FUNC_DECLARATION( crude_physics_dynamic_body );
CRUDE_API CRUDE_PARSE_COMPONENT_TO_JSON_FUNC_DECLARATION( crude_physics_dynamic_body );

CRUDE_API CRUDE_COMPONENT_STRING_DECLARE( crude_physics_static_body );
CRUDE_API CRUDE_COMPONENT_STRING_DECLARE( crude_physics_dynamic_body );

/************************************************
 *
 * Functions Declaratin
 * 
 ***********************************************/
CRUDE_ECS_MODULE_IMPORT_DECL( crude_physics_components );
