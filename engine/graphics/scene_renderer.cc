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
 * Scene Draw Task Constants
 * 
 */
#define _PARALLEL_RECORDINGS                               ( 4 )

/**
 *
 * Renderer Scene Draw Task Declaration
 * 
 */
typedef struct secondary_draw_task_container
{
  _In_ crude_gfx_scene_renderer_geometry_pass             *pass;
  crude_gfx_cmd_buffer                                    *primary_cmd;
  crude_gfx_cmd_buffer                                    *secondary_cmd;
  uint32                                                   start_mesh_draw_index;
  uint32                                                   end_mesh_draw_index;
  uint32                                                   thread_id;
} secondary_draw_task_container;

static void
secondary_draw_task_
(
  _In_ uint32_t                                            start,
  _In_ uint32_t                                            end,
  _In_ uint32_t                                            thread_num,
  _In_ void                                               *ctx
);

/**
 *
 * Renderer Scene Draw Declaration
 * 
 */
static void
draw_mesh_
(
  _In_ crude_gfx_cmd_buffer                               *cmd,
  _In_ crude_gfx_mesh                                     *mesh
);

static void
draw_scene_
(
  _In_ crude_gfx_cmd_buffer                               *cmd,
  _In_ crude_gfx_scene_renderer_geometry_pass             *pass
);

/**
 *
 * Common Renderer Scene Structes
 * 
 */
bool
crude_gfx_mesh_is_transparent
(
  _In_ crude_gfx_mesh                                     *mesh
)
{
  return ( mesh->flags & ( CRUDE_GFX_DRAW_FLAGS_ALPHA_MASK | CRUDE_GFX_DRAW_FLAGS_TRANSPARENT_MASK ) ) != 0;
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
);

/**
 *
 * Renderer Scene Geometry Pass
 * 
 */
static void
crude_gfx_render_graph_pass_container_pre_render_empry
(
  _In_ void                                               *ctx,
  _In_ crude_gfx_cmd_buffer                               *primary_cmd
)
{
}

static void
crude_gfx_render_graph_pass_container_on_resize_empty
(
  _In_ void                                               *ctx,
  _In_ crude_gfx_device                                   *gpu,
  _In_ uint32                                              new_width,
  _In_ uint32                                              new_height
)
{
}

