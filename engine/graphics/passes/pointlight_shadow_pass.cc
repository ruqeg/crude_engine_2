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
  crude_gfx_pipeline_handle                                pointlight_shadow_culling_pipeline;
  crude_gfx_descriptor_set_layout_handle                   pointlight_shadow_culling_dsl;

  pass->scene_renderer = scene_renderer;

  pointlight_shadow_culling_pipeline = crude_gfx_renderer_access_technique_pass_by_name( pass->scene_renderer->renderer, "meshlet", "pointlight_shadow_culling" )->pipeline;
  pointlight_shadow_culling_dsl = crude_gfx_get_descriptor_set_layout( scene_renderer->renderer->gpu, pointlight_shadow_culling_pipeline, CRUDE_GFX_MATERIAL_DESCRIPTOR_SET_INDEX );

  for ( uint32 i = 0; i < CRUDE_GFX_MAX_SWAPCHAIN_IMAGES; ++i )
  {
    crude_gfx_descriptor_set_creation                    ds_creation;
    
    ds_creation = crude_gfx_descriptor_set_creation_empty();
    ds_creation.layout = pointlight_shadow_culling_dsl;
    ds_creation.name = "pointlight_shadow_culling_ds";
  
    crude_gfx_scene_renderer_add_scene_resources_to_descriptor_set_creation( &ds_creation, pass->scene_renderer, i );
    crude_gfx_scene_renderer_add_mesh_resources_to_descriptor_set_creation( &ds_creation, pass->scene_renderer, i );
    crude_gfx_scene_renderer_add_meshlet_resources_to_descriptor_set_creation( &ds_creation, pass->scene_renderer, i );
    crude_gfx_scene_renderer_add_debug_resources_to_descriptor_set_creation( &ds_creation, pass->scene_renderer, i );
    crude_gfx_scene_renderer_add_light_resources_to_descriptor_set_creation( &ds_creation, pass->scene_renderer, i );
    crude_gfx_descriptor_set_creation_add_buffer( &ds_creation, scene_renderer->light_meshlet_instances_sb[ i ], 10u );
    crude_gfx_descriptor_set_creation_add_buffer( &ds_creation, scene_renderer->per_light_meshlet_instances_count_sb[ i ], 11u );
    
    pass->pointlight_shadow_culling_ds[ i ] = crude_gfx_create_descriptor_set( scene_renderer->renderer->gpu, &ds_creation );
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
    crude_gfx_destroy_descriptor_set( pass->scene_renderer->renderer->gpu, pass->pointlight_shadow_culling_ds[ i ] );
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
  crude_gfx_buffer                                        *count_late_sb;
  crude_gfx_buffer                                        *commands_late_sb;
  crude_gfx_pointlight_shadow_pass                        *pass;
  crude_gfx_pipeline_handle                                mesh_culling_pipeline;

  pass = CRUDE_REINTERPRET_CAST( crude_gfx_pointlight_shadow_pass*, ctx );

  if ( !pass->scene_renderer->total_meshes_instances_count )
  {
    return;
  }

  gpu = pass->scene_renderer->renderer->gpu;
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