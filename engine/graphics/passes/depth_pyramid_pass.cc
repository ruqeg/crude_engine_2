#include <graphics/passes/depth_pyramid_pass.h>

void
crude_gfx_depth_pyramid_pass_initialize
(
  _In_ crude_gfx_depth_pyramid_pass                       *pass,
  _In_ crude_gfx_scene_renderer                           *scene_renderer
)
{
}

void
crude_gfx_depth_pyramid_pass_deinitialize
(
  _In_ crude_gfx_depth_pyramid_pass                       *pass
)
{
}

void
crude_gfx_depth_pyramid_pass_render
(
  _In_ void                                               *ctx,
  _In_ crude_gfx_cmd_buffer                               *primary_cmd
)
{
}

static void
crude_gfx_depth_pyramid_pass_on_resize
(
  _In_ void                                               *ctx,
  _In_ crude_gfx_device                                   *gpu,
  _In_ uint32                                              new_width,
  _In_ uint32                                              new_height
)
{
}

crude_gfx_render_graph_pass_container
crude_gfx_depth_pyramid_pass_pack
(
  _In_ crude_gfx_depth_pyramid_pass                       *pass
)
{
  crude_gfx_render_graph_pass_container container;
  container.ctx = pass;
  container.on_resize = crude_gfx_depth_pyramid_pass_on_resize;
  container.render = crude_gfx_depth_pyramid_pass_render;
  return container;
}