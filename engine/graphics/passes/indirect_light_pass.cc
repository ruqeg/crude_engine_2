#include <engine/core/profiler.h>
#include <engine/core/hashmapstr.h>
#include <engine/scene/scene_ecs.h>
#include <engine/graphics/scene_renderer.h>

#define PROBE_RAYTRACER
#include <engine/graphics/shaders/ddgi.crude_shader>

#include <engine/graphics/passes/indirect_light_pass.h>

#if CRUDE_GFX_RAY_TRACING_DDGI_ENABLED

void
crude_gfx_indirect_light_pass_initialize
(
  _In_ crude_gfx_indirect_light_pass                      *pass,
  _In_ crude_gfx_scene_renderer                           *scene_renderer
)
{
  crude_gfx_device                                        *gpu;
  crude_gfx_buffer_creation                                buffer_creation;
  crude_gfx_texture_creation                               texture_creation;
  crude_gfx_sampler_creation                               sampler_creation;
  uint32                                                   probe_count;
  
  pass->scene_renderer = scene_renderer;
  pass->options.probe_spacing = XMFLOAT3{ 2.0, 2.0, 2.0 };
  pass->options.self_shadow_bias = 0.3f;
  pass->options.infinite_bounces_multiplier = 0.75f;
  pass->options.hysteresis = 0.95f;
  pass->options.probe_grid_position = XMFLOAT3{ -20.0,0.5,-13.0 };
  pass->options.max_probe_offset = 0.4f;
  pass->options.probe_debug_flags = 0;
  pass->options.shadow_weight_power = 2.5;
  pass->options.probe_update_per_frame = 1000;

  pass->probe_count_x = 20;
  pass->probe_count_y = 20;
  pass->probe_count_z = 20;
  probe_count = pass->probe_count_x * pass->probe_count_y * pass->probe_count_z;

  pass->offsets_calculations_count = 24;
  pass->probe_update_offset = 0u;
  pass->probe_rays = 128;
  pass->use_half_resolution = true;
  pass->irradiance_side_length = 6;
  pass->visibility_side_length = 6;
  pass->irradiance_side_length_with_borders = pass->irradiance_side_length + 2;
  pass->visibility_side_length_with_borders = pass->visibility_side_length + 2;
  pass->irradiance_atlas_width = ( pass->irradiance_side_length_with_borders * pass->probe_count_x * pass->probe_count_y );
  pass->irradiance_atlas_height = ( pass->irradiance_side_length_with_borders * pass->probe_count_z );
  pass->visibility_atlas_width = ( pass->visibility_side_length_with_borders * pass->probe_count_x * pass->probe_count_y );
  pass->visibility_atlas_height = ( pass->visibility_side_length_with_borders * pass->probe_count_z );
  
  gpu = pass->scene_renderer->gpu;

  texture_creation = crude_gfx_texture_creation_empty( );
  texture_creation.width = pass->probe_rays;
  texture_creation.height = probe_count;
  texture_creation.format = VK_FORMAT_R16G16B16A16_SFLOAT;
  texture_creation.type = CRUDE_GFX_TEXTURE_TYPE_TEXTURE_2D;
  texture_creation.flags = CRUDE_GFX_TEXTURE_MASK_COMPUTE;
  crude_string_copy( texture_creation.name, "probe_rt_radiance", sizeof( texture_creation.name ) );
  pass->probe_raytrace_radiance_texture_handle = crude_gfx_create_texture( pass->scene_renderer->gpu, &texture_creation );
  
  texture_creation = crude_gfx_texture_creation_empty( );
  texture_creation.width = pass->irradiance_atlas_width;
  texture_creation.height = pass->irradiance_atlas_height;
  texture_creation.format = VK_FORMAT_R16G16B16A16_SFLOAT;
  texture_creation.type = CRUDE_GFX_TEXTURE_TYPE_TEXTURE_2D;
  texture_creation.flags = CRUDE_GFX_TEXTURE_MASK_COMPUTE;
  crude_string_copy( texture_creation.name, "probe_irradiance", sizeof( texture_creation.name ) );
  pass->probe_grid_irradiance_texture_handle = crude_gfx_create_texture( pass->scene_renderer->gpu, &texture_creation );
  
  texture_creation = crude_gfx_texture_creation_empty( );
  texture_creation.width = pass->visibility_atlas_width;
  texture_creation.height = pass->visibility_atlas_height;
  texture_creation.format = VK_FORMAT_R16G16_SFLOAT;
  texture_creation.type = CRUDE_GFX_TEXTURE_TYPE_TEXTURE_2D;
  texture_creation.flags = CRUDE_GFX_TEXTURE_MASK_COMPUTE;
  crude_string_copy( texture_creation.name, "probe_visibility", sizeof( texture_creation.name ) );
  pass->probe_grid_visibility_texture_handle = crude_gfx_create_texture( pass->scene_renderer->gpu, &texture_creation );
  
  sampler_creation = crude_gfx_sampler_creation_empty( );
  sampler_creation.address_mode_u = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
  sampler_creation.address_mode_v = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
  sampler_creation.address_mode_w = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
  sampler_creation.mag_filter = VK_FILTER_LINEAR;
  sampler_creation.min_filter = VK_FILTER_LINEAR;
  sampler_creation.mip_filter = VK_SAMPLER_MIPMAP_MODE_NEAREST;
  pass->probe_grid_sampler_handle = crude_gfx_create_sampler( pass->scene_renderer->gpu, &sampler_creation );

  crude_gfx_link_texture_sampler( pass->scene_renderer->gpu, pass->probe_grid_irradiance_texture_handle, pass->probe_grid_sampler_handle );
  crude_gfx_link_texture_sampler( pass->scene_renderer->gpu, pass->probe_grid_visibility_texture_handle, pass->probe_grid_sampler_handle );

  texture_creation = crude_gfx_texture_creation_empty( );
  texture_creation.width = pass->probe_count_x * pass->probe_count_y;
  texture_creation.height = pass->probe_count_z;
  texture_creation.format = VK_FORMAT_R16G16B16A16_SFLOAT;
  texture_creation.type = CRUDE_GFX_TEXTURE_TYPE_TEXTURE_2D;
  texture_creation.flags = CRUDE_GFX_TEXTURE_MASK_COMPUTE;
  crude_string_copy( texture_creation.name, "probe_offsets", sizeof( texture_creation.name ) );
  pass->probe_offsets_texture_handle = crude_gfx_create_texture( pass->scene_renderer->gpu, &texture_creation );
  
  texture_creation = crude_gfx_texture_creation_empty( );
  texture_creation.width = pass->use_half_resolution ? ( pass->scene_renderer->gpu->renderer_size.x ) / 2 : pass->scene_renderer->gpu->renderer_size.x;
  texture_creation.height = pass->use_half_resolution ? ( pass->scene_renderer->gpu->renderer_size.y ) / 2 : pass->scene_renderer->gpu->renderer_size.y;
  texture_creation.format = VK_FORMAT_R16G16B16A16_SFLOAT;
  texture_creation.type = CRUDE_GFX_TEXTURE_TYPE_TEXTURE_2D;
  texture_creation.flags = CRUDE_GFX_TEXTURE_MASK_COMPUTE;
  crude_string_copy( texture_creation.name, "indirect_texture", sizeof( texture_creation.name ) );
  pass->indirect_texture_handle = crude_gfx_create_texture( pass->scene_renderer->gpu, &texture_creation );

  crude_gfx_indirect_light_pass_on_resize( pass, 0, 0 );
}

