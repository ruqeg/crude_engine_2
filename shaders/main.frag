#version 450

#extension GL_EXT_nonuniform_qualifier : enable

layout(binding = 0, row_major) uniform LocalConstants
{
  mat4 viewToClip;
};

layout(binding = 1, row_major) uniform Mesh
{
  mat4  modelToWorld;
  mat4  worldToModel;
  uvec4 textures;
  vec4  base_color_factor;
  vec4  metallic_roughness_occlusion_factor;
  float alpha_cutoff;
  uint  flags;
};

layout(set = 1, binding = 10) uniform sampler2D global_textures[];
layout(set = 1, binding = 10) uniform sampler3D global_textures_3d[];

layout(location = 0) out vec4 outColor;

void main()
{
  outColor = vec4( 1.0, 0.0, 0.0, 1.0 );
}
