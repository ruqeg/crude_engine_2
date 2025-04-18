#pragma once

#include <core/math.h>
#include <graphics/renderer.h>

typedef struct crude_mesh_draw
{
  crude_material              *material;
  crude_float3                 scale;
  crude_buffer_handle          index_buffer;
  crude_buffer_handle          position_buffer;
  crude_buffer_handle          tangent_buffer;
  crude_buffer_handle          normal_buffer;
  crude_buffer_handle          texcoord_buffer;
  crude_buffer_handle          material_buffer;
  uint32                       index_offset;
  uint32                       position_offset;
  uint32                       tangent_offset;
  uint32                       normal_offset;
  uint32                       texcoord_offset;
  uint32                       primitive_count;
  crude_float4                 base_color_factor;
  crude_float3                 metallic_roughness_occlusion_factor;
  float32                      alpha_cutoff;
  uint32                       flags;
  uint16                       albedo_texture_index;
  uint16                       roughness_texture_index;
  uint16                       normal_texture_index;
  uint16                       occlusion_texture_index;
} crude_mesh_draw;

typedef struct crude_mesh_data
{
  crude_float4x4a              m;
  crude_float4x4a              inverse_m;
  uint32                       textures[ 4 ];
  crude_float4a                base_color_factor;
  crude_float4a                metallic_roughness_occlusion_factor;
  crude_float1a                alpha_cutoff;
  uint32                       flags;
} crude_mesh_data;

typedef struct crude_scene
{
  crude_sampler_resource      *samplers;
  crude_texture_resource      *images;
  crude_buffer_resource       *buffers;
  crude_mesh_draw             *mesh_draws;
} crude_scene;

CRUDE_API void
crude_load_gltf_from_file
(
  _In_ crude_renderer         *renderer,
  _In_ char const             *path,
  _Out_ crude_scene           *scene
);

CRUDE_API void
crude_unload_gltf_from_file
(
  _In_ crude_renderer         *renderer,
  _In_ crude_scene            *scene
);