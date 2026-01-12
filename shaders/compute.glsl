
#ifdef CRUDE_VALIDATOR_LINTING
#extension GL_GOOGLE_include_directive : enable
//#define LUMINANCE_AVERAGE_CALCULATION
//#define LIGHT_LUT
//#define LUMINANCE_HISTOGRAM_GENERATION
//#define CULLING_EARLY
//#define CULLING_LATE
//#define DEPTH_PYRAMID
//#define SSR_COMPOSE
//#define SSR_CONVOLVE_VERTICAL
//#define SSR_CONVOLVE_HORIZONTAL
//#define SSR_HIT_CALCULATION
#define CRUDE_COMPUTE
#include "crude/platform.glsli"
#include "crude/debug.glsli"
#include "crude/scene.glsli"
#include "crude/culling.glsli"
#endif /* CRUDE_VALIDATOR_LINTING */

#if defined( CULLING_EARLY ) || defined( CULLING_LATE )
layout(local_size_x=64, local_size_y=1, local_size_z=1) in;

CRUDE_RBUFFER_REF( MeshDrawCountRef )
{
  uint                                                     opaque_mesh_visible_early_count;
  uint                                                     opaque_mesh_visible_late_count;
  uint                                                     opaque_mesh_culled_count;
  uint                                                     transparent_mesh_visible_count;

  uint                                                     transparent_mesh_culled_count;
  uint                                                     total_mesh_count;
  uint                                                     depth_pyramid_texture_index;
  uint                                                     meshlet_index_count;

  uint                                                     dispatch_task_x;
  uint                                                     dispatch_task_y;
  uint                                                     dispatch_task_z;
  uint                                                     meshlet_instances_count;
};

CRUDE_PUSH_CONSTANT
{
  SceneRef                                                 scene;
  MeshDrawsRef                                             mesh_draws;

  MeshInstancesDrawsRef                                    mesh_instance_draws;
  MeshBoundsRef                                            mesh_bounds;

  MeshDrawCommandsRef                                      mesh_draw_commands;
  MeshDrawCommandsRef                                      mesh_draw_commands_culled;

  MeshDrawCountRef                                         mesh_draw_count;
  vec2                                                     _padding;
};

void write_mesh_draw_command
(
  out crude_mesh_draw_command                              mesh_draw_command,
  in uint                                                  mesh_instance_draw_index,
  in uint                                                  mesh_draw_index
)
{
  mesh_draw_command.draw_id = mesh_instance_draw_index;
  mesh_draw_command.indirect_meshlet_group_count_x = ( mesh_draws.data[ mesh_draw_index ].meshletes_count + 31 ) / 32;
  mesh_draw_command.indirect_meshlet_group_count_y = 1;
  mesh_draw_command.indirect_meshlet_group_count_z = 1;
  mesh_draw_command.indirect_mesh_vertex_count = mesh_draws.data[ mesh_draw_index ].mesh_indices_count;
  mesh_draw_command.indirect_mesh_instance_count = 1;
  mesh_draw_command.indirect_mesh_first_vertex = 0;
  mesh_draw_command.indirect_mesh_first_instance = 0;
}

