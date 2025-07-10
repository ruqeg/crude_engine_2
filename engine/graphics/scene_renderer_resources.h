#pragma once

#include <core/ecs.h>
#include <graphics/renderer_resources.h>

#define CRUDE_GFX_MAX_RENDERER_SCENE_PATH_LEN              ( 512 )

typedef CRUDE_ALIGNED_STRUCT( 16 ) crude_gfx_scene_constant_gpu
{
  XMFLOAT3A                                       camera_position;
  XMFLOAT4A                                       camera_frustum_planes[ 6 ];
  XMFLOAT4X4A                                     world_to_view;
  XMFLOAT4X4A                                     view_to_clip;
  XMFLOAT4X4A                                     clip_to_view;
  XMFLOAT4X4A                                     view_to_world;
} crude_gfx_scene_constant_gpu;

typedef CRUDE_ALIGNED_STRUCT( 16 ) crude_gfx_mesh_draw_command_gpu
{
  uint32                                                   draw_id;
  VkDrawIndexedIndirectCommand                             indirect;
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
  uint32                                                   late_flag;
  uint32                                                   meshlet_index_count;

  uint32                                                   dispatch_task_x;
  uint32                                                   dispatch_task_y;
  uint32                                                   dispatch_task_z;
  uint32                                                   pad001;
} crude_gfx_mesh_draw_counts_gpu;

typedef CRUDE_ALIGNED_STRUCT( 16 ) crude_gfx_meshlet_gpu
{
  XMFLOAT3                                             center;
  float32                                                  radius;
  int8                                                     cone_axis[ 3 ];
  int8                                                     cone_cutoff;
  uint32                                                   vertices_offset;
  uint32                                                   triangles_offset;
  uint8                                                    vertices_count;
  uint8                                                    triangles_count;
  uint32                                                   mesh_index;
} crude_gfx_meshlet_gpu;

typedef CRUDE_ALIGNED_STRUCT( 16 ) crude_gfx_mesh_material_gpu
{
  XMFLOAT4X4A                                     model_to_world;

  XMUINT4                                         textures;
  XMFLOAT4                                        emissive;
  XMFLOAT4                                        albedo_color_factor;
  XMFLOAT4                                        metallic_roughness_occlusion_factor;

  uint32                                                   flags;
  float32                                                  alpha_cutoff;
  uint32                                                   vertices_offset;
  uint32                                                   mesh_index;

  uint32                                                   meshletes_offset;
  uint32                                                   meshletes_count;
  uint32                                                   meshletes_index_count;
  uint32                                                   padding1;
} crude_gfx_mesh_material_gpu;

typedef struct crude_gfx_meshlet_vertex_gpu
{
  XMFLOAT3A                                       position;
  uint8                                                    normal[ 4 ];
  uint8                                                    tangent[ 4 ];
  uint16                                                   texcoords[ 2 ];
  float32                                                  padding;
} crude_gfx_meshlet_vertex_gpu;

typedef struct crude_gfx_mesh_cpu
{
  crude_entity                                             node;
  crude_gfx_renderer_material                             *material;
  XMFLOAT3                                             scale;
  XMFLOAT3                                             translation;
  XMFLOAT4                                             rotation;
  crude_gfx_buffer_handle                                  index_buffer;
  crude_gfx_buffer_handle                                  position_buffer;
  crude_gfx_buffer_handle                                  tangent_buffer;
  crude_gfx_buffer_handle                                  normal_buffer;
  crude_gfx_buffer_handle                                  texcoord_buffer;
  crude_gfx_buffer_handle                                  material_buffer;
  uint32                                                   index_offset;
  uint32                                                   position_offset;
  uint32                                                   tangent_offset;
  uint32                                                   normal_offset;
  uint32                                                   texcoord_offset;
  uint32                                                   primitive_count;
  XMFLOAT4                                             albedo_color_factor;
  XMFLOAT3                                             metallic_roughness_occlusion_factor;
  float32                                                  alpha_cutoff;
  uint32                                                   flags;
  uint16                                                   albedo_texture_index;
  uint16                                                   roughness_texture_index;
  uint16                                                   normal_texture_index;
  uint16                                                   occlusion_texture_index;
  uint32                                                   gpu_mesh_index;
  uint32                                                   meshlets_offset;
  uint32                                                   meshlets_count;
} crude_gfx_mesh_cpu;

typedef struct crude_gfx_mesh_instance_cpu
{
  crude_gfx_mesh_cpu                                      *mesh;
  uint32                                                   material_pass_index;
} crude_gfx_mesh_instance_cpu;

CRUDE_API bool
crude_gfx_mesh_is_transparent
(
  _In_ crude_gfx_mesh_cpu                                 *mesh
);