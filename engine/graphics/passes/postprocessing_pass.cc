#include <core/hash_map.h>
#include <core/time.h>
#include <graphics/scene_renderer.h>

#include <graphics/passes/postprocessing_pass.h>

typedef struct crude_gfx_luminance_histogram_generation_push_constant
{
  float32                                                  inverse_log_lum_range;
  float32                                                  min_log_lum;
  uint32                                                   hdr_color_texture_index;
} crude_gfx_luminance_histogram_generation_push_constant;

typedef struct crude_gfx_luminance_average_calculation_push_constant
{
  float32                                                  log_lum_range;
  float32                                                  min_log_lum;
  float32                                                  time_coeff;
  uint32                                                   num_pixels;
} crude_gfx_luminance_average_calculation_push_constant;

typedef struct crude_gfx_postprocessing_push_constant
{
  uint32                                                   luminance_average_texture_index;
  uint32                                                   pbr_texture_index;
} crude_gfx_postprocessing_push_constant;

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
  for ( uint32 i = 0; i < CRUDE_GRAPHICS_MAX_SWAPCHAIN_IMAGES; ++i )
  {
    pass->luminance_average_texture_handle[ i ] = crude_gfx_create_texture( pass->scene_renderer->gpu, &luminance_average_texture_creation ); 
  }

  for ( uint32 i = 0; i < CRUDE_GRAPHICS_MAX_SWAPCHAIN_IMAGES; ++i )
  {
    crude_gfx_buffer_creation buffer_creation = crude_gfx_buffer_creation_empty( );
    buffer_creation.type_flags = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
    buffer_creation.usage = CRUDE_GFX_RESOURCE_USAGE_TYPE_IMMUTABLE;
    buffer_creation.size = sizeof( uint32 ) * 256;
    buffer_creation.name = "luminance_histogram_sb";
    buffer_creation.device_only = true;
    pass->packed_data_sb_handle[ i ] = crude_gfx_create_buffer( pass->scene_renderer->gpu, &buffer_creation );
  }

  for ( uint32 i = 0; i < CRUDE_GRAPHICS_MAX_SWAPCHAIN_IMAGES; ++i )
  {
    pass->luminance_histogram_generation_ds[ i ] = CRUDE_GFX_DESCRIPTOR_SET_HANDLE_INVALID;
    pass->luminance_average_calculation_ds[ i ] = CRUDE_GFX_DESCRIPTOR_SET_HANDLE_INVALID;
  }

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
  
  for ( uint32 i = 0; i < CRUDE_GRAPHICS_MAX_SWAPCHAIN_IMAGES; ++i )
  {
    crude_gfx_destroy_texture( gpu, pass->luminance_average_texture_handle[ i ] );
  }

  for ( uint32 i = 0; i < CRUDE_GRAPHICS_MAX_SWAPCHAIN_IMAGES; ++i )
  {
    crude_gfx_destroy_buffer( pass->scene_renderer->gpu, pass->packed_data_sb_handle[ i ] );
  }

  for ( uint32 i = 0; i < CRUDE_GRAPHICS_MAX_SWAPCHAIN_IMAGES; ++i )
  {
    if ( CRUDE_RESOURCE_HANDLE_IS_VALID( pass->luminance_histogram_generation_ds[ i ] ) )
    {
      crude_gfx_destroy_descriptor_set( gpu, pass->luminance_histogram_generation_ds[ i ] );
    }
    if ( CRUDE_RESOURCE_HANDLE_IS_VALID( pass->luminance_average_calculation_ds[ i ] ) )
    {
      crude_gfx_destroy_descriptor_set( gpu, pass->luminance_average_calculation_ds[ i ] );
    }
  }
}

