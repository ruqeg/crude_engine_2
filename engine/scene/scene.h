#pragma once

#include <TaskScheduler_c.h>

#include <core/math.h>
#include <graphics/renderer.h>
#include <graphics/asynchronous_loader.h>
#include <graphics/render_graph.h>

/**
 *
 * Common Scene Structes
 * 
 */
typedef struct crude_gfx_mesh
{
  crude_gfx_renderer_material                             *material;
  crude_float3                                             scale;
  crude_float3                                             translation;
  crude_float4                                             rotation;
  crude_gfx_buffer_handle                                  index_buffer;
  crude_gfx_buffer_handle                                  position_buffer;
  crude_gfx_buffer_handle                                  tangent_buffer;
  crude_gfx_buffer_handle                                  normal_buffer;
  crude_gfx_buffer_handle                                  texcoord_buffer;
  crude_gfx_buffer_handle                                  material_buffer;
  uint32                                                   index_offset;
  uint32                                                   position_offset;
  uint32                                                   tangent_offset;
  uint32                                                   normal_offset;
  uint32                                                   texcoord_offset;
  uint32                                                   primitive_count;
  crude_float4                                             base_color_factor;
  crude_float3                                             metallic_roughness_occlusion_factor;
  float32                                                  alpha_cutoff;
  uint32                                                   flags;
  uint16                                                   albedo_texture_index;
  uint16                                                   roughness_texture_index;
  uint16                                                   normal_texture_index;
  uint16                                                   occlusion_texture_index;
} crude_gfx_mesh;

typedef struct crude_gfx_mesh_instance
{
  crude_gfx_mesh                                          *mesh;
  uint32                                                   material_pass_index;
} crude_gfx_mesh_instance;

CRUDE_API bool
crude_gfx_mesh_is_transparent
(
  _In_ crude_gfx_mesh                                     *mesh
);

/**
 *
 * GLTF Scene Constants
 * 
 */
#define CRUDE_MAX_GLTF_SCENE_PATH_LEN 512

/**
 *
 * GLTF Scene
 * 
 */
typedef struct crude_gltf_scene_creation
{
  crude_gfx_renderer                                      *renderer;
  char const                                              *path;
  crude_gfx_asynchronous_loader                           *async_loader;
  crude_allocator_container                                allocator;
  crude_stack_allocator                                   *temprorary_stack_allocator;
} crude_gltf_scene_creation;


/**
 *
 * Scene Geometry Pass
 * 
 */

typedef struct crude_gltf_scene crude_gltf_scene;

typedef struct crude_gfx_geometry_pass
{
  crude_gltf_scene                                        *scene;
  crude_gfx_mesh_instance                                 *mesh_instances;
  crude_gfx_renderer                                      *renderer;
} crude_gfx_geometry_pass;

CRUDE_API void
crude_gfx_geometry_pass_render
(
  _In_ crude_gfx_geometry_pass                            *pass,
  _In_ crude_gfx_cmd_buffer                               *gpu_commands
);

CRUDE_API void
crude_gfx_geometry_pass_prepare_draws
(
  _In_ crude_gfx_geometry_pass                            *pass,
  _In_ crude_gfx_render_graph                             *render_graph,
  _In_ crude_stack_allocator                              *temporary_allocator
);

typedef struct crude_gltf_scene
{
  crude_gfx_renderer                                      *renderer;
  crude_gfx_render_graph                                  *render_graph;
  crude_gfx_asynchronous_loader                           *async_loader;
  crude_gfx_renderer_sampler                              *samplers;
  crude_gfx_renderer_texture                              *images;
  crude_gfx_renderer_buffer                               *buffers;
  crude_gfx_mesh                                          *meshes;
  char const                                               path[ CRUDE_MAX_GLTF_SCENE_PATH_LEN ];

  crude_gfx_geometry_pass                                  geometry_pass;
} crude_gltf_scene;

/**
 *
 * GLTF Scene Function
 * 
 */
CRUDE_API void
crude_gltf_scene_load_from_file
(
  _In_ crude_gltf_scene                                   *scene,
  _In_ crude_gltf_scene_creation const                    *creation
);

CRUDE_API void
crude_gltf_scene_unload
(
  _In_ crude_gltf_scene                                   *scene
);

CRUDE_API void
crude_gfx_scene_prepare_draws
(
  _In_ crude_gltf_scene                                   *scene,
  _In_ crude_stack_allocator                              *temporary_allocator
);

CRUDE_API void
crude_gltf_scene_submit_draw_task
(
  _In_ crude_gltf_scene                                   *scene,
  _In_ enkiTaskScheduler                                  *task_sheduler,
  _In_ bool                                                use_secondary
);

CRUDE_API void
crude_register_render_passes
(
  _In_ crude_gltf_scene                                   *scene,
  _In_ crude_gfx_render_graph  *render_graph
);