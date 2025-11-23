#include <engine/core/array.h>
#include <engine/core/assert.h>

#include <engine/core/octree.h>

void
crude_octree_initialize
(
  _In_ crude_octree                                       *octree,
  _In_ crude_allocator_container                           allocator_container
)
{
  CRUDE_ARRAY_INITIALIZE_WITH_CAPACITY( octree->points, 128 * 3, allocator_container );
}

void
crude_octree_add_triangle
(
  _In_ crude_octree                                       *octree,
  _In_ XMVECTOR                                            t0,
  _In_ XMVECTOR                                            t1,
  _In_ XMVECTOR                                            t2
)
{
  uint32 points_count = CRUDE_ARRAY_LENGTH( octree->points );
  CRUDE_ARRAY_SET_LENGTH( octree->points, points_count + 3 );
  XMStoreFloat3( &octree->points[ points_count + 0 ], t0 );
  XMStoreFloat3( &octree->points[ points_count + 1 ], t1 );
  XMStoreFloat3( &octree->points[ points_count + 2 ], t2 );
}

XMVECTOR
crude_octree_closest_point
(
  _In_ crude_octree                                       *octree,
  _In_ XMVECTOR                                            point
)
{
  XMVECTOR                                                 min_closest_point;
  float32                                                  min_closest_point_distance_sq;

  CRUDE_ASSERT( CRUDE_ARRAY_LENGTH( octree->points ) );

  min_closest_point = point;
  min_closest_point_distance_sq = FLT_MAX;

  for ( uint32 i = 0; i < CRUDE_ARRAY_LENGTH( octree->points ); i += 3 )
  {
    XMVECTOR                                               t0, t1, t2, closest_point;
    float32                                                closest_point_distance_sq;

    t0 = XMLoadFloat3( &octree->points[ i ] );
    t1 = XMLoadFloat3( &octree->points[ i + 1 ] );
    t2 = XMLoadFloat3( &octree->points[ i + 2 ] );
    closest_point = crude_closest_point_to_triangle( t0, t1, t2, point );

    closest_point_distance_sq = XMVectorGetX( XMVector3LengthSq( XMVectorSubtract( closest_point, point ) ) );

    if ( closest_point_distance_sq < min_closest_point_distance_sq )
    {
      min_closest_point_distance_sq = closest_point_distance_sq; 
      min_closest_point = closest_point;
    }
  }

  return min_closest_point;
}

bool
crude_octree_cast_ray
(
  _In_ crude_octree                                       *octree,
  _In_ XMVECTOR                                            ray_origin,
  _In_ XMVECTOR                                            ray_direction,
  _Out_opt_ crude_raycast_result                          *result
)
{
  float32                                                  nearest_t;

  CRUDE_ASSERT( CRUDE_ARRAY_LENGTH( octree->points ) );

  if ( result )
  {
    *result = crude_raycast_result_empty( );
  }

  nearest_t = FLT_MAX;

  for ( uint32 i = 0; i < CRUDE_ARRAY_LENGTH( octree->points ); i += 3 )
  {
    XMVECTOR                                               t0, t1, t2, closest_point;
    float32                                                closest_point_distance_sq;
    crude_raycast_result                                   current_triangle_result;
    bool                                                   intersected;

    t0 = XMLoadFloat3( &octree->points[ i ] );
    t1 = XMLoadFloat3( &octree->points[ i + 1 ] );
    t2 = XMLoadFloat3( &octree->points[ i + 2 ] );
    
    intersected = crude_raycast_triangle( ray_origin, ray_direction, t0, t1, t2, &current_triangle_result ); // !TODO it can be optimized (and i'm not even about actual octree :D, length to triangle < t to raycast but i'm too lazy)

    if ( intersected )
    {
      if ( current_triangle_result.t < nearest_t )
      {
        nearest_t = current_triangle_result.t;

        if ( result )
        {
          *result = current_triangle_result;
        }
      }
    }
  }

  return nearest_t != FLT_MAX;
}

void
crude_octree_deinitialize
(
  _In_ crude_octree                                       *octree
)
{
  CRUDE_ARRAY_DEINITIALIZE( octree->points );
}