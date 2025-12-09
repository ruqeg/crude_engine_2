#pragma once

#include <engine/graphics/scene_renderer.h>
#include <engine/graphics/gpu_profiler.h>
#include <engine/platform/platform_components.h>

typedef struct game_t game_t;

typedef struct crude_game_menu
{
  bool                                                     enabled;
} crude_game_menu;

/***********************
 * 
 * Game Menu
 * 
 ***********************/
CRUDE_API void
crude_game_menu_initialize
(
  _In_ crude_game_menu                                    *menu
);

CRUDE_API void
crude_game_menu_deinitialize
(
  _In_ crude_game_menu                                    *menu
);

CRUDE_API void
crude_game_menu_draw
(
  _In_ crude_game_menu                                    *menu
);

CRUDE_API void
crude_game_menu_update
(
  _In_ crude_game_menu                                    *menu
);

CRUDE_API void
crude_game_menu_handle_input
(
  _In_ crude_game_menu                                    *menu,
  _In_ crude_input                                        *input
);