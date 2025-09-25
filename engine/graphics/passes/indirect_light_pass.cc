#include <core/profiler.h>
#include <core/hash_map.h>
#include <scene/scene_components.h>
#include <graphics/scene_renderer.h>

#include <graphics/passes/indirect_light_pass.h>

void
crude_gfx_indirect_light_pass_initialize
(
  _In_ crude_gfx_indirect_light_pass                      *pass,
  _In_ crude_gfx_scene_renderer                           *scene_renderer
)
{
  crude_gfx_renderer_technique_pass                       *probe_raytracer_technique_pass;
  crude_gfx_descriptor_set_layout_handle                   probe_raytracer_technique_dsl;
  crude_gfx_buffer_creation                                buffer_creation;
  crude_gfx_texture_creation                               texture_creation;
  
  pass->scene_renderer = scene_renderer;
  probe_raytracer_technique_pass = crude_gfx_renderer_access_technique_pass_by_name( pass->scene_renderer->renderer, "ray_tracing", "probe_raytracer" );
  probe_raytracer_technique_dsl = crude_gfx_get_descriptor_set_layout( pass->scene_renderer->renderer->gpu, probe_raytracer_technique_pass->pipeline, CRUDE_GFX_MATERIAL_DESCRIPTOR_SET_INDEX );

  pass->scene_renderer = scene_renderer;
  
  pass->probe_count_x = 20;
  pass->probe_count_y = 12;
  pass->probe_count_z = 20;
  pass->probe_rays = 128;
  
  uint32 probe_count = pass->probe_count_x * pass->probe_count_y * pass->probe_count_z;
  uint32 num_rays = pass->probe_rays;

  texture_creation = crude_gfx_texture_creation_empty( );
  texture_creation.width = num_rays;
  texture_creation.height = probe_count;
  texture_creation.format = VK_FORMAT_R16G16B16A16_SFLOAT;
  texture_creation.type = CRUDE_GFX_TEXTURE_TYPE_TEXTURE_2D;
  texture_creation.flags = CRUDE_GFX_TEXTURE_MASK_COMPUTE;
  texture_creation.name = "probe_rt_radiance";
  for ( uint32 i = 0; i < CRUDE_GFX_MAX_SWAPCHAIN_IMAGES; ++i )
  {
    pass->probe_raytrace_radiance_texture_handle[ i ] = crude_gfx_create_texture( pass->scene_renderer->renderer->gpu, &texture_creation );
  }
  
  buffer_creation = crude_gfx_buffer_creation_empty( );
  buffer_creation.usage = CRUDE_GFX_RESOURCE_USAGE_TYPE_DYNAMIC;
  buffer_creation.type_flags = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
  buffer_creation.size = sizeof( crude_gfx_ddgi_gpu_data );
  buffer_creation.name = "ddgi_sb";
  pass->ddgi_sb = crude_gfx_create_buffer( pass->scene_renderer->renderer->gpu, &buffer_creation );
  
  buffer_creation = crude_gfx_buffer_creation_empty( );
  buffer_creation.usage = CRUDE_GFX_RESOURCE_USAGE_TYPE_IMMUTABLE;
  buffer_creation.type_flags = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
  buffer_creation.size = sizeof( uint32 ) * probe_count;
  buffer_creation.name = "probe_status_sb";
  pass->probe_status_sb = crude_gfx_create_buffer( pass->scene_renderer->renderer->gpu, &buffer_creation );

  for ( uint32 i = 0; i < CRUDE_GFX_MAX_SWAPCHAIN_IMAGES; ++i )
  {
    crude_gfx_descriptor_set_creation                    ds_creation;

    ds_creation = crude_gfx_descriptor_set_creation_empty();
    ds_creation.layout = probe_raytracer_technique_dsl;
    ds_creation.name = "probe_raytracer_technique_dsl";
    
    crude_gfx_scene_renderer_add_scene_resources_to_descriptor_set_creation( &ds_creation, pass->scene_renderer, i );
    crude_gfx_scene_renderer_add_mesh_resources_to_descriptor_set_creation( &ds_creation, pass->scene_renderer, i );
    crude_gfx_scene_renderer_add_light_resources_to_descriptor_set_creation( &ds_creation, pass->scene_renderer, i );
    crude_gfx_descriptor_set_creation_add_buffer( &ds_creation, pass->ddgi_sb, 40u );
    crude_gfx_descriptor_set_creation_add_buffer( &ds_creation, pass->probe_status_sb, 41u );
    
    pass->probe_raytrace_ds[ i ] = crude_gfx_create_descriptor_set( pass->scene_renderer->renderer->gpu, &ds_creation );
  }
}

void
crude_gfx_indirect_light_pass_deinitialize
(
  _In_ crude_gfx_indirect_light_pass                      *pass
)
{
  for ( uint32 i = 0; i < CRUDE_GFX_MAX_SWAPCHAIN_IMAGES; ++i )
  {
    crude_gfx_destroy_descriptor_set( pass->scene_renderer->renderer->gpu, pass->probe_raytrace_ds[ i ] );
    crude_gfx_destroy_texture( pass->scene_renderer->renderer->gpu, pass->probe_raytrace_radiance_texture_handle[ i ] );
  }
}

