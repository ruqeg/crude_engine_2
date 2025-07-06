#define CRUDE_GLOBAL_SET 0
#define CRUDE_MATERIAL_SET 1

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
