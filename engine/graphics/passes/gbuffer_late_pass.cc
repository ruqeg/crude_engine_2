#include <engine/core/profiler.h>
#include <engine/core/hash_map.h>
#include <engine/scene/scene_components.h>
#include <engine/graphics/scene_renderer.h>

#include <engine/graphics/passes/gbuffer_late_pass.h>

void
crude_gfx_gbuffer_late_pass_initialize
(
  _In_ crude_gfx_gbuffer_late_pass                        *pass,
  _In_ crude_gfx_scene_renderer                           *scene_renderer
)
{
  pass->scene_renderer = scene_renderer;
}

void
crude_gfx_gbuffer_late_pass_deinitialize
(
  _In_ crude_gfx_gbuffer_late_pass                        *pass
)
{
}

void
crude_gfx_gbuffer_late_pass_pre_render
(
  _In_ void                                               *ctx,
  _In_ crude_gfx_cmd_buffer                               *primary_cmd
)
{
  crude_gfx_gbuffer_late_pass                             *pass;
  crude_gfx_device                                        *gpu;
 
  pass = CRUDE_REINTERPRET_CAST( crude_gfx_gbuffer_late_pass*, ctx );
  gpu = pass->scene_renderer->gpu;

  crude_gfx_cmd_add_buffer_barrier( primary_cmd, pass->scene_renderer->mesh_task_indirect_count_hga.buffer_handle, CRUDE_GFX_RESOURCE_STATE_UNORDERED_ACCESS, CRUDE_GFX_RESOURCE_STATE_INDIRECT_ARGUMENT );
  crude_gfx_cmd_add_buffer_barrier( primary_cmd, pass->scene_renderer->mesh_task_indirect_commands_hga.buffer_handle, CRUDE_GFX_RESOURCE_STATE_UNORDERED_ACCESS, CRUDE_GFX_RESOURCE_STATE_INDIRECT_ARGUMENT );
}

void
crude_gfx_gbuffer_late_pass_render
(
  _In_ void                                               *ctx,
  _In_ crude_gfx_cmd_buffer                               *primary_cmd
)
{
  CRUDE_ALIGNED_STRUCT( 16 ) push_constant_
  {
    VkDeviceAddress                                        scene;
    VkDeviceAddress                                        mesh_draws;
    VkDeviceAddress                                        mesh_instance_draws;
    VkDeviceAddress                                        meshlets;
    VkDeviceAddress                                        vertices;
    VkDeviceAddress                                        triangles_indices;
    VkDeviceAddress                                        vertices_indices;
    VkDeviceAddress                                        visible_mesh_count;
    VkDeviceAddress                                        mesh_draw_commands;
  };

  crude_gfx_gbuffer_late_pass                             *pass;
  crude_gfx_device                                        *gpu;
  crude_gfx_pipeline_handle                                pipeline;
  push_constant_                                           pust_constant;
  return;
  pass = CRUDE_REINTERPRET_CAST( crude_gfx_gbuffer_late_pass*, ctx );

  gpu = pass->scene_renderer->gpu;

  pipeline = crude_gfx_access_technique_pass_by_name( gpu, "deferred_meshlet", "deferred_meshlet" )->pipeline;
  crude_gfx_cmd_bind_pipeline( primary_cmd, pipeline );

  
  pust_constant.meshlets = pass->scene_renderer->model_renderer_resources_manager->meshlets_hga.gpu_address;
  pust_constant.mesh_draws = pass->scene_renderer->model_renderer_resources_manager->meshes_draws_hga.gpu_address;
  pust_constant.triangles_indices = pass->scene_renderer->model_renderer_resources_manager->meshlets_triangles_indices_hga.gpu_address;
  pust_constant.vertices = pass->scene_renderer->model_renderer_resources_manager->meshlets_vertices_hga.gpu_address;
  pust_constant.vertices_indices = pass->scene_renderer->model_renderer_resources_manager->meshlets_vertices_indices_hga.gpu_address;
  pust_constant.mesh_instance_draws = pass->scene_renderer->meshes_instances_draws_hga.gpu_address;
  pust_constant.scene = pass->scene_renderer->scene_hga.gpu_address;
  pust_constant.mesh_draw_commands = pass->scene_renderer->mesh_task_indirect_commands_hga.gpu_address;
  pust_constant.visible_mesh_count = pass->scene_renderer->mesh_task_indirect_count_hga.gpu_address;
  crude_gfx_cmd_push_constant( primary_cmd, &pust_constant, sizeof( pust_constant ) );
  
  crude_gfx_cmd_bind_bindless_descriptor_set( primary_cmd );

  crude_gfx_cmd_draw_mesh_task_indirect_count(
    primary_cmd,
    pass->scene_renderer->mesh_task_indirect_commands_culled_hga.buffer_handle,
    CRUDE_OFFSETOF( crude_gfx_mesh_draw_command_gpu, indirect_meshlet ),
    pass->scene_renderer->mesh_task_indirect_count_hga.buffer_handle,
    CRUDE_OFFSETOF( crude_gfx_mesh_draw_counts_gpu, opaque_mesh_visible_late_count ),
    pass->scene_renderer->total_visible_meshes_instances_count,
    sizeof( crude_gfx_mesh_draw_command_gpu )
  );
}

crude_gfx_render_graph_pass_container
crude_gfx_gbuffer_late_pass_pack
(
  _In_ crude_gfx_gbuffer_late_pass                        *pass
)
{
  crude_gfx_render_graph_pass_container container = crude_gfx_render_graph_pass_container_empty();
  container.ctx = pass;
  container.render = crude_gfx_gbuffer_late_pass_render;
  container.pre_render = crude_gfx_gbuffer_late_pass_pre_render;
  return container;
}