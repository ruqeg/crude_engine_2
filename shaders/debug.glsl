
#ifdef CRUDE_VALIDATOR_LINTING
#extension GL_GOOGLE_include_directive : enable
#define DEBUG_LINE3D
#define CRUDE_STAGE_FRAGMENT

#include "crude/platform.glsli"
#include "crude/scene.glsli"
#endif /* CRUDE_VALIDATOR_LINTING */

#if defined( DEBUG_LINE2D ) && defined( CRUDE_STAGE_VERTEX )
layout(location=0) in vec3 in_position;
layout(location=1) in uint in_packed_color;

layout(location=0) out vec4 out_color;

void main()
{
  out_color = crude_unpack_color_rgba( in_packed_color );
  gl_Position = vec4( in_position, 1.0 );
}
#endif /* DEBUG_LINE2D && CRUDE_STAGE_VERTEX */

#if defined( DEBUG_LINE3D ) && defined( CRUDE_STAGE_VERTEX )
layout(location=0) in vec3 in_position;
layout(location=1) in uint in_packed_color;

layout(location=0) out vec4 out_color;

CRUDE_UNIFORM( SceneConstant, 0 ) 
{
  crude_scene                                              scene;
};

void main()
{
  out_color = crude_unpack_color_rgba( in_packed_color );
  gl_Position = vec4( in_position, 1.0 ) * scene.camera.world_to_view * scene.camera.view_to_clip;
}
#endif /* DEBUG_LINE3D && CRUDE_STAGE_VERTEX */

#if ( defined( DEBUG_LINE2D ) || defined( DEBUG_LINE3D ) ) && defined( CRUDE_STAGE_FRAGMENT )
layout(location=0) in vec4 in_color;

layout(location=0) out vec4 out_color;

void main()
{
  out_color = in_color;
}
#endif /* ( DEBUG_LINE2D || DEBUG_LINE3D ) && CRUDE_STAGE_FRAGMENT */