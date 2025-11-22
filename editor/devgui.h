#pragma once

#include <imgui.h>

#include <core/ecs.h>
#include <platform/platform_components.h>
#include <graphics/scene_renderer.h>
#include <graphics/gpu_profiler.h>

typedef struct crude_editor crude_editor;
typedef struct crude_devgui crude_devgui;

typedef struct crude_devgui_added_node_data
{
  char                                                     buffer[ 128 ];
} crude_devgui_added_node_data;

typedef struct crude_devgui_nodes_tree
{
  bool                                                     enabled;
  crude_entity                                             selected_node;
  crude_devgui                                            *devgui;
} crude_devgui_nodes_tree;

typedef struct crude_devgui_node_inspector
{
  bool                                                     enabled;
} crude_devgui_node_inspector;

typedef struct crude_devgui_viewport
{
  crude_gfx_device                                        *gpu;
  crude_devgui_nodes_tree                                 *devgui_nodes_tree;
  crude_gfx_texture_handle                                 selected_texture;
} crude_devgui_scene_node_viewport;

typedef struct crude_devgui_render_graph
{
  crude_gfx_render_graph                                  *render_graph;
  bool                                                     enabled;
} crude_devgui_render_graph;

typedef struct crude_devgui_gpu
{
  crude_gfx_device                                        *gpu;
  crude_stack_allocator                                   *temporary_allocator;
  bool                                                     enabled;
} crude_devgui_gpu;

typedef struct crude_devgui_gpu_visual_profiler
{
  crude_gfx_device                                        *gpu;
  float32                                                  max_duration;
  uint32                                                   max_frames;
  uint32                                                   max_queries_per_frame;
  uint32                                                   current_frame;
  int32                                                    max_visible_depth;
  float32                                                  average_time;
  float32                                                  max_time;
  float32                                                  min_time;
  float32                                                  new_average;
  uint32                                                   framebuffer_pixel_count;
  uint16                                                  *per_frame_active;
  crude_gfx_gpu_time_query                                *timestamps;
  crude_gfx_gpu_pipeline_statistics                       *pipeline_statistics;
  crude_heap_allocator                                    *allocator;
  struct { uint64 key; uint32 value; }                    *name_hashed_to_color_index;
  uint32                                                   initial_frames_paused;
  bool                                                     paused;
  bool                                                     enabled;
} crude_devgui_gpu_visual_profiler;

typedef struct crude_devgui_scene_renderer
{
  crude_gfx_scene_renderer                                *scene_renderer;
  bool                                                     debug_probes_statuses;
  bool                                                     debug_probes_radiance;
  bool                                                     enabled;
} crude_devgui_scene_renderer;

typedef struct crude_devgui
{
  char const                                              *last_focused_menutab_name;
  crude_editor                                            *editor;
  bool                                                     menubar_enabled;
  crude_devgui_nodes_tree                                  dev_nodes_tree;
  crude_devgui_node_inspector                              dev_node_inspector;
  crude_devgui_viewport                                    dev_viewport;
  crude_devgui_render_graph                                dev_render_graph;
  crude_devgui_gpu                                         dev_gpu;
  crude_devgui_gpu_visual_profiler                         dev_gpu_profiler;
  crude_devgui_scene_renderer                              dev_scene_renderer;
  crude_devgui_added_node_data                             added_node_data;
  crude_entity                                             node_to_add;
  crude_entity                                             node_to_remove;
} crude_devgui;

/*********
 * Dev Gui
 *************/
CRUDE_API void
crude_devgui_initialize
(
  _In_ crude_devgui                                       *devgui,
  _In_ crude_editor                                       *editor
);

CRUDE_API void
crude_devgui_deinitialize
(
  _In_ crude_devgui                                       *devgui
);

CRUDE_API void
crude_devgui_draw
(
  _In_ crude_devgui                                       *devgui,
  _In_ crude_entity                                        main_scene_node,
  _In_ crude_entity                                        camera_node
);

CRUDE_API void
crude_devgui_handle_input
(
  _In_ crude_devgui                                       *devgui,
  _In_ crude_input                                        *input
);

CRUDE_API void
crude_devgui_on_resize
(
  _In_ crude_devgui                                       *devgui
);

