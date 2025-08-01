#pragma once

#include <core/ecs.h>
#include <scene/scene_components.h>
#include <graphics/renderer_resources.h>

#define CRUDE_GFX_MAX_RENDERER_SCENE_PATH_LEN              ( 512 )
#define CRUDE_GFX_DEBUG_LINE_2D_OFFSET                     ( 1000 )
#define CRUDE_GFX_MAX_DEBUG_LINES                          ( 640000 )

typedef CRUDE_ALIGNED_STRUCT( 16 ) crude_gfx_debug_line_vertex_gpu
{
  XMFLOAT3                                                 position;
  uint32                                                   color;
} crude_gfx_debug_line_vertex_gpu;

typedef CRUDE_ALIGNED_STRUCT( 16 ) crude_gfx_debug_draw_command_gpu
{
  VkDrawIndirectCommand                                    draw_indirect_3dline;
  VkDrawIndirectCommand                                    draw_indirect_2dline;
} crude_gfx_debug_draw_command_gpu;

typedef CRUDE_ALIGNED_STRUCT( 16 ) crude_gfx_camera_gpu
{
  XMFLOAT4X4A                                              world_to_clip;
  XMFLOAT4X4A                                              world_to_view;
  XMFLOAT4X4A                                              view_to_clip;
  XMFLOAT4X4A                                              clip_to_view;
  XMFLOAT4X4A                                              view_to_world;
  XMFLOAT4A                                                frustum_planes_culling[ 6 ];
  XMFLOAT3A                                                position;
  float32                                                  znear;
  float32                                                  zfar;
  XMFLOAT2                                                 padding;
} crude_gfx_camera_gpu;

typedef CRUDE_ALIGNED_STRUCT( 16 ) crude_gfx_scene_constant_gpu
{
  crude_gfx_camera_gpu                                     camera;
  crude_gfx_camera_gpu                                     camera_debug;
  crude_gfx_camera_gpu                                     camera_previous;
  crude_gfx_camera_gpu                                     camera_debug_previous;
  uint32                                                   flags;
} crude_gfx_scene_constant_gpu;

typedef CRUDE_ALIGNED_STRUCT( 16 ) crude_gfx_mesh_draw_command_gpu
{
  uint32                                                   draw_id;
  VkDrawMeshTasksIndirectCommandEXT                        indirect_meshlet;
} crude_gfx_mesh_draw_command_gpu;

typedef CRUDE_ALIGNED_STRUCT( 16 ) crude_gfx_mesh_draw_counts_gpu
{
  uint32                                                   opaque_mesh_visible_count;
  uint32                                                   opaque_mesh_culled_count;
  uint32                                                   transparent_mesh_visible_count;
  uint32                                                   transparent_mesh_culled_count;

  uint32                                                   total_count;
  uint32                                                   depth_pyramid_texture_index;
  uint32                                                   occlusion_culling_late_flag;
  uint32                                                   meshlet_index_count;

  uint32                                                   dispatch_task_x;
  uint32                                                   dispatch_task_y;
  uint32                                                   dispatch_task_z;
  uint32                                                   pad001;
} crude_gfx_mesh_draw_counts_gpu;

typedef CRUDE_ALIGNED_STRUCT( 16 ) crude_gfx_meshlet_gpu
{
  XMFLOAT3                                                 center;
  float32                                                  radius;
  int8                                                     cone_axis[ 3 ];
  int8                                                     cone_cutoff;
  uint32                                                   vertices_offset;
  uint32                                                   triangles_offset;
  uint8                                                    vertices_count;
  uint8                                                    triangles_count;
  uint32                                                   mesh_index;
} crude_gfx_meshlet_gpu;

typedef CRUDE_ALIGNED_STRUCT( 16 ) crude_gfx_mesh_draw_gpu
{
  XMUINT4                                                  textures;
  XMFLOAT4                                                 emissive;
  XMFLOAT4                                                 albedo_color_factor;
  XMFLOAT4                                                 metallic_roughness_occlusion_factor;

  uint32                                                   flags;
  float32                                                  alpha_cutoff;
  uint32                                                   vertices_offset;
  uint32                                                   mesh_index;

  uint32                                                   meshletes_offset;
  uint32                                                   meshletes_count;
  uint32                                                   meshletes_index_count;
  uint32                                                   padding1;
} crude_gfx_mesh_draw_gpu;

typedef CRUDE_ALIGNED_STRUCT( 16 ) crude_gfx_mesh_instance_draw_gpu
{
  XMFLOAT4X4                                               model_to_world;
  uint32                                                   mesh_draw_index;
  XMFLOAT3A                                                padding;
} crude_gfx_mesh_darw_gpu;

typedef struct crude_gfx_meshlet_vertex_gpu
{
  XMFLOAT3A                                                position;
  uint8                                                    normal[ 4 ];
  uint8                                                    tangent[ 4 ];
  uint16                                                   texcoords[ 2 ];
  float32                                                  padding;
} crude_gfx_meshlet_vertex_gpu;

typedef struct crude_gfx_mesh_cpu
{
  XMFLOAT4                                                 bounding_sphere;
  crude_gfx_buffer_handle                                  index_buffer;
  crude_gfx_buffer_handle                                  position_buffer;
  crude_gfx_buffer_handle                                  tangent_buffer;
  crude_gfx_buffer_handle                                  normal_buffer;
  crude_gfx_buffer_handle                                  texcoord_buffer;
  uint32                                                   index_offset;
  uint32                                                   position_offset;
  uint32                                                   tangent_offset;
  uint32                                                   normal_offset;
  uint32                                                   texcoord_offset;
  uint32                                                   primitive_count;
  XMFLOAT4                                                 albedo_color_factor;
  XMFLOAT3                                                 metallic_roughness_occlusion_factor;
  float32                                                  alpha_cutoff;
  uint32                                                   flags;
  crude_gfx_texture_handle                                 albedo_texture_handle;
  crude_gfx_texture_handle                                 roughness_texture_handle;
  crude_gfx_texture_handle                                 normal_texture_handle;
  crude_gfx_texture_handle                                 occlusion_texture_handle;
  uint32                                                   gpu_mesh_index;
  uint32                                                   meshlets_offset;
  uint32                                                   meshlets_count;
} crude_gfx_mesh_cpu;

typedef struct crude_gfx_mesh_instance_cpu
{
  crude_gfx_mesh_cpu                                      *mesh;
  crude_entity                                             node;
  uint32                                                   material_pass_index;
} crude_gfx_mesh_instance_cpu;

CRUDE_API bool
crude_gfx_mesh_is_transparent
(
  _In_ crude_gfx_mesh_cpu                                 *mesh
);

CRUDE_API void
crude_gfx_camera_to_camera_gpu
(
  _In_ crude_entity                                        camera_node,
  _Out_ crude_gfx_camera_gpu                              *camera_gpu
);

CRUDE_API void
crude_gfx_mesh_cpu_to_mesh_draw_gpu
(
  _In_ crude_gfx_mesh_cpu const                           *mesh,
  _Out_ crude_gfx_mesh_draw_gpu                           *mesh_draw_gpu
);