#include <engine/core/profiler.h>
#include <engine/core/hashmapstr.h>
#include <engine/scene/scene_ecs.h>
#include <engine/graphics/scene_renderer.h>

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

  probe_count = scene_renderer->options.indirect_light.probe_count_x * scene_renderer->options.indirect_light.probe_count_y * scene_renderer->options.indirect_light.probe_count_z;

  pass->probe_update_offset = 0u;
  pass->irradiance_side_length = 6;
  pass->visibility_side_length = 6;
  pass->irradiance_side_length_with_borders = pass->irradiance_side_length + 2;
  pass->visibility_side_length_with_borders = pass->visibility_side_length + 2;
  pass->irradiance_atlas_width = ( pass->irradiance_side_length_with_borders * scene_renderer->options.indirect_light.probe_count_x * scene_renderer->options.indirect_light.probe_count_y );
  pass->irradiance_atlas_height = ( pass->irradiance_side_length_with_borders * scene_renderer->options.indirect_light.probe_count_z );
  pass->visibility_atlas_width = ( pass->visibility_side_length_with_borders * scene_renderer->options.indirect_light.probe_count_x * scene_renderer->options.indirect_light.probe_count_y );
  pass->visibility_atlas_height = ( pass->visibility_side_length_with_borders * scene_renderer->options.indirect_light.probe_count_z );
  
  gpu = pass->scene_renderer->gpu;

  texture_creation = crude_gfx_texture_creation_empty( );
  texture_creation.width = scene_renderer->options.indirect_light.probe_rays;
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
  texture_creation.width = scene_renderer->options.indirect_light.probe_count_x * scene_renderer->options.indirect_light.probe_count_y;
  texture_creation.height = scene_renderer->options.indirect_light.probe_count_z;
  texture_creation.format = VK_FORMAT_R16G16B16A16_SFLOAT;
  texture_creation.type = CRUDE_GFX_TEXTURE_TYPE_TEXTURE_2D;
  texture_creation.flags = CRUDE_GFX_TEXTURE_MASK_COMPUTE;
  crude_string_copy( texture_creation.name, "probe_offsets", sizeof( texture_creation.name ) );
  pass->probe_offsets_texture_handle = crude_gfx_create_texture( pass->scene_renderer->gpu, &texture_creation );
  
  texture_creation = crude_gfx_texture_creation_empty( );
  texture_creation.width = scene_renderer->options.indirect_light.use_half_resolution ? ( pass->scene_renderer->gpu->renderer_size.x ) / 2 : pass->scene_renderer->gpu->renderer_size.x;
  texture_creation.height = scene_renderer->options.indirect_light.use_half_resolution ? ( pass->scene_renderer->gpu->renderer_size.y ) / 2 : pass->scene_renderer->gpu->renderer_size.y;
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
  uint32                                                   probe_count;

  pass = CRUDE_REINTERPRET_CAST( crude_gfx_indirect_light_pass*, ctx );
  gpu = pass->scene_renderer->gpu;

  probe_count = pass->scene_renderer->options.indirect_light.probe_count_x * pass->scene_renderer->options.indirect_light.probe_count_y * pass->scene_renderer->options.indirect_light.probe_count_z;
  pass->probe_update_offset = ( pass->probe_update_offset + pass->scene_renderer->options.indirect_light.probe_update_per_frame ) % probe_count;
  
  /* Probe Raytracer */
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

    push_constant = CRUDE_COMPOUNT_EMPTY( crude_gfx_probe_raytracer_push_constant_ );
    push_constant.scene = pass->scene_renderer->scene_hga.gpu_address;
    push_constant.lights = pass->scene_renderer->lights_hga.gpu_address;
    push_constant.mesh_draws = pass->scene_renderer->model_renderer_resources_manager->meshes_draws_hga.gpu_address;
    push_constant.mesh_instance_draws = pass->scene_renderer->meshes_instances_draws_hga.gpu_address;
    push_constant.ddgi = pass->scene_renderer->ddgi_hga.gpu_address;
    crude_gfx_cmd_push_constant( primary_cmd, &push_constant, sizeof( push_constant ) );
     
    crude_gfx_cmd_add_image_barrier( primary_cmd, probe_raytrace_radiance_texture, CRUDE_GFX_RESOURCE_STATE_UNORDERED_ACCESS, 0, 1, false );
    crude_gfx_cmd_trace_rays( primary_cmd, pipeline, pass->scene_renderer->options.indirect_light.probe_rays, pass->scene_renderer->options.indirect_light.probe_update_per_frame, 1u );
    crude_gfx_cmd_add_image_barrier( primary_cmd, probe_raytrace_radiance_texture, CRUDE_GFX_RESOURCE_STATE_UNORDERED_ACCESS, 0, 1, false );
    crude_gfx_cmd_pop_marker( primary_cmd );
  }
  
  /* Calculate Probe Offsets */
  if ( pass->scene_renderer->options.indirect_light.offsets_calculations_count >= 0 )
  {
    crude_gfx_texture                                     *probe_offsets_texture;
    crude_gfx_calculate_probe_offsets_push_constant_       push_constant;
    crude_gfx_pipeline_handle                              pipeline;
    uint32                                                 first_frame;
    
    first_frame = pass->scene_renderer->options.indirect_light.offsets_calculations_count == 23 ? 1 : 0;
    --pass->scene_renderer->options.indirect_light.offsets_calculations_count;
    
    probe_offsets_texture = crude_gfx_access_texture( gpu, pass->probe_offsets_texture_handle );

    pipeline = crude_gfx_access_technique_pass_by_name( gpu, "ddgi", "calculate_probe_offsets" )->pipeline;

    crude_gfx_cmd_push_marker( primary_cmd, "calculate_probe_offsets" );
    
    crude_gfx_cmd_add_image_barrier( primary_cmd, probe_offsets_texture, CRUDE_GFX_RESOURCE_STATE_UNORDERED_ACCESS, 0, 1, false );
    crude_gfx_cmd_bind_pipeline( primary_cmd, pipeline );
    crude_gfx_cmd_bind_bindless_descriptor_set( primary_cmd );
    
    push_constant = CRUDE_COMPOUNT_EMPTY( crude_gfx_calculate_probe_offsets_push_constant_ );
    push_constant.ddgi = pass->scene_renderer->ddgi_hga.gpu_address;
    push_constant.first_frame = first_frame;
    crude_gfx_cmd_push_constant( primary_cmd, &push_constant, sizeof( push_constant ) );

    crude_gfx_cmd_dispatch( primary_cmd, ( probe_count + 31 ) / 32, 1u, 1 );

    crude_gfx_cmd_pop_marker( primary_cmd );
  }

  /* Probe Update Irradiance */
  {
    crude_gfx_texture                                     *probe_grid_irradiance_texture;
    crude_gfx_probe_update_push_constant_                  push_constant;
    crude_gfx_pipeline_handle                              pipeline;
    
    pipeline = crude_gfx_access_technique_pass_by_name( gpu, "ddgi", "probe_update_irradiance" )->pipeline;
    
    probe_grid_irradiance_texture = crude_gfx_access_texture( gpu, pass->probe_grid_irradiance_texture_handle );

    crude_gfx_cmd_push_marker( primary_cmd, "probe_update_irradiance" );

    crude_gfx_cmd_add_image_barrier( primary_cmd, probe_grid_irradiance_texture, CRUDE_GFX_RESOURCE_STATE_UNORDERED_ACCESS, 0, 1, false );
    crude_gfx_cmd_bind_pipeline( primary_cmd, pipeline );
    crude_gfx_cmd_bind_bindless_descriptor_set( primary_cmd );
    
    push_constant = CRUDE_COMPOUNT_EMPTY( crude_gfx_probe_update_push_constant_ );
    push_constant.ddgi = pass->scene_renderer->ddgi_hga.gpu_address;
    push_constant.irradiance_image_index = pass->probe_grid_irradiance_texture_handle.index;
    push_constant.visibility_image_index = pass->probe_grid_visibility_texture_handle.index;
    crude_gfx_cmd_push_constant( primary_cmd, &push_constant, sizeof( push_constant ) );

    crude_gfx_cmd_dispatch( primary_cmd, 
      ( pass->irradiance_side_length_with_borders + 7 ) / 8,
      ( pass->irradiance_side_length_with_borders + 7 ) / 8,
      pass->scene_renderer->options.indirect_light.probe_update_per_frame
    );

    crude_gfx_cmd_add_image_barrier( primary_cmd, probe_grid_irradiance_texture, CRUDE_GFX_RESOURCE_STATE_UNORDERED_ACCESS, 0, 1, false );

    crude_gfx_cmd_pop_marker( primary_cmd );
  }
  
  /* Probe Update Visibility */
  {
    crude_gfx_texture                                     *probe_grid_visibility_texture;
    crude_gfx_probe_update_push_constant_                  push_constant;
    crude_gfx_pipeline_handle                              pipeline;
    
    pipeline = crude_gfx_access_technique_pass_by_name( gpu, "ddgi", "probe_update_irradiance" )->pipeline;
    
    probe_grid_visibility_texture = crude_gfx_access_texture( gpu, pass->probe_grid_visibility_texture_handle );

    crude_gfx_cmd_push_marker( primary_cmd, "probe_update_visibility" );
    
    crude_gfx_cmd_add_image_barrier( primary_cmd, probe_grid_visibility_texture, CRUDE_GFX_RESOURCE_STATE_UNORDERED_ACCESS, 0, 1, false );
    crude_gfx_cmd_bind_pipeline( primary_cmd, pipeline );
    crude_gfx_cmd_bind_bindless_descriptor_set( primary_cmd );
    
    push_constant = CRUDE_COMPOUNT_EMPTY( crude_gfx_probe_update_push_constant_ );
    push_constant.ddgi = pass->scene_renderer->ddgi_hga.gpu_address;
    push_constant.irradiance_image_index = pass->probe_grid_irradiance_texture_handle.index;
    push_constant.visibility_image_index = pass->probe_grid_visibility_texture_handle.index;
    crude_gfx_cmd_push_constant( primary_cmd, &push_constant, sizeof( push_constant ) );

    crude_gfx_cmd_dispatch( primary_cmd,
      ( pass->visibility_side_length_with_borders + 7 ) / 8,
      ( pass->visibility_side_length_with_borders + 7 ) / 8,
      pass->scene_renderer->options.indirect_light.probe_update_per_frame
    );
    crude_gfx_cmd_add_image_barrier( primary_cmd, probe_grid_visibility_texture, CRUDE_GFX_RESOURCE_STATE_UNORDERED_ACCESS, 0, 1, false );

    crude_gfx_cmd_pop_marker( primary_cmd );
  }

  /* Sample Irradiance */
  {
    crude_gfx_texture                                     *indirect_texture;
    crude_gfx_sample_irradiance_push_constant_             push_constant;
    crude_gfx_pipeline_handle                              pipeline;
    uint32                                                 width, height, half_resolution;
    float32                                                resolution_divider;
    
    half_resolution = pass->scene_renderer->options.indirect_light.use_half_resolution ? 1 : 0;
    resolution_divider = pass->scene_renderer->options.indirect_light.use_half_resolution ? 0.5f : 1.0f;
    width = gpu->renderer_size.x * resolution_divider;
    height = gpu->renderer_size.y * resolution_divider;

    pipeline = crude_gfx_access_technique_pass_by_name( gpu, "ddgi", "sample_irradiance" )->pipeline;
    
    indirect_texture = crude_gfx_access_texture( gpu, pass->indirect_texture_handle );

    crude_gfx_cmd_push_marker( primary_cmd, "sample_irradiance" );

    crude_gfx_cmd_add_image_barrier( primary_cmd, indirect_texture, CRUDE_GFX_RESOURCE_STATE_UNORDERED_ACCESS, 0, 1, false );
    
    crude_gfx_cmd_bind_pipeline( primary_cmd, pipeline );
    crude_gfx_cmd_bind_bindless_descriptor_set( primary_cmd );
    
    push_constant = CRUDE_COMPOUNT_EMPTY( crude_gfx_sample_irradiance_push_constant_ );
    push_constant.ddgi = pass->scene_renderer->ddgi_hga.gpu_address;
    push_constant.scene = pass->scene_renderer->scene_hga.gpu_address;
    push_constant.output_resolution_half = half_resolution;
    crude_gfx_cmd_push_constant( primary_cmd, &push_constant, sizeof( push_constant ) );

    crude_gfx_cmd_dispatch( primary_cmd, ( width + 7 ) / 8, ( height + 7 ) / 8, 1 );
    
    crude_gfx_cmd_add_image_barrier( primary_cmd, indirect_texture, CRUDE_GFX_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, 0, 1, false );
    
    crude_gfx_cmd_pop_marker( primary_cmd );
  }
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

  uint32 new_indirect_texture_width = pass->scene_renderer->options.indirect_light.use_half_resolution ? ( pass->scene_renderer->gpu->renderer_size.x ) / 2 : pass->scene_renderer->gpu->renderer_size.x;
  uint32 new_indirect_texture_height = pass->scene_renderer->options.indirect_light.use_half_resolution ? ( pass->scene_renderer->gpu->renderer_size.y ) / 2 : pass->scene_renderer->gpu->renderer_size.y;
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