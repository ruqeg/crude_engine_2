#include <core/profiler.h>
#include <core/hash_map.h>
#include <scene/scene_components.h>
#include <graphics/scene_renderer.h>

#include <graphics/passes/ray_tracing_solid_pass.h>

#ifdef CRUDE_GRAPHICS_RAY_TRACING_ENABLED

typedef CRUDE_ALIGNED_STRUCT( 16 ) crude_gfx_ray_tracing_solid_gpu_data
{
  uint32                                                   sbt_offset;
  uint32                                                   sbt_stride;
  uint32                                                   miss_index;
  uint32                                                   out_image_index;
} crude_gfx_ray_tracing_solid_gpu_data;

void
crude_gfx_ray_tracing_solid_pass_initialize
(
  _In_ crude_gfx_ray_tracing_solid_pass                   *pass,
  _In_ crude_gfx_scene_renderer                           *scene_renderer
)
{
  crude_gfx_buffer_creation                                buffer_creation;

  pass->scene_renderer = scene_renderer;

  buffer_creation = crude_gfx_buffer_creation_empty( );
  buffer_creation.type_flags = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
  buffer_creation.usage = CRUDE_GFX_RESOURCE_USAGE_TYPE_DYNAMIC;
  buffer_creation.size = sizeof ( crude_gfx_ray_tracing_solid_gpu_data );
  buffer_creation.name = "ray_tracing_uniform_buffer";
  
  for ( uint32 i = 0; i < CRUDE_GFX_MAX_SWAPCHAIN_IMAGES; ++i )
  {
    pass->uniform_buffer[ i ] = crude_gfx_create_buffer( pass->scene_renderer->renderer->gpu, &buffer_creation );
  }
  
  for ( uint32 i = 0; i < CRUDE_GFX_MAX_SWAPCHAIN_IMAGES; ++i )
  {
    pass->ray_tracing_solid_ds[ i ] = CRUDE_GFX_DESCRIPTOR_SET_HANDLE_INVALID;
  }

  crude_gfx_ray_tracing_solid_pass_on_techniques_reloaded( pass );
}

void
crude_gfx_ray_tracing_solid_pass_deinitialize
(
  _In_ crude_gfx_ray_tracing_solid_pass                   *pass
)
{
  for ( uint32 i = 0; i < CRUDE_GFX_MAX_SWAPCHAIN_IMAGES; ++i )
  {
    crude_gfx_destroy_descriptor_set( pass->scene_renderer->renderer->gpu, pass->ray_tracing_solid_ds[ i ] );
    crude_gfx_destroy_buffer( pass->scene_renderer->renderer->gpu, pass->uniform_buffer[ i ] );
  }
}

void
crude_gfx_ray_tracing_solid_pass_render
(
  _In_ void                                               *ctx,
  _In_ crude_gfx_cmd_buffer                               *primary_cmd
)
{
  crude_gfx_ray_tracing_solid_pass                        *pass;
  crude_gfx_renderer                                      *renderer;
  crude_gfx_texture                                       *ray_tracing_solid_texture;
  crude_gfx_texture_handle                                 ray_tracing_solid_texture_handle;
  crude_gfx_pipeline_handle                                pipeline;
  
  pass = CRUDE_REINTERPRET_CAST( crude_gfx_ray_tracing_solid_pass*, ctx );
  renderer = pass->scene_renderer->renderer;
  
  ray_tracing_solid_texture_handle = crude_gfx_render_graph_builder_access_resource_by_name( pass->scene_renderer->render_graph->builder, "ray_tracing_solid" )->resource_info.texture.handle;
  ray_tracing_solid_texture = crude_gfx_access_texture( renderer->gpu, ray_tracing_solid_texture_handle );

  crude_gfx_map_buffer_parameters map_parameters = CRUDE_COMPOUNT_EMPTY( crude_gfx_map_buffer_parameters );
  map_parameters.buffer = pass->uniform_buffer[ renderer->gpu->current_frame ];

  crude_gfx_ray_tracing_solid_gpu_data *gpu_data = CRUDE_CAST( crude_gfx_ray_tracing_solid_gpu_data*, crude_gfx_map_buffer( renderer->gpu, &map_parameters ) );
  if ( gpu_data )
  {
    gpu_data->sbt_offset = 0;
    gpu_data->sbt_stride = renderer->gpu->ray_tracing_pipeline_properties.shaderGroupHandleAlignment;
    gpu_data->miss_index = 0;
    gpu_data->out_image_index = ray_tracing_solid_texture_handle.index;
    crude_gfx_unmap_buffer( renderer->gpu, map_parameters.buffer );
  }
  
  pipeline = crude_gfx_renderer_access_technique_pass_by_name( renderer, "ray_tracing", "ray_tracing_solid" )->pipeline;
  
  crude_gfx_cmd_add_image_barrier( primary_cmd, ray_tracing_solid_texture, CRUDE_GFX_RESOURCE_STATE_UNORDERED_ACCESS, 0, 1, false );

  crude_gfx_cmd_bind_pipeline( primary_cmd, pipeline );
  crude_gfx_cmd_bind_descriptor_set( primary_cmd, pass->ray_tracing_solid_ds[ renderer->gpu->current_frame ] );
  crude_gfx_cmd_trace_rays( primary_cmd, pipeline, ray_tracing_solid_texture->width, ray_tracing_solid_texture->height, 1u );
  crude_gfx_cmd_add_image_barrier( primary_cmd, ray_tracing_solid_texture, CRUDE_GFX_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, 0, 1, false );
}

