#include <core/hash_map.h>
#include <graphics/scene_renderer.h>
#include <graphics/scene_renderer_resources.h>

#include <graphics/passes/culling_late_pass.h>

void
crude_gfx_culling_late_pass_initialize
(
  _In_ crude_gfx_culling_late_pass                        *pass,
  _In_ crude_gfx_scene_renderer                           *scene_renderer
)
{
  pass->scene_renderer = scene_renderer;
  
  for ( uint32 i = 0; i < CRUDE_GFX_MAX_SWAPCHAIN_IMAGES; ++i )
  {
    pass->culling_late_ds[ i ] = CRUDE_GFX_DESCRIPTOR_SET_HANDLE_INVALID;
  }

  crude_gfx_culling_late_pass_on_techniques_reloaded( pass );
}

void
crude_gfx_culling_late_pass_deinitialize
(
  _In_ crude_gfx_culling_late_pass                        *pass
)
{
  for ( uint32 i = 0; i < CRUDE_GFX_MAX_SWAPCHAIN_IMAGES; ++i )
  {
    crude_gfx_destroy_descriptor_set( pass->scene_renderer->renderer->gpu, pass->culling_late_ds[ i ] );
  }
}

void
crude_gfx_culling_late_pass_render
(
  _In_ void                                               *ctx,
  _In_ crude_gfx_cmd_buffer                               *primary_cmd
)
{
  crude_gfx_device                                        *gpu;
  crude_gfx_culling_late_pass                             *pass;
  crude_gfx_pipeline_handle                                mesh_culling_pipeline;

  pass = CRUDE_REINTERPRET_CAST( crude_gfx_culling_late_pass*, ctx );

  if ( !pass->scene_renderer->total_meshes_instances_count )
  {
    return;
  }

  gpu = pass->scene_renderer->renderer->gpu;
  mesh_culling_pipeline = crude_gfx_renderer_access_technique_pass_by_name( pass->scene_renderer->renderer, "compute", "culling" )->pipeline;

  crude_gfx_cmd_bind_pipeline( primary_cmd, mesh_culling_pipeline );
  
  crude_gfx_cmd_add_buffer_barrier( primary_cmd, pass->scene_renderer->mesh_task_indirect_count_late_sb[ gpu->current_frame ], CRUDE_GFX_RESOURCE_STATE_INDIRECT_ARGUMENT, CRUDE_GFX_RESOURCE_STATE_UNORDERED_ACCESS );
  crude_gfx_cmd_add_buffer_barrier( primary_cmd, pass->scene_renderer->mesh_task_indirect_commands_late_sb[ gpu->current_frame ], CRUDE_GFX_RESOURCE_STATE_INDIRECT_ARGUMENT, CRUDE_GFX_RESOURCE_STATE_UNORDERED_ACCESS );

  crude_gfx_cmd_bind_descriptor_set( primary_cmd, pass->culling_late_ds[ primary_cmd->gpu->current_frame ] );
  crude_gfx_cmd_dispatch( primary_cmd, ( pass->scene_renderer->total_meshes_instances_count + 63u ) / 64u, 1u, 1u );

  crude_gfx_cmd_add_buffer_barrier( primary_cmd, pass->scene_renderer->mesh_task_indirect_count_late_sb[ gpu->current_frame ], CRUDE_GFX_RESOURCE_STATE_UNORDERED_ACCESS, CRUDE_GFX_RESOURCE_STATE_INDIRECT_ARGUMENT );
  crude_gfx_cmd_add_buffer_barrier( primary_cmd, pass->scene_renderer->mesh_task_indirect_commands_late_sb[ gpu->current_frame ], CRUDE_GFX_RESOURCE_STATE_UNORDERED_ACCESS, CRUDE_GFX_RESOURCE_STATE_INDIRECT_ARGUMENT );
}

void
crude_gfx_culling_late_pass_on_techniques_reloaded
(
  _In_ void                                               *ctx
)
{
  crude_gfx_culling_late_pass                             *pass;
  crude_gfx_pipeline_handle                                culling_pipeline;
  crude_gfx_descriptor_set_layout_handle                   culling_descriptor_sets_layout_handle;
  
  pass = CRUDE_REINTERPRET_CAST( crude_gfx_culling_late_pass*, ctx );

  culling_pipeline = crude_gfx_renderer_access_technique_pass_by_name( pass->scene_renderer->renderer, "compute", "culling" )->pipeline;
  culling_descriptor_sets_layout_handle = crude_gfx_get_descriptor_set_layout( pass->scene_renderer->renderer->gpu, culling_pipeline, CRUDE_GFX_MATERIAL_DESCRIPTOR_SET_INDEX );
  
  for ( uint32 i = 0; i < CRUDE_GFX_MAX_SWAPCHAIN_IMAGES; ++i )
  {
    if ( CRUDE_RESOURCE_HANDLE_IS_VALID( pass->culling_late_ds[ i ] ) )
    {
      crude_gfx_destroy_descriptor_set( pass->scene_renderer->renderer->gpu, pass->culling_late_ds[ i ] );
    }
  }

  for ( uint32 i = 0; i < CRUDE_GFX_MAX_SWAPCHAIN_IMAGES; ++i )
  {
    crude_gfx_descriptor_set_creation                    ds_creation;
    
    ds_creation = crude_gfx_descriptor_set_creation_empty();
    ds_creation.layout = culling_descriptor_sets_layout_handle;
    ds_creation.name = "meshlet_descriptor_set";
  
    crude_gfx_descriptor_set_creation_add_buffer( &ds_creation, pass->scene_renderer->scene_cb, 0u );
    crude_gfx_descriptor_set_creation_add_buffer( &ds_creation, pass->scene_renderer->meshes_draws_sb, 1u );
    crude_gfx_descriptor_set_creation_add_buffer( &ds_creation, pass->scene_renderer->meshes_instances_draws_sb, 2u );
    crude_gfx_descriptor_set_creation_add_buffer( &ds_creation, pass->scene_renderer->meshes_bounds_sb, 3u );
    crude_gfx_descriptor_set_creation_add_buffer( &ds_creation, pass->scene_renderer->mesh_task_indirect_commands_late_sb[ i ], 4u );
    crude_gfx_descriptor_set_creation_add_buffer( &ds_creation, pass->scene_renderer->mesh_task_indirect_commands_early_sb[ i ], 5u );
    crude_gfx_descriptor_set_creation_add_buffer( &ds_creation, pass->scene_renderer->mesh_task_indirect_count_early_sb[ i ], 6u );
    crude_gfx_descriptor_set_creation_add_buffer( &ds_creation, pass->scene_renderer->mesh_task_indirect_count_late_sb[ i ], 7u );
    crude_gfx_scene_renderer_add_debug_resources_to_descriptor_set_creation( &ds_creation, pass->scene_renderer, i );
    
    pass->culling_late_ds[ i ] = crude_gfx_create_descriptor_set( pass->scene_renderer->renderer->gpu, &ds_creation );
  }
}

void
crude_gfx_culling_late_pass_register
(
  _In_ crude_gfx_culling_late_pass                        *pass
)
{
}

crude_gfx_render_graph_pass_container
crude_gfx_culling_late_pass_pack
(
  _In_ crude_gfx_culling_late_pass                        *pass
)
{
  crude_gfx_render_graph_pass_container container = crude_gfx_render_graph_pass_container_empty();
  container.ctx = pass;
  container.render = crude_gfx_culling_late_pass_render;
  container.on_techniques_reloaded = crude_gfx_culling_late_pass_on_techniques_reloaded;
  return container;
}