#pragma once

#include <graphics/render_graph.h>

typedef struct crude_gfx_scene_renderer crude_gfx_scene_renderer;

typedef struct crude_gfx_imgui_pass
{
  crude_gfx_scene_renderer                                *scene_renderer;
  crude_gfx_descriptor_set_handle                          imgui_ds;
  crude_gfx_buffer_handle                                  vertex_buffer;
  crude_gfx_buffer_handle                                  index_buffer;
  crude_gfx_buffer_handle                                  ui_cb;
  crude_gfx_texture_handle                                 font_texture;
} crude_gfx_imgui_pass;

CRUDE_API void
crude_gfx_imgui_pass_initialize
(
  _In_ crude_gfx_imgui_pass                               *pass,
  crude_gfx_scene_renderer                                *scene_renderer
);

CRUDE_API void
crude_gfx_imgui_pass_deinitialize
(
  _In_ crude_gfx_imgui_pass                               *pass
);

CRUDE_API void
crude_gfx_imgui_pass_render
(
  _In_ void                                               *ctx,
  _In_ crude_gfx_cmd_buffer                               *primary_cmd
);

CRUDE_API crude_gfx_render_graph_pass_container
crude_gfx_imgui_pass_pack
(
  _In_ crude_gfx_imgui_pass                               *pass
);