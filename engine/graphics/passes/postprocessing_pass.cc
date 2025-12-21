#include <engine/core/hash_map.h>
#include <engine/core/time.h>
#include <engine/graphics/scene_renderer.h>

#include <engine/graphics/passes/postprocessing_pass.h>

CRUDE_ALIGNED_STRUCT( 16 ) crude_gfx_luminance_histogram_generation_push_constant
{
  VkDeviceAddress                                          histogram;
  float32                                                  inverse_log_lum_range;
  float32                                                  min_log_lum;
  uint32                                                   hdr_color_texture_index;
};

CRUDE_ALIGNED_STRUCT( 16 ) crude_gfx_luminance_average_calculation_push_constant
{
  VkDeviceAddress                                          histogram;
  float32                                                  log_lum_range;
  float32                                                  min_log_lum;
  float32                                                  time_coeff;
  uint32                                                   num_pixels;
  uint32                                                   luminance_avarage_texture;
};

CRUDE_ALIGNED_STRUCT( 16 ) crude_gfx_postprocessing_push_constant
{
  uint32                                                   luminance_average_texture_index;
  uint32                                                   pbr_texture_index;
  float32                                                  inv_gamma;
};

void
crude_gfx_postprocessing_pass_initialize
(
  _In_ crude_gfx_postprocessing_pass                      *pass,
  _In_ crude_gfx_scene_renderer                           *scene_renderer
)
{
  crude_gfx_texture_creation                               luminance_average_texture_creation;

  pass->scene_renderer = scene_renderer;

  luminance_average_texture_creation = crude_gfx_texture_creation_empty();
  luminance_average_texture_creation.format = VK_FORMAT_R32_SFLOAT;
  luminance_average_texture_creation.type = CRUDE_GFX_TEXTURE_TYPE_TEXTURE_2D;
  luminance_average_texture_creation.width = 1u;
  luminance_average_texture_creation.height = 1u;
  luminance_average_texture_creation.depth = 1u;
  luminance_average_texture_creation.flags = CRUDE_GFX_TEXTURE_MASK_COMPUTE;
  luminance_average_texture_creation.name = "luminance_average_texture";
  pass->luminance_average_texture_handle = crude_gfx_create_texture( pass->scene_renderer->gpu, &luminance_average_texture_creation ); 
  
  pass->luminance_histogram_hga = crude_gfx_memory_allocate_with_name( pass->scene_renderer->gpu, sizeof( uint32 ) * 256, CRUDE_GFX_MEMORY_TYPE_GPU, "luminance_histogram_hga" );

  pass->luminance_avarge_last_update_time = 0.f;
  pass->min_log_lum = -8.0f;
  pass->max_log_lum = 3.5f;
}

void
crude_gfx_postprocessing_pass_deinitialize
(
  _In_ crude_gfx_postprocessing_pass                      *pass
)
{
  crude_gfx_device *gpu = pass->scene_renderer->gpu;
  
  crude_gfx_destroy_texture( gpu, pass->luminance_average_texture_handle );
  crude_gfx_memory_deallocate( gpu, pass->luminance_histogram_hga );
}

