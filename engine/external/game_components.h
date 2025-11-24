#pragma once

#include <engine/core/ecs.h>
#include <engine/core/math.h>
#include <engine/scene/components_serialization.h>

typedef struct crude_enemy
{
  float32                                                  moving_speed;
  crude_entity                                             player_node;
} crude_enemy;

typedef struct crude_level_01
{
  crude_entity                                             syringes_spawn_points_parent_node;
  crude_entity                                             enemies_spawn_points_parent_node;
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

  bool                                                     input_enabled;

#if CRUDE_DEVELOP
  bool                                                     fly_mode;
  float32                                                  fly_speed_scale;
#endif
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