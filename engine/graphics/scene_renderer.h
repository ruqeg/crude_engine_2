#pragma once

#include <core/math.h>
#include <graphics/renderer.h>
#include <graphics/asynchronous_loader.h>
#include <graphics/render_graph.h>

#define CRUDE_GFX_MAX_RENDERER_SCENE_PATH_LEN             ( 512 )

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

typedef struct crude_gfx_scene_renderer crude_gfx_scene_renderer;

typedef struct crude_gfx_scene_renderer_geometry_pass
{
  crude_gfx_scene_renderer                                *scene;
  crude_gfx_mesh_instance                                 *mesh_instances;
} crude_gfx_scene_renderer_geometry_pass;

typedef struct crude_gfx_scene_renderer_creation
{
  crude_gfx_renderer                                      *renderer;
  crude_gfx_asynchronous_loader                           *async_loader;
  crude_allocator_container                                allocator_container;
  void                                                    *task_scheduler;
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
  crude_gfx_mesh                                          *meshes;

  crude_allocator_container                                allocator_container;

  crude_gfx_scene_renderer_geometry_pass                   geometry_pass;
} crude_gfx_scene_renderer;

/**
 *
 * Common Renderer Scene Structes
 * 
 */
CRUDE_API bool
crude_gfx_mesh_is_transparent
(
  _In_ crude_gfx_mesh                                     *mesh
);

/**
 *
 * Renderer Scene Geometry Pass
 * 
 */
CRUDE_API void
crude_gfx_scene_renderer_geometry_pass_render
(
  _In_ crude_gfx_scene_renderer_geometry_pass             *pass,
  _In_ crude_gfx_cmd_buffer                               *primary_cmd
);

CRUDE_API void
crude_gfx_scene_renderer_geometry_pass_prepare_draws
(
  _In_ crude_gfx_scene_renderer_geometry_pass             *pass,
  _In_ crude_gfx_render_graph                             *render_graph,
  _In_ crude_stack_allocator                              *temporary_allocator
);

CRUDE_API crude_gfx_render_graph_pass_container
crude_gfx_scene_renderer_geometry_pass_pack
(
  _In_ crude_gfx_scene_renderer_geometry_pass             *pass
);

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
crude_gfx_scene_renderer_prepare_draws
(
  _In_ crude_gfx_scene_renderer                           *scene_renderer,
  _In_ crude_stack_allocator                              *temporary_allocator
);

CRUDE_API void
crude_gfx_scene_renderer_submit_draw_task
(
  _In_ crude_gfx_scene_renderer                           *scene_renderer,
  _In_ bool                                                use_secondary
);

CRUDE_API void
crude_gfx_scene_renderer_register_render_passes
(
  _In_ crude_gfx_scene_renderer                           *scene_renderer,
  _In_ crude_gfx_render_graph                             *render_graph
);