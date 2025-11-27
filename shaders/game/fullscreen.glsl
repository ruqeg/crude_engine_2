
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
  float                                                    wave_size;
  float                                                    wave_texcoord_scale;
  float                                                    wave_absolute_frame_scale;
  float                                                    aberration_strength_scale;
  float                                                    aberration_strength_offset;
  float                                                    aberration_strength_sin_affect;
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
  /* Drunk Effect */
  float wave_x = sin( in_texcoord.y * wave_texcoord_scale + scene.absolute_frame * wave_absolute_frame_scale ) * wave_size;
  float wave_y = cos( in_texcoord.x * wave_texcoord_scale + scene.absolute_frame * wave_absolute_frame_scale ) * wave_size;
  
  vec2 drunk_texcoord = vec2( in_texcoord.x + wave_x, in_texcoord.y + wave_y ); 
  
  float aberration_strength = sin( scene.absolute_frame * aberration_strength_sin_affect ) * aberration_strength_scale + aberration_strength_offset;

  vec2 offset = vec2( aberration_strength, aberration_strength );
  
  vec3 drunk_radiance;
  vec3 drunk_depth;
  drunk_depth.r = CRUDE_TEXTURE_LOD( depth_texture_index, drunk_texcoord + offset, 0 ).r;
  drunk_depth.g = CRUDE_TEXTURE_LOD( depth_texture_index, drunk_texcoord, 0 ).g;
  drunk_depth.b = CRUDE_TEXTURE_LOD( depth_texture_index, drunk_texcoord - offset, 0 ).b;
  drunk_radiance.r = CRUDE_TEXTURE_LOD( pbr_texture_index, drunk_texcoord + offset, 0 ).r;
  drunk_radiance.g = CRUDE_TEXTURE_LOD( pbr_texture_index, drunk_texcoord, 0 ).g;
  drunk_radiance.b = CRUDE_TEXTURE_LOD( pbr_texture_index, drunk_texcoord - offset, 0 ).b;

  vec3 drunk_pixel_world_position_r = crude_world_position_from_depth( in_texcoord, drunk_depth.r, scene.camera.clip_to_world );
  vec3 drunk_pixel_world_position_g = crude_world_position_from_depth( in_texcoord, drunk_depth.g, scene.camera.clip_to_world );
  vec3 drunk_pixel_world_position_b = crude_world_position_from_depth( in_texcoord, drunk_depth.b, scene.camera.clip_to_world );

  float drunk_visbility_r = length( player_position - drunk_pixel_world_position_r );
  float drunk_visbility_g = length( player_position - drunk_pixel_world_position_g );
  float drunk_visbility_b = length( player_position - drunk_pixel_world_position_b );

  vec3 drunk_with_fog_radiance;
  drunk_with_fog_radiance.r = ( drunk_depth.r != 1.f ) ? ( mix( drunk_radiance.x, fog_color.a * fog_color.x, clamp( pow( drunk_visbility_r / fog_distance, fog_coeff ), 0, 1 ) ) ) :  fog_color.a * fog_color.x;
  drunk_with_fog_radiance.g = ( drunk_depth.g != 1.f ) ? ( mix( drunk_radiance.y, fog_color.a * fog_color.y, clamp( pow( drunk_visbility_g / fog_distance, fog_coeff ), 0, 1 ) ) ) :  fog_color.a * fog_color.y;
  drunk_with_fog_radiance.b = ( drunk_depth.b != 1.f ) ? ( mix( drunk_radiance.z, fog_color.a * fog_color.z, clamp( pow( drunk_visbility_b / fog_distance, fog_coeff ), 0, 1 ) ) ) :  fog_color.a * fog_color.z;
  out_radiance = vec4( drunk_with_fog_radiance, 1.f );
}

#endif /* CRUDE_STAGE_FRAGMENT */

#endif /* GAME_POSTPROCESSING */