#pragma once

#include <threads.h>

#include <engine/graphics/graphics_config.h>
#include <engine/graphics/asynchronous_loader.h>
#include <engine/graphics/scene_renderer_resources.h>
#include <engine/graphics/passes/imgui_pass.h>
#include <engine/graphics/passes/opaque_early_pass.h>
#include <engine/graphics/passes/opaque_late_pass.h>
#include <engine/graphics/passes/depth_pyramid_pass.h>
#include <engine/graphics/passes/culling_early_pass.h>
#include <engine/graphics/passes/culling_late_pass.h>
#include <engine/graphics/passes/debug_pass.h>
#include <engine/graphics/passes/compose_pass.h>
#include <engine/graphics/passes/pointlight_shadow_pass.h>
#include <engine/graphics/passes/postprocessing_pass.h>
#include <engine/graphics/passes/ray_tracing_solid_pass.h>
#include <engine/graphics/passes/indirect_light_pass.h>
#include <engine/graphics/passes/indirect_light_debug_pass.h>
#include <engine/graphics/passes/translucent_pass.h>
#include <engine/graphics/passes/light_lut_pass.h>
#include <engine/graphics/passes/ssr_pass.h>
#include <engine/graphics/model_renderer_resources_manager.h>

#if CRUDE_DEVELOP
#include <engine/physics/physics_shapes_manager.h>
#endif

typedef struct crude_gfx_scene_renderer_creation
{
  crude_gfx_model_renderer_resources_manager              *model_renderer_resources_manager;
  void                                                    *imgui_context;
  crude_gfx_asynchronous_loader                           *async_loader;
  crude_heap_allocator                                    *allocator;
  crude_stack_allocator                                   *temporary_allocator;
  bool                                                     imgui_pass_enalbed;
#if CRUDE_DEVELOP
  crude_physics_shapes_manager                            *physics_shapes_manager;
#endif
} crude_gfx_scene_renderer_creation;

typedef struct crude_gfx_scene_renderer_options
{
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
    float32                                                fade_start;
    float32                                                fade_end;

    float32                                                stride_zcutoff;
    float32                                                stride;
    float32                                                z_thickness;
    char const                                            *depth_texture;
    char const                                            *direct_radiance_texture;
    char const                                            *normal_texture;
    char const                                            *roughness_metalness_texture;
    char const                                            *ssr_hit_uv_depth_rdotv_texture;
    char const                                            *ssr_texture;
  } ssr_pass;
  struct
  {
    char const                                            *direct_radiance_texture;
    char const                                            *ssr_texture;
    char const                                            *output_texture;
    char const                                            *packed_roughness_metalness_texture;
  } compose_pass;
  
  struct 
  {
    crude_camera                                           camera;
    XMFLOAT4X4                                             camera_view_to_world;
    XMFLOAT3                                               background_color;
    float32                                                background_intensity;
    XMFLOAT3                                               ambient_color;
    float32                                                ambient_intensity;
  } scene;

  struct 
  {
    char const                                            *normal_texture;
    char const                                            *depth_texture;
   
    XMFLOAT3                                               probe_spacing;
    XMFLOAT3                                               probe_grid_position;
    float32                                                hysteresis;
    float32                                                self_shadow_bias;
    float32                                                infinite_bounces_multiplier;
    float32                                                max_probe_offset;
    uint32                                                 probe_debug_flags;
    float32                                                shadow_weight_power;
    int32                                                  probe_update_per_frame;
    uint32                                                 probe_count_x;
    uint32                                                 probe_count_y;
    uint32                                                 probe_count_z;
    int32                                                  probe_rays;
    int32                                                  offsets_calculations_count;
    bool                                                   use_half_resolution;
  } indirect_light;
  
