#pragma once

#include <core/alias.h>

#include <graphics/gpu_device.h>

CRUDE_API void
crude_gui_initialize
(
);

CRUDE_API void
crude_gui_deinitialize
(
);

CRUDE_API void
crude_gui_create
(
  _In_ crude_gfx_device                                   *gpu 
);

CRUDE_API void
crude_gui_destroy
(
  _In_ crude_gfx_device                                   *gpu 
);