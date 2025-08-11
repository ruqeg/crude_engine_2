#include <graphics/scene_renderer.h>
#include <graphics/scene_renderer_resources.h>

#include <graphics/passes/pointlight_shadow_pass.h>

void
crude_gfx_pointlight_shadow_pass_initialize
(
  _In_ crude_gfx_pointlight_shadow_pass                   *pass,
  _In_ crude_gfx_scene_renderer                           *scene_renderer
)
{
  crude_gfx_pipeline_handle                                pointshadow_culling_pipeline;
  crude_gfx_descriptor_set_layout_handle                   pointshadow_culling_dsl;
  crude_gfx_pipeline_handle                                pointshadow_commands_generation_pipeline;
  crude_gfx_descriptor_set_layout_handle                   pointshadow_commands_generation_dsl;

  pass->scene_renderer = scene_renderer;

  pointshadow_culling_pipeline = crude_gfx_renderer_access_technique_pass_by_name( pass->scene_renderer->renderer, "meshlet", "pointshadow_culling" )->pipeline;
  pointshadow_culling_dsl = crude_gfx_get_descriptor_set_layout( scene_renderer->renderer->gpu, pointshadow_culling_pipeline, CRUDE_GFX_MATERIAL_DESCRIPTOR_SET_INDEX );
  
  for ( uint32 i = 0; i < CRUDE_GFX_MAX_SWAPCHAIN_IMAGES; ++i )
  {
    crude_gfx_descriptor_set_creation                    ds_creation;
    
    ds_creation = crude_gfx_descriptor_set_creation_empty();
    ds_creation.layout = pointshadow_culling_dsl;
    ds_creation.name = "pointlight_shadow_culling_ds";
  
    crude_gfx_scene_renderer_add_scene_resources_to_descriptor_set_creation( &ds_creation, pass->scene_renderer, i );
    crude_gfx_scene_renderer_add_mesh_resources_to_descriptor_set_creation( &ds_creation, pass->scene_renderer, i );
    crude_gfx_scene_renderer_add_meshlet_resources_to_descriptor_set_creation( &ds_creation, pass->scene_renderer, i );
    crude_gfx_scene_renderer_add_debug_resources_to_descriptor_set_creation( &ds_creation, pass->scene_renderer, i );
    crude_gfx_scene_renderer_add_light_resources_to_descriptor_set_creation( &ds_creation, pass->scene_renderer, i );
    crude_gfx_descriptor_set_creation_add_buffer( &ds_creation, scene_renderer->light_meshlet_instances_sb[ i ], 10u );
    crude_gfx_descriptor_set_creation_add_buffer( &ds_creation, scene_renderer->per_light_meshlet_instances_sb[ i ], 11u );
    
    pass->pointshadow_culling_ds[ i ] = crude_gfx_create_descriptor_set( scene_renderer->renderer->gpu, &ds_creation );
  }
  
  pointshadow_commands_generation_pipeline = crude_gfx_renderer_access_technique_pass_by_name( pass->scene_renderer->renderer, "meshlet", "pointshadow_commands_generation" )->pipeline;
  pointshadow_commands_generation_dsl = crude_gfx_get_descriptor_set_layout( scene_renderer->renderer->gpu, pointshadow_commands_generation_pipeline, CRUDE_GFX_MATERIAL_DESCRIPTOR_SET_INDEX );

  for ( uint32 i = 0; i < CRUDE_GFX_MAX_SWAPCHAIN_IMAGES; ++i )
  {
    crude_gfx_descriptor_set_creation                    ds_creation;
    
    ds_creation = crude_gfx_descriptor_set_creation_empty();
    ds_creation.layout = pointshadow_commands_generation_dsl;
    ds_creation.name = "pointlight_shadow_culling_ds";
  
    crude_gfx_scene_renderer_add_scene_resources_to_descriptor_set_creation( &ds_creation, pass->scene_renderer, i );
    crude_gfx_scene_renderer_add_mesh_resources_to_descriptor_set_creation( &ds_creation, pass->scene_renderer, i );
    crude_gfx_scene_renderer_add_light_resources_to_descriptor_set_creation( &ds_creation, pass->scene_renderer, i );
    crude_gfx_descriptor_set_creation_add_buffer( &ds_creation, scene_renderer->per_light_meshlet_instances_sb[ i ], 10u );
    crude_gfx_descriptor_set_creation_add_buffer( &ds_creation, scene_renderer->light_meshlet_draw_commands_sb[ i ], 11u );
    
    pass->pointshadow_commands_generation_ds[ i ] = crude_gfx_create_descriptor_set( scene_renderer->renderer->gpu, &ds_creation );
  }
}

void
crude_gfx_pointlight_shadow_pass_deinitialize
(
  _In_ crude_gfx_pointlight_shadow_pass                   *pass
)
{
  for ( uint32 i = 0; i < CRUDE_GFX_MAX_SWAPCHAIN_IMAGES; ++i )
  {
    crude_gfx_destroy_descriptor_set( pass->scene_renderer->renderer->gpu, pass->pointshadow_culling_ds[ i ] );
    crude_gfx_destroy_descriptor_set( pass->scene_renderer->renderer->gpu, pass->pointshadow_commands_generation_ds[ i ] );
  }
}

void
crude_gfx_pointlight_shadow_pass_render
(
  _In_ void                                               *ctx,
  _In_ crude_gfx_cmd_buffer                               *primary_cmd
)
{
  crude_gfx_device                                        *gpu;
  crude_gfx_pointlight_shadow_pass                        *pass;
  crude_gfx_pipeline_handle                                pointshadow_culling_pipeline;
  crude_gfx_pipeline_handle                                pointshadow_commands_generation_pipeline;

  pass = CRUDE_REINTERPRET_CAST( crude_gfx_pointlight_shadow_pass*, ctx );
  gpu = pass->scene_renderer->renderer->gpu;

  pointshadow_culling_pipeline = crude_gfx_renderer_access_technique_pass_by_name( pass->scene_renderer->renderer, "meshlet", "pointshadow_culling" )->pipeline;
  pointshadow_commands_generation_pipeline = crude_gfx_renderer_access_technique_pass_by_name( pass->scene_renderer->renderer, "meshlet", "pointshadow_commands_generation" )->pipeline;

  crude_gfx_cmd_bind_pipeline( primary_cmd, pointshadow_culling_pipeline );
  crude_gfx_cmd_bind_descriptor_set( primary_cmd, pass->pointshadow_culling_ds[ gpu->current_frame ] );
  crude_gfx_cmd_dispatch( primary_cmd, ( pass->scene_renderer->total_meshes_instances_count * CRUDE_ARRAY_LENGTH( pass->scene_renderer->lights ) + 31 ) / 32, 1u, 1u );

  crude_gfx_cmd_bind_pipeline( primary_cmd, pointshadow_commands_generation_pipeline );
  crude_gfx_cmd_bind_descriptor_set( primary_cmd, pass->pointshadow_commands_generation_ds[ gpu->current_frame ] );
  crude_gfx_cmd_dispatch( primary_cmd, ( CRUDE_ARRAY_LENGTH( pass->scene_renderer->lights ) + 31 ) / 32, 1u, 1u );
}

crude_gfx_render_graph_pass_container
crude_gfx_pointlight_shadow_pass_pack
(
  _In_ crude_gfx_pointlight_shadow_pass                   *pass
)
{
  crude_gfx_render_graph_pass_container container = crude_gfx_render_graph_pass_container_empty();
  container.ctx = pass;
  container.render = crude_gfx_pointlight_shadow_pass_render;
  return container;
}