void
crude_gfx_indirect_light_pass_render
(
  _In_ void                                               *ctx,
  _In_ crude_gfx_cmd_buffer                               *primary_cmd
)
{
  crude_gfx_indirect_light_pass                           *pass;
  crude_gfx_renderer                                      *renderer;
  crude_gfx_texture                                       *probe_raytrace_radiance_texture;
  crude_gfx_pipeline_handle                                probe_raytrace_pipeline;
  
  pass = CRUDE_REINTERPRET_CAST( crude_gfx_indirect_light_pass*, ctx );
  renderer = pass->scene_renderer->renderer;
  
  crude_gfx_map_buffer_parameters map_parameters = CRUDE_COMPOUNT_EMPTY( crude_gfx_map_buffer_parameters );
  map_parameters.buffer = pass->ddgi_sb;

  crude_gfx_ddgi_gpu_data *gpu_data = CRUDE_CAST( crude_gfx_ddgi_gpu_data*, crude_gfx_map_buffer( renderer->gpu, &map_parameters ) );
  if ( gpu_data )
  {
    gpu_data->radiance_output_index = pass->probe_raytrace_radiance_texture_handle[ renderer->gpu->current_frame ].index;
    gpu_data->probe_grid_position = XMFLOAT3{ -10.0,0.5,-10.0 };
    gpu_data->probe_spacing = XMFLOAT3{ 1, 1, 1 };
    gpu_data->probe_counts.x = pass->probe_count_x;
    gpu_data->probe_counts.y = pass->probe_count_y;
    gpu_data->probe_counts.z = pass->probe_count_z;
    gpu_data->probe_rays = pass->probe_rays;

    float32 rotation_scaler = 0.001f;
    XMStoreFloat4x4( &gpu_data->random_rotation, XMMatrixRotationRollPitchYaw( 0, 0, 0 ) );//get_random_value( -1,1 ) * rotation_scaler, get_random_value( -1,1 ) * rotation_scaler, get_random_value( -1,1 ) * rotation_scaler ) );
    crude_gfx_unmap_buffer( renderer->gpu, map_parameters.buffer );
  }

  probe_raytrace_radiance_texture = crude_gfx_access_texture( renderer->gpu, pass->probe_raytrace_radiance_texture_handle[ renderer->gpu->current_frame ] );
  probe_raytrace_pipeline = crude_gfx_renderer_access_technique_pass_by_name( renderer, "ray_tracing", "probe_raytracer" )->pipeline;
  
  crude_gfx_cmd_add_image_barrier( primary_cmd, probe_raytrace_radiance_texture, CRUDE_GFX_RESOURCE_STATE_UNORDERED_ACCESS, 0, 1, false );
  crude_gfx_cmd_bind_pipeline( primary_cmd, probe_raytrace_pipeline );
  crude_gfx_cmd_bind_descriptor_set( primary_cmd, pass->probe_raytrace_ds[ renderer->gpu->current_frame ] );
  uint32 probe_count = pass->probe_count_x * pass->probe_count_y * pass->probe_count_z;
  crude_gfx_cmd_trace_rays( primary_cmd, probe_raytrace_pipeline, pass->probe_rays, probe_count, 1u );
  crude_gfx_cmd_add_image_barrier( primary_cmd, probe_raytrace_radiance_texture, CRUDE_GFX_RESOURCE_STATE_UNORDERED_ACCESS, 0, 1, false );

  
    //gpu_commands->issue_texture_barrier( probe_grid_irradiance_texture, RESOURCE_STATE_UNORDERED_ACCESS, 0, 1 );

    //gpu_commands->bind_pipeline( probe_grid_update_irradiance_pipeline );
    //gpu_commands->bind_descriptor_set( &probe_grid_update_descriptor_set, 1, nullptr, 0 );
    //gpu_commands->dispatch( raptor::ceilu32( irradiance_atlas_width / 8.f ),
    //                        raptor::ceilu32( irradiance_atlas_height / 8.f ), 1 );

    //gpu_commands->pop_marker();

    //gpu_commands->push_marker( "Blend Vis" );
    //// Probe grid update: visibility
    //gpu_commands->issue_texture_barrier( probe_grid_visibility_texture, RESOURCE_STATE_UNORDERED_ACCESS, 0, 1 );

    //gpu_commands->bind_pipeline( probe_grid_update_visibility_pipeline );
    //gpu_commands->bind_descriptor_set( &probe_grid_update_descriptor_set, 1, nullptr, 0 );
    //gpu_commands->dispatch( raptor::ceilu32( visibility_atlas_width / 8.f ),
    //                        raptor::ceilu32( visibility_atlas_height / 8.f ), 1 );

    //gpu_commands->issue_texture_barrier( probe_grid_irradiance_texture, RESOURCE_STATE_UNORDERED_ACCESS, 0, 1 );
    //gpu_commands->issue_texture_barrier( probe_grid_visibility_texture, RESOURCE_STATE_UNORDERED_ACCESS, 0, 1 );

    //gpu_commands->pop_marker();
    //gpu_commands->global_debug_barrier();
}

crude_gfx_render_graph_pass_container
crude_gfx_indirect_light_pass_pack
(
  _In_ crude_gfx_indirect_light_pass                      *pass
)
{
  crude_gfx_render_graph_pass_container container = crude_gfx_render_graph_pass_container_empty();
  container.ctx = pass;
  container.render = crude_gfx_indirect_light_pass_render;
  return container;
}