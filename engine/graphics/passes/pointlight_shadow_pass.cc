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
  crude_gfx_pipeline_handle                                pointshadow_culling_pipeline;
  crude_gfx_descriptor_set_layout_handle                   pointshadow_culling_dsl;
  crude_gfx_pipeline_handle                                pointshadow_commands_generation_pipeline;
  crude_gfx_descriptor_set_layout_handle                   pointshadow_commands_generation_dsl;
  crude_gfx_pipeline_handle                                pointshadow_pipeline;
  crude_gfx_descriptor_set_layout_handle                   pointshadow_dsl;
  crude_gfx_buffer_creation                                buffer_creation;
  crude_gfx_texture_creation                               texture_creation;
  crude_gfx_framebuffer_creation                           framebuffer_creation;
  crude_gfx_render_pass_creation                           render_pass_creation;

  pass->scene_renderer = scene_renderer;
    
  for ( uint32 i = 0; i < CRUDE_GFX_MAX_SWAPCHAIN_IMAGES; ++i )
  {
    buffer_creation = CRUDE_COMPOUNT_EMPTY( crude_gfx_buffer_creation );
    buffer_creation.type_flags = VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
    buffer_creation.usage = CRUDE_GFX_RESOURCE_USAGE_TYPE_DYNAMIC;
    buffer_creation.size = sizeof( XMFLOAT4X4 ) * CRUDE_GFX_LIGHTS_MAX_COUNT;
    buffer_creation.name = "pointlight_world_to_clip_sb";
    pass->pointlight_world_to_clip_sb[ i ] = crude_gfx_create_buffer( scene_renderer->renderer->gpu, &buffer_creation );
  
    buffer_creation = CRUDE_COMPOUNT_EMPTY( crude_gfx_buffer_creation );
    buffer_creation.type_flags = VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
    buffer_creation.usage = CRUDE_GFX_RESOURCE_USAGE_TYPE_DYNAMIC;
    buffer_creation.size = sizeof( XMUINT4 ) * CRUDE_GFX_LIGHTS_MAX_COUNT;
    buffer_creation.name = "pointlight_spheres_sb";
    pass->pointlight_spheres_sb[ i ] = crude_gfx_create_buffer( scene_renderer->renderer->gpu, &buffer_creation );

    buffer_creation = CRUDE_COMPOUNT_EMPTY( crude_gfx_buffer_creation );
    buffer_creation.type_flags = VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
    buffer_creation.usage = CRUDE_GFX_RESOURCE_USAGE_TYPE_IMMUTABLE;
    buffer_creation.size = sizeof( XMUINT4 ) * ( CRUDE_GFX_LIGHTS_MAX_COUNT * 4u );
    buffer_creation.name = "pointshadow_meshlet_draw_commands_sb";
    pass->pointshadow_meshlet_draw_commands_sb[ i ] = crude_gfx_create_buffer( scene_renderer->renderer->gpu, &buffer_creation );
  
    buffer_creation = CRUDE_COMPOUNT_EMPTY( crude_gfx_buffer_creation );
    buffer_creation.type_flags = VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
    buffer_creation.usage = CRUDE_GFX_RESOURCE_USAGE_TYPE_IMMUTABLE;
    buffer_creation.size = sizeof( XMUINT2 ) * CRUDE_GFX_MAX_MESHLETS_PER_LIGHT * CRUDE_GFX_LIGHTS_MAX_COUNT;
    buffer_creation.name = "meshletes_instances_sb";
    pass->meshletes_instances_sb[ i ] = crude_gfx_create_buffer( scene_renderer->renderer->gpu, &buffer_creation );

    buffer_creation = CRUDE_COMPOUNT_EMPTY( crude_gfx_buffer_creation );
    buffer_creation.type_flags = VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
    buffer_creation.usage = CRUDE_GFX_RESOURCE_USAGE_TYPE_IMMUTABLE;
    buffer_creation.size = sizeof( uint32 ) * ( CRUDE_GFX_LIGHTS_MAX_COUNT + 1u );
    buffer_creation.name = "pointshadow_meshletes_instances_count_sb";
    pass->pointshadow_meshletes_instances_count_sb[ i ] = crude_gfx_create_buffer( scene_renderer->renderer->gpu, &buffer_creation );
  }

  pointshadow_culling_pipeline = crude_gfx_renderer_access_technique_pass_by_name( pass->scene_renderer->renderer, "meshlet", "pointshadow_culling" )->pipeline;
  pointshadow_culling_dsl = crude_gfx_get_descriptor_set_layout( scene_renderer->renderer->gpu, pointshadow_culling_pipeline, CRUDE_GFX_MATERIAL_DESCRIPTOR_SET_INDEX );
  
  for ( uint32 i = 0; i < CRUDE_GFX_MAX_SWAPCHAIN_IMAGES; ++i )
  {
    crude_gfx_descriptor_set_creation                    ds_creation;
    
    ds_creation = crude_gfx_descriptor_set_creation_empty();
    ds_creation.layout = pointshadow_culling_dsl;
    ds_creation.name = "pointlight_shadow_culling_ds";
  
    crude_gfx_scene_renderer_add_scene_resources_to_descriptor_set_creation( &ds_creation, pass->scene_renderer, i );
    crude_gfx_scene_renderer_add_mesh_resources_to_descriptor_set_creation( &ds_creation, pass->scene_renderer, i );
    crude_gfx_scene_renderer_add_meshlet_resources_to_descriptor_set_creation( &ds_creation, pass->scene_renderer, i );
    crude_gfx_scene_renderer_add_debug_resources_to_descriptor_set_creation( &ds_creation, pass->scene_renderer, i );
    crude_gfx_scene_renderer_add_light_resources_to_descriptor_set_creation( &ds_creation, pass->scene_renderer, i );
    crude_gfx_descriptor_set_creation_add_buffer( &ds_creation, pass->meshletes_instances_sb[ i ], 10u );
    crude_gfx_descriptor_set_creation_add_buffer( &ds_creation, pass->pointshadow_meshletes_instances_count_sb[ i ], 11u );
    
    pass->pointshadow_culling_ds[ i ] = crude_gfx_create_descriptor_set( scene_renderer->renderer->gpu, &ds_creation );
  }
  
  pointshadow_commands_generation_pipeline = crude_gfx_renderer_access_technique_pass_by_name( pass->scene_renderer->renderer, "meshlet", "pointshadow_commands_generation" )->pipeline;
  pointshadow_commands_generation_dsl = crude_gfx_get_descriptor_set_layout( scene_renderer->renderer->gpu, pointshadow_commands_generation_pipeline, CRUDE_GFX_MATERIAL_DESCRIPTOR_SET_INDEX );

  for ( uint32 i = 0; i < CRUDE_GFX_MAX_SWAPCHAIN_IMAGES; ++i )
  {
    crude_gfx_descriptor_set_creation                    ds_creation;
    
    ds_creation = crude_gfx_descriptor_set_creation_empty();
    ds_creation.layout = pointshadow_commands_generation_dsl;
    ds_creation.name = "pointlight_shadow_culling_ds";
  
    crude_gfx_scene_renderer_add_scene_resources_to_descriptor_set_creation( &ds_creation, pass->scene_renderer, i );
    crude_gfx_scene_renderer_add_mesh_resources_to_descriptor_set_creation( &ds_creation, pass->scene_renderer, i );
    crude_gfx_scene_renderer_add_light_resources_to_descriptor_set_creation( &ds_creation, pass->scene_renderer, i );
    crude_gfx_descriptor_set_creation_add_buffer( &ds_creation, pass->pointshadow_meshletes_instances_count_sb[ i ], 10u );
    crude_gfx_descriptor_set_creation_add_buffer( &ds_creation, pass->pointshadow_meshlet_draw_commands_sb[ i ], 11u );
    
    pass->pointshadow_commands_generation_ds[ i ] = crude_gfx_create_descriptor_set( scene_renderer->renderer->gpu, &ds_creation );
  }
  
  pointshadow_pipeline = crude_gfx_renderer_access_technique_pass_by_name( pass->scene_renderer->renderer, "meshlet", "pointshadow" )->pipeline;
  pointshadow_dsl = crude_gfx_get_descriptor_set_layout( scene_renderer->renderer->gpu, pointshadow_pipeline, CRUDE_GFX_MATERIAL_DESCRIPTOR_SET_INDEX );

  for ( uint32 i = 0; i < CRUDE_GFX_MAX_SWAPCHAIN_IMAGES; ++i )
  {
    crude_gfx_descriptor_set_creation                    ds_creation;
    
    ds_creation = crude_gfx_descriptor_set_creation_empty();
    ds_creation.layout = pointshadow_dsl;
    ds_creation.name = "pointshadow_ds";
  
    crude_gfx_scene_renderer_add_scene_resources_to_descriptor_set_creation( &ds_creation, pass->scene_renderer, i );
    crude_gfx_scene_renderer_add_mesh_resources_to_descriptor_set_creation( &ds_creation, pass->scene_renderer, i );
    crude_gfx_scene_renderer_add_light_resources_to_descriptor_set_creation( &ds_creation, pass->scene_renderer, i );
    crude_gfx_descriptor_set_creation_add_buffer( &ds_creation, pass->pointlight_spheres_sb[ i ], 10u );
    crude_gfx_descriptor_set_creation_add_buffer( &ds_creation, pass->pointlight_world_to_clip_sb[ i ], 11u );
    crude_gfx_descriptor_set_creation_add_buffer( &ds_creation, pass->pointshadow_meshlet_draw_commands_sb[ i ], 12u );
    crude_gfx_descriptor_set_creation_add_buffer( &ds_creation, pass->meshletes_instances_sb[ i ], 13u );
    
    pass->pointshadow_ds[ i ] = crude_gfx_create_descriptor_set( scene_renderer->renderer->gpu, &ds_creation );
  }

  texture_creation = crude_gfx_texture_creation_empty( );
  texture_creation.width = 512;
  texture_creation.height = 512;
  texture_creation.depth = 1u;
  texture_creation.format = VK_FORMAT_D16_UNORM;
  texture_creation.type = CRUDE_GFX_TEXTURE_TYPE_TEXTURE_2D;
  texture_creation.flags = CRUDE_GFX_TEXTURE_MASK_RENDER_TARGET;
  texture_creation.name = "tetrahedron_shadow_texture";
  pass->tetrahedron_shadow_texture = crude_gfx_create_texture( scene_renderer->renderer->gpu, &texture_creation );

  framebuffer_creation = crude_gfx_framebuffer_creation_empty( );
  framebuffer_creation.depth_stencil_texture = pass->tetrahedron_shadow_texture;
  framebuffer_creation.name = "tetrahedron_framebuffer";
  framebuffer_creation.width = 512;
  framebuffer_creation.height = 512;
  framebuffer_creation.manual_resources_free = true;
  pass->tetrahedron_framebuffer_handle = crude_gfx_create_framebuffer( scene_renderer->renderer->gpu, &framebuffer_creation );

  render_pass_creation = crude_gfx_render_pass_creation_empty( );
  render_pass_creation.name = "tetrahedron_render_pass";
  render_pass_creation.depth_stencil_final_layout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL;
  render_pass_creation.depth_stencil_format = VK_FORMAT_D16_UNORM;
  render_pass_creation.depth_operation = CRUDE_GFX_RENDER_PASS_OPERATION_DONT_CARE;
  pass->tetrahedron_render_pass_handle = crude_gfx_create_render_pass( scene_renderer->renderer->gpu, &render_pass_creation );
}

