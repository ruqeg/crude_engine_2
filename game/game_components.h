#pragma once

#include <core/ecs.h>
#include <core/math.h>
#include <scene/components_serialization.h>

typedef struct crude_enemy
{
  float32                                                  moving_speed;
  crude_entity                                             player_node;
} crude_enemy;

typedef struct crude_level_01
{
  bool                                                     editor_camera_controller_enabled;
} crude_level_01;

typedef struct crude_player_controller
{
  float32                                                  weight;
  float32                                                  rotation_speed;
  float32                                                  walk_speed;
  float32                                                  run_speed;
  float32                                                  move_change_coeff;
  float32                                                  stop_change_coeff;
  float32                                                  jump_velocity;
  
  float32                                                  withdrawal;

  bool                                                     input_enabled;
} crude_player_controller;


CRUDE_PARSE_JSON_TO_COMPONENT_FUNC_DECLARATION( crude_enemy );
CRUDE_PARSE_COMPONENT_TO_JSON_FUNC_DECLARATION( crude_enemy );
CRUDE_PARSE_COMPONENT_TO_IMGUI_FUNC_DECLARATION( crude_enemy );
CRUDE_API CRUDE_COMPONENT_STRING_DECLARE( crude_enemy );

CRUDE_PARSE_JSON_TO_COMPONENT_FUNC_DECLARATION( crude_level_01 );
CRUDE_PARSE_COMPONENT_TO_JSON_FUNC_DECLARATION( crude_level_01 );
CRUDE_PARSE_COMPONENT_TO_IMGUI_FUNC_DECLARATION( crude_level_01 );
CRUDE_API CRUDE_COMPONENT_STRING_DECLARE( crude_level_01 );

CRUDE_PARSE_JSON_TO_COMPONENT_FUNC_DECLARATION( crude_player_controller );
CRUDE_PARSE_COMPONENT_TO_JSON_FUNC_DECLARATION( crude_player_controller );
CRUDE_PARSE_COMPONENT_TO_IMGUI_FUNC_DECLARATION( crude_player_controller );
CRUDE_API CRUDE_COMPONENT_STRING_DECLARE( crude_player_controller );


CRUDE_API ECS_COMPONENT_DECLARE( crude_enemy );
CRUDE_API ECS_COMPONENT_DECLARE( crude_level_01 );
CRUDE_API ECS_COMPONENT_DECLARE( crude_player_controller );

CRUDE_ECS_MODULE_IMPORT_DECL( crude_game_components );