
#ifdef CRUDE_VALIDATOR_LINTING
#extension GL_GOOGLE_include_directive : enable
#include "crude/platform.glsli"
#endif /* CRUDE_VALIDATOR_LINTING */

#extension GL_EXT_ray_tracing : enable

layout(location=0) rayPayloadInEXT vec4 payload;

void main()
{
  payload = vec4( 0.0, 1.0, 0.0, 1.0 );
}