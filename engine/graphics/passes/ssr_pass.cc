#include <engine/core/profiler.h>
#include <engine/core/hash_map.h>
#include <engine/scene/scene_ecs.h>
#include <engine/graphics/scene_renderer.h>

#include <engine/graphics/passes/ssr_pass.h>

void
crude_gfx_ssr_pass_initialize
(
  _In_ crude_gfx_ssr_pass                                 *pass,
  _In_ crude_gfx_scene_renderer                           *scene_renderer
)
{
  crude_gfx_sampler_creation                               sampler_creation;

  pass->scene_renderer = scene_renderer;

  for ( uint32 i = 0; i < CRUDE_GRAPHICS_SSR_RADIANCE_TEXTURES_MAX_LEVELS; ++i )
  {
    pass->radiance_hierarchy_views_handles[ i ] = CRUDE_GFX_TEXTURE_HANDLE_INVALID;
  }
  pass->radiance_hierarchy_texture_handle = CRUDE_GFX_TEXTURE_HANDLE_INVALID;

  sampler_creation = crude_gfx_sampler_creation_empty();
  sampler_creation.address_mode_u = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
  sampler_creation.address_mode_v = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
  sampler_creation.address_mode_w = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
  sampler_creation.mag_filter = VK_FILTER_LINEAR;
  sampler_creation.min_filter = VK_FILTER_LINEAR;
  sampler_creation.mip_filter = VK_SAMPLER_MIPMAP_MODE_LINEAR;
  sampler_creation.name = "radiance_hierarchy_sampler";

  pass->radiance_hierarchy_sampler = crude_gfx_create_sampler( pass->scene_renderer->gpu, &sampler_creation );

  crude_gfx_ssr_pass_on_resize( pass, pass->scene_renderer->gpu->renderer_size.x, pass->scene_renderer->gpu->renderer_size.y );
}

void
crude_gfx_ssr_pass_deinitialize
(
  _In_ crude_gfx_ssr_pass                                 *pass
)
{
  crude_gfx_device *gpu = pass->scene_renderer->gpu;
  crude_gfx_destroy_texture( gpu, pass->radiance_hierarchy_texture_handle );
  crude_gfx_destroy_sampler( gpu, pass->radiance_hierarchy_sampler );

  for ( uint32 i = 0; i < CRUDE_GRAPHICS_SSR_RADIANCE_TEXTURES_MAX_LEVELS; ++i )
  {
    if ( CRUDE_RESOURCE_HANDLE_IS_VALID( pass->radiance_hierarchy_views_handles[ i ] ) )
    {
      crude_gfx_destroy_texture( gpu, pass->radiance_hierarchy_views_handles[ i ] );
    }
  }
}

