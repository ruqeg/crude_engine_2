#include <core/profiler.h>
#include <core/hash_map.h>
#include <scene/scene_components.h>
#include <graphics/scene_renderer.h>

#include <graphics/passes/meshlet_pass.h>

void
crude_gfx_meshlet_pass_initialize
(
  _In_ crude_gfx_meshlet_pass                             *pass,
  _In_ crude_gfx_scene_renderer                           *scene_renderer,
  bool                                                     early_pass
)
{
  crude_gfx_renderer_technique_pass                       *meshlet_pass;
  crude_gfx_descriptor_set_layout_handle                   layout;

  pass->scene_renderer = scene_renderer;
  pass->early_pass = early_pass;

  meshlet_pass = crude_gfx_renderer_access_technique_pass_by_name( scene_renderer->renderer, "meshlet", "meshlet" );
  layout = crude_gfx_get_descriptor_set_layout( scene_renderer->renderer->gpu, meshlet_pass->pipeline, CRUDE_GFX_MATERIAL_DESCRIPTOR_SET_INDEX );
  
  for ( uint32 i = 0; i < CRUDE_GFX_MAX_SWAPCHAIN_IMAGES; ++i )
  {
    crude_gfx_descriptor_set_creation                    ds_creation;
    
    ds_creation = crude_gfx_descriptor_set_creation_empty();
    ds_creation.layout = layout;
    ds_creation.name = "meshlet_descriptor_set";
  
    crude_gfx_scene_renderer_add_scene_resources_to_descriptor_set_creation( &ds_creation, pass->scene_renderer, i );
    crude_gfx_scene_renderer_add_mesh_resources_to_descriptor_set_creation( &ds_creation, pass->scene_renderer, i );
    crude_gfx_scene_renderer_add_meshlet_resources_to_descriptor_set_creation( &ds_creation, pass->scene_renderer, i );
    crude_gfx_scene_renderer_add_debug_resources_to_descriptor_set_creation( &ds_creation, pass->scene_renderer, i );
    if ( pass->early_pass )
    {
      crude_gfx_descriptor_set_creation_add_buffer( &ds_creation, scene_renderer->mesh_task_indirect_count_early_sb[ i ], 10u );
      crude_gfx_descriptor_set_creation_add_buffer( &ds_creation, scene_renderer->mesh_task_indirect_commands_early_sb[ i ], 11u );
    }
    else
    {
      crude_gfx_descriptor_set_creation_add_buffer( &ds_creation, scene_renderer->mesh_task_indirect_count_late_sb[ i ], 10u );
      crude_gfx_descriptor_set_creation_add_buffer( &ds_creation, scene_renderer->mesh_task_indirect_commands_late_sb[ i ], 11u );
    }
    
    pass->meshlet_shader_ds[ i ] = crude_gfx_create_descriptor_set( scene_renderer->renderer->gpu, &ds_creation );
  }
}

void
crude_gfx_meshlet_pass_deinitialize
(
  _In_ crude_gfx_meshlet_pass                             *pass
)
{
  for ( uint32 i = 0; i < CRUDE_GFX_MAX_SWAPCHAIN_IMAGES; ++i )
  {
    crude_gfx_destroy_descriptor_set( pass->scene_renderer->renderer->gpu, pass->meshlet_shader_ds[ i ] );
  }
}

void
crude_gfx_meshlet_pass_render
(
  _In_ void                                               *ctx,
  _In_ crude_gfx_cmd_buffer                               *primary_cmd
)
{
  crude_gfx_meshlet_pass                                  *pass;
  crude_gfx_renderer                                      *renderer;
  crude_gfx_pipeline_handle                                pipeline;
 
  pass = CRUDE_REINTERPRET_CAST( crude_gfx_meshlet_pass*, ctx );

  renderer = pass->scene_renderer->renderer;

  pipeline = crude_gfx_renderer_access_technique_pass_by_name( renderer, "meshlet", "meshlet" )->pipeline;
  crude_gfx_cmd_bind_pipeline( primary_cmd, pipeline );
  crude_gfx_cmd_bind_descriptor_set( primary_cmd, pass->meshlet_shader_ds[ renderer->gpu->current_frame ] );
  crude_gfx_cmd_draw_mesh_task_indirect_count(
    primary_cmd,
    pass->early_pass ? pass->scene_renderer->mesh_task_indirect_commands_early_sb[ renderer->gpu->current_frame ] : pass->scene_renderer->mesh_task_indirect_commands_late_sb[ renderer->gpu->current_frame ],
    CRUDE_OFFSETOF( crude_gfx_mesh_draw_command_gpu, indirect_meshlet ),
    pass->early_pass ? pass->scene_renderer->mesh_task_indirect_count_early_sb[ renderer->gpu->current_frame ] : pass->scene_renderer->mesh_task_indirect_count_late_sb[ renderer->gpu->current_frame ],
    0,
    CRUDE_ARRAY_LENGTH( pass->scene_renderer->meshes_instances ),
    sizeof( crude_gfx_mesh_draw_command_gpu )
  );
}

crude_gfx_render_graph_pass_container
crude_gfx_meshlet_pass_pack
(
  _In_ crude_gfx_meshlet_pass                             *pass
)
{
  crude_gfx_render_graph_pass_container container = crude_gfx_render_graph_pass_container_empty();
  container.ctx = pass;
  container.render = crude_gfx_meshlet_pass_render;
  return container;
}