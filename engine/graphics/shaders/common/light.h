
#ifndef CRUDE_LIGHT_GLSLI
#define CRUDE_LIGHT_GLSLI

#define CRUDE_MAX_MESHLETS_PER_LIGHT  ( 45000 )
#define CRUDE_LIGHT_Z_BINS            ( 16 )
#define CRUDE_LIGHTS_MAX_COUNT        ( 256 )
#define CRUDE_LIGHT_TILE_SIZE         ( 8 )
#define CRUDE_LIGHT_WORDS_COUNT       ( ( CRUDE_LIGHTS_MAX_COUNT + 31 ) / 32 )

#define CRUDE_SHADOW_FILTER_NUM_SAMPLES ( 16 )
#define CRUDE_SHADOW_FILTER_RADIUS ( 1.5 )

CRUDE_SHADER_STRUCT( crude_gfx_light )
{
  XMFLOAT3                                                 world_position;
  float32                                                  radius;
  XMFLOAT3                                                 color;
  float32                                                  intensity;
};

CRUDE_SHADER_RBUFFER_REF_ARRAY_SCALAR( LightsZBinsRef, uint32 );
CRUDE_SHADER_RBUFFER_REF_ARRAY_SCALAR( LightsRef, crude_gfx_light );
CRUDE_SHADER_RBUFFER_REF_ARRAY_SCALAR( LightsTilesRef, uint32 );
CRUDE_SHADER_RBUFFER_REF_ARRAY_SCALAR( LightsIndicesRef, uint32 );
CRUDE_SHADER_RBUFFER_REF_ARRAY_SCALAR( LightsShadowViewsRef, XMFLOAT4X4 );
CRUDE_SHADER_RBUFFER_REF_ARRAY_SCALAR( LightsTrianglesIndicesRef, uint8 );

#ifndef __cplusplus

const vec2 filter_kernel[ CRUDE_SHADOW_FILTER_NUM_SAMPLES ] =
{
  vec2(-0.94201624, -0.39906216),
  vec2(0.94558609, -0.76890725),
  vec2(-0.094184101, -0.92938870),
  vec2(0.34495938, 0.29387760),
  vec2(-0.91588581, 0.45771432),
  vec2(-0.81544232, -0.87912464),
  vec2(-0.38277543, 0.27676845),
  vec2(0.97484398, 0.75648379),
  vec2(0.44323325, -0.97511554),
  vec2(0.53742981, -0.47373420),
  vec2(-0.26496911, -0.41893023),
  vec2(0.79197514, 0.19090188),
  vec2(-0.24188840, 0.99706507),
  vec2(-0.81409955, 0.91437590),
  vec2(0.19984126, 0.78641367),
  vec2(0.14383161, -0.14100790)
};

const vec3 crude_tetrahedron_face_a = vec3( 0.0, -0.57735026, 0.81649661 );
const vec3 crude_tetrahedron_face_b = vec3( 0.0, -0.57735026, -0.81649661 );
const vec3 crude_tetrahedron_face_c = vec3( -0.81649661, 0.57735026, 0.0 );
const vec3 crude_tetrahedron_face_d = vec3( 0.81649661, 0.57735026, 0.0 );

vec3
crude_schlick_fresnel
(
  in vec3                                                  f0,
  in float                                                 ndotl
)
{
  return f0 + ( 1.f - f0 ) * pow( ( 1.f - max( ndotl, 0.0 ) ), 5.f );
}

float
crude_trowbridge_reitz_distribution
(
  in float                                                 ndotm,
  in float                                                 r2
)
{
  float x1 = max( CRUDE_PI * ( 1.f + ndotm * ndotm * ( r2 - 1.f ) ), 0.0001f );
  float x2 = sign( max( ndotm, 0.f ) ) * r2;
  return x2 / x1;
}

float 
crude_hammon_smith_g_approximation
(
  in float                                                 ndotl_abs,
  in float                                                 ndotv_abs,
  in float                                                 r
)
{
  return 0.5f / mix( 2 * ndotl_abs * ndotv_abs, ndotl_abs + ndotv_abs, r );
}

uint 
crude_get_tetrahedron_face_index
(
  in vec3                                                  dir
)
{
  mat4x3 face_matrix;
  face_matrix[ 0 ] = crude_tetrahedron_face_a;
  face_matrix[ 1 ] = crude_tetrahedron_face_b;
  face_matrix[ 2 ] = crude_tetrahedron_face_c;
  face_matrix[ 3 ] = crude_tetrahedron_face_d; 
  vec4 dot_products = dir * face_matrix;
  float maximum = max (max( dot_products.x, dot_products.y ), max( dot_products.z, dot_products.w ) );
  
  uint index;
  if ( maximum == dot_products.x )
  {
    index = 0;
  }
  else if ( maximum == dot_products.y )
  {
    index = 1;
  }
  else if ( maximum == dot_products.z )
  {
    index = 2;
  }
  else
  { 
    index = 3;
  }

  return index;
}

