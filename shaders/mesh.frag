
#ifdef CRUDE_VALIDATOR_LINTING
#extension GL_GOOGLE_include_directive: require
#include "crude/platform.glsli"
#include "crude/scene.glsli"
#endif /* CRUDE_VALIDATOR_LINTING */

layout(set=CRUDE_MATERIAL_SET, binding=1, row_major) uniform MeshConstants
{
  mat4                                                     model_to_world;
  uvec4                                                    textures;
  vec4                                                     albedo_color_factor;
  vec4                                                     metallic_roughness_occlusion_factor;
  float                                                    alpha_cutoff;
  uint                                                     flags;
};

layout(location=0) in vec2 in_texcoord0;

layout(location=0) out vec4 out_color;

void main()
{
  vec4 albedo = texture( global_textures[ nonuniformEXT( textures.x ) ], in_texcoord0 ) * albedo_color_factor;
  out_color = vec4( albedo );
}
