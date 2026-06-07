#include <engine/graphics/scene_renderer.h>

#define OPAQUE_TERRAIN_MESHLET
#include <engine/graphics/shaders/geometry.crude_shader>

void
crude_gfx_opaque_terrain_pass_initialize
(
  _In_ crude_gfx_opaque_terrain_pass                      *pass,
  _In_ crude_gfx_scene_renderer                           *scene_renderer
)
{
  pass->scene_renderer = scene_renderer;
}

void
crude_gfx_opaque_terrain_pass_deinitialize
(
  _In_ crude_gfx_opaque_terrain_pass                      *pass
)
{
}

void
crude_gfx_opaque_terrain_pass_render
(
  _In_ void                                               *ctx,
  _In_ crude_gfx_cmd_buffer                               *primary_cmd
)
{
  _In_ crude_gfx_opaque_terrain_pass                      *pass;
  crude_gfx_device                                        *gpu;
  crude_gfx_pipeline_handle                                pipeline;
  XMFLOAT2                                                 inv_radiance_texture_resolution;

  pass = CRUDE_REINTERPRET_CAST( crude_gfx_opaque_terrain_pass*, ctx );

  gpu = pass->scene_renderer->gpu;

  pipeline = crude_gfx_access_technique_pass_by_name( gpu, "geometry", "opaque_terrain_meshlet" )->pipeline;
  crude_gfx_cmd_bind_pipeline( primary_cmd, pipeline );

  crude_gfx_cmd_bind_bindless_descriptor_set( primary_cmd );
  
  if ( gpu->mesh_shaders_extension_present )
  {
    crude_gfx_opaque_terrain_meshlet_pass_push_constant_   push_constant;

    push_constant = CRUDE_COMPOUNT_EMPTY( crude_gfx_opaque_terrain_meshlet_pass_push_constant_ );
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
    push_constant.culled_lights = pass->scene_renderer->culled_lights_hga.gpu_address;
    push_constant.debug_line_vertices = pass->scene_renderer->debug_line_vertices_hga.gpu_address;
    push_constant.debug_counts = pass->scene_renderer->debug_commands_hga.gpu_address;
    push_constant.joint_matrices = pass->scene_renderer->joint_matrices_hga.gpu_address;
    push_constant.culled_lights_world_to_texture = pass->scene_renderer->culled_lights_world_to_texture_hga.gpu_address;
    crude_gfx_cmd_push_constant( primary_cmd, &push_constant, sizeof( push_constant ) );
    
    crude_gfx_cmd_draw_mesh_task( primary_cmd, 1u, 1u, 1u );
  }
  else
  {
    CRUDE_ASSERT( false );
  }
}

crude_gfx_render_graph_pass_container
crude_gfx_opaque_terrain_pass_pack
(
  _In_ crude_gfx_opaque_terrain_pass                      *pass
)
{
  crude_gfx_render_graph_pass_container container = crude_gfx_render_graph_pass_container_empty();
  container.ctx = pass;
  container.render = crude_gfx_opaque_terrain_pass_render;
  return container;
}