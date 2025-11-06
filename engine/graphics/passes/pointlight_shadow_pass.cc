#include <graphics/scene_renderer.h>
#include <graphics/scene_renderer_resources.h>

#include <graphics/passes/pointlight_shadow_pass.h>

void
crude_gfx_pointlight_shadow_pass_initialize
(
  _In_ crude_gfx_pointlight_shadow_pass                   *pass,
  _In_ crude_gfx_scene_renderer                           *scene_renderer
)
{
  crude_gfx_buffer_creation                                buffer_creation;
  crude_gfx_texture_creation                               texture_creation;
  crude_gfx_framebuffer_creation                           framebuffer_creation;
  crude_gfx_render_pass_creation                           render_pass_creation;
  crude_gfx_sampler_creation                               sampler_creation;

  pass->scene_renderer = scene_renderer;
    
  for ( uint32 i = 0; i < CRUDE_GRAPHICS_MAX_SWAPCHAIN_IMAGES; ++i )
  {
    buffer_creation = CRUDE_COMPOUNT_EMPTY( crude_gfx_buffer_creation );
    buffer_creation.type_flags = VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
    buffer_creation.usage = CRUDE_GFX_RESOURCE_USAGE_TYPE_DYNAMIC;
    buffer_creation.size = sizeof( XMUINT4 ) * CRUDE_GRAPHICS_SCENE_RENDERER_LIGHTS_MAX_COUNT;
    buffer_creation.name = "pointlight_spheres_sb";
    pass->pointlight_spheres_sb[ i ] = crude_gfx_create_buffer( scene_renderer->gpu, &buffer_creation );

    buffer_creation = CRUDE_COMPOUNT_EMPTY( crude_gfx_buffer_creation );
    buffer_creation.type_flags = VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
    buffer_creation.usage = CRUDE_GFX_RESOURCE_USAGE_TYPE_IMMUTABLE;
    buffer_creation.size = sizeof( XMUINT4 ) * ( CRUDE_GRAPHICS_SCENE_RENDERER_LIGHTS_MAX_COUNT * 4u );
    buffer_creation.device_only = true;
    buffer_creation.name = "pointshadow_meshlet_draw_commands_sb";
    pass->pointshadow_meshlet_draw_commands_sb[ i ] = crude_gfx_create_buffer( scene_renderer->gpu, &buffer_creation );
  
    buffer_creation = CRUDE_COMPOUNT_EMPTY( crude_gfx_buffer_creation );
    buffer_creation.type_flags = VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
    buffer_creation.usage = CRUDE_GFX_RESOURCE_USAGE_TYPE_IMMUTABLE;
    buffer_creation.size = sizeof( XMUINT2 ) * CRUDE_GRAPHICS_SCENE_RENDERER_MAX_MESHLETS_PER_LIGHT * CRUDE_GRAPHICS_SCENE_RENDERER_LIGHTS_MAX_COUNT;
    buffer_creation.device_only = true;
    buffer_creation.name = "meshletes_instances_sb";
    pass->meshletes_instances_sb[ i ] = crude_gfx_create_buffer( scene_renderer->gpu, &buffer_creation );

    buffer_creation = CRUDE_COMPOUNT_EMPTY( crude_gfx_buffer_creation );
    buffer_creation.type_flags = VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
    buffer_creation.usage = CRUDE_GFX_RESOURCE_USAGE_TYPE_IMMUTABLE;
    buffer_creation.size = sizeof( uint32 ) * ( CRUDE_GRAPHICS_SCENE_RENDERER_LIGHTS_MAX_COUNT + 1u );
    buffer_creation.device_only = true;
    buffer_creation.name = "pointshadow_meshletes_instances_count_sb";
    pass->pointshadow_meshletes_instances_count_sb[ i ] = crude_gfx_create_buffer( scene_renderer->gpu, &buffer_creation );
  }

  texture_creation = crude_gfx_texture_creation_empty( );
  texture_creation.width = CRUDE_GRAPHICS_TETRAHEDRON_SHADOWMAP_SIZE;
  texture_creation.height = CRUDE_GRAPHICS_TETRAHEDRON_SHADOWMAP_SIZE;
  texture_creation.depth = 1u;
  texture_creation.format = VK_FORMAT_D16_UNORM;
  texture_creation.type = CRUDE_GFX_TEXTURE_TYPE_TEXTURE_2D;
  texture_creation.flags = CRUDE_GFX_TEXTURE_MASK_RENDER_TARGET;
  texture_creation.name = "tetrahedron_shadow_texture";
  pass->tetrahedron_shadow_texture = crude_gfx_create_texture( scene_renderer->gpu, &texture_creation );

  framebuffer_creation = crude_gfx_framebuffer_creation_empty( );
  framebuffer_creation.depth_stencil_texture = pass->tetrahedron_shadow_texture;
  framebuffer_creation.name = "tetrahedron_framebuffer";
  framebuffer_creation.width = CRUDE_GRAPHICS_TETRAHEDRON_SHADOWMAP_SIZE;
  framebuffer_creation.height = CRUDE_GRAPHICS_TETRAHEDRON_SHADOWMAP_SIZE;
  framebuffer_creation.manual_resources_free = true;
  pass->tetrahedron_framebuffer_handle = crude_gfx_create_framebuffer( scene_renderer->gpu, &framebuffer_creation );

  render_pass_creation = crude_gfx_render_pass_creation_empty( );
  render_pass_creation.name = "tetrahedron_render_pass";
  render_pass_creation.depth_stencil_final_layout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL;
  render_pass_creation.depth_stencil_format = VK_FORMAT_D16_UNORM;
  render_pass_creation.depth_operation = CRUDE_GFX_RENDER_PASS_OPERATION_CLEAR;
  pass->tetrahedron_render_pass_handle = crude_gfx_create_render_pass( scene_renderer->gpu, &render_pass_creation );

  sampler_creation = crude_gfx_sampler_creation_empty();
  sampler_creation.address_mode_u = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
  sampler_creation.address_mode_v = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
  sampler_creation.address_mode_w = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
  sampler_creation.mag_filter = VK_FILTER_LINEAR;
  sampler_creation.min_filter = VK_FILTER_LINEAR;
  sampler_creation.mip_filter = VK_SAMPLER_MIPMAP_MODE_NEAREST;
  sampler_creation.reduction_mode = VK_SAMPLER_REDUCTION_MODE_MAX;
  sampler_creation.name = "tetrahedron_shadow_sampler";

  pass->tetrahedron_shadow_sampler = crude_gfx_create_sampler( scene_renderer->gpu, &sampler_creation );
  
  crude_gfx_link_texture_sampler( scene_renderer->gpu, pass->tetrahedron_shadow_texture, pass->tetrahedron_shadow_sampler );
  
  for ( uint32 i = 0; i < CRUDE_GRAPHICS_MAX_SWAPCHAIN_IMAGES; ++i )
  {
    pass->pointshadow_culling_ds[ i ] = CRUDE_GFX_DESCRIPTOR_SET_HANDLE_INVALID;
    pass->pointshadow_commands_generation_ds[ i ] = CRUDE_GFX_DESCRIPTOR_SET_HANDLE_INVALID;
    pass->pointshadow_ds[ i ] = CRUDE_GFX_DESCRIPTOR_SET_HANDLE_INVALID;
  }
  crude_gfx_pointlight_shadow_pass_on_techniques_reloaded( pass );
}

