#include <graphics/scene_renderer.h>

#include <graphics/passes/debug_pass.h>

void
crude_gfx_debug_pass_initialize
(
  _In_ crude_gfx_debug_pass                               *pass,
  _In_ crude_gfx_scene_renderer                           *scene_renderer
)
{
  pass->scene_renderer = scene_renderer;

  for ( uint32 i = 0; i < CRUDE_GFX_MAX_SWAPCHAIN_IMAGES; ++i )
  {
    pass->depth_lines3d_ds[ i ] = CRUDE_GFX_DESCRIPTOR_SET_HANDLE_INVALID;
    pass->depth_cube_ds[ i ] = CRUDE_GFX_DESCRIPTOR_SET_HANDLE_INVALID;
  }

  crude_gfx_debug_pass_on_techniques_reloaded( pass );
}

void
crude_gfx_debug_pass_deinitialize
(
  _In_ crude_gfx_debug_pass                               *pass
)
{
  crude_gfx_device *gpu = pass->scene_renderer->gpu;

  for ( uint32 i = 0; i < CRUDE_GFX_MAX_SWAPCHAIN_IMAGES; ++i )
  {
    crude_gfx_destroy_descriptor_set( gpu, pass->depth_lines3d_ds[ i ] );
    crude_gfx_destroy_descriptor_set( gpu, pass->depth_cube_ds[ i ] );
  }
}

void
crude_gfx_debug_pass_pre_render
(
  _In_ void                                               *ctx,
  _In_ crude_gfx_cmd_buffer                               *primary_cmd
)
{
  crude_gfx_debug_pass                                    *pass;
  crude_gfx_device                                        *gpu;
  
  
  pass = CRUDE_REINTERPRET_CAST( crude_gfx_debug_pass*, ctx );
  gpu = pass->scene_renderer->gpu;
  
  crude_gfx_cmd_add_buffer_barrier( primary_cmd, pass->scene_renderer->debug_line_vertices_sb[ gpu->current_frame ], CRUDE_GFX_RESOURCE_STATE_UNORDERED_ACCESS, CRUDE_GFX_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER );
  crude_gfx_cmd_add_buffer_barrier( primary_cmd, pass->scene_renderer->debug_commands_sb[ gpu->current_frame ], CRUDE_GFX_RESOURCE_STATE_UNORDERED_ACCESS, CRUDE_GFX_RESOURCE_STATE_INDIRECT_ARGUMENT );
  crude_gfx_cmd_add_buffer_barrier( primary_cmd, pass->scene_renderer->debug_cubes_instances_sb[ gpu->current_frame ], CRUDE_GFX_RESOURCE_STATE_UNORDERED_ACCESS, CRUDE_GFX_RESOURCE_STATE_SHADER_RESOURCE );
}

void
crude_gfx_debug_pass_render
(
  _In_ void                                               *ctx,
  _In_ crude_gfx_cmd_buffer                               *primary_cmd
)
{
  crude_gfx_debug_pass                                    *pass;
  crude_gfx_device                                        *gpu;
  crude_gfx_pipeline_handle                                debug_line3d_pipeline;
  crude_gfx_pipeline_handle                                debug_line2d_pipeline;
  crude_gfx_pipeline_handle                                debug_cube_pipeline;

  pass = CRUDE_REINTERPRET_CAST( crude_gfx_debug_pass*, ctx );
  gpu = pass->scene_renderer->gpu;
  debug_line3d_pipeline = crude_gfx_access_technique_pass_by_name( gpu, "debug", "debug_line3d" )->pipeline;
  debug_line2d_pipeline = crude_gfx_access_technique_pass_by_name( gpu, "debug", "debug_line2d" )->pipeline;
  debug_cube_pipeline = crude_gfx_access_technique_pass_by_name( gpu, "debug", "debug_cube" )->pipeline;

  crude_gfx_cmd_bind_pipeline( primary_cmd, debug_line3d_pipeline );
  crude_gfx_cmd_bind_descriptor_set( primary_cmd, pass->depth_lines3d_ds[ gpu->current_frame ] );
  crude_gfx_cmd_bind_vertex_buffer( primary_cmd, pass->scene_renderer->debug_line_vertices_sb[ gpu->current_frame ], 0u, 0u );
  crude_gfx_cmd_draw_inderect( primary_cmd, pass->scene_renderer->debug_commands_sb[ gpu->current_frame ], 0u, 1u, sizeof( crude_gfx_debug_draw_command_gpu ) );

  crude_gfx_cmd_bind_pipeline( primary_cmd, debug_line2d_pipeline );
  crude_gfx_cmd_bind_vertex_buffer( primary_cmd, pass->scene_renderer->debug_line_vertices_sb[ gpu->current_frame ], 0u, CRUDE_GFX_DEBUG_LINE_2D_OFFSET * sizeof( crude_gfx_debug_line_vertex_gpu ) );
  crude_gfx_cmd_draw_inderect( primary_cmd, pass->scene_renderer->debug_commands_sb[ gpu->current_frame ], CRUDE_OFFSETOF( crude_gfx_debug_draw_command_gpu, draw_indirect_2dline ), 1u, sizeof( crude_gfx_debug_draw_command_gpu ) );

  crude_gfx_cmd_bind_pipeline( primary_cmd, debug_cube_pipeline );
  crude_gfx_cmd_bind_descriptor_set( primary_cmd, pass->depth_cube_ds[ gpu->current_frame ] );
  crude_gfx_cmd_draw_inderect( primary_cmd, pass->scene_renderer->debug_commands_sb[ gpu->current_frame ], CRUDE_OFFSETOF( crude_gfx_debug_draw_command_gpu, draw_indirect_cube ), 1u, sizeof( crude_gfx_debug_draw_command_gpu ) );
}

