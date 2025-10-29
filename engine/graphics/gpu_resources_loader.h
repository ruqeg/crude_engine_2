#pragma once

#include <graphics/scene_renderer.h>

CRUDE_API void
crude_gfx_technique_load_from_file
(
  _In_ char const                                         *json_name,
  _In_ crude_gfx_device                                   *gpu,
  _In_ crude_gfx_render_graph                             *render_graph,
  _In_ crude_stack_allocator                              *temporary_allocator
);