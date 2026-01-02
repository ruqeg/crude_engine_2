
#ifdef CRUDE_VALIDATOR_LINTING
#extension GL_GOOGLE_include_directive : enable
#define CRUDE_STAGE_FRAGMENT
//#define POSTPROCESSING
//#define COMPOSE
#define SSR

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


#if defined( COMPOSE ) && defined( CRUDE_STAGE_FRAGMENT ) 

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
  LightsIndicesRef                                         lights_indices;
  
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

#endif /* COMPOSE && CRUDE_STAGE_FRAGMENT */

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

#if defined( SSR )

layout(location=0) in vec2 in_texcoord;

/* Efficient GPU Screen-Space Ray Tracing (Morgan McGuire Michael Mara, 2014) */
bool crude_trace_screen_space_ray
(
  in vec3                                                  origin,
  in vec3                                                  direction,
  in mat4x4                                                projection,
  in uint                                                  zbuffer_texture_index,
  in vec2                                                  zbuffer_texture_size,
  in float                                                 z_thickness,
  in float                                                 near_plane_z,
  in float                                                 stride,
  in float                                                 jitter,
  in const float                                           max_steps,
  in float                                                 max_distance,
  out vec2                                                 hit_pixel,
  out vec3                                                 hit_point
)
{
  /* Clip to the near plane */
  float ray_length = ( ( origin.z + direction.z * max_distance ) > near_plane_z ) ? ( near_plane_z - origin.z ) / direction.z : max_distance;
  
  vec3 end_point = origin + direction * ray_length;
  hit_pixel = vec2( -1, -1 );

  vec4 h0 = vec4( origin, 1.0 ) * projection;
  vec4 h1 = vec4( end_point, 1.0 ) * projection;
  float k0 = 1.0 / h0.w;
  float k1 = 1.0 / h1.w;
  vec3 q0 = origin * k0;
  vec3 q1 = end_point * k1;

  /* Screen-space endpoints */
  vec2 p0 = h0.xy * k0;
  vec2 p1 = h1.xy * k1;

  // !TODO maybe [ Optionally clip here using listing 4 ]

  p1 += vec2( (dot( p1 - p0, p1 - p0 ) < 0.0001 ) ? 0.01 : 0.0 );
  vec2 delta = p1 - p0;

  bool permute = false;
  if ( abs( delta.x ) < abs( delta.y ) )
  {
    permute = true;
    delta = delta.yx;
    p0 = p0.yx;
    p1 = p1.yx;
  }

  float step_dir = sign( delta.x );
  float invdx = step_dir / delta.x;

  /* Track the derivatives of Q and k. */
  vec3 dq = ( q1 - q0 ) * invdx;
  float dk = ( k1 - k0 ) * invdx;
  vec2 dp = vec2( step_dir, delta.y * invdx );

  dp *= stride;
  dq *= stride;
  dk *= stride;
  p0 += dp * jitter;
  q0 += dq * jitter;
  k0 += dk * jitter;
  
  float prev_zmax_estimate = origin.z;

  /* Slide P from P0 to P1, (now-homogeneous) Q from Q0 to Q1, k from k0 to k1 */
  vec3 q = q0;
  float k = k0;
  float step_count = 0.0;
  float end = p1.x * step_dir;
  for ( vec2 p = p0; ( ( p.x * step_dir ) <= end ) && ( step_count < max_steps ); p += dp, q.z += dq.z, k += dk, step_count += 1.0 )
  {
    /* Project back from homogeneous to camera space */
    hit_pixel = permute ? p.yx : p;

    /* The depth range that the ray covers within this loop iteration. */
    /* Assume that the ray is moving in increasing z and swap if backwards. */
    float ray_zmin = prev_zmax_estimate;
    
    /* Compute the value at 1/2 pixel into the future */
    float ray_zmax = ( dq.z * 0.5 + q.z ) / ( dk * 0.5 + k );
    prev_zmax_estimate = ray_zmax;
    if ( ray_zmin > ray_zmax )
    {
      float temp = ray_zmin;
      ray_zmin = ray_zmax;
      ray_zmax = temp;
    }

    /* Camera-space z of the background at each layer (there can be up to 4) */
    float scene_zmax = CRUDE_TEXTURE_FETCH( zbuffer_texture_index, ivec2( hit_pixel ), 0).r;

    //if ( zbuffer_is_hyperbolic )
    //{
    //  for ( int layer = 0; layer < numLayers; ++layer )
    //  {
    //    sceneZMax[layer] = reconstructCSZ(sceneZMax[layer], clipInfo);
    //  }
    //}
    
    float scene_zmin = scene_zmax - z_thickness;

    //for (int L = 0; L < numLayers; ++L)
    //{
    //  if (((rayZMax >= sceneZMin[L]) && (rayZMin <= sceneZMax[L])) || (sceneZMax[L] == 0))
    //  {
    //    hitLayer = layer;
    //    break; // Breaks out of both loops, since the inner loop is a macro
    //  }
    //}

    if ( ( ( ray_zmax >= scene_zmin ) && ( scene_zmin <= scene_zmax ) ) || ( scene_zmax == 0 ) )
    {
      break;
    }
  }

  /* Advance Q based on the number of steps */
  q.xy += dq.xy * step_count;
  hit_point = q * ( 1.0 / k );

  return all( lessThanEqual( abs( hit_pixel - ( zbuffer_texture_size * 0.5 ) ), zbuffer_texture_size * 0.5 ) );
}

CRUDE_PUSH_CONSTANT
{
  SceneRef                                                 scene;
  float                                                    ssr_max_steps;
  float                                                    ssr_max_distance;

  float                                                    ssr_jitter;
  float                                                    ssr_stride;
  float                                                    ssr_z_thickness;
  uint                                                     depth_texture_index;

  vec2                                                     depth_texture_size;
  vec2                                                     _padding;
};

void main()
{
  vec2 hit_pixel_coords;
  vec3 hit_point;
  
  //vec4 dir = vec4( in_texcoord, in_texcoord, 1, 1 ) * scene.camera.clip_to_world;
  //dir = normalize( dir );
  //return dir.xyz;
  
  vec3 ray_origin = vec3( 0);
  vec3 ray_direction = vec3( 0 );
  bool hitted = crude_trace_screen_space_ray(
    ray_origin, ray_direction,
    scene.data.camera.view_to_clip, depth_texture_index, depth_texture_size,
    ssr_z_thickness, scene.data.camera.znear, ssr_stride, ssr_jitter, ssr_max_steps, ssr_max_distance, hit_pixel_coords, hit_point );
  
  if ( hitted )
  {
    
  }
}

#endif /* SSR */