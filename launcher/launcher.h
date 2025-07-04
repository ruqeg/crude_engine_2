#pragma once

#include <cr/cr.h>

#include <engine.h>
#include <sandbox.h>
#include <paprika.h>

typedef struct crude_launcher
{
  crude_engine                                             engine;
  crude_sandbox                                            sandbox;
  crude_paprika                                            paprika;
  cr_plugin                                                crude_sandbox_cr;
  cr_plugin                                                crude_paprika_cr;
  cr_plugin                                                crude_engine_simulation_cr;
  crude_entity                                             platform_node;

  SDL_Texture                                             *paprika_texture;
  ImGuiContext                                            *imgui_context;
  SDL_Renderer                                            *sdl_renderer;
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