void
crude_gfx_pointlight_shadow_pass_deinitialize
(
  _In_ crude_gfx_pointlight_shadow_pass                   *pass
)
{
  crude_gfx_device                                        *gpu;

  gpu = pass->scene_renderer->gpu;

  for ( uint32 i = 0; i < CRUDE_GRAPHICS_MAX_SWAPCHAIN_IMAGES; ++i )
  {
    crude_gfx_destroy_descriptor_set( gpu, pass->pointshadow_culling_ds[ i ] );
    crude_gfx_destroy_descriptor_set( gpu, pass->pointshadow_commands_generation_ds[ i ] );
    crude_gfx_destroy_descriptor_set( gpu, pass->pointshadow_ds[ i ] );
    crude_gfx_destroy_buffer( gpu, pass->pointlight_spheres_sb[ i ] );
    crude_gfx_destroy_buffer( gpu, pass->pointshadow_meshlet_draw_commands_sb[ i ] );
    crude_gfx_destroy_buffer( gpu, pass->meshletes_instances_sb[ i ] );
    crude_gfx_destroy_buffer( gpu, pass->pointshadow_meshletes_instances_count_sb[ i ] );
  }

  crude_gfx_destroy_sampler( gpu, pass->tetrahedron_shadow_sampler );
  crude_gfx_destroy_texture( gpu, pass->tetrahedron_shadow_texture );
  crude_gfx_destroy_framebuffer( gpu, pass->tetrahedron_framebuffer_handle );
  crude_gfx_destroy_render_pass( gpu, pass->tetrahedron_render_pass_handle );
}

