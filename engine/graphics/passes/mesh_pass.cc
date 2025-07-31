#include <TaskScheduler_c.h>

#include <core/profiler.h>
#include <core/hash_map.h>
#include <scene/scene_components.h>
#include <graphics/scene_renderer.h>

#include <graphics/passes/mesh_pass.h>

#define _PARALLEL_RECORDINGS                               ( 4 )

typedef struct secondary_draw_task_container
{
  crude_gfx_mesh_pass                                     *pass;
  crude_gfx_cmd_buffer                                    *primary_cmd;
  crude_gfx_cmd_buffer                                    *secondary_cmd;
  uint32                                                   start_mesh_draw_index;
  uint32                                                   end_mesh_draw_index;
  uint32                                                   thread_id;
} secondary_draw_task_container;

static void
draw_scene_secondary_task_
(
  _In_ uint32_t                                            start,
  _In_ uint32_t                                            end,
  _In_ uint32_t                                            thread_num,
  _In_ void                                               *ctx
);

static void
draw_scene_primary_
(
  _In_ crude_gfx_cmd_buffer                               *cmd,
  _In_ crude_gfx_mesh_pass                                *pass
);

void
crude_gfx_mesh_pass_initialize
(
  _In_ crude_gfx_mesh_pass                                *pass,
  _In_ crude_gfx_scene_renderer                           *scene_renderer,
  _In_ uint32                                              use_secondary
)
{ 
  pass->scene_renderer = scene_renderer;
  pass->use_secondary = use_secondary;
}

void
crude_gfx_mesh_pass_deinitialize
(
  _In_ crude_gfx_mesh_pass                                *pass
)
{
}

void
crude_gfx_mesh_pass_render
(
  _In_ void                                               *ctx,
  _In_ crude_gfx_cmd_buffer                               *primary_cmd
)
{
  crude_gfx_mesh_pass *pass = CRUDE_REINTERPRET_CAST( crude_gfx_mesh_pass*, ctx );

  if ( pass->use_secondary )
  {
    enkiTaskSet                                           *secondary_draw_tasks[ _PARALLEL_RECORDINGS ];
    secondary_draw_task_container                          secondary_draw_tasks_data[ _PARALLEL_RECORDINGS ];
    secondary_draw_task_container                          offset_secondary_draw_tasks_data;
    uint32                                                 draws_per_secondary, offset;
    
    draws_per_secondary = CRUDE_ARRAY_LENGTH( pass->scene_renderer->meshes_instances ) / _PARALLEL_RECORDINGS;
    offset = draws_per_secondary * _PARALLEL_RECORDINGS;
    
    for ( uint32 i = 0; i < _PARALLEL_RECORDINGS; ++i )
    {
      secondary_draw_tasks_data[ i ] = CRUDE_COMPOUNT_EMPTY( secondary_draw_task_container );
      secondary_draw_tasks_data[ i ].pass                   = pass;
      secondary_draw_tasks_data[ i ].primary_cmd            = primary_cmd;
      secondary_draw_tasks_data[ i ].start_mesh_draw_index  = draws_per_secondary * i;
      secondary_draw_tasks_data[ i ].end_mesh_draw_index    = draws_per_secondary * i + draws_per_secondary;
      
      secondary_draw_tasks[ i ] = enkiCreateTaskSet( CRUDE_REINTERPRET_CAST( enkiTaskScheduler*, pass->scene_renderer->task_scheduler ), draw_scene_secondary_task_ );
      enkiSetArgsTaskSet( secondary_draw_tasks[ i ], &secondary_draw_tasks_data[ i ] );
      enkiAddTaskSet( CRUDE_REINTERPRET_CAST( enkiTaskScheduler*, pass->scene_renderer->task_scheduler ), secondary_draw_tasks[ i ] );
    }
    
    if ( offset < CRUDE_ARRAY_LENGTH( pass->scene_renderer->meshes_instances ) )
    {
      offset_secondary_draw_tasks_data = CRUDE_COMPOUNT_EMPTY( secondary_draw_task_container );
      offset_secondary_draw_tasks_data.pass                   = pass;
      offset_secondary_draw_tasks_data.primary_cmd            = primary_cmd;
      offset_secondary_draw_tasks_data.start_mesh_draw_index  = offset;
      offset_secondary_draw_tasks_data.end_mesh_draw_index    = CRUDE_ARRAY_LENGTH( pass->scene_renderer->meshes_instances );
      draw_scene_secondary_task_( NULL, NULL, 0, &offset_secondary_draw_tasks_data );
    }
    
    for ( uint32 i = 0; i < _PARALLEL_RECORDINGS; ++i )
    {
      enkiWaitForTaskSet( CRUDE_REINTERPRET_CAST( enkiTaskScheduler*, pass->scene_renderer->task_scheduler ), secondary_draw_tasks[ i ] );
      vkCmdExecuteCommands( primary_cmd->vk_cmd_buffer, 1, &secondary_draw_tasks_data[ i ].secondary_cmd->vk_cmd_buffer );
    }
    
    if ( offset < CRUDE_ARRAY_LENGTH( pass->scene_renderer->meshes_instances ) )
    {
      vkCmdExecuteCommands( primary_cmd->vk_cmd_buffer, 1, &offset_secondary_draw_tasks_data.secondary_cmd->vk_cmd_buffer );
    }
  }
  else
  {
    draw_scene_primary_( primary_cmd, pass );
  }
}

