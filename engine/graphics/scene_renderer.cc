#include <TaskScheduler_c.h>

#include <core/profiler.h>
#include <core/array.h>
#include <core/file.h>
#include <core/hash_map.h>

#include <scene/scene_components.h>
#include <scene/scene.h>

#include <graphics/scene_renderer_uploader.h>

#include <graphics/scene_renderer.h>


/**
 *
 * TODO
 * 
 */
void
scene_renderer_prepare_node_draws_
(
  _In_ crude_gfx_scene_renderer                           *scene_renderer,
  _In_ crude_entity                                        node,
  _In_ crude_stack_allocator                              *temporary_allocator
);

static int
sorting_light_
(
  _In_ const void                                         *a,
  _In_ const void                                         *b
)
{
  crude_gfx_sorted_light const * la = CRUDE_CAST( crude_gfx_sorted_light const*, a );
  crude_gfx_sorted_light const * lb = CRUDE_CAST( crude_gfx_sorted_light const*, b );

  if ( la->projected_z < lb->projected_z )
  {
    return -1;
  }
  else if ( la->projected_z > lb->projected_z )
  {
    return 1;
  }
  return 0;
}


/*
 * CGLM converted method from:
 * 2D Polyhedral Bounds of a Clipped, Perspective - Projected 3D Sphere
 * By Michael Mara Morgan McGuire
 */
void
get_bounds_for_axis_
( 
  _In_ XMVECTOR                                            a, /* Bounding axis (camera space) */
  _In_ XMVECTOR                                            c, /* Sphere center (camera space) */
  _In_ float                                               r, /* Sphere radius */
  _In_ float                                               znear, /* Near clipping plane (negative) */
  _Out_ XMVECTOR                                          *l, /* Tangent point (camera space) */
  _Out_ XMVECTOR                                          *u /* Tangent point (camera space) */
)
{
  c = XMVectorSet( XMVectorGetX( XMVector3Dot( a, c ) ), XMVectorGetZ( c ), 0, 0 );
  XMVECTOR bounds[ 2 ];
  const float tsquared = XMVectorGetX( XMVector2Dot( c, c ) ) - ( r * r );
  const bool camera_inside_sphere = ( tsquared <= 0 );
  XMVECTOR v = camera_inside_sphere ? XMVectorZero( ) : XMVectorDivide( XMVectorSet( sqrt( tsquared ), r, 0, 0 ), XMVector2Normalize( c ) );
  const bool clip_sphere = ( XMVectorGetY( c ) + r >= znear );
  
  float k = sqrt( ( r * r ) - ( ( znear - XMVectorGetY( c ) ) * ( znear - XMVectorGetY( c ) ) ) );
  for ( int i = 0; i < 2; ++i )
  {
    if ( !camera_inside_sphere )
    {
      XMMATRIX transform = XMMatrixSet( XMVectorGetX( v ) -XMVectorGetY( v ), 0, 0, XMVectorGetY( v ), XMVectorGetX( v ), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 );
      bounds[ i ] = XMVector2Transform( XMVectorScale( c, XMVectorGetX( v ) ), transform );
    }

    const bool clip_bound = camera_inside_sphere || ( XMVectorGetY( bounds[ i ] ) > znear );

    if ( clip_sphere && clip_bound )
    {
      bounds[ i ] = XMVectorSet( XMVectorGetX( c ) + k, znear, 0, 0 );
    }

    v = XMVectorSetY( v, -XMVectorGetY( v ) );
    k = -k;
  }

  *l = XMVectorScale( a, XMVectorGetX( bounds[ 1 ] ) );
  *l = XMVectorSetZ( *l, XMVectorGetY( bounds[ 1 ] ) );
  *u = XMVectorScale( a, XMVectorGetX( bounds[ 0 ] ) );
  *u = XMVectorSetZ( *u, XMVectorGetY( bounds[ 0 ] ) );
}

XMVECTOR
project_
(
  _In_ XMMATRIX                                            p,
  _In_ XMVECTOR                                            q
)
{
  XMVECTOR v = XMVector4Transform( XMVectorSetW( q, 1.f ), p );
  v = XMVectorScale( v, 1.f / XMVectorGetW( v ) );
  return XMVectorSetW( v, 1 );
}

/**
 *
 * Renderer Scene Function
 * 
 */
