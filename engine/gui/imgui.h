#pragma once

#include <graphics/gpu_device.h>

typedef struct crude_imgui
{
  crude_gfx_device                                        *gpu;
  crude_gfx_buffer_handle                                  vertex_buffer;
  crude_gfx_buffer_handle                                  index_buffer;
  crude_gfx_buffer_handle                                  ui_cb;
  crude_gfx_texture_handle                                 font_texture;
  crude_gfx_pipeline_handle                                pipeline;
  crude_gfx_descriptor_set_layout_handle                   descriptor_set_layout;
  crude_gfx_descriptor_set_handle                          descriptor_set;
  struct { uint64 key; crude_gfx_descriptor_set_handle value; } *texture_to_descriptor_set;
} crude_imgui;

CRUDE_API void
crude_imgui_initialize
(
  _In_ crude_imgui                                        *imgui,
  _In_ crude_gfx_device                                   *gpu,
  _In_ void                                               *window_handle
);

CRUDE_API void
crude_imgui_deinitialize
(
  _In_ crude_imgui                                        *imgui
);

CRUDE_API void
crude_imgui_new_frame
(
  _In_ crude_imgui                                        *imgui
);

CRUDE_API void
crude_imgui_render
(
  _In_ crude_imgui                                        *imgui,
  _In_ crude_gfx_cmd_buffer                               *cmd,
  _In_ crude_gfx_render_pass_handle                        render_pass,
  _In_ crude_gfx_framebuffer_handle                        framebuffer,
  _In_ bool                                                use_secondary
);