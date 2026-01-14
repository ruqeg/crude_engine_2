
#ifndef CRUDE_CULLING_GLSL
#define CRUDE_CULLING_GLSL

bool
crude_clustered_backface_culling
(
  in vec3                                                  center,
  in float                                                 radius,
  in vec3                                                  cone_axis,
  in float                                                 cone_cutoff,
  in vec3                                                  camera_pos
)
{
  return dot( center - camera_pos, cone_axis ) >= cone_cutoff * length( center - camera_pos ) + radius;
}

bool
crude_sphere_intersect
(
  in vec3                                                  center_a,
  in float                                                 radius_a,
  in vec3                                                  center_b,
  in float                                                 radius_b
)
{
  vec3 v = center_b - center_a;
  float total_radius = radius_a + radius_b;
  return dot( v, v ) < total_radius * total_radius;
}

/* 2D Polyhedral Bounds of a Clipped, Perspective-Projected 3D Sphere. Michael Mara, Morgan McGuire. 2013 */
bool
crude_bounding_sphere_to_clipped_aabb
(
  in vec3                                                  c,
  in float                                                 r,
  in float                                                 znear,
  in float                                                 p00,
  in float                                                 p11,
  out vec4                                                 aabb
)
{
  vec2                                                     cx, vx, minx, maxx, cy, vy, miny, maxy;
  if ( c.z - r < znear )
  {
    return false;
  }

  cx = vec2( c.x, c.z );
  vx = vec2( sqrt( dot( cx, cx ) - r * r ), r );
  minx = mat2( vx.x, vx.y, -vx.y, vx.x ) * cx;
  maxx = mat2( vx.x, -vx.y, vx.y, vx.x ) * cx;

  cy = vec2( -c.y, c.z );
  vy = vec2( sqrt( dot( cy, cy ) - r * r ), r );
  miny = mat2( vy.x, vy.y, -vy.y, vy.x ) * cy;
  maxy = mat2( vy.x, -vy.y, vy.y, vy.x ) * cy;

  aabb = vec4( minx.x / minx.y * p00, miny.x / miny.y * p11, maxx.x / maxx.y * p00, maxy.x / maxy.y * p11 );

  return true;
}

vec4
crude_clip_to_uv_space
(
  in vec4                                                  v
)
{
  return v.xwzy * vec4( 0.5f, -0.5f, 0.5f, -0.5f ) + vec4( 0.5f );
}

/*
 * Occlusion Culling
 * TODO:
 *  Continue testing using a finer mip level if result are ambiguous
 */
bool
crude_occlusion_culling
(
  in uint                                                  mesh_instance_draw_index,
  in vec3                                                  view_bounding_center,
  in float                                                 radius,
  in float                                                 znear,
  in float                                                 projection_00,
  in float                                                 projection_11,
  in uint                                                  depth_pyramid_texture_index,
  in vec3                                                  world_bounding_center,
  in vec3                                                  camera_world_position,
  in mat4                                                  culling_view_projection
)
{
  vec4 aabb_clip;
  bool occlusion_visible = true;
  if ( crude_bounding_sphere_to_clipped_aabb( view_bounding_center, radius, znear, projection_00, projection_11, aabb_clip ) )
  {
    vec4 aabb = crude_clip_to_uv_space( aabb_clip );

    ivec2 depth_pyramid_size = textureSize( global_textures[ nonuniformEXT( depth_pyramid_texture_index ) ], 0 );
    float width = ( aabb.z - aabb.x ) * depth_pyramid_size.x;
    float height = ( aabb.w - aabb.y ) * depth_pyramid_size.y;

    float level = floor( log2( max( width, height ) ) );

    vec2 uv = ( aabb.xy + aabb.zw ) * 0.5;
    uv.y = 1 - uv.y;

    float depth = textureLod( global_textures[ nonuniformEXT( depth_pyramid_texture_index ) ], uv, level ).r;
    depth = max( depth, textureLod( global_textures[ nonuniformEXT( depth_pyramid_texture_index ) ], vec2( aabb.x, 1.0f - aabb.y ), level ).r );
    depth = max( depth, textureLod( global_textures[ nonuniformEXT( depth_pyramid_texture_index ) ], vec2( aabb.z, 1.0f - aabb.w ), level ).r );
    depth = max( depth, textureLod( global_textures[ nonuniformEXT( depth_pyramid_texture_index ) ], vec2( aabb.x, 1.0f - aabb.w ), level ).r );
    depth = max( depth, textureLod( global_textures[ nonuniformEXT( depth_pyramid_texture_index ) ], vec2( aabb.z, 1.0f - aabb.y ), level ).r );

    vec3 dir = normalize( camera_world_position - world_bounding_center );
    
    vec4 sceen_space_center_last = vec4( world_bounding_center + dir * radius, 1.0 ) * culling_view_projection;
    float depth_sphere = sceen_space_center_last.z / sceen_space_center_last.w;
    occlusion_visible = ( depth_sphere <= depth );
  }

  return occlusion_visible;
}

uint
crude_get_cube_face_mask
(
  in vec3                                                  cubemap_pos,
  in vec3                                                  aabb_min,
  in vec3                                                  aabb_max
)
{
  vec3 plane_normals[] = { vec3( -1, 1, 0 ), vec3( 1, 1, 0 ), vec3( 1, 0, 1 ), vec3( 1, 0, -1 ), vec3( 0, 1, 1 ), vec3( 0, -1, 1 ) };
  vec3 abs_plane_normals[] = { vec3( 1, 1, 0 ), vec3( 1, 1, 0 ), vec3( 1, 0, 1 ), vec3( 1, 0, 1 ), vec3( 0, 1, 1 ), vec3( 0, 1, 1 ) };

  vec3 aabb_center = ( aabb_min + aabb_max ) * 0.5f;

  vec3 center = aabb_center - cubemap_pos;
  vec3 extents = ( aabb_max - aabb_min ) * 0.5f;

  bool rp[ 6 ];
  bool rn[ 6 ];

  for ( uint i = 0; i < 6; ++i )
  {
    float dist = dot( center, plane_normals[ i ] );
    float radius = dot( extents, abs_plane_normals[ i ] );

    rp[ i ] = dist > -radius;
    rn[ i ] = dist < radius;
  }

  uint fpx = ( rn[ 0 ] && rp[ 1 ] && rp[ 2 ] && rp[ 3 ] && aabb_max.x > cubemap_pos.x ) ? 1 : 0;
  uint fnx = ( rp[ 0 ] && rn[ 1 ] && rn[ 2 ] && rn[ 3 ] && aabb_min.x < cubemap_pos.x ) ? 1 : 0;
  uint fpy = ( rp[ 0 ] && rp[ 1 ] && rp[ 4 ] && rn[ 5 ] && aabb_max.y > cubemap_pos.y ) ? 1 : 0;
  uint fny = ( rn[ 0 ] && rn[ 1 ] && rn[ 4 ] && rp[ 5 ] && aabb_min.y < cubemap_pos.y ) ? 1 : 0;
  uint fpz = ( rp[ 2 ] && rn[ 3 ] && rp[ 4 ] && rp[ 5 ] && aabb_max.z > cubemap_pos.z ) ? 1 : 0;
  uint fnz = ( rn[ 2 ] && rp[ 3 ] && rn[ 4 ] && rn[ 5 ] && aabb_min.z < cubemap_pos.z ) ? 1 : 0;

  return fpx | ( fnx << 1 ) | ( fpy << 2 ) | ( fny << 3 ) | ( fpz << 4 ) | ( fnz << 5 );
}

#endif /* CRUDE_CULLING_GLSL */