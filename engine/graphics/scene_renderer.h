#pragma once

#include <graphics/asynchronous_loader.h>
#include <graphics/scene_renderer_resources.h>
#include <graphics/passes/imgui_pass.h>
#include <graphics/passes/meshlet_pass.h>
#include <graphics/passes/mesh_pass.h>
#include <graphics/passes/depth_pyramid_pass.h>
#include <graphics/passes/mesh_culling_pass.h>
#include <graphics/passes/debug_pass.h>

typedef struct crude_gfx_scene_renderer_creation
{
  crude_entity                                             node;
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
  
  uint32                                                   total_meshes_instances_count;

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
  
  crude_allocator_container                                allocator_container;

  crude_gfx_meshlet_pass                                   meshlet_early_pass;
  crude_gfx_meshlet_pass                                   meshlet_late_pass;
  crude_gfx_mesh_pass                                      mesh_pass;
  crude_gfx_imgui_pass                                     imgui_pass;
  crude_gfx_depth_pyramid_pass                             depth_pyramid_pass;
  crude_gfx_mesh_culling_pass                              mesh_culling_early_pass;
  crude_gfx_mesh_culling_pass                              mesh_culling_late_pass;
  crude_gfx_debug_pass                                     debug_pass;
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