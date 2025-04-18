#version 450

layout( location = 0 ) in vec3 position;
layout( location = 1 ) in vec4 tangent;
layout( location = 2 ) in vec3 normal;
layout( location = 3 ) in vec2 texcoord0;

layout( binding = 0, row_major ) uniform FrameConstants
{
  mat4 worldToView;
  mat4 viewToClip;
};

layout( binding = 1, row_major ) uniform MeshConstants
{
  mat4  modelToWorld;
  uvec4 textures;
  vec4  base_color_factor;
  vec4  metallic_roughness_occlusion_factor;
  float alpha_cutoff;
  uint  flags;
};

layout(location = 0) out vec2 outTexcoord0;

void main()
{
  gl_Position = vec4( position, 1.0 ) * modelToWorld * worldToView * viewToClip;
}
