
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
  float                                                    fog_distance;
  float                                                    fog_coeff;
  float                                                    padding;
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

vec4 drunk_effect
(
  in vec2                                                  texcoord
)
{
  float wave_x = sin( texcoord.y * 20.0 + scene.time * 5.0 ) * 0.02;
  float wave_y = cos( texcoord.x * 15.0 + scene.time * 4.0 ) * 0.02;
    
  texcoord.x += wave_x;
  texcoord.y += wave_y;

  float aberration_strength = 0.005;
  aberration_strength = sin( scene.time * 2.0) * 0.005 + 0.005;

  vec2 offset = vec2( aberration_strength, aberration_strength );
    
  float r = CRUDE_TEXTURE_LOD( pbr_texture_index, texcoord + offset, 0 ).r;
  float g = CRUDE_TEXTURE_LOD( pbr_texture_index, texcoord, 0 ).g;
  float b = CRUDE_TEXTURE_LOD( pbr_texture_index, texcoord - offset, 0 ).b;

  return vec4( r, g, b, 1.0 );
}

void main()
{ 
  float depth = CRUDE_TEXTURE_FETCH( depth_texture_index, ivec2( gl_FragCoord.xy ), 0 ).r;
  //vec4 radiance = CRUDE_TEXTURE_FETCH( pbr_texture_index, ivec2( gl_FragCoord.xy ), 0 );
  vec4 drunk_radiance = drunk_effect( in_texcoord );

  vec3 pixel_world_position = crude_world_position_from_depth( in_texcoord, depth, scene.camera.clip_to_world );

  uvec2 position = uvec2( gl_FragCoord.x - 0.5, gl_FragCoord.y - 0.5 );
  float current_visbility = length( player_position - pixel_world_position );

  vec4 new_radiance = vec4( 0.f, 0.f, 0.f, 0.f );
  if ( depth != 1.f )
  {
    new_radiance = vec4( mix( drunk_radiance.xyz, fog_color.a * fog_color.xyz, clamp( pow( current_visbility / fog_distance, fog_coeff ), 0, 1 ) ), 1.f );
  }
  else
  {
    new_radiance = vec4( fog_color.a * fog_color.xyz, 1.0 );
  }
  out_radiance = new_radiance;
}

#endif /* CRUDE_STAGE_FRAGMENT */

#endif /* GAME_POSTPROCESSING */