void
crude_gfx_scene_renderer_initialize
(
  _In_ crude_gfx_scene_renderer                           *scene_renderer,
  _In_ crude_gfx_scene_renderer_creation                  *creation
)
{
  {
    scene_renderer->allocator_container = creation->allocator_container;
    scene_renderer->renderer = creation->renderer;
    scene_renderer->async_loader = creation->async_loader;
    scene_renderer->task_scheduler = creation->task_scheduler;
    scene_renderer->imgui_context = creation->imgui_context;

    CRUDE_ARRAY_INITIALIZE_WITH_CAPACITY( scene_renderer->images, 0u, scene_renderer->allocator_container );
    CRUDE_ARRAY_INITIALIZE_WITH_CAPACITY( scene_renderer->samplers, 0u, scene_renderer->allocator_container );
    CRUDE_ARRAY_INITIALIZE_WITH_CAPACITY( scene_renderer->buffers, 0u, scene_renderer->allocator_container );
    CRUDE_ARRAY_INITIALIZE_WITH_CAPACITY( scene_renderer->meshes, 0u, scene_renderer->allocator_container );
    CRUDE_ARRAY_INITIALIZE_WITH_CAPACITY( scene_renderer->meshlets_vertices, 0u, scene_renderer->allocator_container );
    CRUDE_ARRAY_INITIALIZE_WITH_CAPACITY( scene_renderer->meshlets_triangles_indices, 0u, scene_renderer->allocator_container );
    CRUDE_ARRAY_INITIALIZE_WITH_CAPACITY( scene_renderer->meshlets_vertices_indices, 0u, scene_renderer->allocator_container );
    CRUDE_ARRAY_INITIALIZE_WITH_CAPACITY( scene_renderer->meshlets, 0u, scene_renderer->allocator_container );
    CRUDE_ARRAY_INITIALIZE_WITH_CAPACITY( scene_renderer->meshes_instances, 0u, scene_renderer->renderer->allocator_container );
  }

  scene_renderer_prepare_node_draws_( scene_renderer, creation->node, creation->temporary_allocator );

  {
    crude_gfx_buffer_creation                                buffer_creation;

    buffer_creation = CRUDE_COMPOUNT_EMPTY( crude_gfx_buffer_creation );
    buffer_creation.type_flags = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
    buffer_creation.usage = CRUDE_GFX_RESOURCE_USAGE_TYPE_DYNAMIC;
    buffer_creation.size = sizeof( crude_gfx_scene_constant_gpu );
    buffer_creation.name = "frame_buffer";
    scene_renderer->scene_cb = crude_gfx_create_buffer( scene_renderer->renderer->gpu, &buffer_creation );

    buffer_creation = CRUDE_COMPOUNT_EMPTY( crude_gfx_buffer_creation );
    buffer_creation.type_flags = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
    buffer_creation.usage = CRUDE_GFX_RESOURCE_USAGE_TYPE_IMMUTABLE;
    buffer_creation.size = sizeof*( scene_renderer->meshlets ) * CRUDE_ARRAY_LENGTH( scene_renderer->meshlets );
    buffer_creation.initial_data = scene_renderer->meshlets;
    buffer_creation.name = "meshlet_sb";
    scene_renderer->meshlets_sb = crude_gfx_create_buffer( scene_renderer->renderer->gpu, &buffer_creation );
    
    buffer_creation = CRUDE_COMPOUNT_EMPTY( crude_gfx_buffer_creation );
    buffer_creation.type_flags = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
    buffer_creation.usage = CRUDE_GFX_RESOURCE_USAGE_TYPE_IMMUTABLE;
    buffer_creation.size = sizeof*( scene_renderer->meshlets_vertices ) * CRUDE_ARRAY_LENGTH( scene_renderer->meshlets_vertices );
    buffer_creation.initial_data = scene_renderer->meshlets_vertices;
    buffer_creation.name = "meshlets_vertices_sb";
    scene_renderer->meshlets_vertices_sb = crude_gfx_create_buffer( scene_renderer->renderer->gpu, &buffer_creation );
    
    buffer_creation = CRUDE_COMPOUNT_EMPTY( crude_gfx_buffer_creation );
    buffer_creation.type_flags = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
    buffer_creation.usage = CRUDE_GFX_RESOURCE_USAGE_TYPE_IMMUTABLE;
    buffer_creation.size = sizeof*( scene_renderer->meshlets_vertices_indices ) * CRUDE_ARRAY_LENGTH( scene_renderer->meshlets_vertices_indices );
    buffer_creation.initial_data = scene_renderer->meshlets_vertices_indices;
    buffer_creation.name = "meshlets_vertices_indices_sb";
    scene_renderer->meshlets_vertices_indices_sb = crude_gfx_create_buffer( scene_renderer->renderer->gpu, &buffer_creation );
    
    buffer_creation = CRUDE_COMPOUNT_EMPTY( crude_gfx_buffer_creation );
    buffer_creation.type_flags = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
    buffer_creation.usage = CRUDE_GFX_RESOURCE_USAGE_TYPE_IMMUTABLE;
    buffer_creation.size = sizeof*( scene_renderer->meshlets_triangles_indices ) * CRUDE_ARRAY_LENGTH( scene_renderer->meshlets_triangles_indices );
    buffer_creation.initial_data = scene_renderer->meshlets_triangles_indices;
    buffer_creation.name = "meshlets_primitives_indices_sb";
    scene_renderer->meshlets_triangles_indices_sb = crude_gfx_create_buffer( scene_renderer->renderer->gpu, &buffer_creation );

    buffer_creation = CRUDE_COMPOUNT_EMPTY( crude_gfx_buffer_creation );
    buffer_creation.type_flags = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
    buffer_creation.usage = CRUDE_GFX_RESOURCE_USAGE_TYPE_DYNAMIC;
    buffer_creation.size = sizeof( crude_gfx_mesh_darw_gpu ) * CRUDE_ARRAY_LENGTH( scene_renderer->meshes );
    buffer_creation.name = "meshes_draws_sb";
    scene_renderer->meshes_draws_sb = crude_gfx_create_buffer( scene_renderer->renderer->gpu, &buffer_creation );

    buffer_creation = CRUDE_COMPOUNT_EMPTY( crude_gfx_buffer_creation );
    buffer_creation.type_flags = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
    buffer_creation.usage = CRUDE_GFX_RESOURCE_USAGE_TYPE_DYNAMIC;
    buffer_creation.size = sizeof( crude_gfx_mesh_instance_draw_gpu ) * CRUDE_ARRAY_LENGTH( scene_renderer->meshes_instances );
    buffer_creation.name = "meshes_instances_draws_sb";
    scene_renderer->meshes_instances_draws_sb = crude_gfx_create_buffer( scene_renderer->renderer->gpu, &buffer_creation );

    buffer_creation = CRUDE_COMPOUNT_EMPTY( crude_gfx_buffer_creation );
    buffer_creation.type_flags = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
    buffer_creation.usage = CRUDE_GFX_RESOURCE_USAGE_TYPE_DYNAMIC;
    buffer_creation.size = sizeof( XMFLOAT4 ) * CRUDE_ARRAY_LENGTH( scene_renderer->meshes );
    buffer_creation.name = "meshes_bounds_sb";
    scene_renderer->meshes_bounds_sb = crude_gfx_create_buffer( scene_renderer->renderer->gpu, &buffer_creation );

    /* Create indirect buffers */
    for ( uint32 i = 0; i < CRUDE_GFX_MAX_SWAPCHAIN_IMAGES; ++i )
    {
      buffer_creation = CRUDE_COMPOUNT_EMPTY( crude_gfx_buffer_creation );
      buffer_creation.type_flags = VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
      buffer_creation.usage = CRUDE_GFX_RESOURCE_USAGE_TYPE_DYNAMIC;
      buffer_creation.size = CRUDE_ARRAY_LENGTH( scene_renderer->meshes_instances ) * sizeof( crude_gfx_mesh_draw_command_gpu );
      buffer_creation.name = "draw_commands_early_sb";
      scene_renderer->mesh_task_indirect_commands_early_sb[ i ] = crude_gfx_create_buffer( scene_renderer->renderer->gpu, &buffer_creation );

      buffer_creation = CRUDE_COMPOUNT_EMPTY( crude_gfx_buffer_creation );
      buffer_creation.type_flags = VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
      buffer_creation.usage = CRUDE_GFX_RESOURCE_USAGE_TYPE_DYNAMIC;
      buffer_creation.size = CRUDE_ARRAY_LENGTH( scene_renderer->meshes_instances ) * sizeof( crude_gfx_mesh_draw_command_gpu );
      buffer_creation.name = "draw_commands_late_sb";
      scene_renderer->mesh_task_indirect_commands_late_sb[ i ] = crude_gfx_create_buffer( scene_renderer->renderer->gpu, &buffer_creation );
      
      buffer_creation = CRUDE_COMPOUNT_EMPTY( crude_gfx_buffer_creation );
      buffer_creation.type_flags = VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
      buffer_creation.usage = CRUDE_GFX_RESOURCE_USAGE_TYPE_DYNAMIC;
      buffer_creation.size = sizeof( crude_gfx_mesh_draw_counts_gpu );
      buffer_creation.name = "mesh_count_early_sb";
      scene_renderer->mesh_task_indirect_count_early_sb[ i ] = crude_gfx_create_buffer( scene_renderer->renderer->gpu, &buffer_creation );

      buffer_creation = CRUDE_COMPOUNT_EMPTY( crude_gfx_buffer_creation );
      buffer_creation.type_flags = VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
      buffer_creation.usage = CRUDE_GFX_RESOURCE_USAGE_TYPE_DYNAMIC;
      buffer_creation.size = sizeof( crude_gfx_mesh_draw_counts_gpu );
      buffer_creation.name = "mesh_count_late_sb";
      scene_renderer->mesh_task_indirect_count_late_sb[ i ] = crude_gfx_create_buffer( scene_renderer->renderer->gpu, &buffer_creation );
      
      buffer_creation = CRUDE_COMPOUNT_EMPTY( crude_gfx_buffer_creation );
      buffer_creation.type_flags = VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
      buffer_creation.usage = CRUDE_GFX_RESOURCE_USAGE_TYPE_IMMUTABLE;
      buffer_creation.size = sizeof( crude_gfx_debug_line_vertex_gpu ) * CRUDE_GFX_MAX_DEBUG_LINES * 2u; /* 2 vertices per line */
      buffer_creation.name = "debug_line_vertices";
      scene_renderer->debug_line_vertices_sb[ i ] = crude_gfx_create_buffer( scene_renderer->renderer->gpu, &buffer_creation );

      buffer_creation = CRUDE_COMPOUNT_EMPTY( crude_gfx_buffer_creation );
      buffer_creation.type_flags = VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
      buffer_creation.usage = CRUDE_GFX_RESOURCE_USAGE_TYPE_DYNAMIC;
      buffer_creation.size = sizeof( crude_gfx_debug_draw_command_gpu );
      buffer_creation.name = "debug_line_commands";
      scene_renderer->debug_line_commands_sb[ i ] = crude_gfx_create_buffer( scene_renderer->renderer->gpu, &buffer_creation );
    }
  }

  crude_gfx_imgui_pass_initialize( &scene_renderer->imgui_pass, scene_renderer );
  crude_gfx_gbuffer_early_pass_initialize( &scene_renderer->gbuffer_early_pass, scene_renderer );
  crude_gfx_gbuffer_late_pass_initialize( &scene_renderer->gbuffer_late_pass, scene_renderer );
  crude_gfx_depth_pyramid_pass_initialize( &scene_renderer->depth_pyramid_pass, scene_renderer );
  crude_gfx_culling_early_pass_initialize( &scene_renderer->culling_early_pass, scene_renderer );
  crude_gfx_culling_late_pass_initialize( &scene_renderer->culling_late_pass, scene_renderer );
  crude_gfx_debug_pass_initialize( &scene_renderer->debug_pass, scene_renderer );
  crude_gfx_light_pass_initialize( &scene_renderer->light_pass, scene_renderer );
}

