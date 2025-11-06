#include <graphics/scene_renderer.h>

#include <graphics/passes/collision_visualizer_pass.h>

void
crude_gfx_collision_visualizer_pass_initialize
(
  _In_ crude_gfx_collision_visualizer_pass                *pass,
  _In_ crude_gfx_scene_renderer                           *scene_renderer
)
{
  pass->scene_renderer = scene_renderer;
  
  for ( uint32 i = 0; i < CRUDE_GRAPHICS_MAX_SWAPCHAIN_IMAGES; ++i )
  {
    pass->collision_visualizer_ds[ i ] = CRUDE_GFX_DESCRIPTOR_SET_HANDLE_INVALID;
  }

  crude_gfx_collision_visualizer_pass_on_techniques_reloaded( pass );
}

void
crude_gfx_collision_visualizer_pass_deinitialize
(
  _In_ crude_gfx_collision_visualizer_pass                *pass
)
{
  for ( uint32 i = 0; i < CRUDE_GRAPHICS_MAX_SWAPCHAIN_IMAGES; ++i )
  {
    crude_gfx_destroy_descriptor_set( pass->scene_renderer->gpu, pass->collision_visualizer_ds[ i ] );
  }
}

void
crude_gfx_collision_visualizer_pass_render
(
  _In_ void                                               *ctx,
  _In_ crude_gfx_cmd_buffer                               *primary_cmd
)
{
  crude_gfx_collision_visualizer_pass                     *pass;
  crude_gfx_device                                        *gpu;
  crude_gfx_pipeline_handle                                pipeline;
 
  pass = CRUDE_REINTERPRET_CAST( crude_gfx_collision_visualizer_pass*, ctx );

  gpu = pass->scene_renderer->gpu;

  pipeline = crude_gfx_access_technique_pass_by_name( gpu, "deferred_meshlet", "collision_visualizer" )->pipeline;
  crude_gfx_cmd_bind_pipeline( primary_cmd, pipeline );
  crude_gfx_cmd_bind_descriptor_set( primary_cmd, pass->collision_visualizer_ds[ gpu->current_frame ] );
  crude_gfx_cmd_draw_mesh_task( primary_cmd, ( pass->scene_renderer->total_collision_meshes_instances_count + 31 ) / 32, 1, 1 );
}

void
crude_gfx_collision_visualizer_pass_on_techniques_reloaded
(
  _In_ void                                               *ctx
)
{
  crude_gfx_collision_visualizer_pass                     *pass;
  crude_gfx_technique_pass                                *collision_visualizer_pass;
  crude_gfx_descriptor_set_layout_handle                   collision_visualizer_dsl;
  
  pass = CRUDE_REINTERPRET_CAST( crude_gfx_collision_visualizer_pass*, ctx );

  collision_visualizer_pass = crude_gfx_access_technique_pass_by_name( pass->scene_renderer->gpu, "deferred_meshlet", "collision_visualizer" );
  collision_visualizer_dsl = crude_gfx_get_descriptor_set_layout( pass->scene_renderer->gpu, collision_visualizer_pass->pipeline, CRUDE_GRAPHICS_MATERIAL_DESCRIPTOR_SET_INDEX );
  
  for ( uint32 i = 0; i < CRUDE_GRAPHICS_MAX_SWAPCHAIN_IMAGES; ++i )
  {
    if ( CRUDE_RESOURCE_HANDLE_IS_VALID( pass->collision_visualizer_ds[ i ] ) )
    {
      crude_gfx_destroy_descriptor_set( pass->scene_renderer->gpu, pass->collision_visualizer_ds[ i ] );
    }
  }

  for ( uint32 i = 0; i < CRUDE_GRAPHICS_MAX_SWAPCHAIN_IMAGES; ++i )
  {
    crude_gfx_descriptor_set_creation                    ds_creation;
    
    ds_creation = crude_gfx_descriptor_set_creation_empty();
    ds_creation.layout = collision_visualizer_dsl;
    ds_creation.name = "collision_visualizer_ds";
  
    crude_gfx_descriptor_set_creation_add_buffer( &ds_creation, pass->scene_renderer->scene_cb, 0u );
    crude_gfx_descriptor_set_creation_add_buffer( &ds_creation, pass->scene_renderer->model_renderer_resources_manager->meshes_draws_sb, 1u );
    crude_gfx_descriptor_set_creation_add_buffer( &ds_creation, pass->scene_renderer->collision_meshes_instances_draws_sb, 2u );
    crude_gfx_descriptor_set_creation_add_buffer( &ds_creation, pass->scene_renderer->model_renderer_resources_manager->meshlets_sb, 3u );
    crude_gfx_descriptor_set_creation_add_buffer( &ds_creation, pass->scene_renderer->model_renderer_resources_manager->meshlets_vertices_sb, 4u );
    crude_gfx_descriptor_set_creation_add_buffer( &ds_creation, pass->scene_renderer->model_renderer_resources_manager->meshlets_triangles_indices_sb, 5u );
    crude_gfx_descriptor_set_creation_add_buffer( &ds_creation, pass->scene_renderer->model_renderer_resources_manager->meshlets_vertices_indices_sb, 6u );
    crude_gfx_scene_renderer_add_debug_resources_to_descriptor_set_creation( &ds_creation, pass->scene_renderer, i );
    
    pass->collision_visualizer_ds[ i ] = crude_gfx_create_descriptor_set( pass->scene_renderer->gpu, &ds_creation );
  }
}

crude_gfx_render_graph_pass_container
crude_gfx_collision_visualizer_pass_pack
(
  _In_ crude_gfx_collision_visualizer_pass                *pass
)
{
  crude_gfx_render_graph_pass_container container = crude_gfx_render_graph_pass_container_empty();
  container.ctx = pass;
  container.render = crude_gfx_collision_visualizer_pass_render;
  container.on_techniques_reloaded = crude_gfx_collision_visualizer_pass_on_techniques_reloaded;
  return container;
}