void
crude_gfx_ssr_pass_render
(
  _In_ void                                               *ctx,
  _In_ crude_gfx_cmd_buffer                               *primary_cmd
)
{
  crude_gfx_ssr_pass                                      *pass;
  crude_gfx_device                                        *gpu;
  crude_gfx_texture                                       *depth_texture;
  crude_gfx_texture                                       *direct_radiance_texture;

  pass = CRUDE_REINTERPRET_CAST( crude_gfx_ssr_pass*, ctx );
  gpu = pass->scene_renderer->gpu;
  
  depth_texture = crude_gfx_access_texture( gpu, CRUDE_GFX_PASS_TEXTURE_HANDLE( ssr_pass.depth_texture ) );
  direct_radiance_texture = crude_gfx_access_texture( gpu, CRUDE_GFX_PASS_TEXTURE_HANDLE( ssr_pass.direct_radiance_texture ) );

  /* SSR Hit Calculation Pass  */
  {
    CRUDE_ALIGNED_STRUCT( 16 ) push_constant_
    {
      VkDeviceAddress                                      scene;
      float32                                              ssr_max_steps;
      float32                                              ssr_max_distance;

      float32                                              ssr_stride;
      float32                                              ssr_z_thickness;
      float32                                              ssr_stride_zcutoff;
      uint32                                               depth_texture_index;

      XMFLOAT2                                             depth_texture_size;
      uint32                                               normal_texture_index;
      uint32                                               ssr_texture_index;
    };

    crude_gfx_pipeline_handle                              pipeline;
    push_constant_                                         pust_constant;

    crude_gfx_cmd_push_marker( primary_cmd, "ssr_hit_calculation" );

    pipeline = crude_gfx_access_technique_pass_by_name( gpu, "compute", "ssr_hit_calculation" )->pipeline;
    crude_gfx_cmd_bind_pipeline( primary_cmd, pipeline );

    pust_constant = CRUDE_COMPOUNT_EMPTY( push_constant_ );
    pust_constant.scene = pass->scene_renderer->scene_hga.gpu_address;
    pust_constant.ssr_max_steps = pass->scene_renderer->options.ssr_pass.max_steps;
    pust_constant.ssr_max_distance = pass->scene_renderer->options.ssr_pass.max_distance;
    pust_constant.ssr_stride_zcutoff = pass->scene_renderer->options.ssr_pass.stride_zcutoff;
    pust_constant.ssr_stride = pass->scene_renderer->options.ssr_pass.stride;
    pust_constant.ssr_z_thickness = pass->scene_renderer->options.ssr_pass.z_thickness;
    pust_constant.depth_texture_index = depth_texture->handle.index;
    pust_constant.depth_texture_size.x = depth_texture->width;
    pust_constant.depth_texture_size.y = depth_texture->height;
    pust_constant.normal_texture_index = CRUDE_GFX_PASS_TEXTURE_INDEX( ssr_pass.normal_texture );
    pust_constant.ssr_texture_index = CRUDE_GFX_PASS_TEXTURE_INDEX( ssr_pass.ssr_texture );

    crude_gfx_cmd_push_constant( primary_cmd, &pust_constant, sizeof( pust_constant ) );

    crude_gfx_cmd_bind_bindless_descriptor_set( primary_cmd );

    crude_gfx_cmd_dispatch( primary_cmd, ( depth_texture->width + 7 ) / 8, ( depth_texture->height + 7 ) / 8, 1u );

    crude_gfx_cmd_pop_marker( primary_cmd );
  }

  /* SSR Blurring Pass  */
  {
    CRUDE_ALIGNED_STRUCT( 16 ) push_constant_
    {
      uint32                                               src_texture_index;
      uint32                                               dst_texture_index;
      XMFLOAT2                                             src_div_dst_texture_size;
    };

    crude_gfx_texture                                     *radiance_hierarchy_texture;
    crude_gfx_pipeline_handle                              pipeline;
    push_constant_                                         pust_constant;
    uint64                                                 mip_width, mip_height;
    
    radiance_hierarchy_texture = crude_gfx_access_texture( gpu, pass->radiance_hierarchy_texture_handle );

    crude_gfx_cmd_push_marker( primary_cmd, "ssr_convolve_vertical" );

    pipeline = crude_gfx_access_technique_pass_by_name( gpu, "compute", "ssr_convolve_vertical" )->pipeline;
    crude_gfx_cmd_bind_pipeline( primary_cmd, pipeline );
    
    crude_gfx_cmd_add_image_barrier( primary_cmd, direct_radiance_texture, CRUDE_GFX_RESOURCE_STATE_COPY_SOURCE, 0u, 1, false );
    crude_gfx_cmd_add_image_barrier( primary_cmd, radiance_hierarchy_texture, CRUDE_GFX_RESOURCE_STATE_COPY_DEST, 0u, radiance_hierarchy_texture->subresource.mip_level_count, false );
    
    crude_gfx_cmd_copy_texture( primary_cmd, direct_radiance_texture->handle, radiance_hierarchy_texture->handle );

    crude_gfx_cmd_add_image_barrier( primary_cmd, radiance_hierarchy_texture, CRUDE_GFX_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, 0u, radiance_hierarchy_texture->subresource.mip_level_count, false );
    
    crude_gfx_cmd_bind_bindless_descriptor_set( primary_cmd );
    
    mip_width = radiance_hierarchy_texture->width;
    mip_height = radiance_hierarchy_texture->height;

    for ( uint32 mip_index = 1; mip_index < radiance_hierarchy_texture->subresource.mip_level_count; ++mip_index )
    {
      uint64                                               prev_mip_width, prev_mip_height;
    
      prev_mip_width = mip_width;
      prev_mip_height = mip_height;
    
      mip_width = mip_width / 2;
      mip_height = mip_height / 2;
    
      crude_gfx_cmd_add_image_barrier_ext2( primary_cmd, radiance_hierarchy_texture->vk_image, CRUDE_GFX_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, CRUDE_GFX_RESOURCE_STATE_UNORDERED_ACCESS, mip_index, 1u, false );
      
      pust_constant = CRUDE_COMPOUNT_EMPTY( push_constant_ );
      pust_constant.dst_texture_index = pass->radiance_hierarchy_views_handles[ mip_index ].index;
      pust_constant.src_texture_index = pass->radiance_hierarchy_views_handles[ mip_index - 1 ].index;
      pust_constant.src_div_dst_texture_size.x = prev_mip_width / CRUDE_CAST( float32, mip_width );
      pust_constant.src_div_dst_texture_size.y = prev_mip_height / CRUDE_CAST( float32, mip_height );
      crude_gfx_cmd_push_constant( primary_cmd, &pust_constant, sizeof( push_constant_ ) );
      
      crude_gfx_cmd_dispatch( primary_cmd, ( mip_width + 7 ) / 8, ( mip_height + 7 ) / 8, 1 );
      
      crude_gfx_cmd_add_image_barrier_ext2( primary_cmd, radiance_hierarchy_texture->vk_image, CRUDE_GFX_RESOURCE_STATE_UNORDERED_ACCESS, CRUDE_GFX_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, mip_index, 1u, false );
    }

    radiance_hierarchy_texture->state = CRUDE_GFX_RESOURCE_STATE_PIXEL_SHADER_RESOURCE; 

    crude_gfx_cmd_pop_marker( primary_cmd  );
  }
}

