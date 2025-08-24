#pragma once

#include <graphics/render_graph.h>
#include <graphics/scene_renderer_resources.h>

typedef struct crude_gfx_scene_renderer crude_gfx_scene_renderer;

typedef CRUDE_ALIGNED_STRUCT( 16 ) crude_gfx_light_constant_gpu
{
  XMUINT4                                                  textures;
} crude_gfx_light_constant_gpu;

typedef struct crude_gfx_light_pass
{
  crude_gfx_scene_renderer                                *scene_renderer;
  crude_gfx_descriptor_set_handle                          light_ds[ CRUDE_GFX_MAX_SWAPCHAIN_IMAGES ];
  crude_gfx_buffer_handle                                  light_cb;
} crude_gfx_light_pass;

CRUDE_API void
crude_gfx_light_pass_initialize
(
  _In_ crude_gfx_light_pass                               *pass,
  _In_ crude_gfx_scene_renderer                           *scene_renderer
);

CRUDE_API void
crude_gfx_light_pass_deinitialize
(
  _In_ crude_gfx_light_pass                               *pass
);

CRUDE_API void
crude_gfx_light_pass_on_render_graph_registered
(
  _In_ crude_gfx_light_pass                               *pass
);

CRUDE_API void
crude_gfx_light_pass_render
(
  _In_ void                                               *ctx,
  _In_ crude_gfx_cmd_buffer                               *primary_cmd
);

CRUDE_API void
crude_gfx_light_pass_on_techniques_reloaded
(
  _In_ void                                               *ctx
);

CRUDE_API void
crude_gfx_light_pass_on_resize
(
  _In_ void                                               *ctx,
  _In_ crude_gfx_device                                   *gpu,
  _In_ uint32                                              new_width,
  _In_ uint32                                              new_height
);

CRUDE_API crude_gfx_render_graph_pass_container
crude_gfx_light_pass_pack
(
  _In_ crude_gfx_light_pass                               *pass
);