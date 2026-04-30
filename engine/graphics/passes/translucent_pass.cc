#include <engine/graphics/scene_renderer.h>

#define TRANSLUCENT_NO_CULL
#define TRANSLUCENT_NO_CULL_CLASSIC
#include <engine/graphics/shaders/geometry.crude_shader>

#include <engine/graphics/passes/translucent_pass.h>

void
crude_gfx_translucent_pass_initialize
(
  _In_ crude_gfx_translucent_pass                         *pass,
  _In_ crude_gfx_scene_renderer                           *scene_renderer
)
{
  pass->scene_renderer = scene_renderer;
}

void
crude_gfx_translucent_pass_deinitialize
(
  _In_ crude_gfx_translucent_pass                         *pass
)
{
}

void
crude_gfx_translucent_pass_render
(
  _In_ void                                               *ctx,
  _In_ crude_gfx_cmd_buffer                               *primary_cmd
)
{
  crude_gfx_translucent_pass                              *pass;
  crude_gfx_device                                        *gpu;
  crude_gfx_pipeline_handle                                pipeline;
  XMFLOAT2                                                 inv_radiance_texture_resolution;

  pass = CRUDE_REINTERPRET_CAST( crude_gfx_translucent_pass*, ctx );

  gpu = pass->scene_renderer->gpu;

  pipeline = crude_gfx_access_technique_pass_by_name( gpu, "geometry", gpu->mesh_shaders_extension_present ? "translucent_no_cull" : "translucent_no_cull_classic" )->pipeline;
  crude_gfx_cmd_bind_pipeline( primary_cmd, pipeline );

  crude_gfx_cmd_bind_bindless_descriptor_set( primary_cmd );
  
  if ( gpu->mesh_shaders_extension_present )
  {
    crude_gfx_translucent_no_cull_pass_push_constant_ push_constant;
  
    push_constant.meshlets = pass->scene_renderer->model_renderer_resources_manager->meshlets_hga.gpu_address;
    push_constant.mesh_draws = pass->scene_renderer->model_renderer_resources_manager->meshes_draws_hga.gpu_address;
    push_constant.triangles_indices = pass->scene_renderer->model_renderer_resources_manager->meshlets_triangles_indices_hga.gpu_address;
    push_constant.vertices = pass->scene_renderer->model_renderer_resources_manager->meshlets_vertices_hga.gpu_address;
    push_constant.vertices_positions = pass->scene_renderer->model_renderer_resources_manager->meshlets_vertices_positions_hga.gpu_address;
    push_constant.vertices_joints = pass->scene_renderer->model_renderer_resources_manager->meshlets_vertices_joints_hga.gpu_address;
    push_constant.vertices_indices = pass->scene_renderer->model_renderer_resources_manager->meshlets_vertices_indices_hga.gpu_address;
    push_constant.mesh_instance_draws = pass->scene_renderer->meshes_instances_draws_hga.gpu_address;
    push_constant.scene = pass->scene_renderer->scene_hga.gpu_address;
    push_constant.mesh_draw_commands = pass->scene_renderer->mesh_task_indirect_commands_hga.gpu_address;
    push_constant.visible_mesh_count = pass->scene_renderer->mesh_task_indirect_count_hga.gpu_address;
    push_constant.lights = pass->scene_renderer->lights_hga.gpu_address;
    push_constant.debug_line_vertices = pass->scene_renderer->debug_line_vertices_hga.gpu_address;
    push_constant.debug_counts = pass->scene_renderer->debug_commands_hga.gpu_address;
    push_constant.joint_matrices = pass->scene_renderer->joint_matrices_hga.gpu_address;
    push_constant.lights_world_to_texture = pass->scene_renderer->lights_world_to_texture_hga.gpu_address;
    crude_gfx_cmd_push_constant( primary_cmd, &push_constant, sizeof( push_constant ) );

    crude_gfx_cmd_draw_mesh_task_indirect_count(
      primary_cmd,
      pass->scene_renderer->mesh_task_indirect_commands_hga.buffer_handle,
      CRUDE_OFFSETOF( crude_gfx_mesh_draw_command, indirect_meshlet ),
      pass->scene_renderer->mesh_task_indirect_count_hga.buffer_handle,
      CRUDE_OFFSETOF( crude_gfx_mesh_draw_count, transparent_mesh_visible_count ),
      pass->scene_renderer->total_visible_meshes_instances_count,
      sizeof( crude_gfx_mesh_draw_command )
    );
  }
  else
  {
    crude_gfx_translucent_no_cull_classic_pass_push_constant_ push_constant;
  
    push_constant.mesh_draws = pass->scene_renderer->model_renderer_resources_manager->meshes_draws_hga.gpu_address;
    push_constant.mesh_instance_draws = pass->scene_renderer->meshes_instances_draws_hga.gpu_address;
    push_constant.scene = pass->scene_renderer->scene_hga.gpu_address;
    push_constant.mesh_draw_commands = pass->scene_renderer->mesh_task_indirect_commands_hga.gpu_address;
    push_constant.visible_mesh_count = pass->scene_renderer->mesh_task_indirect_count_hga.gpu_address;
    push_constant.lights = pass->scene_renderer->lights_hga.gpu_address;
    push_constant.inv_radiance_texture_width = 1.f / gpu->renderer_size.x;
    push_constant.inv_radiance_texture_height = 1.f / gpu->renderer_size.y;
    crude_gfx_cmd_push_constant( primary_cmd, &push_constant, sizeof( push_constant ) );

    crude_gfx_cmd_draw_indirect_count(
      primary_cmd,
      pass->scene_renderer->mesh_task_indirect_commands_hga.buffer_handle,
      CRUDE_OFFSETOF( crude_gfx_mesh_draw_command, indirect_mesh ),
      pass->scene_renderer->mesh_task_indirect_count_hga.buffer_handle,
      CRUDE_OFFSETOF( crude_gfx_mesh_draw_count, transparent_mesh_visible_count ),
      pass->scene_renderer->total_visible_meshes_instances_count,
      sizeof( crude_gfx_mesh_draw_command )
    );
  }
}

crude_gfx_render_graph_pass_container
crude_gfx_translucent_pass_pack
(
  _In_ crude_gfx_translucent_pass                         *pass
)
{
  crude_gfx_render_graph_pass_container container = crude_gfx_render_graph_pass_container_empty();
  container.ctx = pass;
  container.render = crude_gfx_translucent_pass_render;
  return container;
}