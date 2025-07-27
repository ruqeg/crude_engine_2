#include <core/hash_map.h>
#include <graphics/scene_renderer.h>
#include <graphics/scene_renderer_resources.h>

#include <graphics/passes/mesh_culling_pass.h>

void
crude_gfx_mesh_culling_pass_initialize
(
  _In_ crude_gfx_mesh_culling_pass                        *pass,
  _In_ crude_gfx_scene_renderer                           *scene_renderer,
  _In_ bool                                                early_pass
)
{
  pass->scene_renderer = scene_renderer;
  pass->early_pass = early_pass;

  crude_gfx_pipeline_handle depth_pyramid_pipeline = crude_gfx_renderer_access_technique_pass_by_name( pass->scene_renderer->renderer, "culling", "mesh_culling" )->pipeline;
  pass->mesh_culling_descriptor_sets_layout_handle = crude_gfx_get_descriptor_set_layout( scene_renderer->renderer->gpu, depth_pyramid_pipeline, CRUDE_GFX_MATERIAL_DESCRIPTOR_SET_INDEX );

  for ( uint32 i = 0; i < CRUDE_GFX_MAX_SWAPCHAIN_IMAGES; ++i )
  {
    crude_gfx_descriptor_set_creation                    ds_creation;
    
    ds_creation = CRUDE_COMPOUNT_EMPTY( crude_gfx_descriptor_set_creation );
    ds_creation.layout = pass->mesh_culling_descriptor_sets_layout_handle;
    ds_creation.name = "meshlet_descriptor_set";
  
    crude_gfx_descriptor_set_creation_add_buffer( &ds_creation, scene_renderer->scene_cb, 0u );
    crude_gfx_descriptor_set_creation_add_buffer( &ds_creation, scene_renderer->meshes_materials_sb, 1u );
    crude_gfx_descriptor_set_creation_add_buffer( &ds_creation, scene_renderer->meshes_bounds_sb, 2u );

    if ( pass->early_pass )
    {
      crude_gfx_descriptor_set_creation_add_buffer( &ds_creation, scene_renderer->mesh_task_indirect_commands_early_sb[ i ], 10u );
      crude_gfx_descriptor_set_creation_add_buffer( &ds_creation, scene_renderer->mesh_task_indirect_commands_late_sb[ i ], 11u );
      crude_gfx_descriptor_set_creation_add_buffer( &ds_creation, scene_renderer->mesh_task_indirect_count_late_sb[ i ], 12u );
      crude_gfx_descriptor_set_creation_add_buffer( &ds_creation, scene_renderer->mesh_task_indirect_count_early_sb[ i ], 13u );
    }
    else
    {
      crude_gfx_descriptor_set_creation_add_buffer( &ds_creation, scene_renderer->mesh_task_indirect_commands_late_sb[ i ], 10u );
      crude_gfx_descriptor_set_creation_add_buffer( &ds_creation, scene_renderer->mesh_task_indirect_commands_early_sb[ i ], 11u );
      crude_gfx_descriptor_set_creation_add_buffer( &ds_creation, scene_renderer->mesh_task_indirect_count_early_sb[ i ], 12u );
      crude_gfx_descriptor_set_creation_add_buffer( &ds_creation, scene_renderer->mesh_task_indirect_count_late_sb[ i ], 13u );
    }
    
    pass->mesh_culling_descriptor_sets_handles[ i ] = crude_gfx_create_descriptor_set( scene_renderer->renderer->gpu, &ds_creation );
  }
}

void
crude_gfx_mesh_culling_pass_deinitialize
(
  _In_ crude_gfx_mesh_culling_pass                        *pass
)
{
}

void
crude_gfx_mesh_culling_pass_on_render_graph_registered
(
  _In_ crude_gfx_mesh_culling_pass                        *pass
)
{
  crude_gfx_device *gpu = pass->scene_renderer->renderer->gpu;
}

