
#ifdef CRUDE_VALIDATOR_LINTING
#extension GL_GOOGLE_include_directive : enable
//#define LUMINANCE_AVERAGE_CALCULATION
#define LUMINANCE_HISTOGRAM_GENERATION
//#define CULLING_EARLY
//#define CULLING_LATE
//#define DEPTH_PYRAMID
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
  if ( occlusion_visible )
  {
    if ( is_mesh_opaque )
    {
      uint draw_index = atomicAdd( mesh_draw_count.opaque_mesh_visible_early_count, 1 );
      write_mesh_draw_command( mesh_draw_commands.data[ draw_index ], mesh_instance_draw_index, mesh_draw_index );
    }
    else
    {
      uint draw_index = atomicAdd( mesh_draw_count.transparent_mesh_visible_count, 1 ) + mesh_draw_count.total_mesh_count;
      write_mesh_draw_command( mesh_draw_commands.data[ draw_index ], mesh_instance_draw_index, mesh_draw_index );
    }
  }
  else
  {
    if ( is_mesh_opaque )
    {
      /* Add culled object for re-test */
      uint draw_index = atomicAdd( mesh_draw_count.opaque_mesh_culled_count, 1 );
      write_mesh_draw_command( mesh_draw_commands_culled.data[ draw_index ], mesh_instance_draw_index, mesh_draw_index );
    }
    else
    {
      /* Add culled object for re-test */
      uint draw_index = atomicAdd( mesh_draw_count.transparent_mesh_culled_count, 1 ) + mesh_draw_count.total_mesh_count;
      write_mesh_draw_command( mesh_draw_commands_culled.data[ draw_index ], mesh_instance_draw_index, mesh_draw_index );
    }
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