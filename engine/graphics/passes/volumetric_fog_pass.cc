#include <engine/core/profiler.h>
#include <engine/scene/scene_ecs.h>
#include <engine/graphics/scene_renderer.h>

#define COMPUTE_INJECT_DATA
#include <engine/graphics/shaders/volumetric_fog.crude_shader>

#include <engine/graphics/passes/volumetric_fog_pass.h>

void
crude_gfx_volumetric_fog_pass_initialize
(
  _In_ crude_gfx_volumetric_fog_pass                      *pass,
  _In_ crude_gfx_scene_renderer                           *scene_renderer
)
{
  pass->scene_renderer = scene_renderer;

  crude_gfx_volumetric_fog_pass_on_resize( pass, pass->scene_renderer->gpu->renderer_size.x, pass->scene_renderer->gpu->renderer_size.y );
}

void
crude_gfx_volumetric_fog_pass_deinitialize
(
  _In_ crude_gfx_volumetric_fog_pass                                 *pass
)
{
}

void
crude_gfx_volumetric_fog_pass_render
(
  _In_ void                                               *ctx,
  _In_ crude_gfx_cmd_buffer                               *primary_cmd
)
{
  crude_gfx_volumetric_fog_pass                           *pass;
  crude_gfx_device                                        *gpu;

  pass = CRUDE_REINTERPRET_CAST( crude_gfx_volumetric_fog_pass*, ctx );
  gpu = pass->scene_renderer->gpu;
}

void
crude_gfx_volumetric_fog_pass_on_resize
(
  _In_ void                                               *ctx,
  _In_ uint32                                              new_width,
  _In_ uint32                                              new_height
)
{
}

crude_gfx_render_graph_pass_container
crude_gfx_volumetric_fog_pass_pack
(
  _In_ crude_gfx_volumetric_fog_pass                                 *pass
)
{
  crude_gfx_render_graph_pass_container container = crude_gfx_render_graph_pass_container_empty();
  container.ctx = pass;
  container.on_resize = crude_gfx_volumetric_fog_pass_on_resize;
  container.render = crude_gfx_volumetric_fog_pass_render;
  return container;
}