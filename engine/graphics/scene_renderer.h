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

  crude_gfx_light_cpu                                     *lights;

  void                                                    *imgui_context;

  crude_scene                                             *scene;
  crude_stack_allocator                                   *temporary_allocator;

  crude_gfx_scene_renderer_meshes_resources                meshes_resources;
  crude_gfx_scene_renderer_meshlets_resources              meshlets_resources;
  crude_gfx_scene_renderer_frame_resource                  frame_resources;

  crude_gfx_buffer_handle                                  debug_line_vertices_sb[ CRUDE_GFX_MAX_SWAPCHAIN_IMAGES ];
  crude_gfx_buffer_handle                                  debug_line_commands_sb[ CRUDE_GFX_MAX_SWAPCHAIN_IMAGES ];

  crude_gfx_buffer_handle                                  lights_sb;
  crude_gfx_buffer_handle                                  lights_bins_sb[ CRUDE_GFX_MAX_SWAPCHAIN_IMAGES ];
  crude_gfx_buffer_handle                                  lights_tiles_sb[ CRUDE_GFX_MAX_SWAPCHAIN_IMAGES ];
  crude_gfx_buffer_handle                                  lights_indices_sb[ CRUDE_GFX_MAX_SWAPCHAIN_IMAGES ];
  crude_gfx_buffer_handle                                  pointlight_world_to_clip_sb[ CRUDE_GFX_MAX_SWAPCHAIN_IMAGES ];

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
crude_gfx_scene_renderer_scene_add_to_descriptor_set_creation
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

/******************************
 * 
 * Scene Renderer Meshes Resources
 * 
 *******************************/
CRUDE_API void
crude_gfx_scene_renderer_meshes_resources_initialize
(
  _In_ crude_gfx_scene_renderer_meshes_resources          *meshes_resources,
  _In_ crude_gfx_renderer                                 *renderer,
  _In_ crude_heap_allocator                               *allocator,
  _In_ crude_stack_allocator                              *temporary_allocator
);

CRUDE_API void
crude_gfx_scene_renderer_meshes_resources_deinitialize
(
  _In_ crude_gfx_scene_renderer_meshes_resources          *meshes_resources
);

CRUDE_API void
crude_gfx_scene_renderer_meshes_resources_update_frame
(
  _In_ crude_gfx_scene_renderer_meshes_resources          *meshes_resources,
  _In_ uint32                                              depth_pyramid_texture_index
);

CRUDE_API void
crude_gfx_scene_renderer_meshes_resources_add_to_descriptor_set_creation
(
  _In_ crude_gfx_descriptor_set_creation                  *creation,
  _In_ crude_gfx_scene_renderer_meshes_resources          *meshes_resources
);

/******************************
 * 
 * Scene Renderer Meshlets Resources
 * 
 *******************************/
CRUDE_API void
crude_gfx_scene_renderer_meshlets_resources_initialize
(
  _In_ crude_gfx_scene_renderer_meshlets_resources        *meshlets_resources,
  _In_ crude_gfx_renderer                                 *renderer,
  _In_ crude_heap_allocator                               *allocator
);

CRUDE_API void
crude_gfx_scene_renderer_meshlets_resources_deinitialize
(
  _In_ crude_gfx_scene_renderer_meshlets_resources        *meshlets_resources
);

CRUDE_API void
crude_gfx_scene_renderer_meshlets_resources_add_to_descriptor_set_creation
(
  _In_ crude_gfx_descriptor_set_creation                  *creation,
  _In_ crude_gfx_scene_renderer_meshlets_resources        *meshlets_resources
);