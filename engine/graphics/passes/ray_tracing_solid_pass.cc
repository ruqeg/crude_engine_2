#include <engine/core/profiler.h>
#include <engine/core/hashmapstr.h>
#include <engine/scene/scene_ecs.h>
#include <engine/graphics/scene_renderer.h>

#define RAY_TRACING_SOLID
#include <engine/graphics/shaders/ray_tracing_solid.crude_shader>

#include <engine/graphics/passes/ray_tracing_solid_pass.h>

#if CRUDE_GFX_RAY_TRACING_SOLID_DEBUG_ENABLED

void
crude_gfx_ray_tracing_solid_pass_initialize
(
  _In_ crude_gfx_ray_tracing_solid_pass                   *pass,
  _In_ crude_gfx_scene_renderer                           *scene_renderer
)
{
  pass->scene_renderer = scene_renderer;
}

void
crude_gfx_ray_tracing_solid_pass_deinitialize
(
  _In_ crude_gfx_ray_tracing_solid_pass                   *pass
)
{
}

void
crude_gfx_ray_tracing_solid_pass_render
(
  _In_ void                                               *ctx,
  _In_ crude_gfx_cmd_buffer                               *primary_cmd
)
{
  return;
  crude_gfx_ray_tracing_solid_pass                        *pass;
  crude_gfx_texture                                       *ray_tracing_solid_texture;
  crude_gfx_pointshadow_culling_pass_push_constant_        push_constant;
  crude_gfx_texture_handle                                 ray_tracing_solid_texture_handle;
  crude_gfx_pipeline_handle                                pipeline;
  
  pass = CRUDE_REINTERPRET_CAST( crude_gfx_ray_tracing_solid_pass*, ctx );
  
  ray_tracing_solid_texture_handle = crude_gfx_render_graph_builder_access_resource_by_name( pass->scene_renderer->render_graph->builder, "ray_tracing_solid" )->resource_info.texture.handle;
  ray_tracing_solid_texture = crude_gfx_access_texture( pass->scene_renderer->gpu, ray_tracing_solid_texture_handle );

  pipeline = crude_gfx_access_technique_pass_by_name( pass->scene_renderer->gpu, "ray_tracing_debug", "ray_tracing_solid" )->pipeline;
  
  crude_gfx_cmd_push_marker( primary_cmd, "crude_gfx_ray_tracing_solid_pass_render" );
  crude_gfx_cmd_bind_pipeline( primary_cmd, pipeline );
  crude_gfx_cmd_bind_bindless_descriptor_set( primary_cmd );
  crude_gfx_cmd_bind_acceleration_structure_descriptor_set( primary_cmd, pass->scene_renderer->acceleration_stucture_ds );

  CRUDE_ASSERT( sizeof( VkAccelerationStructureKHR ) == sizeof( uint64 ) );

  push_constant = CRUDE_COMPOUNT_EMPTY( crude_gfx_pointshadow_culling_pass_push_constant_ );
  push_constant.scene = pass->scene_renderer->scene_hga.gpu_address;
  push_constant.acceleration_stucture_ref = CRUDE_CAST( uint64, pass->scene_renderer->vk_tlas );
  push_constant.mesh_draws = pass->scene_renderer->model_renderer_resources_manager->meshes_draws_hga.gpu_address;
  push_constant.mesh_instance_draws = pass->scene_renderer->meshes_instances_draws_hga.gpu_address;
  push_constant.sbt_offset = 0;
  push_constant.sbt_stride = pass->scene_renderer->gpu->ray_tracing_pipeline_properties.shaderGroupHandleAlignment;
  push_constant.miss_index = 0;
  push_constant.out_image_index = ray_tracing_solid_texture_handle.index;
  crude_gfx_cmd_push_constant( primary_cmd, &push_constant, sizeof( push_constant ) );
   
  crude_gfx_cmd_add_image_barrier( primary_cmd, ray_tracing_solid_texture, CRUDE_GFX_RESOURCE_STATE_UNORDERED_ACCESS, 0, 1, false );
  crude_gfx_cmd_trace_rays( primary_cmd, pipeline, ray_tracing_solid_texture->width, ray_tracing_solid_texture->height, 1u );
  crude_gfx_cmd_add_image_barrier( primary_cmd, ray_tracing_solid_texture, CRUDE_GFX_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, 0, 1, false );
  
  crude_gfx_cmd_pop_marker( primary_cmd );
}

crude_gfx_render_graph_pass_container
crude_gfx_ray_tracing_solid_pass_pack
(
  _In_ crude_gfx_ray_tracing_solid_pass                   *pass
)
{
  crude_gfx_render_graph_pass_container container = crude_gfx_render_graph_pass_container_empty();
  container.ctx = pass;
  container.render = crude_gfx_ray_tracing_solid_pass_render;
  return container;
}

#endif /* CRUDE_GFX_RAY_TRACING_SOLID_DEBUG_ENABLED */