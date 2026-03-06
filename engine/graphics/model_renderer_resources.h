#pragma once

#include <engine/graphics/gpu_device.h>
#include <engine/graphics/shaders/common/scene.h>

typedef struct crude_gfx_mesh_cpu
{
  XMFLOAT4                                                 bounding_sphere;
  crude_gfx_memory_allocation                              index_hga;
  crude_gfx_memory_allocation                              position_hga;
  crude_gfx_memory_allocation                              tangent_hga;
  crude_gfx_memory_allocation                              normal_hga;
  crude_gfx_memory_allocation                              texcoord_hga;
  uint32                                                   index_offset;
  uint32                                                   position_offset;
  uint32                                                   tangent_offset;
  uint32                                                   normal_offset;
  uint32                                                   texcoord_offset;
  uint32                                                   indices_count;
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

typedef struct crude_gfx_node
{
  crude_gfx_node                                          *parent;
} crude_gfx_node;

typedef struct crude_gfx_mesh_instance_cpu
{
  XMFLOAT4X4                                               mesh_to_model;
  uint32                                                   mesh_gpu_index;
  uint32                                                   joints_matrices_offset;
} crude_gfx_mesh_instance_cpu;

typedef struct crude_gfx_model_renderer_resources
{
  crude_gfx_mesh_instance_cpu                             *meshes_instances;
} crude_gfx_model_renderer_resources;

CRUDE_API void
crude_gfx_mesh_cpu_to_mesh_draw_gpu
(
  _In_ crude_gfx_device                                   *gpu,
  _In_ crude_gfx_mesh_cpu const                           *mesh,
  _Out_ crude_gfx_mesh_draw                               *mesh_draw_gpu
);