#pragma once

#include <engine/graphics/asynchronous_loader.h>
#include <engine/graphics/scene_renderer_resources.h>
#include <engine/graphics/passes/imgui_pass.h>
#include <engine/graphics/passes/gbuffer_early_pass.h>
#include <engine/graphics/passes/gbuffer_late_pass.h>
#include <engine/graphics/passes/depth_pyramid_pass.h>
#include <engine/graphics/passes/culling_early_pass.h>
#include <engine/graphics/passes/culling_late_pass.h>
#include <engine/graphics/passes/debug_pass.h>
#include <engine/graphics/passes/light_pass.h>
#include <engine/graphics/passes/pointlight_shadow_pass.h>
#include <engine/graphics/passes/postprocessing_pass.h>
#include <engine/graphics/passes/ray_tracing_solid_pass.h>
#include <engine/graphics/passes/indirect_light_pass.h>
#include <engine/graphics/passes/transparent_pass.h>
#include <engine/graphics/model_renderer_resources_manager.h>

typedef enum crude_gfx_model_renderer_resoruces_instances_type
{
  CRUDE_GFX_MODEL_RENDERER_RESOURCES_INSTANCE_TYPE_GLTF,
#if CRUDE_DEVELOP
  CRUDE_GFX_MODEL_RENDERER_RESOURCES_INSTANCE_TYPE_DUBUG_GLTF,
  CRUDE_GFX_MODEL_RENDERER_RESOURCES_INSTANCE_TYPE_DUBUG_COLLISION,
#endif
} crude_gfx_model_renderer_resoruces_instances_type;

typedef struct crude_gfx_model_renderer_resources_instance
{
  crude_gfx_model_renderer_resources                       model_renderer_resources;
  crude_entity                                             node;
  crude_gfx_model_renderer_resoruces_instances_type        type;
} crude_gfx_model_renderer_resources_instance;

typedef struct crude_gfx_scene_renderer_creation
{
  crude_gfx_model_renderer_resources_manager              *model_renderer_resources_manager;
  void                                                    *imgui_context;
  crude_gfx_asynchronous_loader                           *async_loader;
  void                                                    *task_scheduler;
  crude_heap_allocator                                    *allocator;
  crude_stack_allocator                                   *temporary_allocator;
  bool                                                     imgui_pass_enalbed;
} crude_gfx_scene_renderer_creation;

typedef struct crude_gfx_scene_renderer_options
{
  crude_entity                                             camera_node;
  XMFLOAT3                                                 background_color;
  float32                                                  background_intensity;
  XMFLOAT3                                                 ambient_color;
  float32                                                  ambient_intensity;
  float32                                                  absolute_time;
  char const                                              *hdr_pre_tonemapping_texture_name;
  char const                                              *depth_texture_name;
#if CRUDE_DEVELOP
  bool                                                     hide_collision;
  bool                                                     hide_debug_gltf;
#endif
} crude_gfx_scene_renderer_options;

typedef struct crude_gfx_scene_renderer
{
  /***********************
   * Context 
   **********************/
  void                                                    *task_scheduler;
  void                                                    *imgui_context;
  crude_gfx_device                                        *gpu;
  crude_gfx_render_graph                                  *render_graph;
  crude_gfx_asynchronous_loader                           *async_loader;
  crude_heap_allocator                                    *allocator;
  crude_stack_allocator                                   *temporary_allocator;
  crude_gfx_model_renderer_resources_manager              *model_renderer_resources_manager;
  
  /***********************
   * Base
   **********************/
  crude_gfx_scene_renderer_options                         options;
  bool                                                     imgui_pass_enalbed;

  /***********************
   * Common Mesh & Meshlets CPU & GPU Data
   **********************/
  crude_gfx_buffer_handle                                  scene_cb;
  
  crude_gfx_model_renderer_resources_instance             *model_renderer_resoruces_instances;
  uint32                                                   total_visible_meshes_instances_count;
  uint32                                                   total_meshes_instances_count;
  crude_gfx_buffer_handle                                  meshes_instances_draws_sb;
  crude_gfx_buffer_handle                                  mesh_task_indirect_commands_sb[ CRUDE_GRAPHICS_MAX_SWAPCHAIN_IMAGES ];
  crude_gfx_buffer_handle                                  mesh_task_indirect_commands_culled_sb[ CRUDE_GRAPHICS_MAX_SWAPCHAIN_IMAGES ];
  crude_gfx_buffer_handle                                  mesh_task_indirect_count_sb[ CRUDE_GRAPHICS_MAX_SWAPCHAIN_IMAGES ];
  
  uint32                                                   total_meshes_instances_buffer_capacity;

  /***********************
   * Common Debug CPU & GPU Data
   **********************/
  crude_gfx_buffer_handle                                  debug_cubes_instances_sb[ CRUDE_GRAPHICS_MAX_SWAPCHAIN_IMAGES ];
  crude_gfx_buffer_handle                                  debug_line_vertices_sb[ CRUDE_GRAPHICS_MAX_SWAPCHAIN_IMAGES ];
  crude_gfx_buffer_handle                                  debug_commands_sb[ CRUDE_GRAPHICS_MAX_SWAPCHAIN_IMAGES ];
  
  /***********************
   * Common Lights & Shadows CPU & GPU Data
   **********************/
  crude_gfx_light_cpu                                     *lights;
  crude_gfx_buffer_handle                                  lights_sb;
  crude_gfx_buffer_handle                                  lights_bins_sb[ CRUDE_GRAPHICS_MAX_SWAPCHAIN_IMAGES ];
  crude_gfx_buffer_handle                                  lights_tiles_sb[ CRUDE_GRAPHICS_MAX_SWAPCHAIN_IMAGES ];
  crude_gfx_buffer_handle                                  lights_indices_sb[ CRUDE_GRAPHICS_MAX_SWAPCHAIN_IMAGES ];
  crude_gfx_buffer_handle                                  pointlight_world_to_clip_sb[ CRUDE_GRAPHICS_MAX_SWAPCHAIN_IMAGES ];

  /***********************
   * Ray Tracing CPU & GPU Data
   **********************/
#if CRUDE_GRAPHICS_RAY_TRACING_ENABLED
  VkAccelerationStructureKHR                              *vk_blases;
  crude_gfx_buffer_handle                                 *blases_buffers;
  VkAccelerationStructureKHR                               vk_tlas;
  crude_gfx_buffer_handle                                  tlas_buffer;
  crude_gfx_buffer_handle                                  tlas_instances_buffer_handle;
  crude_gfx_buffer_handle                                  tlas_scratch_buffer_handle;
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
  crude_gfx_transparent_pass                               transparent_pass;
#if CRUDE_GRAPHICS_RAY_TRACING_ENABLED
#if CRUDE_DEBUG_RAY_TRACING_SOLID_PASS
  crude_gfx_ray_tracing_solid_pass                         ray_tracing_solid_pass;
#endif
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

CRUDE_API bool
crude_gfx_scene_renderer_update_instances_from_node
(
  _In_ crude_gfx_scene_renderer                           *scene_renderer,
  _In_ crude_entity                                        main_node
);

CRUDE_API void
crude_gfx_scene_renderer_rebuild_light_gpu_buffers
(
  _In_ crude_gfx_scene_renderer                           *scene_renderer
);

CRUDE_API void
crude_gfx_scene_renderer_initialize_pases
(
  _In_ crude_gfx_scene_renderer                           *scene_renderer
);

CRUDE_API void
crude_gfx_scene_renderer_deinitialize_passes
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