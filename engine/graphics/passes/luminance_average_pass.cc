#include <core/hash_map.h>
#include <graphics/scene_renderer.h>

#include <graphics/passes/luminance_average_pass.h>

typedef struct crude_gfx_luminance_histogram_generation_push_constant
{
	float32                                                  inverse_log_lum_range;
	float32                                                  min_log_lum;
  uint32                                                   hdr_color_texture_index;
} crude_gfx_luminance_histogram_generation_push_constant;

void
crude_gfx_luminance_average_pass_initialize
(
  _In_ crude_gfx_luminance_average_pass                   *pass,
  _In_ crude_gfx_scene_renderer                           *scene_renderer
)
{
  pass->scene_renderer = scene_renderer;

  for ( uint32 i = 0; i < CRUDE_GFX_MAX_SWAPCHAIN_IMAGES; ++i )
  {
    crude_gfx_buffer_creation buffer_creation = CRUDE_COMPOUNT_EMPTY( crude_gfx_buffer_creation );
    buffer_creation.type_flags = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
    buffer_creation.usage = CRUDE_GFX_RESOURCE_USAGE_TYPE_IMMUTABLE;
    buffer_creation.size = sizeof( uint32 ) * 256;
    buffer_creation.name = "luminance_histogram_sb";
    pass->luminance_histogram_sb_handle[ i ] = crude_gfx_create_buffer( pass->scene_renderer->renderer->gpu, &buffer_creation );
  }

  for ( uint32 i = 0; i < CRUDE_GFX_MAX_SWAPCHAIN_IMAGES; ++i )
  {
    pass->luminance_histogram_generation_ds[ i ] = CRUDE_GFX_DESCRIPTOR_SET_HANDLE_INVALID;
    pass->luminance_average_texture_handle[ i ] = CRUDE_GFX_TEXTURE_HANDLE_INVALID;
  }
}

void
crude_gfx_luminance_average_pass_deinitialize
(
  _In_ crude_gfx_luminance_average_pass                   *pass
)
{
  crude_gfx_device *gpu = pass->scene_renderer->renderer->gpu;
  
  for ( uint32 i = 0; i < CRUDE_GFX_MAX_SWAPCHAIN_IMAGES; ++i )
  {
    crude_gfx_destroy_buffer( pass->scene_renderer->renderer->gpu, pass->luminance_histogram_sb_handle[ i ] );
  }

  for ( uint32 i = 0; i < CRUDE_GFX_MAX_SWAPCHAIN_IMAGES; ++i )
  {
    if ( CRUDE_RESOURCE_HANDLE_IS_VALID( pass->luminance_histogram_generation_ds[ i ] ) )
    {
      crude_gfx_destroy_descriptor_set( gpu, pass->luminance_histogram_generation_ds[ i ] );
    }
    if ( CRUDE_RESOURCE_HANDLE_IS_VALID( pass->luminance_average_texture_handle[ i ] ) )
    {
      crude_gfx_destroy_texture( gpu, pass->luminance_average_texture_handle[ i ] );
    }
  }
}

void
crude_gfx_luminance_average_pass_on_render_graph_registered
(
  _In_ crude_gfx_luminance_average_pass                   *pass
)
{
  crude_gfx_device                                        *gpu;
  
  gpu = pass->scene_renderer->renderer->gpu;

  crude_gfx_luminance_average_pass_on_resize( pass, gpu->vk_swapchain_width, gpu->vk_swapchain_height );
}

void
crude_gfx_luminance_average_pass_render
(
  _In_ void                                               *ctx,
  _In_ crude_gfx_cmd_buffer                               *primary_cmd
)
{

  crude_gfx_device                                        *gpu;
  crude_gfx_luminance_average_pass                        *pass;
  crude_gfx_pipeline_handle                                luminance_histogram_generation_pipeline;
  crude_gfx_luminance_histogram_generation_push_constant   constant;
  crude_gfx_texture_handle                                 hdr_color_texture_handle;
  crude_gfx_texture                                       *hdr_color_texture;
  
  pass = CRUDE_REINTERPRET_CAST( crude_gfx_luminance_average_pass*, ctx );
  gpu = pass->scene_renderer->renderer->gpu;

  luminance_histogram_generation_pipeline = crude_gfx_renderer_access_technique_pass_by_name( pass->scene_renderer->renderer, "postprocessing", "luminance_histogram_generation" )->pipeline;
  
  hdr_color_texture_handle = crude_gfx_render_graph_builder_access_resource_by_name( pass->scene_renderer->render_graph->builder, "pbr" )->resource_info.texture.handle;
  hdr_color_texture = crude_gfx_access_texture( gpu, hdr_color_texture_handle );

  crude_gfx_cmd_bind_pipeline( primary_cmd, luminance_histogram_generation_pipeline );
  crude_gfx_cmd_bind_descriptor_set( primary_cmd, pass->luminance_histogram_generation_ds[ gpu->current_frame ] );
  
  float32 min_log_lum = -8.0f;
  float32 max_log_lum = 3.5f;
  constant.hdr_color_texture_index = hdr_color_texture_handle.index;
  constant.inverse_log_lum_range = 1.f / ( max_log_lum - min_log_lum );
  constant.min_log_lum = min_log_lum;
  crude_gfx_cmd_push_constant( primary_cmd, &constant, sizeof( constant ) );
  
  crude_gfx_cmd_fill_buffer( primary_cmd, pass->luminance_histogram_sb_handle[ gpu->current_frame ], 0u );
  crude_gfx_cmd_dispatch( primary_cmd, ( hdr_color_texture->width + 15 ) / 16, ( hdr_color_texture->height + 15 ) /  16, 1 );
}

