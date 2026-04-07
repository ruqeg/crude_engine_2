#pragma once

#include <nfd.h>

#include <engine/engine.h>
#include <engine/graphics/scene_renderer.h>
#include <engine/core/ecs.h>
#include <engine/platform/platform.h>
#include <engine/scene/scene_ecs.h>
#include <game/player_controller_ecs.h>
#include <game/weapon_ecs.h>
#include <game/zombie_ecs.h>
#include <game/health_ecs.h>
#include <game/training_area_level_ecs.h>

typedef struct crude_game
{
  crude_engine                                            *engine;

  crude_string_buffer                                      debug_strings_buffer;
  crude_string_buffer                                      debug_constant_strings_buffer;

  /* System Context */
  crude_player_controller_system_context                   player_controller_system_context;
  crude_weapon_system_context                              weapon_system_context;
  crude_zombie_system_context                              zombie_system_context;
  crude_training_area_level_system_context                 training_area_level_system_context;
  crude_health_system_context                              health_system_context;
} crude_game;

CRUDE_API void
crude_game_initialize
(
  _In_ crude_game                                         *game,
  _In_ crude_engine                                       *engine,
  _In_ char const                                         *working_directory
);

CRUDE_API void
crude_game_deinitialize
(
  _In_ crude_game                                         *game
);

CRUDE_API void
crude_game_update
(
  _In_ crude_game                                         *game
);

CRUDE_API void
crude_game_instance_intialize
(
);

CRUDE_API void
crude_game_instance_deintialize
(
);

CRUDE_API crude_game*
crude_game_instance
(
);