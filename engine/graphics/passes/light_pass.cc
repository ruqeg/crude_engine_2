#include <engine/core/profiler.h>
#include <engine/core/hash_map.h>
#include <engine/scene/scene_components.h>
#include <engine/graphics/scene_renderer.h>

#include <engine/graphics/passes/light_pass.h>

void
crude_gfx_light_pass_initialize
(
  _In_ crude_gfx_light_pass                               *pass,
  _In_ crude_gfx_scene_renderer                           *scene_renderer
)
{
  pass->scene_renderer = scene_renderer;

  for ( uint32 i = 0; i < CRUDE_GRAPHICS_MAX_SWAPCHAIN_IMAGES; ++i )
  {
    pass->light_ds[ i ] = CRUDE_GFX_DESCRIPTOR_SET_HANDLE_INVALID;
  }
  pass->light_cb = CRUDE_GFX_BUFFER_HANDLE_INVALID;
}

void
crude_gfx_light_pass_deinitialize
(
  _In_ crude_gfx_light_pass                               *pass
)
{
  for ( uint32 i = 0; i < CRUDE_GRAPHICS_MAX_SWAPCHAIN_IMAGES; ++i )
  {
    crude_gfx_destroy_descriptor_set( pass->scene_renderer->gpu, pass->light_ds[ i ] );
  }
  crude_gfx_destroy_buffer( pass->scene_renderer->gpu, pass->light_cb );
}

void
crude_gfx_light_pass_on_render_graph_registered
(
  _In_ crude_gfx_light_pass                               *pass
)
{
  crude_gfx_light_constant_gpu                             light_constant;
  crude_gfx_buffer_creation                                buffer_creation;
  
  light_constant = CRUDE_COMPOUNT_EMPTY( crude_gfx_light_constant_gpu );
  light_constant.textures.x = crude_gfx_render_graph_builder_access_resource_by_name( pass->scene_renderer->render_graph->builder, "gbuffer_albedo" )->resource_info.texture.handle.index;
  light_constant.textures.y = crude_gfx_render_graph_builder_access_resource_by_name( pass->scene_renderer->render_graph->builder, "gbuffer_normal" )->resource_info.texture.handle.index;
  light_constant.textures.z = crude_gfx_render_graph_builder_access_resource_by_name( pass->scene_renderer->render_graph->builder, "gbuffer_roughness_metalness" )->resource_info.texture.handle.index;
  light_constant.textures.w = crude_gfx_render_graph_builder_access_resource_by_name( pass->scene_renderer->render_graph->builder, pass->scene_renderer->options.depth_texture_name )->resource_info.texture.handle.index;

  if ( CRUDE_RESOURCE_HANDLE_IS_VALID( pass->light_cb ) )
  {
    crude_gfx_destroy_buffer( pass->scene_renderer->gpu, pass->light_cb );
  }

  buffer_creation = CRUDE_COMPOUNT_EMPTY( crude_gfx_buffer_creation );
  buffer_creation.type_flags = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
  buffer_creation.usage = CRUDE_GFX_RESOURCE_USAGE_TYPE_IMMUTABLE;
  buffer_creation.size = sizeof( crude_gfx_light_constant_gpu );
  buffer_creation.initial_data = &light_constant;
  buffer_creation.name = "light_constant";
  pass->light_cb = crude_gfx_create_buffer( pass->scene_renderer->gpu, &buffer_creation );

  crude_gfx_light_pass_on_techniques_reloaded( pass );
}

void
crude_gfx_light_pass_render
(
  _In_ void                                               *ctx,
  _In_ crude_gfx_cmd_buffer                               *primary_cmd
)
{
  crude_gfx_light_pass                                    *pass;
  crude_gfx_device                                        *gpu;
  crude_gfx_pipeline_handle                                pipeline;
  
  pass = CRUDE_REINTERPRET_CAST( crude_gfx_light_pass*, ctx );
  gpu = pass->scene_renderer->gpu;

  pipeline = crude_gfx_access_technique_pass_by_name( gpu, "fullscreen", "light_pbr" )->pipeline;
  crude_gfx_cmd_bind_pipeline( primary_cmd, pipeline );
  crude_gfx_cmd_bind_descriptor_set( primary_cmd, pass->light_ds[ gpu->current_frame ] );
  crude_gfx_cmd_draw( primary_cmd, 0u, 3u, 0u, 1u );
}