void main()
{
  uint mesh_instance_draw_index = gl_GlobalInvocationID.x;

#if defined( CULLING_LATE )
  if ( mesh_instance_draw_index < mesh_draw_count.opaque_mesh_culled_count )
  {
    mesh_instance_draw_index = mesh_draw_commands_culled.data[ mesh_instance_draw_index ].draw_id;
  }
  else if ( int(mesh_instance_draw_index) - mesh_draw_count.opaque_mesh_culled_count < mesh_draw_count.transparent_mesh_culled_count )
  {
    mesh_instance_draw_index = mesh_draw_commands_culled.data[ int(mesh_instance_draw_index) - mesh_draw_count.opaque_mesh_culled_count + mesh_draw_count.total_mesh_count ].draw_id;
  }
  else
  {
    return;
  }
#else
  if ( mesh_instance_draw_index >= mesh_draw_count.total_mesh_count )
  {
    return;
  }
#endif
  
  uint mesh_draw_index = mesh_instance_draws.data[ mesh_instance_draw_index ].mesh_draw_index;
  mat4 mesh_to_world = mesh_instance_draws.data[ mesh_instance_draw_index ].mesh_to_world;
  vec4 bounding_sphere = mesh_bounds.data[ mesh_draw_index ];
  vec4 world_center = vec4( bounding_sphere.xyz, 1 ) * mesh_to_world;
  float scale = crude_calculate_scale_from_matrix( mat3( mesh_to_world ) );
  float radius = bounding_sphere.w * scale * 1.1;

  vec4 view_center = world_center * scene.data.camera.world_to_view;

  bool frustum_visible = true;
  for ( uint i = 0; i < 6; ++i )
  {
    frustum_visible = frustum_visible && ( dot( scene.data.camera.frustum_planes_culling[ i ], view_center ) > -radius );
  }

  bool occlusion_visible = false;
  if ( frustum_visible )
  {
    occlusion_visible = crude_occlusion_culling(
      mesh_draw_index,
      view_center.xyz, radius, scene.data.camera.znear, 
      scene.data.camera.view_to_clip[ 0 ][ 0 ],
      scene.data.camera.view_to_clip[ 1 ][ 1 ],
      mesh_draw_count.depth_pyramid_texture_index, world_center.xyz,
      scene.data.camera.position,
      scene.data.camera.world_to_clip
    );
  }

  bool is_mesh_opaque = ( ( mesh_draws.data[ mesh_draw_index ].flags & ( CRUDE_DRAW_FLAGS_ALPHA_MASK | CRUDE_DRAW_FLAGS_TRANSPARENT_MASK ) ) == 0 );
#if defined( CULLING_LATE )
  if ( occlusion_visible )
  {
    if ( is_mesh_opaque )
    {
      uint draw_index = atomicAdd( mesh_draw_count.opaque_mesh_visible_late_count, 1 );
      write_mesh_draw_command( mesh_draw_commands.data[ draw_index ], mesh_instance_draw_index, mesh_draw_index );
    }
    else
    {
      uint draw_index = atomicAdd( mesh_draw_count.transparent_mesh_visible_count, 1 ) + mesh_draw_count.total_mesh_count;
      write_mesh_draw_command( mesh_draw_commands.data[ draw_index ], mesh_instance_draw_index, mesh_draw_index );
    }
  }
#else
  if ( occlusion_visible && is_mesh_opaque ) /* Add only opaque objects for early draw */
  {
    uint draw_index = atomicAdd( mesh_draw_count.opaque_mesh_visible_early_count, 1 );
    write_mesh_draw_command( mesh_draw_commands.data[ draw_index ], mesh_instance_draw_index, mesh_draw_index );
  }
  else if ( is_mesh_opaque )
  {
    /* Add culled opaque object for re-test */
    uint draw_index = atomicAdd( mesh_draw_count.opaque_mesh_culled_count, 1 );
    write_mesh_draw_command( mesh_draw_commands_culled.data[ draw_index ], mesh_instance_draw_index, mesh_draw_index );
  }
  else
  {
    /* Add culled transparent object for re-test */
    uint draw_index = atomicAdd( mesh_draw_count.transparent_mesh_culled_count, 1 ) + mesh_draw_count.total_mesh_count;
    write_mesh_draw_command( mesh_draw_commands_culled.data[ draw_index ], mesh_instance_draw_index, mesh_draw_index );
  }
#endif
}
#endif /* CULLING_EARLY || CULLING_LATE */

#if defined( DEPTH_PYRAMID )
layout(set=CRUDE_MATERIAL_SET, binding=0) uniform sampler2D src;
layout(set=CRUDE_MATERIAL_SET, binding=1) uniform writeonly image2D dst;

layout(local_size_x=8, local_size_y=8, local_size_z=1) in;

CRUDE_PUSH_CONSTANT
{
  uint                                                     src_image_index;
  uint                                                     dst_image_index;
  vec2                                                     _padding;
};