void
crude_gfx_luminance_average_pass_on_resize
(
  _In_ void                                               *ctx,
  _In_ uint32                                              new_width,
  _In_ uint32                                              new_height
)
{
  crude_gfx_luminance_average_pass                        *pass;
  crude_gfx_texture_creation                               luminance_average_texture_creation;
  
  pass = CRUDE_REINTERPRET_CAST( crude_gfx_luminance_average_pass*, ctx );
  
  for ( uint32 i = 0; i < CRUDE_GFX_MAX_SWAPCHAIN_IMAGES; ++i )
  {
    if ( CRUDE_RESOURCE_HANDLE_IS_VALID( pass->luminance_average_texture_handle[ i ] ) )
    {
      crude_gfx_destroy_texture( pass->scene_renderer->renderer->gpu, pass->luminance_average_texture_handle[ i ] );
    }
  }
  
  luminance_average_texture_creation = crude_gfx_texture_creation_empty();
  luminance_average_texture_creation.format = VK_FORMAT_R32_SFLOAT;
  luminance_average_texture_creation.type = CRUDE_GFX_TEXTURE_TYPE_TEXTURE_2D;
  luminance_average_texture_creation.width = 1u;
  luminance_average_texture_creation.height = 1u;
  luminance_average_texture_creation.depth = 1u;
  luminance_average_texture_creation.flags = CRUDE_GFX_TEXTURE_MASK_COMPUTE;
  luminance_average_texture_creation.name = "luminance_average";

  for ( uint32 i = 0; i < CRUDE_GFX_MAX_SWAPCHAIN_IMAGES; ++i )
  {
    pass->luminance_average_texture_handle[ i ] = crude_gfx_create_texture( pass->scene_renderer->renderer->gpu, &luminance_average_texture_creation );
  } 
  
  crude_gfx_luminance_average_pass_on_techniques_reloaded( ctx );
}

void
crude_gfx_luminance_average_pass_on_techniques_reloaded
(
  _In_ void                                               *ctx
)
{
  crude_gfx_luminance_average_pass                        *pass;
  crude_gfx_device                                        *gpu;
  crude_gfx_descriptor_set_layout_handle                   luminance_histogram_generation_dsl;
  crude_gfx_pipeline_handle                                luminance_histogram_generation_pipeline;
  
  pass = CRUDE_REINTERPRET_CAST( crude_gfx_luminance_average_pass*, ctx );
  gpu = pass->scene_renderer->renderer->gpu;

  luminance_histogram_generation_pipeline = crude_gfx_renderer_access_technique_pass_by_name( pass->scene_renderer->renderer, "postprocessing", "luminance_histogram_generation" )->pipeline;
  luminance_histogram_generation_dsl = crude_gfx_get_descriptor_set_layout( gpu, luminance_histogram_generation_pipeline, CRUDE_GFX_MATERIAL_DESCRIPTOR_SET_INDEX );
  
  for ( uint32 i = 0; i < CRUDE_GFX_MAX_SWAPCHAIN_IMAGES; ++i )
  {
    if ( CRUDE_RESOURCE_HANDLE_IS_VALID( pass->luminance_histogram_generation_ds[ i ] ) )
    {
      crude_gfx_destroy_descriptor_set( pass->scene_renderer->renderer->gpu, pass->luminance_histogram_generation_ds[ i ] );
    }
  }

  for ( uint32 i = 0; i < CRUDE_GFX_MAX_SWAPCHAIN_IMAGES; ++i )
  {
    crude_gfx_descriptor_set_creation                      ds_creation;

    ds_creation = crude_gfx_descriptor_set_creation_empty();
    ds_creation.name = "luminance_histogram_generation_ds";
    ds_creation.layout = luminance_histogram_generation_dsl;
    
    crude_gfx_descriptor_set_creation_add_buffer( &ds_creation, pass->luminance_histogram_sb_handle[ i ], 0u );
    
    pass->luminance_histogram_generation_ds[ i ] = crude_gfx_create_descriptor_set( gpu, &ds_creation );
  }
}

crude_gfx_render_graph_pass_container
crude_gfx_luminance_average_pass_pack
(
  _In_ crude_gfx_luminance_average_pass                   *pass
)
{
  crude_gfx_render_graph_pass_container container = crude_gfx_render_graph_pass_container_empty();
  container.ctx = pass;
  container.render = crude_gfx_luminance_average_pass_render;
  container.on_techniques_reloaded = crude_gfx_luminance_average_pass_on_techniques_reloaded;
  container.on_resize = crude_gfx_luminance_average_pass_on_resize;
  return container;
}