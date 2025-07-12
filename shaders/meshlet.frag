#version 460
#extension GL_EXT_shader_16bit_storage: require
#extension GL_EXT_shader_8bit_storage: require
#extension GL_GOOGLE_include_directive: require
#extension GL_EXT_nonuniform_qualifier : require

layout(location = 0) out vec4 out_color;
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

struct crude_mesh_draw
{
  mat4                                                     model_to_world;

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
  uint                                                     padding1;
};

layout(set=1, binding=0, row_major, std140) uniform SceneConstant
{
  vec3                                                     camera_position;
  float                                                    padding1;
  mat4                                                     world_to_view;
  mat4                                                     view_to_clip;
  mat4                                                     clip_to_view;
  mat4                                                     view_to_world;
  vec4                                                     camera_frustum_planes_culling[ 6 ];
  vec3                                                     camera_position_culling;
  float                                                    padding2;
  mat4                                                     world_to_view_culling;
};

layout(set=1, binding=1) readonly buffer Meshlets
{
  crude_meshlet                                            meshlets[];
};

layout(set=1, binding=2) readonly buffer Vertices
{
  crude_vertex                                             vertices[];
};

layout(set=1, binding=3) readonly buffer TrianglesIndices
{
  uint8_t                                                  triangles_indices[];
};

layout(set=1, binding=4) readonly buffer VerticesIndices
{
  uint                                                     vertices_indices[];
};

layout(set=1, binding=5, row_major, std430) readonly buffer MeshDraws
{
  crude_mesh_draw                                          mesh_draws[];
};

layout(set=0, binding=10) uniform sampler2D global_textures[];
//layout(set=1, binding=10) uniform sampler3D global_textures_3d[];

layout(location=0) in vec2 in_texcoord0;
layout(location=1) in flat uint mesh_draw_index;

void main()
{
  crude_mesh_draw mesh_draw = mesh_draws[ mesh_draw_index ];
  vec4 albedo = texture( global_textures[ nonuniformEXT( mesh_draw.textures.x ) ], in_texcoord0 ) * mesh_draw.albedo_color_factor;
  out_color = vec4( albedo );
}