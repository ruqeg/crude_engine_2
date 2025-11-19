#pragma once

#include <core/math.h>
#include <core/memory.h>

// !TODO implemente later, i'm too lazy for now 
typedef struct crude_octree
{
  XMFLOAT3                                                *points;
} crude_octree;

CRUDE_API void
crude_octree_initialize
(
  _In_ crude_octree                                       *octree,
  _In_ crude_allocator_container                          allocator_container
);

CRUDE_API void
crude_octree_add_triangle
(
  _In_ crude_octree                                       *octree,
  _In_ XMVECTOR                                            t0,
  _In_ XMVECTOR                                            t1,
  _In_ XMVECTOR                                            t2
);

CRUDE_API XMVECTOR
crude_octree_closest_point
(
  _In_ crude_octree                                       *octree,
  _In_ XMVECTOR                                            point
);

CRUDE_API bool
crude_octree_cast_ray
(
  _In_ crude_octree                                       *octree,
  _In_ XMVECTOR                                            ray_origin,
  _In_ XMVECTOR                                            ray_direction,
  _Out_opt_ crude_raycast_result                          *result
);

CRUDE_API void
crude_octree_deinitialize
(
  _In_ crude_octree                                       *octree
);