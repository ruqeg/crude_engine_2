#pragma once

#include <engine/scene/scene_ecs.h>
#include <engine/graphics/gpu_resources.h>
#include <engine/graphics/shaders/common/scene.h>

typedef CRUDE_ALIGNED_STRUCT( 16 ) crude_gfx_debug_cube_instance_gpu
{
  XMFLOAT3                                                 translation;
  uint32                                                   color;
  XMFLOAT3                                                 scale;
  uint32                                                   padding;
} crude_gfx_debug_cube_instance_gpu;

typedef CRUDE_ALIGNED_STRUCT( 16 ) crude_gfx_debug_line_vertex_gpu
{
  XMFLOAT3                                                 position;
  uint32                                                   color;
} crude_gfx_debug_line_vertex_gpu;

typedef CRUDE_ALIGNED_STRUCT( 16 ) crude_gfx_debug_draw_command_gpu
{
  VkDrawIndirectCommand                                    draw_indirect_3dline;
  VkDrawIndirectCommand                                    draw_indirect_2dline;
  VkDrawIndirectCommand                                    draw_indirect_cube;
} crude_gfx_debug_draw_command_gpu;

typedef CRUDE_ALIGNED_STRUCT( 16 ) crude_gfx_mesh_draw_counts_gpu
{
  uint32                                                   opaque_mesh_visible_early_count;
  uint32                                                   opaque_mesh_visible_late_count;
  uint32                                                   opaque_mesh_culled_count;
  uint32                                                   transparent_mesh_visible_count;

  uint32                                                   transparent_mesh_culled_count;
  uint32                                                   total_mesh_count;
  uint32                                                   depth_pyramid_texture_index;
  uint32                                                   meshlet_index_count;

  uint32                                                   dispatch_task_x;
  uint32                                                   dispatch_task_y;
  uint32                                                   dispatch_task_z;
  uint32                                                   meshlet_instances_count;
} crude_gfx_mesh_draw_counts_gpu;

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