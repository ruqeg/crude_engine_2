#include <TaskScheduler_c.h>

#include <core/profiler.h>
#include <core/hash_map.h>
#include <scene/scene_components.h>
#include <graphics/scene_renderer.h>

#include <graphics/passes/geometry_pass.h>

#define _PARALLEL_RECORDINGS                               ( 4 )

typedef struct secondary_draw_task_container
{
  crude_gfx_geometry_pass                                 *pass;
  crude_gfx_cmd_buffer                                    *primary_cmd;
  crude_gfx_cmd_buffer                                    *secondary_cmd;
  uint32                                                   start_mesh_draw_index;
  uint32                                                   end_mesh_draw_index;
  uint32                                                   thread_id;
} secondary_draw_task_container;

static void
copy_mesh_material_gpu_
(
  _In_ crude_gfx_mesh_cpu const                           *mesh,
  _Out_ crude_gfx_mesh_material_gpu                       *gpu_mesh_material
);

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
  _In_ crude_gfx_geometry_pass                            *pass
);

static void
geometry_pass_render_classic_
(
  _In_ crude_gfx_cmd_buffer                               *cmd,
  _In_ crude_gfx_geometry_pass                            *pass
);

static void
geometry_pass_render_secondary_
(
  _In_ crude_gfx_cmd_buffer                               *cmd,
  _In_ crude_gfx_geometry_pass                            *pass
);

static void
geometry_pass_render_meshlets_
(
  _In_ crude_gfx_cmd_buffer                               *cmd,
  _In_ crude_gfx_geometry_pass                            *pass
);

void
crude_gfx_geometry_pass_initialize
(
  _In_ crude_gfx_geometry_pass                            *pass,
  _In_ crude_gfx_scene_renderer                           *scene_renderer,
  _In_ uint32                                              flags
)
{ 
  pass->scene_renderer = scene_renderer;
  pass->flags = flags;
}

void
crude_gfx_geometry_pass_deinitialize
(
  _In_ crude_gfx_geometry_pass                            *pass
)
{
}

void
crude_gfx_geometry_pass_render
(
  _In_ void                                               *ctx,
  _In_ crude_gfx_cmd_buffer                               *primary_cmd
)
{
  crude_gfx_geometry_pass *pass = CRUDE_REINTERPRET_CAST( crude_gfx_geometry_pass*, ctx );

  if ( pass->flags & CRUDE_GFX_GEOMETRY_PASS_MESHLETS_BIT )
  {
    geometry_pass_render_meshlets_( primary_cmd, pass );
  }
  else if ( pass->flags & CRUDE_GFX_GEOMETRY_PASS_SECONDARY_BIT )
  {
    geometry_pass_render_secondary_( primary_cmd, pass );
  }
  else
  {
    geometry_pass_render_classic_( primary_cmd, pass );
  }
}

static void
crude_gfx_geometry_pass_on_resize
(
  _In_ void                                               *ctx,
  _In_ crude_gfx_device                                   *gpu,
  _In_ uint32                                              new_width,
  _In_ uint32                                              new_height
)
{
}

crude_gfx_render_graph_pass_container
crude_gfx_geometry_pass_pack
(
  _In_ crude_gfx_geometry_pass                            *pass
)
{
  crude_gfx_render_graph_pass_container container;
  container.ctx = pass;
  container.on_resize = crude_gfx_geometry_pass_on_resize;
  container.render = crude_gfx_geometry_pass_render;
  return container;
}


static void
geometry_pass_render_classic_
(
  _In_ crude_gfx_cmd_buffer                               *cmd,
  _In_ crude_gfx_geometry_pass                            *pass
)
{
  draw_scene_primary_( cmd, pass );
}

