#include <TaskScheduler_c.h>

#include <core/profiler.h>
#include <core/hash_map.h>
#include <graphics/scene_renderer.h>

#include <graphics/passes/geometry_pass.h>

#define _PARALLEL_RECORDINGS                               ( 4 )

typedef struct secondary_draw_task_container
{
  _In_ crude_gfx_geometry_pass                            *pass;
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
secondary_draw_task_
(
  _In_ uint32_t                                            start,
  _In_ uint32_t                                            end,
  _In_ uint32_t                                            thread_num,
  _In_ void                                               *ctx
);

static void
draw_scene_
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
  crude_gfx_renderer_material                             *main_material;
 
  pass->scene_renderer = scene_renderer;
  pass->flags = flags;

  {
    uint64 main_technique_name_hashed = crude_hash_string( "main", 0 );
    uint64 main_technique_index = CRUDE_HASHMAP_GET_INDEX( scene_renderer->renderer->resource_cache.techniques, main_technique_name_hashed );
    if ( main_technique_index < 0)
    {
      CRUDE_ASSERT( false );
      return;
    }

    crude_gfx_renderer_technique *main_technique = scene_renderer->renderer->resource_cache.techniques[ main_technique_index ].value;

    crude_gfx_renderer_material_creation material_creation = CRUDE_COMPOUNT_EMPTY( crude_gfx_renderer_material_creation );
    material_creation.technique = main_technique;
    material_creation.name = "material_no_cull";
    material_creation.render_index = 0;
    main_material = crude_gfx_renderer_create_material( scene_renderer->renderer, &material_creation );
  }

  CRUDE_ARRAY_INITIALIZE_WITH_CAPACITY( pass->mesh_instances, 16, scene_renderer->renderer->allocator_container );
  for ( size_t i = 0; i < CRUDE_ARRAY_LENGTH( scene_renderer->meshes ); ++i )
  {
    if ( crude_gfx_mesh_is_transparent( &scene_renderer->meshes[ i ] ) )
    {
      continue;
    }
    scene_renderer->meshes[ i ].material = main_material;
 
    crude_gfx_mesh_instance_cpu mesh_instance;
    mesh_instance.mesh = &scene_renderer->meshes[ i ];
    mesh_instance.material_pass_index = 0;
    CRUDE_ARRAY_PUSH( pass->mesh_instances, mesh_instance );
  }

  if ( scene_renderer->renderer->gpu->mesh_shaders_extension_present )
  {
    uint64 meshlet_hashed = crude_hash_string( "meshlet", 0 );
    crude_gfx_renderer_technique *main_technique = CRUDE_HASHMAP_GET( scene_renderer->renderer->resource_cache.techniques, meshlet_hashed )->value;
    pass->meshlet_technique_index = crude_gfx_renderer_technique_get_pass_index( main_technique, "main" );
  }
}

