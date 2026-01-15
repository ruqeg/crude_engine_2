
#ifndef CRUDE_SCENE_GLSL
#define CRUDE_SCENE_GLSL

#ifndef __cplusplus

CRUDE_RBUFFER_REF_SCALAR( MeshPositionsRef )
{
  vec3                                                     data[];
};

CRUDE_RBUFFER_REF_SCALAR( MeshTexcoordsRef )
{
  float                                                    data[];
};

CRUDE_RBUFFER_REF_SCALAR( MeshIndices16Ref )
{
  uint16                                                   data[];
};

CRUDE_RBUFFER_REF_SCALAR( MeshIndices32Ref )
{
  uint32                                                   data[];
};

CRUDE_RBUFFER_REF_SCALAR( MeshNormalsRef )
{
  vec3                                                     data[];
};

CRUDE_RBUFFER_REF_SCALAR( MeshTangentsRef )
{
  vec4                                                     data[];
};

struct crude_mesh_draw_command
{
  uint                                                     draw_id;
  uint                                                     indirect_meshlet_group_count_x;
  uint                                                     indirect_meshlet_group_count_y;
  uint                                                     indirect_meshlet_group_count_z;
  uint                                                     indirect_mesh_vertex_count;
  uint                                                     indirect_mesh_instance_count;
  uint                                                     indirect_mesh_first_vertex;
  uint                                                     indirect_mesh_first_instance;
};

struct crude_mesh_draw
{
  uvec4                                                    textures;
  vec4                                                     emissive;
  vec4                                                     albedo_color_factor;
  vec4                                                     metallic_roughness_occlusion_factor;

  uint                                                     flags;
  float                                                    alpha_cutoff;
  uint                                                     vertices_offset;
  uint                                                     mesh_index;

  uint                                                     meshletes_offset;
  uint                                                     meshletes_count;
  uint                                                     meshletes_index_count;
  uint                                                     mesh_indices_count;

  MeshPositionsRef                                         positions;
  MeshTexcoordsRef                                         texcoords;

  uint64_t                                                 indices;
  MeshNormalsRef                                           normals;

  MeshTangentsRef                                          tangents;
  vec2                                                     padding2;
};

struct crude_mesh_instance_draw
{
  mat4                                                     mesh_to_world;
  mat4                                                     world_to_mesh;
  uint                                                     mesh_draw_index;
  vec3                                                     padding1;
};

struct crude_meshlet
{
  vec3                                                     center;
  float                                                    radius;
  int8_t                                                   cone_axis[ 3 ];
  int8_t                                                   cone_cutoff;
  uint                                                     vertices_offset;
  uint                                                     triangles_offset;
  uint8_t                                                  vertices_count;
  uint8_t                                                  triangles_count;
  uint                                                     mesh_index;
};

struct crude_vertex
{
  vec3                                                     position;
  float                                                    padding1;
  uint8_t                                                  nx, ny, nz, nw;
  uint8_t                                                  tx, ty, tz, tw;
  float16_t                                                tu, tv;
  float                                                    padding2;
};

struct crude_camera
{
  mat4                                                     world_to_clip;
  mat4                                                     world_to_view;
  mat4                                                     view_to_clip;
  mat4                                                     clip_to_view;
  mat4                                                     view_to_world;
  mat4                                                     clip_to_world;
  vec4                                                     frustum_planes_culling[ 6 ];
  vec3                                                     position;
  float                                                    padding1;
  float                                                    znear;
  float                                                    zfar;
  vec2                                                     padding2;
};

struct crude_scene
{
  crude_camera                                             camera;
  crude_camera                                             camera_previous;

  vec2                                                     resolution;
  uint                                                     flags;
  uint                                                     meshes_instances_count;

  uint                                                     active_lights_count;
  uint                                                     tiled_shadowmap_texture_index;
  vec2                                                     inv_shadow_map_size;

  vec3                                                     background_color;
  float                                                    background_intensity;

  vec3                                                     ambient_color;
  float                                                    ambient_intensity;

  uint                                                     indirect_light_texture_index;
  uint                                                     absolute_frame;
  float                                                    absolute_time;
  float                                                    resolution_ratio;
#if CRUDE_SHADER_DEVELOP
  uint                                                     debug_mode;
  uint                                                     debug_flags1;
  float                                                    debug_force_roughness;
  float                                                    debug_force_metalness;
#endif
};

CRUDE_RBUFFER_REF( SceneRef ) 
{
  crude_scene                                              data;
};

CRUDE_RBUFFER_REF( MeshDrawsRef )
{
  crude_mesh_draw                                          data[];
};

CRUDE_RBUFFER_REF( MeshInstancesDrawsRef )
{
  crude_mesh_instance_draw                                 data[];
};

CRUDE_RBUFFER_REF( MeshletsRef )
{
  crude_meshlet                                            data[];
};

CRUDE_RBUFFER_REF( VerticesRef )
{
  crude_vertex                                             data[];
};

CRUDE_RBUFFER_REF( TrianglesIndicesRef )
{
  uint8_t                                                  data[];
};

CRUDE_RBUFFER_REF( VerticesIndicesRef )
{
  uint                                                     data[];
};

CRUDE_RBUFFER_REF( MeshDrawCommandsRef )
{
  crude_mesh_draw_command                                  data[];
};

CRUDE_RBUFFER_REF( MeshBoundsRef )
{
  vec4                                                     data[];
};

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

#endif

#endif /* CRUDE_SCENE_GLSL */