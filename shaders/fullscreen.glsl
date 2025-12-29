
#ifdef CRUDE_VALIDATOR_LINTING
#extension GL_GOOGLE_include_directive : enable
#define CRUDE_STAGE_FRAGMENT
//#define POSTPROCESSING
#define LIGHT_PBR

#include "crude/platform.glsli"
#include "crude/debug.glsli"
#include "crude/scene.glsli"
#include "crude/light.glsli"
#include "crude/culling.glsli"
#endif /* CRUDE_VALIDATOR_LINTING */

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


#if defined( LIGHT_PBR ) && defined( CRUDE_STAGE_FRAGMENT ) 

#if defined ( CRUDE_RAYTRACED_SHADOWS )
#extension GL_EXT_ray_tracing : require
#extension GL_EXT_ray_query : require

layout(set=CRUDE_MATERIAL_SET, binding=10) uniform accelerationStructureEXT acceleration_structure;
#endif /* CRUDE_RAYTRACED_SHADOWS */

layout(location = 0) out vec4 out_color;

layout(location=0) in vec2 in_texcoord;


CRUDE_PUSH_CONSTANT
{
  uvec4                                                    textures;

  SceneRef                                                 scene;
  LightsZBinsRef                                           zbins;
  
  LightsTilesRef                                           lights_tiles;
  LightsTrianglesIndicesRef                                lights_indices;
  
  LightsRef                                                lights;
  LightsShadowViewsRef                                     light_shadow_views;
};

void main()
{ 
  float depth = CRUDE_TEXTURE_FETCH( textures.w, ivec2( gl_FragCoord.xy ), 0 ).r;
  vec4 albedo = texture( global_textures[ nonuniformEXT( textures.x ) ], in_texcoord.st ).rgba;
  vec2 packed_normal = texture( global_textures[ nonuniformEXT( textures.y ) ], in_texcoord.st ).xy;
  vec2 packed_roughness_metalness = texture( global_textures[ nonuniformEXT( textures.z ) ], in_texcoord.st ).xy;
  vec3 normal = crude_octahedral_decode( packed_normal );

  vec3 pixel_world_position = crude_world_position_from_depth( in_texcoord, depth, scene.data.camera.clip_to_world );

  uvec2 position = uvec2( gl_FragCoord.x - 0.5, gl_FragCoord.y - 0.5 );
  
  vec3 radiance = vec3( 0.f, 0.f, 0.f );
  if ( depth != 1.f )
  {
    radiance = crude_calculate_lighting(
      albedo, packed_roughness_metalness.x, packed_roughness_metalness.y, normal, pixel_world_position, scene.data.camera.position, position, in_texcoord.st,
      scene, zbins, lights_tiles, lights_indices, lights, light_shadow_views );
  }
  else
  {
    radiance = scene.data.background_color * scene.data.background_intensity;
  }
  out_color = vec4( radiance, 1.f );
}

#endif /* LIGHT_PBR && CRUDE_STAGE_FRAGMENT */

#if defined( POSTPROCESSING ) && defined( CRUDE_STAGE_FRAGMENT ) 

layout(location = 0) out vec4 out_color;
layout(location=0) in vec2 in_texcoord;

CRUDE_PUSH_CONSTANT
{
  uint                                                     luminance_average_texture_index;
  uint                                                     pbr_texture_index;
  float                                                    inv_gamma;
  float                                                    _pust_constant_padding;
};

void main()
{ 
  vec4 color = CRUDE_TEXTURE_FETCH( pbr_texture_index, ivec2( gl_FragCoord.xy ), 0 );
  float luminance_average = CRUDE_TEXTURE_FETCH( luminance_average_texture_index, ivec2( 0, 0 ), 0 ).r;
  
  float luminance = crude_rgb_to_luminance( color.xyz );
  color.xyz = color.xyz * ( luminance / ( 9.6 * luminance_average ) );
  color.xyz = crude_aces_fitted( color.xyz );
  color.xyz = pow( color.xyz, vec3( inv_gamma ) );
  out_color = color;
}
#endif /* POSTPROCESSING && CRUDE_STAGE_FRAGMENT */