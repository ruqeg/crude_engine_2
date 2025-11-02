#include <graphics/model_renderer_resources.h>

void
crude_gfx_mesh_cpu_to_mesh_draw_gpu
(
  _In_ crude_gfx_device                                   *gpu,
  _In_ crude_gfx_mesh_cpu const                           *mesh,
  _Out_ crude_gfx_mesh_draw_gpu                           *mesh_draw_gpu
)
{
  mesh_draw_gpu->textures.x = mesh->albedo_texture_handle.index; /* in case i will be confused in the future, bindless textures bineded by their handles, look at gpu_present... at least at the moment I write this comment */
  mesh_draw_gpu->textures.y = mesh->metallic_roughness_texture_handle.index;
  mesh_draw_gpu->textures.z = mesh->normal_texture_handle.index;
  mesh_draw_gpu->textures.w = mesh->occlusion_texture_handle.index;
  mesh_draw_gpu->albedo_color_factor = mesh->albedo_color_factor;
  mesh_draw_gpu->flags = mesh->flags;
  mesh_draw_gpu->mesh_index = mesh->gpu_mesh_index;
  mesh_draw_gpu->meshletes_count = mesh->meshlets_count;
  mesh_draw_gpu->meshletes_offset = mesh->meshlets_offset;
#ifdef CRUDE_GRAPHICS_RAY_TRACING_ENABLED
  mesh_draw_gpu->position_buffer = crude_gfx_get_buffer_device_address( gpu, mesh->position_buffer ) + mesh->position_offset;
  mesh_draw_gpu->texcoord_buffer = crude_gfx_get_buffer_device_address( gpu, mesh->texcoord_buffer ) + mesh->texcoord_offset;
  mesh_draw_gpu->index_buffer = crude_gfx_get_buffer_device_address( gpu, mesh->index_buffer ) + mesh->index_offset;
  mesh_draw_gpu->normal_buffer = crude_gfx_get_buffer_device_address( gpu, mesh->normal_buffer ) + mesh->normal_offset;
#endif /* CRUDE_GRAPHICS_RAY_TRACING_ENABLED */
}