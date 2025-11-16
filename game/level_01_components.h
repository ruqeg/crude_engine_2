#pragma once

#include <core/ecs.h>
#include <scene/components_serialization.h>

typedef struct game_t game_t;

typedef struct crude_level_01
{
  bool                                                     editor_camera_controller_enabled;
} crude_level_01;

CRUDE_PARSE_JSON_TO_COMPONENT_FUNC_DECLARATION( crude_level_01 );
CRUDE_PARSE_COMPONENT_TO_JSON_FUNC_DECLARATION( crude_level_01 );
CRUDE_PARSE_COMPONENT_TO_IMGUI_FUNC_DECLARATION( crude_level_01 );
CRUDE_API CRUDE_COMPONENT_STRING_DECLARE( crude_level_01 );

CRUDE_API ECS_COMPONENT_DECLARE( crude_level_01 );

CRUDE_ECS_MODULE_IMPORT_DECL( crude_level_01_components );