#include <core/array.h>
#include <core/assert.h>

#include <core/octree.h>

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
  CRUDE_ARRAY_PUSH( octree->points, t0 );
  CRUDE_ARRAY_PUSH( octree->points, t1 );
  CRUDE_ARRAY_PUSH( octree->points, t2 );
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
    XMVECTOR                                               closest_point;
    float32                                                closest_point_distance_sq;

    closest_point = crude_closest_point_to_triangle( octree->points[ i ], octree->points[ i + 1 ], octree->points[ i + 2 ], point );

    closest_point_distance_sq = XMVectorGetX( XMVector3LengthSq( XMVectorSubtract( closest_point, point ) ) );

    if ( min_closest_point_distance_sq > closest_point_distance_sq )
    {
      min_closest_point_distance_sq = closest_point_distance_sq; 
      min_closest_point = closest_point;
    }
  }

  return min_closest_point;
}

void
crude_octree_deinitialize
(
  _In_ crude_octree                                       *octree
)
{
  CRUDE_ARRAY_DEINITIALIZE( octree->points );
}