void
crude_gfx_scene_renderer_deinitialize
(
  _In_ crude_gfx_scene_renderer                           *scene_renderer
)
{
  crude_gfx_imgui_pass_deinitialize( &scene_renderer->imgui_pass );
  crude_gfx_gbuffer_early_pass_deinitialize( &scene_renderer->gbuffer_early_pass );
  crude_gfx_gbuffer_late_pass_deinitialize( &scene_renderer->gbuffer_late_pass );
  crude_gfx_depth_pyramid_pass_deinitialize( &scene_renderer->depth_pyramid_pass );
  crude_gfx_culling_early_pass_deinitialize( &scene_renderer->culling_early_pass );
  crude_gfx_culling_late_pass_deinitialize( &scene_renderer->culling_late_pass );
  crude_gfx_debug_pass_deinitialize( &scene_renderer->debug_pass );
  crude_gfx_light_pass_deinitialize( &scene_renderer->light_pass );
  
  CRUDE_ARRAY_DEINITIALIZE( scene_renderer->meshes_instances );

  for ( uint32 i = 0; i < CRUDE_ARRAY_LENGTH( scene_renderer->images ); ++i )
  {
    crude_gfx_renderer_destroy_texture( scene_renderer->renderer, &scene_renderer->images[ i ] );
  }
  CRUDE_ARRAY_DEINITIALIZE( scene_renderer->images );

  for ( uint32 i = 0; i < CRUDE_ARRAY_LENGTH( scene_renderer->samplers ); ++i )
  {
    crude_gfx_renderer_destroy_sampler( scene_renderer->renderer, &scene_renderer->samplers[ i ] );
  }
  CRUDE_ARRAY_DEINITIALIZE( scene_renderer->samplers );
  
  for ( uint32 i = 0; i < CRUDE_ARRAY_LENGTH( scene_renderer->buffers ); ++i )
  {
    crude_gfx_renderer_destroy_buffer( scene_renderer->renderer, &scene_renderer->buffers[ i ] );
  }
  CRUDE_ARRAY_DEINITIALIZE( scene_renderer->buffers );

  crude_gfx_destroy_buffer( scene_renderer->renderer->gpu, scene_renderer->scene_cb );
  crude_gfx_destroy_buffer( scene_renderer->renderer->gpu, scene_renderer->meshlets_sb );
  crude_gfx_destroy_buffer( scene_renderer->renderer->gpu, scene_renderer->meshlets_vertices_sb );
  crude_gfx_destroy_buffer( scene_renderer->renderer->gpu, scene_renderer->meshlets_vertices_indices_sb );
  crude_gfx_destroy_buffer( scene_renderer->renderer->gpu, scene_renderer->meshlets_triangles_indices_sb );
  crude_gfx_destroy_buffer( scene_renderer->renderer->gpu, scene_renderer->meshes_draws_sb );
  crude_gfx_destroy_buffer( scene_renderer->renderer->gpu, scene_renderer->meshes_bounds_sb );
  crude_gfx_destroy_buffer( scene_renderer->renderer->gpu, scene_renderer->meshes_instances_draws_sb );

  for ( uint32 i = 0; i < CRUDE_GFX_MAX_SWAPCHAIN_IMAGES; ++i )
  {
    crude_gfx_destroy_buffer( scene_renderer->renderer->gpu, scene_renderer->debug_line_vertices_sb[ i ] );
    crude_gfx_destroy_buffer( scene_renderer->renderer->gpu, scene_renderer->debug_line_commands_sb[ i ] );
    crude_gfx_destroy_buffer( scene_renderer->renderer->gpu, scene_renderer->mesh_task_indirect_commands_early_sb[ i ] );
    crude_gfx_destroy_buffer( scene_renderer->renderer->gpu, scene_renderer->mesh_task_indirect_count_early_sb[ i ] );
    crude_gfx_destroy_buffer( scene_renderer->renderer->gpu, scene_renderer->mesh_task_indirect_commands_late_sb[ i ] );
    crude_gfx_destroy_buffer( scene_renderer->renderer->gpu, scene_renderer->mesh_task_indirect_count_late_sb[ i ] );
  }

  CRUDE_ARRAY_DEINITIALIZE( scene_renderer->meshes );
  CRUDE_ARRAY_DEINITIALIZE( scene_renderer->buffers );
  CRUDE_ARRAY_DEINITIALIZE( scene_renderer->meshes_instances );
  CRUDE_ARRAY_DEINITIALIZE( scene_renderer->meshlets_vertices );
  CRUDE_ARRAY_DEINITIALIZE( scene_renderer->meshlets_triangles_indices );
  CRUDE_ARRAY_DEINITIALIZE( scene_renderer->meshlets_vertices_indices );
  CRUDE_ARRAY_DEINITIALIZE( scene_renderer->meshlets );
}

