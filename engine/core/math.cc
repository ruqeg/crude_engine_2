#include <stdlib.h>
#include <thirdparty/cJSON/cJSON.h>

#include <engine/core/memory.h>
#include <engine/core/math.h>
#include <engine/core/assert.h>

XMVECTOR
crude_quaternion_to_pitch_yaw_roll
(
  _In_ FXMVECTOR                                           quaternion
)
{
  XMVECTOR nq = XMQuaternionNormalize( quaternion );
  return XMVectorSet(
    atan2( 2.0f * ( XMVectorGetY( nq ) * XMVectorGetZ( nq ) + XMVectorGetX( nq ) * XMVectorGetW( nq ) ), 1.0f - 2.0f * ( XMVectorGetY( nq ) * XMVectorGetY( nq ) + XMVectorGetZ( nq ) * XMVectorGetZ( nq ) ) ),
    asin( 2.0f * ( XMVectorGetY( nq ) * XMVectorGetW( nq ) - XMVectorGetX( nq ) * XMVectorGetZ( nq ) ) ),
    atan2( 2.0f * ( XMVectorGetX( nq ) * XMVectorGetY( nq ) + XMVectorGetZ( nq ) * XMVectorGetW( nq ) ), 1.0f - 2.0f * ( XMVectorGetX( nq ) * XMVectorGetX( nq ) + XMVectorGetZ( nq ) * XMVectorGetZ( nq ) ) ),
    1.f
  );
}

float32
crude_random_unit_f32
(
)
{
  return CRUDE_CAST( float32, ( rand( ) - RAND_MAX / 2 ) ) / ( RAND_MAX / 2 );
}

float32
crude_lerp_angle
(
  _In_ float32                                             from,
  _In_ float32                                             to,
  _In_ float32                                             weight
)
{
  float32 difference = fmod( to - from, XM_2PI );
  float32 distance = fmod( 2.0 * difference, XM_2PI ) - difference;
  return from + distance * weight;
}

float32
crude_angle_diff
(
  _In_ float32                                             from,
  _In_ float32                                             to
)
{
  float32 diff = fmodf( to - from, 2.f * XM_PI );
  if ( diff > XM_PI )
  {
    diff -= 2.f * XM_PI;
  }
  else if ( diff < -XM_PI )
  {
    diff += 2.f * XM_PI;
  }
  return diff;
}

XMVECTOR
crude_closest_point_to_obb
(
  _In_ XMVECTOR                                             point,
  _In_ XMVECTOR                                             obb_position,
  _In_ XMVECTOR                                             obb_size,
  _In_ XMMATRIX                                             obb_orientation
)
{
  XMVECTOR result = obb_position;
  XMVECTOR dir = XMVectorSubtract( point, obb_position );

  for ( uint32 i = 0; i < 3; ++i )
  {
    XMVECTOR                                               axis;
    float32                                                length;
    float32                                                distance;

    axis = XMVector3Normalize( obb_orientation.r[ i ] );
    distance = XMVectorGetX( XMVector3Dot( dir, axis ) );
    length = XMVectorGetByIndex( obb_size, i );

    if ( distance > length )
    {
      distance = length;
    }

    if ( distance < -length )
    {
      distance = -length;
    }

    result = XMVectorAdd( result, XMVectorScale( axis, distance ) );
  }

  return result;
}

bool
crude_intersection_sphere_obb
(
  _In_ XMVECTOR                                             closest_point,
  _In_ XMVECTOR                                             sphere_position,
  _In_ float32                                              sphere_radius
)
{
  return XMVectorGetX( XMVector3LengthSq( sphere_position - closest_point ) ) < sphere_radius * sphere_radius;
}

XMVECTOR
crude_project_vector3
(
  _In_ XMVECTOR                                            length,
  _In_ XMVECTOR                                            direction
)
{
  float32 dot = XMVectorGetX( XMVector3Dot( length, direction ) );
  float32 mag_sq = XMVectorGetX( XMVector3LengthSq( direction ) );
  return direction * ( dot / mag_sq );
}

