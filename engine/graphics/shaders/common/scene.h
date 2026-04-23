
#ifndef CRUDE_SCENE_GLSL
#define CRUDE_SCENE_GLSL

#ifdef __cplusplus
#include <engine/graphics/shaders/common/platform.h>
#endif

#define CRUDE_GFX_MESH_INSTANCE_DRAW_FLAG_CAST_SHADOW      ( 1 << 0 )
#define CRUDE_GFX_MESH_INSTANCE_DRAW_FLAG_ANIMATED         ( 1 << 1 )

CRUDE_SHADER_RBUFFER_REF_ARRAY_SCALAR( MeshPositionsRef, XMFLOAT3 );
CRUDE_SHADER_RBUFFER_REF_ARRAY_SCALAR( MeshTexcoordsRef, XMFLOAT2 );
CRUDE_SHADER_RBUFFER_REF_ARRAY_SCALAR( MeshIndicesRef, uint32 );
CRUDE_SHADER_RBUFFER_REF_ARRAY_SCALAR( MeshNormalsRef, XMFLOAT3 );
CRUDE_SHADER_RBUFFER_REF_ARRAY_SCALAR( MeshTangentsRef, XMFLOAT4 );

CRUDE_SHADER_STRUCT( crude_gfx_meshlet )
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
};

CRUDE_SHADER_STRUCT( crude_gfx_mesh_draw )
{
  XMUINT4                                                  textures;
  XMFLOAT4                                                 emissive;
  XMFLOAT4                                                 albedo_color_factor;
  XMFLOAT3A                                                metallic_roughness_occlusion_factor;

  uint32                                                   flags;
  float32                                                  alpha_cutoff;
  uint32                                                   vertices_offset;
  uint32                                                   mesh_index;

  uint32                                                   meshletes_offset;
  uint32                                                   meshletes_count;
  uint32                                                   meshletes_index_count;
  uint32                                                   mesh_indices_count;
  
  MeshIndicesRef                                           index_buffer;
  XMFLOAT2                                                 padding;
};

CRUDE_SHADER_STRUCT( crude_gfx_mesh_draw_command )
{
#if __cplusplus
  uint32                                                   draw_id;
  VkDrawMeshTasksIndirectCommandEXT                        indirect_meshlet;
  VkDrawIndirectCommand                                    indirect_mesh;
#else
  uint32                                                   draw_id;
  uint32                                                   indirect_meshlet_group_count_x;
  uint32                                                   indirect_meshlet_group_count_y;
  uint32                                                   indirect_meshlet_group_count_z;
  uint32                                                   indirect_mesh_vertex_count;
  uint32                                                   indirect_mesh_instance_count;
  uint32                                                   indirect_mesh_first_vertex;
  uint32                                                   indirect_mesh_first_instance;
#endif
};

CRUDE_SHADER_STRUCT( crude_gfx_mesh_instance_draw )
{
  XMFLOAT4X4                                               mesh_to_world;
  XMFLOAT4X4                                               world_to_mesh;
  XMFLOAT4                                                 bounding_sphere;
  uint32                                                   mesh_draw_index;
  uint32                                                   joints_matrices_offset;
  uint32                                                   flags;
  float32                                                  padding;
};

#define crude_gfx_vertex_position XMFLOAT3

CRUDE_SHADER_STRUCT( crude_gfx_vertex )
{
#if __cplusplus
  uint8                                                    normal[ 4 ];
  uint8                                                    tangent[ 4 ];
  uint16                                                   texcoords[ 2 ];
#else
  uint8_t                                                  nx, ny, nz, nw;
  uint8_t                                                  tx, ty, tz, tw;
  float16_t                                                tu, tv;
#endif
  float32                                                  padding2;
};

CRUDE_SHADER_STRUCT( crude_gfx_vertex_joint )
{
  XMFLOAT4                                                 joint_indices;
  XMFLOAT4                                                 joint_weights;
};

CRUDE_SHADER_STRUCT( crude_gfx_camera )
{
  XMFLOAT4X4                                               world_to_clip;
  XMFLOAT4X4                                               world_to_view;
  XMFLOAT4X4                                               view_to_clip;
  XMFLOAT4X4                                               clip_to_view;
  XMFLOAT4X4                                               view_to_world;
  XMFLOAT4X4                                               clip_to_world;
  XMFLOAT4                                                 frustum_planes_culling[ 6 ];
  XMFLOAT3                                                 position;
  float32                                                  padding1;
  float32                                                  znear;
  float32                                                  zfar;
  XMFLOAT2                                                 padding;
};

