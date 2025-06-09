#pragma once

#include <graphics/scene_renderer.h>

CRUDE_API void
crude_scene_renderer_upload_gltf
(
  _In_ crude_gfx_scene_renderer                          *scene_renderer,
  _In_ char const                                        *gltf_path,
  _In_ crude_entity                                       parent_node,
  _In_ crude_stack_allocator                             *temporary_allocator
);