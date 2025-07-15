#pragma once

#include <imgui/imgui.h>
#include <ImGuizmo.h>

#include <engine.h>
#include <scene/scene.h>
#include <graphics/scene_renderer.h>
#include <core/ecs.h>
#include <platform/platform_components.h>

typedef struct crude_paprika_graphics
{
  crude_gfx_device                                         gpu;
  crude_gfx_renderer                                       renderer;
  crude_gfx_render_graph                                   render_graph;
  crude_gfx_render_graph_builder                           render_graph_builder;
  crude_gfx_asynchronous_loader                            async_loader;
  crude_gfx_scene_renderer                                 scene_renderer;
  crude_allocator_container                                allocator_container;
  crude_gfx_asynchronous_loader_manager                   *asynchronous_loader_manager;
} crude_paprika_graphics;

typedef struct crude_paprika
{
  crude_engine                                            *engine;
  crude_heap_allocator                                     graphics_allocator;
  crude_stack_allocator                                    temporary_allocator;
  crude_scene                                              scene;
  crude_paprika_graphics                                   graphics;
  crude_entity                                             platform_node;
  void                                                    *imgui_context;
  bool                                                     working;
  crude_mouse_input                                        wrapwnd;
  
  uint32                                                   viewport_bindless_texture;

  uint32                                                   selected_node_index;
  crude_entity                                             selected_node;
  ImGuizmo::OPERATION                                      gizmo_operation;
  ImGuizmo::MODE                                           gizmo_mode;

  bool                                                     debug_camera_culling;
  bool                                                     debug_camera_view;
} crude_paprika;

CRUDE_API void
crude_paprika_initialize
(
  _In_ crude_paprika                                      *paprika,
  _In_ crude_engine                                       *engine
);

CRUDE_API void
crude_paprika_deinitialize
(
  _In_ crude_paprika                                      *paprika
);

CRUDE_API void
crude_paprika_update
(
  _In_ crude_paprika                                      *paprika
);