
#include <engine/core/hash_map.h>
#include <engine/graphics/scene_renderer.h>

#include <engine/graphics/passes/depth_pyramid_pass.h>

void
crude_gfx_depth_pyramid_pass_initialize
(
  _In_ crude_gfx_depth_pyramid_pass                       *pass,
  _In_ crude_gfx_scene_renderer                           *scene_renderer
)
{
  crude_gfx_sampler_creation                               sampler_creation;
  pass->scene_renderer = scene_renderer;

  for ( uint32 i = 0; i < CRUDE_GFX_DEPTH_PYRAMID_PASS_MAX_LEVELS; ++i )
  {
    pass->depth_pyramid_views_handles[ i ] = CRUDE_GFX_TEXTURE_HANDLE_INVALID;
  }
  pass->depth_pyramid_texture_handle = CRUDE_GFX_TEXTURE_HANDLE_INVALID;

  sampler_creation = crude_gfx_sampler_creation_empty();
  sampler_creation.address_mode_u = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
  sampler_creation.address_mode_v = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
  sampler_creation.address_mode_w = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
  sampler_creation.mag_filter = VK_FILTER_LINEAR;
  sampler_creation.min_filter = VK_FILTER_LINEAR;
  sampler_creation.mip_filter = VK_SAMPLER_MIPMAP_MODE_NEAREST;
  sampler_creation.reduction_mode = VK_SAMPLER_REDUCTION_MODE_MAX;
  sampler_creation.name = "depth_pyramid_sampler";

  pass->depth_pyramid_sampler = crude_gfx_create_sampler( pass->scene_renderer->gpu, &sampler_creation );

  crude_gfx_depth_pyramid_pass_on_resize( pass, pass->scene_renderer->gpu->renderer_size.x, pass->scene_renderer->gpu->renderer_size.y );
}

void
crude_gfx_depth_pyramid_pass_deinitialize
(
  _In_ crude_gfx_depth_pyramid_pass                       *pass
)
{
  crude_gfx_device *gpu = pass->scene_renderer->gpu;
  crude_gfx_destroy_texture( gpu, pass->depth_pyramid_texture_handle );
  crude_gfx_destroy_sampler( gpu, pass->depth_pyramid_sampler );

  for ( uint32 i = 0; i < CRUDE_GFX_DEPTH_PYRAMID_PASS_MAX_LEVELS; ++i )
  {
    if ( CRUDE_RESOURCE_HANDLE_IS_VALID( pass->depth_pyramid_views_handles[ i ] ) )
    {
      crude_gfx_destroy_texture( gpu, pass->depth_pyramid_views_handles[ i ] );
    }
  }
}

void
crude_gfx_depth_pyramid_pass_render
(
  _In_ void                                               *ctx,
  _In_ crude_gfx_cmd_buffer                               *primary_cmd
)
{
  CRUDE_ALIGNED_STRUCT( 16 ) push_constant_
  {
    uint32                                                 src_image_index;
    uint32                                                 dst_image_index;
  };
  
  crude_gfx_depth_pyramid_pass                            *pass;
  crude_gfx_device                                        *gpu;
  crude_gfx_texture                                       *depth_pyramid_texture;
  uint64                                                   depth_texture_handle_index;
  crude_gfx_pipeline_handle                                depth_pyramid_pipeline;
  push_constant_                                           push_constant;
  uint32                                                   width, height, group_x, group_y;

  pass = CRUDE_REINTERPRET_CAST( crude_gfx_depth_pyramid_pass*, ctx );

  depth_pyramid_pipeline = crude_gfx_access_technique_pass_by_name( pass->scene_renderer->gpu, "compute", "depth_pyramid" )->pipeline;

  crude_gfx_cmd_bind_pipeline( primary_cmd, depth_pyramid_pipeline );
  
  gpu = pass->scene_renderer->gpu;
  depth_pyramid_texture = crude_gfx_access_texture( gpu, pass->depth_pyramid_texture_handle );
  
  width = depth_pyramid_texture->width;
  height = depth_pyramid_texture->height;
  
  crude_gfx_cmd_add_image_barrier( primary_cmd, depth_pyramid_texture, CRUDE_GFX_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, 0u, depth_pyramid_texture->subresource.mip_level_count, false );
  
  crude_gfx_cmd_bind_bindless_descriptor_set( primary_cmd );

  for ( uint32 mip_index = 0; mip_index < depth_pyramid_texture->subresource.mip_level_count; ++mip_index )
  {
    crude_gfx_cmd_add_image_barrier_ext2( primary_cmd, depth_pyramid_texture->vk_image, CRUDE_GFX_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, CRUDE_GFX_RESOURCE_STATE_UNORDERED_ACCESS, mip_index, 1u, false );
    
    if ( mip_index == 0 )
    {
      push_constant.src_image_index = CRUDE_GFX_PASS_TEXTURE_INDEX( depth_pyramid_pass.depth );
      push_constant.dst_image_index = pass->depth_pyramid_views_handles[ mip_index ].index;
    }
    else
    {
      push_constant.src_image_index = pass->depth_pyramid_views_handles[ mip_index - 1 ].index;
      push_constant.dst_image_index = pass->depth_pyramid_views_handles[ mip_index ].index;
    }
    crude_gfx_cmd_push_constant( primary_cmd, &push_constant, sizeof( push_constant ) );
    
    group_x = ( width + 7 ) / 8;
    group_y = ( height + 7 ) / 8;
    crude_gfx_cmd_dispatch( primary_cmd, group_x, group_y, 1 );
    
    crude_gfx_cmd_add_image_barrier_ext2( primary_cmd, depth_pyramid_texture->vk_image, CRUDE_GFX_RESOURCE_STATE_UNORDERED_ACCESS, CRUDE_GFX_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, mip_index, 1u, false );
    
    width = width / 2;
    height = height / 2;
  }

  depth_pyramid_texture->state = CRUDE_GFX_RESOURCE_STATE_PIXEL_SHADER_RESOURCE; 
}