#if defined( CRUDE_STAGE_FRAGMENT )

float
crude_calculate_point_light_shadow_contribution
(
  in crude_gfx_light                                       light,
  in vec3                                                  vertex_position,
  in SceneRef                                              scene,
  in LightsShadowViewsRef                                  light_shadow_views
)
{
#ifdef CRUDE_RAYTRACED_SHADOWS
  rayQueryEXT                                              ray_query;
  float                                                    vertex_to_light_distance;
  vec3                                                     vertex_to_light, vertex_to_light_normalized;

  vertex_to_light = light.world_position - vertex_position.xyz;
  vertex_to_light_distance = length( vertex_to_light );
  vertex_to_light_normalized = vertex_to_light / vertex_to_light_distance;

  float visiblity = 0.f;
  if ( vertex_to_light_distance <= light.radius )
  {
    rayQueryInitializeEXT( ray_query, acceleration_structure, gl_RayFlagsOpaqueEXT | gl_RayFlagsTerminateOnFirstHitEXT, 0xff, vertex_position, 0.05, vertex_to_light_normalized, vertex_to_light_distance );
    rayQueryProceedEXT( ray_query );
    visiblity = float( rayQueryGetIntersectionTypeEXT( ray_query, true ) == gl_RayQueryCommittedIntersectionNoneEXT );
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
  proj_pos = vec4( vertex_position.xyz, 1.0 ) * light_shadow_views.data[ 0 * 4 + face_index ];
  proj_pos.xyz /= proj_pos.w;
    
  proj_uv = ( proj_pos.xy * 0.5 ) + 0.5;
  proj_uv.y = 1.f - proj_uv.y;

  bias = 0.003f;
  current_depth = proj_pos.z;
    
  filter_radius = scene.data.inv_shadow_map_size.xy * CRUDE_SHADOW_FILTER_RADIUS; 
  
  shadow_factor = 0;
  for ( uint i = 0; i < CRUDE_SHADOW_FILTER_NUM_SAMPLES; ++i )
  {
    vec2 texcoords = proj_uv.xy + ( filter_kernel[i] * filter_radius );
    float closest_depth = texture( global_textures[ nonuniformEXT( scene.data.tiled_shadowmap_texture_index ) ], texcoords ).r;
    shadow_factor += current_depth - bias < closest_depth ? 1 : 0;
  }

  return shadow_factor / CRUDE_SHADOW_FILTER_NUM_SAMPLES;
#endif /* CRUDE_RAYTRACED_SHADOWS */
}

vec3
crude_calculate_point_light_contribution
(
  in crude_gfx_light                                        light,
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
  float                                                    light_distance, ndotl, ndoth, ndotv, hdotl;

  light_to_position = light.world_position - vertex_position;
  light_distance = length( light_to_position );

  l = light_to_position / light_distance;
  v = normalize( camera_position - vertex_position );
  h = normalize( l + v );

  ndotl = dot( normal, l );
  ndoth = dot( normal, h );
  ndotv = dot( normal, v );
  hdotl = dot( h, l );

  radiance = light.color * light.intensity * crude_light_attenuation( light_distance, light.radius );

  f = crude_schlick_fresnel( f0, hdotl );
  spec = f * crude_trowbridge_reitz_distribution( ndoth, roughness * roughness ) * crude_hammon_smith_g_approximation( abs( ndotl ), abs( ndotv ), roughness );
  diff = ( 1.f - f ) * albedo / CRUDE_PI;
  return radiance * max( ndotl, 0.f ) * ( ( 1.f - metalness ) * diff + spec );
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
  in SceneRef                                              scene,
  in LightsRef                                             lights
)
{
  // !TODO LIGHTS INDICES PER CLUSTER INSTEAD + WAVE INSTRUCTION PER CLUSTER in case I will need it in future
  // (don't want to overcook for now)

  vec4                                                     view_position;
  vec3                                                     direct_radiance, indirect_irradiance, indirect_diffuse;
  vec3                                                     f0;

  f0 = CRUDE_DEAFULT_F0;

  direct_radiance = vec3( 0 );

  direct_radiance.xyz += ( 1.f - metalness ) * albedo.xyz * scene.data.ambient_color * scene.data.ambient_intensity;

  if ( scene.data.active_lights_count < 1 )
  {
    return direct_radiance;
  }

  for ( uint i = 0; i < scene.data.active_lights_count; ++i )
  {
    direct_radiance += crude_calculate_point_light_contribution( lights.data[ i ], albedo.rgb, roughness, metalness, normal, vertex_position, camera_position, f0 );
  }

  return direct_radiance;
}

#endif /* CRUDE_STAGE_FRAGMENT */

#endif

#endif /* CRUDE_LIGHT_GLSLI */