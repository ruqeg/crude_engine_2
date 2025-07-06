#version 460

layout(location=0) in vec3 in_position;
layout(location=1) in vec4 in_tangent;
layout(location=2) in vec3 in_normal;
layout(location=3) in vec2 in_texcoord0;

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

layout(location=0) out vec2 out_texcoord0;

void main()
{
  out_texcoord0 = in_texcoord0;
  gl_Position = vec4( in_position, 1.0 ) * model_to_world * world_to_view * view_to_clip;
}
