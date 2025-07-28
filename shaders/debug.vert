
#ifdef CRUDE_VALIDATOR_LINTING
#include "crude/platform.glsli"
#include "crude/debug.glsli"
#include "crude/scene.glsli"
#endif /* CRUDE_VALIDATOR_LINTING */

layout(location=0) in vec3 in_position;
layout(location=1) in uint in_packed_color;

layout(location=0) out vec4 out_color;

void main()
{
  out_color = unpack_color_rgba( in_packed_color );
  gl_Position = vec4( in_position, 1.0 ) * camera.world_to_view * camera.view_to_clip;
}
