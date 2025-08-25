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
  crude_heap_allocator                                    *allocator;
  crude_stack_allocator                                   *temporary_allocator;
} crude_gfx_scene_renderer_creation;

typedef struct crude_gfx_scene_renderer
{
  void                                                    *world;
  crude_gfx_renderer                                      *renderer;
  crude_gfx_render_graph                                  *render_graph;
  crude_gfx_asynchronous_loader                           *async_loader;
  void                                                    *task_scheduler;
  crude_heap_allocator                                    *allocator;
  crude_stack_allocator                                   *temporary_allocator;
  void                                                    *imgui_context;
  crude_scene                                             *scene;

  crude_gfx_renderer_sampler                              *samplers;
  crude_gfx_renderer_texture                              *images;
  crude_gfx_renderer_buffer                               *buffers;

  crude_gfx_scene_renderer_meshes_resources                meshes_resources;
  crude_gfx_scene_renderer_meshlets_resources              meshlets_resources;
  crude_gfx_scene_renderer_frame_resources                 frame_resources;
  crude_gfx_scene_renderer_lights_resources                light_resources;
  crude_gfx_scene_renderer_debug_resources                 debug_resources;

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
  _In_ crude_heap_allocator                               *allocator
);

CRUDE_API void
crude_gfx_scene_renderer_meshes_resources_initialize_post_registred
(
  _In_ crude_gfx_scene_renderer_meshes_resources          *meshes_resources,
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
 * Scene Renderer Frame Resources
 * 
 *******************************/
CRUDE_API void
crude_gfx_scene_renderer_frame_resources_initialize
(
  _In_ crude_gfx_scene_renderer_frame_resources           *frame_resources,
  _In_ crude_gfx_renderer                                 *renderer,
  _In_ crude_heap_allocator                               *allocator
);

CRUDE_API void
crude_gfx_scene_renderer_frame_resources_deinitialize
(
  _In_ crude_gfx_scene_renderer_frame_resources           *frame_resources
);

CRUDE_API void
crude_gfx_scene_renderer_frame_resources_update_frame
(
  _In_ crude_gfx_scene_renderer_frame_resources           *frame_resources,
  _In_ crude_gfx_scene_renderer_meshes_resources          *meshes_resources,
  _In_ crude_gfx_scene_renderer_lights_resources          *lights_resources,
  _In_ crude_scene                                        *scene,
  _In_ uint32                                              tetrahedron_shadow_texture_index
);

CRUDE_API void
crude_gfx_scene_renderer_frame_resources_add_to_descriptor_set_creation
(
  _In_ crude_gfx_descriptor_set_creation                  *creation,
  _In_ crude_gfx_scene_renderer_frame_resources           *frame_resources
);

/******************************
 * 
 * Scene Renderer Lights Resources
 * 
 *******************************/
CRUDE_API void
crude_gfx_scene_renderer_lights_resources_initialize
(
  _In_ crude_gfx_scene_renderer_lights_resources          *lights_resources,
  _In_ crude_gfx_renderer                                 *renderer,
  _In_ crude_heap_allocator                               *allocator
);

CRUDE_API void
crude_gfx_scene_renderer_lights_resources_initialize_post_registred
(
  _In_ crude_gfx_scene_renderer_lights_resources          *lights_resources
);

CRUDE_API void
crude_gfx_scene_renderer_lights_resources_deinitialize
(
  _In_ crude_gfx_scene_renderer_lights_resources          *lights_resources
);

CRUDE_API void
crude_gfx_scene_renderer_lights_resources_update_frame
(
  _In_ crude_gfx_scene_renderer_lights_resources          *lights_resources,
  _In_ crude_entity                                        camera_node,
  _In_ crude_stack_allocator                              *temporary_allocator
);

CRUDE_API void
crude_gfx_scene_renderer_lights_resources_add_to_descriptor_set_creation
(
  _In_ crude_gfx_descriptor_set_creation                  *creation,
  _In_ crude_gfx_scene_renderer_lights_resources          *lights_resources,
  _In_ uint32                                              frame
);

CRUDE_API void
crude_gfx_scene_renderer_lights_resources_on_resize
(
  _In_ crude_gfx_scene_renderer_lights_resources          *lights_resources
);

/******************************
 * 
 * Scene Renderer Debug Resources
 * 
 *******************************/
CRUDE_API void
crude_gfx_scene_renderer_debug_resources_initialize
(
  _In_ crude_gfx_scene_renderer_debug_resources           *debug_resources,
  _In_ crude_gfx_renderer                                 *renderer,
  _In_ crude_heap_allocator                               *allocator
);

CRUDE_API void
crude_gfx_scene_renderer_debug_resources_deinitialize
(
  _In_ crude_gfx_scene_renderer_debug_resources           *debug_resources
);

CRUDE_API void
crude_gfx_scene_renderer_debug_resources_update_frame
(
  _In_ crude_gfx_scene_renderer_debug_resources           *debug_resources
);

CRUDE_API void
crude_gfx_scene_renderer_debug_resources_add_to_descriptor_set_creation
(
  _In_ crude_gfx_descriptor_set_creation                  *creation,
  _In_ crude_gfx_scene_renderer_debug_resources           *debug_resources,
  _In_ uint32                                              frame
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
crude_gfx_scene_renderer_meshlets_resources_initialize_post_registred
(
  _In_ crude_gfx_scene_renderer_meshlets_resources        *meshlets_resources
);

CRUDE_API void
crude_gfx_scene_renderer_meshlets_resources_deinitialize
(
  _In_ crude_gfx_scene_renderer_meshlets_resources        *meshlets_resources
);

CRUDE_API void
crude_gfx_scene_renderer_meshlets_resources_update_frame
(
  _In_ crude_gfx_scene_renderer_meshlets_resources        *meshlets_resources
);

CRUDE_API void
crude_gfx_scene_renderer_meshlets_resources_add_to_descriptor_set_creation
(
  _In_ crude_gfx_descriptor_set_creation                  *creation,
  _In_ crude_gfx_scene_renderer_meshlets_resources        *meshlets_resources
);