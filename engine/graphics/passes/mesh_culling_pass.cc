#include <core/hash_map.h>
#include <graphics/scene_renderer.h>

#include <graphics/passes/mesh_culling_pass.h>

void
crude_gfx_mesh_culling_pass_initialize
(
  _In_ crude_gfx_mesh_culling_pass                        *pass,
  _In_ crude_gfx_scene_renderer                           *scene_renderer
)
{
  pass->scene_renderer = scene_renderer;
}

void
crude_gfx_mesh_culling_pass_deinitialize
(
  _In_ crude_gfx_mesh_culling_pass                        *pass
)
{
}

void
crude_gfx_mesh_culling_pass_on_render_graph_registered
(
  _In_ crude_gfx_mesh_culling_pass                        *pass
)
{
  crude_gfx_device *gpu = pass->scene_renderer->renderer->gpu;
}

void
crude_gfx_mesh_culling_pass_render
(
  _In_ void                                               *ctx,
  _In_ crude_gfx_cmd_buffer                               *primary_cmd
)
{
}

void
crude_gfx_mesh_culling_pass_register
(
  _In_ crude_gfx_mesh_culling_pass                        *pass
)
{
}

static void
crude_gfx_mesh_culling_pass_on_resize
(
  _In_ void                                               *ctx,
  _In_ crude_gfx_device                                   *gpu,
  _In_ uint32                                              new_width,
  _In_ uint32                                              new_height
)
{
}

crude_gfx_render_graph_pass_container
crude_gfx_mesh_culling_pass_pack
(
  _In_ crude_gfx_mesh_culling_pass                        *pass
)
{
  crude_gfx_render_graph_pass_container container;
  container.ctx = pass;
  container.on_resize = crude_gfx_mesh_culling_pass_on_resize;
  container.render = crude_gfx_mesh_culling_pass_render;
  return container;
}