#pragma once

#include <graphics/asynchronous_loader.h>
#include <graphics/scene_renderer_resources.h>
#include <graphics/passes/imgui_pass.h>
#include <graphics/passes/gbuffer_early_pass.h>
#include <graphics/passes/gbuffer_late_pass.h>
#include <graphics/passes/depth_pyramid_pass.h>
#include <graphics/passes/culling_early_pass.h>
#include <graphics/passes/culling_late_pass.h>
#include <graphics/passes/debug_pass.h>
#include <graphics/passes/light_pass.h>
#include <graphics/passes/pointlight_shadow_pass.h>
#include <graphics/passes/postprocessing_pass.h>
#include <graphics/passes/ray_tracing_solid_pass.h>
#include <graphics/passes/indirect_light_pass.h>

typedef struct crude_scene crude_scene;

typedef struct crude_gfx_scene_renderer_creation
{
  crude_scene                                             *scene;
  void                                                    *imgui_context;
  crude_gfx_renderer                                      *renderer;
  crude_gfx_asynchronous_loader                           *async_loader;
  void                                                    *task_scheduler;
  crude_heap_allocator                                    *allocator;
  crude_heap_allocator                                    *resources_allocator;
  crude_stack_allocator                                   *temporary_allocator;
} crude_gfx_scene_renderer_creation;

typedef struct crude_gfx_scene_renderer_options
{
  XMFLOAT3                                                 background_color;
  float32                                                  background_intensity;
} crude_gfx_scene_renderer_options;

typedef struct crude_gfx_scene_renderer
{
  /***********************
   * Context 
   **********************/
  void                                                    *world;
  void                                                    *task_scheduler;
  void                                                    *imgui_context;
  crude_gfx_renderer                                      *renderer;
  crude_gfx_render_graph                                  *render_graph;
  crude_gfx_asynchronous_loader                           *async_loader;
  crude_scene                                             *scene;
  crude_heap_allocator                                    *allocator;
  crude_heap_allocator                                    *resources_allocator;
  crude_stack_allocator                                   *temporary_allocator;
  
  /***********************
   * Base
   **********************/
  crude_gfx_scene_renderer_options                         options;
  crude_heap_allocator                                     gltf_allocator;

  /***********************
   * Common CPU & GPU Data
   **********************/
  crude_gfx_renderer_sampler                              *samplers;
  crude_gfx_renderer_texture                              *images;
  crude_gfx_renderer_buffer                               *buffers;
  crude_gfx_buffer_handle                                  scene_cb;
  
  /***********************
   * Common Mesh & Meshlets CPU & GPU Data
   **********************/
  crude_gfx_mesh_cpu                                      *meshes;
  crude_gfx_mesh_instance_cpu                             *meshes_instances;
  crude_gfx_meshlet_gpu                                   *meshlets;
  crude_gfx_meshlet_vertex_gpu                            *meshlets_vertices;
  uint32                                                  *meshlets_vertices_indices;
  uint8                                                   *meshlets_triangles_indices;
  uint32                                                   total_meshes_instances_count;
  crude_gfx_buffer_handle                                  meshes_draws_sb;
  crude_gfx_buffer_handle                                  meshes_instances_draws_sb;
  crude_gfx_buffer_handle                                  meshes_bounds_sb;
  crude_gfx_buffer_handle                                  meshlets_sb;
  crude_gfx_buffer_handle                                  meshlets_vertices_sb;
  crude_gfx_buffer_handle                                  meshlets_vertices_indices_sb;
  crude_gfx_buffer_handle                                  meshlets_triangles_indices_sb;
  crude_gfx_buffer_handle                                  mesh_task_indirect_commands_early_sb[ CRUDE_GFX_MAX_SWAPCHAIN_IMAGES ];
  crude_gfx_buffer_handle                                  mesh_task_indirect_commands_late_sb[ CRUDE_GFX_MAX_SWAPCHAIN_IMAGES ];
  crude_gfx_buffer_handle                                  mesh_task_indirect_count_early_sb[ CRUDE_GFX_MAX_SWAPCHAIN_IMAGES ];
  crude_gfx_buffer_handle                                  mesh_task_indirect_count_late_sb[ CRUDE_GFX_MAX_SWAPCHAIN_IMAGES ];

  /***********************
   * Common Debug CPU & GPU Data
   **********************/
  crude_gfx_buffer_handle                                  debug_cubes_instances_sb[ CRUDE_GFX_MAX_SWAPCHAIN_IMAGES ];
  crude_gfx_buffer_handle                                  debug_line_vertices_sb[ CRUDE_GFX_MAX_SWAPCHAIN_IMAGES ];
  crude_gfx_buffer_handle                                  debug_commands_sb[ CRUDE_GFX_MAX_SWAPCHAIN_IMAGES ];
  
  /***********************
   * Common Lights & Shadows CPU & GPU Data
   **********************/
  crude_gfx_light_cpu                                     *lights;
  crude_gfx_buffer_handle                                  lights_sb;
  crude_gfx_buffer_handle                                  lights_bins_sb[ CRUDE_GFX_MAX_SWAPCHAIN_IMAGES ];
  crude_gfx_buffer_handle                                  lights_tiles_sb[ CRUDE_GFX_MAX_SWAPCHAIN_IMAGES ];
  crude_gfx_buffer_handle                                  lights_indices_sb[ CRUDE_GFX_MAX_SWAPCHAIN_IMAGES ];
  crude_gfx_buffer_handle                                  pointlight_world_to_clip_sb[ CRUDE_GFX_MAX_SWAPCHAIN_IMAGES ];
  
  /***********************
   * Common Settings Data
   **********************/
  bool                                                     ray_trace_shadows;

  /***********************
   * Ray Tracing CPU & GPU Data
   **********************/

#ifdef CRUDE_GRAPHICS_RAY_TRACING_ENABLED
  crude_gfx_buffer_handle                                  geometry_transform_asb;
  VkAccelerationStructureKHR                               blas;
  VkAccelerationStructureKHR                               tlas;
  crude_gfx_buffer_handle                                  blas_buffer;
  crude_gfx_buffer_handle                                  tlas_buffer;
#endif /* CRUDE_GRAPHICS_RAY_TRACING_ENABLED */

  /***********************
   * Render Graph Passes 
   **********************/
  crude_gfx_culling_early_pass                             culling_early_pass;
  crude_gfx_culling_late_pass                              culling_late_pass;
  crude_gfx_gbuffer_early_pass                             gbuffer_early_pass;
  crude_gfx_gbuffer_late_pass                              gbuffer_late_pass;
  crude_gfx_imgui_pass                                     imgui_pass;
  crude_gfx_depth_pyramid_pass                             depth_pyramid_pass;
  crude_gfx_pointlight_shadow_pass                         pointlight_shadow_pass;
  crude_gfx_debug_pass                                     debug_pass;
  crude_gfx_light_pass                                     light_pass;
  crude_gfx_postprocessing_pass                            postprocessing_pass;
#ifdef CRUDE_GRAPHICS_RAY_TRACING_ENABLED
  crude_gfx_ray_tracing_solid_pass                         ray_tracing_solid_pass;
  crude_gfx_indirect_light_pass                            indirect_light_pass;
#endif /* CRUDE_GRAPHICS_RAY_TRACING_ENABLED */
} crude_gfx_scene_renderer;


