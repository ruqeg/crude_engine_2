#pragma once

#include <engine/core/ecs.h>
#include <engine/core/math.h>
#include <engine/scene/components_serialization.h>
#include <engine/external/game_resources.h>
#include <engine/audio/audio_device_resources.h>

typedef enum crude_enemy_state
{
  CRUDE_ENEMY_STATE_IDLE,
  CRUDE_ENEMY_STATE_DEAD,
  CRUDE_ENEMY_STATE_LOOKING_FOR_PLAYER,
  CRUDE_ENEMY_STATE_FOLLOW_PLAYER,
  CRUDE_ENEMY_STATE_RETURN_TO_SPAWN,
  CRUDE_ENEMY_STATE_STANNED,
} crude_enemy_state;

typedef enum crude_recycle_station_state
{
  CRUDE_RECYCLE_STATION_STATE_EMPTY,
  CRUDE_RECYCLE_STATION_STATE_ACTIVE,
  CRUDE_RECYCLE_STATION_STATE_DONE,
} crude_recycle_station_state;

typedef struct crude_enemy
{
  crude_entity                                             player_look_ray_origin_node;
  float32                                                  moving_speed;
  XMFLOAT3                                                 spawn_node_translation;
  float32                                                  stanned_time_left;
  crude_enemy_state                                        state;
  float32                                                  player_last_visible_time;
  XMFLOAT3                                                 player_last_visible_translation;
  float32                                                  player_last_visible_translation_updated_time;
  float32                                                  target_looking_angle;
  float32                                                  health;
  crude_sound_handle                                       enemy_idle_sound_handle;
  crude_sound_handle                                       enemy_notice_sound_handle;
  crude_sound_handle                                       enemy_attack_sound_handle;
} crude_enemy;

typedef struct crude_serum_station
{
} crude_serum_station;

typedef struct crude_teleport_station
{
} crude_teleport_station;

typedef struct crude_recycle_station
{
  crude_game_item                                          game_item;
  float32                                                  last_recycle_time;
  crude_recycle_station_state                              state;
} crude_recycle_station;

typedef struct crude_weapon
{
  int32                                                    max_ammo;
  int32                                                    ammo;
  crude_entity                                             weapon_basic_node;
  crude_entity                                             weapon_scoped_node;
  crude_entity                                             weapon_shot_node;
  float32                                                  last_shot_timer;
} crude_weapon;

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
  bool                                                     inside_safe_zone;
  float32                                                  safe_zone_volume;
  float32                                                  time_to_reload_scene;
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
  float32                                                  walking_sound_volume;
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

CRUDE_API ECS_COMPONENT_DECLARE( crude_weapon );
CRUDE_PARSE_JSON_TO_COMPONENT_FUNC_DECLARATION( crude_weapon );
CRUDE_PARSE_COMPONENT_TO_JSON_FUNC_DECLARATION( crude_weapon );
CRUDE_PARSE_COMPONENT_TO_IMGUI_FUNC_DECLARATION( crude_weapon );
CRUDE_API CRUDE_COMPONENT_STRING_DECLARE( crude_weapon );

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

CRUDE_API ECS_COMPONENT_DECLARE( crude_teleport_station );
CRUDE_PARSE_JSON_TO_COMPONENT_FUNC_DECLARATION( crude_teleport_station );
CRUDE_PARSE_COMPONENT_TO_JSON_FUNC_DECLARATION( crude_teleport_station );
CRUDE_PARSE_COMPONENT_TO_IMGUI_FUNC_DECLARATION( crude_teleport_station );
CRUDE_API CRUDE_COMPONENT_STRING_DECLARE( crude_teleport_station );

CRUDE_ECS_MODULE_IMPORT_DECL( crude_game_components );