void
crude_gfx_ray_tracing_solid_pass_on_techniques_reloaded
(
  _In_ void                                               *ctx
)
{
  crude_gfx_ray_tracing_solid_pass                        *pass;
  crude_gfx_renderer_technique_pass                       *ray_tracing_pass;
  crude_gfx_descriptor_set_layout_handle                   layout;

  pass = CRUDE_CAST( crude_gfx_ray_tracing_solid_pass*, ctx );
  ray_tracing_pass = crude_gfx_renderer_access_technique_pass_by_name( pass->scene_renderer->renderer, "ray_tracing", "ray_tracing_solid" );
  layout = crude_gfx_get_descriptor_set_layout( pass->scene_renderer->renderer->gpu, ray_tracing_pass->pipeline, CRUDE_GFX_MATERIAL_DESCRIPTOR_SET_INDEX );
  
  for ( uint32 i = 0; i < CRUDE_GFX_MAX_SWAPCHAIN_IMAGES; ++i )
  {
    if ( CRUDE_RESOURCE_HANDLE_IS_VALID( pass->ray_tracing_solid_ds[ i ] ) )
    {
      crude_gfx_destroy_descriptor_set( pass->scene_renderer->renderer->gpu, pass->ray_tracing_solid_ds[ i ] );
    }
  }

  for ( uint32 i = 0; i < CRUDE_GFX_MAX_SWAPCHAIN_IMAGES; ++i )
  {
    crude_gfx_descriptor_set_creation                    ds_creation;

    ds_creation = crude_gfx_descriptor_set_creation_empty();
    ds_creation.layout = layout;
    ds_creation.name = "ray_tracing_solid_ds";

    crude_gfx_descriptor_set_creation_add_buffer( &ds_creation, pass->scene_renderer->scene_cb, 0u );
    crude_gfx_descriptor_set_creation_add_acceleration_structure( &ds_creation, pass->scene_renderer->vk_tlas, 1u );
    crude_gfx_descriptor_set_creation_add_buffer( &ds_creation, pass->scene_renderer->meshes_draws_sb, 2u );
    crude_gfx_descriptor_set_creation_add_buffer( &ds_creation, pass->scene_renderer->meshes_instances_draws_sb, 3u );
    crude_gfx_descriptor_set_creation_add_buffer( &ds_creation, pass->uniform_buffer[ i ], 4u );
    crude_gfx_scene_renderer_add_debug_resources_to_descriptor_set_creation( &ds_creation, pass->scene_renderer, pass->scene_renderer->renderer->gpu->current_frame );
    
    pass->ray_tracing_solid_ds[ i ] = crude_gfx_create_descriptor_set( pass->scene_renderer->renderer->gpu, &ds_creation );
  }
}

crude_gfx_render_graph_pass_container
crude_gfx_ray_tracing_solid_pass_pack
(
  _In_ crude_gfx_ray_tracing_solid_pass                   *pass
)
{
  crude_gfx_render_graph_pass_container container = crude_gfx_render_graph_pass_container_empty();
  container.ctx = pass;
  container.render = crude_gfx_ray_tracing_solid_pass_render;
  container.on_techniques_reloaded = crude_gfx_ray_tracing_solid_pass_on_techniques_reloaded;
  return container;
}

#endif /* CRUDE_GRAPHICS_RAY_TRACING_ENABLED */