void
crude_gfx_scene_renderer_geometry_pass_render
(
  _In_ crude_gfx_scene_renderer_geometry_pass             *pass,
  _In_ crude_gfx_cmd_buffer                               *primary_cmd
)
{
  bool use_secondary = false;
  if ( pass->scene->use_meshlets )
  {
    {
    crude_gfx_map_buffer_parameters cb_map = CRUDE_COMPOUNT_EMPTY( crude_gfx_map_buffer_parameters );
    cb_map.buffer = pass->scene->mesh_task_indirect_commands_sb[ pass->scene->renderer->gpu->current_frame ];
    cb_map.offset = 0;
    cb_map.size = sizeof( crude_gfx_mesh_draw_command );
    crude_gfx_mesh_draw_command *draw_data = CRUDE_CAST( crude_gfx_mesh_draw_command*, crude_gfx_map_buffer( pass->scene->renderer->gpu, &cb_map ) );
    if ( draw_data )
    {
      draw_data->indirect_meshlet.groupCountX = 1;
      draw_data->indirect_meshlet.groupCountY = 1;
      draw_data->indirect_meshlet.groupCountZ = 1;
      crude_gfx_unmap_buffer( pass->scene->renderer->gpu, cb_map.buffer );
    }
    }
    
    {
    crude_gfx_map_buffer_parameters cb_map = CRUDE_COMPOUNT_EMPTY( crude_gfx_map_buffer_parameters );
    cb_map.buffer = pass->scene->mesh_task_indirect_count_sb[ pass->scene->renderer->gpu->current_frame ];
    cb_map.offset = 0;
    cb_map.size = sizeof( crude_gfx_mesh_draw_counts );
    crude_gfx_mesh_draw_counts *draw_data = CRUDE_CAST( crude_gfx_mesh_draw_counts*, crude_gfx_map_buffer( pass->scene->renderer->gpu, &cb_map ) );
    if ( draw_data )
    {
      draw_data->opaque_mesh_visible_count = 1;
      crude_gfx_unmap_buffer( pass->scene->renderer->gpu, cb_map.buffer );
    }
    }

    crude_gfx_renderer *renderer = pass->scene->renderer;
    uint64 current_frame_index = renderer->gpu->current_frame;
    uint64 meshlet_hashed_name = crude_hash_string( "meshlet", 0 );
    crude_gfx_renderer_technique *meshlet_technique = CRUDE_HASHMAP_GET( renderer->resource_cache.techniques, meshlet_hashed_name )->value;
    crude_gfx_pipeline_handle pipeline = meshlet_technique->passes[ pass->meshlet_technique_index ].pipeline;
    crude_gfx_cmd_bind_pipeline( primary_cmd, pipeline );
    crude_gfx_cmd_bind_descriptor_set( primary_cmd, pass->scene->mesh_shader_ds[ renderer->gpu->current_frame ] );
    crude_gfx_cmd_draw_mesh_task_indirect_count( primary_cmd, pass->scene->mesh_task_indirect_commands_sb[ current_frame_index ], CRUDE_OFFSETOF( crude_gfx_mesh_draw_command, indirect_meshlet ), pass->scene->mesh_task_indirect_count_sb[ current_frame_index ], 0, 1u/*render_scene->mesh_instances.size*/, sizeof( crude_gfx_mesh_draw_command ) );
  }
  else if ( use_secondary )
  {
    enkiTaskSet                                           *secondary_draw_tasks[ _PARALLEL_RECORDINGS ];
    secondary_draw_task_container                          secondary_draw_tasks_data[ _PARALLEL_RECORDINGS ];
    secondary_draw_task_container                          offset_secondary_draw_tasks_data;
    uint32                                                 draws_per_secondary, offset;

    draws_per_secondary = CRUDE_ARRAY_LENGTH( pass->mesh_instances ) / _PARALLEL_RECORDINGS;
    offset = draws_per_secondary * _PARALLEL_RECORDINGS;

    for ( uint32 i = 0; i < _PARALLEL_RECORDINGS; ++i )
    {
      secondary_draw_tasks_data[ i ] = CRUDE_COMPOUNT( secondary_draw_task_container, {
        .pass                   = pass,
        .primary_cmd            = primary_cmd,
        .start_mesh_draw_index  = draws_per_secondary * i,
        .end_mesh_draw_index    = draws_per_secondary * i + draws_per_secondary
      } );
      
      secondary_draw_tasks[ i ] = enkiCreateTaskSet( CRUDE_REINTERPRET_CAST( enkiTaskScheduler*, pass->scene->task_scheduler ), secondary_draw_task_ );
      enkiSetArgsTaskSet( secondary_draw_tasks[ i ], &secondary_draw_tasks_data[ i ] );
      enkiAddTaskSet( CRUDE_REINTERPRET_CAST( enkiTaskScheduler*, pass->scene->task_scheduler ), secondary_draw_tasks[ i ] );
    }
    
    if ( offset < CRUDE_ARRAY_LENGTH( pass->mesh_instances ) )
    {
      offset_secondary_draw_tasks_data = CRUDE_COMPOUNT( secondary_draw_task_container, {
        .pass                   = pass,
        .primary_cmd            = primary_cmd,
        .start_mesh_draw_index  = offset,
        .end_mesh_draw_index    = CRUDE_ARRAY_LENGTH( pass->mesh_instances )
      } );
      secondary_draw_task_( NULL, NULL, 0, &offset_secondary_draw_tasks_data );
    }
    
    for ( uint32 i = 0; i < _PARALLEL_RECORDINGS; ++i )
    {
      enkiWaitForTaskSet( CRUDE_REINTERPRET_CAST( enkiTaskScheduler*, pass->scene->task_scheduler ), secondary_draw_tasks[ i ] );
      vkCmdExecuteCommands( primary_cmd->vk_cmd_buffer, 1, &secondary_draw_tasks_data[ i ].secondary_cmd->vk_cmd_buffer );
    }
    
    if ( offset < CRUDE_ARRAY_LENGTH( pass->mesh_instances ) )
    {
      vkCmdExecuteCommands( primary_cmd->vk_cmd_buffer, 1, &offset_secondary_draw_tasks_data.secondary_cmd->vk_cmd_buffer );
    }
  }
  else
  {
    draw_scene_( primary_cmd, pass );
  }
}

