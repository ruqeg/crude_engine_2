
#ifdef CRUDE_VALIDATOR_LINTING
#extension GL_GOOGLE_include_directive : enable
#define CRUDE_STAGE_FRAGMENT
//#define CRUDE_STAGE_VERTEX
#define GAME_POSTPROCESSING

#include "crude/platform.glsli"
#include "crude/debug.glsli"
#include "crude/scene.glsli"
#endif /* CRUDE_VALIDATOR_LINTING */

#if defined( GAME_POSTPROCESSING ) 

CRUDE_UNIFORM( SceneConstant, 0 ) 
{
  crude_scene                                              scene;
};

CRUDE_PUSH_CONSTANT( Constants )
{
  vec3                                                     player_position;
  uint                                                     depth_texture_index;
  vec4                                                     fog_color;
  uint                                                     pbr_texture_index;
  float                                                    visibility_sq;
  vec2                                                     padding;
};

#if defined( CRUDE_STAGE_VERTEX )

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
  out_texcoord = fullscreen_texcoord[ gl_VertexIndex ];
  gl_Position = fullscreen_vertices[ gl_VertexIndex ];
}

#endif /* CRUDE_STAGE_VERTEX */

#if defined( CRUDE_STAGE_FRAGMENT ) 

layout(location = 0) out vec4 out_radiance;

layout(location=0) in vec2 in_texcoord;

void main()
{ 
  float depth = CRUDE_TEXTURE_FETCH( depth_texture_index, ivec2( gl_FragCoord.xy ), 0 ).r;
  vec4 radiance = CRUDE_TEXTURE_FETCH( pbr_texture_index, ivec2( gl_FragCoord.xy ), 0 );

  vec3 pixel_world_position = crude_world_position_from_depth( in_texcoord, depth, scene.camera.clip_to_world );

  uvec2 position = uvec2( gl_FragCoord.x - 0.5, gl_FragCoord.y - 0.5 );
  vec3 player_to_pixel = player_position - pixel_world_position;
  float player_to_pixel_length_sq = abs( dot( player_to_pixel, player_to_pixel ) );

  vec4 new_radiance = vec4( 0.f, 0.f, 0.f, 0.f );
  if ( depth != 1.f )
  {
    new_radiance = vec4( mix( radiance.xyz, fog_color.a * fog_color.xyz, clamp( pow( player_to_pixel_length_sq / visibility_sq, 1.0 / 10.0 ), 0, 1 ) ), 1.f );
  }
  else
  {
    new_radiance = radiance;
  }
  out_radiance = new_radiance;
}

#endif /* CRUDE_STAGE_FRAGMENT */

#endif /* GAME_POSTPROCESSING */