#pragma once

#include <graphics/render_graph.h>
#include <graphics/scene_renderer_resources.h>

#define CRUDE_GFX_DEPTH_PYRAMID_PASS_MAX_LEVELS ( 16 )

typedef struct crude_gfx_scene_renderer crude_gfx_scene_renderer;

typedef struct crude_gfx_depth_pyramid_pass
{
  crude_gfx_scene_renderer                                *scene_renderer;
  crude_gfx_texture_handle                                 depth_pyramid_texture_handle;
  crude_gfx_texture_handle                                 depth_pyramid_views_handles[ CRUDE_GFX_DEPTH_PYRAMID_PASS_MAX_LEVELS ];
  crude_gfx_descriptor_set_handle                          depth_hierarchy_descriptor_sets_handles[ CRUDE_GFX_DEPTH_PYRAMID_PASS_MAX_LEVELS ];
  crude_gfx_descriptor_set_layout_handle                   depth_pyramid_layout_handle;
  crude_gfx_sampler_handle                                 depth_pyramid_sampler;
  uint32                                                   depth_pyramid_levels;
} crude_gfx_depth_pyramid_pass;

CRUDE_API void
crude_gfx_depth_pyramid_pass_initialize
(
  _In_ crude_gfx_depth_pyramid_pass                       *pass,
  _In_ crude_gfx_scene_renderer                           *scene_renderer
);

CRUDE_API void
crude_gfx_depth_pyramid_pass_deinitialize
(
  _In_ crude_gfx_depth_pyramid_pass                       *pass
);

CRUDE_API void
crude_gfx_depth_pyramid_pass_on_render_graph_registered
(
  _In_ crude_gfx_depth_pyramid_pass                       *pass
);

CRUDE_API void
crude_gfx_depth_pyramid_pass_render
(
  _In_ void                                               *ctx,
  _In_ crude_gfx_cmd_buffer                               *primary_cmd
);

CRUDE_API crude_gfx_render_graph_pass_container
crude_gfx_depth_pyramid_pass_pack
(
  _In_ crude_gfx_depth_pyramid_pass                       *pass
);