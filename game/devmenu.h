#pragma once

#if CRUDE_DEVELOP

#include <engine/graphics/scene_renderer.h>
#include <engine/graphics/gpu_profiler.h>
#include <engine/platform/platform_components.h>

typedef struct game_t game_t;

typedef struct crude_devmenu_texture_inspector
{
	bool																										 enabled;
  crude_gfx_texture_handle                                 texture_handle;
} crude_devmenu_texture_inspector;

typedef struct crude_devmenu_gpu_visual_profiler
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
} crude_devmenu_gpu_visual_profiler;

typedef struct crude_devmenu_render_graph
{
  bool                                                     enabled;
} crude_devmenu_render_graph;

typedef struct crude_devmenu_gpu_pool
{
  bool                                                     enabled;
} crude_devmenu_gpu_pool;

typedef struct crude_devmenu_scene_renderer
{
  bool                                                     debug_probes_statuses;
  bool                                                     debug_probes_radiance;
  bool                                                     enabled;
} crude_devmenu_scene_renderer;

typedef struct crude_devmenu_nodes_tree
{
  crude_entity                                             selected_node;
	bool																										 enabled;
} crude_devmenu_nodes_tree;

typedef struct crude_devmenu_gameplay
{
	bool																										 enabled;
} crude_devmenu_gameplay;

typedef struct crude_devmenu
{
	bool																										 enabled;
  crude_devmenu_gpu_visual_profiler                        gpu_visual_profiler;
  crude_devmenu_texture_inspector                          texture_inspector;
  crude_devmenu_render_graph                               render_graph;
  crude_devmenu_gpu_pool                                   gpu_pool;
  crude_devmenu_scene_renderer                             scene_renderer;
  crude_devmenu_nodes_tree                                 nodes_tree;
  crude_devmenu_gameplay                                   gameplay;
  uint32                                                   selected_option;
} crude_devmenu;

/***********************
 * 
 * Develop Menu
 * 
 ***********************/
CRUDE_API void
crude_devmenu_initialize
(
	_In_ crude_devmenu																			*devmenu
);

CRUDE_API void
crude_devmenu_deinitialize
(
	_In_ crude_devmenu																			*devmenu
);

CRUDE_API void
crude_devmenu_draw
(
	_In_ crude_devmenu																			*devmenu
);

CRUDE_API void
crude_devmenu_update
(
	_In_ crude_devmenu																			*devmenu
);

CRUDE_API void
crude_devmenu_handle_input
(
	_In_ crude_devmenu																			*devmenu,
	_In_ crude_input																				*input
);

/***********************
 * 
 * Common Commmads
 * 
 ***********************/
CRUDE_API void
crude_devmenu_debug_gltf_view_callback
(
	_In_ crude_devmenu									                    *devmenu
);

CRUDE_API bool
crude_devmenu_debug_gltf_view_callback_hotkey_pressed_callback
(
  _In_ crude_input																				*input
);

CRUDE_API void
crude_devmenu_collisions_view_callback
(
	_In_ crude_devmenu									                    *devmenu
);

CRUDE_API bool
crude_devmenu_collisions_view_callback_hotkey_pressed_callback
(
  _In_ crude_input																				*input
);

CRUDE_API void
crude_devmenu_free_camera_callback
(
	_In_ crude_devmenu									                    *devmenu
);

CRUDE_API bool
crude_devmenu_free_camera_callback_hotkey_pressed_callback
(
  _In_ crude_input																				*input
);

CRUDE_API void
crude_devmenu_reload_techniques_callback
(
	_In_ crude_devmenu									                    *devmenu
);

CRUDE_API bool
crude_devmenu_reload_techniques_hotkey_pressed_callback
(
  _In_ crude_input																				*input
);

/***********************
 * 
 * Develop GPU Visual Profiler
 * 
 ***********************/
CRUDE_API void
crude_devmenu_gpu_visual_profiler_initialize
(
	_In_ crude_devmenu_gpu_visual_profiler									*dev_gpu_profiler
);

CRUDE_API void
crude_devmenu_gpu_visual_profiler_deinitialize
(
	_In_ crude_devmenu_gpu_visual_profiler									*dev_gpu_profiler
);

CRUDE_API void
crude_devmenu_gpu_visual_profiler_update
(
	_In_ crude_devmenu_gpu_visual_profiler									*dev_gpu_profiler
);

CRUDE_API void
crude_devmenu_gpu_visual_profiler_draw
(
	_In_ crude_devmenu_gpu_visual_profiler									*dev_gpu_profiler
);

CRUDE_API void
crude_devmenu_gpu_visual_profiler_callback
(
	_In_ crude_devmenu									                    *devmenu
);

/***********************
 * 
 * Develop Texture Inspector
 * 
 ***********************/
