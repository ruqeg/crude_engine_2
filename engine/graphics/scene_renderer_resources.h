#pragma once

#include <engine/scene/scene_ecs.h>
#include <engine/graphics/gpu_resources.h>
#include <engine/graphics/shaders/common/scene.h>

typedef struct crude_gfx_light_cpu
{
  crude_light                                              light;
  XMFLOAT3                                                 translation;
} crude_gfx_light_cpu;

typedef struct crude_gfx_culled_light_cpu
{
  uint32                                                   light_index;
  float32                                                  screen_area;
  XMFLOAT2                                                 tile_position;
  float32                                                  tile_size;
} crude_gfx_culled_light_cpu;

typedef struct crude_gfx_ddgi_area_cpu
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
  XMINT3                                                   probe_count;
  int32                                                    probe_rays;
  int32                                                    offsets_calculations_count;
  bool                                                     use_half_resolution;
  float32                                                  probe_debug_model_scale;
} crude_gfx_ddgi_area_cpu;

typedef struct crude_gfx_world_environment_cpu
{
  XMFLOAT3                                                 background_radiance;
} crude_gfx_world_environment_cpu;

CRUDE_API void
crude_gfx_camera_to_camera_gpu
(
  _In_ crude_camera                                       *camera,
  _In_ XMFLOAT4X4                                          camera_view_to_world,
  _Out_ crude_gfx_camera                                  *camera_gpu
);