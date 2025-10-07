
#ifdef CRUDE_VALIDATOR_LINTING
#extension GL_GOOGLE_include_directive : enable
#define CRUDE_STAGE_FRAGMENT
#define LIGHT_PBR

#include "crude/platform.glsli"
#include "crude/debug.glsli"
#include "crude/scene.glsli"
#include "crude/light.glsli"
#include "crude/culling.glsli"
#endif /* CRUDE_VALIDATOR_LINTING */

#define CRUDE_RAYTRACED_SHADOWS

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

CRUDE_UNIFORM( SceneConstant, 0 ) 
{
  crude_scene                                              scene;
};

CRUDE_RBUFFER( ZBins, 1 ) 
{
  uint                                                     bins[];
};

CRUDE_RBUFFER( Lights, 2 ) 
{
  crude_light                                              lights[];
};

CRUDE_RBUFFER( Tiles, 3 ) 
{
  uint                                                     lights_tiles[];
};

CRUDE_RBUFFER( LightIndices, 4 ) 
{
  uint                                                     lights_indices[];
};

CRUDE_RBUFFER( ShadowViews, 5 )
{
  mat4                                                      pointlight_world_to_clip[];
};

CRUDE_UNIFORM( LightingConstants, 6 )
{
  uvec4                                                    textures;
};

float
crude_calculate_point_light_shadow_contribution
(
  in crude_light                                           light,
  in vec3                                                  vertex_position
)
{
#ifdef CRUDE_RAYTRACED_SHADOWS
  rayQueryEXT                                              ray_query;
  float                                                    vertex_to_light_distance;
  vec3                                                     vertex_to_light, vertex_to_light_normalized;

  vertex_to_light = light.world_position - vertex_position.xyz;
  vertex_to_light_distance = length( vertex_to_light );
  vertex_to_light_normalized = vertex_to_light / vertex_to_light_distance;
  rayQueryInitializeEXT( ray_query, acceleration_structure, gl_RayFlagsOpaqueEXT | gl_RayFlagsTerminateOnFirstHitEXT, 0xff, vertex_position, 0.05, vertex_to_light_normalized, vertex_to_light_distance );
  rayQueryProceedEXT( ray_query );

  float visiblity = 0.f;
  if ( rayQueryGetIntersectionTypeEXT( ray_query, true ) != gl_RayQueryCommittedIntersectionNoneEXT )
  {
    visiblity = rayQueryGetIntersectionTEXT( ray_query, true ) > vertex_to_light_distance ? 1.f : 0.f;
  }
  else
  {
    visiblity = 1.f;
  }
  return visiblity;
#else /* CRUDE_RAYTRACED_SHADOWS */
  vec4                                                     proj_pos;
  vec3                                                     vertex_to_light;
  vec2                                                     proj_uv, filter_radius;
  uint                                                     face_index;
  float                                                    bias, current_depth, shadow_factor;

  vertex_to_light = light.world_position - vertex_position.xyz;
  face_index = crude_get_tetrahedron_face_index( normalize( -vertex_to_light ) );
  proj_pos = vec4( vertex_position.xyz, 1.0 ) * pointlight_world_to_clip[ 0 * 4 + face_index ];
  proj_pos.xyz /= proj_pos.w;
    
  proj_uv = ( proj_pos.xy * 0.5 ) + 0.5;
  proj_uv.y = 1.f - proj_uv.y;

  bias = 0.003f;
  current_depth = proj_pos.z;
    
  filter_radius = scene.inv_shadow_map_size.xy * CRUDE_SHADOW_FILTER_RADIUS; 
  
  shadow_factor = 0;
  for ( uint i = 0; i < CRUDE_SHADOW_FILTER_NUM_SAMPLES; ++i )
  {
    vec2 texcoords = proj_uv.xy + ( filter_kernel[i] * filter_radius );
    float closest_depth = texture( global_textures[ nonuniformEXT( scene.tiled_shadowmap_texture_index ) ], texcoords ).r;
    shadow_factor += current_depth - bias < closest_depth ? 1 : 0;
  }

  return shadow_factor / CRUDE_SHADOW_FILTER_NUM_SAMPLES;
#endif /* CRUDE_RAYTRACED_SHADOWS */
}

vec3
crude_calculate_point_light_contribution
(
  in crude_light                                           light,
  in vec3                                                  albedo,
  in float                                                 roughness,
  in float                                                 metalness,
  in vec3                                                  normal,
  in vec3                                                  vertex_position,
  in vec3                                                  camera_position,
  in vec3                                                  f0
)
{
  vec3                                                     indirect_irradiance, indirect_diffuse, light_to_position, l, radiance, f, spec, diff, h, v;
  float                                                    light_distance, attenuation, ndotl, ndoth, ndotv, hdotl;

  roughness = 0.2;
  light_to_position = light.world_position - vertex_position;
  light_distance = length( light_to_position );

  l = light_to_position / light_distance;
  v = normalize( camera_position - vertex_position );
  h = normalize( l + v );

  ndotl = dot( normal, l );
  ndoth = dot( normal, h );
  ndotv = dot( normal, v );
  hdotl = dot( h, l );

  attenuation = max( 1.f - pow( light_distance / light.radius, 2.f ), 0.f );
  attenuation = attenuation * attenuation;

  radiance = light.color * light.intensity * attenuation;

  f = crude_schlick_fresnel( f0, hdotl );
  spec = f * crude_trowbridge_reitz_distribution( ndoth, roughness * roughness ) * crude_hammon_smith_g_approximation( abs( ndotl ), abs( ndotv ), roughness ); /* in case i will be confused in the future, crude_hammon_smith_g_approximation already contains /4ndotlndotv (real time rendering p342) */
  diff = ( 1.f - f ) * albedo / PI;
  return radiance * max( ndotl, 0.f ) * ( diff + spec );
}

