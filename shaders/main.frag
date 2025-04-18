#version 450
#extension GL_EXT_nonuniform_qualifier : require

layout( binding = 0, row_major ) uniform FrameConstants
{
  mat4 worldToView;
  mat4 viewToClip;
};

layout( binding = 1, row_major ) uniform MeshConstants
{
  mat4  modelToWorld;
  uvec4 textures;
  vec4  albedo_color_factor;
  vec4  metallic_roughness_occlusion_factor;
  float alpha_cutoff;
  uint  flags;
};

layout( set = 1, binding = 10 ) uniform sampler2D global_textures[];
//layout( set = 1, binding = 10 ) uniform sampler3D global_textures_3d[];

layout(location = 0) in vec2 inTexcoord0;

layout(location = 0) out vec4 outColor;

void main()
{
  vec4 albedo = texture(global_textures[nonuniformEXT(textures.x)], inTexcoord0) * albedo_color_factor;
  outColor = vec4( albedo );
}
