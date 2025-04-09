#pragma once

#include <core/math.h>
#include <graphics/renderer.h>

typedef struct crude_mesh_draw
{
  crude_float3                 scale;
  crude_buffer_handle          index_buffer;
  crude_buffer_handle          position_buffer;
  crude_buffer_handle          tangent_buffer;
  crude_buffer_handle          normal_buffer;
  crude_buffer_handle          texcoord_buffer;
  uint32                       index_offset;
  uint32                       position_offset;
  uint32                       tangent_offset;
  uint32                       normal_offset;
  uint32                       texcoord_offset;
  uint32                       primitive_count;
  crude_descriptor_set_handle  descriptor_set;
} crude_mesh_draw;

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