void
crude_gfx_scene_renderer_geometry_pass_prepare_draws
(
  _In_ crude_gfx_scene_renderer_geometry_pass             *pass,
  _In_ crude_gfx_render_graph                             *render_graph,
  _In_ crude_stack_allocator                              *temporary_allocator
)
{
  crude_gfx_render_graph_node                             *geometry_pass_node;
  crude_gfx_renderer_material                             *main_material;
  crude_gfx_renderer_technique                            *main_technique;
  uint64                                                   main_technique_name_hashed, main_technique_index;

  main_technique_name_hashed = crude_hash_string( "main", 0 );
  main_technique_index = CRUDE_HASHMAP_GET_INDEX( pass->scene->renderer->resource_cache.techniques, main_technique_name_hashed );
  if ( main_technique_index < 0)
  {
    CRUDE_ASSERT( false );
    return;
  }

  main_technique = pass->scene->renderer->resource_cache.techniques[ main_technique_index ].value;

  {
    crude_gfx_renderer_material_creation material_creation = {
      .technique    = main_technique,
      .name         = "material_no_cull",
      .render_index = 0,
    };
    main_material = crude_gfx_renderer_create_material( pass->scene->renderer, &material_creation );
  }

  geometry_pass_node = crude_gfx_render_graph_builder_access_node_by_name( render_graph->builder, "geometry_pass" );
  CRUDE_ASSERT ( geometry_pass_node );
  

  CRUDE_ARRAY_INITIALIZE_WITH_CAPACITY( pass->mesh_instances, 16, pass->scene->renderer->allocator_container );
  for ( size_t i = 0; i < CRUDE_ARRAY_LENGTH( pass->scene->meshes ); ++i )
  {
    crude_gfx_mesh_instance                                mesh_instance;
    crude_gfx_mesh                                        *mesh;

    mesh = &pass->scene->meshes[ i ];
    if ( crude_gfx_mesh_is_transparent( mesh ) )
    {
      continue;
    }
  
    mesh->material = main_material;
    mesh_instance.mesh = mesh;
    mesh_instance.material_pass_index = 0;
    CRUDE_ARRAY_PUSH( pass->mesh_instances, mesh_instance );
  }

  if ( pass->scene->renderer->gpu->mesh_shaders_extension_present )
  {
    uint64 meshlet_hashed = crude_hash_string( "meshlet", 0 );
    crude_gfx_renderer_technique *main_technique = CRUDE_HASHMAP_GET( pass->scene->renderer->resource_cache.techniques, meshlet_hashed )->value;
    pass->meshlet_technique_index = crude_gfx_renderer_technique_get_pass_index( main_technique, "main" );
  }
}


void
crude_gfx_scene_renderer_geometry_pass_render_raw
(
  _In_ void                                               *ctx,
  _In_ crude_gfx_cmd_buffer                               *primary_cmd
)
{
  crude_gfx_scene_renderer_geometry_pass_render( CRUDE_REINTERPRET_CAST( crude_gfx_scene_renderer_geometry_pass*, ctx ), primary_cmd );
}