void
crude_gfx_pointlight_shadow_pass_render
(
  _In_ void                                               *ctx,
  _In_ crude_gfx_cmd_buffer                               *primary_cmd
)
{
  crude_gfx_device                                        *gpu;
  crude_gfx_pointlight_shadow_pass                        *pass;
  crude_gfx_pipeline_handle                                pointshadow_culling_pipeline;
  crude_gfx_pipeline_handle                                pointshadow_commands_generation_pipeline;
  crude_gfx_pipeline_handle                                pointshadow_pipeline;
  crude_gfx_texture                                       *tetrahedron_shadow_texture;
  uint32                                                   width, height;

  pass = CRUDE_REINTERPRET_CAST( crude_gfx_pointlight_shadow_pass*, ctx );
  gpu = pass->scene_renderer->gpu;

  if ( CRUDE_ARRAY_LENGTH( pass->scene_renderer->lights ) == 0 )
  {
    return;
  }

  pointshadow_culling_pipeline = crude_gfx_access_technique_pass_by_name( pass->scene_renderer->gpu, "pointshadow_meshlet", "pointshadow_culling" )->pipeline;
  pointshadow_commands_generation_pipeline = crude_gfx_access_technique_pass_by_name( pass->scene_renderer->gpu, "pointshadow_meshlet", "pointshadow_commands_generation" )->pipeline;
  pointshadow_pipeline = crude_gfx_access_technique_pass_by_name( pass->scene_renderer->gpu, "pointshadow_meshlet", "pointshadow" )->pipeline;
  
  /* Pointshadow Culling */
  {
    crude_gfx_cmd_push_marker( primary_cmd, "pointshadow_culling" );
    crude_gfx_cmd_add_buffer_barrier( primary_cmd, pass->meshletes_instances_sb[ gpu->current_frame ], CRUDE_GFX_RESOURCE_STATE_SHADER_RESOURCE, CRUDE_GFX_RESOURCE_STATE_UNORDERED_ACCESS );
    crude_gfx_cmd_add_buffer_barrier( primary_cmd, pass->pointshadow_meshletes_instances_count_sb[ gpu->current_frame ], CRUDE_GFX_RESOURCE_STATE_INDIRECT_ARGUMENT, CRUDE_GFX_RESOURCE_STATE_UNORDERED_ACCESS );
  
    crude_gfx_cmd_bind_pipeline( primary_cmd, pointshadow_culling_pipeline );
    crude_gfx_cmd_bind_descriptor_set( primary_cmd, pass->pointshadow_culling_ds[ gpu->current_frame ] );
  
    crude_gfx_cmd_fill_buffer( primary_cmd, pass->pointshadow_meshletes_instances_count_sb[ gpu->current_frame ], 0u );

    crude_gfx_cmd_dispatch( primary_cmd, ( pass->scene_renderer->total_meshes_instances_count * CRUDE_ARRAY_LENGTH( pass->scene_renderer->lights ) + 31 ) / 32, 1u, 1u );
  
    crude_gfx_cmd_add_buffer_barrier( primary_cmd, pass->pointshadow_meshletes_instances_count_sb[ gpu->current_frame ], CRUDE_GFX_RESOURCE_STATE_UNORDERED_ACCESS, CRUDE_GFX_RESOURCE_STATE_INDIRECT_ARGUMENT );
    crude_gfx_cmd_add_buffer_barrier( primary_cmd, pass->meshletes_instances_sb[ gpu->current_frame ], CRUDE_GFX_RESOURCE_STATE_UNORDERED_ACCESS, CRUDE_GFX_RESOURCE_STATE_SHADER_RESOURCE );
    crude_gfx_cmd_pop_marker( primary_cmd );
  }
  
  /* Pointshadow Commands Generation */
  {
    crude_gfx_cmd_push_marker( primary_cmd, "pointshadow_commands_generation" );
    crude_gfx_cmd_add_buffer_barrier( primary_cmd, pass->pointshadow_meshlet_draw_commands_sb[ gpu->current_frame ], CRUDE_GFX_RESOURCE_STATE_INDIRECT_ARGUMENT, CRUDE_GFX_RESOURCE_STATE_UNORDERED_ACCESS );
    
    crude_gfx_cmd_bind_pipeline( primary_cmd, pointshadow_commands_generation_pipeline );
    crude_gfx_cmd_bind_descriptor_set( primary_cmd, pass->pointshadow_commands_generation_ds[ gpu->current_frame ] );
    crude_gfx_cmd_dispatch( primary_cmd, ( CRUDE_ARRAY_LENGTH( pass->scene_renderer->lights ) + 31 ) / 32, 1u, 1u );
  
    crude_gfx_cmd_add_buffer_barrier( primary_cmd, pass->pointshadow_meshlet_draw_commands_sb[ gpu->current_frame ], CRUDE_GFX_RESOURCE_STATE_UNORDERED_ACCESS, CRUDE_GFX_RESOURCE_STATE_INDIRECT_ARGUMENT );
    crude_gfx_cmd_pop_marker( primary_cmd );
  }

  tetrahedron_shadow_texture = crude_gfx_access_texture( gpu, pass->tetrahedron_shadow_texture );
  
  width = tetrahedron_shadow_texture->width;
  height = tetrahedron_shadow_texture->height;
  
  /* Pointshadow Rendering */
  {
    crude_gfx_rect2d_int                                   scissor;
    crude_gfx_viewport                                     dev_viewport;
    
    crude_gfx_cmd_set_clear_depth_and_stencil( primary_cmd, 1.f, 0.f );

    scissor = CRUDE_COMPOUNT_EMPTY( crude_gfx_rect2d_int );
    scissor.x = 0u;
    scissor.y = 0u;
    scissor.width = width;
    scissor.height = height;
    crude_gfx_cmd_set_scissor( primary_cmd, &scissor );

    dev_viewport = CRUDE_COMPOUNT_EMPTY( crude_gfx_viewport );
    dev_viewport.rect.x = 0u;
    dev_viewport.rect.y = 0u;
    dev_viewport.rect.width = width;
    dev_viewport.rect.height = height;
    dev_viewport.min_depth = 0.0f;
    dev_viewport.max_depth = 1.0f;
    crude_gfx_cmd_set_viewport( primary_cmd, &dev_viewport );
  }
  
  /* GPU Pro 6 Tile-Based Omnidirectional Shadows Hawar Doghramachi, mixed with http://www.hd-prg.com/tileBasedShadows.html */
  {
    XMFLOAT4                                              *pointlight_spheres_mapped;
    XMFLOAT4X4                                            *pointlight_world_to_clip_mapped;
    crude_gfx_map_buffer_parameters                        cb_map;
    float32                                                fov0, fov1; 
    
    fov0 = 143.98570868f + 1.99273682f;
    fov1 = 125.26438968f + 2.78596497f;

    cb_map = CRUDE_COMPOUNT_EMPTY( crude_gfx_map_buffer_parameters );
    cb_map.buffer = pass->scene_renderer->pointlight_world_to_clip_sb[ gpu->current_frame ];
    pointlight_world_to_clip_mapped = CRUDE_CAST( XMFLOAT4X4*, crude_gfx_map_buffer( gpu, &cb_map ) );

    cb_map = CRUDE_COMPOUNT_EMPTY( crude_gfx_map_buffer_parameters );
    cb_map.buffer = pass->pointlight_spheres_sb[ gpu->current_frame ];
    pointlight_spheres_mapped = CRUDE_CAST( XMFLOAT4*, crude_gfx_map_buffer( gpu, &cb_map ) );

    if ( pointlight_world_to_clip_mapped && pointlight_spheres_mapped )
    {
      XMMATRIX                                               clip_to_face_clip[ 4 ];
      XMMATRIX                                               view_to_faceed_view[ 4 ];
      XMMATRIX                                               x_rot_matrix, y_rot_matrix, z_rot_matrix;
      float32                                                tile_size, tile_position_x, tile_position_y;
      
      x_rot_matrix = XMMatrixRotationX( XMConvertToRadians( -27.36780516f ) );
      y_rot_matrix = XMMatrixRotationY( XMConvertToRadians( 0.f ) );
      z_rot_matrix = XMMatrixRotationZ( XMConvertToRadians( 0.f ) );
      view_to_faceed_view[ 0 ] = XMMatrixMultiply( XMMatrixMultiply( y_rot_matrix, x_rot_matrix ), z_rot_matrix );
      x_rot_matrix = XMMatrixRotationX( XMConvertToRadians( -27.36780516f ) );
      y_rot_matrix = XMMatrixRotationY( XMConvertToRadians( 180.f ) );
      z_rot_matrix = XMMatrixRotationZ( XMConvertToRadians( 90.f ) );
      view_to_faceed_view[ 1 ] = XMMatrixMultiply( XMMatrixMultiply( y_rot_matrix, x_rot_matrix ), z_rot_matrix );
      y_rot_matrix = XMMatrixRotationY( XMConvertToRadians( 90.0f ) );
      x_rot_matrix = XMMatrixRotationX( XMConvertToRadians( 27.36780516f ) );
      view_to_faceed_view[ 2 ] = XMMatrixMultiply( y_rot_matrix, x_rot_matrix );
      x_rot_matrix = XMMatrixRotationX( XMConvertToRadians( 27.36780516f ) );
      y_rot_matrix = XMMatrixRotationY( XMConvertToRadians( 270.f ) );
      z_rot_matrix = XMMatrixRotationZ( XMConvertToRadians( 90.f ) );
      view_to_faceed_view[ 3 ] = XMMatrixMultiply( XMMatrixMultiply( y_rot_matrix, x_rot_matrix ), z_rot_matrix );

      tile_size = 1.f;
      tile_position_x = 0.f;
      tile_position_y = 0.f;

      clip_to_face_clip[ 0 ] = XMMatrixSet(
        tile_size, 0.f , 0.f, 0.f, 0.f, tile_size * 0.5f, 0.f, 0.f,
        0.f, 0.f, 1.f, 0.f, tile_position_x, tile_position_y - ( tile_size * 0.5f ), 0.f, 1.f
      ) ;
      clip_to_face_clip[ 1 ] = XMMatrixSet(
        tile_size * 0.5f, 0.f, 0.f, 0.f, 0.f, tile_size, 0.f, 0.f,
        0.f, 0.f, 1.f, 0.f, tile_position_x + ( tile_size * 0.5f ), tile_position_y, 0.f, 1.f
      );
      clip_to_face_clip[ 2 ] = XMMatrixSet(
        tile_size, 0.f, 0.f, 0.f, 0.f, tile_size * 0.5f, 0.f, 0.f,
        0.f, 0.f, 1.f, 0.f, tile_position_x, tile_position_y + ( tile_size * 0.5f ), 0.f, 1.f
      );
      clip_to_face_clip[ 3 ] = XMMatrixSet(
        tile_size * 0.5f, 0.f, 0.f, 0.f, 0.f, tile_size, 0.f, 0.f,
        0.f, 0.f, 1.f, 0.f, tile_position_x - ( tile_size * 0.5f ), tile_position_y, 0.f, 1.f
      );

      for ( uint32 i = 0; i < CRUDE_ARRAY_LENGTH( pass->scene_renderer->lights ); ++i )
      {
        crude_gfx_light_cpu const                         *light_cpu;
        crude_light const                                 *light;
        crude_transform const                             *light_transform;
        XMMATRIX                                           view_to_clip[ 2 ];
        XMMATRIX                                           world_to_view, world_to_faced_view, world_to_clip;

        light_cpu = &pass->scene_renderer->lights[ i ];
        light = CRUDE_ENTITY_GET_IMMUTABLE_COMPONENT( light_cpu->node, crude_light );
        light_transform = CRUDE_ENTITY_GET_IMMUTABLE_COMPONENT( light_cpu->node, crude_transform );

        XMStoreFloat4( &pointlight_spheres_mapped[ i ], XMVectorSet( light_transform->translation.x, light_transform->translation.y, light_transform->translation.z, light->radius ) );
   
        view_to_clip[ 0 ] = XMMatrixPerspectiveFovLH( XMConvertToRadians( fov1 ), tanf( XMConvertToRadians( fov0 ) * 0.5f ) / tanf( XMConvertToRadians( fov1 ) * 0.5f ), 0.2f, light->radius );
        view_to_clip[ 1 ] = XMMatrixPerspectiveFovLH( XMConvertToRadians( fov0 ), tanf( XMConvertToRadians( fov1 ) * 0.5f ) / tanf( XMConvertToRadians( fov0 ) * 0.5f ), 0.2f, light->radius );

        world_to_view = XMMatrixTranslation( -1.f * light_transform->translation.x, -1.f * light_transform->translation.y, -1.f * light_transform->translation.z );

        world_to_faced_view = XMMatrixMultiply( world_to_view, view_to_faceed_view[ 0 ] );
        world_to_clip = XMMatrixMultiply( world_to_faced_view, view_to_clip[ 0 ] );
        XMStoreFloat4x4( &pointlight_world_to_clip_mapped[ i * 4 + 0 ], XMMatrixMultiply( world_to_clip, clip_to_face_clip[ 0 ] ) );

        world_to_faced_view = XMMatrixMultiply( world_to_view, view_to_faceed_view[ 1 ] );
        world_to_clip = XMMatrixMultiply( world_to_faced_view, view_to_clip[ 1 ] );
        XMStoreFloat4x4( &pointlight_world_to_clip_mapped[ i * 4 + 1], XMMatrixMultiply( world_to_clip, clip_to_face_clip[ 1 ] ) );
        
        world_to_faced_view = XMMatrixMultiply( world_to_view, view_to_faceed_view[ 2 ] );
        world_to_clip = XMMatrixMultiply( world_to_faced_view, view_to_clip[ 0 ] );
        XMStoreFloat4x4( &pointlight_world_to_clip_mapped[ i * 4 + 2 ], XMMatrixMultiply( world_to_clip, clip_to_face_clip[ 2 ] ) );
 
        world_to_faced_view = XMMatrixMultiply( world_to_view, view_to_faceed_view[ 3 ] );
        world_to_clip = XMMatrixMultiply( world_to_faced_view, view_to_clip[ 1 ] );
        XMStoreFloat4x4( &pointlight_world_to_clip_mapped[ i * 4 + 3 ], XMMatrixMultiply( world_to_clip, clip_to_face_clip[ 3 ] ) );
      }
      crude_gfx_unmap_buffer( gpu, pass->scene_renderer->pointlight_world_to_clip_sb[ gpu->current_frame ] );
      crude_gfx_unmap_buffer( gpu, pass->pointlight_spheres_sb[ gpu->current_frame ] );
    }
    
    crude_gfx_cmd_push_marker( primary_cmd, "pointshadow_draw_pass" );
    crude_gfx_cmd_add_image_barrier( primary_cmd, crude_gfx_access_texture( gpu, pass->tetrahedron_shadow_texture ), CRUDE_GFX_RESOURCE_STATE_DEPTH_WRITE, 0u, 1u, true );

    crude_gfx_cmd_bind_render_pass( primary_cmd, pass->tetrahedron_render_pass_handle, pass->tetrahedron_framebuffer_handle, false );
    crude_gfx_cmd_bind_pipeline( primary_cmd, pointshadow_pipeline );

    crude_gfx_cmd_bind_descriptor_set( primary_cmd, pass->pointshadow_ds[ gpu->current_frame ] );
  
    crude_gfx_cmd_draw_mesh_task_indirect_count( primary_cmd,
      pass->pointshadow_meshlet_draw_commands_sb[ gpu->current_frame ], 0u,
      pass->pointshadow_meshletes_instances_count_sb[ gpu->current_frame ], sizeof( uint32 ) * CRUDE_GRAPHICS_SCENE_RENDERER_LIGHTS_MAX_COUNT,
      4u * CRUDE_GRAPHICS_SCENE_RENDERER_LIGHTS_MAX_COUNT, sizeof( XMUINT4 ) );

    crude_gfx_cmd_end_render_pass( primary_cmd );

    crude_gfx_cmd_add_image_barrier( primary_cmd, crude_gfx_access_texture( gpu, pass->tetrahedron_shadow_texture ), CRUDE_GFX_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, 0u, 1u, true );
    crude_gfx_cmd_pop_marker( primary_cmd );
  }
}

