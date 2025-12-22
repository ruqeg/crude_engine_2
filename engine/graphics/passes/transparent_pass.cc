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
}

void
crude_gfx_transparent_pass_deinitialize
(
  _In_ crude_gfx_transparent_pass                         *pass
)
{
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

  pipeline = crude_gfx_access_technique_pass_by_name( gpu, "deferred", gpu->mesh_shaders_extension_present ? "transparent_no_cull" : "transparent_no_cull_classic" )->pipeline;
  crude_gfx_cmd_bind_pipeline( primary_cmd, pipeline );

  crude_gfx_cmd_bind_bindless_descriptor_set( primary_cmd );
  
  if ( gpu->mesh_shaders_extension_present )
  {
    CRUDE_ALIGNED_STRUCT( 16 ) push_constant_
    {
      VkDeviceAddress                                      scene;
      VkDeviceAddress                                      mesh_draws;
      VkDeviceAddress                                      mesh_instance_draws;
      VkDeviceAddress                                      meshlets;
      VkDeviceAddress                                      vertices;
      VkDeviceAddress                                      triangles_indices;
      VkDeviceAddress                                      vertices_indices;
      VkDeviceAddress                                      visible_mesh_count;
      VkDeviceAddress                                      mesh_draw_commands;
      VkDeviceAddress                                      zbins;
      VkDeviceAddress                                      lights_tiles;
      VkDeviceAddress                                      lights_indices;
      VkDeviceAddress                                      lights;
      VkDeviceAddress                                      light_shadow_views;
      float32                                              inv_radiance_texture_width;
      float32                                              inv_radiance_texture_height;
    };
    push_constant_                                         pust_constant;
  
    pust_constant.meshlets = pass->scene_renderer->model_renderer_resources_manager->meshlets_hga.gpu_address;
    pust_constant.mesh_draws = pass->scene_renderer->model_renderer_resources_manager->meshes_draws_hga.gpu_address;
    pust_constant.triangles_indices = pass->scene_renderer->model_renderer_resources_manager->meshlets_triangles_indices_hga.gpu_address;
    pust_constant.vertices = pass->scene_renderer->model_renderer_resources_manager->meshlets_vertices_hga.gpu_address;
    pust_constant.vertices_indices = pass->scene_renderer->model_renderer_resources_manager->meshlets_vertices_indices_hga.gpu_address;
    pust_constant.mesh_instance_draws = pass->scene_renderer->meshes_instances_draws_hga.gpu_address;
    pust_constant.scene = pass->scene_renderer->scene_hga.gpu_address;
    pust_constant.mesh_draw_commands = pass->scene_renderer->mesh_task_indirect_commands_hga.gpu_address;
    pust_constant.visible_mesh_count = pass->scene_renderer->mesh_task_indirect_count_hga.gpu_address;
    pust_constant.zbins = pass->scene_renderer->lights_bins_hga.gpu_address;
    pust_constant.lights_tiles = pass->scene_renderer->lights_tiles_hga.gpu_address;
    pust_constant.lights_indices = pass->scene_renderer->lights_indices_hga.gpu_address;
    pust_constant.lights = pass->scene_renderer->lights_hga.gpu_address;
    pust_constant.light_shadow_views = pass->scene_renderer->lights_world_to_clip_hga.gpu_address;
    pust_constant.inv_radiance_texture_width = 1.f / gpu->vk_swapchain_width;
    pust_constant.inv_radiance_texture_height = 1.f / gpu->vk_swapchain_height;
    crude_gfx_cmd_push_constant( primary_cmd, &pust_constant, sizeof( pust_constant ) );

    crude_gfx_cmd_draw_mesh_task_indirect_count(
      primary_cmd,
      pass->scene_renderer->mesh_task_indirect_commands_hga.buffer_handle,
      CRUDE_OFFSETOF( crude_gfx_mesh_draw_command_gpu, indirect_meshlet ),
      pass->scene_renderer->mesh_task_indirect_count_hga.buffer_handle,
      CRUDE_OFFSETOF( crude_gfx_mesh_draw_counts_gpu, transparent_mesh_visible_count ),
      pass->scene_renderer->total_visible_meshes_instances_count,
      sizeof( crude_gfx_mesh_draw_command_gpu )
    );
  }
  else
  {
    CRUDE_ALIGNED_STRUCT( 16 ) push_constant_
    {
      VkDeviceAddress                                      scene;
      VkDeviceAddress                                      mesh_draws;
      VkDeviceAddress                                      mesh_instance_draws;
      VkDeviceAddress                                      visible_mesh_count;
      VkDeviceAddress                                      mesh_draw_commands;
      VkDeviceAddress                                      zbins;
      VkDeviceAddress                                      lights_tiles;
      VkDeviceAddress                                      lights_indices;
      VkDeviceAddress                                      lights;
      VkDeviceAddress                                      light_shadow_views;
      float32                                              inv_radiance_texture_width;
      float32                                              inv_radiance_texture_height;
    };
    push_constant_                                         pust_constant;
  
    pust_constant.mesh_draws = pass->scene_renderer->model_renderer_resources_manager->meshes_draws_hga.gpu_address;
    pust_constant.mesh_instance_draws = pass->scene_renderer->meshes_instances_draws_hga.gpu_address;
    pust_constant.scene = pass->scene_renderer->scene_hga.gpu_address;
    pust_constant.mesh_draw_commands = pass->scene_renderer->mesh_task_indirect_commands_hga.gpu_address;
    pust_constant.visible_mesh_count = pass->scene_renderer->mesh_task_indirect_count_hga.gpu_address;
    pust_constant.zbins = pass->scene_renderer->lights_bins_hga.gpu_address;
    pust_constant.lights_tiles = pass->scene_renderer->lights_tiles_hga.gpu_address;
    pust_constant.lights_indices = pass->scene_renderer->lights_indices_hga.gpu_address;
    pust_constant.lights = pass->scene_renderer->lights_hga.gpu_address;
    pust_constant.light_shadow_views = pass->scene_renderer->lights_world_to_clip_hga.gpu_address;
    pust_constant.inv_radiance_texture_width = 1.f / gpu->vk_swapchain_width;
    pust_constant.inv_radiance_texture_height = 1.f / gpu->vk_swapchain_height;
    crude_gfx_cmd_push_constant( primary_cmd, &pust_constant, sizeof( pust_constant ) );

    crude_gfx_cmd_draw_indirect_count(
      primary_cmd,
      pass->scene_renderer->mesh_task_indirect_commands_hga.buffer_handle,
      CRUDE_OFFSETOF( crude_gfx_mesh_draw_command_gpu, indirect_mesh ),
      pass->scene_renderer->mesh_task_indirect_count_hga.buffer_handle,
      CRUDE_OFFSETOF( crude_gfx_mesh_draw_counts_gpu, transparent_mesh_visible_count ),
      pass->scene_renderer->total_visible_meshes_instances_count,
      sizeof( crude_gfx_mesh_draw_command_gpu )
    );
  }
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
  return container;
}