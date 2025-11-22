#if CRUDE_DEVELOP
#pragma once

#include <graphics/scene_renderer.h>
#include <graphics/gpu_profiler.h>
#include <platform/platform_components.h>

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

typedef struct crude_devmenu
{
	bool																										 enabled;
  crude_devmenu_gpu_visual_profiler                        gpu_visual_profiler;
  crude_devmenu_texture_inspector                          texture_inspector;
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
#endif