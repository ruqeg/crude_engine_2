#pragma once

// !TODO maybe

#include <imgui/imgui.h>
#include <imgui_node_editor.h>
#include <ImGuizmo.h>

#include <engine.h>
#include <scene/scene.h>
#include <graphics/scene_renderer.h>
#include <core/ecs.h>
#include <platform/platform_components.h>

namespace axn = ax::NodeEditor;

typedef struct LinkInfo
{
  axn::LinkId                                         Id;
  axn::PinId                                          InputId;
  axn::PinId                                          OutputId;
} LinkInfo;

typedef struct crude_render_graph_resource
{
  crude_gfx_render_graph_resource_type                     type;
  bool                                                     external;
  char                                                     name[ 512 ];
} crude_render_graph_resource;

typedef struct crude_render_graph_node
{
  crude_render_graph_resource                             *inputs;
  crude_render_graph_resource                             *outputs;
  bool                                                     enabled;
  char                                                     name[ 512 ];
  crude_gfx_render_graph_node_type                         type;
} crude_render_graph_node;

typedef struct crude_dragoninn
{
  bool                                                     working;
  crude_engine                                            *engine;
  crude_entity                                             platform_node;
  ImGuiContext                                            *imgui_context;
  SDL_Renderer                                            *sdl_renderer;
  axn::EditorContext                                      *node_editor_context;
  crude_heap_allocator                                     common_allocator;
  crude_render_graph_node                                 *render_graph_nodes;
  ImVector<LinkInfo>   m_Links;
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