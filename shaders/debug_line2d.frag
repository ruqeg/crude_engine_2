
#ifdef CRUDE_VALIDATOR_LINTING
#extension GL_GOOGLE_include_directive : enable
#include "crude/platform.glsli"
#endif /* CRUDE_VALIDATOR_LINTING */

layout(location=0) in vec4 in_color;

layout(location=0) out vec4 out_color;

void main()
{
  out_color = in_color;
}