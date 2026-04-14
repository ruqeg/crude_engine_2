#include <engine/graphics/scene_renderer.h>
#include <engine/graphics/scene_renderer_resources.h>

#include <engine/graphics/passes/pointlight_shadow_pass.h>

static int
crude_gfx_culled_light_cpu_area_cmp_
(
  _In_ void const                                         *a,
  _In_ void const                                         *b
)
{
  crude_gfx_culled_light_cpu                              *light_a;
  crude_gfx_culled_light_cpu                              *light_b;

  light_a = CRUDE_CAST( crude_gfx_culled_light_cpu*, a );
  light_b = CRUDE_CAST( crude_gfx_culled_light_cpu*, b );

  return light_a->screen_area - light_b->screen_area;
}


void
crude_gfx_pointlight_shadow_pass_initialize
(
  _In_ crude_gfx_pointlight_shadow_pass                   *pass,
  _In_ crude_gfx_scene_renderer                           *scene_renderer
)
{
  crude_gfx_texture_creation                               texture_creation;
  crude_gfx_framebuffer_creation                           framebuffer_creation;
  crude_gfx_render_pass_creation                           render_pass_creation;
  crude_gfx_sampler_creation                               sampler_creation;

  pass->scene_renderer = scene_renderer;
    
  pass->enabled = true;
  pass->pointlight_spheres_hga = crude_gfx_memory_allocate_with_name( scene_renderer->gpu, sizeof( XMUINT4 ) * CRUDE_LIGHTS_MAX_COUNT, CRUDE_GFX_MEMORY_TYPE_GPU, "pointlight_spheres_hga" );
  pass->pointshadow_meshlet_draw_commands_hga = crude_gfx_memory_allocate_with_name( scene_renderer->gpu, sizeof( XMUINT4 ) * ( CRUDE_LIGHTS_MAX_COUNT * 4u ), CRUDE_GFX_MEMORY_TYPE_GPU, "pointshadow_meshlet_draw_commands_hga" );
  pass->pointshadow_meshletes_instances_hga = crude_gfx_memory_allocate_with_name( scene_renderer->gpu, sizeof( XMUINT2 ) * CRUDE_MAX_MESHLETS_PER_LIGHT * CRUDE_LIGHTS_MAX_COUNT, CRUDE_GFX_MEMORY_TYPE_GPU, "pointshadow_meshletes_instances_hga" );
  pass->pointshadow_meshletes_instances_count_hga = crude_gfx_memory_allocate_with_name( scene_renderer->gpu, sizeof( uint32 ) * ( CRUDE_LIGHTS_MAX_COUNT + 1u ), CRUDE_GFX_MEMORY_TYPE_GPU, "pointshadow_meshletes_instances_count_hga" );

  texture_creation = crude_gfx_texture_creation_empty( );
  texture_creation.width = CRUDE_GFX_TETRAHEDRON_SHADOWMAP_SIZE;
  texture_creation.height = CRUDE_GFX_TETRAHEDRON_SHADOWMAP_SIZE;
  texture_creation.depth = 1u;
  texture_creation.format = VK_FORMAT_D32_SFLOAT;
  texture_creation.type = CRUDE_GFX_TEXTURE_TYPE_TEXTURE_2D;
  texture_creation.flags = CRUDE_GFX_TEXTURE_MASK_RENDER_TARGET;
  crude_string_copy( texture_creation.name, "tetrahedron_shadow_texture", sizeof( texture_creation.name ) );
  pass->tetrahedron_shadow_texture = crude_gfx_create_texture( scene_renderer->gpu, &texture_creation );

  framebuffer_creation = crude_gfx_framebuffer_creation_empty( );
  framebuffer_creation.depth_stencil_texture = pass->tetrahedron_shadow_texture;
  crude_string_copy( framebuffer_creation.name, "tetrahedron_framebuffer", sizeof( framebuffer_creation.name ) );
  framebuffer_creation.width = CRUDE_GFX_TETRAHEDRON_SHADOWMAP_SIZE;
  framebuffer_creation.height = CRUDE_GFX_TETRAHEDRON_SHADOWMAP_SIZE;
  framebuffer_creation.manual_resources_free = true;
  pass->tetrahedron_framebuffer_handle = crude_gfx_create_framebuffer( scene_renderer->gpu, &framebuffer_creation );

  render_pass_creation = crude_gfx_render_pass_creation_empty( );
  render_pass_creation.name = "tetrahedron_render_pass";
  render_pass_creation.depth_stencil_final_layout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL;
  render_pass_creation.depth_stencil_format = texture_creation.format;
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
}

