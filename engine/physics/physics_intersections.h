#pragma once

#include <core/alias.h>
#include <core/math.h>

CRUDE_API XMVECTOR
crude_physics_closest_point_to_obb
(
  _In_ XMVECTOR                                             point,
  _In_ XMVECTOR                                             obb_position,
  _In_ XMVECTOR                                             obb_size,
  _In_ XMMATRIX                                             obb_orientation
);

CRUDE_API bool
crude_physics_intersection_sphere_obb
(
  _In_ XMVECTOR                                             closest_point,
  _In_ XMVECTOR                                             sphere_position,
  _In_ float32                                              sphere_radius
);