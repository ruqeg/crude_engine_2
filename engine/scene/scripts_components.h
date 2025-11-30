#pragma once

#include <engine/core/math.h>
#include <engine/core/ecs.h>
#include <engine/scene/components_serialization.h>

typedef struct crude_free_camera
{
  float32                                                  moving_speed_multiplier;
  float32                                                  rotating_speed_multiplier;
  crude_entity                                             input_node;
  bool                                                     input_enabled;
} crude_free_camera;

CRUDE_API CRUDE_PARSE_JSON_TO_COMPONENT_FUNC_DECLARATION( crude_free_camera );
CRUDE_API CRUDE_PARSE_COMPONENT_TO_JSON_FUNC_DECLARATION( crude_free_camera );
CRUDE_API CRUDE_COMPONENT_STRING_DECLARE( crude_free_camera );

CRUDE_API ECS_COMPONENT_DECLARE( crude_free_camera );

CRUDE_ECS_MODULE_IMPORT_DECL( crude_scripts_components );