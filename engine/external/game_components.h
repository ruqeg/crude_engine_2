#pragma once

#include <engine/core/ecs.h>
#include <engine/core/math.h>
#include <engine/scene/components_serialization.h>
#include <engine/external/game_resources.h>

typedef struct crude_enemy
{
  float32                                                  moving_speed;
  float32                                                  last_player_hit_timer;
  XMFLOAT3                                                 spawn_node_translation;
  XMFLOAT3                                                 last_player_visible_translation;
  float32                                                  time_near_last_player_visible_translaion;
  float32                                                  looking_angle;
} crude_enemy;

typedef struct crude_serum_station
{
} crude_serum_station;

typedef struct crude_recycle_station
{
  crude_game_item                                          game_item;
} crude_recycle_station;

typedef struct crude_level_01
{
  crude_entity                                             serum_stations_spawn_points_parent_node;
  crude_entity                                             enemies_spawn_points_parent_node;
  bool                                                     editor_camera_controller_enabled;
} crude_level_01;

typedef struct crude_player
{
  bool                                                     stop_updating_gameplay_values;
  bool                                                     stop_updating_visual_values;
  float32                                                  health;
  float32                                                  drug_withdrawal;
  float32                                                  sanity;
  crude_game_item                                          inventory_items[ CRUDE_GAME_PLAYER_ITEMS_MAX_COUNT ];
} crude_player;

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

CRUDE_API ECS_TAG_DECLARE( crude_serum_station_enabled );

CRUDE_API ECS_COMPONENT_DECLARE( crude_serum_station );
CRUDE_PARSE_JSON_TO_COMPONENT_FUNC_DECLARATION( crude_serum_station );
CRUDE_PARSE_COMPONENT_TO_JSON_FUNC_DECLARATION( crude_serum_station );
CRUDE_PARSE_COMPONENT_TO_IMGUI_FUNC_DECLARATION( crude_serum_station );
CRUDE_API CRUDE_COMPONENT_STRING_DECLARE( crude_serum_station );

CRUDE_API ECS_COMPONENT_DECLARE( crude_enemy );
CRUDE_PARSE_JSON_TO_COMPONENT_FUNC_DECLARATION( crude_enemy );
CRUDE_PARSE_COMPONENT_TO_JSON_FUNC_DECLARATION( crude_enemy );
CRUDE_PARSE_COMPONENT_TO_IMGUI_FUNC_DECLARATION( crude_enemy );
CRUDE_API CRUDE_COMPONENT_STRING_DECLARE( crude_enemy );

CRUDE_API ECS_COMPONENT_DECLARE( crude_level_01 );
CRUDE_PARSE_JSON_TO_COMPONENT_FUNC_DECLARATION( crude_level_01 );
CRUDE_PARSE_COMPONENT_TO_JSON_FUNC_DECLARATION( crude_level_01 );
CRUDE_PARSE_COMPONENT_TO_IMGUI_FUNC_DECLARATION( crude_level_01 );
CRUDE_API CRUDE_COMPONENT_STRING_DECLARE( crude_level_01 );

CRUDE_API ECS_COMPONENT_DECLARE( crude_player_controller );
CRUDE_PARSE_JSON_TO_COMPONENT_FUNC_DECLARATION( crude_player_controller );
CRUDE_PARSE_COMPONENT_TO_JSON_FUNC_DECLARATION( crude_player_controller );
CRUDE_PARSE_COMPONENT_TO_IMGUI_FUNC_DECLARATION( crude_player_controller );
CRUDE_API CRUDE_COMPONENT_STRING_DECLARE( crude_player_controller );

CRUDE_API ECS_COMPONENT_DECLARE( crude_player );
CRUDE_PARSE_JSON_TO_COMPONENT_FUNC_DECLARATION( crude_player );
CRUDE_PARSE_COMPONENT_TO_JSON_FUNC_DECLARATION( crude_player );
CRUDE_PARSE_COMPONENT_TO_IMGUI_FUNC_DECLARATION( crude_player );
CRUDE_API CRUDE_COMPONENT_STRING_DECLARE( crude_player );

CRUDE_API ECS_COMPONENT_DECLARE( crude_recycle_station );
CRUDE_PARSE_JSON_TO_COMPONENT_FUNC_DECLARATION( crude_recycle_station );
CRUDE_PARSE_COMPONENT_TO_JSON_FUNC_DECLARATION( crude_recycle_station );
CRUDE_PARSE_COMPONENT_TO_IMGUI_FUNC_DECLARATION( crude_recycle_station );
CRUDE_API CRUDE_COMPONENT_STRING_DECLARE( crude_recycle_station );

CRUDE_ECS_MODULE_IMPORT_DECL( crude_game_components );