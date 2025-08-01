#pragma once

#include <cr/cr.h>

#include <engine.h>
#include <paprika.h>

typedef struct crude_launcher
{
  crude_engine                                             engine;
  crude_paprika                                            paprika;
  cr_plugin                                                paprika_cr;
  cr_plugin                                                engine_simulation_cr;
  crude_entity                                             platform_node;

  SDL_Texture                                             *paprika_texture;
  SDL_Texture                                             *shaders_button_texture;
  SDL_Texture                                             *dragoninn_texture;

  ImGuiContext                                            *imgui_context;
  SDL_Renderer                                            *sdl_renderer;

  crude_stack_allocator                                    temporary_allocator;
} crude_launcher;

void
crude_launcher_initialize
(
  _In_ crude_launcher                                     *launcher
);

void
crude_launcher_deinitialize
(
  _In_ crude_launcher                                     *launcher
);

void
crude_launcher_update
(
  _In_ crude_launcher                                     *launcher
);