#include <graphics/passes/collision_pass.h>

void
crude_gfx_collision_pass_initialize
(
  _In_ crude_gfx_collision_pass                           *pass,
  _In_ crude_gfx_scene_renderer                           *scene_renderer
)
{
}

void
crude_gfx_collision_pass_deinitialize
(
  _In_ crude_gfx_collision_pass                           *pass
)
{
}

void
crude_gfx_collision_pass_render
(
  _In_ void                                               *ctx,
  _In_ crude_gfx_cmd_buffer                               *primary_cmd
)
{
}

void
crude_gfx_collision_pass_on_techniques_reloaded
(
  _In_ void                                               *ctx
)
{
}

crude_gfx_render_graph_pass_container
crude_gfx_collision_pass_pack
(
  _In_ crude_gfx_collision_pass                           *pass
)
{
  crude_gfx_render_graph_pass_container container = crude_gfx_render_graph_pass_container_empty();
  container.ctx = pass;
  container.render = crude_gfx_collision_pass_render;
  container.on_techniques_reloaded = crude_gfx_collision_pass_on_techniques_reloaded;
  return container;
}