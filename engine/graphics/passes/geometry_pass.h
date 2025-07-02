#pragma once

#include <graphics/render_graph.h>
#include <graphics/scene_renderer_resources.h>

typedef struct crude_gfx_scene_renderer crude_gfx_scene_renderer;

typedef enum crude_gfx_geometry_pass_flag_bits
{
  CRUDE_GFX_GEOMETRY_PASS_CLASSIC_BIT = 0x00000001,
  CRUDE_GFX_GEOMETRY_PASS_SECONDARY_BIT = 0x00000002,
  CRUDE_GFX_GEOMETRY_PASS_MESHLETS_BIT = 0x00000004,
  CRUDE_GFX_GEOMETRY_PASS_MAX_BIT = 0x00000008
} crude_gfx_geometry_pass_flag_bits;

typedef struct crude_gfx_geometry_pass
{
  crude_gfx_scene_renderer                                *scene_renderer;
  crude_gfx_mesh_instance_cpu                             *mesh_instances;
  uint32                                                   meshlet_technique_index;
  uint32                                                   flags;
} crude_gfx_geometry_pass;

CRUDE_API void
crude_gfx_geometry_pass_initialize
(
  _In_ crude_gfx_geometry_pass                            *pass,
  _In_ crude_gfx_scene_renderer                           *scene_renderer,
  _In_ uint32                                              flags
);

CRUDE_API void
crude_gfx_geometry_pass_deinitialize
(
  _In_ void                                               *ctx,
  _In_ crude_gfx_cmd_buffer                               *primary_cmd
);

CRUDE_API void
crude_gfx_geometry_pass_render
(
  _In_ void                                               *ctx,
  _In_ crude_gfx_cmd_buffer                               *primary_cmd
);

CRUDE_API crude_gfx_render_graph_pass_container
crude_gfx_geometry_pass_pack
(
  _In_ crude_gfx_geometry_pass                            *pass
);