crude_gfx_render_graph_pass_container
crude_gfx_scene_renderer_geometry_pass_pack
(
  _In_ crude_gfx_scene_renderer_geometry_pass             *pass
)
{
  crude_gfx_render_graph_pass_container container = {
    .pre_render = crude_gfx_render_graph_pass_container_pre_render_empry,
    .render     = crude_gfx_scene_renderer_geometry_pass_render_raw,
    .on_resize  = crude_gfx_render_graph_pass_container_on_resize_empty,
    .ctx        = pass,
  };
  return container;
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
  scene_renderer->allocator_container = creation->allocator_container;
  scene_renderer->renderer = creation->renderer;
  scene_renderer->async_loader = creation->async_loader;
  scene_renderer->task_scheduler = creation->task_scheduler;

  scene_renderer->geometry_pass.scene = scene_renderer;
  CRUDE_ARRAY_INITIALIZE_WITH_CAPACITY( scene_renderer->images, 0u, scene_renderer->allocator_container );
  CRUDE_ARRAY_INITIALIZE_WITH_CAPACITY( scene_renderer->samplers, 0u, scene_renderer->allocator_container );
  CRUDE_ARRAY_INITIALIZE_WITH_CAPACITY( scene_renderer->buffers, 0u, scene_renderer->allocator_container );
  CRUDE_ARRAY_INITIALIZE_WITH_CAPACITY( scene_renderer->meshes, 0u, scene_renderer->allocator_container );
  CRUDE_ARRAY_INITIALIZE_WITH_CAPACITY( scene_renderer->meshlets_vertices, 0u, scene_renderer->allocator_container );
  CRUDE_ARRAY_INITIALIZE_WITH_CAPACITY( scene_renderer->meshlets_triangles_indices, 0u, scene_renderer->allocator_container );
  CRUDE_ARRAY_INITIALIZE_WITH_CAPACITY( scene_renderer->meshlets_vertices_indices, 0u, scene_renderer->allocator_container );
  CRUDE_ARRAY_INITIALIZE_WITH_CAPACITY( scene_renderer->meshlets, 0u, scene_renderer->allocator_container );
}

void
crude_gfx_scene_renderer_deinitialize
(
  _In_ crude_gfx_scene_renderer                           *scene_renderer
)
{
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

  for ( uint32 i = 0; i < CRUDE_ARRAY_LENGTH( scene_renderer->meshes ); ++i )
  {
     crude_gfx_destroy_buffer( scene_renderer->renderer->gpu, scene_renderer->meshes[ i ].material_buffer );
  }
  CRUDE_ARRAY_DEINITIALIZE( scene_renderer->meshes );
  CRUDE_ARRAY_DEINITIALIZE( scene_renderer->buffers );
  CRUDE_ARRAY_DEINITIALIZE( scene_renderer->geometry_pass.mesh_instances );
  CRUDE_ARRAY_DEINITIALIZE( scene_renderer->meshlets_vertices );
  CRUDE_ARRAY_DEINITIALIZE( scene_renderer->meshlets_triangles_indices );
  CRUDE_ARRAY_DEINITIALIZE( scene_renderer->meshlets_vertices_indices );
  CRUDE_ARRAY_DEINITIALIZE( scene_renderer->meshlets );
}