void main()
{
  ivec2 src_size = textureSize( global_textures[ nonuniformEXT( src_image_index ) ], 0 );

  if ( all( greaterThan( ivec2( 2 * gl_GlobalInvocationID ), src_size - 2 ) ) )
  {
    return;
  }

  ivec2 texel_position00 = ivec2( gl_GlobalInvocationID.xy ) * 2;
  ivec2 texel_position01 = texel_position00 + ivec2( 0, 1 );
  ivec2 texel_position10 = texel_position00 + ivec2( 1, 0 );
  ivec2 texel_position11 = texel_position00 + ivec2( 1, 1 );

  float color00 = CRUDE_TEXTURE_FETCH( src_image_index, texel_position00, 0 ).r;
  float color01 = CRUDE_TEXTURE_FETCH( src_image_index, texel_position01, 0 ).r;
  float color10 = CRUDE_TEXTURE_FETCH( src_image_index, texel_position10, 0 ).r;
  float color11 = CRUDE_TEXTURE_FETCH( src_image_index, texel_position11, 0 ).r;

  float result = max( max( max( color00, color01 ), color10 ), color11 );

  // !TODO make better
  /* Handle edge case when dimensions aren't divisible by 2, looks awful, but looks like minimal performance impact */
  if ( 2 * gl_GlobalInvocationID.x == src_size.x - 3 )
  {
    ivec2 texel_position20 = texel_position10 + ivec2( 1, 0 );
    ivec2 texel_position21 = texel_position11 + ivec2( 1, 0 );
    float color20 = CRUDE_TEXTURE_FETCH( src_image_index, texel_position20, 0 ).r;
    float color21 = CRUDE_TEXTURE_FETCH( src_image_index, texel_position21, 0 ).r;
    result = max( result, max( color20, color21 ) );
  }
  if ( 2 * gl_GlobalInvocationID.y == src_size.y - 3 )
  {
    ivec2 texel_position02 = texel_position01 + ivec2( 0, 1 );
    ivec2 texel_position12 = texel_position11 + ivec2( 0, 1 );
    float color02 = CRUDE_TEXTURE_FETCH( src_image_index, texel_position02, 0 ).r;
    float color12 = CRUDE_TEXTURE_FETCH( src_image_index, texel_position12, 0 ).r;
    result = max( result, max( color02, color12 ) );
  }
  if ( 2 * gl_GlobalInvocationID.x == src_size.x - 3 && 2 * gl_GlobalInvocationID.y == src_size.y - 3 )
  {
    ivec2 texel_position22 = texel_position11 + ivec2( 1, 1 );
    float color22 = CRUDE_TEXTURE_FETCH( src_image_index, texel_position22, 0 ).r;
    result = max( result, color22 );
  }

  CRUDE_IMAGE_STORE( dst_image_index, ivec2( gl_GlobalInvocationID.xy ), vec4( result, 0, 0, 0 ) );
}
#endif /* DEPTH_PYRAMID */

#if defined( LUMINANCE_AVERAGE_CALCULATION ) || defined( LUMINANCE_HISTOGRAM_GENERATION )

CRUDE_RWBUFFER_REF( HistogramRef )
{
  uint                                                     data[];
};

#endif

#if defined( LUMINANCE_AVERAGE_CALCULATION )
layout(local_size_x=256, local_size_y=1, local_size_z=1) in;

CRUDE_PUSH_CONSTANT
{
  HistogramRef                                             histogram;
  float                                                    log_lum_range;
  float                                                    min_log_lum;

  float                                                    time_coeff;
  uint32                                                   num_pixels;
  uint32                                                   luminance_avarage_texture;
  uint32                                                   _padding;
};

shared uint histogram_shared[ 256 ];

void main()
{
  uint local_index = gl_LocalInvocationIndex;

  uint count_for_this_bin = histogram.data[ local_index ];
  histogram_shared[ local_index ] = count_for_this_bin * local_index;

  barrier( );

  histogram.data[ local_index ] = 0;

  /* Add all histogram_shared values to histogram_shared[0] in O(log(n)) instead of O(n) */ 
  [[unroll]]
  for ( uint cutoff = ( 256 >> 1 ); cutoff > 0; cutoff >>= 1 )
  {
    if ( uint( local_index ) < cutoff )
    {
      histogram_shared[ local_index ] += histogram_shared[ local_index + cutoff ];
    }

    barrier( );
  }

  if ( local_index == 0 )
  {
    float weighted_log_average = ( histogram_shared[ 0 ] / max( num_pixels - float( count_for_this_bin ), 1.0 ) ) - 1.0;

    float weighted_avg_lum = exp2( ( ( weighted_log_average / 254.0 ) * log_lum_range ) + min_log_lum );
    float lum_last_frame = imageLoad( global_images_2d_r32f[ luminance_avarage_texture ], ivec2( 0, 0 ) ).x;
    float adapted_lum = lum_last_frame + ( weighted_avg_lum - lum_last_frame ) * time_coeff;
    imageStore( global_images_2d_r32f[ luminance_avarage_texture ], ivec2( 0, 0 ), vec4( adapted_lum, 0.0, 0.0, 0.0 ) );
  }
}
#endif /* LUMINANCE_AVERAGE_CALCULATION */

