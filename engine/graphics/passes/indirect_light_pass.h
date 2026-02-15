#pragma once

#include <engine/graphics/render_graph.h>
#include <engine/graphics/scene_renderer_resources.h>

#if CRUDE_GRAPHICS_RAY_TRACING_ENABLED

typedef struct crude_gfx_scene_renderer crude_gfx_scene_renderer;

typedef CRUDE_ALIGNED_STRUCT( 16 ) crude_gfx_ddgi_gpu_data
{
  XMINT3                                                   probe_counts;
  int32                                                    probe_rays;
  XMFLOAT3                                                 probe_spacing;
  int32                                                    probe_update_offset;
  XMFLOAT3                                                 probe_grid_position;
  float32                                                  max_probe_offset;

  uint32                                                   radiance_output_index;
  uint32                                                   indirect_output_index;
  uint32                                                   normal_texture_index;
  uint32                                                   depth_fullscreen_texture_index;
  uint32                                                   grid_irradiance_output_index;
  uint32                                                   grid_visibility_texture_index;
  uint32                                                   probe_offset_texture_index;
  float32                                                  hysteresis;

  XMFLOAT4X4                                               random_rotation;

  int32                                                    irradiance_texture_width;
  int32                                                    irradiance_texture_height;
  int32                                                    irradiance_side_length;

  int32                                                    visibility_texture_width;
  int32                                                    visibility_texture_height;
  int32                                                    visibility_side_length;

  float32                                                  self_shadow_bias;
  float32                                                  infinite_bounces_multiplier;
  XMFLOAT3                                                 reciprocal_probe_spacing;

  float32                                                  shadow_weight_power;
  int32                                                    probe_update_per_frame;
} crude_gfx_ddgi_gpu_data;

typedef struct crude_gfx_indirect_light_pass_options
{
  XMFLOAT3                                                 probe_spacing;
  XMFLOAT3                                                 probe_grid_position;
  float32                                                  hysteresis;
  float32                                                  self_shadow_bias;
  float32                                                  infinite_bounces_multiplier;
  float32                                                  max_probe_offset;
  uint32                                                   probe_debug_flags;
  float32                                                  shadow_weight_power;
  int32                                                    probe_update_per_frame;
} crude_gfx_indirect_light_pass_options;

typedef struct crude_gfx_indirect_light_pass
{
  crude_gfx_scene_renderer                                *scene_renderer;
  crude_gfx_descriptor_set_handle                          probe_raytrace_ds[ CRUDE_GFX_SWAPCHAIN_IMAGES_MAX_COUNT ];
  crude_gfx_descriptor_set_handle                          probe_debug_ds[ CRUDE_GFX_SWAPCHAIN_IMAGES_MAX_COUNT ];
  crude_gfx_descriptor_set_handle                          calculate_probe_offsets_ds[ CRUDE_GFX_SWAPCHAIN_IMAGES_MAX_COUNT ];
  crude_gfx_descriptor_set_handle                          probe_update_irradiance_ds[ CRUDE_GFX_SWAPCHAIN_IMAGES_MAX_COUNT ];
  crude_gfx_descriptor_set_handle                          probe_update_visibility_ds[ CRUDE_GFX_SWAPCHAIN_IMAGES_MAX_COUNT ];
  crude_gfx_descriptor_set_handle                          sample_irradiance_ds[ CRUDE_GFX_SWAPCHAIN_IMAGES_MAX_COUNT ];
  crude_gfx_texture_handle                                 probe_grid_irradiance_texture_handle;
  crude_gfx_texture_handle                                 probe_grid_visibility_texture_handle;
  crude_gfx_texture_handle                                 probe_offsets_texture_handle;
  crude_gfx_texture_handle                                 indirect_texture_handle;
  crude_gfx_texture_handle                                 probe_raytrace_radiance_texture_handle;
  crude_gfx_buffer_handle                                  ddgi_cb;
  crude_gfx_sampler_handle                                 probe_grid_sampler_handle;

  uint32                                                   irradiance_atlas_width;
  uint32                                                   irradiance_atlas_height;
  uint32                                                   irradiance_side_length;
  uint32                                                   irradiance_side_length_with_borders;
  uint32                                                   visibility_atlas_width;
  uint32                                                   visibility_atlas_height;
  uint32                                                   visibility_side_length;
  uint32                                                   visibility_side_length_with_borders;

  uint32                                                   probe_update_offset;
  
  uint32                                                   probe_count_x;
  uint32                                                   probe_count_y;
  uint32                                                   probe_count_z;
  int32                                                    probe_rays;
  bool                                                     use_half_resolution;

  int32                                                    offsets_calculations_count;

  crude_gfx_indirect_light_pass_options                    options;
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

CRUDE_API void
crude_gfx_indirect_light_pass_on_resize
(
  _In_ void                                               *ctx,
  _In_ uint32                                              new_width,
  _In_ uint32                                              new_height
);

CRUDE_API void
crude_gfx_indirect_light_pass_on_techniques_reloaded
(
  _In_ void                                               *ctx
);

CRUDE_API crude_gfx_render_graph_pass_container
crude_gfx_indirect_light_pass_pack
(
  _In_ crude_gfx_indirect_light_pass                      *pass
);

#endif /* CRUDE_GRAPHICS_RAY_TRACING_ENABLED */