vec3
crude_calculate_lighting
(
  in vec4                                                  albedo,
  in float                                                 roughness,
  in float                                                 metalness,
  in vec3                                                  normal,
  in vec3                                                  vertex_position,
  in vec3                                                  camera_position,
  in uvec2                                                 position,
  in vec2                                                  screen_texcoord
)
{
  vec4                                                     view_position;
  vec3                                                     radiance, indirect_irradiance, indirect_diffuse;
  uvec2                                                    tile;
  float                                                    linear_d;
  int                                                      bin_index;
  uint                                                     bin_value, min_light_id, max_light_id, stride, address;

  vec3 f0 = vec3( 0.04f );

  radiance = vec3( 0 );

  view_position = vec4( vertex_position, 1.0 ) * scene.camera.world_to_view;

  linear_d = ( view_position.z - scene.camera.znear ) / ( scene.camera.zfar - scene.camera.znear );
  bin_index = int( linear_d * CRUDE_LIGHT_BINS_COUNT );
  bin_value = bins[ bin_index ];

  min_light_id = bin_value & 0xFFFF;
  max_light_id = ( bin_value >> 16 ) & 0xFFFF;

  tile = position / uint( CRUDE_LIGHT_TILE_SIZE );

  stride = uint( CRUDE_LIGHT_WORDS_COUNT ) * ( uint( scene.resolution.x ) / uint( CRUDE_LIGHT_TILE_SIZE ) );
  address = tile.y * stride + tile.x * CRUDE_LIGHT_WORDS_COUNT;

  if ( min_light_id != CRUDE_LIGHTS_MAX_COUNT + 1 )
  {
    for ( uint light_id = min_light_id; light_id <= max_light_id; ++light_id )
    {
      uint word_id = light_id / 32;
      uint bit_id = light_id % 32;

      if ( ( lights_tiles[ address + word_id ] & ( 1 << bit_id ) ) != 0 )
      {
        uint global_light_index = lights_indices[ light_id ];
        radiance += crude_calculate_point_light_shadow_contribution( lights[ global_light_index ], vertex_position ) * crude_calculate_point_light_contribution( lights[ global_light_index ], albedo.rgb, roughness, metalness, normal, vertex_position, camera_position, f0 );
      }
    }
  }

  indirect_irradiance = CRUDE_TEXTURE_LOD( scene.indirect_light_texture_index, screen_texcoord, 0 ).rgb;
  indirect_diffuse = indirect_irradiance * albedo.rgb;
  const float ao = 1.0f;
  radiance.xyz += indirect_diffuse * ao;

  return radiance;
}

void main()
{ 
  float depth = CRUDE_TEXTURE_FETCH( textures.w, ivec2( gl_FragCoord.xy ), 0 ).r;
  vec4 albedo = texture( global_textures[ nonuniformEXT( textures.x ) ], in_texcoord.st ).rgba;
  vec2 packed_normal = texture( global_textures[ nonuniformEXT( textures.y ) ], in_texcoord.st ).xy;
  vec2 packed_roughness_metalness = texture( global_textures[ nonuniformEXT( textures.z ) ], in_texcoord.st ).xy;
  vec3 normal = crude_octahedral_decode( packed_normal );

  vec3 pixel_world_position = crude_world_position_from_depth( in_texcoord, depth, scene.camera.clip_to_world );

  uvec2 position = uvec2( gl_FragCoord.x - 0.5, gl_FragCoord.y - 0.5 );
  
  vec4 radiance = vec4( 0.f, 0.f, 0.f, 1.f );
  if ( depth != 1.f )
  {
    radiance = vec4( crude_calculate_lighting( albedo, packed_roughness_metalness.x, packed_roughness_metalness.y, normal, pixel_world_position, scene.camera.position, position, in_texcoord.st ), 1 );
  }
  out_color = radiance;
}

#endif /* LIGHT_PBR && CRUDE_STAGE_FRAGMENT */

#if defined( POSTPROCESSING ) && defined( CRUDE_STAGE_FRAGMENT ) 

layout(location = 0) out vec4 out_color;
layout(location=0) in vec2 in_texcoord;

layout( push_constant ) uniform Constants
{
  uint                                                     luminance_average_texture_index;
  uint                                                     pbr_texture_index;
};

void main()
{ 
  vec4 color = CRUDE_TEXTURE_FETCH( pbr_texture_index, ivec2( gl_FragCoord.xy ), 0 );
  float luminance_average = CRUDE_TEXTURE_FETCH( luminance_average_texture_index, ivec2( 0, 0 ), 0 ).r;
  
  float luminance = crude_rgb_to_luminance( color.xyz );
  color.xyz = color.xyz * ( luminance / ( 9.6 * luminance_average ) );
  color.xyz = crude_aces_fitted2( color.xyz );
  color.xyz = pow( color.xyz, vec3( 1 / 2.2 ) );
  out_color = color;
}
#endif /* POSTPROCESSING && CRUDE_STAGE_FRAGMENT */