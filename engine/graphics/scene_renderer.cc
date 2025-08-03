#include <TaskScheduler_c.h>

#include <core/profiler.h>
#include <core/array.h>
#include <core/file.h>
#include <core/hash_map.h>
#include <scene/scene_components.h>
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
  crude_gfx_mesh_draw_gpu                                 *meshes_draws;
  crude_gfx_mesh_instance_draw_gpu                        *meshes_instances_draws;
  XMFLOAT4                                                *meshes_bounds;
  crude_gfx_mesh_draw_counts_gpu                          *mesh_draw_counts_early;
  crude_gfx_mesh_draw_counts_gpu                          *mesh_draw_counts_late;
  crude_gfx_debug_draw_command_gpu                        *debug_draw_command;
  crude_gfx_map_buffer_parameters                          buffer_map;
 
  gpu = scene_renderer->renderer->gpu;

  primary_cmd = crude_gfx_get_primary_cmd( gpu, 0, true );
  
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