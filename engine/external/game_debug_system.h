#pragma once

#if CRUDE_DEVELOP

#include <engine/core/ecs.h>
#include <engine/core/string.h>

typedef struct crude_game_debug_system_context
{
  crude_string_buffer                                     *constant_string_bufffer;
  char const                                              *resources_absolute_directory;
  char const                                              *syringe_spawnpoint_model_absolute_filepath;
  char const                                              *enemy_spawnpoint_model_absolute_filepath;
} crude_game_debug_system_context;

CRUDE_API void
crude_game_debug_system_import
(
  _In_ ecs_world_t                                        *world,
  _In_ crude_game_debug_system_context                    *ctx
);

#endif
