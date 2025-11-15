#include <stdlib.h>
#include <cJSON.h>

#include <core/memory.h>
#include <core/math.h>
#include <core/assert.h>

CRUDE_COMPONENT_STRING_DEFINE( XMFLOAT2, "float2" );
CRUDE_COMPONENT_STRING_DEFINE( XMFLOAT3, "float3" );
CRUDE_COMPONENT_STRING_DEFINE( XMFLOAT4, "float4" );

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
  _In_ XMMATRIX                                             obb_orientation
)
{
  float32                                                  t[ 6 ];
  XMVECTOR                                                 x_axis, y_axis, z_axis, p, f, e;
  float32                                                  tmin, tmax;
  
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
        return -1;
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
    return -1.0f;
  }

  if ( tmin > tmax )
  {
    return -1.0f;
  }

  if ( tmin < 0.0f )
  {
    return tmax;
  }

  return tmin;
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
  
  if ( XMVectorGetX( XMVector3Cross( norm_pbc, norm_pca ) ) < 0.f )
  {
    return false;
  }
  else if ( XMVectorGetX( XMVector3Cross( norm_pbc, norm_pab ) ) < 0.f )
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
crude_closest_point_to_triangle
(
  _In_ XMVECTOR                                             t0,
  _In_ XMVECTOR                                             t1,
  _In_ XMVECTOR                                             t2,
  _In_ XMVECTOR                                             p
)
{
  XMVECTOR plane = XMPlaneFromPoints( t0, t1, t2 );
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

CRUDE_PARSE_JSON_TO_COMPONENT_FUNC_DECLARATION( XMFLOAT2 )
{
  CRUDE_ASSERT( cJSON_GetArraySize( component_json ) == 2 );

  component->x = CRUDE_STATIC_CAST( float32, cJSON_GetNumberValue( cJSON_GetArrayItem( component_json , 0 ) ) );
  component->y = CRUDE_STATIC_CAST( float32, cJSON_GetNumberValue( cJSON_GetArrayItem( component_json , 1 ) ) );
  return true;
}

CRUDE_PARSE_JSON_TO_COMPONENT_FUNC_DECLARATION( XMFLOAT3 )
{
  CRUDE_ASSERT( cJSON_GetArraySize( component_json ) == 3 );
  component->x = CRUDE_STATIC_CAST( float32, cJSON_GetNumberValue( cJSON_GetArrayItem( component_json , 0 ) ) );
  component->y = CRUDE_STATIC_CAST( float32, cJSON_GetNumberValue( cJSON_GetArrayItem( component_json , 1 ) ) );
  component->z = CRUDE_STATIC_CAST( float32, cJSON_GetNumberValue( cJSON_GetArrayItem( component_json , 2 ) ) );
  return true;
}

CRUDE_PARSE_JSON_TO_COMPONENT_FUNC_DECLARATION( XMFLOAT4 )
{
  CRUDE_ASSERT( cJSON_GetArraySize( component_json ) == 4 );
  component->x = CRUDE_STATIC_CAST( float32, cJSON_GetNumberValue( cJSON_GetArrayItem( component_json , 0 ) ) );
  component->y = CRUDE_STATIC_CAST( float32, cJSON_GetNumberValue( cJSON_GetArrayItem( component_json , 1 ) ) );
  component->z = CRUDE_STATIC_CAST( float32, cJSON_GetNumberValue( cJSON_GetArrayItem( component_json , 2 ) ) );
  component->w = CRUDE_STATIC_CAST( float32, cJSON_GetNumberValue( cJSON_GetArrayItem( component_json , 3 ) ) );
  return true;
}

CRUDE_PARSE_COMPONENT_TO_JSON_FUNC_DECLARATION( XMFLOAT2 )
{
  return cJSON_CreateFloatArray( &component->x, 2 );
}

CRUDE_PARSE_COMPONENT_TO_JSON_FUNC_DECLARATION( XMFLOAT3 )
{
  return cJSON_CreateFloatArray( &component->x, 3 );
}

CRUDE_PARSE_COMPONENT_TO_JSON_FUNC_DECLARATION( XMFLOAT4 )
{
  return cJSON_CreateFloatArray( &component->x, 4 );
}