void
crude_gfx_scene_renderer_prepare_draws
(
  _In_ crude_gfx_scene_renderer                           *scene_renderer,
  _In_ crude_entity                                        node,
  _In_ crude_stack_allocator                              *temporary_allocator
)
{
  crude_gfx_buffer_creation                                buffer_creation;

  scene_renderer_prepare_node_draws_( scene_renderer, node, temporary_allocator );
  
  buffer_creation = CRUDE_COMPOUNT_EMPTY( crude_gfx_buffer_creation );
  buffer_creation.type_flags = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
  buffer_creation.usage = CRUDE_GFX_RESOURCE_USAGE_TYPE_DYNAMIC;
  buffer_creation.size = sizeof( crude_gfx_per_frame );
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

  /* Create indirect buffers */
  for ( uint32 i = 0; i < CRUDE_GFX_MAX_SWAPCHAIN_IMAGES; ++i )
  {
    buffer_creation = CRUDE_COMPOUNT_EMPTY( crude_gfx_buffer_creation );
    buffer_creation.type_flags = VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
    buffer_creation.usage = CRUDE_GFX_RESOURCE_USAGE_TYPE_DYNAMIC;
    buffer_creation.size = /*mesh_instances.size*/ 1 * sizeof( crude_gfx_mesh_draw_command );
    buffer_creation.name = "draw_commands_sb";
    scene_renderer->mesh_task_indirect_commands_sb[ i ] = crude_gfx_create_buffer( scene_renderer->renderer->gpu, &buffer_creation );
    
    buffer_creation = CRUDE_COMPOUNT_EMPTY( crude_gfx_buffer_creation );
    buffer_creation.type_flags = VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
    buffer_creation.usage = CRUDE_GFX_RESOURCE_USAGE_TYPE_DYNAMIC;
    buffer_creation.size = sizeof( crude_gfx_mesh_draw_counts );
    buffer_creation.name = "mesh_count_sb";
    scene_renderer->mesh_task_indirect_count_sb[ i ] = crude_gfx_create_buffer( scene_renderer->renderer->gpu, &buffer_creation );
  }

  scene_renderer->use_meshlets = scene_renderer->renderer->gpu->mesh_shaders_extension_present;

  /* Meshlets descriptors */
  uint64 meshlet_hashed = crude_hash_string( "meshlet", 0u );
  crude_gfx_renderer_technique *meshlet_technique = CRUDE_HASHMAP_GET( scene_renderer->renderer->resource_cache.techniques, meshlet_hashed )->value;
  
  uint32 meshlet_index = crude_gfx_renderer_technique_get_pass_index( meshlet_technique, "main" );
  crude_gfx_renderer_technique_pass *meshlet_pass = &meshlet_technique->passes[ meshlet_index ];
  crude_gfx_descriptor_set_layout_handle layout = crude_gfx_get_descriptor_set_layout( scene_renderer->renderer->gpu, meshlet_pass->pipeline, 0u );
  
  for ( uint32 i = 0; i < CRUDE_GFX_MAX_SWAPCHAIN_IMAGES; ++i )
  {
    crude_gfx_descriptor_set_creation                    ds_creation;
    
    ds_creation = CRUDE_COMPOUNT_EMPTY( crude_gfx_descriptor_set_creation );
    ds_creation.layout = layout;

    crude_gfx_descriptor_set_creation_add_buffer( &ds_creation, scene_renderer->scene_cb, 0u );
    crude_gfx_descriptor_set_creation_add_buffer( &ds_creation, scene_renderer->meshlets_sb, 1u );
    crude_gfx_descriptor_set_creation_add_buffer( &ds_creation, scene_renderer->meshlets_vertices_sb, 2u );
    crude_gfx_descriptor_set_creation_add_buffer( &ds_creation, scene_renderer->meshlets_triangles_indices_sb, 3u );
    crude_gfx_descriptor_set_creation_add_buffer( &ds_creation, scene_renderer->meshlets_vertices_indices_sb, 4u );
    
    scene_renderer->mesh_shader_ds[ i ] = crude_gfx_create_descriptor_set( scene_renderer->renderer->gpu, &ds_creation );
  }

  crude_gfx_scene_renderer_geometry_pass_prepare_draws( &scene_renderer->geometry_pass, scene_renderer->render_graph, temporary_allocator );
}

void
crude_gfx_scene_renderer_submit_draw_task
(
  _In_ crude_gfx_scene_renderer                           *scene_renderer,
  _In_ bool                                                use_secondary
)
{
  crude_gfx_cmd_buffer *gpu_commands = crude_gfx_get_primary_cmd( scene_renderer->renderer->gpu, 0, true );
  crude_gfx_render_graph_render( scene_renderer->render_graph, gpu_commands );
  crude_gfx_queue_cmd( gpu_commands );
}

void
crude_gfx_scene_renderer_register_render_passes
(
  _In_ crude_gfx_scene_renderer                           *scene_renderer,
  _In_ crude_gfx_render_graph                             *render_graph
)
{
  scene_renderer->render_graph = render_graph;

  crude_gfx_render_graph_builder_register_render_pass( render_graph->builder, "geometry_pass", crude_gfx_scene_renderer_geometry_pass_pack( &scene_renderer->geometry_pass ) );
}

/**
 *
 * Renderer Scene Draw Task Implementation
 * 
 */
