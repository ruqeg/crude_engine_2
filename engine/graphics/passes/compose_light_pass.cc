#include <engine/core/profiler.h>
#include <engine/core/hash_map.h>
#include <engine/scene/scene_ecs.h>
#include <engine/graphics/scene_renderer.h>

#include <engine/graphics/passes/compose_light_pass.h>

void
crude_gfx_compose_light_pass_initialize
(
  _In_ crude_gfx_compose_light_pass                       *pass,
  _In_ crude_gfx_scene_renderer                           *scene_renderer
)
{
  pass->scene_renderer = scene_renderer;
}

void
crude_gfx_compose_light_pass_deinitialize
(
  _In_ crude_gfx_compose_light_pass                       *pass
)
{
}

void
crude_gfx_compose_light_pass_render
(
  _In_ void                                               *ctx,
  _In_ crude_gfx_cmd_buffer                               *primary_cmd
)
{
  CRUDE_ALIGNED_STRUCT( 16 ) push_constant_
  {
    uint32                                                 direct_radiance_texture_index;
    uint32                                                 ssr_texture_index;
    uint32                                                 output_texture_index;
    uint32                                                 packed_roughness_metalness_texture_index;
  };
  
  crude_gfx_compose_direct_light_pass                     *pass;
  crude_gfx_device                                        *gpu;
  crude_gfx_pipeline_handle                                pipeline;
  push_constant_                                           pust_constant;

  pass = CRUDE_REINTERPRET_CAST( crude_gfx_compose_direct_light_pass*, ctx );
  gpu = pass->scene_renderer->gpu;

  pipeline = crude_gfx_access_technique_pass_by_name( gpu, "compute", "compose_light" )->pipeline;
  crude_gfx_cmd_bind_pipeline( primary_cmd, pipeline );
  
  pust_constant = CRUDE_COMPOUNT_EMPTY( push_constant_ );
  pust_constant.direct_radiance_texture_index = CRUDE_GFX_PASS_TEXTURE_INDEX( compose_light_pass.direct_radiance_texture );
  pust_constant.ssr_texture_index = CRUDE_GFX_PASS_TEXTURE_INDEX( compose_light_pass.ssr_texture );
  pust_constant.output_texture_index = CRUDE_GFX_PASS_TEXTURE_INDEX( compose_light_pass.output_texture );
  pust_constant.packed_roughness_metalness_texture_index = CRUDE_GFX_PASS_TEXTURE_INDEX( compose_light_pass.packed_roughness_metalness_texture );
  crude_gfx_cmd_push_constant( primary_cmd, &pust_constant, sizeof( pust_constant ) );

  crude_gfx_cmd_bind_bindless_descriptor_set( primary_cmd );

  crude_gfx_cmd_dispatch( primary_cmd, ( pass->scene_renderer->gpu->renderer_size.x + 7 ) / 8, ( pass->scene_renderer->gpu->renderer_size.y + 7 ) / 8, 1u );
}

crude_gfx_render_graph_pass_container
crude_gfx_compose_light_pass_pack
(
  _In_ crude_gfx_compose_light_pass                       *pass
)
{
  crude_gfx_render_graph_pass_container container = crude_gfx_render_graph_pass_container_empty();
  container.ctx = pass;
  container.render = crude_gfx_compose_light_pass_render;
  return container;
}