void
crude_gfx_pointlight_shadow_pass_on_techniques_reloaded
(
  _In_ void                                               *ctx
)
{
  crude_gfx_pointlight_shadow_pass                        *pass;
  crude_gfx_pipeline_handle                                pointshadow_culling_pipeline;
  crude_gfx_descriptor_set_layout_handle                   pointshadow_culling_dsl;
  crude_gfx_pipeline_handle                                pointshadow_commands_generation_pipeline;
  crude_gfx_descriptor_set_layout_handle                   pointshadow_commands_generation_dsl;
  crude_gfx_pipeline_handle                                pointshadow_pipeline;
  crude_gfx_descriptor_set_layout_handle                   pointshadow_dsl;

  pass = CRUDE_REINTERPRET_CAST( crude_gfx_pointlight_shadow_pass*, ctx );

  pointshadow_culling_pipeline = crude_gfx_access_technique_pass_by_name( pass->scene_renderer->gpu, "pointshadow_meshlet", "pointshadow_culling" )->pipeline;
  pointshadow_culling_dsl = crude_gfx_get_descriptor_set_layout( pass->scene_renderer->gpu, pointshadow_culling_pipeline, CRUDE_GRAPHICS_MATERIAL_DESCRIPTOR_SET_INDEX );
  
  for ( uint32 i = 0; i < CRUDE_GRAPHICS_MAX_SWAPCHAIN_IMAGES; ++i )
  {
    if ( CRUDE_RESOURCE_HANDLE_IS_VALID( pass->pointshadow_culling_ds[ i ] ) )
    {
      crude_gfx_destroy_descriptor_set( pass->scene_renderer->gpu, pass->pointshadow_culling_ds[ i ] );
    }
    if ( CRUDE_RESOURCE_HANDLE_IS_VALID( pass->pointshadow_commands_generation_ds[ i ] ) )
    {
      crude_gfx_destroy_descriptor_set( pass->scene_renderer->gpu, pass->pointshadow_commands_generation_ds[ i ] );
    }
    if ( CRUDE_RESOURCE_HANDLE_IS_VALID( pass->pointshadow_ds[ i ] ) )
    {
      crude_gfx_destroy_descriptor_set( pass->scene_renderer->gpu, pass->pointshadow_ds[ i ] );
    }
  }

  for ( uint32 i = 0; i < CRUDE_GRAPHICS_MAX_SWAPCHAIN_IMAGES; ++i )
  {
    crude_gfx_descriptor_set_creation                    ds_creation;
    
    ds_creation = crude_gfx_descriptor_set_creation_empty();
    ds_creation.layout = pointshadow_culling_dsl;
    ds_creation.name = "pointlight_shadow_culling_ds";
  
    crude_gfx_descriptor_set_creation_add_buffer( &ds_creation, pass->scene_renderer->scene_cb, 0u );
    crude_gfx_descriptor_set_creation_add_buffer( &ds_creation, pass->scene_renderer->model_renderer_resources_manager->meshes_draws_sb, 1u );
    crude_gfx_descriptor_set_creation_add_buffer( &ds_creation, pass->scene_renderer->meshes_instances_draws_sb, 2u );
    crude_gfx_descriptor_set_creation_add_buffer( &ds_creation, pass->scene_renderer->model_renderer_resources_manager->meshes_bounds_sb, 3u );
    crude_gfx_descriptor_set_creation_add_buffer( &ds_creation, pass->scene_renderer->model_renderer_resources_manager->meshlets_sb, 4u );
    crude_gfx_descriptor_set_creation_add_buffer( &ds_creation, pass->meshletes_instances_sb[ i ], 5u );
    crude_gfx_descriptor_set_creation_add_buffer( &ds_creation, pass->pointshadow_meshletes_instances_count_sb[ i ], 6u );
    crude_gfx_descriptor_set_creation_add_buffer( &ds_creation, pass->scene_renderer->lights_sb, 7u );
    
    pass->pointshadow_culling_ds[ i ] = crude_gfx_create_descriptor_set( pass->scene_renderer->gpu, &ds_creation );
  }
  
  pointshadow_commands_generation_pipeline = crude_gfx_access_technique_pass_by_name( pass->scene_renderer->gpu, "pointshadow_meshlet", "pointshadow_commands_generation" )->pipeline;
  pointshadow_commands_generation_dsl = crude_gfx_get_descriptor_set_layout( pass->scene_renderer->gpu, pointshadow_commands_generation_pipeline, CRUDE_GRAPHICS_MATERIAL_DESCRIPTOR_SET_INDEX );

  for ( uint32 i = 0; i < CRUDE_GRAPHICS_MAX_SWAPCHAIN_IMAGES; ++i )
  {
    crude_gfx_descriptor_set_creation                    ds_creation;
    
    ds_creation = crude_gfx_descriptor_set_creation_empty();
    ds_creation.layout = pointshadow_commands_generation_dsl;
    ds_creation.name = "pointshadow_commands_generation_dsl";
  
    crude_gfx_descriptor_set_creation_add_buffer( &ds_creation, pass->scene_renderer->scene_cb, 0u );
    crude_gfx_descriptor_set_creation_add_buffer( &ds_creation, pass->pointshadow_meshletes_instances_count_sb[ i ], 1u );
    crude_gfx_descriptor_set_creation_add_buffer( &ds_creation, pass->pointshadow_meshlet_draw_commands_sb[ i ], 2u );
    
    pass->pointshadow_commands_generation_ds[ i ] = crude_gfx_create_descriptor_set( pass->scene_renderer->gpu, &ds_creation );
  }
  
  pointshadow_pipeline = crude_gfx_access_technique_pass_by_name( pass->scene_renderer->gpu, "pointshadow_meshlet", "pointshadow" )->pipeline;
  pointshadow_dsl = crude_gfx_get_descriptor_set_layout( pass->scene_renderer->gpu, pointshadow_pipeline, CRUDE_GRAPHICS_MATERIAL_DESCRIPTOR_SET_INDEX );

  for ( uint32 i = 0; i < CRUDE_GRAPHICS_MAX_SWAPCHAIN_IMAGES; ++i )
  {
    crude_gfx_descriptor_set_creation                    ds_creation;
    
    ds_creation = crude_gfx_descriptor_set_creation_empty();
    ds_creation.layout = pointshadow_dsl;
    ds_creation.name = "pointshadow_ds";

    crude_gfx_descriptor_set_creation_add_buffer( &ds_creation, pass->scene_renderer->meshes_instances_draws_sb, 1u );
    crude_gfx_descriptor_set_creation_add_buffer( &ds_creation, pass->scene_renderer->model_renderer_resources_manager->meshlets_sb, 2u );
    crude_gfx_descriptor_set_creation_add_buffer( &ds_creation, pass->scene_renderer->model_renderer_resources_manager->meshes_bounds_sb, 3u );
    crude_gfx_descriptor_set_creation_add_buffer( &ds_creation, pass->scene_renderer->model_renderer_resources_manager->meshlets_vertices_sb, 4u );
    crude_gfx_descriptor_set_creation_add_buffer( &ds_creation, pass->scene_renderer->model_renderer_resources_manager->meshlets_triangles_indices_sb, 5u );
    crude_gfx_descriptor_set_creation_add_buffer( &ds_creation, pass->scene_renderer->model_renderer_resources_manager->meshlets_vertices_indices_sb, 6u );
    crude_gfx_descriptor_set_creation_add_buffer( &ds_creation, pass->pointlight_spheres_sb[ i ], 7u );
    crude_gfx_descriptor_set_creation_add_buffer( &ds_creation, pass->pointshadow_meshlet_draw_commands_sb[ i ], 8u );
    crude_gfx_descriptor_set_creation_add_buffer( &ds_creation, pass->meshletes_instances_sb[ i ], 9u );
    crude_gfx_descriptor_set_creation_add_buffer( &ds_creation, pass->pointshadow_meshletes_instances_count_sb[ i ], 10u );
    crude_gfx_descriptor_set_creation_add_buffer( &ds_creation, pass->scene_renderer->pointlight_world_to_clip_sb[ i ], 11u );
    
    pass->pointshadow_ds[ i ] = crude_gfx_create_descriptor_set( pass->scene_renderer->gpu, &ds_creation );
  }
}

void
crude_gfx_pointlight_shadow_pass_on_resize
(
  _In_ void                                               *ctx,
  _In_ uint32                                              new_width,
  _In_ uint32                                              new_height
)
{
  crude_gfx_pointlight_shadow_pass_on_techniques_reloaded( ctx );
}

crude_gfx_render_graph_pass_container
crude_gfx_pointlight_shadow_pass_pack
(
  _In_ crude_gfx_pointlight_shadow_pass                   *pass
)
{
  crude_gfx_render_graph_pass_container container = crude_gfx_render_graph_pass_container_empty();
  container.ctx = pass;
  container.render = crude_gfx_pointlight_shadow_pass_render;
  container.on_techniques_reloaded = crude_gfx_pointlight_shadow_pass_on_techniques_reloaded;
  container.on_resize = crude_gfx_pointlight_shadow_pass_on_resize;
  return container;
}