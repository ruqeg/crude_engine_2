#include <core/profiler.h>
#include <core/array.h>
#include <core/file.h>
#include <core/hash_map.h>

#include <graphics/renderer_scene.h>

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
  _In_ crude_gfx_renderer_scene_geometry_pass             *pass;
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
  _In_ crude_gfx_renderer_scene_geometry_pass             *pass
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
crude_gfx_renderer_scene_geometry_pass_render
(
  _In_ crude_gfx_renderer_scene_geometry_pass             *pass,
  _In_ crude_gfx_cmd_buffer                               *primary_cmd
)
{
  bool use_secondary = true;
  if ( use_secondary )
  {
    enkiTaskSet                                           *secondary_draw_tasks[ _PARALLEL_RECORDINGS ];
    secondary_draw_task_container                          secondary_draw_tasks_data[ _PARALLEL_RECORDINGS ];
    secondary_draw_task_container                          offset_secondary_draw_tasks_data;
    uint32                                                 draws_per_secondary, offset;

    draws_per_secondary = CRUDE_ARRAY_LENGTH( pass->mesh_instances ) / _PARALLEL_RECORDINGS;
    offset = draws_per_secondary * _PARALLEL_RECORDINGS;

    for ( uint32 i = 0; i < _PARALLEL_RECORDINGS; ++i )
    {
      secondary_draw_tasks_data[ i ] = ( secondary_draw_task_container ) {
        .pass                   = pass,
        .primary_cmd            = primary_cmd,
        .start_mesh_draw_index  = draws_per_secondary * i,
        .end_mesh_draw_index    = draws_per_secondary * i + draws_per_secondary
      };
      
      secondary_draw_tasks[ i ] = enkiCreateTaskSet( pass->scene->task_scheduler, secondary_draw_task_ );
      enkiSetArgsTaskSet( secondary_draw_tasks[ i ], &secondary_draw_tasks_data[ i ] );
      enkiAddTaskSet( pass->scene->task_scheduler, secondary_draw_tasks[ i ] );
    }
    
    if ( offset < CRUDE_ARRAY_LENGTH( pass->mesh_instances ) )
    {
      offset_secondary_draw_tasks_data = ( secondary_draw_task_container ) {
        .pass                   = pass,
        .primary_cmd            = primary_cmd,
        .start_mesh_draw_index  = offset,
        .end_mesh_draw_index    = CRUDE_ARRAY_LENGTH( pass->mesh_instances )
      };
      secondary_draw_task_( NULL, NULL, 0, &offset_secondary_draw_tasks_data );
    }
    
    for ( uint32 i = 0; i < _PARALLEL_RECORDINGS; ++i )
    {
      enkiWaitForTaskSet( pass->scene->task_scheduler, secondary_draw_tasks[ i ] );
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
crude_gfx_renderer_scene_geometry_pass_prepare_draws
(
  _In_ crude_gfx_renderer_scene_geometry_pass             *pass,
  _In_ crude_gfx_render_graph                             *render_graph,
  _In_ crude_stack_allocator                              *temporary_allocator
)
{
  crude_gfx_render_graph_node                             *geometry_pass_node;
  crude_gfx_renderer_material                             *main_material;
  crude_gfx_renderer_technique                            *main_technique;
  uint64                                                   main_technique_name_hashed, main_technique_index;

  main_technique_name_hashed = crude_hash_bytes( ( void* )"main", strlen( "main" ), 0 );
  main_technique_index = CRUDE_HASHMAP_GET_INDEX( pass->scene->renderer->resource_cache.techniques, main_technique_name_hashed );
  if ( main_technique_index < 0)
  {
    CRUDE_ASSERT( false );
    return;
  }

  main_technique = pass->scene->renderer->resource_cache.techniques[ main_technique_index ].value;

  {
    crude_gfx_renderer_material_creation material_creation = {
      .name         = "material_no_cull",
      .technique    = main_technique,
      .render_index = 0
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
}

crude_gfx_render_graph_pass_container
crude_gfx_renderer_scene_geometry_pass_pack
(
  _In_ crude_gfx_renderer_scene_geometry_pass             *pass
)
{
  crude_gfx_render_graph_pass_container container = {
    .pre_render = crude_gfx_render_graph_pass_container_pre_render_empry,
    .render     = crude_gfx_renderer_scene_geometry_pass_render,
    .on_resize  = crude_gfx_render_graph_pass_container_on_resize_empty,
    .ctx        = pass 
  };
  return container;
}

/**
 *
 * Renderer Scene Function
 * 
 */
void
crude_gfx_renderer_scene_initialize
(
  _In_ crude_gfx_renderer_scene                           *scene,
  _In_ crude_gfx_renderer_scene_creation                  *creation
)
{
  scene->allocator_container = creation->allocator_container;
  scene->renderer = creation->renderer;
  scene->async_loader = creation->async_loader;
  scene->task_scheduler = creation->task_scheduler;

  scene->geometry_pass.scene = scene;
  scene->images = NULL;
  scene->samplers = NULL;
  scene->buffers = NULL;
  scene->meshes = NULL;
}

void
crude_gfx_renderer_scene_deinitialize
(
  _In_ crude_gfx_renderer_scene                           *scene
)
{
  for ( uint32 i = 0; i < CRUDE_ARRAY_LENGTH( scene->images ); ++i )
  {
    crude_gfx_renderer_destroy_texture( scene->renderer, &scene->images[ i ] );
  }
  CRUDE_ARRAY_FREE( scene->images );

  for ( uint32 i = 0; i < CRUDE_ARRAY_LENGTH( scene->samplers ); ++i )
  {
    crude_gfx_renderer_destroy_sampler( scene->renderer, &scene->samplers[ i ] );
  }
  CRUDE_ARRAY_FREE( scene->samplers );
  
  for ( uint32 i = 0; i < CRUDE_ARRAY_LENGTH( scene->buffers ); ++i )
  {
    crude_gfx_renderer_destroy_buffer( scene->renderer, &scene->buffers[ i ] );
  }
  CRUDE_ARRAY_FREE( scene->buffers );

  for ( uint32 i = 0; i < CRUDE_ARRAY_LENGTH( scene->meshes ); ++i )
  {
     crude_gfx_destroy_buffer( scene->renderer->gpu, scene->meshes[ i ].material_buffer );
  }
  CRUDE_ARRAY_FREE( scene->meshes );
  CRUDE_ARRAY_FREE( scene->buffers );
  CRUDE_ARRAY_FREE( scene->geometry_pass.mesh_instances );
}

void
crude_gfx_renderer_scene_prepare_draws
(
  _In_ crude_gfx_renderer_scene                           *scene,
  _In_ crude_stack_allocator                              *temporary_allocator
)
{
  crude_gfx_renderer_scene_geometry_pass_prepare_draws( &scene->geometry_pass, scene->render_graph, temporary_allocator );
}

void
crude_gfx_renderer_scene_submit_draw_task
(
  _In_ crude_gfx_renderer_scene                           *scene,
  _In_ enkiTaskScheduler                                  *task_sheduler,
  _In_ bool                                                use_secondary
)
{
  crude_gfx_cmd_buffer *gpu_commands = crude_gfx_get_primary_cmd( scene->renderer->gpu, 0, true );
  crude_gfx_render_graph_render( scene->render_graph, gpu_commands );
  crude_gfx_queue_cmd( gpu_commands );
}

void
crude_gfx_renderer_scene_register_render_passes
(
  _In_ crude_gfx_renderer_scene                           *scene,
  _In_ crude_gfx_render_graph                             *render_graph
)
{
  scene->render_graph = render_graph;

  crude_gfx_render_graph_builder_register_render_pass( render_graph->builder, "geometry_pass", crude_gfx_renderer_scene_geometry_pass_pack( &scene->geometry_pass ) );
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

  secondary_draw_task_container *secondary_draw_task = ctx;
  
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
draw_mesh_
(
  _In_ crude_gfx_cmd_buffer                               *cmd,
  _In_ crude_gfx_mesh                                     *mesh
)
{
  bool mesh_buffers_ready = crude_gfx_buffer_ready( cmd->gpu, mesh->position_buffer )
    && crude_gfx_buffer_ready( cmd->gpu, mesh->tangent_buffer )
    && crude_gfx_buffer_ready( cmd->gpu, mesh->normal_buffer )
    && crude_gfx_buffer_ready( cmd->gpu, mesh->texcoord_buffer )
    && crude_gfx_buffer_ready( cmd->gpu, mesh->index_buffer );

  if ( !mesh_buffers_ready )
  {
    return;
  }

  crude_gfx_descriptor_set_creation ds_creation = crude_gfx_descriptor_set_creation_empty();
  ds_creation.samplers[ 0 ] = CRUDE_GFX_SAMPLER_HANDLE_INVALID;
  ds_creation.samplers[ 1 ] = CRUDE_GFX_SAMPLER_HANDLE_INVALID;
  ds_creation.bindings[ 0 ] = 0;
  ds_creation.bindings[ 1 ] = 1;
  ds_creation.resources[ 0 ] = cmd->gpu->frame_buffer.index;
  ds_creation.resources[ 1 ] = mesh->material_buffer.index;
  ds_creation.num_resources = 2;
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

void
draw_scene_
(
  _In_ crude_gfx_cmd_buffer                               *cmd,
  _In_ crude_gfx_renderer_scene_geometry_pass             *pass
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
    draw_mesh_( cmd, mesh );
  }
}