void
crude_gfx_pointlight_shadow_pass_deinitialize
(
  _In_ crude_gfx_pointlight_shadow_pass                   *pass
)
{
  crude_gfx_device                                        *gpu;

  gpu = pass->scene_renderer->gpu;
  
  crude_gfx_memory_deallocate( gpu, pass->pointlight_spheres_hga );
  crude_gfx_memory_deallocate( gpu, pass->pointshadow_meshlet_draw_commands_hga );
  crude_gfx_memory_deallocate( gpu, pass->pointshadow_meshletes_instances_hga );
  crude_gfx_memory_deallocate( gpu, pass->pointshadow_meshletes_instances_count_hga );

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
  crude_gfx_texture                                       *tetrahedron_shadow_texture;
  crude_gfx_pipeline_handle                                pointshadow_culling_pipeline;
  crude_gfx_pipeline_handle                                pointshadow_commands_generation_pipeline;
  crude_gfx_pipeline_handle                                pointshadow_pipeline;
  uint32                                                   width, height;

  pass = CRUDE_REINTERPRET_CAST( crude_gfx_pointlight_shadow_pass*, ctx );
  gpu = pass->scene_renderer->gpu;

  if ( !pass->enabled || CRUDE_ARRAY_LENGTH( pass->scene_renderer->culled_lights ) == 0 )
  {
    return;
  }

  pointshadow_culling_pipeline = crude_gfx_access_technique_pass_by_name( pass->scene_renderer->gpu, "pointshadow_meshlet", "pointshadow_culling" )->pipeline;
  pointshadow_commands_generation_pipeline = crude_gfx_access_technique_pass_by_name( pass->scene_renderer->gpu, "pointshadow_meshlet", "pointshadow_commands_generation" )->pipeline;
  pointshadow_pipeline = crude_gfx_access_technique_pass_by_name( pass->scene_renderer->gpu, "pointshadow_meshlet", "pointshadow" )->pipeline;
  
  /* Pointshadow Culling */
  {
    CRUDE_ALIGNED_STRUCT( 16 ) push_constant_
    {
      SceneRef                                             scene;
      MeshDrawsRef                                         mesh_draws;

      MeshInstancesDrawsRef                                mesh_instance_draws;
      MeshletsRef                                          meshlets;

      LightsRef                                            lights;
      PointshadowMeshletesInstancesRef                     pointshadow_meshletes_instances;

      PointshadowMeshletesInstancesCountRef                pointshadow_meshletes_instances_count;
      XMFLOAT2                                             padding;
    };
    
    push_constant_                                         push_constant;

    crude_gfx_cmd_push_marker( primary_cmd, "pointshadow_culling" );
    
    crude_gfx_cmd_add_buffer_barrier( primary_cmd, pass->pointshadow_meshletes_instances_hga.buffer_handle, CRUDE_GFX_RESOURCE_STATE_SHADER_RESOURCE, CRUDE_GFX_RESOURCE_STATE_UNORDERED_ACCESS );
    crude_gfx_cmd_add_buffer_barrier( primary_cmd, pass->pointshadow_meshletes_instances_count_hga.buffer_handle, CRUDE_GFX_RESOURCE_STATE_INDIRECT_ARGUMENT, CRUDE_GFX_RESOURCE_STATE_UNORDERED_ACCESS );
  
    crude_gfx_cmd_fill_buffer( primary_cmd, pass->pointshadow_meshletes_instances_count_hga.buffer_handle, 0u );

    crude_gfx_cmd_bind_pipeline( primary_cmd, pointshadow_culling_pipeline );
    
    push_constant.scene = pass->scene_renderer->scene_hga.gpu_address;
    push_constant.mesh_draws = pass->scene_renderer->model_renderer_resources_manager->meshes_draws_hga.gpu_address;
    push_constant.mesh_instance_draws = pass->scene_renderer->meshes_instances_draws_hga.gpu_address;
    push_constant.meshlets = pass->scene_renderer->model_renderer_resources_manager->meshlets_hga.gpu_address;
    push_constant.lights = pass->scene_renderer->lights_hga.gpu_address;
    push_constant.pointshadow_meshletes_instances = pass->pointshadow_meshletes_instances_hga.gpu_address;
    push_constant.pointshadow_meshletes_instances_count = pass->pointshadow_meshletes_instances_count_hga.gpu_address;
    crude_gfx_cmd_push_constant( primary_cmd, &push_constant, sizeof( push_constant ) );

    crude_gfx_cmd_bind_bindless_descriptor_set( primary_cmd );

    crude_gfx_cmd_dispatch( primary_cmd, ( pass->scene_renderer->total_visible_meshes_instances_count * CRUDE_ARRAY_LENGTH( pass->scene_renderer->culled_lights ) + 31 ) / 32, 1u, 1u );
  
    crude_gfx_cmd_add_buffer_barrier( primary_cmd, pass->pointshadow_meshletes_instances_count_hga.buffer_handle, CRUDE_GFX_RESOURCE_STATE_UNORDERED_ACCESS, CRUDE_GFX_RESOURCE_STATE_INDIRECT_ARGUMENT );
    crude_gfx_cmd_add_buffer_barrier( primary_cmd, pass->pointshadow_meshletes_instances_hga.buffer_handle, CRUDE_GFX_RESOURCE_STATE_UNORDERED_ACCESS, CRUDE_GFX_RESOURCE_STATE_SHADER_RESOURCE );
    
    crude_gfx_cmd_pop_marker( primary_cmd );
  }
  
  /* Pointshadow Commands Generation */
  {
    CRUDE_ALIGNED_STRUCT( 16 ) push_constant_
    {
      SceneRef                                             scene;
      PointshadowMeshletesInstancesCountRef                pointshadow_meshletes_instances_count;
      PointshadowMeshletDrawCommands                       pointshadow_meshlet_draw_commands;
      XMFLOAT2                                             push_constant_padding_;
    };
    
    push_constant_                                         push_constant;

    crude_gfx_cmd_push_marker( primary_cmd, "pointshadow_commands_generation" );

    crude_gfx_cmd_add_buffer_barrier( primary_cmd, pass->pointshadow_meshlet_draw_commands_hga.buffer_handle, CRUDE_GFX_RESOURCE_STATE_INDIRECT_ARGUMENT, CRUDE_GFX_RESOURCE_STATE_UNORDERED_ACCESS );
    
    crude_gfx_cmd_bind_pipeline( primary_cmd, pointshadow_commands_generation_pipeline );

    push_constant.scene = pass->scene_renderer->scene_hga.gpu_address;
    push_constant.pointshadow_meshletes_instances_count = pass->pointshadow_meshletes_instances_count_hga.gpu_address;
    push_constant.pointshadow_meshlet_draw_commands = pass->pointshadow_meshlet_draw_commands_hga.gpu_address;
    crude_gfx_cmd_push_constant( primary_cmd, &push_constant, sizeof( push_constant ) );

    crude_gfx_cmd_bind_bindless_descriptor_set( primary_cmd );

    crude_gfx_cmd_dispatch( primary_cmd, ( CRUDE_ARRAY_LENGTH( pass->scene_renderer->culled_lights ) + 31 ) / 32, 1u, 1u );
  
    // !TODO idk why but it kills all the perf XDXDXD
    crude_gfx_cmd_add_buffer_barrier( primary_cmd, pass->pointshadow_meshlet_draw_commands_hga.buffer_handle, CRUDE_GFX_RESOURCE_STATE_UNORDERED_ACCESS, CRUDE_GFX_RESOURCE_STATE_INDIRECT_ARGUMENT );
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
    CRUDE_ALIGNED_STRUCT( 16 ) push_constant_
    {
      SceneRef                                             scene;
      MeshDrawsRef                                         mesh_draws;

      MeshInstancesDrawsRef                                mesh_instance_draws;
      MeshletsRef                                          meshlets;

      VerticesRef                                          vertices;
      TrianglesIndicesRef                                  triangles_indices;

      VerticesIndicesRef                                   vertices_indices;
      VkDeviceAddress                                      debug_counts;

      JointMatricesRef                                     joint_matrices;
      LightsRef                                            lights;

      PointshadowMeshletesInstancesRef                     pointshadow_meshletes_instances;
      PointshadowMeshletesInstancesCountRef                pointshadow_meshletes_instances_count;

      PointlightSpheresRef                                 pointlight_spheres;
      PointshadowMeshletDrawCommands                       pointshadow_meshlet_draw_commands;

      LightsWorldToTextureRef                              lights_world_to_texture;
      XMFLOAT2                                             padding;
    };
    
    XMFLOAT4                                              *pointlight_spheres;
    XMFLOAT4X4                                            *pointlight_world_to_clip;
    push_constant_                                         push_constant;
    crude_gfx_memory_allocation                            pointlight_spheres_tca;
    crude_gfx_memory_allocation                            pointlight_world_to_clip_tca;
    float32                                                fov0, fov1; 

    pointlight_spheres_tca = crude_gfx_linear_allocator_allocate( &gpu->frame_linear_allocator, pass->pointlight_spheres_hga.size );
    pointlight_spheres = CRUDE_CAST( XMFLOAT4*, pointlight_spheres_tca.cpu_address );

    pointlight_world_to_clip_tca = crude_gfx_linear_allocator_allocate( &gpu->frame_linear_allocator, pass->scene_renderer->lights_world_to_texture_hga.size );
    pointlight_world_to_clip = CRUDE_CAST( XMFLOAT4X4*, pointlight_world_to_clip_tca.cpu_address );

    fov0 = 143.98570868f + 1.99273682f;
    fov1 = 125.26438968f + 2.78596497f;
  
    if ( pointlight_spheres && pointlight_world_to_clip )
    {
      XMMATRIX                                             clip_to_face_clip[ 4 ];
      XMMATRIX                                             view_to_faceed_view[ 4 ];
      XMMATRIX                                             x_rot_matrix, y_rot_matrix, z_rot_matrix;
      
      x_rot_matrix = XMMatrixRotationX( XMConvertToRadians( 27.36780516f ) );
      y_rot_matrix = XMMatrixRotationY( XMConvertToRadians( 0.f ) );
      z_rot_matrix = XMMatrixRotationZ( XMConvertToRadians( 0.f ) );
      view_to_faceed_view[ 0 ] = XMMatrixMultiply( XMMatrixMultiply( y_rot_matrix, x_rot_matrix ), z_rot_matrix );
      x_rot_matrix = XMMatrixRotationX( XMConvertToRadians( 27.36780516f ) );
      y_rot_matrix = XMMatrixRotationY( XMConvertToRadians( 180.f ) );
      z_rot_matrix = XMMatrixRotationZ( XMConvertToRadians( 90.f ) );
      view_to_faceed_view[ 1 ] = XMMatrixMultiply( XMMatrixMultiply( y_rot_matrix, x_rot_matrix ), z_rot_matrix );
      y_rot_matrix = XMMatrixRotationY( XMConvertToRadians( 90.f ) );
      x_rot_matrix = XMMatrixRotationX( XMConvertToRadians( -27.36780516f ) );
      view_to_faceed_view[ 2 ] = XMMatrixMultiply( y_rot_matrix, x_rot_matrix );
      x_rot_matrix = XMMatrixRotationX( XMConvertToRadians( -27.36780516f ) );
      y_rot_matrix = XMMatrixRotationY( XMConvertToRadians( 270.f ) );
      z_rot_matrix = XMMatrixRotationZ( XMConvertToRadians( 90.f ) );
      view_to_faceed_view[ 3 ] = XMMatrixMultiply( XMMatrixMultiply( y_rot_matrix, x_rot_matrix ), z_rot_matrix );

      for ( uint32 i = 0; i < CRUDE_ARRAY_LENGTH( pass->scene_renderer->culled_lights ); ++i )
      {
        crude_gfx_culled_light_cpu const                  *culled_light_cpu;
        crude_gfx_light_cpu const                         *light_cpu;
        crude_light const                                 *light;
        XMMATRIX                                           view_to_clip[ 2 ];
        XMMATRIX                                           world_to_view, world_to_faced_view, world_to_clip;
        float32                                            tile_position_x, tile_position_y;

        culled_light_cpu = &pass->scene_renderer->culled_lights[ i ];
        light_cpu = &pass->scene_renderer->lights[ culled_light_cpu->light_index ];
        light = &light_cpu->light;

        /* from [0;1] to [-1;1] with tile_size offset from begining */
        tile_position_x = -1.f + culled_light_cpu->tile_position.x * 2 + culled_light_cpu->tile_size;
        tile_position_y = 1.f - culled_light_cpu->tile_position.y * 2 - culled_light_cpu->tile_size;

        clip_to_face_clip[ 0 ] = XMMatrixSet(
          culled_light_cpu->tile_size, 0.f , 0.f, 0.f,
          0.f, culled_light_cpu->tile_size * 0.5f, 0.f, 0.f,
          0.f, 0.f, 1.f, 0.f,
          tile_position_x, tile_position_y - ( culled_light_cpu->tile_size * 0.5f ), 0.f, 1.f
        );
        clip_to_face_clip[ 1 ] = XMMatrixSet(
          culled_light_cpu->tile_size * 0.5f, 0.f, 0.f, 0.f,
          0.f, culled_light_cpu->tile_size, 0.f, 0.f,
          0.f, 0.f, 1.f, 0.f,
          tile_position_x + ( culled_light_cpu->tile_size * 0.5f ), tile_position_y, 0.f, 1.f
        );
        clip_to_face_clip[ 2 ] = XMMatrixSet(
          culled_light_cpu->tile_size, 0.f, 0.f, 0.f,
          0.f, culled_light_cpu->tile_size * 0.5f, 0.f, 0.f,
          0.f, 0.f, 1.f, 0.f,
          tile_position_x, tile_position_y + ( culled_light_cpu->tile_size * 0.5f ), 0.f, 1.f
        );
        clip_to_face_clip[ 3 ] = XMMatrixSet(
          culled_light_cpu->tile_size * 0.5f, 0.f, 0.f, 0.f,
          0.f, culled_light_cpu->tile_size, 0.f, 0.f,
          0.f, 0.f, 1.f, 0.f,
          tile_position_x - ( culled_light_cpu->tile_size * 0.5f ), tile_position_y, 0.f, 1.f
        );

        XMStoreFloat4( &pointlight_spheres[ i ], XMVectorSet( light_cpu->translation.x, light_cpu->translation.y, light_cpu->translation.z, light->radius ) );
   
        view_to_clip[ 0 ] = XMMatrixPerspectiveFovRH( XMConvertToRadians( fov1 ), tanf( XMConvertToRadians( fov0 ) * 0.5f ) / tanf( XMConvertToRadians( fov1 ) * 0.5f ), 0.2f, light->radius );
        view_to_clip[ 1 ] = XMMatrixPerspectiveFovRH( XMConvertToRadians( fov0 ), tanf( XMConvertToRadians( fov1 ) * 0.5f ) / tanf( XMConvertToRadians( fov0 ) * 0.5f ), 0.2f, light->radius );
  
        world_to_view = XMMatrixTranslation( -1.f * light_cpu->translation.x, -1.f * light_cpu->translation.y, -1.f * light_cpu->translation.z );
  
        world_to_faced_view = XMMatrixMultiply( world_to_view, view_to_faceed_view[ 0 ] );
        world_to_clip = XMMatrixMultiply( world_to_faced_view, view_to_clip[ 0 ] );
        XMStoreFloat4x4( &pointlight_world_to_clip[ i * 4 + 0 ], XMMatrixMultiply( world_to_clip, clip_to_face_clip[ 0 ] ) );
  
        world_to_faced_view = XMMatrixMultiply( world_to_view, view_to_faceed_view[ 1 ] );
        world_to_clip = XMMatrixMultiply( world_to_faced_view, view_to_clip[ 1 ] );
        XMStoreFloat4x4( &pointlight_world_to_clip[ i * 4 + 1], XMMatrixMultiply( world_to_clip, clip_to_face_clip[ 1 ] ) );
        
        world_to_faced_view = XMMatrixMultiply( world_to_view, view_to_faceed_view[ 2 ] );
        world_to_clip = XMMatrixMultiply( world_to_faced_view, view_to_clip[ 0 ] );
        XMStoreFloat4x4( &pointlight_world_to_clip[ i * 4 + 2 ], XMMatrixMultiply( world_to_clip, clip_to_face_clip[ 2 ] ) );
  
        world_to_faced_view = XMMatrixMultiply( world_to_view, view_to_faceed_view[ 3 ] );
        world_to_clip = XMMatrixMultiply( world_to_faced_view, view_to_clip[ 1 ] );
        XMStoreFloat4x4( &pointlight_world_to_clip[ i * 4 + 3 ], XMMatrixMultiply( world_to_clip, clip_to_face_clip[ 3 ] ) );
      }
      
      crude_gfx_cmd_memory_copy( primary_cmd, pointlight_world_to_clip_tca, pass->scene_renderer->lights_world_to_texture_hga, 0, 0 );
      crude_gfx_cmd_memory_copy( primary_cmd, pointlight_spheres_tca, pass->pointlight_spheres_hga, 0, 0 );
    }
    
    crude_gfx_cmd_push_marker( primary_cmd, "pointshadow_draw_pass" );

    crude_gfx_cmd_add_image_barrier( primary_cmd, crude_gfx_access_texture( gpu, pass->tetrahedron_shadow_texture ), CRUDE_GFX_RESOURCE_STATE_DEPTH_WRITE, 0u, 1u, true );
  
    crude_gfx_cmd_bind_render_pass( primary_cmd, pass->tetrahedron_render_pass_handle, pass->tetrahedron_framebuffer_handle, false );
    crude_gfx_cmd_bind_pipeline( primary_cmd, pointshadow_pipeline );
  
    push_constant.scene = pass->scene_renderer->scene_hga.gpu_address;
    push_constant.mesh_draws = pass->scene_renderer->model_renderer_resources_manager->meshes_draws_hga.gpu_address;
    push_constant.mesh_instance_draws = pass->scene_renderer->meshes_instances_draws_hga.gpu_address;
    push_constant.meshlets = pass->scene_renderer->model_renderer_resources_manager->meshlets_hga.gpu_address;
    push_constant.vertices = pass->scene_renderer->model_renderer_resources_manager->meshlets_vertices_hga.gpu_address;
    push_constant.triangles_indices = pass->scene_renderer->model_renderer_resources_manager->meshlets_triangles_indices_hga.gpu_address;
    push_constant.vertices_indices = pass->scene_renderer->model_renderer_resources_manager->meshlets_vertices_indices_hga.gpu_address;
    push_constant.debug_counts = pass->scene_renderer->debug_commands_hga.gpu_address;
    push_constant.joint_matrices = pass->scene_renderer->joint_matrices_hga.gpu_address;
    push_constant.lights = pass->scene_renderer->lights_hga.gpu_address;
    push_constant.pointshadow_meshletes_instances = pass->pointshadow_meshletes_instances_hga.gpu_address;
    push_constant.pointshadow_meshletes_instances_count = pass->pointshadow_meshletes_instances_count_hga.gpu_address;
    push_constant.pointlight_spheres = pass->pointlight_spheres_hga.gpu_address;
    push_constant.pointshadow_meshlet_draw_commands = pass->pointshadow_meshlet_draw_commands_hga.gpu_address;
    push_constant.lights_world_to_texture = pass->scene_renderer->lights_world_to_texture_hga.gpu_address;
    crude_gfx_cmd_push_constant( primary_cmd, &push_constant, sizeof( push_constant ) );

    crude_gfx_cmd_bind_bindless_descriptor_set( primary_cmd );
  
    crude_gfx_cmd_draw_mesh_task_indirect_count( primary_cmd,
      pass->pointshadow_meshlet_draw_commands_hga.buffer_handle, 0u,
      pass->pointshadow_meshletes_instances_count_hga.buffer_handle, sizeof( uint32 ) * CRUDE_LIGHTS_MAX_COUNT,
      4u * CRUDE_LIGHTS_MAX_COUNT, sizeof( XMUINT4 ) );
  
    crude_gfx_cmd_end_render_pass( primary_cmd );
  
    crude_gfx_cmd_add_image_barrier( primary_cmd, crude_gfx_access_texture( gpu, pass->tetrahedron_shadow_texture ), CRUDE_GFX_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, 0u, 1u, true );
    crude_gfx_cmd_pop_marker( primary_cmd );
  }
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