void
crude_gfx_light_pass_on_techniques_reloaded
(
  _In_ void                                               *ctx
)
{
  crude_gfx_light_pass                                    *pass;
  crude_gfx_device                                        *gpu;
  crude_gfx_technique_pass                                *light_pass;
  crude_gfx_descriptor_set_layout_handle                   light_dsl;

  pass = CRUDE_REINTERPRET_CAST( crude_gfx_light_pass*, ctx );
  gpu = pass->scene_renderer->gpu;

  light_pass = crude_gfx_access_technique_pass_by_name( pass->scene_renderer->gpu, "fullscreen", "light_pbr" );
  light_dsl = crude_gfx_get_descriptor_set_layout( pass->scene_renderer->gpu, light_pass->pipeline, CRUDE_GRAPHICS_MATERIAL_DESCRIPTOR_SET_INDEX );
  
  for ( uint32 i = 0; i < CRUDE_GRAPHICS_MAX_SWAPCHAIN_IMAGES; ++i )
  {
    if ( CRUDE_RESOURCE_HANDLE_IS_VALID( pass->light_ds[ i ] ) )
    {
      crude_gfx_destroy_descriptor_set( pass->scene_renderer->gpu, pass->light_ds[ i ] );
    }
  }

  for ( uint32 i = 0; i < CRUDE_GRAPHICS_MAX_SWAPCHAIN_IMAGES; ++i )
  {
    crude_gfx_descriptor_set_creation                    ds_creation;

    ds_creation = crude_gfx_descriptor_set_creation_empty();
    ds_creation.layout = light_dsl;
    ds_creation.name = "light_pass_ds";
    
    crude_gfx_descriptor_set_creation_add_buffer( &ds_creation, pass->scene_renderer->scene_hga.buffer_handle, 0u );
    crude_gfx_descriptor_set_creation_add_buffer( &ds_creation, pass->scene_renderer->lights_bins_sb[ gpu->current_frame ], 10u );
    crude_gfx_descriptor_set_creation_add_buffer( &ds_creation, pass->scene_renderer->lights_sb, 11u );
    crude_gfx_descriptor_set_creation_add_buffer( &ds_creation, pass->scene_renderer->lights_tiles_sb[ gpu->current_frame ], 12u );
    crude_gfx_descriptor_set_creation_add_buffer( &ds_creation, pass->scene_renderer->lights_indices_sb[ gpu->current_frame ], 13u );
    crude_gfx_descriptor_set_creation_add_buffer( &ds_creation, pass->scene_renderer->pointlight_world_to_clip_sb[ gpu->current_frame ], 14u );
    crude_gfx_descriptor_set_creation_add_buffer( &ds_creation, pass->light_cb, 15u );
    
#if CRUDE_GRAPHICS_RAY_TRACING_ENABLED
    crude_gfx_descriptor_set_creation_add_acceleration_structure( &ds_creation, pass->scene_renderer->vk_tlas, 10u );
#endif /* CRUDE_GRAPHICS_RAY_TRACING_ENABLED */

    crude_gfx_scene_renderer_add_debug_resources_to_descriptor_set_creation( &ds_creation, pass->scene_renderer, i );

    pass->light_ds[ i ] = crude_gfx_create_descriptor_set( pass->scene_renderer->gpu, &ds_creation );
  }
}

void
crude_gfx_light_pass_on_resize
(
  _In_ void                                               *ctx,
  _In_ uint32                                              new_width,
  _In_ uint32                                              new_height
)
{
  crude_gfx_light_pass_on_techniques_reloaded( ctx );
}

crude_gfx_render_graph_pass_container
crude_gfx_light_pass_pack
(
  _In_ crude_gfx_light_pass                               *pass
)
{
  crude_gfx_render_graph_pass_container container = crude_gfx_render_graph_pass_container_empty();
  container.ctx = pass;
  container.render = crude_gfx_light_pass_render;
  container.on_techniques_reloaded = crude_gfx_light_pass_on_techniques_reloaded;
  container.on_resize = crude_gfx_light_pass_on_resize;
  return container;
}