void
crude_gfx_mesh_culling_pass_render
(
  _In_ void                                               *ctx,
  _In_ crude_gfx_cmd_buffer                               *primary_cmd
)
{
  crude_gfx_device                                        *gpu;
  crude_gfx_buffer                                        *count_write_sb;
  crude_gfx_buffer                                        *commands_write_sb;
  crude_gfx_mesh_culling_pass                             *pass;
  crude_gfx_pipeline_handle                                mesh_culling_pipeline;
  uint32                                                   meshes_count;

  pass = CRUDE_REINTERPRET_CAST( crude_gfx_mesh_culling_pass*, ctx );

  if ( !pass->scene_renderer->total_meshes_count )
  {
    return;
  }

  gpu = pass->scene_renderer->renderer->gpu;
  mesh_culling_pipeline = crude_gfx_renderer_access_technique_pass_by_name( pass->scene_renderer->renderer, "culling", "mesh_culling" )->pipeline;

  crude_gfx_cmd_bind_pipeline( primary_cmd, mesh_culling_pipeline );
  
  if ( pass->early_pass )
  {
    count_write_sb = crude_gfx_access_buffer( gpu, pass->scene_renderer->mesh_task_indirect_count_early_sb[ gpu->current_frame ] );
    commands_write_sb = crude_gfx_access_buffer( gpu, pass->scene_renderer->mesh_task_indirect_commands_early_sb[ gpu->current_frame ] );
  }
  else
  {
    count_write_sb = crude_gfx_access_buffer( gpu, pass->scene_renderer->mesh_task_indirect_count_late_sb[ gpu->current_frame ] );
    commands_write_sb = crude_gfx_access_buffer( gpu, pass->scene_renderer->mesh_task_indirect_commands_late_sb[ gpu->current_frame ] );
  }

  crude_gfx_cmd_add_buffer_barrier( primary_cmd, count_write_sb, CRUDE_GFX_RESOURCE_STATE_INDIRECT_ARGUMENT, CRUDE_GFX_RESOURCE_STATE_UNORDERED_ACCESS );
  crude_gfx_cmd_add_buffer_barrier( primary_cmd, commands_write_sb, CRUDE_GFX_RESOURCE_STATE_INDIRECT_ARGUMENT, CRUDE_GFX_RESOURCE_STATE_UNORDERED_ACCESS );

  crude_gfx_cmd_bind_descriptor_set( primary_cmd, pass->mesh_culling_descriptor_sets_handles[ primary_cmd->gpu->current_frame ] );
  
  if ( pass->early_pass )
  {
    meshes_count = pass->scene_renderer->total_meshes_count;
  }
  else
  {
    crude_gfx_mesh_draw_counts_gpu                        *mesh_draw_counts_early;
    crude_gfx_map_buffer_parameters                        buffer_map;
 
    buffer_map = CRUDE_COMPOUNT_EMPTY( crude_gfx_map_buffer_parameters );
    buffer_map.buffer = pass->scene_renderer->mesh_task_indirect_count_early_sb[ gpu->current_frame ];
    buffer_map.offset = 0;
    buffer_map.size = sizeof( crude_gfx_mesh_draw_counts_gpu );
    mesh_draw_counts_early = CRUDE_CAST( crude_gfx_mesh_draw_counts_gpu*, crude_gfx_map_buffer( gpu, &buffer_map ) );

    if ( mesh_draw_counts_early )
    {
      meshes_count = mesh_draw_counts_early->opaque_mesh_culled_count;
      crude_gfx_unmap_buffer( gpu, pass->scene_renderer->mesh_task_indirect_count_early_sb[ gpu->current_frame ] );
    }
    else
    {
      CRUDE_ASSERT( false && "damn" );
    }
  }
  
  crude_gfx_cmd_dispatch( primary_cmd, ( meshes_count + 63u ) / 64u, 1u, 1u );

  crude_gfx_cmd_add_buffer_barrier( primary_cmd, count_write_sb, CRUDE_GFX_RESOURCE_STATE_UNORDERED_ACCESS, CRUDE_GFX_RESOURCE_STATE_INDIRECT_ARGUMENT );
  crude_gfx_cmd_add_buffer_barrier( primary_cmd, commands_write_sb, CRUDE_GFX_RESOURCE_STATE_UNORDERED_ACCESS, CRUDE_GFX_RESOURCE_STATE_INDIRECT_ARGUMENT );
}

void
crude_gfx_mesh_culling_pass_register
(
  _In_ crude_gfx_mesh_culling_pass                        *pass
)
{
}

static void
crude_gfx_mesh_culling_pass_on_resize
(
  _In_ void                                               *ctx,
  _In_ crude_gfx_device                                   *gpu,
  _In_ uint32                                              new_width,
  _In_ uint32                                              new_height
)
{
}

crude_gfx_render_graph_pass_container
crude_gfx_mesh_culling_pass_pack
(
  _In_ crude_gfx_mesh_culling_pass                        *pass
)
{
  crude_gfx_render_graph_pass_container container;
  container.ctx = pass;
  container.on_resize = crude_gfx_mesh_culling_pass_on_resize;
  container.render = crude_gfx_mesh_culling_pass_render;
  return container;
}