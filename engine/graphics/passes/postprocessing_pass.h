#pragma once

#include <graphics/render_graph.h>
#include <graphics/scene_renderer_resources.h>

typedef struct crude_gfx_scene_renderer crude_gfx_scene_renderer;

typedef struct crude_gfx_postprocessing_pass
{
  crude_gfx_scene_renderer                                *scene_renderer;
  crude_gfx_texture_handle                                 luminance_average_texture_handle[ CRUDE_GFX_MAX_SWAPCHAIN_IMAGES ];
  crude_gfx_descriptor_set_handle                          luminance_histogram_generation_ds[ CRUDE_GFX_MAX_SWAPCHAIN_IMAGES ];
  crude_gfx_descriptor_set_handle                          luminance_average_calculation_ds[ CRUDE_GFX_MAX_SWAPCHAIN_IMAGES ];
  crude_gfx_buffer_handle                                  luminance_histogram_sb_handle[ CRUDE_GFX_MAX_SWAPCHAIN_IMAGES ];
  int64                                                    luminance_avarge_last_update_time;
  float32                                                  min_log_lum;
  float32                                                  max_log_lum;
} crude_gfx_postprocessing_pass;

CRUDE_API void
crude_gfx_postprocessing_pass_initialize
(
  _In_ crude_gfx_postprocessing_pass                      *pass,
  _In_ crude_gfx_scene_renderer                           *scene_renderer
);

CRUDE_API void
crude_gfx_postprocessing_pass_deinitialize
(
  _In_ crude_gfx_postprocessing_pass                      *pass
);

CRUDE_API void
crude_gfx_postprocessing_pass_on_render_graph_registered
(
  _In_ crude_gfx_postprocessing_pass                      *pass
);

CRUDE_API void
crude_gfx_postprocessing_pass_render
(
  _In_ void                                               *ctx,
  _In_ crude_gfx_cmd_buffer                               *primary_cmd
);

CRUDE_API void
crude_gfx_postprocessing_pass_on_resize
(
  _In_ void                                               *ctx,
  _In_ uint32                                              new_width,
  _In_ uint32                                              new_height
);

CRUDE_API void
crude_gfx_postprocessing_pass_on_techniques_reloaded
(
  _In_ void                                               *ctx
);

CRUDE_API void
crude_gfx_postprocessing_pass_pre_render
(
  _In_ void                                               *ctx,
  _In_ crude_gfx_cmd_buffer                               *primary_cmd
);

CRUDE_API crude_gfx_render_graph_pass_container
crude_gfx_postprocessing_pass_pack
(
  _In_ crude_gfx_postprocessing_pass                      *pass
);