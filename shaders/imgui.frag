
#ifdef CRUDE_VALIDATOR_LINTING
#extension GL_GOOGLE_include_directive : enable
#include "crude/platform.glsli"
#endif /* CRUDE_VALIDATOR_LINTING */

layout(location = 0) in vec2 in_uv;
layout(location = 1) in vec4 in_color;
layout(location = 2) flat in uint in_texture_id;

layout(location = 0) out vec4 out_color;

void main()
{
  out_color = in_color * texture( global_textures[ nonuniformEXT( in_texture_id ) ], in_uv.st );
}