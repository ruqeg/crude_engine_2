#include <core/memory.h>

#include <physics/physics_intersections.h>

XMVECTOR
crude_physics_closest_point_to_obb
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
crude_physics_intersection_sphere_obb
(
  _In_ XMVECTOR                                             closest_point,
  _In_ XMVECTOR                                             sphere_position,
  _In_ float32                                              sphere_radius
)
{
  return XMVectorGetX( XMVector3LengthSq( sphere_position - closest_point ) ) < sphere_radius * sphere_radius;
}

float32
crude_physics_raycast_obb
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