#if defined( LUMINANCE_HISTOGRAM_GENERATION )

/* Thanks to https://www.alextardif.com/HistogramLuminance.html */
layout(local_size_x=16, local_size_y=16, local_size_z=1) in;

CRUDE_PUSH_CONSTANT
{
  HistogramRef                                             histogram;
  float                                                    inverse_log_lum_range;
  float                                                    min_log_lum;

  uint                                                     hdr_color_texture_index;
  vec2                                                     _padding;
};

shared uint histogram_shared[ 256 ];

uint hdr_color_to_bin( vec3 color )
{
  float lum = crude_rgb_to_luminance( color );

  if ( lum < 0.005f )
  {
    return 0;
  }

  float log_lum = clamp( ( log2( lum ) - min_log_lum ) * inverse_log_lum_range, 0.0, 1.0 );
  return uint( log_lum * 254.0 + 1.0 );
}

void main()
{
  histogram_shared[ gl_LocalInvocationIndex ] = 0;

  barrier( );
  
  uvec2 hdr_color_texture_size = textureSize( global_textures[ nonuniformEXT( hdr_color_texture_index ) ], 0 ).xy;
  if ( gl_GlobalInvocationID.x < hdr_color_texture_size.x && gl_GlobalInvocationID.y < hdr_color_texture_size.y )
  {
    vec3 hdr_color = CRUDE_TEXTURE_FETCH( hdr_color_texture_index, ivec2( gl_GlobalInvocationID.xy ), 0 ).xyz;
    uint bin_index = hdr_color_to_bin( hdr_color );
    atomicAdd( histogram_shared[ bin_index ], 1 );
  }

  barrier( );
  
  atomicAdd( histogram.data[ gl_LocalInvocationIndex ], histogram_shared[ gl_LocalInvocationIndex ] );
}

#endif /* LUMINANCE_HISTOGRAM_GENERATION */

#if defined( LIGHT_LUT )

// !TODO
layout(local_size_x=8, local_size_y=8, local_size_z=1) in;

void main()
{
}

#endif /* LIGHT_LUT */

#if defined( SSR_HIT_CALCULATION )

layout(local_size_x=8, local_size_y=8, local_size_z=1) in;

CRUDE_PUSH_CONSTANT
{
  SceneRef                                                 scene;
  float                                                    ssr_max_steps;
  float                                                    ssr_max_distance;

  float                                                    ssr_stride;
  float                                                    ssr_z_thickness;
  float                                                    ssr_stride_zcutoff;
  uint                                                     depth_texture_index;

  vec2                                                     depth_texture_size;
  uint                                                     normal_texture_index;
  uint                                                     ssr_hit_uv_depth_rdotv_texture_index;
};

float
distance_squared
(
  in vec2                                                  a,
  in vec2                                                  b
)
{
  a -= b;
  return dot( a, a );
}

bool
intersects_depth_buffer
(
  in float                                                 z,
  in float                                                 min_z,
  in float                                                 max_z
)
{
  /*
   * Based on how far away from the camera the depth is,
   * adding a bit of extra thickness can help improve some
   * artifacts. Driving this value up too high can cause
   * artifacts of its own.
   */
  float depth_scale = min( 1.0f, z * ssr_stride_zcutoff );
  z += ssr_z_thickness + mix( 0.0f, 2.0f, depth_scale );
  return ( max_z >= z ) && ( min_z - ssr_z_thickness <= z );
}

