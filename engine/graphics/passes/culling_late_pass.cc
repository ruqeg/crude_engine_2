#include <engine/core/hash_map.h>
#include <engine/graphics/scene_renderer.h>
#include <engine/graphics/scene_renderer_resources.h>

#include <engine/graphics/passes/culling_late_pass.h>

void
crude_gfx_culling_late_pass_initialize
(
  _In_ crude_gfx_culling_late_pass                        *pass,
  _In_ crude_gfx_scene_renderer                           *scene_renderer
)
{
  pass->scene_renderer = scene_renderer;
  
  for ( uint32 i = 0; i < CRUDE_GRAPHICS_MAX_SWAPCHAIN_IMAGES; ++i )
  {
    pass->culling_late_ds[ i ] = CRUDE_GFX_DESCRIPTOR_SET_HANDLE_INVALID;
  }
}

void
crude_gfx_culling_late_pass_deinitialize
(
  _In_ crude_gfx_culling_late_pass                        *pass
)
{
  for ( uint32 i = 0; i < CRUDE_GRAPHICS_MAX_SWAPCHAIN_IMAGES; ++i )
  {
    crude_gfx_destroy_descriptor_set( pass->scene_renderer->gpu, pass->culling_late_ds[ i ] );
  }
}

void
crude_gfx_culling_late_pass_render
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
    VkDeviceAddress                                        mesh_bounds;
    VkDeviceAddress                                        mesh_draw_commands;
    VkDeviceAddress                                        mesh_draw_commands_culled;
    VkDeviceAddress                                        mesh_draw_count;
  };

  crude_gfx_device                                        *gpu;
  crude_gfx_culling_late_pass                             *pass;
  crude_gfx_pipeline_handle                                mesh_culling_pipeline;
  push_constant_                                           push_constant;

  pass = CRUDE_REINTERPRET_CAST( crude_gfx_culling_late_pass*, ctx );

  if ( !pass->scene_renderer->total_visible_meshes_instances_count )
  {
    return;
  }

  gpu = pass->scene_renderer->gpu;
  mesh_culling_pipeline = crude_gfx_access_technique_pass_by_name( pass->scene_renderer->gpu, "compute", "culling_late" )->pipeline;

  crude_gfx_cmd_bind_pipeline( primary_cmd, mesh_culling_pipeline );
  
  crude_gfx_cmd_add_buffer_barrier( primary_cmd, pass->scene_renderer->mesh_task_indirect_count_hga.buffer_handle, CRUDE_GFX_RESOURCE_STATE_INDIRECT_ARGUMENT, CRUDE_GFX_RESOURCE_STATE_UNORDERED_ACCESS );
  crude_gfx_cmd_add_buffer_barrier( primary_cmd, pass->scene_renderer->mesh_task_indirect_commands_culled_hga.buffer_handle, CRUDE_GFX_RESOURCE_STATE_INDIRECT_ARGUMENT, CRUDE_GFX_RESOURCE_STATE_UNORDERED_ACCESS );
  crude_gfx_cmd_add_buffer_barrier( primary_cmd, pass->scene_renderer->mesh_task_indirect_commands_hga.buffer_handle, CRUDE_GFX_RESOURCE_STATE_INDIRECT_ARGUMENT, CRUDE_GFX_RESOURCE_STATE_UNORDERED_ACCESS );
  
  push_constant.scene = pass->scene_renderer->scene_hga.gpu_address;
  push_constant.mesh_draws = pass->scene_renderer->model_renderer_resources_manager->meshes_draws_hga.gpu_address;
  push_constant.mesh_instance_draws = pass->scene_renderer->meshes_instances_draws_hga.gpu_address;
  push_constant.mesh_bounds = pass->scene_renderer->model_renderer_resources_manager->meshes_bounds_hga.gpu_address;
  push_constant.mesh_draw_commands = pass->scene_renderer->mesh_task_indirect_commands_hga.gpu_address;
  push_constant.mesh_draw_commands_culled = pass->scene_renderer->mesh_task_indirect_commands_culled_hga.gpu_address;
  push_constant.mesh_draw_count = pass->scene_renderer->mesh_task_indirect_count_hga.gpu_address;
  crude_gfx_cmd_push_constant( primary_cmd, &push_constant, sizeof( push_constant ) );

  crude_gfx_cmd_bind_bindless_descriptor_set( primary_cmd );

  crude_gfx_cmd_dispatch( primary_cmd, ( pass->scene_renderer->total_visible_meshes_instances_count + 63u ) / 64u, 1u, 1u );
}

void
crude_gfx_culling_late_pass_register
(
  _In_ crude_gfx_culling_late_pass                        *pass
)
{
}

crude_gfx_render_graph_pass_container
crude_gfx_culling_late_pass_pack
(
  _In_ crude_gfx_culling_late_pass                        *pass
)
{
  crude_gfx_render_graph_pass_container container = crude_gfx_render_graph_pass_container_empty();
  container.ctx = pass;
  container.render = crude_gfx_culling_late_pass_render;
  return container;
}