#pragma once

#if CRUDE_DEVELOP

#include <engine/core/ecs.h>
#include <engine/core/string.h>
#include <engine/graphics/model_renderer_resources_manager.h>

/**********************************************************
 *
 *                 System
 *
 *********************************************************/
typedef struct crude_game_debug_system_context
{
  char const                                              *syringe_serum_station_active_model_absolute_filepath;
  char const                                              *syringe_spawnpoint_model_absolute_filepath;
  char const                                              *enemy_spawnpoint_model_absolute_filepath;
} crude_game_debug_system_context;

CRUDE_API void
crude_game_debug_system_import
(
  _In_ crude_ecs                                          *world,
  _In_ crude_game_debug_system_context                    *ctx
);

#endif
