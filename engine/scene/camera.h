#pragma once

#include <flecs.h>

#include <core/math.h>

typedef struct crude_camera
{
  crude_float4x4      view_to_clip_float4x4;
  crude_float4x4      clip_to_view_float4x4;
  float32             fov_radians;
  float32             near_z;
  float32             far_z;
  float32             aspect_ratio;
} crude_camera;

CRUDE_API ECS_COMPONENT_DECLARE( crude_camera );

CRUDE_API void
crude_calculate_camera
(
  _Out_ crude_camera *camera,
  _In_ float32        fov_radians,
  _In_ float32        aspect_ratio,
  _In_ float32        near_z,
  _In_ float32        far_z
);