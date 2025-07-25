#pragma once

#include <graphics/render_graph.h>
#include <graphics/scene_renderer_resources.h>

typedef struct crude_gfx_scene_renderer crude_gfx_scene_renderer;

typedef struct crude_gfx_meshlet_pass
{
  crude_gfx_scene_renderer                                *scene_renderer;
  bool                                                     early_pass;
  crude_gfx_descriptor_set_handle                          meshlet_shader_ds[ CRUDE_GFX_MAX_SWAPCHAIN_IMAGES ];
} crude_gfx_meshlet_pass;

CRUDE_API void
crude_gfx_meshlet_pass_initialize
(
  _In_ crude_gfx_meshlet_pass                             *pass,
  _In_ crude_gfx_scene_renderer                           *scene_renderer,
  bool                                                     early_pass
);

CRUDE_API void
crude_gfx_meshlet_pass_deinitialize
(
  _In_ crude_gfx_meshlet_pass                             *pass
);

CRUDE_API void
crude_gfx_meshlet_pass_render
(
  _In_ void                                               *ctx,
  _In_ crude_gfx_cmd_buffer                               *primary_cmd
);

CRUDE_API crude_gfx_render_graph_pass_container
crude_gfx_meshlet_pass_pack
(
  _In_ crude_gfx_meshlet_pass                             *pass
);