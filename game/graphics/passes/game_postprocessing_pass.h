#pragma once

#include <engine/graphics/render_graph.h>
#include <engine/graphics/scene_renderer_resources.h>

typedef struct crude_gfx_scene_renderer crude_gfx_scene_renderer;

typedef struct crude_gfx_game_postprocessing_pass_options
{
  XMFLOAT4                                                 fog_color;
  float32                                                  fog_distance;
  float32                                                  fog_coeff;
  float32                                                  wave_size;
  float32                                                  wave_texcoord_scale;
  float32                                                  wave_absolute_frame_scale;
  float32                                                  aberration_strength_scale;
  float32                                                  aberration_strength_offset;
  float32                                                  aberration_strength_sin_affect;
  XMFLOAT4                                                 pulse_color;
  float32                                                  pulse_frame_scale;
  float32                                                  pulse_scale;
  float32                                                  pulse_distance_coeff;
  float32                                                  pulse_distance;
} crude_gfx_game_postprocessing_pass_options;

typedef struct crude_gfx_game_postprocessing_pass
{
  crude_gfx_scene_renderer                                *scene_renderer;
  crude_gfx_descriptor_set_handle                          game_postprocessing_ds[ CRUDE_GRAPHICS_MAX_SWAPCHAIN_IMAGES ];
  crude_gfx_game_postprocessing_pass_options               options;
} crude_gfx_game_postprocessing_pass;

CRUDE_API void
crude_gfx_game_postprocessing_pass_initialize
(
  _In_ crude_gfx_game_postprocessing_pass                 *pass,
  _In_ crude_gfx_scene_renderer                           *scene_renderer
);

CRUDE_API void
crude_gfx_game_postprocessing_pass_deinitialize
(
  _In_ crude_gfx_game_postprocessing_pass                 *pass
);

CRUDE_API void
crude_gfx_game_postprocessing_pass_render
(
  _In_ void                                               *ctx,
  _In_ crude_gfx_cmd_buffer                               *primary_cmd
);

CRUDE_API void
crude_gfx_game_postprocessing_pass_on_techniques_reloaded
(
  _In_ void                                               *ctx
);

CRUDE_API crude_gfx_render_graph_pass_container
crude_gfx_game_postprocessing_pass_pack
(
  _In_ crude_gfx_game_postprocessing_pass                 *pass
);