static void
crude_gfx_mesh_pass_on_resize
(
  _In_ void                                               *ctx,
  _In_ crude_gfx_device                                   *gpu,
  _In_ uint32                                              new_width,
  _In_ uint32                                              new_height
)
{
}

crude_gfx_render_graph_pass_container
crude_gfx_mesh_pass_pack
(
  _In_ crude_gfx_mesh_pass                                *pass
)
{
  crude_gfx_render_graph_pass_container container;
  container.ctx = pass;
  container.on_resize = crude_gfx_mesh_pass_on_resize;
  container.render = crude_gfx_mesh_pass_render;
  return container;
}

void
draw_scene_secondary_task_
(
  _In_ uint32_t                                            start,
  _In_ uint32_t                                            end,
  _In_ uint32_t                                            thread_num,
  _In_ void                                               *ctx
)
{
  CRUDE_PROFILER_SET_THREAD_NAME( "SecondaryDrawTaskThread" );

  secondary_draw_task_container *secondary_draw_task = CRUDE_REINTERPRET_CAST( secondary_draw_task_container*, ctx );
  
  crude_gfx_cmd_buffer *secondary_cmd = crude_gfx_get_secondary_cmd( secondary_draw_task->pass->scene_renderer->renderer->gpu, thread_num );
  crude_gfx_cmd_begin_secondary( secondary_cmd, secondary_draw_task->primary_cmd->current_render_pass, secondary_draw_task->primary_cmd->current_framebuffer );
  
  crude_gfx_cmd_set_viewport( secondary_cmd, NULL );
  crude_gfx_cmd_set_scissor( secondary_cmd, NULL );
  draw_scene_primary_( secondary_cmd, secondary_draw_task->pass );

  crude_gfx_cmd_end( secondary_cmd );
  secondary_draw_task->secondary_cmd = secondary_cmd;
}

