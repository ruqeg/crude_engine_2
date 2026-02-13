#pragma once

#include <engine/graphics/scene_renderer.h>
#include <engine/graphics/gpu_profiler.h>
#include <engine/graphics/imgui.h>

typedef struct crude_gui_log_viewer
{
  uint64                                                   prev_log_buffer_length;
} crude_gui_log_viewer;

CRUDE_API void
crude_gui_log_viewer_initialize
(
  _In_ crude_gui_log_viewer                               *log_viewer
);

CRUDE_API void
crude_gui_log_viewer_deinitialize
(
  _In_ crude_gui_log_viewer                               *log_viewer
);

CRUDE_API void
crude_gui_log_viewer_update
(
  _In_ crude_gui_log_viewer                               *log_viewer
);

CRUDE_API void
crude_gui_log_viewer_queue_draw
(
  _In_ crude_gui_log_viewer                               *log_viewer
);