void
crude_gfx_debug_pass_post_render
(
  _In_ void                                               *ctx,
  _In_ crude_gfx_cmd_buffer                               *primary_cmd
)
{
  crude_gfx_debug_pass                                    *pass;
  crude_gfx_device                                        *gpu;

  pass = CRUDE_REINTERPRET_CAST( crude_gfx_debug_pass*, ctx );
  gpu = pass->scene_renderer->gpu;

  crude_gfx_cmd_add_buffer_barrier( primary_cmd, pass->scene_renderer->debug_line_vertices_sb[ gpu->current_frame ], CRUDE_GFX_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER, CRUDE_GFX_RESOURCE_STATE_UNORDERED_ACCESS );
  crude_gfx_cmd_add_buffer_barrier( primary_cmd, pass->scene_renderer->debug_commands_sb[ gpu->current_frame ], CRUDE_GFX_RESOURCE_STATE_INDIRECT_ARGUMENT, CRUDE_GFX_RESOURCE_STATE_UNORDERED_ACCESS );
  crude_gfx_cmd_add_buffer_barrier( primary_cmd, pass->scene_renderer->debug_cubes_instances_sb[ gpu->current_frame ], CRUDE_GFX_RESOURCE_STATE_SHADER_RESOURCE, CRUDE_GFX_RESOURCE_STATE_UNORDERED_ACCESS );
}

void
crude_gfx_debug_pass_on_techniques_reloaded
(
  _In_ void                                               *ctx
)
{
  crude_gfx_debug_pass                                    *pass;
  crude_gfx_device                                        *gpu;
  crude_gfx_pipeline_handle                                pipeline;
  crude_gfx_descriptor_set_layout_handle                   debug_line3d_dsl, debug_cube_dsl;
  
  pass = CRUDE_REINTERPRET_CAST( crude_gfx_debug_pass*, ctx );

  gpu = pass->scene_renderer->gpu;
  pipeline = crude_gfx_access_technique_pass_by_name( pass->scene_renderer->gpu, "debug", "debug_line3d" )->pipeline;
  debug_line3d_dsl = crude_gfx_get_descriptor_set_layout( gpu, pipeline, CRUDE_GFX_MATERIAL_DESCRIPTOR_SET_INDEX );

  pipeline = crude_gfx_access_technique_pass_by_name( pass->scene_renderer->gpu, "debug", "debug_cube" )->pipeline;
  debug_cube_dsl = crude_gfx_get_descriptor_set_layout( gpu, pipeline, CRUDE_GFX_MATERIAL_DESCRIPTOR_SET_INDEX );
  
  for ( uint32 i = 0; i < CRUDE_GFX_MAX_SWAPCHAIN_IMAGES; ++i )
  {
    if ( CRUDE_RESOURCE_HANDLE_IS_VALID( pass->depth_lines3d_ds[ i ] ) )
    {
      crude_gfx_destroy_descriptor_set( pass->scene_renderer->gpu, pass->depth_lines3d_ds[ i ] );
    }
    if ( CRUDE_RESOURCE_HANDLE_IS_VALID( pass->depth_cube_ds[ i ] ) )
    {
      crude_gfx_destroy_descriptor_set( pass->scene_renderer->gpu, pass->depth_cube_ds[ i ] );
    }
  }

  for ( uint32 i = 0; i < CRUDE_GFX_MAX_SWAPCHAIN_IMAGES; ++i )
  {
    crude_gfx_descriptor_set_creation                      ds_creation;

    ds_creation = crude_gfx_descriptor_set_creation_empty();
    ds_creation.name = "depth_lines3d_ds";
    ds_creation.layout = debug_line3d_dsl;
    
    crude_gfx_descriptor_set_creation_add_buffer( &ds_creation, pass->scene_renderer->scene_cb, 0u );
    
    pass->depth_lines3d_ds[ i ] = crude_gfx_create_descriptor_set( gpu, &ds_creation );
  }

  for ( uint32 i = 0; i < CRUDE_GFX_MAX_SWAPCHAIN_IMAGES; ++i )
  {
    crude_gfx_descriptor_set_creation                      ds_creation;

    ds_creation = crude_gfx_descriptor_set_creation_empty();
    ds_creation.name = "depth_cube_ds";
    ds_creation.layout = debug_cube_dsl;
    
    crude_gfx_descriptor_set_creation_add_buffer( &ds_creation, pass->scene_renderer->scene_cb, 0u );
    crude_gfx_descriptor_set_creation_add_buffer( &ds_creation, pass->scene_renderer->debug_cubes_instances_sb[ gpu->current_frame ], 1u );
    
    pass->depth_cube_ds[ i ] = crude_gfx_create_descriptor_set( gpu, &ds_creation );
  }
}

crude_gfx_render_graph_pass_container
crude_gfx_debug_pass_pack
(
  _In_ crude_gfx_debug_pass                               *pass
)
{
  crude_gfx_render_graph_pass_container container = crude_gfx_render_graph_pass_container_empty();
  container.ctx = pass;
  container.pre_render = crude_gfx_debug_pass_pre_render;
  container.render = crude_gfx_debug_pass_render;
  container.post_render = crude_gfx_debug_pass_post_render;
  container.on_techniques_reloaded = crude_gfx_debug_pass_on_techniques_reloaded;
  return container;
}