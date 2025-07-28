#pragma once

#include <graphics/render_graph.h>
#include <graphics/scene_renderer_resources.h>

typedef struct crude_gfx_scene_renderer crude_gfx_scene_renderer;

typedef struct crude_gfx_debug_pass
{
  crude_gfx_scene_renderer                                *scene_renderer;
  crude_gfx_descriptor_set_handle                          depth_lines3d_descriptor_sets_handles[ CRUDE_GFX_MAX_SWAPCHAIN_IMAGES ];
} crude_gfx_debug_pass;

CRUDE_API void
crude_gfx_debug_pass_initialize
(
  _In_ crude_gfx_debug_pass                               *pass,
  _In_ crude_gfx_scene_renderer                           *scene_renderer
);

CRUDE_API void
crude_gfx_debug_pass_deinitialize
(
  _In_ crude_gfx_debug_pass                               *pass
);

CRUDE_API void
crude_gfx_debug_pass_pre_render
(
  _In_ void                                               *ctx,
  _In_ crude_gfx_cmd_buffer                               *primary_cmd
);

CRUDE_API void
crude_gfx_debug_pass_render
(
  _In_ void                                               *ctx,
  _In_ crude_gfx_cmd_buffer                               *primary_cmd
);

CRUDE_API void
crude_gfx_debug_pass_post_render
(
  _In_ void                                               *ctx,
  _In_ crude_gfx_cmd_buffer                               *primary_cmd
);

CRUDE_API crude_gfx_render_graph_pass_container
crude_gfx_debug_pass_pack
(
  _In_ crude_gfx_debug_pass                               *pass
);