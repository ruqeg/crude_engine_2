#pragma once

#include <graphics/render_graph.h>
#include <graphics/scene_renderer_resources.h>

typedef struct crude_gfx_scene_renderer crude_gfx_scene_renderer;

typedef struct crude_gfx_mesh_pass
{
  crude_gfx_scene_renderer                                *scene_renderer;
  uint32                                                   use_secondary;
} crude_gfx_mesh_pass;

CRUDE_API void
crude_gfx_mesh_pass_initialize
(
  _In_ crude_gfx_mesh_pass                                *pass,
  _In_ crude_gfx_scene_renderer                           *scene_renderer,
  _In_ uint32                                              use_secondary
);

CRUDE_API void
crude_gfx_mesh_pass_deinitialize
(
  _In_ crude_gfx_mesh_pass                                *pass
);

CRUDE_API void
crude_gfx_mesh_pass_render
(
  _In_ void                                               *ctx,
  _In_ crude_gfx_cmd_buffer                               *primary_cmd
);

CRUDE_API crude_gfx_render_graph_pass_container
crude_gfx_mesh_pass_pack
(
  _In_ crude_gfx_mesh_pass                                *pass
);