void
crude_gfx_depth_pyramid_pass_on_resize
(
  _In_ void                                               *ctx,
  _In_ uint32                                              new_width,
  _In_ uint32                                              new_height
)
{
  crude_gfx_depth_pyramid_pass                            *pass;
  crude_gfx_sampler_creation                               sampler_creation;
  crude_gfx_texture_creation                               depth_hierarchy_creation;
  crude_gfx_texture_view_creation                          depth_pyramid_view_creation;
  uint32                                                   depth_hierarchy_width, depth_hierarchy_height, width, height;

  pass = CRUDE_REINTERPRET_CAST( crude_gfx_depth_pyramid_pass*, ctx );
  
  depth_hierarchy_width = width = pass->scene_renderer->gpu->renderer_size.x / 2;
  depth_hierarchy_height = height = pass->scene_renderer->gpu->renderer_size.y / 2;
  
  pass->depth_pyramid_levels = 0;
  while ( width >= 1 && height >= 1 )
  {
    pass->depth_pyramid_levels++;
    
    width = width / 2;
    height = height / 2;
  }

  if ( CRUDE_RESOURCE_HANDLE_IS_VALID( pass->depth_pyramid_texture_handle ) )
  {
    crude_gfx_destroy_texture( pass->scene_renderer->gpu, pass->depth_pyramid_texture_handle );
  }
  
  depth_hierarchy_creation = crude_gfx_texture_creation_empty();
  depth_hierarchy_creation.format = VK_FORMAT_R32_SFLOAT;
  depth_hierarchy_creation.type = CRUDE_GFX_TEXTURE_TYPE_TEXTURE_2D;
  depth_hierarchy_creation.width = depth_hierarchy_width;
  depth_hierarchy_creation.height = depth_hierarchy_height;
  depth_hierarchy_creation.depth = 1u;
  depth_hierarchy_creation.subresource.mip_level_count = pass->depth_pyramid_levels;
  depth_hierarchy_creation.flags = CRUDE_GFX_TEXTURE_MASK_COMPUTE;
  crude_string_copy( depth_hierarchy_creation.name, "depth_hierarchy", sizeof( depth_hierarchy_creation.name ) );
  pass->depth_pyramid_texture_handle = crude_gfx_create_texture( pass->scene_renderer->gpu, &depth_hierarchy_creation );

  crude_gfx_link_texture_sampler( pass->scene_renderer->gpu, pass->depth_pyramid_texture_handle, pass->depth_pyramid_sampler );

  depth_pyramid_view_creation = crude_gfx_texture_view_creation_empty( );
  depth_pyramid_view_creation.view_type = VK_IMAGE_VIEW_TYPE_2D;
  depth_pyramid_view_creation.parent_texture_handle = pass->depth_pyramid_texture_handle;
  depth_pyramid_view_creation.name = "depth_pyramid_view";
  for ( uint32 i = 0; i < pass->depth_pyramid_levels; ++i )
  {
    if ( CRUDE_RESOURCE_HANDLE_IS_VALID( pass->depth_pyramid_views_handles[ i ] ) )
    {
      crude_gfx_destroy_texture( pass->scene_renderer->gpu, pass->depth_pyramid_views_handles[ i ] );
    }
  
    depth_pyramid_view_creation.subresource.mip_base_level = i;
    pass->depth_pyramid_views_handles[ i ] = crude_gfx_create_texture_view( pass->scene_renderer->gpu, &depth_pyramid_view_creation );
  }
}

void
crude_gfx_depth_pyramid_pass_register
(
  _In_ crude_gfx_depth_pyramid_pass                       *pass
)
{
}

crude_gfx_render_graph_pass_container
crude_gfx_depth_pyramid_pass_pack
(
  _In_ crude_gfx_depth_pyramid_pass                       *pass
)
{
  crude_gfx_render_graph_pass_container container = crude_gfx_render_graph_pass_container_empty();
  container.ctx = pass;
  container.render = crude_gfx_depth_pyramid_pass_render;
  container.on_resize = crude_gfx_depth_pyramid_pass_on_resize;
  return container;
}