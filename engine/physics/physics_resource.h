#pragma once

#include <Jolt/Jolt.h>
#include <Jolt/RegisterTypes.h>
#include <Jolt/Core/Factory.h>
#include <Jolt/Core/TempAllocator.h>
#include <Jolt/Core/JobSystemThreadPool.h>
#include <Jolt/Physics/PhysicsSettings.h>
#include <Jolt/Physics/PhysicsSystem.h>
#include <Jolt/Physics/Collision/Shape/BoxShape.h>
#include <Jolt/Physics/Collision/Shape/SphereShape.h>
#include <Jolt/Physics/Body/BodyCreationSettings.h>
#include <Jolt/Physics/Body/BodyActivationListener.h>
#include <Jolt/Physics/Character/Character.h>
#include <Jolt/Physics/Collision/Shape/RotatedTranslatedShape.h>
#include <Jolt/Physics/Collision/Shape/CapsuleShape.h>
#include <Jolt/Physics/Collision/Shape/MeshShape.h>
#include <Jolt/Physics/Collision/NarrowPhaseQuery.h>
#include <Jolt/Physics/Collision/RayCast.h>
#include <Jolt/Physics/Collision/CastResult.h>

#include <engine/physics/physics_config.h>
#include <engine/scene/scene_ecs.h>

typedef enum crude_physics_body_shape_type
{
  CRUDE_PHYSICS_BODY_SHAPE_TYPE_BOX,
  CRUDE_PHYSICS_BODY_SHAPE_TYPE_MESH,
  CRUDE_PHYSICS_BODY_SHAPE_TYPE_COUNT
} crude_physics_body_shape_type;

typedef struct crude_physics_character_handle
{
  uint32                                                   index;
} crude_physics_character_handle;

typedef struct crude_physics_static_body_handle
{
  uint32                                                   index;
} crude_physics_static_body_handle;

typedef struct crude_physics_kinematic_body_handle
{
  uint32                                                   index;
} crude_physics_kinematic_body_handle;

typedef struct crude_physics_mesh_shape_handle
{
  uint32                                                   index;
} crude_physics_mesh_shape_handle;

typedef void (*crude_physics_kinematic_body_contact_added_callback)
(
  _In_ crude_entity                                        signal_entity,
  _In_ crude_entity                                        hitted_entity
);

typedef struct crude_physics_character_creation
{
  float32                                                  character_height_standing;
  float32                                                  character_radius_standing;
  float32                                                  friction;
  float32                                                  max_slop_angle;
  uint16                                                   layers;
} crude_physics_character_creation;

typedef struct crude_physics_character_container
{
  JPH::RMat44                                              manually_stored_transform;
  JPH::Ref< JPH::Character >                               jph_character_class;
} crude_physics_character_container;

typedef struct crude_physics_character
{
  float32                                                  height;
  float32                                                  radius;
  float32                                                  friction;
  float32                                                  max_slop_angle;
  crude_physics_character_handle                           handle;
  int32                                                    layers;
} crude_physics_character;

typedef struct crude_physics_static_body_creation
{
  crude_physics_body_shape_type                            type;
  union
  {
    struct
    {
      XMFLOAT3                                             extent;
    } box;
    struct
    {
      crude_physics_mesh_shape_handle                      handle;
    } mesh;
  };
  uint16                                                   layers;
  crude_entity                                             entity;
} crude_physics_static_body_creation;

typedef struct crude_physics_static_body_container
{
  JPH::BodyID                                              jph_body_class;
  crude_entity                                             entity;
} crude_physics_static_body_container;

typedef struct crude_physics_static_body
{
  crude_physics_body_shape_type                            type;
  union
  {
    struct
    {
      XMFLOAT3                                             extent;
    } box;
    struct
    {
      crude_physics_mesh_shape_handle                      handle;
    } mesh;
  };
  uint32                                                   layers;
} crude_physics_static_body;

typedef struct crude_physics_kinematic_body_creation
{
  crude_physics_body_shape_type                            type;
  union
  {
    struct
    {
      XMFLOAT3                                             extent;
    } box;
    struct
    {
      crude_physics_mesh_shape_handle                      handle;
    } mesh;
  };
  uint16                                                   layers;
  crude_entity                                             entity;
  bool                                                     sensor;
} crude_physics_kinematic_body_creation;

typedef struct crude_physics_kinematic_body_container
{
  JPH::BodyID                                              jph_body_class;
  crude_entity                                             entity;
  crude_physics_kinematic_body_contact_added_callback      contact_added_callback;
} crude_physics_kinematic_body_container;

typedef struct crude_physics_kinematic_body
{
  crude_physics_body_shape_type                            type;
  union
  {
    struct
    {
      XMFLOAT3                                             extent;
    } box;
    struct
    {
      crude_physics_mesh_shape_handle                      handle;
    } mesh;
  };
  uint32                                                   layers;
  bool                                                     sensor;
} crude_physics_kinematic_body;

typedef struct crude_physics_ray_cast_result
{
  crude_entity                                             entity;
  uint32                                                   layer;
} crude_physics_ray_cast_result;

CRUDE_API crude_physics_character
crude_physics_character_empty
(
);

CRUDE_API crude_physics_static_body
crude_physics_static_body_empty
(
);

CRUDE_API crude_physics_kinematic_body
crude_physics_kinematic_body_empty
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