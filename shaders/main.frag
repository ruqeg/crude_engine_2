#version 460
#extension GL_EXT_nonuniform_qualifier : require

layout(set=1, binding=0, row_major, std140) uniform SceneConstant
{
  vec3                                                     camera_position;
  float                                                    padding1;
  mat4                                                     world_to_view;
  mat4                                                     view_to_clip;
  mat4                                                     clip_to_view;
  mat4                                                     view_to_world;
};

layout(set=1, binding=1, row_major) uniform MeshConstants
{
  mat4                                                     model_to_world;
  uvec4                                                    textures;
  vec4                                                     albedo_color_factor;
  vec4                                                     metallic_roughness_occlusion_factor;
  float                                                    alpha_cutoff;
  uint                                                     flags;
};

layout(set=0, binding=10) uniform sampler2D global_textures[];
//layout( set = 1, binding = 10 ) uniform sampler3D global_textures_3d[];

layout(location=0) in vec2 in_texcoord0;

layout(location=0) out vec4 out_color;

void main()
{
  vec4 albedo = texture( global_textures[ nonuniformEXT( textures.x ) ], in_texcoord0 ) * albedo_color_factor;
  // out_color = vec4( albedo );
}
