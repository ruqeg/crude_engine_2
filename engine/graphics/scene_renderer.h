#pragma once

#include <core/math.h>
#include <core/ecs.h>
#include <graphics/renderer.h>
#include <graphics/asynchronous_loader.h>
#include <graphics/render_graph.h>

#define CRUDE_GFX_MAX_RENDERER_SCENE_PATH_LEN             ( 512 )

typedef CRUDE_ALIGNED_STRUCT( 16 ) crude_gfx_camera
{
  crude_float3a                                            position;
  crude_float4x4a                                          world_to_view;
  crude_float4x4a                                          view_to_clip;
  crude_float4x4a                                          clip_to_view;
  crude_float4x4a                                          view_to_world;
} crude_gfx_camera;

typedef CRUDE_ALIGNED_STRUCT( 16 ) crude_gfx_per_frame
{
  crude_gfx_camera                                         camera;
} crude_gfx_per_frame;

typedef CRUDE_ALIGNED_STRUCT( 16 ) crude_gfx_mesh_draw_command
{
  uint32                                                   draw_id;
  VkDrawIndexedIndirectCommand                             indirect;
  VkDrawMeshTasksIndirectCommandEXT                        indirect_meshlet;
} crude_gfx_mesh_draw_command;

typedef CRUDE_ALIGNED_STRUCT( 16 ) crude_gfx_mesh_draw_counts
{
  uint32                                                   opaque_mesh_visible_count;
  uint32                                                   opaque_mesh_culled_count;
  uint32                                                   transparent_mesh_visible_count;
  uint32                                                   transparent_mesh_culled_count;

  uint32                                                   total_count;
  uint32                                                   depth_pyramid_texture_index;
  uint32                                                   late_flag;
  uint32                                                   meshlet_index_count;

  uint32                                                   dispatch_task_x;
  uint32                                                   dispatch_task_y;
  uint32                                                   dispatch_task_z;
  uint32                                                   pad001;
} crude_gfx_mesh_draw_counts;

typedef CRUDE_ALIGNED_STRUCT( 16 ) crude_gfx_meshlet
{
  crude_float3                                             center;
  float32                                                  radius;
  int8                                                     cone_axis[ 3 ];
  int8                                                     cone_cutoff;
  uint32                                                   vertices_offset;
  uint32                                                   triangles_offset;
  uint8                                                    vertices_count;
  uint8                                                    triangles_count;
  uint32                                                   mesh_index;
} crude_gfx_meshlet;

typedef CRUDE_ALIGNED_STRUCT( 16 ) crude_gfx_mesh_draw
{
  crude_uint4                                              textures;
  crude_float4                                             emissive;
  crude_float4                                             albedo_color_factor;
  crude_float4                                             metallic_roughness_occlusion_factor;

  uint32                                                   flags;
  float32                                                  alpha_cutoff;
  uint32                                                   vertices_offset;
  uint32                                                   mesh_index;

  uint32                                                   meshletes_offset;
  uint32                                                   meshletes_count;
  uint32                                                   meshletes_index_count;
  uint32                                                   padding1;
} crude_gfx_mesh_draw;

typedef struct crude_gfx_meshlet_vertex
{
  crude_float3a                                            position;
  uint8                                                    normal[ 4 ];
  uint8                                                    tangent[ 4 ];
  uint16                                                   texcoords[ 2 ];
  float32                                                  padding;
} crude_gfx_meshlet_vertex;

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
  uint32                                                   meshlet_technique_index;
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
  
  crude_gfx_meshlet                                       *meshlets;
  crude_gfx_meshlet_vertex                                *meshlets_vertices;
  uint32                                                  *meshlets_vertices_indices;
  uint8                                                   *meshlets_triangles_indices;
  
  crude_gfx_buffer_handle                                  scene_cb;

  crude_gfx_buffer_handle                                  meshlets_sb;
  crude_gfx_buffer_handle                                  meshlets_vertices_sb;
  crude_gfx_buffer_handle                                  meshlets_vertices_indices_sb;
  crude_gfx_buffer_handle                                  meshlets_triangles_indices_sb;
  crude_gfx_descriptor_set_handle                          mesh_shader_ds[ CRUDE_GFX_MAX_SWAPCHAIN_IMAGES ];
  
  crude_gfx_buffer_handle                                  mesh_task_indirect_commands_sb[ CRUDE_GFX_MAX_SWAPCHAIN_IMAGES ];
  crude_gfx_buffer_handle                                  mesh_task_indirect_count_sb[ CRUDE_GFX_MAX_SWAPCHAIN_IMAGES ];

  crude_allocator_container                                allocator_container;

  crude_gfx_scene_renderer_geometry_pass                   geometry_pass;

  bool                                                     use_meshlets;
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
  _In_ crude_entity                                        node,
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