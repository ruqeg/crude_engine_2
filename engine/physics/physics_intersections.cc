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