void
crude_gfx_scene_renderer_submit_draw_task
(
  _In_ crude_gfx_scene_renderer                           *scene_renderer,
  _In_ bool                                                use_secondary
)
{
  crude_gfx_device                                        *gpu;
  crude_gfx_cmd_buffer                                    *primary_cmd;
  crude_gfx_map_buffer_parameters                          buffer_map;
 
  gpu = scene_renderer->renderer->gpu;

  primary_cmd = crude_gfx_get_primary_cmd( gpu, 0, true );
  
  /* Update scene constant buffer*/
  {
    crude_gfx_scene_constant_gpu                          *scene_constant;

    buffer_map = CRUDE_COMPOUNT_EMPTY( crude_gfx_map_buffer_parameters );
    buffer_map.buffer = scene_renderer->scene_cb;
    buffer_map.offset = 0;
    buffer_map.size = 0;
    scene_constant = CRUDE_CAST( crude_gfx_scene_constant_gpu*, crude_gfx_map_buffer( gpu, &buffer_map ) );
    if ( scene_constant )
    {
      *scene_constant = scene_renderer->scene_constant;
      crude_gfx_unmap_buffer( gpu, scene_renderer->scene_cb );
    }
  }
  /* Update meshes storage buffers*/
  {
    crude_gfx_mesh_draw_gpu                               *meshes_draws;
    crude_gfx_mesh_instance_draw_gpu                      *meshes_instances_draws;
    XMFLOAT4                                              *meshes_bounds;
    
    buffer_map = CRUDE_COMPOUNT_EMPTY( crude_gfx_map_buffer_parameters );
    buffer_map.buffer = scene_renderer->meshes_draws_sb;
    buffer_map.offset = 0;
    buffer_map.size = 0;
    meshes_draws = CRUDE_CAST( crude_gfx_mesh_draw_gpu*, crude_gfx_map_buffer( gpu, &buffer_map ) );
  
    buffer_map = CRUDE_COMPOUNT_EMPTY( crude_gfx_map_buffer_parameters );
    buffer_map.buffer = scene_renderer->meshes_bounds_sb;
    buffer_map.offset = 0;
    buffer_map.size = 0;
    meshes_bounds = CRUDE_CAST( XMFLOAT4*, crude_gfx_map_buffer( gpu, &buffer_map ) );
  
    if ( meshes_draws && meshes_bounds )
    {
      for ( uint32 i = 0; i < CRUDE_ARRAY_LENGTH( scene_renderer->meshes ); ++i )
      {
        crude_gfx_mesh_cpu_to_mesh_draw_gpu( &scene_renderer->meshes[ i ], &meshes_draws[ i ] );
        meshes_bounds[ i ] = scene_renderer->meshes[ i ].bounding_sphere;
      }
    }

    if ( meshes_draws )
    {
      crude_gfx_unmap_buffer( gpu, scene_renderer->meshes_draws_sb );
    }
    if ( meshes_bounds )
    {
      crude_gfx_unmap_buffer( gpu, scene_renderer->meshes_bounds_sb );
    }
  
    buffer_map = CRUDE_COMPOUNT_EMPTY( crude_gfx_map_buffer_parameters );
    buffer_map.buffer = scene_renderer->meshes_instances_draws_sb;
    buffer_map.offset = 0;
    buffer_map.size = 0;
    meshes_instances_draws = CRUDE_CAST( crude_gfx_mesh_instance_draw_gpu*, crude_gfx_map_buffer( gpu, &buffer_map ) );
  
    if ( meshes_instances_draws )
    {
      scene_renderer->total_meshes_instances_count = 0u;
      for ( uint32 i = 0; i < CRUDE_ARRAY_LENGTH( scene_renderer->meshes_instances ); ++i )
      {
        crude_gfx_mesh_cpu *mesh_cpu = scene_renderer->meshes_instances[ i ].mesh;
        
        bool mesh_textures_ready = ( CRUDE_RESOURCE_HANDLE_IS_INVALID( mesh_cpu->albedo_texture_handle ) || crude_gfx_texture_ready( gpu, mesh_cpu->albedo_texture_handle ) )
          && ( CRUDE_RESOURCE_HANDLE_IS_INVALID( mesh_cpu->normal_texture_handle ) || crude_gfx_texture_ready( gpu, mesh_cpu->normal_texture_handle ) )
          && ( CRUDE_RESOURCE_HANDLE_IS_INVALID( mesh_cpu->occlusion_texture_handle ) || crude_gfx_texture_ready( gpu, mesh_cpu->occlusion_texture_handle ) )
          && ( CRUDE_RESOURCE_HANDLE_IS_INVALID( mesh_cpu->roughness_texture_handle ) || crude_gfx_texture_ready( gpu, mesh_cpu->roughness_texture_handle ) );

        if ( mesh_textures_ready )
        {
          crude_transform const                             *transform;
          XMMATRIX                                           model_to_world;

          transform = CRUDE_ENTITY_GET_IMMUTABLE_COMPONENT( scene_renderer->meshes_instances[ i ].node, crude_transform );
          model_to_world = crude_transform_node_to_world( scene_renderer->meshes_instances[ i ].node, transform );

          XMStoreFloat4x4( &meshes_instances_draws[ scene_renderer->total_meshes_instances_count ].model_to_world, model_to_world );
          meshes_instances_draws[ scene_renderer->total_meshes_instances_count ].mesh_draw_index = scene_renderer->meshes_instances[ i ].mesh->gpu_mesh_index;

          ++scene_renderer->total_meshes_instances_count;
        }
      }
    }

    if ( meshes_instances_draws )
    {
      crude_gfx_unmap_buffer( gpu, scene_renderer->meshes_instances_draws_sb );
    }
  }
  
  /* Update meshlets counes storage buffers*/
  {
    crude_gfx_mesh_draw_counts_gpu                        *mesh_draw_counts_early;
    crude_gfx_mesh_draw_counts_gpu                        *mesh_draw_counts_late;

    buffer_map = CRUDE_COMPOUNT_EMPTY( crude_gfx_map_buffer_parameters );
    buffer_map.buffer = scene_renderer->mesh_task_indirect_count_early_sb[ gpu->current_frame ];
    buffer_map.offset = 0;
    buffer_map.size = sizeof( crude_gfx_mesh_draw_counts_gpu );
    mesh_draw_counts_early = CRUDE_CAST( crude_gfx_mesh_draw_counts_gpu*, crude_gfx_map_buffer( gpu, &buffer_map ) );
    if ( mesh_draw_counts_early )
    {
      *mesh_draw_counts_early = CRUDE_COMPOUNT_EMPTY( crude_gfx_mesh_draw_counts_gpu );
      mesh_draw_counts_early->total_count = scene_renderer->total_meshes_instances_count;
      mesh_draw_counts_early->depth_pyramid_texture_index = scene_renderer->depth_pyramid_pass.depth_pyramid_texture_handle.index;
      mesh_draw_counts_early->occlusion_culling_late_flag = false;
      crude_gfx_unmap_buffer( gpu, scene_renderer->mesh_task_indirect_count_early_sb[ gpu->current_frame ] );
    }
  
    buffer_map = CRUDE_COMPOUNT_EMPTY( crude_gfx_map_buffer_parameters );
    buffer_map.buffer = scene_renderer->mesh_task_indirect_count_late_sb[ gpu->current_frame ];
    buffer_map.offset = 0;
    buffer_map.size = sizeof( crude_gfx_mesh_draw_counts_gpu );
    mesh_draw_counts_late = CRUDE_CAST( crude_gfx_mesh_draw_counts_gpu*, crude_gfx_map_buffer( gpu, &buffer_map ) );
    if ( mesh_draw_counts_late )
    {
      *mesh_draw_counts_late = CRUDE_COMPOUNT_EMPTY( crude_gfx_mesh_draw_counts_gpu );
      mesh_draw_counts_late->total_count = scene_renderer->total_meshes_instances_count;
      mesh_draw_counts_late->depth_pyramid_texture_index = scene_renderer->depth_pyramid_pass.depth_pyramid_texture_handle.index;
      mesh_draw_counts_late->occlusion_culling_late_flag = true;
      crude_gfx_unmap_buffer( gpu, scene_renderer->mesh_task_indirect_count_late_sb[ gpu->current_frame ] );
    }
  }
  
  /* Update debug draw commands */
  {
    crude_gfx_debug_draw_command_gpu                      *debug_draw_command;
  
    buffer_map = CRUDE_COMPOUNT_EMPTY( crude_gfx_map_buffer_parameters );
    buffer_map.buffer = scene_renderer->debug_line_commands_sb[ gpu->current_frame ];
    buffer_map.offset = 0;
    buffer_map.size = sizeof( crude_gfx_debug_draw_command_gpu );
    debug_draw_command = CRUDE_CAST( crude_gfx_debug_draw_command_gpu*, crude_gfx_map_buffer( gpu, &buffer_map ) );
    if ( debug_draw_command )
    {
      *debug_draw_command = CRUDE_COMPOUNT_EMPTY( crude_gfx_debug_draw_command_gpu );
      debug_draw_command->draw_indirect_2dline.instanceCount = 1u;
      debug_draw_command->draw_indirect_3dline.instanceCount = 1u;
      crude_gfx_unmap_buffer( gpu, scene_renderer->debug_line_commands_sb[ gpu->current_frame ] );
    }
  }
  
  ///* Update lights buffers */
  //{
  //  crude_gfx_sorted_light                                *sorted_lights;
  //  crude_gfx_light_gpu                                   *lights_gpu;
  //  uint32                                                *bin_range_per_light;
  //  uint32                                                *lights_indices_gpu;
  //  uint32                                                *lights_luts_gpu;
  //  uint32                                                *light_tiles_gpu;
  //  XMMATRIX                                               world_to_view;
  //  float32                                                zfar, znear, bin_size;
  //
  //  // !TODO tmp allocator
  //  CRUDE_ARRAY_INITIALIZE_WITH_LENGTH( sorted_lights, CRUDE_ARRAY_LENGTH( scene_renderer->lights ), scene_renderer->allocator_container );
  //
  //  /* Sort lights based on Z */
  //  world_to_view = XMLoadFloat4x4( &scene_renderer->scene_constant.camera.world_to_view );
  //  zfar = scene_renderer->scene_constant.camera.zfar;
  //  znear = scene_renderer->scene_constant.camera.znear;
  //  for ( uint32 i = 0; i < CRUDE_ARRAY_LENGTH( scene_renderer->lights ); ++i )
  //  {
  //    crude_gfx_sorted_light                              *sorted_light;
  //    crude_gfx_light                                     *light;
  //    XMVECTOR                                             world_pos, view_pos, view_pos_min, view_pos_max;
  //
  //    light = &scene_renderer->lights[ i ];
  //
  //    world_pos = XMVectorSet( light->world_position.x, light->world_position.y, light->world_position.z, 1.0f );
  //
  //    view_pos = XMVector4Transform( world_pos, world_to_view );
  //    view_pos_min = XMVectorAdd( view_pos, XMVectorSet( 0, 0, -light->radius, 0 ) );
  //    view_pos_max = XMVectorAdd( view_pos, XMVectorSet( 0, 0, light->radius, 0 ) );
  //
  //    sorted_light = &sorted_lights[ i ];
  //    sorted_light->light_index = i;
  //    sorted_light->projected_z = ( ( XMVectorGetZ( view_pos ) - znear ) / ( zfar - znear ) );
  //    sorted_light->projected_z_min = ( ( XMVectorGetZ( view_pos_min ) - znear ) / ( zfar - znear ) );
  //    sorted_light->projected_z_max = ( ( XMVectorGetZ( view_pos_max ) - znear ) / ( zfar - znear ) );
  //  }
  //
  //  qsort( sorted_lights, CRUDE_ARRAY_LENGTH( scene_renderer->lights ), sizeof( crude_gfx_sorted_light ), sorting_light_ );
  //  
  //  /* Upload light to gpu */
  //  buffer_map = CRUDE_COMPOUNT_EMPTY( crude_gfx_map_buffer_parameters );
  //  buffer_map.buffer = scene_renderer->lights_sb;
  //  buffer_map.offset = 0;
  //  buffer_map.size = sizeof( crude_gfx_light_gpu );
  //  lights_gpu = CRUDE_CAST( crude_gfx_light_gpu*, crude_gfx_map_buffer( gpu, &buffer_map ) );
  //  if ( lights_gpu )
  //  {
  //    for ( uint32 i = 0; i < CRUDE_ARRAY_LENGTH( scene_renderer->lights ); ++i )
  //    {
  //      crude_gfx_light                                     *light;
  //      crude_gfx_light_gpu                                 *light_gpu;
  //      
  //      light = &scene_renderer->lights[ i ];
  //      light_gpu = &lights_gpu[ i ];
  //
  //      lights_gpu->world_position = light->world_position;
  //      lights_gpu->radius = light->radius;
  //      lights_gpu->color = light->color;
  //      lights_gpu->intensity = light->intensity;
  //    }
  //    crude_gfx_unmap_buffer( gpu, scene_renderer->lights_sb );
  //  }
  //  
  //  /* Calculate lights clusters */
  //  bin_size = 1.f / CRUDE_GFX_LIGHT_Z_BINS;
  //
  //  CRUDE_ARRAY_INITIALIZE_WITH_LENGTH( bin_range_per_light, CRUDE_ARRAY_LENGTH( scene_renderer->lights ), scene_renderer->allocator_container );
  //  
  //  for ( uint32 i = 0; i < CRUDE_ARRAY_LENGTH( scene_renderer->lights ); ++i )
  //  {
  //    crude_gfx_sorted_light const                        *light;
  //    uint32                                               min_bin, max_bin;
  //
  //    light = &sorted_lights[ i ];
  //
  //    if ( light->projected_z_min < 0.f && light->projected_z_max < 0.f )
  //    {
  //      bin_range_per_light[ i ] = UINT32_MAX;
  //      continue;
  //    }
  //    min_bin = CRUDE_MAX( 0u, CRUDE_FLOOR( light->projected_z_min * CRUDE_GFX_LIGHT_Z_BINS ) );
  //    max_bin = CRUDE_MAX( 0u, CRUDE_CEIL( light->projected_z_max * CRUDE_GFX_LIGHT_Z_BINS ) );
  //    bin_range_per_light[ i ] = ( min_bin & 0xffff ) | ( ( max_bin & 0xffff ) << 16 );
  //  }
  //
  //  for ( uint32 bin = 0; bin < CRUDE_GFX_LIGHT_Z_BINS; ++bin )
  //  {
  //    float32                                              bin_min, bin_max;
  //    uint32                                               min_light_id, max_light_id;
  //
  //    min_light_id = CRUDE_GFX_LIGHTS_MAX_COUNT + 1;
  //    max_light_id = 0;
  //
  //    bin_min = bin_size * bin;
  //    bin_max = bin_min + bin_size;
  //
  //    for ( uint32 i = 0; i < CRUDE_ARRAY_LENGTH( scene_renderer->lights ); ++i )
  //    {
  //      crude_gfx_sorted_light const                      *light;
  //      uint32                                             light_bins, min_bin, max_bin;
  //
  //      light = &sorted_lights[ i ];
  //      light_bins = bin_range_per_light[ i ];
  //
  //      if ( light_bins == UINT32_MAX )
  //      {
  //        continue;
  //      }
  //
  //      min_bin = light_bins & 0xffff;
  //      max_bin = light_bins >> 16;
  //
  //      if ( bin >= min_bin && bin <= max_bin )
  //      {
  //        if ( i < min_light_id )
  //        {
  //          min_light_id = i;
  //        }
  //        if ( i > max_light_id )
  //        {
  //          max_light_id = i;
  //        }
  //      }
  //    }
  //
  //    scene_renderer->lights_lut[ bin ] = min_light_id | ( max_light_id << 16 );
  //  }
  //  
  //  /* Upload light indices */
  //  buffer_map = CRUDE_COMPOUNT_EMPTY( crude_gfx_map_buffer_parameters );
  //  buffer_map.buffer = scene_renderer->lights_indices_sb[ gpu->current_frame ];
  //  buffer_map.offset = 0;
  //  buffer_map.size = sizeof( uint32 );
  //  lights_indices_gpu = CRUDE_CAST( uint32*, crude_gfx_map_buffer( gpu, &buffer_map ) );
  //  if ( lights_indices_gpu )
  //  {
  //    for ( uint32 i = 0; i < CRUDE_ARRAY_LENGTH( scene_renderer->lights ); ++i )
  //    {
  //      lights_indices_gpu[ i ] = sorted_lights[ i ].light_index;
  //    }
  //    crude_gfx_unmap_buffer( gpu, scene_renderer->lights_indices_sb[ gpu->current_frame ] );
  //  }
  //
  //  /* Upload lights LUT */
  //  buffer_map = CRUDE_COMPOUNT_EMPTY( crude_gfx_map_buffer_parameters );
  //  buffer_map.buffer = scene_renderer->lights_lut_sb[ gpu->current_frame ];
  //  buffer_map.offset = 0;
  //  buffer_map.size = sizeof( uint32 );
  //  lights_luts_gpu = CRUDE_CAST( uint32*, crude_gfx_map_buffer( gpu, &buffer_map ) );
  //  if ( lights_luts_gpu )
  //  {
  //    memcpy( lights_luts_gpu, scene_renderer->lights_lut, CRUDE_ARRAY_LENGTH( scene_renderer->lights_lut )* sizeof( scene_renderer->lights_lut[ 0 ] ) );
  //    crude_gfx_unmap_buffer( gpu, scene_renderer->lights_lut_sb[ gpu->current_frame ] );
  //  }
  //
  //  const uint32 tile_x_count = scene_renderer->scene_constant.resolution.x / CRUDE_GFX_LIGHT_TILE_SIZE;
  //  const uint32 tile_y_count = scene_renderer->scene_constant.resolution.y / CRUDE_GFX_LIGHT_TILE_SIZE;
  //  const uint32 tiles_entry_count = tile_x_count * tile_y_count * CRUDE_GFX_LIGHT_WORDS_COUNT;
  //  const uint32 buffer_size = tiles_entry_count * sizeof( uint32 );
  //
  //  uint32 *light_tiles_bits;
  //  CRUDE_ARRAY_INITIALIZE_WITH_LENGTH( light_tiles_bits, tiles_entry_count, scene_renderer->allocator_container );
  //  memset( light_tiles_bits, 0, buffer_size );
  //
  //  znear = scene_renderer->scene_constant.camera.znear;
  //  float32 tile_size_inv = 1.0f / CRUDE_GFX_LIGHT_TILE_SIZE;
  //
  //  uint32 tile_stride = tile_x_count * CRUDE_GFX_LIGHT_WORDS_COUNT;
  //
  //  for ( uint32 i = 0; i < CRUDE_ARRAY_LENGTH( scene_renderer->lights ); ++i )
  //  {
  //    const uint32 light_index = sorted_lights[ i ].light_index;
  //    crude_gfx_light *light = &scene_renderer->lights[ light_index ];
  //
  //    XMVECTOR pos = XMVectorSet( light->world_position.x, light->world_position.y, light->world_position.z, 1.0f );
  //    float32 radius = light->radius;
  //
  //    XMVECTOR view_space_pos = XMVector4Transform( pos, XMLoadFloat4x4( &scene_renderer->scene_constant.camera.world_to_view ) );
  //    bool camera_visible = -XMVectorGetZ( view_space_pos ) - radius < znear;
  //
  //    if ( !camera_visible )
  //    {
  //      continue;
  //    }
  //
  //    XMVECTOR cx = XMVectorSet( XMVectorGetX( view_space_pos ), XMVectorGetZ( view_space_pos ), 0, 0 );
  //    const float32 tx_squared = XMVectorGetX( XMVector2Dot( cx, cx ) ) - ( radius * radius );
  //    const bool tx_camera_inside = tx_squared <= 0;
  //    XMVECTOR vx = XMVectorSet( sqrtf( tx_squared ), radius, 0, 0 );
  //    XMMATRIX xtransf_min = XMMatrixSet( XMVectorGetX( vx ), XMVectorGetY( vx ), 0, 0, -XMVectorGetY( vx ), XMVectorGetX( vx ), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 );
  //    XMVECTOR minx = XMVector2Transform( cx, xtransf_min );
  //    XMMATRIX xtransf_max = XMMatrixSet( XMVectorGetX( vx ), -XMVectorGetY( vx ), 0, 0, XMVectorGetY( vx ), XMVectorGetX( vx ), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 );
  //    XMVECTOR maxx = XMVector2Transform( cx, xtransf_max );
  //
  //    XMVECTOR cy = XMVectorSet( -XMVectorGetY( view_space_pos ), XMVectorGetZ( view_space_pos ), 0, 0 );
  //    const float32 ty_squared = XMVectorGetX( XMVector2Dot( cy, cy ) ) - ( radius * radius );
  //    const bool ty_camera_inside = ty_squared <= 0;//
  //    XMVECTOR vy{ sqrtf( ty_squared ), radius };
  //    XMMATRIX ytransf_min = XMMatrixSet( XMVectorGetX( vy ), XMVectorGetY( vy ), 0, 0, -XMVectorGetY( vy ), XMVectorGetX( vy ), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 );
  //    XMVECTOR miny = XMVector2Transform( cy, ytransf_min );
  //    XMMATRIX ytransf_max = XMMatrixSet( XMVectorGetX( vy ), XMVectorGetY( vy ), 0, 0, -XMVectorGetY( vy ), XMVectorGetX( vy ), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 );
  //    XMVECTOR maxy = XMVector2Transform( cy, ytransf_max );
  //
  //    XMVECTOR aabb = XMVectorSet( 
  //      XMVectorGetX( minx ) / XMVectorGetY( minx ) * scene_renderer->scene_constant.camera.view_to_clip._11,
  //      XMVectorGetX( miny ) / XMVectorGetY( miny ) * scene_renderer->scene_constant.camera.view_to_clip._22,
  //      XMVectorGetX( maxx ) / XMVectorGetY( maxx ) * scene_renderer->scene_constant.camera.view_to_clip._11,
  //      XMVectorGetX( maxy ) / XMVectorGetY( maxy ) * scene_renderer->scene_constant.camera.view_to_clip._22
  //    );
  //
  //    XMVECTOR left, right, top, bottom;
  //    get_bounds_for_axis_( XMVectorSet( 1, 0, 0, 0 ), view_space_pos, radius, znear, &left, &right );
  //    get_bounds_for_axis_( XMVectorSet( 0, 1, 0, 0 ), view_space_pos, radius, znear, &top, &bottom );
  //    
  //    XMMATRIX view_to_clip = XMLoadFloat4x4( &scene_renderer->scene_constant.camera.view_to_clip );
  //    left = project_( view_to_clip, left );
  //    right = project_( view_to_clip, right );
  //    top = project_( view_to_clip, top );
  //    bottom = project_( view_to_clip, bottom );
  //    
  //    aabb = XMVectorSet( XMVectorGetX( right ), -XMVectorGetY( top ), XMVectorGetX( left ), -XMVectorGetY( bottom ) );
  //
  //    XMVECTOR aabb_min = XMVectorSet( FLT_MAX, FLT_MAX, FLT_MAX, 0 );
  //    XMVECTOR aabb_max = XMVectorSet( -FLT_MAX, -FLT_MAX, -FLT_MAX, 0 );
  //
  //    for ( uint32 c = 0; c < 8; ++c )
  //    {
  //      XMVECTOR corner = XMVectorSet( ( c % 2 ) ? 1.f : -1.f, ( c & 2 ) ? 1.f : -1.f, ( c & 4 ) ? 1.f : -1.f, 1 );
  //      corner = XMVectorScale( corner, radius );
  //      corner = XMVectorAdd( corner, pos );
  //
  //      XMVECTOR corner_vs = XMVector4Transform( corner, XMLoadFloat4x4( &scene_renderer->scene_constant.camera.world_to_view ) );
  //      corner_vs = XMVectorSetZ( corner_vs, CRUDE_MAX( znear, XMVectorGetZ( corner_vs ) ) );
  //      XMVECTOR corner_ndc = XMVector4Transform( corner_vs, XMLoadFloat4x4( &scene_renderer->scene_constant.camera.view_to_clip ) );
  //      corner_ndc = XMVectorScale( corner_ndc, 1.f / XMVectorGetW( corner_ndc ) );
  //
  //      aabb_min = XMVectorSet(
  //        CRUDE_MIN( XMVectorGetX( aabb_min ), XMVectorGetX( corner_ndc ) ),
  //        CRUDE_MIN( XMVectorGetY( aabb_min ), XMVectorGetY( corner_ndc ) ), 0, 0
  //      );
  //      
  //      aabb_max = XMVectorSet(
  //        CRUDE_MIN( XMVectorGetX( aabb_max ), XMVectorGetX( aabb_max ) ),
  //        CRUDE_MIN( XMVectorGetY( aabb_max ), XMVectorGetY( aabb_max ) ), 0, 0
  //      );
  //    }
  //
  //    aabb = XMVectorSet( XMVectorGetX( aabb_min ), -1.f * XMVectorGetY( aabb_max ), XMVectorGetX( aabb_max ), -1.f * XMVectorGetY( aabb_min ) );
  //
  //    const float32 position_len = XMVectorGetX( XMVector3Length( view_space_pos ) );
  //    const bool camera_inside = ( position_len - radius ) < znear;
  //
  //    if ( camera_inside )
  //    {
  //      aabb = { -1,-1, 1, 1 };
  //    }
  //
  //    XMVECTOR aabb_screen = XMVectorSet(
  //      ( XMVectorGetX( aabb ) * 0.5f + 0.5f ) * ( gpu->vk_swapchain_width - 1 ),
  //      ( XMVectorGetY( aabb ) * 0.5f + 0.5f ) * ( gpu->vk_swapchain_height - 1 ),
  //      ( XMVectorGetZ( aabb ) * 0.5f + 0.5f ) * ( gpu->vk_swapchain_width - 1 ),
  //      ( XMVectorGetW( aabb ) * 0.5f + 0.5f ) * ( gpu->vk_swapchain_height - 1 )
  //    );
  //
  //    float32 width = XMVectorGetZ( aabb_screen ) - XMVectorGetX( aabb_screen );
  //    float32 height = XMVectorGetW( aabb_screen ) - XMVectorGetY( aabb_screen );
  //
  //    if ( width < 0.0001f || height < 0.0001f )
  //    {
  //      continue;
  //    }
  //
  //    float min_x = XMVectorGetX( aabb_screen );
  //    float min_y = XMVectorGetY( aabb_screen );
  //
  //    float max_x = min_x + width;
  //    float max_y = min_y + height;
  //
  //    if ( min_x > gpu->vk_swapchain_width || min_y > gpu->vk_swapchain_height )
  //    {
  //      continue;
  //    }
  //
  //    if ( max_x < 0.0f || max_y < 0.0f )
  //    {
  //      continue;
  //    }
  //
  //    min_x = CRUDE_MAX( min_x, 0.0f );
  //    min_y = CRUDE_MAX( min_y, 0.0f );
  //
  //    max_x = CRUDE_MIN( max_x, gpu->vk_swapchain_width );
  //    max_y = CRUDE_MIN( max_y, gpu->vk_swapchain_height );
  //
  //    uint32 first_tile_x = min_x * tile_size_inv;
  //    uint32 last_tile_x = CRUDE_MIN( tile_x_count - 1, max_x * tile_size_inv );
  //
  //    uint32 first_tile_y = min_y * tile_size_inv;
  //    uint32 last_tile_y = CRUDE_MIN( tile_y_count - 1, max_y * tile_size_inv );
  //
  //    for ( uint32 y = first_tile_y; y <= last_tile_y; ++y )
  //    {
  //      for ( uint32 x = first_tile_x; x <= last_tile_x; ++x )
  //      {
  //        uint32 array_index = y * tile_stride + x;
  //        uint32 word_index = i / 32;
  //        uint32 bit_index = i % 32;
  //
  //        light_tiles_bits[ array_index + word_index ] |= ( 1 << bit_index );
  //      }
  //    }
  //  }
  //  
  //  buffer_map = CRUDE_COMPOUNT_EMPTY( crude_gfx_map_buffer_parameters );
  //  buffer_map.buffer = scene_renderer->lights_tiles_sb[ gpu->current_frame ];
  //  buffer_map.offset = 0;
  //  buffer_map.size = sizeof( uint32 );
  //  light_tiles_gpu = CRUDE_CAST( uint32*, crude_gfx_map_buffer( gpu, &buffer_map ) );
  //  if ( light_tiles_gpu )
  //  {
  //    memcpy( light_tiles_gpu, light_tiles_bits, CRUDE_ARRAY_LENGTH( light_tiles_bits ) * sizeof( uint32 ) );
  //    crude_gfx_unmap_buffer( gpu, scene_renderer->lights_tiles_sb[ gpu->current_frame ] );
  //  }
  //}
  ////
  crude_gfx_render_graph_render( scene_renderer->render_graph, primary_cmd );
  crude_gfx_queue_cmd( primary_cmd );
}

