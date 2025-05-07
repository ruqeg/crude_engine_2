#pragma once

#include <graphics/renderer.h>
#include <graphics/render_graph.h>

CRUDE_API void
crude_gfx_renderer_resource_load_technique
(
  _In_ char const                                         *json_name,
  crude_gfx_renderer                                      *renderer,
  crude_gfx_render_graph                                  *render_graph,
  crude_stack_allocator                                   *temporary_allocator
);