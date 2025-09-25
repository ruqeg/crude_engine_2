#pragma once

#include <graphics/render_graph.h>
#include <graphics/scene_renderer_resources.h>

typedef struct crude_gfx_scene_renderer crude_gfx_scene_renderer;

typedef struct crude_gfx_ddgi_gpu_data
{
  XMINT3                                                   probe_counts;
  int32                                                    probe_rays;
  XMFLOAT3                                                 probe_spacing;
  uint32                                                   radiance_output_index;
  XMFLOAT4X4                                               random_rotation;
  XMFLOAT3                                                 probe_grid_position;
  int32                                                    irradiance_texture_width;
  int32                                                    irradiance_texture_height;
  int32                                                    irradiance_side_length;
  float32                                                  hysteresis;
  int32                                                    visibility_texture_width;
  int32                                                    visibility_texture_height;
  int32                                                    visibility_side_length;
  float32                                                  self_shadow_bias;
} crude_gfx_ddgi_gpu_data;

typedef struct crude_gfx_indirect_light_pass
{
  crude_gfx_scene_renderer                                *scene_renderer;
  crude_gfx_descriptor_set_handle                          probe_raytrace_ds[ CRUDE_GFX_MAX_SWAPCHAIN_IMAGES ];
  crude_gfx_descriptor_set_handle                          probe_update_visibility_ds[ CRUDE_GFX_MAX_SWAPCHAIN_IMAGES ];
  crude_gfx_descriptor_set_handle                          probe_update_irradiance_ds[ CRUDE_GFX_MAX_SWAPCHAIN_IMAGES ];
  crude_gfx_descriptor_set_handle                          calculate_probe_status_ds[ CRUDE_GFX_MAX_SWAPCHAIN_IMAGES ];
  crude_gfx_texture_handle                                 probe_grid_irradiance_texture_handle;
  crude_gfx_texture_handle                                 probe_grid_visibility_texture_handle;
  crude_gfx_texture_handle                                 probe_offsets_texture_handle;
  crude_gfx_texture_handle                                 indirect_texture_handle;
  crude_gfx_texture_handle                                 probe_raytrace_radiance_texture_handle[ CRUDE_GFX_MAX_SWAPCHAIN_IMAGES ];
  crude_gfx_buffer_handle                                  ddgi_sb;
  crude_gfx_buffer_handle                                  probe_status_sb;
  uint32                                                   probe_count_x;
  uint32                                                   probe_count_y;
  uint32                                                   probe_count_z;
  int32                                                    probe_rays;
  uint32                                                   irradiance_atlas_width;
  uint32                                                   irradiance_atlas_height;
  uint32                                                   visibility_atlas_width;
  uint32                                                   visibility_atlas_height;
  bool                                                     use_half_resolution;
} crude_gfx_indirect_light_pass;

CRUDE_API void
crude_gfx_indirect_light_pass_initialize
(
  _In_ crude_gfx_indirect_light_pass                      *pass,
  _In_ crude_gfx_scene_renderer                           *scene_renderer
);

CRUDE_API void
crude_gfx_indirect_light_pass_deinitialize
(
  _In_ crude_gfx_indirect_light_pass                      *pass
);

CRUDE_API void
crude_gfx_indirect_light_pass_render
(
  _In_ void                                               *ctx,
  _In_ crude_gfx_cmd_buffer                               *primary_cmd
);

CRUDE_API crude_gfx_render_graph_pass_container
crude_gfx_indirect_light_pass_pack
(
  _In_ crude_gfx_indirect_light_pass                      *pass
);