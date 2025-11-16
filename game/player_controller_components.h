#pragma once

#include <core/ecs.h>
#include <core/math.h>
#include <scene/components_serialization.h>

typedef struct crude_player_controller
{
  float32                                                  weight;
  float32                                                  rotation_speed;
  float32                                                  walk_speed;
  float32                                                  run_speed;
  float32                                                  move_change_coeff;
  float32                                                  stop_change_coeff;
  float32                                                  jump_velocity;

  bool                                                     _input_enabled;
} crude_player_controller;

CRUDE_PARSE_JSON_TO_COMPONENT_FUNC_DECLARATION( crude_player_controller );
CRUDE_PARSE_COMPONENT_TO_JSON_FUNC_DECLARATION( crude_player_controller );
CRUDE_PARSE_COMPONENT_TO_IMGUI_FUNC_DECLARATION( crude_player_controller );
CRUDE_API CRUDE_COMPONENT_STRING_DECLARE( crude_player_controller );

CRUDE_API ECS_COMPONENT_DECLARE( crude_player_controller );

CRUDE_ECS_MODULE_IMPORT_DECL( crude_player_controller_components );