#pragma once

#include <graphics/gpu_device.h>
#include <core/ecs.h>

typedef enum crude_gfx_mesh_draw_flags
{
  CRUDE_GFX_MESH_DRAW_FLAGS_HAS_NORMAL = ( 1 << 4 ),
  CRUDE_GFX_MESH_DRAW_FLAGS_HAS_TANGENTS = ( 1 << 8 ),
} crude_gfx_mesh_draw_flags;

typedef CRUDE_ALIGNED_STRUCT( 16 ) crude_gfx_meshlet_gpu
{
  XMFLOAT3                                                 center;
  float32                                                  radius;
  int8                                                     cone_axis[ 3 ];
  int8                                                     cone_cutoff;
  uint32                                                   vertices_offset;
  uint32                                                   triangles_offset;
  uint8                                                    vertices_count;
  uint8                                                    triangles_count;
  uint32                                                   mesh_index;
} crude_gfx_meshlet_gpu;

typedef CRUDE_ALIGNED_STRUCT( 16 ) crude_gfx_mesh_draw_gpu
{
  XMUINT4                                                  textures;
  XMFLOAT4                                                 emissive;
  XMFLOAT4                                                 albedo_color_factor;
  XMFLOAT4                                                 metallic_roughness_occlusion_factor;

  uint32                                                   flags;
  float32                                                  alpha_cutoff;
  uint32                                                   vertices_offset;
  uint32                                                   mesh_index;

  uint32                                                   meshletes_offset;
  uint32                                                   meshletes_count;
  uint32                                                   meshletes_index_count;
  uint32                                                   padding1;

  uint64                                                   position_buffer;
  uint64                                                   texcoord_buffer;
  uint64                                                   index_buffer;
  uint64                                                   normal_buffer;
} crude_gfx_mesh_draw_gpu;

typedef struct crude_gfx_meshlet_vertex_gpu
{
  XMFLOAT3A                                                position;
  uint8                                                    normal[ 4 ];
  uint8                                                    tangent[ 4 ];
  uint16                                                   texcoords[ 2 ];
  float32                                                  padding;
} crude_gfx_meshlet_vertex_gpu;

typedef struct crude_gfx_mesh_cpu
{
  XMFLOAT4                                                 bounding_sphere;
  crude_gfx_buffer_handle                                  index_buffer;
  crude_gfx_buffer_handle                                  position_buffer;
  crude_gfx_buffer_handle                                  tangent_buffer;
  crude_gfx_buffer_handle                                  normal_buffer;
  crude_gfx_buffer_handle                                  texcoord_buffer;
  uint32                                                   index_offset;
  uint32                                                   position_offset;
  uint32                                                   tangent_offset;
  uint32                                                   normal_offset;
  uint32                                                   texcoord_offset;
  uint32                                                   primitive_count;
  XMFLOAT4                                                 albedo_color_factor;
  XMFLOAT3                                                 metallic_roughness_occlusion_factor;
  float32                                                  alpha_cutoff;
  uint32                                                   flags;
  crude_gfx_texture_handle                                 albedo_texture_handle;
  crude_gfx_texture_handle                                 metallic_roughness_texture_handle;
  crude_gfx_texture_handle                                 normal_texture_handle;
  crude_gfx_texture_handle                                 occlusion_texture_handle;
  uint32                                                   gpu_mesh_index;
  uint32                                                   meshlets_offset;
  uint32                                                   meshlets_count;
} crude_gfx_mesh_cpu;

typedef struct crude_gfx_mesh_instance_cpu
{
  uint32                                                   mesh_gpu_index;
  crude_entity                                             node;
} crude_gfx_mesh_instance_cpu;

typedef CRUDE_ALIGNED_STRUCT( 16 ) crude_gfx_mesh_instance_draw_gpu
{
  XMFLOAT4X4                                               mesh_to_world;
  XMFLOAT4X4                                               world_to_mesh;
  uint32                                                   mesh_draw_index;
  XMFLOAT3A                                                padding;
} crude_gfx_mesh_instance_draw_gpu;

typedef struct crude_gfx_model_renderer_resources
{
  crude_entity                                             main_node;
  crude_gfx_mesh_instance_cpu                             *meshes_instances;
} crude_gfx_model_renderer_resources;

CRUDE_API void
crude_gfx_mesh_cpu_to_mesh_draw_gpu
(
  _In_ crude_gfx_device                                   *gpu,
  _In_ crude_gfx_mesh_cpu const                           *mesh,
  _Out_ crude_gfx_mesh_draw_gpu                           *mesh_draw_gpu
);