/**
 * Source list:
 * Screen Space Ray Tracing (McGuire Mara, 2014)
 * GPU Pro 5 Hi-ZScreen-SpaceCone-Traced Reflections Yasin Uludag
 * https://willpgfx.com/2015/07/screen-space-glossy-reflections/
 * https://sugulee.wordpress.com/2021/01/16/performance-optimizations-for-screen-space-reflections-technique-part-1-linear-tracing-method/
 */
bool
trace_screen_space_ray
(
  in vec3                                                  origin,
  in vec3                                                  direction,
  in float                                                 jitter,
  out vec2                                                 hit_coords,
  out vec3                                                 hit_point
)
{
  vec4                                                     h0, h1, pqk, dpqk;
  vec3                                                     dq, q, q0, q1, end_point;
  vec2                                                     dp, p0, p1, delta;
  float                                                    dk, k0, k1;
  float                                                    stride, stride_scale;
  float                                                    ray_length, step_dir, invdx;
  float                                                    end, step_count, prev_zmax_estimate, ray_zmin, ray_zmax, scene_zmax;
  bool                                                     permute;

  ray_length = ( ( origin.z + direction.z * ssr_max_distance ) < scene.data.camera.znear ) ? ( scene.data.camera.znear - origin.z ) / direction.z : ssr_max_distance;
  end_point = origin + direction * ray_length;

  h0 = vec4( origin, 1.0f ) * scene.data.camera.view_to_clip; h0.xy *= depth_texture_size;
  h1 = vec4( end_point, 1.0f ) * scene.data.camera.view_to_clip; h1.xy *= depth_texture_size;

  k0 = 1.0f / h0.w;
  k1 = 1.0f / h1.w;

  q0 = origin * k0;
  q1 = end_point * k1;

  p0 = h0.xy * k0;
  p1 = h1.xy * k1;

  /* If the line is degenerate, make it cover at least one pixel to avoid handling zero-pixel extent as a special case later */
  p1 += ( distance_squared( p0, p1 ) < 0.0001f ) ? vec2( 0.01f, 0.01f ) : vec2( 0.0f );
  delta = p1 - p0;

  /* Permute so that the primary iteration is in x to collapse all quadrant-specific DDA cases later */
  permute = false;
  if ( abs( delta.x ) < abs( delta.y ) )
  {
    /* This is a more-vertical line */
    permute = true;
    delta = delta.yx;
    p0 = p0.yx;
    p1 = p1.yx;
  }

  step_dir = sign( delta.x );
  invdx = step_dir / delta.x;

  /* Track the derivatives of Q and k */
  dq = ( q1 - q0 ) * invdx;
  dk = ( k1 - k0 ) * invdx;
  dp = vec2( step_dir, delta.y * invdx );

  /* Scale derivatives by the desired pixel stride and then offset the starting values by the jitter fraction */
  stride_scale = 1.0f - min( 1.0f, origin.z * ssr_stride_zcutoff );
  stride = 1.0f + stride_scale * ssr_stride;
  
  dp *= stride;
  dq *= stride;
  dk *= stride;

  p0 += dp * jitter;
  q0 += dq * jitter;
  k0 += dk * jitter;

  /* Slide P from P0 to P1, (now-homogeneous) Q from Q0 to Q1, k from k0 to k1 */
  pqk = vec4( p0, q0.z, k0 );
  dpqk = vec4( dp, dq.z, dk );
  q = q0; 

  /* Adjust end condition for iteration direction */
  end = p1.x * step_dir;

  step_count = 0.0f;
  prev_zmax_estimate = origin.z;
  ray_zmin = prev_zmax_estimate;
  ray_zmax = prev_zmax_estimate;
  scene_zmax = ray_zmax + 100.0f;
    
  while ( ( ( pqk.x * step_dir ) <= end ) && ( step_count < ssr_max_steps) && !intersects_depth_buffer( scene_zmax, ray_zmin, ray_zmax ) && ( scene_zmax != 0.0f )  )
  {
    ray_zmin = prev_zmax_estimate;
    ray_zmax = ( dq.z * 0.5 + pqk.z ) / ( dpqk.w * 0.5 + pqk.w );

    prev_zmax_estimate = ray_zmax;
    if ( ray_zmin > ray_zmax )
    {
      crude_swap( ray_zmin, ray_zmax);
    }

    hit_coords = permute ? pqk.yx : pqk.xy;
    hit_coords = hit_coords * 0.5 + depth_texture_size * 0.5;
    hit_coords.y = depth_texture_size.y - hit_coords.y;

    scene_zmax = CRUDE_TEXTURE_FETCH(depth_texture_index, ivec2( hit_coords ),0).x;
    scene_zmax = crude_linearize_depth( scene_zmax, scene.data.camera.znear, scene.data.camera.zfar );

    pqk += dpqk;
    ++step_count;
  }

  /* Advance Q based on the number of steps */
  q.xy += dq.xy * step_count;
  hit_point = q * ( 1.0f / pqk.w );
  return intersects_depth_buffer( scene_zmax, ray_zmin, ray_zmax );
}