void
secondary_draw_task_
(
  _In_ uint32_t                                            start,
  _In_ uint32_t                                            end,
  _In_ uint32_t                                            thread_num,
  _In_ void                                               *ctx
)
{
  CRUDE_PROFILER_SET_THREAD_NAME( "SecondaryDrawTaskThread" );

  secondary_draw_task_container *secondary_draw_task = CRUDE_REINTERPRET_CAST( secondary_draw_task_container*, ctx );
  
  crude_gfx_cmd_buffer *secondary_cmd = crude_gfx_get_secondary_cmd( secondary_draw_task->pass->scene->renderer->gpu, thread_num );
  crude_gfx_cmd_begin_secondary( secondary_cmd, secondary_draw_task->primary_cmd->current_render_pass, secondary_draw_task->primary_cmd->current_framebuffer );
  
  crude_gfx_cmd_set_viewport( secondary_cmd, NULL );
  crude_gfx_cmd_set_scissor( secondary_cmd, NULL );
  draw_scene_( secondary_cmd, secondary_draw_task->pass );

  crude_gfx_cmd_end( secondary_cmd );
  secondary_draw_task->secondary_cmd = secondary_cmd;
}

/**
 *
 * Renderer Scene Draw Implementation
 * 
 */
void
draw_scene_
(
  _In_ crude_gfx_cmd_buffer                               *cmd,
  _In_ crude_gfx_scene_renderer_geometry_pass             *pass
)
{
  crude_gfx_renderer_material *last_material = NULL;
  for ( uint32 mesh_index = 0; mesh_index < CRUDE_ARRAY_LENGTH( pass->mesh_instances ); ++mesh_index )
  {
    crude_gfx_mesh_instance                               *mesh_instance;
    crude_gfx_mesh                                        *mesh;
  
    mesh_instance = &pass->mesh_instances[ mesh_index ];
    mesh = mesh_instance->mesh;
    if ( mesh->material != last_material )
    {
      crude_gfx_cmd_bind_pipeline( cmd, mesh->material->technique->passes[ mesh_instance->material_pass_index ].pipeline );
      last_material = mesh->material;
    }
    
    bool mesh_buffers_ready = crude_gfx_buffer_ready( cmd->gpu, mesh->position_buffer )
      && crude_gfx_buffer_ready( cmd->gpu, mesh->tangent_buffer )
      && crude_gfx_buffer_ready( cmd->gpu, mesh->normal_buffer )
      && crude_gfx_buffer_ready( cmd->gpu, mesh->texcoord_buffer )
      && crude_gfx_buffer_ready( cmd->gpu, mesh->index_buffer );

    if ( !mesh_buffers_ready )
    {
      continue;
    }

    crude_gfx_descriptor_set_creation ds_creation = crude_gfx_descriptor_set_creation_empty();
    crude_gfx_descriptor_set_creation_add_buffer( &ds_creation, pass->scene->scene_cb, 0u );
    crude_gfx_descriptor_set_creation_add_buffer( &ds_creation, mesh->material_buffer, 1u );
    ds_creation.layout = crude_gfx_access_pipeline( cmd->gpu, mesh->material->technique->passes[ 0 ].pipeline )->descriptor_set_layout_handle[ 0 ];

    crude_gfx_descriptor_set_handle descriptor_set = crude_gfx_cmd_create_local_descriptor_set( cmd, &ds_creation );
    crude_gfx_cmd_bind_vertex_buffer( cmd, mesh->position_buffer, 0, mesh->position_offset );
    crude_gfx_cmd_bind_vertex_buffer( cmd, mesh->tangent_buffer, 1, mesh->tangent_offset );
    crude_gfx_cmd_bind_vertex_buffer( cmd, mesh->normal_buffer, 2, mesh->normal_offset );
    crude_gfx_cmd_bind_vertex_buffer( cmd, mesh->texcoord_buffer, 3, mesh->texcoord_offset );
    crude_gfx_cmd_bind_index_buffer( cmd, mesh->index_buffer, mesh->index_offset );
    crude_gfx_cmd_bind_local_descriptor_set( cmd, descriptor_set );
    crude_gfx_cmd_draw_indexed( cmd, mesh->primitive_count, 1, 0, 0, 0 );
  }
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