void
crude_gfx_ssr_pass_on_resize
(
  _In_ void                                               *ctx,
  _In_ uint32                                              new_width,
  _In_ uint32                                              new_height
)
{
  crude_gfx_ssr_pass                                      *pass;
  crude_gfx_sampler_creation                               sampler_creation;
  crude_gfx_texture_creation                               radiance_hierarchy_creation;
  crude_gfx_texture_view_creation                          radiance_hierarchy_view_creation;
  uint32                                                   radiance_hierarchy_width, radiance_hierarchy_height, width, height;

  pass = CRUDE_REINTERPRET_CAST( crude_gfx_ssr_pass*, ctx );
  
  radiance_hierarchy_width = width = pass->scene_renderer->gpu->renderer_size.x;
  radiance_hierarchy_height = height = pass->scene_renderer->gpu->renderer_size.y;
  
  pass->radiance_hierarchy_levels = 0;
  while ( width >= 1 && height >= 1 )
  {
    pass->radiance_hierarchy_levels++;
    
    width = width / 2;
    height = height / 2;
  }

  if ( CRUDE_RESOURCE_HANDLE_IS_VALID( pass->radiance_hierarchy_texture_handle ) )
  {
    crude_gfx_destroy_texture( pass->scene_renderer->gpu, pass->radiance_hierarchy_texture_handle );
  }
  
  radiance_hierarchy_creation = crude_gfx_texture_creation_empty();
  radiance_hierarchy_creation.format = VK_FORMAT_R16G16B16A16_UNORM;
  radiance_hierarchy_creation.type = CRUDE_GFX_TEXTURE_TYPE_TEXTURE_2D;
  radiance_hierarchy_creation.width = radiance_hierarchy_width;
  radiance_hierarchy_creation.height = radiance_hierarchy_height;
  radiance_hierarchy_creation.depth = 1u;
  radiance_hierarchy_creation.subresource.mip_level_count = pass->radiance_hierarchy_levels;
  radiance_hierarchy_creation.flags = CRUDE_GFX_TEXTURE_MASK_COMPUTE;
  radiance_hierarchy_creation.name = "radiance_hierarchy_texture_handle";
  pass->radiance_hierarchy_texture_handle = crude_gfx_create_texture( pass->scene_renderer->gpu, &radiance_hierarchy_creation );

  crude_gfx_link_texture_sampler( pass->scene_renderer->gpu, pass->radiance_hierarchy_texture_handle, pass->radiance_hierarchy_sampler );

  radiance_hierarchy_view_creation = crude_gfx_texture_view_creation_empty( );
  radiance_hierarchy_view_creation.view_type = VK_IMAGE_VIEW_TYPE_2D;
  radiance_hierarchy_view_creation.parent_texture_handle = pass->radiance_hierarchy_texture_handle;
  radiance_hierarchy_view_creation.name = "radiance_hierarchy_view";
  for ( uint32 i = 0; i < pass->radiance_hierarchy_levels; ++i )
  {
    if ( CRUDE_RESOURCE_HANDLE_IS_VALID( pass->radiance_hierarchy_views_handles[ i ] ) )
    {
      crude_gfx_destroy_texture( pass->scene_renderer->gpu, pass->radiance_hierarchy_views_handles[ i ] );
    }
  
    radiance_hierarchy_view_creation.subresource.mip_base_level = i;
    pass->radiance_hierarchy_views_handles[ i ] = crude_gfx_create_texture_view( pass->scene_renderer->gpu, &radiance_hierarchy_view_creation );
  }
}

crude_gfx_render_graph_pass_container
crude_gfx_ssr_pass_pack
(
  _In_ crude_gfx_ssr_pass                                 *pass
)
{
  crude_gfx_render_graph_pass_container container = crude_gfx_render_graph_pass_container_empty();
  container.ctx = pass;
  container.on_resize = crude_gfx_ssr_pass_on_resize;
  container.render = crude_gfx_ssr_pass_render;
  return container;
}