static void
geometry_pass_render_secondary_
(
  _In_ crude_gfx_cmd_buffer                               *cmd,
  _In_ crude_gfx_geometry_pass                            *pass
)
{
  enkiTaskSet                                           *secondary_draw_tasks[ _PARALLEL_RECORDINGS ];
  secondary_draw_task_container                          secondary_draw_tasks_data[ _PARALLEL_RECORDINGS ];
  secondary_draw_task_container                          offset_secondary_draw_tasks_data;
  uint32                                                 draws_per_secondary, offset;
  
  draws_per_secondary = CRUDE_ARRAY_LENGTH( pass->scene_renderer->mesh_instances ) / _PARALLEL_RECORDINGS;
  offset = draws_per_secondary * _PARALLEL_RECORDINGS;
  
  for ( uint32 i = 0; i < _PARALLEL_RECORDINGS; ++i )
  {
    secondary_draw_tasks_data[ i ] = CRUDE_COMPOUNT_EMPTY( secondary_draw_task_container );
    secondary_draw_tasks_data[ i ].pass                   = pass;
    secondary_draw_tasks_data[ i ].primary_cmd            = cmd;
    secondary_draw_tasks_data[ i ].start_mesh_draw_index  = draws_per_secondary * i;
    secondary_draw_tasks_data[ i ].end_mesh_draw_index    = draws_per_secondary * i + draws_per_secondary;
    
    secondary_draw_tasks[ i ] = enkiCreateTaskSet( CRUDE_REINTERPRET_CAST( enkiTaskScheduler*, pass->scene_renderer->task_scheduler ), draw_scene_secondary_task_ );
    enkiSetArgsTaskSet( secondary_draw_tasks[ i ], &secondary_draw_tasks_data[ i ] );
    enkiAddTaskSet( CRUDE_REINTERPRET_CAST( enkiTaskScheduler*, pass->scene_renderer->task_scheduler ), secondary_draw_tasks[ i ] );
  }
  
  if ( offset < CRUDE_ARRAY_LENGTH( pass->scene_renderer->mesh_instances ) )
  {
    offset_secondary_draw_tasks_data = CRUDE_COMPOUNT_EMPTY( secondary_draw_task_container );
    offset_secondary_draw_tasks_data.pass                   = pass;
    offset_secondary_draw_tasks_data.primary_cmd            = cmd;
    offset_secondary_draw_tasks_data.start_mesh_draw_index  = offset;
    offset_secondary_draw_tasks_data.end_mesh_draw_index    = CRUDE_ARRAY_LENGTH( pass->scene_renderer->mesh_instances );
    draw_scene_secondary_task_( NULL, NULL, 0, &offset_secondary_draw_tasks_data );
  }
  
  for ( uint32 i = 0; i < _PARALLEL_RECORDINGS; ++i )
  {
    enkiWaitForTaskSet( CRUDE_REINTERPRET_CAST( enkiTaskScheduler*, pass->scene_renderer->task_scheduler ), secondary_draw_tasks[ i ] );
    vkCmdExecuteCommands( cmd->vk_cmd_buffer, 1, &secondary_draw_tasks_data[ i ].secondary_cmd->vk_cmd_buffer );
  }
  
  if ( offset < CRUDE_ARRAY_LENGTH( pass->scene_renderer->mesh_instances ) )
  {
    vkCmdExecuteCommands( cmd->vk_cmd_buffer, 1, &offset_secondary_draw_tasks_data.secondary_cmd->vk_cmd_buffer );
  }
}

