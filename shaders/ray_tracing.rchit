

#ifdef CRUDE_VALIDATOR_LINTING
#extension GL_GOOGLE_include_directive : enable
#include "crude/platform.glsli"
#endif /* CRUDE_VALIDATOR_LINTING */

#extension GL_EXT_ray_tracing : enable

layout(location=0) rayPayloadInEXT vec4 payload;
hitAttributeEXT vec2 attribs;

void main()
{
  payload = vec4( 1.0f - attribs.x, attribs.y, attribs.x, 1.0 );
}