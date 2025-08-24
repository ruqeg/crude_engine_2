
#ifdef CRUDE_VALIDATOR_LINTING
#extension GL_GOOGLE_include_directive : enable
#include "crude/platform.glsli"
#endif /* CRUDE_VALIDATOR_LINTING */

layout(location=0) in vec3 in_position;
layout(location=1) in uint in_packed_color;

layout(location=0) out vec4 out_color;

void main()
{
  out_color = crude_unpack_color_rgba( in_packed_color );
  gl_Position = vec4( in_position, 1.0 );
}