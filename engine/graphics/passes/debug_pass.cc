#include <engine/graphics/scene_renderer.h>

#include <engine/graphics/passes/debug_pass.h>

void
crude_gfx_debug_pass_initialize
(
  _In_ crude_gfx_debug_pass                               *pass,
  _In_ crude_gfx_scene_renderer                           *scene_renderer
)
{
  pass->scene_renderer = scene_renderer;
}

void
crude_gfx_debug_pass_deinitialize
(
  _In_ crude_gfx_debug_pass                               *pass
)
{
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
  
  crude_gfx_cmd_add_buffer_barrier( primary_cmd, pass->scene_renderer->debug_line_vertices_hga.buffer_handle, CRUDE_GFX_RESOURCE_STATE_UNORDERED_ACCESS, CRUDE_GFX_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER );
  crude_gfx_cmd_add_buffer_barrier( primary_cmd, pass->scene_renderer->debug_commands_hga.buffer_handle, CRUDE_GFX_RESOURCE_STATE_UNORDERED_ACCESS, CRUDE_GFX_RESOURCE_STATE_INDIRECT_ARGUMENT );
  crude_gfx_cmd_add_buffer_barrier( primary_cmd, pass->scene_renderer->debug_cubes_instances_hga.buffer_handle, CRUDE_GFX_RESOURCE_STATE_UNORDERED_ACCESS, CRUDE_GFX_RESOURCE_STATE_SHADER_RESOURCE );
}

void
crude_gfx_debug_pass_render
(
  _In_ void                                               *ctx,
  _In_ crude_gfx_cmd_buffer                               *primary_cmd
)
{
  CRUDE_ALIGNED_STRUCT( 16 ) debug_line3d_push_constant_
  {
    VkDeviceAddress                                        scene;
    VkDeviceAddress                                        debug_lines_vertices;
  };
  CRUDE_ALIGNED_STRUCT( 16 ) debug_line2d_push_constant_
  {
    VkDeviceAddress                                        debug_lines_vertices;
  };
  CRUDE_ALIGNED_STRUCT( 16 ) debug_cube_push_constant_
  {
    VkDeviceAddress                                        scene;
    VkDeviceAddress                                        debug_cube_instances;
  };

  crude_gfx_debug_pass                                    *pass;
  crude_gfx_device                                        *gpu;
  crude_gfx_pipeline_handle                                debug_line3d_pipeline;
  crude_gfx_pipeline_handle                                debug_line2d_pipeline;
  crude_gfx_pipeline_handle                                debug_cube_pipeline;
  debug_line3d_push_constant_                              lined3d_push_constant;
  debug_line2d_push_constant_                              lines2d_push_constant;
  debug_cube_push_constant_                                cube_push_constant;

  pass = CRUDE_REINTERPRET_CAST( crude_gfx_debug_pass*, ctx );
  gpu = pass->scene_renderer->gpu;
  debug_line3d_pipeline = crude_gfx_access_technique_pass_by_name( gpu, "debug", "debug_line3d" )->pipeline;
  debug_line2d_pipeline = crude_gfx_access_technique_pass_by_name( gpu, "debug", "debug_line2d" )->pipeline;
  debug_cube_pipeline = crude_gfx_access_technique_pass_by_name( gpu, "debug", "debug_cube" )->pipeline;

  crude_gfx_cmd_bind_pipeline( primary_cmd, debug_line3d_pipeline );
  crude_gfx_cmd_bind_bindless_descriptor_set( primary_cmd );
  lined3d_push_constant.debug_lines_vertices = pass->scene_renderer->debug_line_vertices_hga.gpu_address;
  lined3d_push_constant.scene = pass->scene_renderer->scene_hga.gpu_address;
  crude_gfx_cmd_push_constant( primary_cmd, &lined3d_push_constant, sizeof( lined3d_push_constant ) );
  crude_gfx_cmd_draw_inderect( primary_cmd, pass->scene_renderer->debug_commands_hga.buffer_handle, 0u, 1u, sizeof( crude_gfx_debug_draw_command_gpu ) );

  crude_gfx_cmd_bind_pipeline( primary_cmd, debug_line2d_pipeline );
  crude_gfx_cmd_bind_bindless_descriptor_set( primary_cmd );
  lines2d_push_constant.debug_lines_vertices = pass->scene_renderer->debug_line_vertices_hga.gpu_address;
  crude_gfx_cmd_push_constant( primary_cmd, &lines2d_push_constant, sizeof( lines2d_push_constant ) );
  crude_gfx_cmd_draw_inderect( primary_cmd, pass->scene_renderer->debug_commands_hga.buffer_handle, CRUDE_OFFSETOF( crude_gfx_debug_draw_command_gpu, draw_indirect_2dline ), 1u, sizeof( crude_gfx_debug_draw_command_gpu ) );

  crude_gfx_cmd_bind_pipeline( primary_cmd, debug_cube_pipeline );
  crude_gfx_cmd_bind_bindless_descriptor_set( primary_cmd );
  cube_push_constant.scene = pass->scene_renderer->scene_hga.gpu_address;
  cube_push_constant.debug_cube_instances = pass->scene_renderer->debug_cubes_instances_hga.gpu_address;
  crude_gfx_cmd_push_constant( primary_cmd, &cube_push_constant, sizeof( cube_push_constant ) );
  crude_gfx_cmd_draw_inderect( primary_cmd, pass->scene_renderer->debug_commands_hga.buffer_handle, CRUDE_OFFSETOF( crude_gfx_debug_draw_command_gpu, draw_indirect_cube ), 1u, sizeof( crude_gfx_debug_draw_command_gpu ) );
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

  crude_gfx_cmd_add_buffer_barrier( primary_cmd, pass->scene_renderer->debug_line_vertices_hga.buffer_handle, CRUDE_GFX_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER, CRUDE_GFX_RESOURCE_STATE_UNORDERED_ACCESS );
  crude_gfx_cmd_add_buffer_barrier( primary_cmd, pass->scene_renderer->debug_commands_hga.buffer_handle, CRUDE_GFX_RESOURCE_STATE_INDIRECT_ARGUMENT, CRUDE_GFX_RESOURCE_STATE_UNORDERED_ACCESS );
  crude_gfx_cmd_add_buffer_barrier( primary_cmd, pass->scene_renderer->debug_cubes_instances_hga.buffer_handle, CRUDE_GFX_RESOURCE_STATE_SHADER_RESOURCE, CRUDE_GFX_RESOURCE_STATE_UNORDERED_ACCESS );
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