void
draw_scene_primary_
(
  _In_ crude_gfx_cmd_buffer                               *cmd,
  _In_ crude_gfx_mesh_pass                                *pass
)
{
  //crude_gfx_renderer_material *last_material = NULL;
  //for ( uint32 mesh_index = 0; mesh_index < CRUDE_ARRAY_LENGTH( pass->scene_renderer->meshes_instances ); ++mesh_index )
  //{
  //  crude_gfx_device                                      *gpu;
  //  crude_gfx_mesh_instance_cpu                           *mesh_instance;
  //  crude_gfx_mesh_cpu                                    *mesh_cpu;
  //
  //  gpu = pass->scene_renderer->renderer->gpu;
  //  mesh_instance = &pass->scene_renderer->meshes_instances[ mesh_index ];
  //  mesh_cpu = mesh_instance->mesh;
  //  if ( mesh_cpu->material != last_material )
  //  {
  //    crude_gfx_cmd_bind_pipeline( cmd, mesh_cpu->material->technique->passes[ mesh_instance->material_pass_index ].pipeline );
  //    last_material = mesh_cpu->material;
  //  }
  //  
  //  bool mesh_textures_ready = ( CRUDE_RESOURCE_HANDLE_IS_INVALID( mesh_cpu->albedo_texture_handle ) || crude_gfx_texture_ready( gpu, mesh_cpu->albedo_texture_handle ) )
  //    && ( CRUDE_RESOURCE_HANDLE_IS_INVALID( mesh_cpu->normal_texture_handle ) || crude_gfx_texture_ready( gpu, mesh_cpu->normal_texture_handle ) )
  //    && ( CRUDE_RESOURCE_HANDLE_IS_INVALID( mesh_cpu->occlusion_texture_handle ) || crude_gfx_texture_ready( gpu, mesh_cpu->occlusion_texture_handle ) )
  //    && ( CRUDE_RESOURCE_HANDLE_IS_INVALID( mesh_cpu->roughness_texture_handle ) || crude_gfx_texture_ready( gpu, mesh_cpu->roughness_texture_handle ) );

  //  bool mesh_buffers_ready = crude_gfx_buffer_ready( cmd->gpu, mesh_cpu->position_buffer )
  //    && crude_gfx_buffer_ready( cmd->gpu, mesh_cpu->tangent_buffer )
  //    && crude_gfx_buffer_ready( cmd->gpu, mesh_cpu->normal_buffer )
  //    && crude_gfx_buffer_ready( cmd->gpu, mesh_cpu->texcoord_buffer )
  //    && crude_gfx_buffer_ready( cmd->gpu, mesh_cpu->index_buffer );

  //  if ( !mesh_buffers_ready || !mesh_textures_ready )
  //  {
  //    continue;
  //  }

  //  crude_gfx_descriptor_set_creation ds_creation = crude_gfx_descriptor_set_creation_empty();
  //  ds_creation.name = "mesh_material_descriptor_set";
  //  crude_gfx_descriptor_set_creation_add_buffer( &ds_creation, pass->scene_renderer->scene_cb, 0u );
  //  crude_gfx_descriptor_set_creation_add_buffer( &ds_creation, mesh_cpu->material_buffer, 1u );
  //  crude_gfx_scene_renderer_add_debug_resources_to_descriptor_set_creation( &ds_creation, pass->scene_renderer, gpu->current_frame );
  //  ds_creation.layout = crude_gfx_get_descriptor_set_layout( cmd->gpu, mesh_cpu->material->technique->passes[ 0 ].pipeline, CRUDE_GFX_MATERIAL_DESCRIPTOR_SET_INDEX );

  //  crude_gfx_descriptor_set_handle descriptor_set = crude_gfx_cmd_create_local_descriptor_set( cmd, &ds_creation );
  //  crude_gfx_cmd_bind_vertex_buffer( cmd, mesh_cpu->position_buffer, 0, mesh_cpu->position_offset );
  //  crude_gfx_cmd_bind_vertex_buffer( cmd, mesh_cpu->tangent_buffer, 1, mesh_cpu->tangent_offset );
  //  crude_gfx_cmd_bind_vertex_buffer( cmd, mesh_cpu->normal_buffer, 2, mesh_cpu->normal_offset );
  //  crude_gfx_cmd_bind_vertex_buffer( cmd, mesh_cpu->texcoord_buffer, 3, mesh_cpu->texcoord_offset );
  //  crude_gfx_cmd_bind_index_buffer( cmd, mesh_cpu->index_buffer, mesh_cpu->index_offset );
  //  crude_gfx_cmd_bind_local_descriptor_set( cmd, descriptor_set );
  //  crude_gfx_cmd_draw_indexed( cmd, mesh_cpu->primitive_count, 1, 0, 0, 0 );
  //}
}