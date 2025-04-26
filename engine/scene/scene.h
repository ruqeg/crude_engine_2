#pragma once

#include <TaskScheduler_c.h>

#include <core/math.h>
#include <graphics/renderer.h>
#include <graphics/asynchronous_loader.h>

/**
 *
 * Common Scene Structes
 * 
 */
typedef struct crude_mesh_draw
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
} crude_mesh_draw;

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
} crude_gltf_scene_creation;

typedef struct crude_gltf_scene
{
  crude_gfx_renderer                                      *renderer;
  crude_gfx_renderer_sampler                              *samplers;
  crude_gfx_renderer_texture                              *images;
  crude_gfx_renderer_buffer                               *buffers;
  crude_mesh_draw                                         *mesh_draws;
  char const                                               path[ CRUDE_MAX_GLTF_SCENE_PATH_LEN ];
  
  crude_gfx_asynchronous_loader                           *async_loader;

  crude_gfx_renderer_program                              *program;
  crude_gfx_renderer_material                             *material;
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
crude_gltf_scene_submit_draw_task
(
  _In_ crude_gltf_scene                                   *scene,
  _In_ enkiTaskScheduler                                  *draw_task_sheduler
);