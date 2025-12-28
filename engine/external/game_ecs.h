#pragma once

#include <engine/core/ecs.h>
#include <engine/core/math.h>
#include <engine/scene/components_serialization.h>
#include <engine/external/game_resources.h>
#include <engine/audio/audio_device_resources.h>

/**********************************************************
 *
 *                 Components
 *
 *********************************************************/
typedef struct crude_level_mars
{
} crude_level_mars;

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

CRUDE_API ECS_COMPONENT_DECLARE( crude_player_controller );
CRUDE_API CRUDE_PARSE_JSON_TO_COMPONENT_FUNC_DECLARATION( crude_player_controller );
CRUDE_API CRUDE_PARSE_COMPONENT_TO_JSON_FUNC_DECLARATION( crude_player_controller );
CRUDE_API CRUDE_PARSE_COMPONENT_TO_IMGUI_FUNC_DECLARATION( crude_player_controller );
CRUDE_API CRUDE_COMPONENT_STRING_DECLARE( crude_player_controller );

CRUDE_API ECS_COMPONENT_DECLARE( crude_level_mars );
CRUDE_API CRUDE_PARSE_JSON_TO_COMPONENT_FUNC_DECLARATION( crude_level_mars );
CRUDE_API CRUDE_PARSE_COMPONENT_TO_JSON_FUNC_DECLARATION( crude_level_mars );
CRUDE_API CRUDE_PARSE_COMPONENT_TO_IMGUI_FUNC_DECLARATION( crude_level_mars );
CRUDE_API CRUDE_COMPONENT_STRING_DECLARE( crude_level_mars );

CRUDE_API void
crude_game_components_import
(
  _In_ crude_ecs                                          *world
);