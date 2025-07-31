
#include <core/hash_map.h>
#include <graphics/scene_renderer.h>

#include <graphics/passes/depth_pyramid_pass.h>

void
crude_gfx_depth_pyramid_pass_initialize
(
  _In_ crude_gfx_depth_pyramid_pass                       *pass,
  _In_ crude_gfx_scene_renderer                           *scene_renderer,
  _In_ char const                                         *depth_resource_name
)
{
  pass->scene_renderer = scene_renderer;
  pass->depth_resource_name = depth_resource_name;
}

void
crude_gfx_depth_pyramid_pass_deinitialize
(
  _In_ crude_gfx_depth_pyramid_pass                       *pass
)
{
  crude_gfx_device *gpu = pass->scene_renderer->renderer->gpu;
  crude_gfx_destroy_texture( gpu, pass->depth_pyramid_texture_handle );
  crude_gfx_destroy_sampler( gpu, pass->depth_pyramid_sampler );

  for ( uint32 i = 0; i < pass->depth_pyramid_levels; ++i )
  {
    crude_gfx_destroy_texture( gpu, pass->depth_pyramid_views_handles[ i ] );
    crude_gfx_destroy_descriptor_set( gpu, pass->depth_hierarchy_descriptor_sets_handles[ i ] );
  }
}

void
crude_gfx_depth_pyramid_pass_on_render_graph_registered
(
  _In_ crude_gfx_depth_pyramid_pass                       *pass
)
{
  crude_gfx_device *gpu = pass->scene_renderer->renderer->gpu;

  crude_gfx_render_graph_resource *depth_resource = crude_gfx_render_graph_builder_access_resource_by_name( pass->scene_renderer->render_graph->builder, pass->depth_resource_name );
  crude_gfx_texture_handle depth_texture_handle = depth_resource->resource_info.texture.handle;
  crude_gfx_texture *depth_texture = crude_gfx_access_texture( gpu, depth_texture_handle );

  uint32 width = depth_texture->width / 2;
  uint32 height = depth_texture->height / 2;
  
  pass->depth_pyramid_levels = 0;
  while ( width >= 2 && height >= 2 )
  {
    pass->depth_pyramid_levels++;
    
    width /= 2;
    height /= 2;
  }
  
  crude_gfx_texture_creation depth_hierarchy_creation = crude_gfx_texture_creation_empty();
  depth_hierarchy_creation.format = VK_FORMAT_R32_SFLOAT;
  depth_hierarchy_creation.type = CRUDE_GFX_TEXTURE_TYPE_TEXTURE_2D;
  depth_hierarchy_creation.width = depth_texture->width / 2;
  depth_hierarchy_creation.height = depth_texture->height / 2;
  depth_hierarchy_creation.depth = 1u;
  depth_hierarchy_creation.subresource.mip_level_count = pass->depth_pyramid_levels;
  depth_hierarchy_creation.flags = CRUDE_GFX_TEXTURE_MASK_COMPUTE;
  depth_hierarchy_creation.name = "depth_hierarchy";
  
  pass->depth_pyramid_texture_handle = crude_gfx_create_texture( gpu, &depth_hierarchy_creation );
  
  crude_gfx_sampler_creation sampler_creation = crude_gfx_sampler_creation_empty();
  sampler_creation.address_mode_u = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
  sampler_creation.address_mode_v = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
  sampler_creation.address_mode_w = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
  sampler_creation.mag_filter = VK_FILTER_LINEAR;
  sampler_creation.min_filter = VK_FILTER_LINEAR;
  sampler_creation.mip_filter = VK_SAMPLER_MIPMAP_MODE_NEAREST;
  sampler_creation.reduction_mode = VK_SAMPLER_REDUCTION_MODE_MAX;
  sampler_creation.name = "depth_pyramid_sampler";

  pass->depth_pyramid_sampler = crude_gfx_create_sampler( gpu, &sampler_creation );

  crude_gfx_link_texture_sampler( gpu, pass->depth_pyramid_texture_handle, pass->depth_pyramid_sampler );

  crude_gfx_texture_view_creation depth_pyramid_view_creation = crude_gfx_texture_view_creation_empty( );
  depth_pyramid_view_creation.view_type = VK_IMAGE_VIEW_TYPE_2D;
  depth_pyramid_view_creation.parent_texture_handle = pass->depth_pyramid_texture_handle;
  depth_pyramid_view_creation.name = "depth_pyramid_view";

  crude_gfx_pipeline_handle depth_pyramid_pipeline = crude_gfx_renderer_access_technique_pass_by_name( pass->scene_renderer->renderer, "culling", "depth_pyramid" )->pipeline;
  pass->depth_pyramid_layout_handle = crude_gfx_get_descriptor_set_layout( gpu, depth_pyramid_pipeline, CRUDE_GFX_MATERIAL_DESCRIPTOR_SET_INDEX );

  for ( uint32 i = 0; i < pass->depth_pyramid_levels; ++i )
  {
    crude_gfx_descriptor_set_creation                      ds_creation;

    depth_pyramid_view_creation.subresource.mip_base_level = i;

    pass->depth_pyramid_views_handles[ i ] = crude_gfx_create_texture_view( gpu, &depth_pyramid_view_creation );

    ds_creation = crude_gfx_descriptor_set_creation_empty();
    ds_creation.name = "depth_hierarchy_descriptor_set";
    ds_creation.layout = pass->depth_pyramid_layout_handle;

    if ( i == 0 )
    {
      crude_gfx_descriptor_set_creation_add_texture( &ds_creation, depth_texture_handle, 0u );
      crude_gfx_descriptor_set_creation_add_texture( &ds_creation, pass->depth_pyramid_views_handles[ i ], 1u );
    }
    else
    {
      crude_gfx_descriptor_set_creation_add_texture( &ds_creation, pass->depth_pyramid_views_handles[ i - 1 ], 0u );
      crude_gfx_descriptor_set_creation_add_texture( &ds_creation, pass->depth_pyramid_views_handles[ i ], 1u );
    }
    
    pass->depth_hierarchy_descriptor_sets_handles[ i ] = crude_gfx_create_descriptor_set( gpu, &ds_creation );
  }
}