void
crude_gfx_scene_renderer_register_passes
(
  _In_ crude_gfx_scene_renderer                           *scene_renderer,
  _In_ crude_gfx_render_graph                             *render_graph
)
{
  scene_renderer->render_graph = render_graph;

  crude_gfx_render_graph_builder_register_render_pass( render_graph->builder, "gbuffer_early_pass", crude_gfx_gbuffer_early_pass_pack( &scene_renderer->gbuffer_early_pass ) );
  crude_gfx_render_graph_builder_register_render_pass( render_graph->builder, "gbuffer_late_pass", crude_gfx_gbuffer_late_pass_pack( &scene_renderer->gbuffer_late_pass ) );
  crude_gfx_render_graph_builder_register_render_pass( render_graph->builder, "imgui_pass", crude_gfx_imgui_pass_pack( &scene_renderer->imgui_pass ) );
  crude_gfx_render_graph_builder_register_render_pass( render_graph->builder, "depth_pyramid_pass", crude_gfx_depth_pyramid_pass_pack( &scene_renderer->depth_pyramid_pass ) );
  crude_gfx_render_graph_builder_register_render_pass( render_graph->builder, "culling_early_pass", crude_gfx_culling_early_pass_pack( &scene_renderer->culling_early_pass ) );
  crude_gfx_render_graph_builder_register_render_pass( render_graph->builder, "culling_late_pass", crude_gfx_culling_late_pass_pack( &scene_renderer->culling_late_pass ) );
  crude_gfx_render_graph_builder_register_render_pass( render_graph->builder, "debug_pass", crude_gfx_debug_pass_pack( &scene_renderer->debug_pass ) );
  crude_gfx_render_graph_builder_register_render_pass( render_graph->builder, "light_pass", crude_gfx_light_pass_pack( &scene_renderer->light_pass ) );

  crude_gfx_depth_pyramid_pass_on_render_graph_registered( &scene_renderer->depth_pyramid_pass );
  crude_gfx_light_pass_on_render_graph_registered( &scene_renderer->light_pass );
}