static void
geometry_pass_render_meshlets_
(
  _In_ crude_gfx_cmd_buffer                               *cmd,
  _In_ crude_gfx_geometry_pass                            *pass
)
{
  crude_gfx_renderer                                      *renderer;
  crude_gfx_mesh_material_gpu                             *mesh_materials;
  crude_gfx_mesh_draw_counts_gpu                          *mesh_draw_counts;
  crude_gfx_mesh_draw_command_gpu                         *mesh_draw_commands;
  crude_gfx_map_buffer_parameters                          buffer_map;
  
  renderer = pass->scene_renderer->renderer;

  buffer_map = CRUDE_COMPOUNT_EMPTY( crude_gfx_map_buffer_parameters );
  buffer_map.buffer = pass->scene_renderer->mesh_task_indirect_commands_sb[ renderer->gpu->current_frame ];
  buffer_map.offset = 0;
  buffer_map.size = sizeof( crude_gfx_mesh_draw_command_gpu ) * CRUDE_ARRAY_LENGTH( pass->scene_renderer->mesh_instances );

  mesh_draw_commands = CRUDE_CAST( crude_gfx_mesh_draw_command_gpu*, crude_gfx_map_buffer( renderer->gpu, &buffer_map ) );
  if ( mesh_draw_commands )
  {
    for ( uint32 i = 0; i < CRUDE_ARRAY_LENGTH( pass->scene_renderer->mesh_instances ); ++i )
    {
      mesh_draw_commands[ i ].indirect_meshlet.groupCountX = crude_ceil( pass->scene_renderer->mesh_instances[ i ].mesh->meshlets_count / 32.0 );
      mesh_draw_commands[ i ].indirect_meshlet.groupCountY = 1;
      mesh_draw_commands[ i ].indirect_meshlet.groupCountZ = 1;
    }
    crude_gfx_unmap_buffer( renderer->gpu, buffer_map.buffer );
  }
  
  buffer_map = CRUDE_COMPOUNT_EMPTY( crude_gfx_map_buffer_parameters );
  buffer_map.buffer = pass->scene_renderer->mesh_task_indirect_count_sb[ renderer->gpu->current_frame ];
  buffer_map.offset = 0;
  buffer_map.size = sizeof( crude_gfx_mesh_draw_counts_gpu );
  mesh_draw_counts = CRUDE_CAST( crude_gfx_mesh_draw_counts_gpu*, crude_gfx_map_buffer( renderer->gpu, &buffer_map ) );
  if ( mesh_draw_counts )
  {
    mesh_draw_counts->opaque_mesh_visible_count = CRUDE_ARRAY_LENGTH( pass->scene_renderer->mesh_instances );
    crude_gfx_unmap_buffer( renderer->gpu, buffer_map.buffer );
  }
  
  buffer_map = CRUDE_COMPOUNT_EMPTY( crude_gfx_map_buffer_parameters );
  buffer_map.buffer = pass->scene_renderer->meshes_materials_sb;
  buffer_map.offset = 0;
  buffer_map.size = 0;
  mesh_materials = CRUDE_CAST( crude_gfx_mesh_material_gpu*, crude_gfx_map_buffer( renderer->gpu, &buffer_map ) );
  if ( mesh_materials )
  {
    for ( uint32 i = 0; i < CRUDE_ARRAY_LENGTH( pass->scene_renderer->mesh_instances ); ++i )
    {
      copy_mesh_material_gpu_( pass->scene_renderer->mesh_instances[ i ].mesh, &mesh_materials[ i ] );
    }
    crude_gfx_unmap_buffer( renderer->gpu, buffer_map.buffer );
  }
  
  {
    crude_gfx_renderer_technique                          *meshlet_technique;
    crude_gfx_pipeline_handle                              pipeline;
    uint64                                                 meshlet_hashed_name;

    meshlet_hashed_name = crude_hash_string( "meshlet", 0 );
    meshlet_technique = CRUDE_HASHMAP_GET( renderer->resource_cache.techniques, meshlet_hashed_name )->value;
    pipeline = meshlet_technique->passes[ pass->scene_renderer->meshlet_technique_index ].pipeline;
    crude_gfx_cmd_bind_pipeline( cmd, pipeline );
    crude_gfx_cmd_bind_descriptor_set( cmd, pass->scene_renderer->mesh_shader_ds[ renderer->gpu->current_frame ] );
    crude_gfx_cmd_draw_mesh_task_indirect_count(
      cmd,
      pass->scene_renderer->mesh_task_indirect_commands_sb[ renderer->gpu->current_frame ],
      CRUDE_OFFSETOF( crude_gfx_mesh_draw_command_gpu, indirect_meshlet ),
      pass->scene_renderer->mesh_task_indirect_count_sb[ renderer->gpu->current_frame ],
      0,
      CRUDE_ARRAY_LENGTH( pass->scene_renderer->mesh_instances ),
      sizeof( crude_gfx_mesh_draw_command_gpu )
    );
  }
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
  _In_ crude_gfx_geometry_pass                            *pass
)
{
  crude_gfx_renderer_material *last_material = NULL;
  for ( uint32 mesh_index = 0; mesh_index < CRUDE_ARRAY_LENGTH( pass->scene_renderer->mesh_instances ); ++mesh_index )
  {
    crude_gfx_mesh_instance_cpu                           *mesh_instance;
    crude_gfx_mesh_cpu                                    *mesh;
  
    mesh_instance = &pass->scene_renderer->mesh_instances[ mesh_index ];
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
    ds_creation.name = "mesh_material_descriptor_set";
    crude_gfx_descriptor_set_creation_add_buffer( &ds_creation, pass->scene_renderer->scene_cb, 0u );
    crude_gfx_descriptor_set_creation_add_buffer( &ds_creation, mesh->material_buffer, 1u );
    ds_creation.layout = crude_gfx_get_descriptor_set_layout( cmd->gpu, mesh->material->technique->passes[ 0 ].pipeline, CRUDE_GFX_MATERIAL_DESCRIPTOR_SET_INDEX );

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

void
copy_mesh_material_gpu_
(
  _In_ crude_gfx_mesh_cpu const                           *mesh,
  _Out_ crude_gfx_mesh_material_gpu                       *gpu_mesh_material
)
{
  crude_transform const *transform = CRUDE_ENTITY_GET_IMMUTABLE_COMPONENT( mesh->node, crude_transform );
  crude_matrix model_to_world = crude_transform_node_to_world( mesh->node, transform );
  crude_store_float4x4a( &gpu_mesh_material->model_to_world, model_to_world ); 
  gpu_mesh_material->textures.x = mesh->albedo_texture_index;
  gpu_mesh_material->textures.y = mesh->roughness_texture_index;
  gpu_mesh_material->textures.z = mesh->normal_texture_index;
  gpu_mesh_material->textures.w = mesh->occlusion_texture_index;
  gpu_mesh_material->albedo_color_factor = mesh->albedo_color_factor;
  gpu_mesh_material->flags = mesh->flags;
  gpu_mesh_material->mesh_index = mesh->gpu_mesh_index;
  gpu_mesh_material->meshletes_count = mesh->meshlets_count;
  gpu_mesh_material->meshletes_offset = mesh->meshlets_offset;
}