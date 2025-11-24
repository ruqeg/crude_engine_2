#include <engine/graphics/scene_renderer.h>

#include <engine/graphics/passes/transparent_pass.h>

void
crude_gfx_transparent_pass_initialize
(
  _In_ crude_gfx_transparent_pass                         *pass,
  _In_ crude_gfx_scene_renderer                           *scene_renderer
)
{
  pass->scene_renderer = scene_renderer;
  
  for ( uint32 i = 0; i < CRUDE_GRAPHICS_MAX_SWAPCHAIN_IMAGES; ++i )
  {
    pass->transparent_ds[ i ] = CRUDE_GFX_DESCRIPTOR_SET_HANDLE_INVALID;
  }

  crude_gfx_transparent_pass_on_techniques_reloaded( pass );
}

void
crude_gfx_transparent_pass_deinitialize
(
  _In_ crude_gfx_transparent_pass                         *pass
)
{
  for ( uint32 i = 0; i < CRUDE_GRAPHICS_MAX_SWAPCHAIN_IMAGES; ++i )
  {
    crude_gfx_destroy_descriptor_set( pass->scene_renderer->gpu, pass->transparent_ds[ i ] );
  }
}

void
crude_gfx_transparent_pass_render
(
  _In_ void                                               *ctx,
  _In_ crude_gfx_cmd_buffer                               *primary_cmd
)
{
  crude_gfx_transparent_pass                              *pass;
  crude_gfx_device                                        *gpu;
  crude_gfx_pipeline_handle                                pipeline;
  XMFLOAT2                                                 inv_radiance_texture_resolution;

  pass = CRUDE_REINTERPRET_CAST( crude_gfx_transparent_pass*, ctx );

  gpu = pass->scene_renderer->gpu;

  pipeline = crude_gfx_access_technique_pass_by_name( gpu, "deferred_meshlet", "transparent_no_cull" )->pipeline;
  crude_gfx_cmd_bind_pipeline( primary_cmd, pipeline );
  crude_gfx_cmd_bind_descriptor_set( primary_cmd, pass->transparent_ds[ gpu->current_frame ] );
  inv_radiance_texture_resolution.x = 1.f / gpu->vk_swapchain_width;
  inv_radiance_texture_resolution.y = 1.f / gpu->vk_swapchain_height;
  crude_gfx_cmd_push_constant( primary_cmd, &inv_radiance_texture_resolution, sizeof( inv_radiance_texture_resolution ) );
  crude_gfx_cmd_draw_mesh_task_indirect_count(
    primary_cmd,
    pass->scene_renderer->mesh_task_indirect_commands_early_sb[ gpu->current_frame ],
    CRUDE_OFFSETOF( crude_gfx_mesh_draw_command_gpu, indirect_meshlet ),
    pass->scene_renderer->mesh_task_indirect_count_early_sb[ gpu->current_frame ],
    CRUDE_OFFSETOF( crude_gfx_mesh_draw_counts_gpu, transparent_mesh_visible_count ),
    pass->scene_renderer->total_visible_meshes_instances_count,
    sizeof( crude_gfx_mesh_draw_command_gpu ) );
}