/**
 *
 * Renderer Scene
 * 
 */
CRUDE_API void
crude_gfx_scene_renderer_initialize
(
  _In_ crude_gfx_scene_renderer                           *scene_renderer,
  _In_ crude_gfx_scene_renderer_creation                  *creation
);

CRUDE_API void
crude_gfx_scene_renderer_deinitialize
(
  _In_ crude_gfx_scene_renderer                           *scene_renderer
);

CRUDE_API void
crude_gfx_scene_renderer_submit_draw_task
(
  _In_ crude_gfx_scene_renderer                           *scene_renderer,
  _In_ bool                                                use_secondary
);

CRUDE_API void
crude_gfx_scene_renderer_register_passes
(
  _In_ crude_gfx_scene_renderer                           *scene_renderer,
  _In_ crude_gfx_render_graph                             *render_graph
);

CRUDE_API void
crude_gfx_scene_renderer_on_resize
(
  _In_ crude_gfx_scene_renderer                           *scene_renderer
);

/**
 *
 * Renderer Scene Utils
 * 
 */
CRUDE_API void
crude_gfx_scene_renderer_add_debug_resources_to_descriptor_set_creation
(
  _In_ crude_gfx_descriptor_set_creation                  *creation,
  _In_ crude_gfx_scene_renderer                           *scene_renderer,
  _In_ uint32                                              frame
);

CRUDE_API void
crude_gfx_mesh_cpu_to_mesh_draw_gpu
(
  _In_ crude_gfx_device                                   *gpu,
  _In_ crude_gfx_mesh_cpu const                           *mesh,
  _Out_ crude_gfx_mesh_draw_gpu                           *mesh_draw_gpu
);