void
crude_gfx_indirect_light_pass_deinitialize
(
  _In_ crude_gfx_indirect_light_pass                      *pass
)
{
  crude_gfx_destroy_texture( pass->scene_renderer->gpu, pass->probe_raytrace_radiance_texture_handle );
  crude_gfx_destroy_sampler( pass->scene_renderer->gpu, pass->probe_grid_sampler_handle );
  crude_gfx_destroy_texture( pass->scene_renderer->gpu, pass->probe_grid_irradiance_texture_handle );
  crude_gfx_destroy_texture( pass->scene_renderer->gpu, pass->probe_grid_visibility_texture_handle );
  crude_gfx_destroy_texture( pass->scene_renderer->gpu, pass->probe_offsets_texture_handle );
  crude_gfx_destroy_texture( pass->scene_renderer->gpu, pass->indirect_texture_handle );
}

void
crude_gfx_indirect_light_pass_render
(
  _In_ void                                               *ctx,
  _In_ crude_gfx_cmd_buffer                               *primary_cmd
)
{
  crude_gfx_indirect_light_pass                           *pass;
  crude_gfx_device                                        *gpu;
  crude_gfx_ddgi_constants                                 ddgi_constants;
  uint32                                                   probe_count;

  pass = CRUDE_REINTERPRET_CAST( crude_gfx_indirect_light_pass*, ctx );
  gpu = pass->scene_renderer->gpu;

  probe_count = pass->probe_count_x * pass->probe_count_y * pass->probe_count_z;
  pass->probe_update_offset = ( pass->probe_update_offset + pass->options.probe_update_per_frame ) % probe_count;
  
  ddgi_constants = CRUDE_COMPOUNT_EMPTY( crude_gfx_ddgi_constants );
  ddgi_constants.probe_counts.x = pass->probe_count_x;
  ddgi_constants.probe_counts.y = pass->probe_count_y;
  ddgi_constants.probe_counts.z = pass->probe_count_z;
  ddgi_constants.probe_rays = pass->probe_rays;
  ddgi_constants.radiance_output_index = pass->probe_raytrace_radiance_texture_handle.index;
  ddgi_constants.indirect_output_index = pass->indirect_texture_handle.index;
  ddgi_constants.depth_fullscreen_texture_index = CRUDE_GFX_PASS_TEXTURE_INDEX( indirect_light.depth_texture );
  ddgi_constants.normal_texture_index = CRUDE_GFX_PASS_TEXTURE_INDEX( indirect_light.normal_texture );
  ddgi_constants.grid_irradiance_output_index = pass->probe_grid_irradiance_texture_handle.index;
  ddgi_constants.grid_visibility_texture_index = pass->probe_grid_visibility_texture_handle.index;
  ddgi_constants.probe_offset_texture_index = pass->probe_offsets_texture_handle.index;
  XMStoreFloat4x4( &ddgi_constants.random_rotation, XMMatrixRotationRollPitchYaw( 0.001f * crude_random_unit_f32( ), 0.001f * crude_random_unit_f32( ), 0.001f * crude_random_unit_f32( ) ) );
  //XMStoreFloat4x4( &ddgi_mapped_data->random_rotation, XMMatrixRotationAxis( XMVector3Normalize( XMVectorSet( crude_random_unit_f32( ), crude_random_unit_f32( ), crude_random_unit_f32( ), 1.0 ) ), crude_random_unit_f32( ) * XM_2PI ) );//get_random_value( -1,1 ) * rotation_scaler, get_random_value( -1,1 ) * rotation_scaler, get_random_value( -1,1 ) * rotation_scaler ) );
  ddgi_constants.irradiance_texture_width = pass->irradiance_atlas_width;
  ddgi_constants.irradiance_texture_height = pass->irradiance_atlas_height;
  ddgi_constants.irradiance_side_length = pass->irradiance_side_length;
  ddgi_constants.visibility_texture_width = pass->visibility_atlas_width;
  ddgi_constants.visibility_texture_height = pass->visibility_atlas_height;
  ddgi_constants.visibility_side_length = pass->visibility_side_length;
  ddgi_constants.hysteresis = pass->options.hysteresis;
  ddgi_constants.self_shadow_bias = pass->options.self_shadow_bias;
  ddgi_constants.probe_grid_position = pass->options.probe_grid_position;
  ddgi_constants.max_probe_offset = pass->options.max_probe_offset;
  ddgi_constants.infinite_bounces_multiplier = pass->options.infinite_bounces_multiplier;
  ddgi_constants.probe_spacing = pass->options.probe_spacing;
  ddgi_constants.probe_update_per_frame = pass->options.probe_update_per_frame;
  ddgi_constants.reciprocal_probe_spacing = CRUDE_COMPOUNT( XMFLOAT3, { 1.f / pass->options.probe_spacing.x, 1.f / pass->options.probe_spacing.y, 1.f / pass->options.probe_spacing.z } );
  ddgi_constants.shadow_weight_power = pass->options.shadow_weight_power;
  ddgi_constants.probe_update_offset = pass->probe_update_offset;
  
  //probe_raytrace_pipeline = crude_gfx_renderer_access_technique_pass_by_name( renderer, "ray_tracing", "probe_raytracer" )->pipeline;
  //probe_update_irradiance_pipeline = crude_gfx_renderer_access_technique_pass_by_name( renderer, "ray_tracing", "probe_update_irradiance" )->pipeline;
  //probe_update_visibility_pipeline = crude_gfx_renderer_access_technique_pass_by_name( renderer, "ray_tracing", "probe_update_visibility" )->pipeline;
  //sample_irradiance_pipeline = crude_gfx_renderer_access_technique_pass_by_name( renderer, "ray_tracing", "sample_irradiance" )->pipeline;
  //probe_debug_pipeline = crude_gfx_renderer_access_technique_pass_by_name( renderer, "ray_tracing", "probe_debug" )->pipeline;
  //calculate_probe_offset_pipeline = crude_gfx_renderer_access_technique_pass_by_name( renderer, "ray_tracing", "calculate_probe_offsets" )->pipeline;
  
  {
    crude_gfx_texture                                     *probe_raytrace_radiance_texture;
    crude_gfx_probe_raytracer_push_constant_               push_constant;
    crude_gfx_pipeline_handle                              pipeline;
    
    probe_raytrace_radiance_texture = crude_gfx_access_texture( gpu, pass->probe_raytrace_radiance_texture_handle );
    
    pipeline = crude_gfx_access_technique_pass_by_name( pass->scene_renderer->gpu, "ddgi", "probe_raytracer" )->pipeline;

    crude_gfx_cmd_push_marker( primary_cmd, "probe_raytracer" );
    
    crude_gfx_cmd_bind_pipeline( primary_cmd, pipeline );
    crude_gfx_cmd_bind_bindless_descriptor_set( primary_cmd );
    crude_gfx_cmd_bind_acceleration_structure_descriptor_set( primary_cmd, pass->scene_renderer->acceleration_stucture_ds );

    CRUDE_ASSERT( sizeof( VkAccelerationStructureKHR ) == sizeof( uint64 ) );

    push_constant = CRUDE_COMPOUNT_EMPTY( crude_gfx_probe_raytracer_push_constant_ );
    push_constant.scene = pass->scene_renderer->scene_hga.gpu_address;
    push_constant.lights = pass->scene_renderer->lights_hga.gpu_address;
    push_constant.mesh_draws = pass->scene_renderer->model_renderer_resources_manager->meshes_draws_hga.gpu_address;
    push_constant.mesh_instance_draws = pass->scene_renderer->meshes_instances_draws_hga.gpu_address;
    push_constant.ddgi = ddgi_constants;
    crude_gfx_cmd_push_constant( primary_cmd, &push_constant, sizeof( push_constant ) );
     
    crude_gfx_cmd_add_image_barrier( primary_cmd, probe_raytrace_radiance_texture, CRUDE_GFX_RESOURCE_STATE_UNORDERED_ACCESS, 0, 1, false );
    crude_gfx_cmd_trace_rays( primary_cmd, pipeline, pass->probe_rays, pass->options.probe_update_per_frame, 1u );
    crude_gfx_cmd_add_image_barrier( primary_cmd, probe_raytrace_radiance_texture, CRUDE_GFX_RESOURCE_STATE_UNORDERED_ACCESS, 0, 1, false );
    crude_gfx_cmd_pop_marker( primary_cmd );
  }
  //
  //if ( pass->offsets_calculations_count >= 0 )
  //{
  //  --pass->offsets_calculations_count;
  //  crude_gfx_cmd_push_marker( primary_cmd, "probe_offsets" );
  //  
  //  crude_gfx_cmd_add_image_barrier( primary_cmd, crude_gfx_access_texture( renderer->gpu, pass->probe_offsets_texture_handle ), CRUDE_GFX_RESOURCE_STATE_UNORDERED_ACCESS, 0, 1, false );
  //  crude_gfx_cmd_bind_pipeline( primary_cmd, calculate_probe_offset_pipeline );
  //  crude_gfx_cmd_bind_descriptor_set( primary_cmd, pass->calculate_probe_offsets_ds[ renderer->gpu->current_frame ] );
  //  
  //  uint32 first_frame = pass->offsets_calculations_count == 23 ? 1 : 0;
  //  crude_gfx_cmd_push_constant( primary_cmd, &first_frame, sizeof( first_frame ) );
  //  crude_gfx_cmd_dispatch( primary_cmd, ( probe_count + 31 ) / 32, 1u, 1 );
  //  crude_gfx_cmd_pop_marker( primary_cmd );
  //}
  //
  //crude_gfx_cmd_push_marker( primary_cmd, "probe_update_irradiance" );
  //crude_gfx_cmd_add_image_barrier( primary_cmd, crude_gfx_access_texture( renderer->gpu, pass->probe_grid_irradiance_texture_handle ), CRUDE_GFX_RESOURCE_STATE_UNORDERED_ACCESS, 0, 1, false );
  //crude_gfx_cmd_bind_pipeline( primary_cmd, probe_update_irradiance_pipeline );
  //crude_gfx_cmd_bind_descriptor_set( primary_cmd, pass->probe_update_irradiance_ds[ renderer->gpu->current_frame ] );
  //crude_gfx_cmd_dispatch( primary_cmd, 
  //  ( pass->irradiance_side_length_with_borders + 7 ) / 8,
  //  ( pass->irradiance_side_length_with_borders + 7 ) / 8,
  //  pass->options.probe_update_per_frame
  //);
  //crude_gfx_cmd_pop_marker( primary_cmd );
  //
  //crude_gfx_cmd_push_marker( primary_cmd, "probe_update_visibility" );
  //crude_gfx_cmd_add_image_barrier( primary_cmd, crude_gfx_access_texture( renderer->gpu, pass->probe_grid_visibility_texture_handle ), CRUDE_GFX_RESOURCE_STATE_UNORDERED_ACCESS, 0, 1, false );
  //crude_gfx_cmd_bind_pipeline( primary_cmd, probe_update_visibility_pipeline );
  //crude_gfx_cmd_bind_descriptor_set( primary_cmd, pass->probe_update_visibility_ds[ renderer->gpu->current_frame ] );
  //crude_gfx_cmd_dispatch( primary_cmd,
  //  ( pass->visibility_side_length_with_borders + 7 ) / 8,
  //  ( pass->visibility_side_length_with_borders + 7 ) / 8,
  //  pass->options.probe_update_per_frame
  //);
  //crude_gfx_cmd_add_image_barrier( primary_cmd, crude_gfx_access_texture( renderer->gpu, pass->probe_grid_irradiance_texture_handle ), CRUDE_GFX_RESOURCE_STATE_UNORDERED_ACCESS, 0, 1, false );
  //crude_gfx_cmd_add_image_barrier( primary_cmd, crude_gfx_access_texture( renderer->gpu, pass->probe_grid_visibility_texture_handle ), CRUDE_GFX_RESOURCE_STATE_UNORDERED_ACCESS, 0, 1, false );
  //crude_gfx_cmd_pop_marker( primary_cmd );
  //
  //crude_gfx_cmd_push_marker( primary_cmd, "sample_irradiance" );
  //crude_gfx_cmd_add_image_barrier( primary_cmd, crude_gfx_access_texture( renderer->gpu, pass->indirect_texture_handle ), CRUDE_GFX_RESOURCE_STATE_UNORDERED_ACCESS, 0, 1, false );
  //crude_gfx_cmd_bind_pipeline( primary_cmd, sample_irradiance_pipeline );
  //crude_gfx_cmd_bind_descriptor_set( primary_cmd, pass->sample_irradiance_ds[ renderer->gpu->current_frame ] );
  //uint32 half_resolution = pass->use_half_resolution ? 1 : 0;
  //crude_gfx_cmd_push_constant( primary_cmd, &half_resolution, sizeof( half_resolution ) );
  //float32 resolution_divider = pass->use_half_resolution ? 0.5f : 1.0f;
  //uint32 width = renderer->gpu->vk_swapchain_width * resolution_divider;
  //uint32 height = renderer->gpu->vk_swapchain_height * resolution_divider;
  //crude_gfx_cmd_dispatch( primary_cmd, ( width + 7 ) / 8, ( height + 7 ) / 8, 1 );
  //crude_gfx_cmd_add_image_barrier( primary_cmd, crude_gfx_access_texture( renderer->gpu, pass->indirect_texture_handle ), CRUDE_GFX_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, 0, 1, false );
  //crude_gfx_cmd_pop_marker( primary_cmd );
  //
  //crude_gfx_cmd_push_marker( primary_cmd, "probe_debug" );
  //crude_gfx_cmd_bind_pipeline( primary_cmd, probe_debug_pipeline );
  //crude_gfx_cmd_bind_descriptor_set( primary_cmd, pass->probe_debug_ds[ renderer->gpu->current_frame ] );
  //crude_gfx_cmd_push_constant( primary_cmd, &pass->options.probe_debug_flags, sizeof( pass->options.probe_debug_flags ) );
  //crude_gfx_cmd_dispatch( primary_cmd, ( probe_count + 31 ) / 32, 1u, 1 );
  //crude_gfx_cmd_pop_marker( primary_cmd );
}

void
crude_gfx_indirect_light_pass_on_resize
(
  _In_ void                                               *ctx,
  _In_ uint32                                              new_width,
  _In_ uint32                                              new_height
)
{
  crude_gfx_indirect_light_pass                           *pass;
  crude_gfx_texture_creation                               texture_creation;

  pass = CRUDE_REINTERPRET_CAST( crude_gfx_indirect_light_pass*, ctx );

  uint32 new_indirect_texture_width = pass->use_half_resolution ? ( pass->scene_renderer->gpu->renderer_size.x ) / 2 : pass->scene_renderer->gpu->renderer_size.x;
  uint32 new_indirect_texture_height = pass->use_half_resolution ? ( pass->scene_renderer->gpu->renderer_size.y ) / 2 : pass->scene_renderer->gpu->renderer_size.y;
  crude_gfx_resize_texture( pass->scene_renderer->gpu, pass->indirect_texture_handle, new_indirect_texture_width, new_indirect_texture_height );
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
  container.on_resize = crude_gfx_indirect_light_pass_on_resize;
  return container;
}

#endif /* CRUDE_GFX_RAY_TRACING_DDGI_ENABLED */