void
crude_gfx_transparent_pass_on_techniques_reloaded
(
  _In_ void                                               *ctx
)
{
  crude_gfx_transparent_pass                              *pass;
  crude_gfx_technique_pass                                *collision_visualizer_pass;
  crude_gfx_descriptor_set_layout_handle                   transparent_dsl;
  
  pass = CRUDE_REINTERPRET_CAST( crude_gfx_transparent_pass*, ctx );

  collision_visualizer_pass = crude_gfx_access_technique_pass_by_name( pass->scene_renderer->gpu, "deferred_meshlet", "transparent_no_cull" );
  transparent_dsl = crude_gfx_get_descriptor_set_layout( pass->scene_renderer->gpu, collision_visualizer_pass->pipeline, CRUDE_GRAPHICS_MATERIAL_DESCRIPTOR_SET_INDEX );
  
  for ( uint32 i = 0; i < CRUDE_GRAPHICS_MAX_SWAPCHAIN_IMAGES; ++i )
  {
    if ( CRUDE_RESOURCE_HANDLE_IS_VALID( pass->transparent_ds[ i ] ) )
    {
      crude_gfx_destroy_descriptor_set( pass->scene_renderer->gpu, pass->transparent_ds[ i ] );
    }
  }

  for ( uint32 i = 0; i < CRUDE_GRAPHICS_MAX_SWAPCHAIN_IMAGES; ++i )
  {
    crude_gfx_descriptor_set_creation                    ds_creation;
    
    ds_creation = crude_gfx_descriptor_set_creation_empty();
    ds_creation.layout = transparent_dsl;
    ds_creation.name = "transparent_ds";
  
    crude_gfx_descriptor_set_creation_add_buffer( &ds_creation, pass->scene_renderer->scene_cb, 0u );
    crude_gfx_descriptor_set_creation_add_buffer( &ds_creation, pass->scene_renderer->model_renderer_resources_manager->meshes_draws_sb, 1u );
    crude_gfx_descriptor_set_creation_add_buffer( &ds_creation, pass->scene_renderer->meshes_instances_draws_sb, 2u );
    crude_gfx_descriptor_set_creation_add_buffer( &ds_creation, pass->scene_renderer->model_renderer_resources_manager->meshlets_sb, 3u );
    crude_gfx_descriptor_set_creation_add_buffer( &ds_creation, pass->scene_renderer->model_renderer_resources_manager->meshlets_vertices_sb, 4u );
    crude_gfx_descriptor_set_creation_add_buffer( &ds_creation, pass->scene_renderer->model_renderer_resources_manager->meshlets_triangles_indices_sb, 5u );
    crude_gfx_descriptor_set_creation_add_buffer( &ds_creation, pass->scene_renderer->model_renderer_resources_manager->meshlets_vertices_indices_sb, 6u );
    crude_gfx_descriptor_set_creation_add_buffer( &ds_creation, pass->scene_renderer->mesh_task_indirect_count_late_sb[ i ], 7u );
    crude_gfx_descriptor_set_creation_add_buffer( &ds_creation, pass->scene_renderer->mesh_task_indirect_commands_early_sb[ i ], 8u );
    crude_gfx_descriptor_set_creation_add_buffer( &ds_creation, pass->scene_renderer->lights_bins_sb[ pass->scene_renderer->gpu->current_frame ], 10u );
    crude_gfx_descriptor_set_creation_add_buffer( &ds_creation, pass->scene_renderer->lights_sb, 11u );
    crude_gfx_descriptor_set_creation_add_buffer( &ds_creation, pass->scene_renderer->lights_tiles_sb[ pass->scene_renderer->gpu->current_frame ], 12u );
    crude_gfx_descriptor_set_creation_add_buffer( &ds_creation, pass->scene_renderer->lights_indices_sb[ pass->scene_renderer->gpu->current_frame ], 13u );
    crude_gfx_descriptor_set_creation_add_buffer( &ds_creation, pass->scene_renderer->pointlight_world_to_clip_sb[ pass->scene_renderer->gpu->current_frame ], 14u );

    crude_gfx_scene_renderer_add_debug_resources_to_descriptor_set_creation( &ds_creation, pass->scene_renderer, i );
    
    pass->transparent_ds[ i ] = crude_gfx_create_descriptor_set( pass->scene_renderer->gpu, &ds_creation );
  }
}

void
crude_gfx_transparent_pass_on_resize
(
  _In_ void                                               *ctx,
  _In_ uint32                                              new_width,
  _In_ uint32                                              new_height
)
{
  return crude_gfx_transparent_pass_on_techniques_reloaded( ctx );
}

crude_gfx_render_graph_pass_container
crude_gfx_transparent_pass_pack
(
  _In_ crude_gfx_transparent_pass                         *pass
)
{
  crude_gfx_render_graph_pass_container container = crude_gfx_render_graph_pass_container_empty();
  container.ctx = pass;
  container.render = crude_gfx_transparent_pass_render;
  container.on_techniques_reloaded = crude_gfx_transparent_pass_on_techniques_reloaded;
  container.on_resize = crude_gfx_transparent_pass_on_resize;
  return container;
}