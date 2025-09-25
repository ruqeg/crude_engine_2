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
  crude_gfx_renderer_technique_pass                       *technique_pass;
  crude_gfx_descriptor_set_layout_handle                   probe_raytrace_dsl;
  crude_gfx_descriptor_set_layout_handle                   probe_update_visibility_dsl;
  crude_gfx_descriptor_set_layout_handle                   probe_update_irradiance_dsl;
  crude_gfx_descriptor_set_layout_handle                   calculate_probe_status_dsl;
  crude_gfx_buffer_creation                                buffer_creation;
  crude_gfx_texture_creation                               texture_creation;
  
  pass->scene_renderer = scene_renderer;
  technique_pass = crude_gfx_renderer_access_technique_pass_by_name( pass->scene_renderer->renderer, "ray_tracing", "probe_raytracer" );
  probe_raytrace_dsl = crude_gfx_get_descriptor_set_layout( pass->scene_renderer->renderer->gpu, technique_pass->pipeline, CRUDE_GFX_MATERIAL_DESCRIPTOR_SET_INDEX );
  
  technique_pass = crude_gfx_renderer_access_technique_pass_by_name( pass->scene_renderer->renderer, "ray_tracing", "probe_update_visibility" );
  probe_update_visibility_dsl = crude_gfx_get_descriptor_set_layout( pass->scene_renderer->renderer->gpu, technique_pass->pipeline, CRUDE_GFX_MATERIAL_DESCRIPTOR_SET_INDEX );
  
  technique_pass = crude_gfx_renderer_access_technique_pass_by_name( pass->scene_renderer->renderer, "ray_tracing", "probe_update_irradiance" );
  probe_update_irradiance_dsl = crude_gfx_get_descriptor_set_layout( pass->scene_renderer->renderer->gpu, technique_pass->pipeline, CRUDE_GFX_MATERIAL_DESCRIPTOR_SET_INDEX );

  technique_pass = crude_gfx_renderer_access_technique_pass_by_name( pass->scene_renderer->renderer, "ray_tracing", "calculate_probe_status" );
  calculate_probe_status_dsl = crude_gfx_get_descriptor_set_layout( pass->scene_renderer->renderer->gpu, technique_pass->pipeline, CRUDE_GFX_MATERIAL_DESCRIPTOR_SET_INDEX );

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
  
  uint32 irradiance_probe_size = 6;
  uint32 octahedral_irradiance_size = irradiance_probe_size + 2;
  pass->irradiance_atlas_width = ( octahedral_irradiance_size * pass->probe_count_x * pass->probe_count_y );
  pass->irradiance_atlas_height = ( octahedral_irradiance_size * pass->probe_count_z );
  
  texture_creation = crude_gfx_texture_creation_empty( );
  texture_creation.width = pass->irradiance_atlas_width;
  texture_creation.height = pass->irradiance_atlas_height;
  texture_creation.format = VK_FORMAT_R16G16B16A16_SFLOAT;
  texture_creation.type = CRUDE_GFX_TEXTURE_TYPE_TEXTURE_2D;
  texture_creation.flags = CRUDE_GFX_TEXTURE_MASK_COMPUTE;
  texture_creation.name = "probe_irradiance";
  pass->probe_grid_irradiance_texture_handle = crude_gfx_create_texture( pass->scene_renderer->renderer->gpu, &texture_creation );
  
  uint32                     visibility_probe_size = 6;
  uint32 octahedral_visibility_size = visibility_probe_size + 2;
  pass->visibility_atlas_width = ( octahedral_visibility_size * pass->probe_count_x * pass->probe_count_y );
  pass->visibility_atlas_height = ( octahedral_visibility_size * pass->probe_count_z );
  texture_creation = crude_gfx_texture_creation_empty( );
  texture_creation.width = pass->visibility_atlas_width;
  texture_creation.height = pass->visibility_atlas_height;
  texture_creation.format = VK_FORMAT_R16G16_SFLOAT;
  texture_creation.type = CRUDE_GFX_TEXTURE_TYPE_TEXTURE_2D;
  texture_creation.flags = CRUDE_GFX_TEXTURE_MASK_COMPUTE;
  texture_creation.name = "probe_visibility";
  pass->probe_grid_visibility_texture_handle = crude_gfx_create_texture( pass->scene_renderer->renderer->gpu, &texture_creation );
  
  texture_creation = crude_gfx_texture_creation_empty( );
  texture_creation.width = pass->probe_count_x * pass->probe_count_y;
  texture_creation.height = pass->probe_count_z;
  texture_creation.format = VK_FORMAT_R16G16B16A16_SFLOAT;
  texture_creation.type = CRUDE_GFX_TEXTURE_TYPE_TEXTURE_2D;
  texture_creation.flags = CRUDE_GFX_TEXTURE_MASK_COMPUTE;
  texture_creation.name = "probe_offsets";
  pass->probe_offsets_texture_handle = crude_gfx_create_texture( pass->scene_renderer->renderer->gpu, &texture_creation );
  
  pass->use_half_resolution = true;
  texture_creation = crude_gfx_texture_creation_empty( );
  texture_creation.width = pass->use_half_resolution ? ( pass->scene_renderer->renderer->gpu->vk_swapchain_width ) / 2 : pass->scene_renderer->renderer->gpu->vk_swapchain_width;
  texture_creation.height = pass->use_half_resolution ? ( pass->scene_renderer->renderer->gpu->vk_swapchain_height ) / 2 : pass->scene_renderer->renderer->gpu->vk_swapchain_height;
  texture_creation.format = VK_FORMAT_R16G16B16A16_SFLOAT;
  texture_creation.type = CRUDE_GFX_TEXTURE_TYPE_TEXTURE_2D;
  texture_creation.flags = CRUDE_GFX_TEXTURE_MASK_COMPUTE;
  texture_creation.name = "indirect_texture";
  pass->indirect_texture_handle = crude_gfx_create_texture( pass->scene_renderer->renderer->gpu, &texture_creation );

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
    ds_creation.layout = probe_raytrace_dsl;
    ds_creation.name = "probe_raytrace_dsl";
    
    crude_gfx_descriptor_set_creation_add_buffer( &ds_creation, pass->scene_renderer->meshes_draws_sb, 1u );
    crude_gfx_descriptor_set_creation_add_buffer( &ds_creation, pass->scene_renderer->meshes_instances_draws_sb, 2u );
    crude_gfx_descriptor_set_creation_add_buffer( &ds_creation, pass->scene_renderer->lights_sb, 3u );
    crude_gfx_descriptor_set_creation_add_acceleration_structure( &ds_creation, pass->scene_renderer->tlas, 4u );
    crude_gfx_descriptor_set_creation_add_buffer( &ds_creation, pass->ddgi_sb, 10u );
    crude_gfx_descriptor_set_creation_add_buffer( &ds_creation, pass->probe_status_sb, 11u );
    crude_gfx_scene_renderer_add_debug_resources_to_descriptor_set_creation( &ds_creation, pass->scene_renderer, i );
    pass->probe_raytrace_ds[ i ] = crude_gfx_create_descriptor_set( pass->scene_renderer->renderer->gpu, &ds_creation );
  }

  for ( uint32 i = 0; i < CRUDE_GFX_MAX_SWAPCHAIN_IMAGES; ++i )
  {
    crude_gfx_descriptor_set_creation                    ds_creation;

    ds_creation = crude_gfx_descriptor_set_creation_empty();
    ds_creation.layout = probe_update_irradiance_dsl;
    ds_creation.name = "probe_update_irradiance_dsl";
    
    crude_gfx_descriptor_set_creation_add_buffer( &ds_creation, pass->ddgi_sb, 10u );
    crude_gfx_descriptor_set_creation_add_buffer( &ds_creation, pass->probe_status_sb, 11u );
    crude_gfx_scene_renderer_add_debug_resources_to_descriptor_set_creation( &ds_creation, pass->scene_renderer, i );
    pass->probe_update_irradiance_ds[ i ] = crude_gfx_create_descriptor_set( pass->scene_renderer->renderer->gpu, &ds_creation );
  }

  for ( uint32 i = 0; i < CRUDE_GFX_MAX_SWAPCHAIN_IMAGES; ++i )
  {
    crude_gfx_descriptor_set_creation                    ds_creation;

    ds_creation = crude_gfx_descriptor_set_creation_empty();
    ds_creation.layout = probe_update_visibility_dsl;
    ds_creation.name = "probe_update_visibility_dsl";
    
    crude_gfx_descriptor_set_creation_add_buffer( &ds_creation, pass->ddgi_sb, 10u );
    crude_gfx_descriptor_set_creation_add_buffer( &ds_creation, pass->probe_status_sb, 11u );
    crude_gfx_scene_renderer_add_debug_resources_to_descriptor_set_creation( &ds_creation, pass->scene_renderer, i );
    pass->probe_update_visibility_ds[ i ] = crude_gfx_create_descriptor_set( pass->scene_renderer->renderer->gpu, &ds_creation );
  }

  for ( uint32 i = 0; i < CRUDE_GFX_MAX_SWAPCHAIN_IMAGES; ++i )
  {
    crude_gfx_descriptor_set_creation                    ds_creation;

    ds_creation = crude_gfx_descriptor_set_creation_empty();
    ds_creation.layout = calculate_probe_status_dsl;
    ds_creation.name = "calculate_probe_status_dsl";
    
    crude_gfx_descriptor_set_creation_add_buffer( &ds_creation, pass->ddgi_sb, 10u );
    crude_gfx_descriptor_set_creation_add_buffer( &ds_creation, pass->probe_status_sb, 11u );
    crude_gfx_scene_renderer_add_debug_resources_to_descriptor_set_creation( &ds_creation, pass->scene_renderer, i );
    pass->calculate_probe_status_ds[ i ] = crude_gfx_create_descriptor_set( pass->scene_renderer->renderer->gpu, &ds_creation );
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
  crude_gfx_pipeline_handle                                probe_raytrace_pipeline, probe_update_irradiance_pipeline, probe_update_visibility_pipeline;
  
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
  probe_update_irradiance_pipeline = crude_gfx_renderer_access_technique_pass_by_name( renderer, "ray_tracing", "probe_update_irradiance" )->pipeline;
  probe_update_visibility_pipeline = crude_gfx_renderer_access_technique_pass_by_name( renderer, "ray_tracing", "probe_update_visibility" )->pipeline;
  crude_gfx_cmd_add_image_barrier( primary_cmd, probe_raytrace_radiance_texture, CRUDE_GFX_RESOURCE_STATE_UNORDERED_ACCESS, 0, 1, false );
  crude_gfx_cmd_bind_pipeline( primary_cmd, probe_raytrace_pipeline );
  crude_gfx_cmd_bind_descriptor_set( primary_cmd, pass->probe_raytrace_ds[ renderer->gpu->current_frame ] );
  uint32 probe_count = pass->probe_count_x * pass->probe_count_y * pass->probe_count_z;
  crude_gfx_cmd_trace_rays( primary_cmd, probe_raytrace_pipeline, pass->probe_rays, probe_count, 1u );
  crude_gfx_cmd_add_image_barrier( primary_cmd, probe_raytrace_radiance_texture, CRUDE_GFX_RESOURCE_STATE_UNORDERED_ACCESS, 0, 1, false );

  crude_gfx_cmd_add_image_barrier( primary_cmd, crude_gfx_access_texture( renderer->gpu, pass->probe_grid_irradiance_texture_handle ), CRUDE_GFX_RESOURCE_STATE_UNORDERED_ACCESS, 0, 1, false );
  crude_gfx_cmd_bind_pipeline( primary_cmd, probe_update_irradiance_pipeline );
  crude_gfx_cmd_bind_descriptor_set( primary_cmd, pass->probe_update_irradiance_ds[ renderer->gpu->current_frame ] );
  crude_gfx_cmd_dispatch( primary_cmd, ( pass->irradiance_atlas_width + 7 ) / 8, ( pass->irradiance_atlas_height + 7 ) / 8, 1 );
  
  crude_gfx_cmd_add_image_barrier( primary_cmd, crude_gfx_access_texture( renderer->gpu, pass->probe_grid_visibility_texture_handle ), CRUDE_GFX_RESOURCE_STATE_UNORDERED_ACCESS, 0, 1, false );
  
  crude_gfx_cmd_bind_pipeline( primary_cmd, probe_update_visibility_pipeline );
  crude_gfx_cmd_bind_descriptor_set( primary_cmd, pass->probe_update_visibility_ds[ renderer->gpu->current_frame ] );
  crude_gfx_cmd_dispatch( primary_cmd, ( pass->visibility_atlas_width + 7 ) / 8, ( pass->visibility_atlas_height + 7 ) / 8, 1 );

  crude_gfx_cmd_add_image_barrier( primary_cmd, crude_gfx_access_texture( renderer->gpu, pass->probe_grid_irradiance_texture_handle ), CRUDE_GFX_RESOURCE_STATE_UNORDERED_ACCESS, 0, 1, false );
  crude_gfx_cmd_add_image_barrier( primary_cmd, crude_gfx_access_texture( renderer->gpu, pass->probe_grid_visibility_texture_handle ), CRUDE_GFX_RESOURCE_STATE_UNORDERED_ACCESS, 0, 1, false );

  crude_gfx_cmd_add_image_barrier( primary_cmd, crude_gfx_access_texture( renderer->gpu, pass->indirect_texture_handle ), CRUDE_GFX_RESOURCE_STATE_UNORDERED_ACCESS, 0, 1, false );
  crude_gfx_cmd_bind_pipeline( primary_cmd, probe_update_visibility_pipeline );
  crude_gfx_cmd_bind_descriptor_set( primary_cmd, pass->probe_update_visibility_ds[ renderer->gpu->current_frame ] );

  gpu_commands->bind_pipeline( sample_irradiance_pipeline );
  gpu_commands->bind_descriptor_set( &sample_irradiance_descriptor_set, 1, nullptr, 0 );

  uint32 half_resolution = pass->use_half_resolution ? 1 : 0;
  crude_gfx_cmd_push_constant( primary_cmd, &half_resolution, sizeof( half_resolution ) );
  float32 resolution_divider = pass->use_half_resolution ? 0.5f : 1.0f;
  uint32 width = renderer->gpu->vk_swapchain_width * resolution_divider;
  uint32 height = renderer->gpu->vk_swapchain_height * resolution_divider;
  crude_gfx_cmd_dispatch( primary_cmd, ( width + 7 ) / 8, ( height + 7 ) / 8, 1 );
  crude_gfx_cmd_add_image_barrier( primary_cmd, crude_gfx_access_texture( renderer->gpu, pass->indirect_texture_handle ), CRUDE_GFX_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, 0, 1, false );
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