// !TODO !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
// DEPTH PYRAMID USING LINKE IN GPU PRO 5
void main()
{
  ivec2 coords = ivec2( gl_GlobalInvocationID.xy );
  float depth = CRUDE_TEXTURE_FETCH( depth_texture_index, coords, 0 ).r;
  vec2 packed_normal = CRUDE_TEXTURE_FETCH( normal_texture_index, coords, 0 ).xy;
  vec2 pixel_uv = crude_uv_nearest( coords, scene.data.resolution );

  vec3 pixel_vs = crude_world_position_from_depth( pixel_uv, depth, scene.data.camera.clip_to_view );
  vec3 normal_vs = crude_octahedral_decode( packed_normal ) * mat3( scene.data.camera.world_to_view );

  vec3 to_ray_origin = normalize( pixel_vs );
  vec3 ray_origin = pixel_vs;
  vec3 ray_direction = reflect( to_ray_origin, normal_vs );

  float rdotv = dot( ray_direction, to_ray_origin );

  vec2 hit_coords = vec2(0.0f, 0.0f);
  vec3 hit_point = vec3(0.0f, 0.0f, 0.0f);

  float jitter = ssr_stride > 1.0f ? float( int( coords.x + coords.y ) & 1) * 0.5f : 0.0f;

  // perform ray tracing - true if hit found, false otherwise
  bool intersection = trace_screen_space_ray( ray_origin, ray_direction, jitter, hit_coords, hit_point );

  vec2 hit_uv = hit_coords / depth_texture_size;
  if( hit_uv.x > 1 || hit_uv.x < 0.0 || hit_uv.y > 1 || hit_uv.y < 0.0 )
  {
    intersection = false;
  }

  depth = CRUDE_TEXTURE_FETCH( depth_texture_index, ivec2( hit_coords ), 0 ).r;
  
  CRUDE_IMAGE_STORE( ssr_hit_uv_depth_rdotv_texture_index, coords, vec4( hit_uv, depth, max( rdotv, 0 ) ) * ( intersection ? 1.0f : 0.0f ) );
  
}

#endif /* SSR_HIT_CALCULATION */

#if defined( SSR_CONVOLVE_VERTICAL ) || defined( SSR_CONVOLVE_HORIZONTAL )

layout(local_size_x=8, local_size_y=8, local_size_z=1) in;

#if defined( SSR_CONVOLVE_HORIZONTAL )
const ivec2 offsets[7] = {{-3, 0}, {-2, 0}, {-1, 0}, {0, 0}, {1, 0}, {2, 0}, {3, 0}};
#elif defined( SSR_CONVOLVE_VERTICAL ) 
const ivec2 offsets[ 7 ] =  {{0, -3}, {0, -2}, {0, -1}, {0, 0}, {0, 1}, {0, 2}, {0, 3}};
#endif
const float weights[7] = {0.001f, 0.028f, 0.233f, 0.474f, 0.233f, 0.028f, 0.001f};

CRUDE_PUSH_CONSTANT
{
  uint                                                     src_texture_index;
  uint                                                     dst_texture_index;
  vec2                                                     src_div_dst_texture_size; /* src_size / dst_size */
};

