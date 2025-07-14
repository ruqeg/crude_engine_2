
#ifdef CRUDE_VALIDATOR_LINTING
#include "crude/platform.glsli"
#include "crude/scene.glsli"
#endif /* CRUDE_VALIDATOR_LINTING */

layout(location=0) in vec3 in_position;
layout(location=1) in vec4 in_tangent;
layout(location=2) in vec3 in_normal;
layout(location=3) in vec2 in_texcoord0;

layout(set=CRUDE_MATERIAL_SET, binding=1, row_major) uniform MeshConstants
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
  gl_Position = vec4( in_position, 1.0 ) * model_to_world * camera.world_to_view * camera.view_to_clip;
}
