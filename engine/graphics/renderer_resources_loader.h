#pragma once

#include <graphics/renderer_scene.h>

typedef struct crude_gfx_renderer_scene_load_info
{
  crude_allocator_container                                allocator_container;
  crude_stack_allocator                                   *temprorary_stack_allocator;
} crude_gfx_renderer_scene_load_info;

CRUDE_API void
crude_gfx_renderer_technique_load_from_file
(
  _In_ char const                                         *json_name,
  _In_ crude_gfx_renderer                                 *renderer,
  _In_ crude_gfx_render_graph                             *render_graph,
  _In_ crude_stack_allocator                              *temporary_allocator
);

CRUDE_API void
crude_gfx_renderer_scene_load_from_file
(
  _In_ crude_gfx_renderer_scene                          *scene,
  _In_ char const                                        *path,
  _In_ crude_stack_allocator                             *temporary_allocator
);