void
crude_gfx_postprocessing_pass_pre_render
(
  _In_ void                                               *ctx,
  _In_ crude_gfx_cmd_buffer                               *primary_cmd
)
{
  crude_gfx_device                                        *gpu;
  crude_gfx_postprocessing_pass                           *pass;
  crude_gfx_texture                                       *hdr_color_texture;
  crude_gfx_pipeline_handle                                luminance_average_calculation_pipeline;
  crude_gfx_pipeline_handle                                luminance_histogram_generation_pipeline;
  crude_gfx_luminance_histogram_generation_push_constant   histogram_generation_constant;
  crude_gfx_luminance_average_calculation_push_constant    luminance_avarge_constant;
  crude_gfx_texture_handle                                 hdr_color_texture_handle;
  float32                                                  luminance_avarge_last_update_delta_time;

  pass = CRUDE_REINTERPRET_CAST( crude_gfx_postprocessing_pass*, ctx );
  gpu = pass->scene_renderer->gpu;
  
  hdr_color_texture_handle = crude_gfx_render_graph_builder_access_resource_by_name( pass->scene_renderer->render_graph->builder, pass->scene_renderer->options.hdr_pre_tonemapping_texture_name )->resource_info.texture.handle;
  hdr_color_texture = crude_gfx_access_texture( gpu, hdr_color_texture_handle );

  luminance_avarge_last_update_delta_time = crude_time_delta_seconds( pass->luminance_avarge_last_update_time, crude_time_now( ) );
  pass->luminance_avarge_last_update_time = crude_time_now( );
  
  crude_gfx_cmd_push_marker( primary_cmd, "luminance_histogram_generation_pass" );
  {
    luminance_histogram_generation_pipeline = crude_gfx_access_technique_pass_by_name( pass->scene_renderer->gpu, "compute", "luminance_histogram_generation" )->pipeline;
  
    crude_gfx_cmd_bind_pipeline( primary_cmd, luminance_histogram_generation_pipeline );
    crude_gfx_cmd_bind_bindless_descriptor_set( primary_cmd );
  
    histogram_generation_constant = CRUDE_COMPOUNT_EMPTY( crude_gfx_luminance_histogram_generation_push_constant );
    histogram_generation_constant.histogram = pass->luminance_histogram_hga.gpu_address;
    histogram_generation_constant.hdr_color_texture_index = hdr_color_texture_handle.index;
    histogram_generation_constant.inverse_log_lum_range = 1.f / ( pass->max_log_lum - pass->min_log_lum );
    histogram_generation_constant.min_log_lum = pass->min_log_lum;
    crude_gfx_cmd_push_constant( primary_cmd, &histogram_generation_constant, sizeof( histogram_generation_constant ) );
  
    crude_gfx_cmd_dispatch( primary_cmd, ( hdr_color_texture->width + 15u ) / 16u, ( hdr_color_texture->height + 15u ) / 16u, 1u );
  }
  crude_gfx_cmd_pop_marker( primary_cmd );
  
  crude_gfx_cmd_push_marker( primary_cmd, "luminance_average_calculation_pass" );
  {
    luminance_average_calculation_pipeline = crude_gfx_access_technique_pass_by_name( pass->scene_renderer->gpu, "compute", "luminance_average_calculation" )->pipeline;
    crude_gfx_cmd_bind_pipeline( primary_cmd, luminance_average_calculation_pipeline );
    crude_gfx_cmd_bind_bindless_descriptor_set( primary_cmd );
 
    luminance_avarge_constant = CRUDE_COMPOUNT_EMPTY( crude_gfx_luminance_average_calculation_push_constant );
    luminance_avarge_constant.histogram = pass->luminance_histogram_hga.gpu_address;
    luminance_avarge_constant.luminance_avarage_texture = pass->luminance_average_texture_handle.index;
    luminance_avarge_constant.log_lum_range = ( pass->max_log_lum - pass->min_log_lum );
    luminance_avarge_constant.min_log_lum = pass->min_log_lum;
    luminance_avarge_constant.num_pixels = hdr_color_texture->width * hdr_color_texture->height;
    luminance_avarge_constant.time_coeff = CRUDE_CLAMP( ( 1 - exp( -luminance_avarge_last_update_delta_time * 1.1f ) ), 1.0, 0.001 );
    crude_gfx_cmd_push_constant( primary_cmd, &luminance_avarge_constant, sizeof( luminance_avarge_constant ) );
  
    crude_gfx_cmd_add_image_barrier( primary_cmd, crude_gfx_access_texture( gpu, pass->luminance_average_texture_handle ), CRUDE_GFX_RESOURCE_STATE_UNORDERED_ACCESS, 0u, 1u, false );
    crude_gfx_cmd_dispatch( primary_cmd, 1u, 1u, 1u );
    crude_gfx_cmd_add_image_barrier( primary_cmd, crude_gfx_access_texture( gpu, pass->luminance_average_texture_handle ), CRUDE_GFX_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, 0u, 1u, false );
  }
  crude_gfx_cmd_pop_marker( primary_cmd );
}

void
crude_gfx_postprocessing_pass_render
(
  _In_ void                                               *ctx,
  _In_ crude_gfx_cmd_buffer                               *primary_cmd
)
{
  crude_gfx_postprocessing_pass                           *pass;
  crude_gfx_device                                        *gpu;
  crude_gfx_postprocessing_push_constant                   postprocessing_constant;
  crude_gfx_pipeline_handle                                postprocessing_pipeline;
  crude_gfx_texture_handle                                 pbr_texture_handle;

  pass = CRUDE_REINTERPRET_CAST( crude_gfx_postprocessing_pass*, ctx );
  gpu = pass->scene_renderer->gpu;
  
  postprocessing_pipeline = crude_gfx_access_technique_pass_by_name( pass->scene_renderer->gpu, "fullscreen", "postprocessing" )->pipeline;
  
  crude_gfx_cmd_push_marker( primary_cmd, "postprocessing_main_pass" );

  crude_gfx_cmd_bind_pipeline( primary_cmd, postprocessing_pipeline );
  crude_gfx_cmd_bind_bindless_descriptor_set( primary_cmd );

  pbr_texture_handle = crude_gfx_render_graph_builder_access_resource_by_name( pass->scene_renderer->render_graph->builder, pass->scene_renderer->options.hdr_pre_tonemapping_texture_name )->resource_info.texture.handle;
  
  postprocessing_constant = CRUDE_COMPOUNT_EMPTY( crude_gfx_postprocessing_push_constant );
  postprocessing_constant.luminance_average_texture_index = pass->luminance_average_texture_handle.index;
  postprocessing_constant.pbr_texture_index = pbr_texture_handle.index;
  postprocessing_constant.inv_gamma = 1.f / pass->scene_renderer->options.gamma;
  crude_gfx_cmd_push_constant( primary_cmd, &postprocessing_constant, sizeof( postprocessing_constant ) );
  crude_gfx_cmd_draw( primary_cmd, 0u, 3u, 0u, 1u );

  crude_gfx_cmd_pop_marker( primary_cmd );
}


crude_gfx_render_graph_pass_container
crude_gfx_postprocessing_pass_pack
(
  _In_ crude_gfx_postprocessing_pass                      *pass
)
{
  crude_gfx_render_graph_pass_container container = crude_gfx_render_graph_pass_container_empty();
  container.ctx = pass;
  container.pre_render = crude_gfx_postprocessing_pass_pre_render;
  container.render = crude_gfx_postprocessing_pass_render;
  return container;
}