CRUDE_API void
crude_devgui_graphics_pre_update
(
  _In_ crude_devgui                                       *devgui
);

/******************************
 * Dev Gui Nodes Tree
 *******************************/
CRUDE_API void
crude_devgui_nodes_tree_initialize
(
  _In_ crude_devgui_nodes_tree                            *devgui_nodes_tree,
  _In_ crude_devgui                                       *devgui
);

CRUDE_API void
crude_devgui_nodes_tree_draw
(
  _In_ crude_devgui_nodes_tree                            *devgui_nodes_tree,
  _In_ crude_entity                                        node
);

/******************************
 * Dev Gui Node Inspector
 *******************************/
CRUDE_API void
crude_devgui_node_inspector_initialize
(
  _In_ crude_devgui_node_inspector                        *devgui_inspector
);

CRUDE_API void
crude_devgui_node_inspector_draw
(
  _In_ crude_devgui_node_inspector                        *devgui_inspector,
  _In_ crude_entity                                        node
);

/******************************
 * Dev Gui Viewport
 *******************************/
CRUDE_API void
crude_devgui_viewport_initialize
(
  _In_ crude_devgui_viewport                              *devgui_viewport,
  _In_ crude_gfx_device                                   *gpu
);

CRUDE_API void
crude_devgui_viewport_draw
(
  _In_ crude_devgui_viewport                              *devgui_viewport,
  _In_ crude_entity                                        camera_node,
  _In_ crude_entity                                        selected_node
);

CRUDE_API void
crude_devgui_viewport_input
(
  _In_ crude_devgui_viewport                              *devgui_viewport,
  _In_ crude_input                                        *input
);

/******************************
 * Dev Gui Render Graph
 *******************************/
CRUDE_API void
crude_devgui_render_graph_initialize
(
  _In_ crude_devgui_render_graph                          *devgui_render_graph,
  _In_ crude_gfx_render_graph                             *render_graph
);

CRUDE_API void
crude_devgui_render_graph_draw
(
  _In_ crude_devgui_render_graph                          *devgui_render_graph
);

/******************************
 * Dev Gui GPU
 *******************************/
CRUDE_API void
crude_devgui_gpu_initialize
(
  _In_ crude_devgui_gpu                                   *dev_gpu,
  _In_ crude_gfx_device                                   *gpu,
  _In_ crude_stack_allocator                              *temporary_allocator
);

CRUDE_API void
crude_devgui_gpu_draw
(
  _In_ crude_devgui_gpu                                   *dev_gpu
);

/******************************
 * Dev Gui GPU Visual Profiler
 *******************************/
CRUDE_API void
crude_devgui_gpu_visual_profiler_initialize
(
  _In_ crude_devgui_gpu_visual_profiler                   *dev_gpu_profiler,
  _In_ crude_gfx_device                                   *gpu,
  _In_ crude_heap_allocator                               *allocator
);

CRUDE_API void
crude_devgui_gpu_visual_profiler_deinitialize
(
  _In_ crude_devgui_gpu_visual_profiler                   *dev_gpu_profiler
);

CRUDE_API void
crude_devgui_gpu_visual_profiler_update
(
  _In_ crude_devgui_gpu_visual_profiler                   *dev_gpu_profiler
);

CRUDE_API void
crude_devgui_gpu_visual_profiler_draw
(
  _In_ crude_devgui_gpu_visual_profiler                   *dev_gpu_profiler
);

/******************************
 * Dev Gui Scene Renderer
 *******************************/
CRUDE_API void
crude_devgui_scene_renderer_initialize
(
  _In_ crude_devgui_scene_renderer                        *dev_scene_renderer,
  _In_ crude_gfx_scene_renderer                           *scene_renderer
);

CRUDE_API void
crude_devgui_scene_renderer_deinitialize
(
  _In_ crude_devgui_scene_renderer                        *dev_scene_renderer
);

CRUDE_API void
crude_devgui_scene_renderer_update
(
  _In_ crude_devgui_scene_renderer                        *dev_scene_renderer
);

CRUDE_API void
crude_devgui_scene_renderer_draw
(
  _In_ crude_devgui_scene_renderer                        *dev_scene_renderer
);