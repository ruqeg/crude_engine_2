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

  crude_gfx_device *gpu = pass->scene_renderer->renderer->gpu;
  crude_gfx_pipeline_handle debug_line3d_pipeline = crude_gfx_renderer_access_technique_pass_by_name( pass->scene_renderer->renderer, "debug", "debug_line3d" )->pipeline;
  crude_gfx_descriptor_set_layout_handle debug_layout_handle = crude_gfx_get_descriptor_set_layout( gpu, debug_line3d_pipeline, CRUDE_GFX_MATERIAL_DESCRIPTOR_SET_INDEX );

  for ( uint32 i = 0; i < CRUDE_GFX_MAX_SWAPCHAIN_IMAGES; ++i )
  {
    crude_gfx_descriptor_set_creation                      ds_creation;

    ds_creation = crude_gfx_descriptor_set_creation_empty();
    ds_creation.name = "debug_descriptor_set";
    ds_creation.layout = debug_layout_handle;

    crude_gfx_scene_renderer_add_scene_resources_to_descriptor_set_creation( &ds_creation, scene_renderer, i );
    crude_gfx_scene_renderer_add_mesh_resources_to_descriptor_set_creation( &ds_creation, scene_renderer, i );
    
    pass->depth_lines3d_descriptor_sets_handles[ i ] = crude_gfx_create_descriptor_set( gpu, &ds_creation );
  }
}

void
crude_gfx_debug_pass_deinitialize
(
  _In_ crude_gfx_debug_pass                               *pass
)
{
  crude_gfx_device *gpu = pass->scene_renderer->renderer->gpu;

  for ( uint32 i = 0; i < CRUDE_GFX_MAX_SWAPCHAIN_IMAGES; ++i )
  {
    crude_gfx_destroy_descriptor_set( gpu, pass->depth_lines3d_descriptor_sets_handles[ i ] );
  }
}

void
crude_gfx_debug_pass_pre_render
(
  _In_ void                                               *ctx,
  _In_ crude_gfx_cmd_buffer                               *primary_cmd
)
{
  crude_gfx_debug_pass *pass = CRUDE_REINTERPRET_CAST( crude_gfx_debug_pass*, ctx );
  crude_gfx_renderer *renderer = pass->scene_renderer->renderer;
  crude_gfx_buffer *debug_draw_command_sb = crude_gfx_access_buffer( renderer->gpu, pass->scene_renderer->debug_line_commands_sb[ renderer->gpu->current_frame ] );
  crude_gfx_buffer *debug_vertices_sb = crude_gfx_access_buffer( renderer->gpu, pass->scene_renderer->debug_line_vertices_sb[ renderer->gpu->current_frame ] );
  
  crude_gfx_cmd_add_buffer_barrier( primary_cmd, debug_vertices_sb, CRUDE_GFX_RESOURCE_STATE_UNORDERED_ACCESS, CRUDE_GFX_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER );
  crude_gfx_cmd_add_buffer_barrier( primary_cmd, debug_draw_command_sb, CRUDE_GFX_RESOURCE_STATE_UNORDERED_ACCESS, CRUDE_GFX_RESOURCE_STATE_INDIRECT_ARGUMENT );
}

void
crude_gfx_debug_pass_render
(
  _In_ void                                               *ctx,
  _In_ crude_gfx_cmd_buffer                               *primary_cmd
)
{
  crude_gfx_debug_pass *pass = CRUDE_REINTERPRET_CAST( crude_gfx_debug_pass*, ctx );
  crude_gfx_renderer *renderer = pass->scene_renderer->renderer;

  crude_gfx_pipeline_handle debug_line3d_pipeline = crude_gfx_renderer_access_technique_pass_by_name( renderer, "debug", "debug_line3d" )->pipeline;
  crude_gfx_pipeline_handle debug_line2d_pipeline = crude_gfx_renderer_access_technique_pass_by_name( renderer, "debug", "debug_line2d" )->pipeline;

  crude_gfx_cmd_bind_pipeline( primary_cmd, debug_line3d_pipeline );
  crude_gfx_cmd_bind_descriptor_set( primary_cmd, pass->depth_lines3d_descriptor_sets_handles[ renderer->gpu->current_frame ] );
  crude_gfx_cmd_bind_vertex_buffer( primary_cmd, pass->scene_renderer->debug_line_vertices_sb[ renderer->gpu->current_frame ], 0u, 0u );
  crude_gfx_cmd_draw_inderect( primary_cmd, pass->scene_renderer->debug_line_commands_sb[ renderer->gpu->current_frame ], 0u, 1u, sizeof( crude_gfx_debug_draw_command_gpu ) );

  crude_gfx_cmd_bind_pipeline( primary_cmd, debug_line2d_pipeline );
  crude_gfx_cmd_bind_vertex_buffer( primary_cmd, pass->scene_renderer->debug_line_vertices_sb[ renderer->gpu->current_frame ], 0u, CRUDE_GFX_DEBUG_LINE_2D_OFFSET * sizeof( crude_gfx_debug_line_vertex_gpu ) );
  crude_gfx_cmd_draw_inderect( primary_cmd, pass->scene_renderer->debug_line_commands_sb[ renderer->gpu->current_frame ], CRUDE_OFFSETOF( crude_gfx_debug_draw_command_gpu, draw_indirect_2dline ), 1u, sizeof( crude_gfx_debug_draw_command_gpu ) );
}

void
crude_gfx_debug_pass_post_render
(
  _In_ void                                               *ctx,
  _In_ crude_gfx_cmd_buffer                               *primary_cmd
)
{
  crude_gfx_debug_pass *pass = CRUDE_REINTERPRET_CAST( crude_gfx_debug_pass*, ctx );
  crude_gfx_renderer *renderer = pass->scene_renderer->renderer;
  crude_gfx_buffer *debug_draw_command_sb = crude_gfx_access_buffer( renderer->gpu, pass->scene_renderer->debug_line_commands_sb[ renderer->gpu->current_frame ] );
  crude_gfx_buffer *debug_vertices_sb = crude_gfx_access_buffer( renderer->gpu, pass->scene_renderer->debug_line_vertices_sb[ renderer->gpu->current_frame ] );

  crude_gfx_cmd_add_buffer_barrier( primary_cmd, debug_vertices_sb, CRUDE_GFX_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER, CRUDE_GFX_RESOURCE_STATE_UNORDERED_ACCESS );
  crude_gfx_cmd_add_buffer_barrier( primary_cmd, debug_draw_command_sb, CRUDE_GFX_RESOURCE_STATE_INDIRECT_ARGUMENT, CRUDE_GFX_RESOURCE_STATE_UNORDERED_ACCESS );
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
  return container;
}