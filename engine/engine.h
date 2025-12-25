#pragma once

#include <engine/engine/environment.h>
#include <engine/engine/engine_commands_manager.h>
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

typedef struct crude_platform_thread_data
{
  crude_platform                                           platform;
  mtx_t                                                    mutex;
  bool                                                     running;
} crude_platform_thread_data;

typedef struct crude_graphics_thread_data
{
  /* Common */
  bool                                                     running;

  /* Graphics */
  crude_gfx_device                                         gpu;
  crude_gfx_render_graph                                   render_graph;
  crude_gfx_render_graph_builder                           render_graph_builder;
  crude_gfx_asynchronous_loader                            async_loader;
  crude_gfx_scene_renderer                                 scene_renderer;
  crude_gfx_model_renderer_resources_manager               model_renderer_resources_manager;
  int64                                                    last_graphics_update_time;
  float64                                                  absolute_time;
  uint32                                                   framerate;
  ImGuiContext                                            *imgui_context;

  /* ecs */
  crude_entity                                             focused_camera_node;
  crude_entity                                             main_node;
  mtx_t                                                    ecs_mutex;
} crude_graphics_thread_data;

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
  crude_environment                                        environment;

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
  crude_engine_commands_manager                            commands_manager;
  mtx_t                                                    ecs_mutex;
  
  /******************************
   *
   * Window & Input
   *
   ******************************/
  crude_platform_thread_data                               platform_thread_data;
  crude_platform                                           platform_copy;
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
  crude_gfx_asynchronous_loader_manager                    asynchronous_loader_manager;
  crude_graphics_thread_data                               graphics_thread_data;

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
  //crude_physics_debug_system_context                       physics_debug_system_context;
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
  _In_ char const                                         *working_directory
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