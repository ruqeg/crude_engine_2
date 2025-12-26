#pragma once

#include <engine/engine/environment.h>
#include <engine/graphics/scene_renderer.h>
#include <engine/graphics/asynchronous_loader_manager.h>
#include <engine/scene/scene_thread_manager.h>
#include <engine/graphics/imgui.h>

typedef void (*crude_graphics_thread_manager_imgui_draw_custom_fun)
(
  _In_ void                                               *ctx,
  _In_ crude_ecs                                          *world
);

typedef struct crude_graphics_thread_manager
{
  /* Common */
  bool                                                     running;
  mtx_t                                                    mutex;

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

  crude_graphics_thread_manager_imgui_draw_custom_fun      imgui_draw_custom_fn;
  void                                                    *imgui_draw_custom_ctx;

  /* Ctx */
  crude_gfx_asynchronous_loader_manager                   *___asynchronous_loader_manager;
  crude_scene_thread_manager                              *___scene_thread_manager;
} crude_graphics_thread_manager;

CRUDE_API void
crude_graphics_thread_manager_initialize
(
  _In_ crude_graphics_thread_manager                      *manager,
  _In_ crude_environment const                            *environment,
  _In_ SDL_Window                                         *sdl_window,
  _In_ crude_task_sheduler                                *task_sheduler,
  _In_ crude_gfx_asynchronous_loader_manager              *___asynchronous_loader_manager,
  _In_ crude_scene_thread_manager                         *___scene_thread_manager,
  _In_ ImGuiContext                                       *imgui_context,
  _In_ crude_heap_allocator                               *cgltf_temporary_allocator,
  _In_ crude_stack_allocator                              *model_renderer_resources_manager_temporary_allocator,
  _In_ crude_heap_allocator                               *common_allocator,
  _In_ crude_stack_allocator                              *temporary_allocator
);

CRUDE_API void
crude_graphics_thread_manager_deinitialize
(
  _In_ crude_graphics_thread_manager                      *manager
);

CRUDE_API void
crude_graphics_thread_manager_stop
(
  _In_ crude_graphics_thread_manager                      *manager
);

CRUDE_API void
crude_graphics_thread_manager_set_imgui_custom_draw
(
  _In_ crude_graphics_thread_manager                      *manager,
  _In_ crude_graphics_thread_manager_imgui_draw_custom_fun imgui_draw_custom_fn,
  _In_ void                                               *imgui_draw_custom_ctx
);