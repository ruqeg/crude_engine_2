#pragma once

#include <engine/graphics/asynchronous_loader_manager.h>
#include <engine/graphics/imgui.h>
#include <engine/graphics/scene_renderer.h>
#include <engine/scene/node_manager.h>
#include <engine/audio/audio_device.h>
#include <engine/audio/audio_system.h>
#include <engine/physics/physics.h>
#include <engine/physics/physics_system.h>
#include <engine/platform/platform.h>
#include <engine/core/ecs.h>

typedef struct crude_engine_creation
{
  struct
  {
    char const                                              *title;
    uint64                                                   width;
    uint64                                                   height;
  } window;
} crude_engine_creation;

typedef struct crude_engine
{
  ecs_world_t                                             *world;
  
  /******************************
   *
   * Common
   *
   ******************************/
  crude_task_sheduler                                      task_sheduler;
  bool                                                     running;
  int64                                                    last_update_time;
  
  /******************************
   *
   * Allocators
   *
   ******************************/
  crude_heap_allocator                                     common_allocator;
  crude_heap_allocator                                     resources_allocator;
  crude_heap_allocator                                     cgltf_temporary_allocator;
  crude_stack_allocator                                    temporary_allocator;
  crude_stack_allocator                                    model_renderer_resources_manager_temporary_allocator;

  /******************************
   *
   * Scene
   *
   ******************************/
  crude_node_manager                                       node_manager;
  
  /******************************
   *
   * Window & Input
   *
   ******************************/
  crude_platform                                           platform;
  XMFLOAT2                                                 last_unrelative_mouse_position;
  
  /******************************
   *
   * ImGui
   *
   ******************************/
  ImGuiContext                                            *imgui_context;
  ImFont                                                  *imgui_font;

  /******************************
   *
   * Graphics
   *
   ******************************/
  crude_gfx_device                                         gpu;
  crude_gfx_render_graph                                   render_graph;
  crude_gfx_render_graph_builder                           render_graph_builder;
  crude_gfx_asynchronous_loader                            async_loader;
  crude_gfx_scene_renderer                                 scene_renderer;
  crude_gfx_model_renderer_resources_manager               model_renderer_resources_manager;
  crude_gfx_asynchronous_loader_manager                    asynchronous_loader_manager;
  int64                                                    last_graphics_update_time;
  float64                                                  graphics_absolute_time;
  crude_entity                                             graphics_focused_camera_node;
  uint32                                                   graphics_framerate;

  /******************************
   *
   * Physics & Collisions
   *
   ******************************/
  crude_collisions_resources_manager                       collision_resources_manager;
  crude_physics_resources_manager                          physics_resources_manager;
  crude_physics                                            physics;
  crude_physics_system_context                             physics_system_context;

  /******************************
   *
   * Audio
   *
   ******************************/
  crude_audio_device                                       audio_device;
  crude_audio_system_context                               audio_system_context;
  
  /******************************
   *
   * Develop
   *
   ******************************/
#if CRUDE_DEVELOP
  crude_physics_debug_system_context                       physics_debug_system_context;
#endif

  /******************************
   *
   * Public
   *
   ******************************/
  bool                                                     pub_engine_should_proccess_imgui_input;
  float32                                                  pub_engine_audio_volume;
} crude_engine;

CRUDE_API void
crude_engine_initialize
(
  _In_ crude_engine                                       *engine,
  _In_ crude_engine_creation const                        *creation
);

CRUDE_API void
crude_engine_deinitialize
(
  _In_ crude_engine                                       *engine
);

CRUDE_API bool
crude_engine_update
(
  _In_ crude_engine                                       *engine
);