void
crude_gfx_depth_pyramid_pass_render
(
  _In_ void                                               *ctx,
  _In_ crude_gfx_cmd_buffer                               *primary_cmd
)
{
  crude_gfx_depth_pyramid_pass                            *pass;
  crude_gfx_pipeline_handle                                depth_pyramid_pipeline;
  
  pass = CRUDE_REINTERPRET_CAST( crude_gfx_depth_pyramid_pass*, ctx );

  depth_pyramid_pipeline = crude_gfx_renderer_access_technique_pass_by_name( pass->scene_renderer->renderer, "culling", "depth_pyramid" )->pipeline;

  crude_gfx_cmd_bind_pipeline( primary_cmd, depth_pyramid_pipeline );
  
  crude_gfx_device *gpu = pass->scene_renderer->renderer->gpu;
  crude_gfx_texture *depth_pyramid_texture = crude_gfx_access_texture( gpu, pass->depth_pyramid_texture_handle );
  
  uint32 width = depth_pyramid_texture->width;
  uint32 height = depth_pyramid_texture->height;
  
  crude_gfx_cmd_add_image_barrier( primary_cmd, depth_pyramid_texture, CRUDE_GFX_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, 0u, depth_pyramid_texture->subresource.mip_level_count, false );

  for ( uint32 mip_index = 0; mip_index < depth_pyramid_texture->subresource.mip_level_count; ++mip_index )
  {
    crude_gfx_cmd_add_image_barrier_ext2( primary_cmd, depth_pyramid_texture->vk_image, CRUDE_GFX_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, CRUDE_GFX_RESOURCE_STATE_UNORDERED_ACCESS, mip_index, 1u, false );
    
    crude_gfx_cmd_bind_descriptor_set( primary_cmd, pass->depth_hierarchy_descriptor_sets_handles[ mip_index ] );
    
    uint32 group_x = ( width + 7 ) / 8;
    uint32 group_y = ( height + 7 ) / 8;
    
    crude_gfx_cmd_dispatch( primary_cmd, group_x, group_y, 1 );
    
    crude_gfx_cmd_add_image_barrier_ext2( primary_cmd, depth_pyramid_texture->vk_image, CRUDE_GFX_RESOURCE_STATE_UNORDERED_ACCESS, CRUDE_GFX_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, mip_index, 1u, false );
    
    width /= 2;
    height /= 2;
  }

  depth_pyramid_texture->state = CRUDE_GFX_RESOURCE_STATE_PIXEL_SHADER_RESOURCE; 
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
  return container;
}