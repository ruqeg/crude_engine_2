#include <engine/core/profiler.h>
#include <engine/core/hash_map.h>
#include <engine/scene/scene_components.h>
#include <engine/graphics/scene_renderer.h>

#include <engine/graphics/passes/gbuffer_early_pass.h>

void
crude_gfx_gbuffer_early_pass_initialize
(
  _In_ crude_gfx_gbuffer_early_pass                       *pass,
  _In_ crude_gfx_scene_renderer                           *scene_renderer
)
{
  pass->scene_renderer = scene_renderer;
  
  for ( uint32 i = 0; i < CRUDE_GRAPHICS_MAX_SWAPCHAIN_IMAGES; ++i )
  {
    pass->meshlet_early_ds[ i ] = CRUDE_GFX_DESCRIPTOR_SET_HANDLE_INVALID;
  }

  crude_gfx_gbuffer_early_pass_on_techniques_reloaded( pass );
}

void
crude_gfx_gbuffer_early_pass_deinitialize
(
  _In_ crude_gfx_gbuffer_early_pass                       *pass
)
{
  for ( uint32 i = 0; i < CRUDE_GRAPHICS_MAX_SWAPCHAIN_IMAGES; ++i )
  {
    crude_gfx_destroy_descriptor_set( pass->scene_renderer->gpu, pass->meshlet_early_ds[ i ] );
  }
}

void
crude_gfx_gbuffer_early_pass_render
(
  _In_ void                                               *ctx,
  _In_ crude_gfx_cmd_buffer                               *primary_cmd
)
{
  crude_gfx_gbuffer_early_pass                            *pass;
  crude_gfx_device                                        *gpu;
  crude_gfx_pipeline_handle                                pipeline;
 
  pass = CRUDE_REINTERPRET_CAST( crude_gfx_gbuffer_early_pass*, ctx );

  gpu = pass->scene_renderer->gpu;

  pipeline = crude_gfx_access_technique_pass_by_name( gpu, "deferred_meshlet", "deferred_meshlet" )->pipeline;
  crude_gfx_cmd_bind_pipeline( primary_cmd, pipeline );
  crude_gfx_cmd_bind_descriptor_set( primary_cmd, pass->meshlet_early_ds[ gpu->current_frame ] );
  crude_gfx_cmd_draw_mesh_task_indirect_count(
    primary_cmd,
    pass->scene_renderer->mesh_task_indirect_commands_early_sb[ gpu->current_frame ],
    CRUDE_OFFSETOF( crude_gfx_mesh_draw_command_gpu, indirect_meshlet ),
    pass->scene_renderer->mesh_task_indirect_count_early_sb[ gpu->current_frame ],
    0,
    pass->scene_renderer->total_meshes_instances_count,
    sizeof( crude_gfx_mesh_draw_command_gpu )
  );
}

void
crude_gfx_gbuffer_early_pass_on_techniques_reloaded
(
  _In_ void                                               *ctx
)
{
  crude_gfx_gbuffer_early_pass                            *pass;
  crude_gfx_technique_pass                                *meshlet_pass;
  crude_gfx_descriptor_set_layout_handle                   layout;
  
  pass = CRUDE_REINTERPRET_CAST( crude_gfx_gbuffer_early_pass*, ctx );

  meshlet_pass = crude_gfx_access_technique_pass_by_name( pass->scene_renderer->gpu, "deferred_meshlet", "deferred_meshlet" );
  layout = crude_gfx_get_descriptor_set_layout( pass->scene_renderer->gpu, meshlet_pass->pipeline, CRUDE_GRAPHICS_MATERIAL_DESCRIPTOR_SET_INDEX );
  
  for ( uint32 i = 0; i < CRUDE_GRAPHICS_MAX_SWAPCHAIN_IMAGES; ++i )
  {
    if ( CRUDE_RESOURCE_HANDLE_IS_VALID( pass->meshlet_early_ds[ i ] ) )
    {
      crude_gfx_destroy_descriptor_set( pass->scene_renderer->gpu, pass->meshlet_early_ds[ i ] );
    }
  }

  for ( uint32 i = 0; i < CRUDE_GRAPHICS_MAX_SWAPCHAIN_IMAGES; ++i )
  {
    crude_gfx_descriptor_set_creation                    ds_creation;
    
    ds_creation = crude_gfx_descriptor_set_creation_empty();
    ds_creation.layout = layout;
    ds_creation.name = "meshlet_descriptor_set";
  
    crude_gfx_descriptor_set_creation_add_buffer( &ds_creation, pass->scene_renderer->scene_cb, 0u );
    crude_gfx_descriptor_set_creation_add_buffer( &ds_creation, pass->scene_renderer->model_renderer_resources_manager->meshes_draws_sb, 1u );
    crude_gfx_descriptor_set_creation_add_buffer( &ds_creation, pass->scene_renderer->meshes_instances_draws_sb, 2u );
    crude_gfx_descriptor_set_creation_add_buffer( &ds_creation, pass->scene_renderer->model_renderer_resources_manager->meshlets_sb, 3u );
    crude_gfx_descriptor_set_creation_add_buffer( &ds_creation, pass->scene_renderer->model_renderer_resources_manager->meshlets_vertices_sb, 4u );
    crude_gfx_descriptor_set_creation_add_buffer( &ds_creation, pass->scene_renderer->model_renderer_resources_manager->meshlets_triangles_indices_sb, 5u );
    crude_gfx_descriptor_set_creation_add_buffer( &ds_creation, pass->scene_renderer->model_renderer_resources_manager->meshlets_vertices_indices_sb, 6u );
    crude_gfx_descriptor_set_creation_add_buffer( &ds_creation, pass->scene_renderer->mesh_task_indirect_count_early_sb[ i ], 7u );
    crude_gfx_descriptor_set_creation_add_buffer( &ds_creation, pass->scene_renderer->mesh_task_indirect_commands_early_sb[ i ], 8u );
    crude_gfx_scene_renderer_add_debug_resources_to_descriptor_set_creation( &ds_creation, pass->scene_renderer, i );
    
    pass->meshlet_early_ds[ i ] = crude_gfx_create_descriptor_set( pass->scene_renderer->gpu, &ds_creation );
  }
}

crude_gfx_render_graph_pass_container
crude_gfx_gbuffer_early_pass_pack
(
  _In_ crude_gfx_gbuffer_early_pass                       *pass
)
{
  crude_gfx_render_graph_pass_container container = crude_gfx_render_graph_pass_container_empty();
  container.ctx = pass;
  container.render = crude_gfx_gbuffer_early_pass_render;
  container.on_techniques_reloaded = crude_gfx_gbuffer_early_pass_on_techniques_reloaded;
  return container;
}