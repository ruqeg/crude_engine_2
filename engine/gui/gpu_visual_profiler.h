#pragma once

#include <engine/graphics/scene_renderer.h>
#include <engine/graphics/gpu_profiler.h>
#include <engine/graphics/imgui.h>

typedef struct crude_gui_gpu_visual_profiler
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
} crude_gui_gpu_visual_profiler;

CRUDE_API void
crude_gui_gpu_visual_profiler_initialize
(
  _In_ crude_gui_gpu_visual_profiler                      *profiler,
  _In_ crude_gfx_device                                   *gpu,
  _In_ crude_heap_allocator                               *allocator
);

CRUDE_API void
crude_gui_gpu_visual_profiler_deinitialize
(
  _In_ crude_gui_gpu_visual_profiler                      *profiler
);

CRUDE_API void
crude_gui_gpu_visual_profiler_update
(
  _In_ crude_gui_gpu_visual_profiler                      *profiler
);

CRUDE_API void
crude_gui_gpu_visual_profiler_queue_draw
(
  _In_ crude_gui_gpu_visual_profiler                      *profiler
);