XMVECTOR
crude_barycentric
(
  _In_ XMVECTOR                                             p,
  _In_ XMVECTOR                                             ta,
  _In_ XMVECTOR                                             tb,
  _In_ XMVECTOR                                             tc
)
{ 
  XMVECTOR ap = XMVectorSubtract( p, ta );
  XMVECTOR bp = XMVectorSubtract( p, tb );
  XMVECTOR cp = XMVectorSubtract( p, tc );

  XMVECTOR ab = XMVectorSubtract( tb, ta );
  XMVECTOR ac = XMVectorSubtract( tc, ta );

  XMVECTOR bc = XMVectorSubtract( tc, tb );
  XMVECTOR cb = XMVectorSubtract( tb, tc );
  XMVECTOR ca = XMVectorSubtract( ta, tc );
  
  XMVECTOR v = XMVectorSubtract( ab, crude_project_vector3( ab, cb ) );
  float32 a = 1.0f - ( XMVectorGetX( XMVector3Dot( v, ap ) ) / XMVectorGetX( XMVector3Dot( v, ab ) ) );

  v = XMVectorSubtract( bc, crude_project_vector3( bc, ac ) );
  float32 b = 1.0f - ( XMVectorGetX( XMVector3Dot( v, bp ) / XMVectorGetX( XMVector3Dot( v, bc ) ) ) );

  v = XMVectorSubtract( ca, crude_project_vector3( ca, ab ) );
  float32 c = 1.0f - ( XMVectorGetX( XMVector3Dot( v, cp ) / XMVectorGetX( XMVector3Dot( v, ca ) ) ) );

  return XMVectorSet( a, b, c, 0 );
}

XMVECTOR
crude_closest_point_to_plane
(
  _In_ XMVECTOR                                            plane,
  _In_ XMVECTOR                                            point
)
{
  float32 distance = XMVectorGetX( XMVector3Dot( plane, point ) ) - XMVectorGetW( plane );
  return XMVectorSubtract( point, XMVectorScale( plane, distance ) );
}

bool
crude_point_in_triangle
(
  _In_ XMVECTOR                                             t0,
  _In_ XMVECTOR                                             t1,
  _In_ XMVECTOR                                             t2,
  _In_ XMVECTOR                                             p
)
{
  XMVECTOR a = t0 - p;
  XMVECTOR b = t1 - p;
  XMVECTOR c = t2 - p;
  
  XMVECTOR norm_pbc = XMVector3Cross( b, c );
  XMVECTOR norm_pca = XMVector3Cross( c, a );
  XMVECTOR norm_pab = XMVector3Cross( a, b );
  
  if ( XMVectorGetX( XMVector3Dot( norm_pbc, norm_pca ) ) < 0.f )
  {
    return false;
  }
  else if ( XMVectorGetX( XMVector3Dot( norm_pbc, norm_pab ) ) < 0.f )
  {
    return false;
  }
  
  return true;
}

XMVECTOR
crude_closest_point_to_line
(
  _In_ XMVECTOR                                             lstart,
  _In_ XMVECTOR                                             lend,
  _In_ XMVECTOR                                             p
)
{
  XMVECTOR lvec = XMVectorSubtract( lend, lstart );
  float32 t = XMVectorGetX( XMVector3Dot( p - lstart, lvec ) ) / XMVectorGetX( XMVector3Dot( lvec, lvec ) );
  t = CRUDE_MAX( t, 0.f );
  t = CRUDE_MIN( t, 1.f );
  return XMVectorAdd( lstart, XMVectorScale( lvec, t ) );
}

XMVECTOR
crude_plane_from_points
(
  _In_ XMVECTOR                                             p0,
  _In_ XMVECTOR                                             p1,
  _In_ XMVECTOR                                             p2
)
{
  XMVECTOR n = XMVector3Normalize( XMVector3Cross( XMVectorSubtract( p1, p0 ), XMVectorSubtract( p2, p0 ) ) );
  XMVECTOR d = XMVector3Dot( n, p0 );
  return XMVectorSelect( d, n, g_XMSelect1110.v);
}

XMVECTOR
crude_closest_point_to_triangle
(
  _In_ XMVECTOR                                             t0,
  _In_ XMVECTOR                                             t1,
  _In_ XMVECTOR                                             t2,
  _In_ XMVECTOR                                             p
)
{
  XMVECTOR plane = crude_plane_from_points( t0, t1, t2 );
  XMVECTOR closest = crude_closest_point_to_plane( plane, p );
  
  if ( crude_point_in_triangle( t0, t1, t2, p ) )
  {
    return closest;
  }
  
  XMVECTOR c1 = crude_closest_point_to_line( t0, t1, closest );
  XMVECTOR c2 = crude_closest_point_to_line( t1, t2, closest );
  XMVECTOR c3 = crude_closest_point_to_line( t2, t0, closest );
  
  float32 mag_sq1 = XMVectorGetX( XMVector3LengthSq( closest - c1 ) );
  float32 mag_sq2 = XMVectorGetX( XMVector3LengthSq( closest - c2 ) );
  float32 mag_sq3 = XMVectorGetX( XMVector3LengthSq( closest - c3 ) );
  
  if ( mag_sq1 < mag_sq2 && mag_sq1 < mag_sq3 )
  {
    return c1;
  }
  else if ( mag_sq2 < mag_sq1 && mag_sq2 < mag_sq3 )
  {
    return c2;
  }
  
  return c3;
}