CRUDE_SHADER_STRUCT( crude_gfx_scene )
{
  crude_gfx_camera                                         camera;
  crude_gfx_camera                                         camera_previous;
  XMFLOAT2                                                 resolution;
  uint32                                                   flags;
  uint32                                                   meshes_instances_count;
  uint32                                                   active_lights_count;
  uint32                                                   tiled_shadowmap_texture_index;
  XMFLOAT2                                                 inv_shadow_map_size;
  XMFLOAT3                                                 background_color;
  float32                                                  background_intensity;
  XMFLOAT3                                                 ambient_color;
  float32                                                  ambient_intensity;
  uint32                                                   indirect_light_texture_index;
  uint32                                                   absolute_frame;
  float32                                                  absolute_time;
  float32                                                  resolution_ratio;
#if CRUDE_DEVELOP
  uint32                                                   debug_mode;
  uint32                                                   debug_flags1;
  float32                                                  debug_force_roughness;
  float32                                                  debug_force_metalness;
#endif
};

CRUDE_SHADER_STRUCT( crude_gfx_mesh_draw_count )
{
  uint32                                                   opaque_mesh_visible_early_count;
  uint32                                                   opaque_mesh_visible_late_count;
  uint32                                                   opaque_mesh_culled_count;
  uint32                                                   transparent_mesh_visible_count;

  uint32                                                   transparent_mesh_culled_count;
  uint32                                                   total_mesh_count;
  uint32                                                   depth_pyramid_texture_index;
  uint32                                                   meshlet_index_count;

  uint32                                                   dispatch_task_x;
  uint32                                                   dispatch_task_y;
  uint32                                                   dispatch_task_z;
  uint32                                                   meshlet_instances_count;
};

CRUDE_SHADER_RBUFFER_REF( MeshDrawCountRef, crude_gfx_mesh_draw_count );
CRUDE_SHADER_RBUFFER_REF( SceneRef, crude_gfx_scene );
CRUDE_SHADER_RBUFFER_REF_ARRAY( MeshDrawCommandsRef, crude_gfx_mesh_draw_command );
CRUDE_SHADER_RBUFFER_REF_ARRAY( MeshInstancesDrawsRef, crude_gfx_mesh_instance_draw );
CRUDE_SHADER_RBUFFER_REF_ARRAY( VerticesRef, crude_gfx_vertex );
CRUDE_SHADER_RBUFFER_REF_ARRAY( VerticesJointsRef, crude_gfx_vertex_joint );
CRUDE_SHADER_RBUFFER_REF_ARRAY_SCALAR( VerticesPositionsRef, crude_gfx_vertex_position );
CRUDE_SHADER_RBUFFER_REF_ARRAY( MeshDrawsRef, crude_gfx_mesh_draw );
CRUDE_SHADER_RBUFFER_REF_ARRAY( MeshletsRef, crude_gfx_meshlet );
CRUDE_SHADER_RBUFFER_REF_ARRAY( TrianglesIndicesRef, uint8 );
CRUDE_SHADER_RBUFFER_REF_ARRAY( VerticesIndicesRef, uint32 );
CRUDE_SHADER_RBUFFER_REF_ARRAY( JointMatricesRef, XMFLOAT4X4 );

#ifndef __cplusplus

mat4
crude_calculate_mesh_to_skin
(
  in uint32                                                vertex_index,
  in VerticesJointsRef                                     vertices_joints,
  in JointMatricesRef                                      joint_matrices,
  in crude_gfx_mesh_instance_draw                          mesh_instance_draw
)
{
  return
    vertices_joints.data[ vertex_index ].joint_weights.x * joint_matrices.data[ mesh_instance_draw.joints_matrices_offset + int( vertices_joints.data[ vertex_index ].joint_indices.x ) ] +
    vertices_joints.data[ vertex_index ].joint_weights.y * joint_matrices.data[ mesh_instance_draw.joints_matrices_offset + int( vertices_joints.data[ vertex_index ].joint_indices.y ) ] +
    vertices_joints.data[ vertex_index ].joint_weights.z * joint_matrices.data[ mesh_instance_draw.joints_matrices_offset + int( vertices_joints.data[ vertex_index ].joint_indices.z ) ] +
    vertices_joints.data[ vertex_index ].joint_weights.w * joint_matrices.data[ mesh_instance_draw.joints_matrices_offset + int( vertices_joints.data[ vertex_index ].joint_indices.w ) ];
}

#if CRUDE_DEVELOP

bool
crude_scene_force_roughness
(
  in SceneRef                                              scene_ref
)
{
  return ( scene_ref.data.debug_flags1 & ( 1 << 0 ) ) != 0;
}

bool
crude_scene_force_metalness
(
  in SceneRef                                              scene_ref
)
{
  return ( scene_ref.data.debug_flags1 & ( 1 << 1 ) ) != 0;
}

#endif /* CRUDE_DEVELOP */

#endif /* __cplusplus */

#endif /* CRUDE_SCENE_GLSL */