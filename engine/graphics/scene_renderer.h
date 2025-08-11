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

typedef struct crude_scene crude_scene;

typedef struct crude_gfx_scene_renderer_creation
{
  crude_scene                                             *scene;
  void                                                    *imgui_context;
  crude_gfx_renderer                                      *renderer;
  crude_gfx_asynchronous_loader                           *async_loader;
  void                                                    *task_scheduler;
  crude_allocator_container                                allocator_container;
  crude_stack_allocator                                   *temporary_allocator;
} crude_gfx_scene_renderer_creation;

typedef struct crude_gfx_scene_renderer
{
  void                                                    *world;

  crude_gfx_renderer                                      *renderer;
  crude_gfx_render_graph                                  *render_graph;
  crude_gfx_asynchronous_loader                           *async_loader;
  void                                                    *task_scheduler;

  crude_gfx_renderer_sampler                              *samplers;
  crude_gfx_renderer_texture                              *images;
  crude_gfx_renderer_buffer                               *buffers;

  crude_gfx_mesh_cpu                                      *meshes;
  crude_gfx_mesh_instance_cpu                             *meshes_instances;

  crude_gfx_meshlet_gpu                                   *meshlets;
  crude_gfx_meshlet_vertex_gpu                            *meshlets_vertices;
  uint32                                                  *meshlets_vertices_indices;
  uint8                                                   *meshlets_triangles_indices;

  crude_gfx_light_cpu                                     *lights;

  uint32                                                   total_meshes_instances_count;

  void                                                    *imgui_context;

  crude_scene                                             *scene;
  crude_stack_allocator                                   *temporary_allocator;

  crude_gfx_buffer_handle                                  scene_cb;

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

  crude_gfx_buffer_handle                                  debug_line_vertices_sb[ CRUDE_GFX_MAX_SWAPCHAIN_IMAGES ];
  crude_gfx_buffer_handle                                  debug_line_commands_sb[ CRUDE_GFX_MAX_SWAPCHAIN_IMAGES ];

  crude_gfx_buffer_handle                                  lights_sb;
  crude_gfx_buffer_handle                                  lights_bins_sb[ CRUDE_GFX_MAX_SWAPCHAIN_IMAGES ];
  crude_gfx_buffer_handle                                  lights_tiles_sb[ CRUDE_GFX_MAX_SWAPCHAIN_IMAGES ];
  crude_gfx_buffer_handle                                  lights_indices_sb[ CRUDE_GFX_MAX_SWAPCHAIN_IMAGES ];
  
  crude_gfx_buffer_handle                                  light_meshlet_draw_commands_sb[ CRUDE_GFX_MAX_SWAPCHAIN_IMAGES ];
  crude_gfx_buffer_handle                                  light_meshlet_instances_sb[ CRUDE_GFX_MAX_SWAPCHAIN_IMAGES ];
  crude_gfx_buffer_handle                                  per_light_meshlet_instances_sb[ CRUDE_GFX_MAX_SWAPCHAIN_IMAGES ];
  
  crude_gfx_buffer_handle                                  pointlight_world_to_view_sb[ CRUDE_GFX_MAX_SWAPCHAIN_IMAGES ];
  crude_gfx_buffer_handle                                  pointlight_spheres_sb[ CRUDE_GFX_MAX_SWAPCHAIN_IMAGES ];

  crude_allocator_container                                allocator_container;

  crude_gfx_culling_early_pass                             culling_early_pass;
  crude_gfx_culling_late_pass                              culling_late_pass;
  crude_gfx_gbuffer_early_pass                             gbuffer_early_pass;
  crude_gfx_gbuffer_late_pass                              gbuffer_late_pass;
  crude_gfx_imgui_pass                                     imgui_pass;
  crude_gfx_depth_pyramid_pass                             depth_pyramid_pass;
  crude_gfx_pointlight_shadow_pass                         pointlight_shadow_pass;
  crude_gfx_debug_pass                                     debug_pass;
  crude_gfx_light_pass                                     light_pass;
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
crude_gfx_scene_renderer_add_meshlet_resources_to_descriptor_set_creation
(
  _In_ crude_gfx_descriptor_set_creation                  *creation,
  _In_ crude_gfx_scene_renderer                           *scene_renderer,
  _In_ uint32                                              frame
);

CRUDE_API void
crude_gfx_scene_renderer_add_scene_resources_to_descriptor_set_creation
(
  _In_ crude_gfx_descriptor_set_creation                  *creation,
  _In_ crude_gfx_scene_renderer                           *scene_renderer,
  _In_ uint32                                              frame
);

CRUDE_API void
crude_gfx_scene_renderer_add_mesh_resources_to_descriptor_set_creation
(
  _In_ crude_gfx_descriptor_set_creation                  *creation,
  _In_ crude_gfx_scene_renderer                           *scene_renderer,
  _In_ uint32                                              frame
);

CRUDE_API void
crude_gfx_scene_renderer_add_light_resources_to_descriptor_set_creation
(
  _In_ crude_gfx_descriptor_set_creation                  *creation,
  _In_ crude_gfx_scene_renderer                           *scene_renderer,
  _In_ uint32                                              frame
);