#if CRUDE_DEVELOP
  struct 
  {
    uint32                                                 debug_mode;
    bool                                                   hide_collision;
    bool                                                   hide_debug_gltf;
    uint32                                                 flags1;
    float32                                                force_roughness;
    float32                                                force_metalness;
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
#if CRUDE_DEVELOP
  crude_physics_shapes_manager                            *physics_shapes_manager;
#endif
  
  /***********************
   * Base
   **********************/
  crude_gfx_scene_renderer_options                         options;
  bool                                                     imgui_pass_enalbed;

  crude_gfx_cmd_buffer                                    *primary_cmd;
  
  /***********************
   * Debug
   **********************/
#if CRUDE_DEVELOP
  crude_gfx_model_renderer_resources_instance              probe_model_renderer_resources_instance;
  crude_gfx_model_renderer_resources_instance              light_model_renderer_resources_instance;
  crude_gfx_model_renderer_resources_instance              camera_model_renderer_resources_instance;
  crude_gfx_model_renderer_resources_instance              capsule_model_renderer_resources_instance;
  crude_gfx_model_renderer_resources_instance              physics_box_collision_model_renderer_resources_instance;
  crude_gfx_model_renderer_resources_instance              audo_player_model_renderer_resources_instance;
  crude_gfx_model_renderer_resources_instance              audio_listener_model_renderer_resources_instance;
  crude_gfx_model_renderer_resources_instance              ray_model_renderer_resources_instance;
#endif

  /***********************
   * Common Mesh & Meshlets CPU & GPU Data
   **********************/
  crude_gfx_memory_allocation                              scene_hga;

  crude_gfx_memory_allocation                              ddgi_hga;
  
  crude_gfx_model_renderer_resources_instance             *model_renderer_resoruces_instances;
  crude_gfx_model_renderer_resources_instance             *prev_model_renderer_resoruces_instances;
  uint32                                                   total_visible_meshes_instances_count;

  crude_gfx_memory_allocation                              meshes_instances_draws_hga;
  crude_gfx_memory_allocation                              mesh_task_indirect_commands_hga;
  crude_gfx_memory_allocation                              mesh_task_indirect_count_hga;
  crude_gfx_memory_allocation                              mesh_task_indirect_commands_culled_hga;

  crude_gfx_memory_allocation                              joint_matrices_hga;
  
  uint32                                                   total_joints_matrices_count;
  uint32                                                   total_joints_matrices_buffer_capacity;
  
  uint32                                                   total_meshes_instances_count;
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
  crude_gfx_culled_light_cpu                              *culled_lights;
  crude_gfx_memory_allocation                              lights_hga;
  crude_gfx_memory_allocation                              lights_world_to_texture_hga;

  /***********************
   * Ray Tracing CPU & GPU Data
   **********************/
#if CRUDE_GFX_RAY_TRACING_ENABLED
  VkAccelerationStructureKHR                               vk_tlas;
  crude_gfx_memory_allocation                              tlas_hga;
  crude_gfx_memory_allocation                              tlas_instances_hga;
  crude_gfx_memory_allocation                              tlas_scratch_hga;
  crude_gfx_descriptor_set_handle                          acceleration_stucture_ds;
  crude_gfx_descriptor_set_layout_handle                   acceleration_stucture_dsl;
#endif /* CRUDE_GFX_RAY_TRACING_ENABLED */

  /***********************
   * Render Graph Passes 
   **********************/
  crude_gfx_culling_early_pass                             culling_early_pass;
  crude_gfx_culling_late_pass                              culling_late_pass;
  crude_gfx_opaque_early_pass                              opaque_early_pass;
  crude_gfx_opaque_late_pass                               opaque_late_pass;
  crude_gfx_imgui_pass                                     imgui_pass;
  crude_gfx_depth_pyramid_pass                             depth_pyramid_pass;
  crude_gfx_pointlight_shadow_pass                         pointlight_shadow_pass;
  crude_gfx_debug_pass                                     debug_pass;
  crude_gfx_compose_pass                                   compose_pass;
  crude_gfx_postprocessing_pass                            postprocessing_pass;
  crude_gfx_translucent_pass                               translucent_pass;
  crude_gfx_light_lut_pass                                 light_lut_pass;
  crude_gfx_ssr_pass                                       ssr_pass;
#if CRUDE_GFX_RAY_TRACING_SOLID_DEBUG_ENABLED
  crude_gfx_ray_tracing_solid_pass                         ray_tracing_solid_pass;
#endif
#if CRUDE_GFX_RAY_TRACING_DDGI_ENABLED
  crude_gfx_indirect_light_pass                            indirect_light_pass;
  crude_gfx_indirect_light_debug_pass                      indirect_light_debug_pass;
#endif
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
crude_gfx_scene_renderer_start_frame
(
  _In_ crude_gfx_scene_renderer                           *scene_renderer
);

CRUDE_API void
crude_gfx_scene_renderer_update_dynamic_buffers
(
  _In_ crude_gfx_scene_renderer                           *scene_renderer
);

CRUDE_API void
crude_gfx_scene_renderer_render
(
  _In_ crude_gfx_scene_renderer                           *scene_renderer
);

CRUDE_API void
crude_gfx_scene_renderer_queue
(
  _In_ crude_gfx_scene_renderer                           *scene_renderer
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