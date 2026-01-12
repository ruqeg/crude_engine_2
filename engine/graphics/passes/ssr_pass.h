#pragma once

#include <engine/graphics/render_graph.h>
#include <engine/graphics/scene_renderer_resources.h>

#define CRUDE_GRAPHICS_SSR_RADIANCE_TEXTURES_MAX_LEVELS ( 16 )

typedef struct crude_gfx_scene_renderer crude_gfx_scene_renderer;

typedef struct crude_gfx_ssr_pass
{
  crude_gfx_scene_renderer                                *scene_renderer;
  crude_gfx_texture_handle                                 radiance_hierarchy_texture_handle;
  crude_gfx_texture_handle                                 radiance_hierarchy_views_handles[ CRUDE_GRAPHICS_SSR_RADIANCE_TEXTURES_MAX_LEVELS ];
  crude_gfx_sampler_handle                                 radiance_hierarchy_sampler;
  uint32                                                   radiance_hierarchy_levels;
} crude_gfx_ssr_pass;

CRUDE_API void
crude_gfx_ssr_pass_initialize
(
  _In_ crude_gfx_ssr_pass                                 *pass,
  _In_ crude_gfx_scene_renderer                           *scene_renderer
);

CRUDE_API void
crude_gfx_ssr_pass_deinitialize
(
  _In_ crude_gfx_ssr_pass                                 *pass
);

CRUDE_API void
crude_gfx_ssr_pass_render
(
  _In_ void                                               *ctx,
  _In_ crude_gfx_cmd_buffer                               *primary_cmd
);

CRUDE_API void
crude_gfx_ssr_pass_on_resize
(
  _In_ void                                               *ctx,
  _In_ uint32                                              new_width,
  _In_ uint32                                              new_height
);

CRUDE_API crude_gfx_render_graph_pass_container
crude_gfx_ssr_pass_pack
(
  _In_ crude_gfx_ssr_pass                                 *pass
);