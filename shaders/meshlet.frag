#version 450
#extension GL_EXT_nonuniform_qualifier : require

layout(location = 0) out vec4 out_color;

//struct crude_mesh_draw
//{
//  uvec4                                                    textures;
//  vec4                                                     emissive;
//  vec4                                                     albedo_color_factor;
//  vec4                                                     metallic_roughness_occlusion_factor;
//
//  uint                                                     flags;
//  float                                                    alpha_cutoff;
//  uint                                                     vertices_offset;
//  uint                                                     mesh_index;
//
//  uint                                                     meshletes_offset;
//  uint                                                     meshletes_count;
//  uint                                                     meshletes_index_count;
//  uint                                                     padding1;
//};
//
//layout(std430, set=1, binding=0) readonly buffer MeshDraws
//{
//  crude_mesh_draw                                          mesh_draws[];
//};
layout(set=0, binding=10) uniform sampler2D global_textures[];
//layout(set=1, binding=10) uniform sampler3D global_textures_3d[];

layout(location = 0) in vec2 in_texcoord0;
layout(location = 1) in flat uint mesh_draw_index;

void main()
{
  //crude_mesh_draw mesh_draw = mesh_draws[ mesh_draw_index ];
  vec4 albedo = texture( global_textures[ nonuniformEXT( 0/*mesh_draw.textures.x*/ ) ], in_texcoord0 )/* * mesh_draw.albedo_color_factor*/;
  out_color = vec4( albedo );
}