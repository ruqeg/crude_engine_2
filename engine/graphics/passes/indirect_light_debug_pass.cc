#include <engine/core/profiler.h>
#include <engine/core/hashmapstr.h>
#include <engine/scene/scene_ecs.h>
#include <engine/graphics/scene_renderer.h>

#include <engine/graphics/passes/indirect_light_debug_pass.h>

void
crude_gfx_indirect_light_debug_pass_initialize
(
  _In_ crude_gfx_indirect_light_debug_pass                *pass,
  _In_ crude_gfx_scene_renderer                           *scene_renderer
)
{
  pass->scene_renderer = scene_renderer;
}

void
crude_gfx_indirect_light_debug_pass_deinitialize
(
  _In_ crude_gfx_indirect_light_debug_pass                *pass
)
{
}

void
crude_gfx_indirect_light_debug_pass_render
(
  _In_ void                                               *ctx,
  _In_ crude_gfx_cmd_buffer                               *primary_cmd
)
{
  return;
  crude_gfx_indirect_light_debug_pass                     *pass;
  crude_gfx_device                                        *gpu;
  crude_gfx_probe_debug_push_constant_                     push_constant;
  crude_gfx_pipeline_handle                                pipeline;
  uint32                                                   probe_count;

  pass = CRUDE_REINTERPRET_CAST( crude_gfx_indirect_light_debug_pass*, ctx );
  
  probe_count = pass->scene_renderer->options.indirect_light.probe_count_x * pass->scene_renderer->options.indirect_light.probe_count_y * pass->scene_renderer->options.indirect_light.probe_count_z;

  gpu = pass->scene_renderer->gpu;
  
  pipeline = crude_gfx_access_technique_pass_by_name( gpu, "ddgi", "probe_debug" )->pipeline;

  crude_gfx_cmd_bind_pipeline( primary_cmd, pipeline );
  crude_gfx_cmd_bind_bindless_descriptor_set( primary_cmd );
  
  push_constant = CRUDE_COMPOUNT_EMPTY( crude_gfx_probe_debug_push_constant_ );
  push_constant.ddgi = pass->scene_renderer->ddgi_hga.gpu_address;
  push_constant.meshlets = pass->scene_renderer->model_renderer_resources_manager->meshlets_hga.gpu_address;
  push_constant.mesh_draws = pass->scene_renderer->model_renderer_resources_manager->meshes_draws_hga.gpu_address;
  push_constant.triangles_indices = pass->scene_renderer->model_renderer_resources_manager->meshlets_triangles_indices_hga.gpu_address;
  push_constant.vertices_indices = pass->scene_renderer->model_renderer_resources_manager->meshlets_vertices_indices_hga.gpu_address;
  push_constant.vertices = pass->scene_renderer->model_renderer_resources_manager->meshlets_vertices_hga.gpu_address;
  push_constant.mesh_instance_draws = pass->scene_renderer->meshes_instances_draws_hga.gpu_address;
  push_constant.scene = pass->scene_renderer->scene_hga.gpu_address;
  push_constant.probe_mesh_draw_index = crude_gfx_model_renderer_resources_manager_access_model_renderer_resources( pass->scene_renderer->model_renderer_resources_manager, pass->scene_renderer->probe_model_renderer_resources_instance.model_renderer_resources_handle )->meshes[ 0 ].gpu_mesh_global_index;
  crude_gfx_cmd_push_constant( primary_cmd, &push_constant, sizeof( push_constant ) );

  crude_gfx_cmd_draw_mesh_task( primary_cmd, ( probe_count + 31 ) / 32, 1u, 1 );
}

crude_gfx_render_graph_pass_container
crude_gfx_indirect_light_debug_pass_pack
(
  _In_ crude_gfx_indirect_light_debug_pass                *pass
)
{
  crude_gfx_render_graph_pass_container container = crude_gfx_render_graph_pass_container_empty();
  container.ctx = pass;
  container.render = crude_gfx_indirect_light_debug_pass_render;
  return container;
}