void
crude_gfx_scene_renderer_register_scene
(
  _In_ crude_gfx_scene_renderer                           *scene_renderer,
  _In_ crude_scene                                        *scene
)
{
}

/**
 *
 * Renderer Scene Utils
 * 
 */
void
crude_gfx_scene_renderer_add_debug_resources_to_descriptor_set_creation
(
  _In_ crude_gfx_descriptor_set_creation                  *creation,
  _In_ crude_gfx_scene_renderer                           *scene_renderer,
  _In_ uint32                                              frame
)
{
  crude_gfx_descriptor_set_creation_add_buffer( creation, scene_renderer->debug_line_vertices_sb[ frame ], 20u );
  crude_gfx_descriptor_set_creation_add_buffer( creation, scene_renderer->debug_line_commands_sb[ frame ], 21u );
}

void
crude_gfx_scene_renderer_add_meshlet_resources_to_descriptor_set_creation
(
  _In_ crude_gfx_descriptor_set_creation                  *creation,
  _In_ crude_gfx_scene_renderer                           *scene_renderer,
  _In_ uint32                                              frame
)
{
  crude_gfx_descriptor_set_creation_add_buffer( creation, scene_renderer->meshlets_sb, 5u );
  crude_gfx_descriptor_set_creation_add_buffer( creation, scene_renderer->meshlets_vertices_sb, 6u );
  crude_gfx_descriptor_set_creation_add_buffer( creation, scene_renderer->meshlets_triangles_indices_sb, 7u );
  crude_gfx_descriptor_set_creation_add_buffer( creation, scene_renderer->meshlets_vertices_indices_sb, 8u );
}

