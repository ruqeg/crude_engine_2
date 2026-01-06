#include <engine/core/profiler.h>
#include <engine/core/hash_map.h>
#include <engine/scene/scene_ecs.h>
#include <engine/graphics/scene_renderer.h>

#include <engine/graphics/passes/compose_pass.h>

void
crude_gfx_compose_pass_initialize
(
  _In_ crude_gfx_compose_pass                             *pass,
  _In_ crude_gfx_scene_renderer                           *scene_renderer
)
{
  pass->scene_renderer = scene_renderer;
}

void
crude_gfx_compose_pass_deinitialize
(
  _In_ crude_gfx_compose_pass                             *pass
)
{
}

void
crude_gfx_compose_pass_pre_render
(
  _In_ void                                               *ctx,
  _In_ crude_gfx_cmd_buffer                               *primary_cmd
)
{
  crude_gfx_compose_pass                                  *pass;
  pass = CRUDE_REINTERPRET_CAST( crude_gfx_compose_pass*, ctx );
  crude_gfx_cmd_add_buffer_barrier( primary_cmd, pass->scene_renderer->lights_bins_hga.buffer_handle, CRUDE_GFX_RESOURCE_STATE_COPY_DEST, CRUDE_GFX_RESOURCE_STATE_SHADER_RESOURCE );
  crude_gfx_cmd_add_buffer_barrier( primary_cmd, pass->scene_renderer->lights_tiles_hga.buffer_handle, CRUDE_GFX_RESOURCE_STATE_COPY_DEST, CRUDE_GFX_RESOURCE_STATE_SHADER_RESOURCE );
  crude_gfx_cmd_add_buffer_barrier( primary_cmd, pass->scene_renderer->lights_indices_hga.buffer_handle, CRUDE_GFX_RESOURCE_STATE_COPY_DEST, CRUDE_GFX_RESOURCE_STATE_SHADER_RESOURCE );
  crude_gfx_cmd_add_buffer_barrier( primary_cmd, pass->scene_renderer->lights_hga.buffer_handle, CRUDE_GFX_RESOURCE_STATE_COPY_DEST, CRUDE_GFX_RESOURCE_STATE_SHADER_RESOURCE );
  crude_gfx_cmd_add_buffer_barrier( primary_cmd, pass->scene_renderer->lights_world_to_clip_hga.buffer_handle, CRUDE_GFX_RESOURCE_STATE_COPY_DEST, CRUDE_GFX_RESOURCE_STATE_SHADER_RESOURCE );
}

void
crude_gfx_compose_pass_render
(
  _In_ void                                               *ctx,
  _In_ crude_gfx_cmd_buffer                               *primary_cmd
)
{
  CRUDE_ALIGNED_STRUCT( 16 ) push_constant_
  {
    XMUINT4                                                textures;
    VkDeviceAddress                                        scene;
    VkDeviceAddress                                        zbins;
    VkDeviceAddress                                        lights_tiles;
    VkDeviceAddress                                        lights_indices;
    VkDeviceAddress                                        lights;
    VkDeviceAddress                                        light_shadow_views;
  };
  
  crude_gfx_compose_pass                                  *pass;
  crude_gfx_device                                        *gpu;
  crude_gfx_pipeline_handle                                pipeline;
  push_constant_                                           pust_constant;

  pass = CRUDE_REINTERPRET_CAST( crude_gfx_compose_pass*, ctx );
  gpu = pass->scene_renderer->gpu;

  pipeline = crude_gfx_access_technique_pass_by_name( gpu, "fullscreen", "compose" )->pipeline;
  crude_gfx_cmd_bind_pipeline( primary_cmd, pipeline );
  
  pust_constant = CRUDE_COMPOUNT_EMPTY( push_constant_ );
  pust_constant.textures.x = CRUDE_GFX_PASS_TEXTURE_INDEX( compose_pass.gbuffer_albedo );
  pust_constant.textures.y = CRUDE_GFX_PASS_TEXTURE_INDEX( compose_pass.gbuffer_normal );
  pust_constant.textures.z = CRUDE_GFX_PASS_TEXTURE_INDEX( compose_pass.gbuffer_roughness_metalness );
  pust_constant.textures.w = CRUDE_GFX_PASS_TEXTURE_INDEX( compose_pass.gbuffer_depth );
  pust_constant.scene = pass->scene_renderer->scene_hga.gpu_address;
  pust_constant.zbins = pass->scene_renderer->lights_bins_hga.gpu_address;
  pust_constant.lights_tiles = pass->scene_renderer->lights_tiles_hga.gpu_address;
  pust_constant.lights_indices = pass->scene_renderer->lights_indices_hga.gpu_address;
  pust_constant.lights = pass->scene_renderer->lights_hga.gpu_address;
  pust_constant.light_shadow_views = pass->scene_renderer->lights_world_to_clip_hga.gpu_address;
  crude_gfx_cmd_push_constant( primary_cmd, &pust_constant, sizeof( pust_constant ) );

  crude_gfx_cmd_bind_bindless_descriptor_set( primary_cmd );

  crude_gfx_cmd_draw( primary_cmd, 0u, 3u, 0u, 1u );
}

crude_gfx_render_graph_pass_container
crude_gfx_compose_pass_pack
(
  _In_ crude_gfx_compose_pass                             *pass
)
{
  crude_gfx_render_graph_pass_container container = crude_gfx_render_graph_pass_container_empty();
  container.ctx = pass;
  container.render = crude_gfx_compose_pass_render;
  container.pre_render = crude_gfx_compose_pass_pre_render;
  return container;
}