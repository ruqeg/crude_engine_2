#pragma once

#include <graphics/render_graph.h>
#include <graphics/scene_renderer_resources.h>

#define CRUDE_GFX_DEPTH_PYRAMID_PASS_MAX_LEVELS ( 16 )

typedef struct crude_gfx_scene_renderer crude_gfx_scene_renderer;

typedef struct crude_gfx_mesh_culling_pass
{
  crude_gfx_scene_renderer                                *scene_renderer;
  crude_gfx_descriptor_set_handle                          mesh_culling_descriptor_sets_handles[ CRUDE_GFX_MAX_SWAPCHAIN_IMAGES ];
  crude_gfx_descriptor_set_layout_handle                   mesh_culling_descriptor_sets_layout_handle;
  bool                                                     early_pass;
} crude_gfx_mesh_culling_pass;

CRUDE_API void
crude_gfx_mesh_culling_pass_initialize
(
  _In_ crude_gfx_mesh_culling_pass                        *pass,
  _In_ crude_gfx_scene_renderer                           *scene_renderer,
  _In_ bool                                                early_pass
);

CRUDE_API void
crude_gfx_mesh_culling_pass_deinitialize
(
  _In_ crude_gfx_mesh_culling_pass                        *pass
);

CRUDE_API void
crude_gfx_mesh_culling_pass_on_render_graph_registered
(
  _In_ crude_gfx_mesh_culling_pass                        *pass
);

CRUDE_API void
crude_gfx_mesh_culling_pass_render
(
  _In_ void                                               *ctx,
  _In_ crude_gfx_cmd_buffer                               *primary_cmd
);

CRUDE_API crude_gfx_render_graph_pass_container
crude_gfx_mesh_culling_pass_pack
(
  _In_ crude_gfx_mesh_culling_pass                        *pass
);