#pragma once

#include <threads.h>

#include <engine/graphics/asynchronous_loader.h>
#include <engine/graphics/scene_renderer_resources.h>
#include <engine/graphics/passes/imgui_pass.h>
#include <engine/graphics/passes/gbuffer_early_pass.h>
#include <engine/graphics/passes/gbuffer_late_pass.h>
#include <engine/graphics/passes/depth_pyramid_pass.h>
#include <engine/graphics/passes/culling_early_pass.h>
#include <engine/graphics/passes/culling_late_pass.h>
#include <engine/graphics/passes/debug_pass.h>
#include <engine/graphics/passes/compose_pass.h>
#include <engine/graphics/passes/pointlight_shadow_pass.h>
#include <engine/graphics/passes/postprocessing_pass.h>
#include <engine/graphics/passes/ray_tracing_solid_pass.h>
#include <engine/graphics/passes/indirect_light_pass.h>
#include <engine/graphics/passes/transparent_pass.h>
#include <engine/graphics/passes/light_lut_pass.h>
#include <engine/graphics/passes/ssr_pass.h>
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
  XMFLOAT4X4                                               model_to_world;
  crude_gfx_model_renderer_resources                       model_renderer_resources;
  crude_gfx_model_renderer_resoruces_instances_type        type;
} crude_gfx_model_renderer_resources_instance;

typedef struct crude_gfx_scene_renderer_creation
{
  crude_gfx_model_renderer_resources_manager              *model_renderer_resources_manager;
  void                                                    *imgui_context;
  crude_gfx_asynchronous_loader                           *async_loader;
  crude_heap_allocator                                    *allocator;
  crude_stack_allocator                                   *temporary_allocator;
  bool                                                     imgui_pass_enalbed;
} crude_gfx_scene_renderer_creation;

typedef struct crude_gfx_scene_renderer_options
{
  struct 
  {
    char const                                            *gbuffer_albedo;
    char const                                            *gbuffer_normal;
    char const                                            *gbuffer_roughness_metalness;
    char const                                            *gbuffer_depth;
  } compose_pass;
  struct 
  {
    char const                                            *depth;
  } depth_pyramid_pass;
  struct 
  {
    char const                                            *hdr_pre_tonemapping;
    
    float32                                                gamma;
  } postprocessing_pass;
  struct
  {
    float32                                                max_steps;
    float32                                                max_distance;

    float32                                                stride_zcutoff;
    float32                                                stride;
    float32                                                z_thickness;
    char const                                            *depth_texture;
    char const                                            *normal_texture;
    char const                                            *pbr_without_ssr_texture;
    char const                                            *pbr_with_ssr_texture;
  } ssr_pass;
  struct 
  {
    crude_camera                                           camera;
    XMFLOAT4X4                                             camera_view_to_world;
    XMFLOAT3                                               background_color;
    float32                                                background_intensity;
    XMFLOAT3                                               ambient_color;
    float32                                                ambient_intensity;
  } scene;
  
#if CRUDE_DEVELOP
  struct 
  {
  uint32                                                   debug_mode;
  bool                                                     hide_collision;
  bool                                                     hide_debug_gltf;
  } debug;
#endif
  float32                                                  absolute_time;
} crude_gfx_scene_renderer_options;

typedef struct crude_gfx_scene_renderer_common_options
{
} crude_gfx_scene_renderer_common_options;

typedef struct crude_gfx_scene_renderer
{
  /***********************
   * Context 
   **********************/
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
  crude_gfx_memory_allocation                              scene_hga;

  crude_gfx_model_renderer_resources_instance             *model_renderer_resoruces_instances;
  uint32                                                   total_visible_meshes_instances_count;
  uint32                                                   total_meshes_instances_count;
  crude_gfx_memory_allocation                              meshes_instances_draws_hga;
  crude_gfx_memory_allocation                              mesh_task_indirect_commands_hga;
  crude_gfx_memory_allocation                              mesh_task_indirect_count_hga;
  crude_gfx_memory_allocation                              mesh_task_indirect_commands_culled_hga;
  
  uint32                                                   total_meshes_instances_buffer_capacity;

  /***********************
   * Common Debug CPU & GPU Data
   **********************/
  crude_gfx_memory_allocation                              debug_cubes_instances_hga;
  crude_gfx_memory_allocation                              debug_line_vertices_hga;
  crude_gfx_memory_allocation                              debug_commands_hga;
  
  /***********************
   * Common Lights & Shadows CPU & GPU Data
   **********************/
  crude_gfx_light_cpu                                     *lights;
  crude_gfx_memory_allocation                              lights_hga;
  crude_gfx_memory_allocation                              lights_bins_hga;
  crude_gfx_memory_allocation                              lights_tiles_hga;
  crude_gfx_memory_allocation                              lights_indices_hga;
  crude_gfx_memory_allocation                              lights_world_to_clip_hga;
  uint32                                                   total_visible_lights_count;

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
  //crude_gfx_pointlight_shadow_pass                         pointlight_shadow_pass;
  crude_gfx_debug_pass                                     debug_pass;
  crude_gfx_compose_pass                                   compose_pass;
  crude_gfx_postprocessing_pass                            postprocessing_pass;
  crude_gfx_transparent_pass                               transparent_pass;
  crude_gfx_light_lut_pass                                 light_lut_pass;
  crude_gfx_ssr_pass                                       ssr_pass;
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
  _In_ crude_ecs                                          *world,
  _In_ crude_entity                                        main_node
);

CRUDE_API void
crude_gfx_scene_renderer_rebuild_light_gpu_buffers
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

/* Utils */
#define CRUDE_GFX_PASS_TEXTURE_HANDLE( name )\
  crude_gfx_render_graph_builder_access_resource_by_name(\
    pass->scene_renderer->render_graph->builder,\
    pass->scene_renderer->options.name\
  )->resource_info.texture.handle

#define CRUDE_GFX_PASS_TEXTURE_INDEX( name ) CRUDE_GFX_PASS_TEXTURE_HANDLE( name ).index