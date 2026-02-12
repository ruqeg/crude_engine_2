#pragma once

#include <engine/engine/environment.h>
#include <engine/engine/engine_commands_manager.h>
#include <engine/graphics/asynchronous_loader_manager.h>
#include <engine/scene/node_manager.h>
#include <engine/audio/audio_device.h>
#include <engine/audio/audio_ecs.h>
#include <engine/physics/physics.h>
#include <engine/physics/physics_ecs.h>
#include <engine/platform/platform.h>
#include <engine/gui/devmenu.h>
#include <engine/gui/editor.h>
#include <engine/graphics/scene_renderer.h>
#include <engine/graphics/imgui.h>

typedef void (*crude_engine_imgui_draw_custom_fun)
(
  _In_ void                                               *ctx
);

typedef struct crude_engine
{
  
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
  crude_stack_allocator                                    temporary_allocator;
  crude_heap_allocator                                     cgltf_temporary_allocator;
  crude_stack_allocator                                    model_renderer_resources_manager_temporary_allocator;
#if CRUDE_DEVELOP
  crude_stack_allocator                                    develop_temporary_allocator;
  crude_heap_allocator                                     develop_heap_allocator;
#endif

  /******************************
   *
   * Scene
   *
   ******************************/
  crude_node_manager                                       node_manager;
  crude_engine_commands_manager                            commands_manager;
  
  /******************************
   *
   * ECS
   *
   ******************************/
  crude_ecs                                               *world;

  crude_entity                                             main_node;
  crude_entity                                             camera_node;
  crude_entity                                             player_controller_node;

  crude_components_serialization_manager                   components_serialization_manager;

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
  crude_gfx_asynchronous_loader_manager                    asynchronous_loader_manager;

  crude_task_set_handle                                    graphics_task_set_handle;

  crude_gfx_device                                         gpu;
  crude_gfx_render_graph                                   render_graph;
  crude_gfx_render_graph_builder                           render_graph_builder;
  crude_gfx_asynchronous_loader                            async_loader;
  crude_gfx_scene_renderer                                 scene_renderer;
  crude_gfx_model_renderer_resources_manager               model_renderer_resources_manager;
  int64                                                    last_graphics_update_time;
  uint32                                                   framerate;
  float32                                                  graphics_absolute_time;

  crude_engine_imgui_draw_custom_fun                       imgui_draw_custom_fn;
  void                                                    *imgui_draw_custom_ctx;

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
  crude_gui_devmenu                                        devmenu;
  crude_gui_editor                                         editor;
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