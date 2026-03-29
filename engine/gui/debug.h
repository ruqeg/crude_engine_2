#pragma once

#include <engine/graphics/scene_renderer.h>
#include <engine/graphics/gpu_profiler.h>
#include <engine/graphics/imgui.h>

typedef struct crude_engine crude_engine;

typedef struct crude_gui_debug
{
  crude_engine                                            *engine;
} crude_gui_debug;

CRUDE_API void
crude_gui_debug_initialize
(
  _In_ crude_gui_debug                                    *debug,
  crude_engine                                            *engine
);

CRUDE_API void
crude_gui_debug_deinitialize
(
  _In_ crude_gui_debug                                    *debug
);

CRUDE_API void
crude_gui_debug_update
(
  _In_ crude_gui_debug                                    *debug
);

CRUDE_API void
crude_gui_debug_queue_draw
(
  _In_ crude_gui_debug                                    *debug
);