#pragma once

#include <core/ecs.h>
#include <scene/scene_components.h>
#include <graphics/gpu_resources.h>

#define CRUDE_GRAPHICS_SCENE_RENDERER_LIGHT_WORDS_COUNT ( ( CRUDE_GRAPHICS_SCENE_RENDERER_LIGHTS_MAX_COUNT + 31 ) / 32 )

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

typedef CRUDE_ALIGNED_STRUCT( 16 ) crude_gfx_camera_gpu
{
  XMFLOAT4X4A                                              world_to_clip;
  XMFLOAT4X4A                                              world_to_view;
  XMFLOAT4X4A                                              view_to_clip;
  XMFLOAT4X4A                                              clip_to_view;
  XMFLOAT4X4A                                              view_to_world;
  XMFLOAT4X4A                                              clip_to_world;
  XMFLOAT4A                                                frustum_planes_culling[ 6 ];
  XMFLOAT3A                                                position;
  float32                                                  znear;
  float32                                                  zfar;
  XMFLOAT2                                                 padding;
} crude_gfx_camera_gpu;

typedef CRUDE_ALIGNED_STRUCT( 16 ) crude_gfx_scene_constant_gpu
{
  crude_gfx_camera_gpu                                     camera;
  crude_gfx_camera_gpu                                     camera_previous;
  XMFLOAT2                                                 resolution;
  uint32                                                   flags;
  uint32                                                   meshes_instances_count;
  uint32                                                   active_lights_count;
  uint32                                                   tiled_shadowmap_texture_index;
  XMFLOAT2                                                 inv_shadow_map_size;
  XMFLOAT3                                                 background_color;
  float32                                                  background_intensity;
  XMFLOAT3                                                 ambient_color;
  float32                                                  ambient_intensity;
  uint32                                                   indirect_light_texture_index;
} crude_gfx_scene_constant_gpu;

typedef CRUDE_ALIGNED_STRUCT( 16 ) crude_gfx_mesh_draw_command_gpu
{
  uint32                                                   draw_id;
  VkDrawMeshTasksIndirectCommandEXT                        indirect_meshlet;
} crude_gfx_mesh_draw_command_gpu;

typedef CRUDE_ALIGNED_STRUCT( 16 ) crude_gfx_mesh_draw_counts_gpu
{
  uint32                                                   opaque_mesh_visible_count;
  uint32                                                   opaque_mesh_culled_count;
  uint32                                                   transparent_mesh_visible_count;
  uint32                                                   transparent_mesh_culled_count;

  uint32                                                   total_count;
  uint32                                                   depth_pyramid_texture_index;
  uint32                                                   occlusion_culling_late_flag;
  uint32                                                   meshlet_index_count;

  uint32                                                   dispatch_task_x;
  uint32                                                   dispatch_task_y;
  uint32                                                   dispatch_task_z;
  uint32                                                   pad001;
} crude_gfx_mesh_draw_counts_gpu;

typedef struct crude_gfx_light_cpu
{
  crude_entity                                             node;
} crude_gfx_light_cpu;

typedef struct crude_gfx_light_gpu
{
  XMFLOAT3                                                 position;
  float32                                                  radius;
  XMFLOAT3                                                 color;
  float32                                                  intensity;
} crude_gfx_light_gpu;

typedef struct crude_gfx_sorted_light
{
  uint32                                                   light_index;
  float32                                                  projected_z;
  float32                                                  projected_z_min;
  float32                                                  projected_z_max;
} crude_gfx_sorted_light;

CRUDE_API void
crude_gfx_camera_to_camera_gpu
(
  _In_ crude_entity                                        camera_node,
  _Out_ crude_gfx_camera_gpu                              *camera_gpu
);