#include <stdlib.h>
#include <cJSON.h>

#include <core/memory.h>
#include <core/math.h>
#include <core/assert.h>

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

float32
crude_raycast_obb
(
  _In_ XMVECTOR                                             ray_origin,
  _In_ XMVECTOR                                             ray_direction,
  _In_ XMVECTOR                                             obb_position,
  _In_ XMVECTOR                                             obb_size,
  _In_ XMMATRIX                                             obb_orientation,
  _Out_opt_ crude_raycast_result                           *result
)
{
  float32                                                  t[ 6 ];
  XMVECTOR                                                 x_axis, y_axis, z_axis, p, f, e;
  float32                                                  tmin, tmax;
  
  if ( result )
  {
    *result = crude_raycast_result_empty( );
  }

  crude_memory_set( t, 0, sizeof( t ) );

  x_axis = XMVector3Normalize( obb_orientation.r[ 0 ] );
  y_axis = XMVector3Normalize( obb_orientation.r[ 1 ] );
  z_axis = XMVector3Normalize( obb_orientation.r[ 2 ] );
  p = XMVectorSubtract( obb_position, ray_origin );
  f = XMVectorSet( XMVectorGetX( XMVector3Dot( x_axis, ray_direction ) ), XMVectorGetX( XMVector3Dot( y_axis, ray_direction ) ), XMVectorGetX( XMVector3Dot( z_axis, ray_direction ) ), 1 );
  e = XMVectorSet( XMVectorGetX( XMVector3Dot( x_axis, p ) ), XMVectorGetX( XMVector3Dot( y_axis, p ) ), XMVectorGetX( XMVector3Dot( z_axis, p ) ), 1 );
  
  for ( int32 i = 0; i < 3; ++i)
  {
    if ( fabsf( XMVectorGetByIndex( f, i ) ) < 0.00001f )
    {
      if ( -XMVectorGetByIndex( e, i ) - XMVectorGetByIndex( obb_size, i ) > 0 || -XMVectorGetByIndex( e, i ) + XMVectorGetByIndex( obb_size, i ) < 0 )
      {
        return false;
      }
      XMVectorSetByIndex( f, 0.00001f, i );
    }

    t[ i * 2 + 0 ] = ( XMVectorGetByIndex( e, i ) + XMVectorGetByIndex( obb_size, i ) ) / XMVectorGetByIndex( f, i );
    t[ i * 2 + 1 ] = ( XMVectorGetByIndex( e, i ) - XMVectorGetByIndex( obb_size, i ) ) / XMVectorGetByIndex( f, i );
  }

  tmin = CRUDE_MAX( CRUDE_MAX( CRUDE_MIN( t[ 0 ], t[ 1 ] ), CRUDE_MIN( t[ 2 ], t[ 3 ] ) ), CRUDE_MIN( t[ 4 ], t[ 5 ] ) );
  tmax = CRUDE_MIN( CRUDE_MIN( CRUDE_MAX( t[ 0 ], t[ 1 ] ), CRUDE_MAX( t[ 2 ], t[ 3 ] ) ), CRUDE_MAX( t[ 4 ], t[ 5 ] ) );

  if ( tmax < 0 )
  {
    return false;
  }

  if ( tmin > tmax )
  {
    return false;
  }
  
  float32 t_result = tmin;
  if ( tmin < 0.0f )
  {
    t_result = tmax;
  }

  if ( result )
  {
    result->hit = true;
    result->t = t_result;
    result->point = XMVectorAdd( ray_origin, XMVectorScale( ray_direction, t_result ) );

    XMVECTOR normals[ ] =
    {
      g_XMIdentityR0,
      g_XMIdentityR0 * -1.0f,
      g_XMIdentityR1,
      g_XMIdentityR1 * -1.0f,
      g_XMIdentityR2,
      g_XMIdentityR2 * -1.0f
    };

    for ( uint32 i = 0; i < 6; ++i )
    {
      if ( fabsf( t_result - t[ i ] ) < 0.0001f )
      {
        result->normal = XMVector3Normalize( normals[ i ] );
      }
    }
  }

  return tmin;
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

bool
crude_raycast_plane
(
  _In_ XMVECTOR                                             ray_origin,
  _In_ XMVECTOR                                             ray_direction,
  _In_ XMVECTOR                                             plane,
  _Out_opt_ crude_raycast_result                           *result
)
{
  float32 nd = XMVectorGetX( XMVector3Dot( ray_direction, plane ) );
  float32 pn = XMVectorGetX( XMVector3Dot( ray_origin, plane ) );

  if ( result )
  {
    *result = crude_raycast_result_empty( );
  }

  if ( nd >= 0.f )
  {
    return false;
  }

  float32 t = ( XMVectorGetW( plane ) - pn ) / nd;

  if ( t >= 0.0f )
  {
		if ( result )
    {
			result->t = t;
			result->hit = true;
			result->point = XMVectorAdd( ray_origin, XMVectorScale( ray_direction, t ) );
			result->normal = XMVector3Normalize( plane );
		}
    return true;
  }

  return false;
}

bool
crude_raycast_triangle
(
  _In_ XMVECTOR                                             ray_origin,
  _In_ XMVECTOR                                             ray_direction,
  _In_ XMVECTOR                                             t0,
  _In_ XMVECTOR                                             t1,
  _In_ XMVECTOR                                             t2,
  _Out_opt_ crude_raycast_result                           *result
)
{
  XMVECTOR plane = crude_plane_from_points( t0, t1, t2 );

  if ( !crude_raycast_plane( ray_origin, ray_direction, plane, result ) )
  {
    return false;
  }

  XMVECTOR result_point = XMVectorAdd( ray_origin, XMVectorScale( ray_direction, result->t ) );
 
  XMVECTOR barycentric = crude_barycentric( result_point, t0, t1, t2 );

  if
  (
    XMVectorGetX( barycentric ) >= 0.f && XMVectorGetX( barycentric ) <= 1.f &&
    XMVectorGetY( barycentric ) >= 0.f && XMVectorGetY( barycentric ) <= 1.f &&
    XMVectorGetZ( barycentric ) >= 0.f && XMVectorGetZ( barycentric ) <= 1.f
  )
  {
    if ( result )
    {
			result->t = result->t;
			result->hit = true;
			result->point = XMVectorAdd( ray_origin, XMVectorScale( ray_direction, result->t ) );
			result->normal = XMVector3Normalize( plane );
    }
    return true;
  }

  return false;
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

crude_raycast_result
crude_raycast_result_empty
(
)
{
  crude_raycast_result result = CRUDE_COMPOUNT_EMPTY( crude_raycast_result );
  result.t = -1.f;
  result.hit = false;
  result.normal = g_XMIdentityR1;
  result.point = XMVectorZero( );
  return result;
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