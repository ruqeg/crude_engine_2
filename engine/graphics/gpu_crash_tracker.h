#pragma once

#include <engine/graphics/graphics_config.h>

#if CRUDE_GFX_USE_NSIGHT_AFTERMATH

#include <threads.h>

#include <engine/core/alias.h>
#include <engine/core/memory.h>

typedef struct crude_gfx_gpu_device_shader_database_ crude_gfx_gpu_device_shader_database_;

typedef struct crude_gfx_gpu_crash_tracker
{
  crude_gfx_gpu_device_shader_database_                   *shader_database;
  crude_heap_allocator                                    *allocator;
  uint32                                                   crash_count;
  mtx_t                                                    mutex;
} crude_gfx_gpu_crash_tracker;

CRUDE_API void
crude_gfx_gpu_crash_tracker_initialize
(
  _In_ crude_gfx_gpu_crash_tracker                        *tracker,
  _In_ crude_heap_allocator                               *allocator
);

CRUDE_API void
crude_gfx_gpu_crash_tracker_deinitialize
(
  _In_ crude_gfx_gpu_crash_tracker                        *tracker
);

CRUDE_API void
crude_gfx_gpu_crash_tracker_handle_device_lost
(
  _In_ crude_gfx_gpu_crash_tracker                        *tracker
);

#endif