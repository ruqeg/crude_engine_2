#include <engine/core/profiler.h>
#include <engine/core/hash_map.h>
#include <engine/scene/scene_ecs.h>
#include <engine/graphics/scene_renderer.h>

#include <engine/graphics/passes/ssr_pass.h>

void
crude_gfx_ssr_pass_initialize
(
  _In_ crude_gfx_ssr_pass                                 *pass,
  _In_ crude_gfx_scene_renderer                           *scene_renderer
)
{
  pass->scene_renderer = scene_renderer;
}

void
crude_gfx_ssr_pass_deinitialize
(
  _In_ crude_gfx_ssr_pass                                 *pass
)
{
}

void
crude_gfx_ssr_pass_render
(
  _In_ void                                               *ctx,
  _In_ crude_gfx_cmd_buffer                               *primary_cmd
)
{
  CRUDE_ALIGNED_STRUCT( 16 ) push_constant_
  {
    VkDeviceAddress                                        scene;
    float32                                                ssr_max_steps;
    float32                                                ssr_max_distance;

    float32                                                ssr_stride;
    float32                                                ssr_z_thickness;
    float32                                                ssr_stride_zcutoff;
    uint32                                                 depth_texture_index;

    XMFLOAT2                                               depth_texture_size;
    uint32                                                 normal_texture_index;
    uint32                                                 pbr_without_ssr_texture_index;

    uint32                                                 pbr_with_ssr_texture_index;
    XMFLOAT3                                               _padding;
  };
  
  crude_gfx_ssr_pass                                      *pass;
  crude_gfx_device                                        *gpu;
  crude_gfx_texture                                       *depth_texture;
  crude_gfx_pipeline_handle                                pipeline;
  push_constant_                                           pust_constant;

  pass = CRUDE_REINTERPRET_CAST( crude_gfx_ssr_pass*, ctx );
  gpu = pass->scene_renderer->gpu;

  pipeline = crude_gfx_access_technique_pass_by_name( gpu, "compute", "ssr" )->pipeline;
  crude_gfx_cmd_bind_pipeline( primary_cmd, pipeline );
  
  depth_texture = crude_gfx_access_texture( gpu, CRUDE_GFX_PASS_TEXTURE_HANDLE( ssr_pass.depth_texture ) );

  pust_constant = CRUDE_COMPOUNT_EMPTY( push_constant_ );
  pust_constant.scene = pass->scene_renderer->scene_hga.gpu_address;
  pust_constant.ssr_max_steps = pass->scene_renderer->options.ssr_pass.max_steps;
  pust_constant.ssr_max_distance = pass->scene_renderer->options.ssr_pass.max_distance;
  pust_constant.ssr_stride_zcutoff = pass->scene_renderer->options.ssr_pass.stride_zcutoff;
  pust_constant.ssr_stride = pass->scene_renderer->options.ssr_pass.stride;
  pust_constant.ssr_z_thickness = pass->scene_renderer->options.ssr_pass.z_thickness;
  pust_constant.depth_texture_index = depth_texture->handle.index;
  pust_constant.depth_texture_size.x = depth_texture->width;
  pust_constant.depth_texture_size.y = depth_texture->height;
  pust_constant.normal_texture_index = CRUDE_GFX_PASS_TEXTURE_INDEX( ssr_pass.normal_texture );
  pust_constant.pbr_without_ssr_texture_index = CRUDE_GFX_PASS_TEXTURE_INDEX( ssr_pass.pbr_without_ssr_texture );
  pust_constant.pbr_with_ssr_texture_index = CRUDE_GFX_PASS_TEXTURE_INDEX( ssr_pass.pbr_with_ssr_texture );

  crude_gfx_cmd_push_constant( primary_cmd, &pust_constant, sizeof( pust_constant ) );

  crude_gfx_cmd_bind_bindless_descriptor_set( primary_cmd );

  crude_gfx_cmd_dispatch( primary_cmd, ( depth_texture->width + 7 ) / 8, ( depth_texture->height + 7 ) / 8, 1u );
}

crude_gfx_render_graph_pass_container
crude_gfx_ssr_pass_pack
(
  _In_ crude_gfx_ssr_pass                                 *pass
)
{
  crude_gfx_render_graph_pass_container container = crude_gfx_render_graph_pass_container_empty();
  container.ctx = pass;
  container.render = crude_gfx_ssr_pass_render;
  return container;
}