void main()
{
  ivec2 dst_coords = ivec2( gl_GlobalInvocationID.xy );
  ivec2 src_coords = ivec2( dst_coords * src_div_dst_texture_size );

  vec4 color = vec4( 0.0f, 0.0f, 0.0f, 1.0f );

  #pragma unroll
  for( uint i = 0u; i < 7u; ++i)
  {
    color += CRUDE_TEXTURE_FETCH( src_texture_index, ivec2( src_coords + offsets[ i ] ), 0 ) * weights[ i ];
  }

  CRUDE_IMAGE_STORE( dst_texture_index, dst_coords, vec4( color.xyz, 1.0f ) );
}

#endif

#if defined( SSR_COMPOSE )

#define CNST_MAX_SPECULAR_EXP 1024 // TODO ?

layout(local_size_x=8, local_size_y=8, local_size_z=1) in;

float
specular_power_to_cone_angle
(
  in float                                                 specular_power
)
{
  if ( specular_power >= exp2( CNST_MAX_SPECULAR_EXP ) )
  {
    return 0.0f;
  }
  
  const float xi = 0.244f;
  float exponent = 1.0f / ( specular_power + 1.0f );
  return acos( pow( xi, exponent ));
}

float
isosceles_triangle_opposite
(
  in float                                                 adjacent_length,
  in float                                                 cone_theta
)
{
  return 2.0f * tan( cone_theta ) * adjacent_length;
}

float
isosceles_triangle_in_radius
(
  in float                                                 a,
  in float                                                 h
)
{
  float a2 = a * a;
  float fh2 = 4.0f * h * h;
  return ( a * ( sqrt( a2 + fh2 ) - a ) ) / ( 4.0f * h );
}

float
isosceles_triangle_next_adjacent
(
  in float                                                 adjacent_length,
  in float                                                 incircle_radius
)
{
  return adjacent_length - ( incircle_radius * 2.0f );
}

vec4
cone_sample_weighted_color
(
  in uint                                                  radiance_texture_index,
  in vec2                                                  sample_pos,
  in float                                                 mip_channel,
  in float                                                 gloss
)
{
  // TODO IT HAVE TO HAVE sampTrilinearClamp SAMPLER!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
  vec3 sample_radiance = CRUDE_TEXTURE_LOD( radiance_texture_index, sample_pos, mip_channel ).rgb;
  return vec4( sample_radiance * gloss, gloss );
}
CRUDE_PUSH_CONSTANT
{
  SceneRef                                                 scene;
  uint                                                     ssr_hit_uv_depth_rdotv_texture_index;
  uint                                                     output_texture_index;
  uint                                                     depth_texture_index;
  uint                                                     color_texture_index;
  vec2                                                     depth_texture_size;
  uint                                                     packed_roughness_metalness;
  uint                                                     cb_numMips;
  float                                                    cb_fadeEnd;
  float                                                    cb_fadeStart;
  float                                                    cb_maxDistance;








  float                                                    ssr_max_steps;
  float                                                    ssr_max_distance;

  float                                                    ssr_stride;
  float                                                    ssr_z_thickness;
  float                                                    ssr_stride_zcutoff;

  uint                                                     normal_texture_index;
  uint                                                     ssr_texture_index;
};