void
crude_gfx_pointlight_shadow_pass_deinitialize
(
  _In_ crude_gfx_pointlight_shadow_pass                   *pass
)
{
  crude_gfx_device                                        *gpu;

  gpu = pass->scene_renderer->renderer->gpu;

  for ( uint32 i = 0; i < CRUDE_GFX_MAX_SWAPCHAIN_IMAGES; ++i )
  {
    crude_gfx_destroy_descriptor_set( gpu, pass->pointshadow_culling_ds[ i ] );
    crude_gfx_destroy_descriptor_set( gpu, pass->pointshadow_commands_generation_ds[ i ] );
    crude_gfx_destroy_descriptor_set( gpu, pass->pointshadow_ds[ i ] );
    crude_gfx_destroy_buffer( gpu, pass->pointlight_world_to_clip_sb[ i ] );
    crude_gfx_destroy_buffer( gpu, pass->pointlight_spheres_sb[ i ] );
    crude_gfx_destroy_buffer( gpu, pass->pointshadow_meshlet_draw_commands_sb[ i ] );
    crude_gfx_destroy_buffer( gpu, pass->meshletes_instances_sb[ i ] );
    crude_gfx_destroy_buffer( gpu, pass->pointshadow_meshletes_instances_count_sb[ i ] );
  }

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
  gpu = pass->scene_renderer->renderer->gpu;

  pointshadow_culling_pipeline = crude_gfx_renderer_access_technique_pass_by_name( pass->scene_renderer->renderer, "meshlet", "pointshadow_culling" )->pipeline;
  pointshadow_commands_generation_pipeline = crude_gfx_renderer_access_technique_pass_by_name( pass->scene_renderer->renderer, "meshlet", "pointshadow_commands_generation" )->pipeline;
  pointshadow_pipeline = crude_gfx_renderer_access_technique_pass_by_name( pass->scene_renderer->renderer, "meshlet", "pointshadow" )->pipeline;

  crude_gfx_cmd_bind_pipeline( primary_cmd, pointshadow_culling_pipeline );
  crude_gfx_cmd_bind_descriptor_set( primary_cmd, pass->pointshadow_culling_ds[ gpu->current_frame ] );
  crude_gfx_cmd_dispatch( primary_cmd, ( pass->scene_renderer->total_meshes_instances_count * CRUDE_ARRAY_LENGTH( pass->scene_renderer->lights ) + 31 ) / 32, 1u, 1u );

  crude_gfx_cmd_bind_pipeline( primary_cmd, pointshadow_commands_generation_pipeline );
  crude_gfx_cmd_bind_descriptor_set( primary_cmd, pass->pointshadow_commands_generation_ds[ gpu->current_frame ] );
  crude_gfx_cmd_dispatch( primary_cmd, ( CRUDE_ARRAY_LENGTH( pass->scene_renderer->lights ) + 31 ) / 32, 1u, 1u );

  tetrahedron_shadow_texture = crude_gfx_access_texture( gpu, pass->tetrahedron_shadow_texture );
  
  width = tetrahedron_shadow_texture->width;
  height = tetrahedron_shadow_texture->height;
  
  {
    VkClearRect                                            clear_rect;
    VkClearDepthStencilValue                               clear_depth_stencil_value;
    VkImageSubresourceRange                                clear_range;
    crude_gfx_rect2d_int                                   scissor;
    crude_gfx_viewport                                     viewport;

    crude_gfx_cmd_add_image_barrier( primary_cmd, tetrahedron_shadow_texture, CRUDE_GFX_RESOURCE_STATE_COPY_DEST, 0, 1, true );
  
    clear_rect = CRUDE_COMPOUNT_EMPTY( VkClearRect );
    clear_rect.baseArrayLayer = 0;
    clear_rect.layerCount = 1u;
    clear_rect.rect.extent.width = width;
    clear_rect.rect.extent.height = height;
    clear_rect.rect.offset.x = 0;
    clear_rect.rect.offset.y = 0;

    clear_depth_stencil_value = CRUDE_COMPOUNT_EMPTY( VkClearDepthStencilValue );
    clear_depth_stencil_value.depth = 1.f;

    clear_range = CRUDE_COMPOUNT_EMPTY( VkImageSubresourceRange );
    clear_range.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
    clear_range.baseArrayLayer = 0;
    clear_range.baseMipLevel = 0;
    clear_range.levelCount = 1;
    clear_range.layerCount = 1u;
    vkCmdClearDepthStencilImage( primary_cmd->vk_cmd_buffer, tetrahedron_shadow_texture->vk_image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, &clear_depth_stencil_value, 1, &clear_range );
    
    crude_gfx_cmd_add_image_barrier( primary_cmd, tetrahedron_shadow_texture, CRUDE_GFX_RESOURCE_STATE_DEPTH_WRITE, 0, 1, true );

    scissor = CRUDE_COMPOUNT_EMPTY( crude_gfx_rect2d_int );
    scissor.x = 0u;
    scissor.y = 0u;
    scissor.width = width;
    scissor.height = height;
    crude_gfx_cmd_set_scissor( primary_cmd, &scissor );

    viewport = CRUDE_COMPOUNT_EMPTY( crude_gfx_viewport );
    viewport.rect.x = 0u;
    viewport.rect.y = 0u;
    viewport.rect.width = width;
    viewport.rect.height = height;
    viewport.min_depth = 0.0f;
    viewport.max_depth = 1.0f;
    crude_gfx_cmd_set_viewport( primary_cmd, &viewport );
  }
  
  crude_gfx_cmd_bind_render_pass( primary_cmd, pass->tetrahedron_render_pass_handle, pass->tetrahedron_framebuffer_handle, false );

  //const float32 fov0 = 143.98570868f + 1.99273682f;
  //const float32 fov1 = 125.26438968f + 2.78596497f;

  //crude_gfx_map_buffer_parameters cb_map = CRUDE_COMPOUNT_EMPTY( crude_gfx_map_buffer_parameters );
  //cb_map.buffer = pass->pointlight_world_to_clip_sb[ gpu->current_frame ];
  //XMFLOAT4X4 *pointlight_world_to_clip_mapped = CRUDE_CAST( XMFLOAT4X4*, crude_gfx_map_buffer( gpu, &cb_map ) );
  //
  //cb_map = CRUDE_COMPOUNT_EMPTY( crude_gfx_map_buffer_parameters );
  //cb_map.buffer = pass->pointlight_spheres_sb[ gpu->current_frame ];
  //XMFLOAT4 *pointlight_spheres_mapped = CRUDE_CAST( XMFLOAT4*, crude_gfx_map_buffer( gpu, &cb_map ) );

  //if ( pointlight_world_to_clip_mapped && pointlight_spheres_mapped )
  //{
  //  XMMATRIX                                               shadow_texture_matrices[ 4 ];
  //  XMMATRIX                                               face_rotation_matrices[ 4 ];
  //  XMMATRIX                                               rotation_matrix_x, rotation_matrix_y, rotation_matrix_z;

  //  rotation_matrix_x = XMMatrixRotationX( XMConvertToRadians( 27.36780516f ) );
  //  rotation_matrix_y = XMMatrixRotationY( XMConvertToRadians( 180.f ) );
  //  face_rotation_matrices[ 0 ] = XMMatrixMultiply( rotation_matrix_y, rotation_matrix_x );

  //  rotation_matrix_x = XMMatrixRotationX( XMConvertToRadians( 27.36780516f ) );
  //  rotation_matrix_y = XMMatrixRotationY( XMConvertToRadians( 0.f ) );
  //  rotation_matrix_z = XMMatrixRotationY( XMConvertToRadians( 90.f ) );
  //  face_rotation_matrices[ 1 ] = XMMatrixMultiply( rotation_matrix_z, XMMatrixMultiply( rotation_matrix_y, rotation_matrix_x ) );

  //  rotation_matrix_x = XMMatrixRotationX( XMConvertToRadians( -27.36780516f ) );
  //  rotation_matrix_y = XMMatrixRotationY( XMConvertToRadians( 270.f ) );
  //  face_rotation_matrices[ 2 ] = XMMatrixMultiply( rotation_matrix_y, rotation_matrix_x );

  //  rotation_matrix_x = XMMatrixRotationX( XMConvertToRadians( -27.36780516f ) );
  //  rotation_matrix_y = XMMatrixRotationY( XMConvertToRadians( 90.f ) );
  //  rotation_matrix_z = XMMatrixRotationY( XMConvertToRadians( 90.f ) );
  //  face_rotation_matrices[ 3 ] = XMMatrixMultiply( rotation_matrix_z, XMMatrixMultiply( rotation_matrix_y, rotation_matrix_x ) );

  //  float32 tile_size = 1.f;
  //  float32 tile_position_x = 0.f;
  //  float32 tile_position_y = 0.f;

  //  shadow_texture_matrices[ 0 ].r[ 0 ] = XMVectorSet( tile_size, 0.f ,0.f, 0.f );
  //  shadow_texture_matrices[ 0 ].r[ 1 ] = XMVectorSet( 0.f, tile_size * .5f, 0.f, 0.f );
  //  shadow_texture_matrices[ 0 ].r[ 2 ] = XMVectorSet( 0.f, 0.f, 1.f, 0.f );
  //  shadow_texture_matrices[ 0 ].r[ 3 ] = XMVectorSet( tile_position_x, tile_position_y - (tile_size * .5f), 0.f, 1.f );
  //  shadow_texture_matrices[ 1 ].r[ 0 ] = XMVectorSet( tile_size * .5f, 0.f ,0.f, 0.f );
  //  shadow_texture_matrices[ 1 ].r[ 1 ] = XMVectorSet( 0.f, tile_size, 0.f, 0.f );
  //  shadow_texture_matrices[ 1 ].r[ 2 ] = XMVectorSet( 0.f, 0.f, 1.f, 0.f );
  //  shadow_texture_matrices[ 1 ].r[ 3 ] = XMVectorSet( tile_position_x + (tile_size * .5f), tile_position_y, 0.f, 1.f );
  //  shadow_texture_matrices[ 2 ].r[ 0 ] = XMVectorSet( tile_size, 0.f ,0.f, 0.f );
  //  shadow_texture_matrices[ 2 ].r[ 1 ] = XMVectorSet( 0.f, tile_size * .5f, 0.f, 0.f );
  //  shadow_texture_matrices[ 2 ].r[ 2 ] = XMVectorSet( 0.f, 0.f, 1.f, 0.f );
  //  shadow_texture_matrices[ 2 ].r[ 3 ] = XMVectorSet( tile_position_x, tile_position_y + (tile_size * .5f), 0.f, 1.f );
  //  shadow_texture_matrices[ 3 ].r[ 0 ] = XMVectorSet( tile_size * .5f, 0.f, 0.f, 0.f );
  //  shadow_texture_matrices[ 3 ].r[ 1 ] = XMVectorSet( 0.f, tile_size, 0.f, 0.f );
  //  shadow_texture_matrices[ 3 ].r[ 2 ] = XMVectorSet( 0.f, 0.f, 0.f, 1.f );
  //  shadow_texture_matrices[ 3 ].r[ 3 ] = XMVectorSet( tile_position_x - (tile_size * .5f), tile_position_y, 0.f, 1.f );

  //  for ( uint32 i = 0; i < CRUDE_ARRAY_LENGTH( pass->scene_renderer->lights ); ++i )
  //  {
  //    crude_gfx_light_cpu const *light_cpu = &pass->scene_renderer->lights[ i ];
  //    crude_light const *light = CRUDE_ENTITY_GET_IMMUTABLE_COMPONENT( light_cpu->node, crude_light );
  //    crude_transform const *light_transform = CRUDE_ENTITY_GET_IMMUTABLE_COMPONENT( light_cpu->node, crude_transform );

  //    XMStoreFloat4( &pointlight_spheres_mapped[ i ], XMVectorSet( light_transform->translation.x, light_transform->translation.y, light_transform->translation.z, light->radius ) );
  //   
  //    mat4s shadow_projections[ 2 ];
  //    shadow_projections[ 0 ] = glms_perspective( glm_rad( fov1 ), fov0 / fov1, 0.01f, light.radius );
  //    shadow_projections[ 1 ] = glms_perspective( glm_rad( fov0 ), fov1 / fov0, 0.01f, light.radius );
  //   
  //    XMMATRIX translation = XMMatrixTranslation( -1.f * light_transform->translation.x, -1.f * light_transform->translation.y, -1.f * light_transform->translation.z );
  //   
  //    XMMATRIX world_to_view = XMMatrixMultiply( face_rotation_matrices[ 0 ], translation );
  //    XMMATRIX view_to_clip = XMMatrixMultiply( shadow_texture_matrices[ 0 ], XMMatrixMultiply( shadow_projections[ 0 ], world_to_view ) );
  //    pointlight_world_to_clip_mapped[ i * 4 + 0 ] = view_projection;
  //   
  //    view = glms_mat4_mul( face_rotation_matrices[ 1 ], translation );
  //    view_projection = glms_mat4_mul( shadow_texture_matrices[ 1 ], glms_mat4_mul( shadow_projections[ 1 ], view ) );
  //    pointlight_world_to_clip_mapped[ i * 4 + 1 ] = view_projection;
  //   
  //    view = glms_mat4_mul( face_rotation_matrices[ 2 ], translation );
  //    view_projection = glms_mat4_mul( shadow_texture_matrices[ 2 ], glms_mat4_mul( shadow_projections[ 0 ], view ) );
  //    pointlight_world_to_clip_mapped[ i * 4 + 2 ] = view_projection;
  //   
  //    view = glms_mat4_mul( face_rotation_matrices[ 3 ], translation );
  //    view_projection = glms_mat4_mul( shadow_texture_matrices[ 3 ], glms_mat4_mul( shadow_projections[ 1 ], view ) );
  //    pointlight_world_to_clip_mapped[ i * 4 + 3 ] = view_projection;
  //   }
  //   crude_gfx_unmap_buffer( gpu, pass->pointlight_world_to_view_sb[ gpu->current_frame ] );
  //   crude_gfx_unmap_buffer( gpu, pass->pointlight_spheres_sb[ gpu->current_frame ] );
  //}

  crude_gfx_cmd_bind_pipeline( primary_cmd, pointshadow_pipeline );

  crude_gfx_cmd_bind_descriptor_set( primary_cmd, pass->pointshadow_ds[ gpu->current_frame ] );
  
  //gpu_commands->draw_mesh_task_indirect_count( meshlet_shadow_indirect_cb[ current_frame_index ], 0, per_light_meshlet_instances[ current_frame_index ], sizeof( u32 ) * k_num_lights, layer_count, sizeof( vec4s ) );

  crude_gfx_cmd_end_render_pass( primary_cmd );
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
  return container;
}