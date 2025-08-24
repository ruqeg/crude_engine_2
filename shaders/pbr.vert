
#ifdef CRUDE_VALIDATOR_LINTING
#extension GL_GOOGLE_include_directive : enable
#include "crude/platform.glsli"
#include "crude/debug.glsli"
#include "crude/scene.glsli"
#include "crude/light.glsli"
#endif /* CRUDE_VALIDATOR_LINTING */

layout(location=0) out vec2 out_texcoord;

const vec4 fullscreen_vertices[ 3 ] =
{
  vec4( -1.0f, -1.0f, 1.0f, 1.0f ),
  vec4( -1.0f,  3.0f, 1.0f, 1.0f ),
  vec4(  3.0f, -1.0f, 1.0f, 1.0f )
};

const vec2 fullscreen_texcoord[ 3 ] =
{
  vec2( 0.0f, 1.0f ),
  vec2( 0.0f, -1.0f ),
  vec2( 2.0f, 1.0f )
};

void main()
{
  if ( gl_VertexIndex == 0 )
  {
    crude_debug_draw_box( lights[ 0 ].world_position - lights[ 0 ].radius, lights[ 0 ].world_position + lights[ 0 ].radius, vec4( 1, 0, 0, 1 ) );
  }
  out_texcoord = fullscreen_texcoord[ gl_VertexIndex ];
  gl_Position = fullscreen_vertices[ gl_VertexIndex ];
}