void main()
{

///////////////////////////////////////////////////////////////////////////////////////

  ivec2 coords = ivec2( gl_GlobalInvocationID.xy );
  
  vec4 ssr_data_packed = CRUDE_TEXTURE_FETCH( ssr_hit_uv_depth_rdotv_texture_index, coords, 0 ).xyzw;
  float depth = CRUDE_TEXTURE_FETCH( depth_texture_index, coords, 0 ).r;
  vec2 packed_normal = CRUDE_TEXTURE_FETCH( normal_texture_index, coords, 0 ).xy;

  vec2 ssr_hit_uv = ssr_data_packed.xy;
  float ssr_hit_depth = ssr_data_packed.z;
  float ssr_hit_rdotv = ssr_data_packed.w;
  vec3 fallback_color = vec3( 0, 0, 0 );// indirectSpecularBuffer.Load(loadIndices).rgb;

  /* either means no hit or the ray faces back towards the camera */
  if ( ssr_hit_rdotv <= 0.0f )
  {
    CRUDE_IMAGE_STORE( output_texture_index, coords, vec4( fallback_color, 1.0f ) );
    return;
  }

  vec2 screen_uv = crude_uv_nearest( coords, scene.data.resolution );
  vec3 position_ss = vec3( screen_uv, depth );
  float linear_depth = crude_linearize_depth( depth, scene.data.camera.znear, scene.data.camera.zfar );
  vec3 pixel_vs = crude_world_position_from_depth( screen_uv, depth, scene.data.camera.clip_to_view );
  vec3 to_position_vs = normalize( pixel_vs );
  vec3 normal_vs = crude_octahedral_decode( packed_normal ) * mat3( scene.data.camera.world_to_view );
  float roughness = CRUDE_TEXTURE_FETCH( packed_roughness_metalness, coords, 0 ).x;

  float gloss = 1.0f - roughness;
  float specular_power = roughness; //roughnessToSpecularPower(roughness);

  float cone_theta = specular_power_to_cone_angle( specular_power ) * 0.5f;

  vec2 delta_uv = ssr_hit_uv - screen_uv;
  float adjacent_length = length( delta_uv );
  vec2 adjacent_unit = normalize( delta_uv );

  vec4 total_color = vec4( 0.0f, 0.0f, 0.0f, 0.0f );
  float remaining_alpha = 1.0f;
  float max_mip_level = float( cb_numMips ) - 1.0f;

  float gloss_mult = gloss;
  
  for ( int i = 0; i < 14; ++i )
  {
    float opposite_length = isosceles_triangle_opposite( adjacent_length, cone_theta );
    float incircle_size = isosceles_triangle_in_radius( opposite_length, adjacent_length );

    vec2 sample_pos = screen_uv + adjacent_unit * ( adjacent_length - incircle_size );
    float mip_channel = clamp( log2( incircle_size * max( depth_texture_size.x, depth_texture_size.y ) ), 0.0f, max_mip_level );

    vec4 new_color = cone_sample_weighted_color(color_texture_index, sample_pos, mip_channel, gloss_mult );

    remaining_alpha -= new_color.a;
    if ( remaining_alpha < 0.0f )
    {
      new_color.rgb *= ( 1.0f - abs( remaining_alpha ) );
    }
    total_color += new_color;

    if ( total_color.a >= 1.0f )
    {
      break;
    }

    adjacent_length = isosceles_triangle_next_adjacent( adjacent_length, incircle_size );
    gloss_mult *= gloss;
  }

  vec3 to_eye = -to_position_vs;
  vec3 specular = vec3(0);//calculateFresnelTerm(CRUDE_DEAFULT_F0, abs(dot(normal, toEye))) * CNST_1DIVPI;

  // fade rays close to screen edge
  vec2 boundary = abs( ssr_hit_uv.xy - vec2( 0.5f, 0.5f ) ) * 2.0f;
  const float fade_diff_rcp = 1.0f / ( cb_fadeEnd - cb_fadeStart );
  float fade_on_border = 1.0f - CRUDE_SATURATE( ( boundary.x - cb_fadeStart ) * fade_diff_rcp );
  fade_on_border *= 1.0f - CRUDE_SATURATE( ( boundary.y - cb_fadeStart ) * fade_diff_rcp );
  fade_on_border = smoothstep( 0.0f, 1.0f, fade_on_border );
  
  vec3 ray_hit_position_vs = vec3(0);//viewSpacePositionFromDepth(raySS.xy, raySS.z);
  float fade_on_distance = 1.0f - CRUDE_SATURATE( distance( ray_hit_position_vs, pixel_vs ) / cb_maxDistance );
  float fade_on_perpendicular = CRUDE_SATURATE( mix( 0.0f, 1.0f, CRUDE_SATURATE( ssr_hit_rdotv * 4.0f ) ) );
  float fade_on_roughness = CRUDE_SATURATE( mix( 0.0f, 1.0f, gloss * 4.0f ) );
  float totalFade = fade_on_border * fade_on_distance * fade_on_perpendicular * fade_on_roughness * ( 1.0f - CRUDE_SATURATE( remaining_alpha ) );

  CRUDE_IMAGE_STORE( output_texture_index, coords, vec4( mix( fallback_color, total_color.rgb * specular, totalFade ), 1.0f ) );
}

#endif /* SSR_COMPOSE */