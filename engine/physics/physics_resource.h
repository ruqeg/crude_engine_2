#pragma once

#include <engine/scene/scene_ecs.h>
#include <engine/physics/physics.h>

typedef struct crude_physics_character
{
  float32                                                  character_height_standing;
  float32                                                  character_radius_standing;
  float32                                                  friction;
  float32                                                  max_slop_angle;
  crude_physics_character_handle                           handle;
} crude_physics_character;

CRUDE_API crude_physics_character
crude_physics_character_empty
(
);

CRUDE_API void
crude_jph_transform_to_transform
(
  _Out_ crude_transform                                   *transform,
  _In_ JPH::RMat44                                        *jph_transform
);

CRUDE_API JPH::RVec3
crude_float3_to_jph_vec3
(
  _In_ XMFLOAT3                                           *float3
);

CRUDE_API JPH::Quat
crude_float4_to_jph_quat
(
  _In_ XMFLOAT4                                           *float4
);

CRUDE_API JPH::RVec3
crude_vector_to_jph_vec3
(
  _In_ XMVECTOR                                            vector
);

CRUDE_API JPH::Quat
crude_vector_to_jph_quat
(
  _In_ XMVECTOR                                            vector
);

CRUDE_API XMVECTOR
crude_jph_vec3_to_vector
(
  _In_ JPH::RVec3                                          v
);

CRUDE_API XMVECTOR
crude_jph_quat_to_vector
(
  _In_ JPH::Quat                                           q
);