void
crude_gfx_scene_renderer_add_scene_resources_to_descriptor_set_creation
(
  _In_ crude_gfx_descriptor_set_creation                  *creation,
  _In_ crude_gfx_scene_renderer                           *scene_renderer,
  _In_ uint32                                              frame
)
{
  crude_gfx_descriptor_set_creation_add_buffer( creation, scene_renderer->scene_cb, 0u );
}

void
crude_gfx_scene_renderer_add_mesh_resources_to_descriptor_set_creation
(
  _In_ crude_gfx_descriptor_set_creation                  *creation,
  _In_ crude_gfx_scene_renderer                           *scene_renderer,
  _In_ uint32                                              frame
)
{
  crude_gfx_descriptor_set_creation_add_buffer( creation, scene_renderer->meshes_draws_sb, 1u );
  crude_gfx_descriptor_set_creation_add_buffer( creation, scene_renderer->meshes_instances_draws_sb, 2u );
  crude_gfx_descriptor_set_creation_add_buffer( creation, scene_renderer->meshes_bounds_sb, 3u );
}

/**
 *
 * TODO
 * 
 */
void
scene_renderer_prepare_node_draws_
(
  _In_ crude_gfx_scene_renderer                           *scene_renderer,
  _In_ crude_entity                                        node,
  _In_ crude_stack_allocator                              *temporary_allocator
)
{
  ecs_iter_t it = ecs_children( node.world, node.handle );
  while ( ecs_children_next( &it ) )
  {
    for ( size_t i = 0; i < it.count; ++i )
    {
      crude_entity child = CRUDE_COMPOUNT( crude_entity, { .handle = it.entities[ i ], .world = node.world } );
      if ( CRUDE_ENTITY_HAS_COMPONENT( child, crude_gltf ) )
      {
        crude_gltf const* child_gltf = CRUDE_ENTITY_GET_IMMUTABLE_COMPONENT( child, crude_gltf );
        crude_scene_renderer_upload_gltf( scene_renderer, child_gltf->path, child, temporary_allocator );
      }

      scene_renderer_prepare_node_draws_( scene_renderer, child, temporary_allocator );
    }
  }
}