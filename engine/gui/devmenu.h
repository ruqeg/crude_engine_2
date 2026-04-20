#pragma once

#if CRUDE_DEVELOP

#include <engine/graphics/scene_renderer.h>
#include <engine/graphics/gpu_profiler.h>
#include <engine/graphics/imgui.h>

typedef struct crude_engine crude_engine;
typedef struct crude_gui_devmenu crude_gui_devmenu;

typedef struct crude_gui_devmenu_memory_visual_profiler
{
  crude_gui_devmenu                                       *devmenu;
  bool                                                     enabled;
  crude_allocator_container                               *allocators_containers;
} crude_gui_devmenu_memory_visual_profiler;

typedef struct crude_gui_devmenu_render_graph
{
  crude_gui_devmenu                                       *devmenu;
  crude_gfx_render_graph                                   render_graph;
  crude_gfx_render_graph_builder                           render_graph_builder;
  char                                                     technique_absolute_filepath[ 1024 ];
  bool                                                     enabled;
} crude_gui_devmenu_render_graph;

typedef struct crude_gui_devmenu_scene_renderer
{
  crude_gui_devmenu                                       *devmenu;
  bool                                                     debug_probes_statuses;
  bool                                                     debug_probes_radiance;
  bool                                                     enabled;
} crude_gui_devmenu_scene_renderer;

typedef struct crude_gui_devmenu
{
  crude_engine                                            *engine;
  bool                                                     enabled;
  crude_gui_devmenu_memory_visual_profiler                 memory_visual_profiler;
  crude_gui_devmenu_render_graph                           render_graph;
  crude_gui_devmenu_scene_renderer                         scene_renderer;
  uint32                                                   selected_option;
  float32                                                  last_framerate_update_time;
  uint32                                                   previous_framerate;
  uint32                                                   current_framerate;

  crude_stack_allocator                                   *dev_stack_allocator;
  crude_heap_allocator                                    *dev_heap_allocator;
} crude_gui_devmenu;

/***********************
 * 
 * Develop Menu
 * 
 ***********************/
CRUDE_API void
crude_gui_devmenu_initialize
(
  _In_ crude_gui_devmenu                                  *devmenu,
  _In_ crude_engine                                       *engine
);

CRUDE_API void
crude_gui_devmenu_deinitialize
(
  _In_ crude_gui_devmenu                                  *devmenu
);

CRUDE_API void
crude_gui_devmenu_draw
(
  _In_ crude_gui_devmenu                                  *devmenu
);

CRUDE_API void
crude_gui_devmenu_update
(
  _In_ crude_gui_devmenu                                  *devmenu
);

CRUDE_API void
crude_gui_devmenu_handle_input
(
  _In_ crude_gui_devmenu                                  *devmenu
);

/***********************
 * 
 * Common Commmads
 * 
 ***********************/
CRUDE_API void
crude_gui_devmenu_debug_gltf_view_callback
(
  _In_ crude_gui_devmenu                                  *devmenu
);

CRUDE_API bool
crude_gui_devmenu_debug_gltf_view_callback_hotkey_pressed_callback
(
  _In_ crude_input                                        *input
);

CRUDE_API void
crude_gui_devmenu_collisions_view_callback
(
  _In_ crude_gui_devmenu                                  *devmenu
);

CRUDE_API bool
crude_gui_devmenu_collisions_view_callback_hotkey_pressed_callback
(
  _In_ crude_input                                        *input
);

CRUDE_API void
crude_gui_devmenu_free_camera_callback
(
  _In_ crude_gui_devmenu                                  *devmenu
);

CRUDE_API bool
crude_gui_devmenu_free_camera_callback_hotkey_pressed_callback
(
  _In_ crude_input                                        *input
);

CRUDE_API void
crude_gui_devmenu_reload_techniques_callback
(
  _In_ crude_gui_devmenu                                  *devmenu
);

CRUDE_API bool
crude_gui_devmenu_reload_techniques_hotkey_pressed_callback
(
  _In_ crude_input                                        *input
);

/***********************
 * 
 * Develop Memory Visual Profiler
 * 
 ***********************/
CRUDE_API void
crude_gui_devmenu_memory_visual_profiler_initialize
(
  _In_ crude_gui_devmenu_memory_visual_profiler           *dev_mem_profiler,
  _In_ crude_gui_devmenu                                  *devmenu
);

CRUDE_API void
crude_gui_devmenu_memory_visual_profiler_deinitialize
(
  _In_ crude_gui_devmenu_memory_visual_profiler           *dev_mem_profiler
);

CRUDE_API void
crude_gui_devmenu_memory_visual_profiler_update
(
  _In_ crude_gui_devmenu_memory_visual_profiler           *dev_mem_profiler
);

CRUDE_API void
crude_gui_devmenu_memory_visual_profiler_draw
(
  _In_ crude_gui_devmenu_memory_visual_profiler           *dev_mem_profiler
);

CRUDE_API void
crude_gui_devmenu_memory_visual_profiler_callback
(
  _In_ crude_gui_devmenu                                  *devmenu
);

/***********************
 * 
 * Develop Render Graph
 * 
 ***********************/
CRUDE_API void
crude_gui_devmenu_render_graph_initialize
(
  _In_ crude_gui_devmenu_render_graph                     *dev_render_graph,
  _In_ crude_gui_devmenu                                  *devmenu
);

CRUDE_API void
crude_gui_devmenu_render_graph_deinitialize
(
  _In_ crude_gui_devmenu_render_graph                     *dev_render_graph
);

CRUDE_API void
crude_gui_devmenu_render_graph_update
(
  _In_ crude_gui_devmenu_render_graph                     *dev_render_graph
);

CRUDE_API void
crude_gui_devmenu_render_graph_draw
(
  _In_ crude_gui_devmenu_render_graph                     *dev_render_graph
);

CRUDE_API void
crude_gui_devmenu_render_graph_callback
(
  _In_ crude_gui_devmenu                                  *devmenu
);

/***********************
 * 
 * Develop Scene Renderer
 * 
 ***********************/
CRUDE_API void
crude_gui_devmenu_scene_renderer_initialize
(
  _In_ crude_gui_devmenu_scene_renderer                   *dev_scene_rendere,
  _In_ crude_gui_devmenu                                  *devmenu
);

CRUDE_API void
crude_gui_devmenu_scene_renderer_deinitialize
(
  _In_ crude_gui_devmenu_scene_renderer                   *dev_scene_rendere
);

CRUDE_API void
crude_gui_devmenu_scene_renderer_update
(
  _In_ crude_gui_devmenu_scene_renderer                   *dev_scene_rendere
);

CRUDE_API void
crude_gui_devmenu_scene_renderer_draw
(
  _In_ crude_gui_devmenu_scene_renderer                   *dev_scene_rendere
);

CRUDE_API void
crude_gui_devmenu_scene_renderer_callback
(
  _In_ crude_gui_devmenu                                  *devmenu
);

#endif