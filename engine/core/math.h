#pragma once

/* okay I give up... directxmath is too good to not to use */
#include <thirdparty/cJSON/cJSON.h>
#include <DirectXMath.h>
#include <math.h>

#include <engine/core/alias.h>

using namespace DirectX;

#define CRUDE_FLOOR( a ) floor( a )
#define CRUDE_CEIL( a ) ceil( a )
#define CRUDE_MAX( a, b ) fmaxf( a, b )
#define CRUDE_MIN( a, b ) fminf( a, b )
#define CRUDE_MIN_INT( a, b ) ( ( ( a ) < ( b ) ) ? ( a ) : ( b ) )
#define CRUDE_CLAMP( x, u, l ) CRUDE_MIN( u, CRUDE_MAX( x, l ) )
#define CRUDE_DEG_TO_RAD( x ) ( XMConvertToRadians( x ) ) 
#define CRUDE_RAD_TO_DEG( x ) ( XMConvertToDegrees( x ) ) 
#define CRUDE_CLAMP( x, u, l ) CRUDE_MIN( u, CRUDE_MAX( x, l ) )
#define CRUDE_LERP( a, b, c ) ( a * ( 1.f - c ) + b * c )

typedef struct crude_raycast_result
{
  XMVECTOR                                                 point;
  XMVECTOR                                                 normal;
  float32                                                  t;
  bool                                                     hit;
} crude_raycast_result;


CRUDE_API XMVECTOR
crude_quaternion_to_pitch_yaw_roll
(
  _In_ FXMVECTOR                                           quaternion
);

CRUDE_API float32
crude_random_unit_f32
(
);

CRUDE_API float32
crude_lerp_angle
(
  _In_ float32                                             from,
  _In_ float32                                             to,
  _In_ float32                                             weight
);

CRUDE_API XMVECTOR
crude_closest_point_to_obb
(
  _In_ XMVECTOR                                             point,
  _In_ XMVECTOR                                             obb_position,
  _In_ XMVECTOR                                             obb_size,
  _In_ XMMATRIX                                             obb_orientation
);

CRUDE_API bool
crude_intersection_sphere_obb
(
  _In_ XMVECTOR                                             closest_point,
  _In_ XMVECTOR                                             sphere_position,
  _In_ float32                                              sphere_radius
);

CRUDE_API XMVECTOR
crude_project_vector3
(
  _In_ XMVECTOR                                            length,
  _In_ XMVECTOR                                            direction
);

CRUDE_API XMVECTOR
crude_barycentric
(
  _In_ XMVECTOR                                             p,
  _In_ XMVECTOR                                             ta,
  _In_ XMVECTOR                                             tb,
  _In_ XMVECTOR                                             tc
);

CRUDE_API float32
crude_raycast_obb
(
  _In_ XMVECTOR                                             ray_origin,
  _In_ XMVECTOR                                             ray_direction,
  _In_ XMVECTOR                                             obb_position,
  _In_ XMVECTOR                                             obb_size,
  _In_ XMMATRIX                                             obb_orientation,
  _Out_opt_ crude_raycast_result                           *result
);

CRUDE_API bool
crude_raycast_plane
(
  _In_ XMVECTOR                                             ray_origin,
  _In_ XMVECTOR                                             ray_direction,
  _In_ XMVECTOR                                             plane,
  _Out_opt_ crude_raycast_result                           *result
);

CRUDE_API bool
crude_raycast_triangle
(
  _In_ XMVECTOR                                             ray_origin,
  _In_ XMVECTOR                                             ray_direction,
  _In_ XMVECTOR                                             t0,
  _In_ XMVECTOR                                             t1,
  _In_ XMVECTOR                                             t2,
  _Out_opt_ crude_raycast_result                           *result
);

CRUDE_API XMVECTOR
crude_closest_point_to_plane
(
  _In_ XMVECTOR                                            plane,
  _In_ XMVECTOR                                            point
);

CRUDE_API bool
crude_point_in_triangle
(
  _In_ XMVECTOR                                             t0,
  _In_ XMVECTOR                                             t1,
  _In_ XMVECTOR                                             t2,
  _In_ XMVECTOR                                             p
);

CRUDE_API XMVECTOR
crude_closest_point_to_line
(
  _In_ XMVECTOR                                             lstart,
  _In_ XMVECTOR                                             lend,
  _In_ XMVECTOR                                             p
);

CRUDE_API XMVECTOR
crude_plane_from_points
(
  _In_ XMVECTOR                                             p0,
  _In_ XMVECTOR                                             p1,
  _In_ XMVECTOR                                             p2
);

CRUDE_API XMVECTOR
crude_closest_point_to_triangle
(
  _In_ XMVECTOR                                             t0,
  _In_ XMVECTOR                                             t1,
  _In_ XMVECTOR                                             t2,
  _In_ XMVECTOR                                             p
);

CRUDE_API bool
crude_intersection_sphere_triangle
(
  _In_ XMVECTOR                                             closest_point,
  _In_ XMVECTOR                                             sphere_position,
  _In_ float32                                              sphere_radius
);

CRUDE_API crude_raycast_result
crude_raycast_result_empty
(
);

CRUDE_API void
crude_parse_json_to_float2
(
  _Out_ XMFLOAT2                                          *float2,
  _In_ cJSON                                              *json
);

CRUDE_API void
crude_parse_json_to_float3
(
  _Out_ XMFLOAT3                                          *float3,
  _In_ cJSON                                              *json
);

CRUDE_API void
crude_parse_json_to_float4
(
  _Out_ XMFLOAT4                                          *float4,
  _In_ cJSON                                              *json
);