CRUDE_API void
crude_devmenu_texture_inspector_initialize
(
	_In_ crude_devmenu_texture_inspector									  *dev_texture_inspector
);

CRUDE_API void
crude_devmenu_texture_inspector_deinitialize
(
	_In_ crude_devmenu_texture_inspector									  *dev_texture_inspector
);

CRUDE_API void
crude_devmenu_texture_inspector_update
(
	_In_ crude_devmenu_texture_inspector									  *dev_texture_inspector
);

CRUDE_API void
crude_devmenu_texture_inspector_draw
(
	_In_ crude_devmenu_texture_inspector									  *dev_texture_inspector
);

CRUDE_API void
crude_devmenu_texture_inspector_callback
(
	_In_ crude_devmenu									                    *devmenu
);

/***********************
 * 
 * Develop Render Graph
 * 
 ***********************/
CRUDE_API void
crude_devmenu_render_graph_initialize
(
  _In_ crude_devmenu_render_graph                         *dev_render_graph
);

CRUDE_API void
crude_devmenu_render_graph_deinitialize
(
  _In_ crude_devmenu_render_graph                         *dev_render_graph
);

CRUDE_API void
crude_devmenu_render_graph_update
(
  _In_ crude_devmenu_render_graph                         *dev_render_graph
);

CRUDE_API void
crude_devmenu_render_graph_draw
(
  _In_ crude_devmenu_render_graph                         *dev_render_graph
);

CRUDE_API void
crude_devmenu_render_graph_callback
(
	_In_ crude_devmenu									                    *devmenu
);

/***********************
 * 
 * Develop GPU Pools
 * 
 ***********************/
CRUDE_API void
crude_devmenu_gpu_pool_initialize
(
  _In_ crude_devmenu_gpu_pool                             *dev_gpu_pool
);

CRUDE_API void
crude_devmenu_gpu_pool_deinitialize
(
  _In_ crude_devmenu_gpu_pool                             *dev_gpu_pool
);

CRUDE_API void
crude_devmenu_gpu_pool_update
(
  _In_ crude_devmenu_gpu_pool                             *dev_gpu_pool
);

CRUDE_API void
crude_devmenu_gpu_pool_draw
(
  _In_ crude_devmenu_gpu_pool                             *dev_gpu_pool
);

CRUDE_API void
crude_devmenu_gpu_pool_callback
(
	_In_ crude_devmenu									                    *devmenu
);

/***********************
 * 
 * Develop Scene Renderer
 * 
 ***********************/
CRUDE_API void
crude_devmenu_scene_renderer_initialize
(
  _In_ crude_devmenu_scene_renderer                       *dev_scene_rendere
);

CRUDE_API void
crude_devmenu_scene_renderer_deinitialize
(
  _In_ crude_devmenu_scene_renderer                       *dev_scene_rendere
);

CRUDE_API void
crude_devmenu_scene_renderer_update
(
  _In_ crude_devmenu_scene_renderer                       *dev_scene_rendere
);

CRUDE_API void
crude_devmenu_scene_renderer_draw
(
  _In_ crude_devmenu_scene_renderer                       *dev_scene_rendere
);

CRUDE_API void
crude_devmenu_scene_renderer_callback
(
	_In_ crude_devmenu									                    *devmenu
);

/***********************
 * 
 * Develop Nodes Tree
 * 
 ***********************/
CRUDE_API void
crude_devmenu_nodes_tree_initialize
(
  _In_ crude_devmenu_nodes_tree                           *dev_nodes_tree
);

CRUDE_API void
crude_devmenu_nodes_tree_deinitialize
(
  _In_ crude_devmenu_nodes_tree                           *dev_nodes_tree
);

CRUDE_API void
crude_devmenu_nodes_tree_update
(
  _In_ crude_devmenu_nodes_tree                           *dev_nodes_tree
);

CRUDE_API void
crude_devmenu_nodes_tree_draw
(
  _In_ crude_devmenu_nodes_tree                           *dev_nodes_tree
);

CRUDE_API void
crude_devmenu_nodes_tree_callback
(
	_In_ crude_devmenu									                    *devmenu
);

/***********************
 * 
 * Develop Gameplay
 * 
 ***********************/
CRUDE_API void
crude_devmenu_gameplay_initialize
(
  _In_ crude_devmenu_gameplay                             *dev_gameplay
);

CRUDE_API void
crude_devmenu_gameplay_deinitialize
(
  _In_ crude_devmenu_gameplay                             *dev_gameplay
);

CRUDE_API void
crude_devmenu_gameplay_update
(
  _In_ crude_devmenu_gameplay                             *dev_gameplay
);

CRUDE_API void
crude_devmenu_gameplay_draw
(
  _In_ crude_devmenu_gameplay                             *dev_gameplay
);

CRUDE_API void
crude_devmenu_gameplay_callback
(
	_In_ crude_devmenu									                    *devmenu
);

#endif