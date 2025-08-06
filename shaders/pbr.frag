
#ifdef CRUDE_VALIDATOR_LINTING
#include "crude/platform.glsli"
#include "crude/debug.glsli"
#include "crude/scene.glsli"
#include "crude/light.glsli"
#endif /* CRUDE_VALIDATOR_LINTING */

layout(location = 0) out vec4 out_color;

layout(location=0) in vec2 in_texcoord;

layout(set=CRUDE_MATERIAL_SET, binding=10, row_major) uniform LightingConstants
{
  uvec4                                                    textures;
};

void main()
{
  out_color = texture( global_textures[ nonuniformEXT( textures.x ) ], in_texcoord.st );
}