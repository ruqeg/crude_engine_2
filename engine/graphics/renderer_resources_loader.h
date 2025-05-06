#pragma once

#include <graphics/renderer.h>
#include <graphics/render_graph.h>

typedef struct crude_gfx_renderer_resource_loader
{
  crude_gfx_renderer                                      *renderer;
  crude_gfx_render_graph                                  *render_graph;
  crude_stack_allocator                                   *temporary_allocator;
} crude_gfx_renderer_resource_loader;

CRUDE_API void
crude_gfx_renderer_resource_loader_initialize
(
  _In_ crude_gfx_renderer_resource_loader                 *resource_loader,
  _In_ crude_gfx_renderer                                 *renderer,
  _In_ crude_gfx_render_graph                             *render_graph,
  _In_ crude_stack_allocator                              *temporary_allocator
);

CRUDE_API void
crude_gfx_renderer_resource_loader_deinitialize
(
  _In_ crude_gfx_renderer_resource_loader                 *resource_loader
);

CRUDE_API void
crude_gfx_renderer_resource_loader_load_technique
(
  _In_ crude_gfx_renderer_resource_loader                 *resource_loader,
  _In_ char const                                         *json_path
);