void
crude_gfx_postprocessing_pass_on_render_graph_registered
(
  _In_ crude_gfx_postprocessing_pass                      *pass
)
{
  crude_gfx_device                                        *gpu;
  
  gpu = pass->scene_renderer->gpu;

  crude_gfx_postprocessing_pass_on_resize( pass, gpu->vk_swapchain_width, gpu->vk_swapchain_height );
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
  
  hdr_color_texture_handle = crude_gfx_render_graph_builder_access_resource_by_name( pass->scene_renderer->render_graph->builder, "pbr" )->resource_info.texture.handle;
  hdr_color_texture = crude_gfx_access_texture( gpu, hdr_color_texture_handle );

  luminance_avarge_last_update_delta_time = crude_time_delta_seconds( pass->luminance_avarge_last_update_time, crude_time_now( ) );
  pass->luminance_avarge_last_update_time = crude_time_now( );
  
  crude_gfx_cmd_push_marker( primary_cmd, "luminance_histogram_generation_pass" );
  {
    luminance_histogram_generation_pipeline = crude_gfx_access_technique_pass_by_name( pass->scene_renderer->gpu, "compute", "luminance_histogram_generation" )->pipeline;
  
    crude_gfx_cmd_bind_pipeline( primary_cmd, luminance_histogram_generation_pipeline );
    crude_gfx_cmd_bind_descriptor_set( primary_cmd, pass->luminance_histogram_generation_ds[ gpu->current_frame ] );
  
    histogram_generation_constant = CRUDE_COMPOUNT_EMPTY( crude_gfx_luminance_histogram_generation_push_constant );
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
    crude_gfx_cmd_bind_descriptor_set( primary_cmd, pass->luminance_average_calculation_ds[ gpu->current_frame ] );
 
    luminance_avarge_constant = CRUDE_COMPOUNT_EMPTY( crude_gfx_luminance_average_calculation_push_constant );
    luminance_avarge_constant.log_lum_range = ( pass->max_log_lum - pass->min_log_lum );
    luminance_avarge_constant.min_log_lum = pass->min_log_lum;
    luminance_avarge_constant.num_pixels = hdr_color_texture->width * hdr_color_texture->height;
    luminance_avarge_constant.time_coeff = CRUDE_CLAMP( ( 1 - exp( -luminance_avarge_last_update_delta_time * 1.1f ) ), 1.0, 0.001 );
    crude_gfx_cmd_push_constant( primary_cmd, &luminance_avarge_constant, sizeof( luminance_avarge_constant ) );
  
    crude_gfx_cmd_add_image_barrier( primary_cmd, crude_gfx_access_texture( gpu, pass->luminance_average_texture_handle[ gpu->current_frame ] ), CRUDE_GFX_RESOURCE_STATE_UNORDERED_ACCESS, 0u, 1u, false );
    crude_gfx_cmd_dispatch( primary_cmd, 1u, 1u, 1u );
    crude_gfx_cmd_add_image_barrier( primary_cmd, crude_gfx_access_texture( gpu, pass->luminance_average_texture_handle[ gpu->current_frame ] ), CRUDE_GFX_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, 0u, 1u, false );
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
  crude_gfx_cmd_bind_descriptor_set( primary_cmd, CRUDE_GFX_DESCRIPTOR_SET_HANDLE_INVALID );

  pbr_texture_handle = crude_gfx_render_graph_builder_access_resource_by_name( pass->scene_renderer->render_graph->builder, "pbr" )->resource_info.texture.handle;
  
  postprocessing_constant = CRUDE_COMPOUNT_EMPTY( crude_gfx_postprocessing_push_constant );
  postprocessing_constant.luminance_average_texture_index = pass->luminance_average_texture_handle[ gpu->current_frame ].index;
  postprocessing_constant.pbr_texture_index = pbr_texture_handle.index;
  crude_gfx_cmd_push_constant( primary_cmd, &postprocessing_constant, sizeof( postprocessing_constant ) );
  crude_gfx_cmd_draw( primary_cmd, 0u, 3u, 0u, 1u );

  crude_gfx_cmd_pop_marker( primary_cmd );
}

void
crude_gfx_postprocessing_pass_on_resize
(
  _In_ void                                               *ctx,
  _In_ uint32                                              new_width,
  _In_ uint32                                              new_height
)
{
  crude_gfx_postprocessing_pass                           *pass;
  
  pass = CRUDE_REINTERPRET_CAST( crude_gfx_postprocessing_pass*, ctx );
  
  crude_gfx_postprocessing_pass_on_techniques_reloaded( ctx );
}

void
crude_gfx_postprocessing_pass_on_techniques_reloaded
(
  _In_ void                                               *ctx
)
{
  crude_gfx_postprocessing_pass                           *pass;
  crude_gfx_device                                        *gpu;
  crude_gfx_descriptor_set_layout_handle                   luminance_histogram_generation_dsl;
  crude_gfx_pipeline_handle                                luminance_histogram_generation_pipeline;
  crude_gfx_descriptor_set_layout_handle                   luminance_average_calculation_dsl;
  crude_gfx_pipeline_handle                                luminance_average_calculation_pipeline;
  
  pass = CRUDE_REINTERPRET_CAST( crude_gfx_postprocessing_pass*, ctx );
  gpu = pass->scene_renderer->gpu;
  
  for ( uint32 i = 0; i < CRUDE_GRAPHICS_MAX_SWAPCHAIN_IMAGES; ++i )
  {
    if ( CRUDE_RESOURCE_HANDLE_IS_VALID( pass->luminance_histogram_generation_ds[ i ] ) )
    {
      crude_gfx_destroy_descriptor_set( pass->scene_renderer->gpu, pass->luminance_histogram_generation_ds[ i ] );
    }
    if ( CRUDE_RESOURCE_HANDLE_IS_VALID( pass->luminance_average_calculation_ds[ i ] ) )
    {
      crude_gfx_destroy_descriptor_set( pass->scene_renderer->gpu, pass->luminance_average_calculation_ds[ i ] );
    }
  }

  luminance_histogram_generation_pipeline = crude_gfx_access_technique_pass_by_name( pass->scene_renderer->gpu, "compute", "luminance_histogram_generation" )->pipeline;
  luminance_histogram_generation_dsl = crude_gfx_get_descriptor_set_layout( gpu, luminance_histogram_generation_pipeline, CRUDE_GRAPHICS_MATERIAL_DESCRIPTOR_SET_INDEX );

  for ( uint32 i = 0; i < CRUDE_GRAPHICS_MAX_SWAPCHAIN_IMAGES; ++i )
  {
    crude_gfx_descriptor_set_creation                      ds_creation;

    ds_creation = crude_gfx_descriptor_set_creation_empty();
    ds_creation.name = "luminance_histogram_generation_ds";
    ds_creation.layout = luminance_histogram_generation_dsl;
    
    crude_gfx_descriptor_set_creation_add_buffer( &ds_creation, pass->packed_data_sb_handle[ i ], 0u );
    
    pass->luminance_histogram_generation_ds[ i ] = crude_gfx_create_descriptor_set( gpu, &ds_creation );
  }

  luminance_average_calculation_pipeline = crude_gfx_access_technique_pass_by_name( pass->scene_renderer->gpu, "compute", "luminance_average_calculation" )->pipeline;
  luminance_average_calculation_dsl = crude_gfx_get_descriptor_set_layout( gpu, luminance_average_calculation_pipeline, CRUDE_GRAPHICS_MATERIAL_DESCRIPTOR_SET_INDEX );

  for ( uint32 i = 0; i < CRUDE_GRAPHICS_MAX_SWAPCHAIN_IMAGES; ++i )
  {
    crude_gfx_descriptor_set_creation                      ds_creation;

    ds_creation = crude_gfx_descriptor_set_creation_empty();
    ds_creation.name = "luminance_average_calculation_ds";
    ds_creation.layout = luminance_average_calculation_dsl;
    
    crude_gfx_descriptor_set_creation_add_texture( &ds_creation, pass->luminance_average_texture_handle[ i ], 0u );
    crude_gfx_descriptor_set_creation_add_buffer( &ds_creation, pass->packed_data_sb_handle[ i ], 1u );
    
    pass->luminance_average_calculation_ds[ i ] = crude_gfx_create_descriptor_set( gpu, &ds_creation );
  }
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
  container.on_techniques_reloaded = crude_gfx_postprocessing_pass_on_techniques_reloaded;
  container.on_resize = crude_gfx_postprocessing_pass_on_resize;
  return container;
}