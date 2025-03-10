#pragma once

#include <core/math.h>
#include <graphics/renderer.h>

CRUDE_API typedef struct crude_mesh_draw
{
  crude_float3           scale;
} crude_mesh_draw;

CRUDE_API typedef struct crude_scene
{
  crude_buffer_resource *buffers;
  crude_mesh_draw       *mesh_draws;
} crude_scene;

CRUDE_API void
crude_load_gltf_from_file
(
  _In_ crude_renderer  *renderer,
  _In_ char const      *path,
  _Out_ crude_scene    *scene
);

CRUDE_API void
crude_unload_gltf_from_file
(
  _In_ crude_renderer  *renderer,
  _In_ crude_scene     *scene
);