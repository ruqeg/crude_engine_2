#pragma once

#include <graphics/render_graph.h>
#include <graphics/scene_renderer_resources.h>

#define CRUDE_GFX_TETRAHEDRON_SHADOWMAP_WIDTH 8192
#define CRUDE_GFX_TETRAHEDRON_SHADOWMAP_HEIGHT 8192

typedef struct crude_gfx_scene_renderer crude_gfx_scene_renderer;

typedef struct crude_gfx_pointlight_shadow_pass
{
  crude_gfx_scene_renderer                                *scene_renderer;
  crude_gfx_descriptor_set_handle                          pointshadow_culling_ds[ CRUDE_GFX_MAX_SWAPCHAIN_IMAGES ];
  crude_gfx_descriptor_set_handle                          pointshadow_commands_generation_ds[ CRUDE_GFX_MAX_SWAPCHAIN_IMAGES ];
  crude_gfx_descriptor_set_handle                          pointshadow_ds[ CRUDE_GFX_MAX_SWAPCHAIN_IMAGES ];
  crude_gfx_texture_handle                                 tetrahedron_shadow_texture;
  crude_gfx_render_pass_handle                             tetrahedron_render_pass_handle;
  crude_gfx_framebuffer_handle                             tetrahedron_framebuffer_handle;
  crude_gfx_buffer_handle                                  pointlight_spheres_sb[ CRUDE_GFX_MAX_SWAPCHAIN_IMAGES ];
  crude_gfx_buffer_handle                                  pointshadow_meshlet_draw_commands_sb[ CRUDE_GFX_MAX_SWAPCHAIN_IMAGES ];
  crude_gfx_buffer_handle                                  meshletes_instances_sb[ CRUDE_GFX_MAX_SWAPCHAIN_IMAGES ];
  crude_gfx_buffer_handle                                  pointshadow_meshletes_instances_count_sb[ CRUDE_GFX_MAX_SWAPCHAIN_IMAGES ];
  crude_gfx_sampler_handle                                 tetrahedron_shadow_sampler;
} crude_gfx_pointlight_shadow_pass;

CRUDE_API void
crude_gfx_pointlight_shadow_pass_initialize
(
  _In_ crude_gfx_pointlight_shadow_pass                   *pass,
  _In_ crude_gfx_scene_renderer                           *scene_renderer
);

CRUDE_API void
crude_gfx_pointlight_shadow_pass_deinitialize
(
  _In_ crude_gfx_pointlight_shadow_pass                   *pass
);

CRUDE_API void
crude_gfx_pointlight_shadow_pass_render
(
  _In_ void                                               *ctx,
  _In_ crude_gfx_cmd_buffer                               *primary_cmd
);

CRUDE_API crude_gfx_render_graph_pass_container
crude_gfx_pointlight_shadow_pass_pack
(
  _In_ crude_gfx_pointlight_shadow_pass                   *pass
);