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

CRUDE_API void
crude_gfx_camera_to_camera_gpu
(
  _In_ crude_camera                                       *camera,
  _In_ XMFLOAT4X4                                          camera_view_to_world,
  _Out_ crude_gfx_camera                                  *camera_gpu
);