bool
crude_intersection_sphere_triangle
(
  _In_ XMVECTOR                                             closest_point,
  _In_ XMVECTOR                                             sphere_position,
  _In_ float32                                              sphere_radius
)
{
  return XMVectorGetX( XMVector3LengthSq( sphere_position - closest_point ) ) < sphere_radius * sphere_radius;
}

// !TODO why can't I just use world_to_clip? Lazy to think, i'll do it lated
XMVECTOR
crude_compute_projected_sphere_aabb
(
  _In_ XMVECTOR                                            light_world_position,
  _In_ float32                                             light_radius,
  _In_ XMMATRIX                                            camera_world_to_view,
  _In_ XMMATRIX                                            camera_view_to_clip,
  _In_ float32                                             camera_near_z
)
{
  XMVECTOR                                                 aabb_min, aabb_max;
  
  aabb_min = XMVectorSet( FLT_MAX, FLT_MAX, FLT_MAX, 0 );
  aabb_max = XMVectorSet( -FLT_MAX, -FLT_MAX, -FLT_MAX, 0 );
  
  for ( uint32 c = 0; c < 8; ++c )
  {
    XMVECTOR                                       corner, corner_vs, corner_ndc;
  
    corner = XMVectorSet( ( c % 2 ) ? 1.f : -1.f, ( c & 2 ) ? 1.f : -1.f, ( c & 4 ) ? 1.f : -1.f, 1 );
    corner = XMVectorScale( corner, light_radius );
    corner = XMVectorAdd( corner, light_world_position );
    corner = XMVectorSetW( corner, 1.f );
  
    corner_vs = XMVector4Transform( corner, camera_world_to_view );
#if CRUDE_RIGHT_HAND
    corner_vs = XMVectorSetZ( corner_vs, -1.f * XMVectorGetZ( corner_vs ) );
#endif
    corner_vs = XMVectorSetZ( corner_vs, CRUDE_MAX( camera_near_z, XMVectorGetZ( corner_vs ) ) );
    corner_ndc = XMVector4Transform( corner_vs, camera_view_to_clip );
    corner_ndc = XMVectorScale( corner_ndc, 1.f / XMVectorGetW( corner_ndc ) );
  
    aabb_min = XMVectorMin( aabb_min, corner_ndc );
    aabb_max = XMVectorMax( aabb_max, corner_ndc );
  }
  
  return XMVectorSet( XMVectorGetX( aabb_min ), -1.f * XMVectorGetY( aabb_max ), XMVectorGetX( aabb_max ), -1.f * XMVectorGetY( aabb_min ) );
}

void
crude_parse_json_to_float2
(
  _Out_ XMFLOAT2                                          *float2,
  _In_ cJSON                                              *json
)
{
  CRUDE_ASSERT( cJSON_GetArraySize( json ) == 2 );

  float2->x = CRUDE_STATIC_CAST( float32, cJSON_GetNumberValue( cJSON_GetArrayItem( json, 0 ) ) );
  float2->y = CRUDE_STATIC_CAST( float32, cJSON_GetNumberValue( cJSON_GetArrayItem( json, 1 ) ) );
}

void
crude_parse_json_to_float3
(
  _Out_ XMFLOAT3                                          *float3,
  _In_ cJSON                                              *json
)
{
  CRUDE_ASSERT( cJSON_GetArraySize( json ) == 3 );

  float3->x = CRUDE_STATIC_CAST( float32, cJSON_GetNumberValue( cJSON_GetArrayItem( json, 0 ) ) );
  float3->y = CRUDE_STATIC_CAST( float32, cJSON_GetNumberValue( cJSON_GetArrayItem( json, 1 ) ) );
  float3->z = CRUDE_STATIC_CAST( float32, cJSON_GetNumberValue( cJSON_GetArrayItem( json, 2 ) ) );
}

void
crude_parse_json_to_float4
(
  _Out_ XMFLOAT4                                          *float4,
  _In_ cJSON                                              *json
)
{
  CRUDE_ASSERT( cJSON_GetArraySize( json ) == 4 );

  float4->x = CRUDE_STATIC_CAST( float32, cJSON_GetNumberValue( cJSON_GetArrayItem( json, 0 ) ) );
  float4->y = CRUDE_STATIC_CAST( float32, cJSON_GetNumberValue( cJSON_GetArrayItem( json, 1 ) ) );
  float4->z = CRUDE_STATIC_CAST( float32, cJSON_GetNumberValue( cJSON_GetArrayItem( json, 2 ) ) );
  float4->w = CRUDE_STATIC_CAST( float32, cJSON_GetNumberValue( cJSON_GetArrayItem( json, 3 ) ) );
}