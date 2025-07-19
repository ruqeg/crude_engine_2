#pragma once

#include <imgui/imgui.h>
#include <ImGuizmo.h>

#include <engine.h>
#include <scene/scene.h>
#include <graphics/scene_renderer.h>
#include <core/ecs.h>
#include <platform/platform_components.h>


typedef struct crude_dragoninn
{
  bool                                                     working;
  crude_engine                                            *engine;
  crude_entity                                             platform_node;
  ImGuiContext                                            *imgui_context;
  SDL_Renderer                                            *sdl_renderer;
} crude_dragoninn;

CRUDE_API void
crude_dragoninn_initialize
(
  _In_ crude_dragoninn                                    *dragoninn,
  _In_ crude_engine                                       *engine
);

CRUDE_API void
crude_dragoninn_deinitialize
(
  _In_ crude_dragoninn                                    *dragoninn
);

CRUDE_API void
crude_dragoninn_update
(
  _In_ crude_dragoninn                                    *dragoninn
);