void
crude_gfx_geometry_pass_deinitialize
(
  _In_ void                                               *ctx,
  _In_ crude_gfx_cmd_buffer                               *primary_cmd
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
    {
      crude_gfx_map_buffer_parameters cb_map = CRUDE_COMPOUNT_EMPTY( crude_gfx_map_buffer_parameters );
      cb_map.buffer = pass->scene_renderer->mesh_task_indirect_commands_sb[ pass->scene_renderer->renderer->gpu->current_frame ];
      cb_map.offset = 0;
      cb_map.size = sizeof( crude_gfx_mesh_draw_command );
      crude_gfx_mesh_draw_command *draw_data = CRUDE_CAST( crude_gfx_mesh_draw_command*, crude_gfx_map_buffer( pass->scene_renderer->renderer->gpu, &cb_map ) );
      if ( draw_data )
      {
        draw_data->indirect_meshlet.groupCountX = ceil( CRUDE_ARRAY_LENGTH( pass->scene_renderer->meshlets ) / 128.0 );
        draw_data->indirect_meshlet.groupCountY = 1;
        draw_data->indirect_meshlet.groupCountZ = 1;
        crude_gfx_unmap_buffer( pass->scene_renderer->renderer->gpu, cb_map.buffer );
      }
    }
    
    {
      crude_gfx_map_buffer_parameters cb_map = CRUDE_COMPOUNT_EMPTY( crude_gfx_map_buffer_parameters );
      cb_map.buffer = pass->scene_renderer->mesh_task_indirect_count_sb[ pass->scene_renderer->renderer->gpu->current_frame ];
      cb_map.offset = 0;
      cb_map.size = sizeof( crude_gfx_mesh_draw_counts );
      crude_gfx_mesh_draw_counts *draw_data = CRUDE_CAST( crude_gfx_mesh_draw_counts*, crude_gfx_map_buffer( pass->scene_renderer->renderer->gpu, &cb_map ) );
      if ( draw_data )
      {
        draw_data->opaque_mesh_visible_count = 1;
        crude_gfx_unmap_buffer( pass->scene_renderer->renderer->gpu, cb_map.buffer );
      }
    }
    
    {
      crude_gfx_map_buffer_parameters cb_map = CRUDE_COMPOUNT_EMPTY( crude_gfx_map_buffer_parameters );
      cb_map.buffer = pass->scene_renderer->meshes_materials_sb;
      cb_map.offset = 0;
      cb_map.size = 0;
      crude_gfx_mesh_material_gpu *meshes_materials = CRUDE_CAST( crude_gfx_mesh_material_gpu*, crude_gfx_map_buffer( pass->scene_renderer->renderer->gpu, &cb_map ) );
      if ( meshes_materials )
      {
        for ( uint32 i = 0; i < CRUDE_ARRAY_LENGTH( pass->scene_renderer->meshes ); ++i )
        {
          copy_mesh_material_gpu_( &pass->scene_renderer->meshes[i ], &meshes_materials[ i ] );
        }
        crude_gfx_unmap_buffer( pass->scene_renderer->renderer->gpu, cb_map.buffer );
      }
    }

    crude_gfx_renderer *renderer = pass->scene_renderer->renderer;
    uint64 current_frame_index = renderer->gpu->current_frame;
    uint64 meshlet_hashed_name = crude_hash_string( "meshlet", 0 );
    crude_gfx_renderer_technique *meshlet_technique = CRUDE_HASHMAP_GET( renderer->resource_cache.techniques, meshlet_hashed_name )->value;
    crude_gfx_pipeline_handle pipeline = meshlet_technique->passes[ pass->meshlet_technique_index ].pipeline;
    crude_gfx_cmd_bind_pipeline( primary_cmd, pipeline );
    crude_gfx_cmd_bind_descriptor_set( primary_cmd, pass->scene_renderer->mesh_shader_ds[ renderer->gpu->current_frame ] );
    crude_gfx_cmd_draw_mesh_task_indirect_count( primary_cmd, pass->scene_renderer->mesh_task_indirect_commands_sb[ current_frame_index ], CRUDE_OFFSETOF( crude_gfx_mesh_draw_command, indirect_meshlet ), pass->scene_renderer->mesh_task_indirect_count_sb[ current_frame_index ], 0, 1u/*render_scene->mesh_instances.size*/, sizeof( crude_gfx_mesh_draw_command ) );
  }
  else if ( pass->flags & CRUDE_GFX_GEOMETRY_PASS_SECONDARY_BIT )
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
      
      secondary_draw_tasks[ i ] = enkiCreateTaskSet( CRUDE_REINTERPRET_CAST( enkiTaskScheduler*, pass->scene_renderer->task_scheduler ), secondary_draw_task_ );
      enkiSetArgsTaskSet( secondary_draw_tasks[ i ], &secondary_draw_tasks_data[ i ] );
      enkiAddTaskSet( CRUDE_REINTERPRET_CAST( enkiTaskScheduler*, pass->scene_renderer->task_scheduler ), secondary_draw_tasks[ i ] );
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
      enkiWaitForTaskSet( CRUDE_REINTERPRET_CAST( enkiTaskScheduler*, pass->scene_renderer->task_scheduler ), secondary_draw_tasks[ i ] );
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
  
  crude_gfx_cmd_buffer *secondary_cmd = crude_gfx_get_secondary_cmd( secondary_draw_task->pass->scene_renderer->renderer->gpu, thread_num );
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
  _In_ crude_gfx_geometry_pass                            *pass
)
{
  crude_gfx_renderer_material *last_material = NULL;
  for ( uint32 mesh_index = 0; mesh_index < CRUDE_ARRAY_LENGTH( pass->mesh_instances ); ++mesh_index )
  {
    crude_gfx_mesh_instance_cpu                           *mesh_instance;
    crude_gfx_mesh_cpu                                    *mesh;
  
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
  gpu_mesh_material->textures.x = mesh->albedo_texture_index;
  gpu_mesh_material->textures.y = mesh->roughness_texture_index;
  gpu_mesh_material->textures.z = mesh->normal_texture_index;
  gpu_mesh_material->textures.w = mesh->occlusion_texture_index;
  gpu_mesh_material->albedo_color_factor = mesh->albedo_color_factor;
  gpu_mesh_material->flags = mesh->flags;
  gpu_mesh_material->mesh_index = mesh->gpu_mesh_index;
}