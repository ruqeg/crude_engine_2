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

#include <engine/physics/physics_resource.h>

/* JPH FORCE ME TO USE CLASSES AND OOP T_T WHY I DONT WAAANT */

const JPH::ObjectLayer g_crude_jph_layer_non_moving        = 0;
const JPH::ObjectLayer g_crude_jph_layer_moving            = 1;
const JPH::ObjectLayer g_crude_jph_num_layers              = 2;

constexpr JPH::BroadPhaseLayer g_crude_jph_broad_phase_layer_non_moving_class      { 0 };
constexpr JPH::BroadPhaseLayer g_crude_jph_broad_phase_layer_moving_class          { 1 };
constexpr JPH::uint g_crude_jph_broad_phase_layer_num_layers                       = 2;

class _crude_jph_object_layer_pair_filter_class : public JPH::ObjectLayerPairFilter
{
public:
  virtual bool
  ShouldCollide
  (
    _In_ JPH::ObjectLayer                                  object1,
    _In_ JPH::ObjectLayer                                  object2
  ) const override;
};

class _crude_jph_bp_layer_interface_class final : public JPH::BroadPhaseLayerInterface
{
public:
  _crude_jph_bp_layer_interface_class
  (
  );

  virtual JPH::uint
  GetNumBroadPhaseLayers
  (
  ) const override;

  virtual JPH::BroadPhaseLayer
  GetBroadPhaseLayer
  (
    _In_ JPH::ObjectLayer                                  layer
  ) const override;

#if defined( JPH_EXTERNAL_PROFILE ) || defined( JPH_PROFILE_ENABLED )
  virtual char const*
  GetBroadPhaseLayerName
  (
    _In_ JPH::BroadPhaseLayer                              layer
  ) const override;
#endif /* JPH_EXTERNAL_PROFILE || JPH_PROFILE_ENABLED */

private:
  JPH::BroadPhaseLayer                                     object_to_broad_phase[ g_crude_jph_num_layers ];
};

class _crude_jph_object_vs_broad_phase_layer_filter : public JPH::ObjectVsBroadPhaseLayerFilter
{
public:
  virtual bool
  ShouldCollide
  (
    _In_ JPH::ObjectLayer                                  layer1,
    _In_ JPH::BroadPhaseLayer                              layer2
  ) const override;
};

class _crude_jph_contact_listener_class : public JPH::ContactListener
{
public:
  virtual JPH::ValidateResult
  OnContactValidate
  (
    _In_ JPH::Body const                                   &body1,
    _In_ JPH::Body const                                   &body2,
    _In_ JPH::RVec3Arg                                      in_base_offset,
    _In_ JPH::CollideShapeResult const                     &in_collision_result
  ) override;

  virtual void
  OnContactAdded
  (
    _In_ const JPH::Body                                   &body1,
    _In_ const JPH::Body                                   &body2,
    _In_ const JPH::ContactManifold                        &manifold,
    _In_ JPH::ContactSettings                              &settings
  ) override;

  virtual void
  OnContactPersisted
  (
    _In_ const JPH::Body                                  &body1,
    _In_ const JPH::Body                                  &body2,
    _In_ const JPH::ContactManifold                       &manifold,
    _In_ JPH::ContactSettings                             &settings
  ) override;

  virtual void
  OnContactRemoved
  (
    _In_ const JPH::SubShapeIDPair                        &subshapepair
  ) override;
};

class _crude_jph_body_activation_listener_class : public JPH::BodyActivationListener
{
public:
  virtual void
  OnBodyActivated
  (
    _In_ const JPH::BodyID                                &body_id,
    _In_ uint64                                            body_user_data
  ) override;

  virtual void
  OnBodyDeactivated
  (
    _In_ const JPH::BodyID                                &body_id,
    _In_ uint64                                            body_user_data
  ) override;
};

typedef struct crude_physics_raycast_result
{
  crude_raycast_result                                     raycast_result;
  crude_entity                                             node;
  uint64                                                   body_layer;
} crude_physics_raycast_result;

typedef struct crude_physics_creation
{
  crude_collisions_resources_manager                      *collision_manager;
  crude_heap_allocator                                    *allocator;
} crude_physics_creation;

typedef struct crude_physics
{
  ecs_query_t                                             *character_body_handle_query;
  ecs_query_t                                             *static_body_handle_query;
  crude_collisions_resources_manager                      *collision_manager;
  bool                                                     simulation_enabled;

  crude_heap_allocator                                    *allocator;
  crude_allocator_container                                allocator_container;
  
  JPH::PhysicsSystem                                      *jph_physics_system_class;
  JPH::TempAllocatorImpl                                  *jph_temporary_allocator_class;
  JPH::JobSystemThreadPool                                *jph_job_system_class;
  _crude_jph_bp_layer_interface_class                         *jph_broad_phase_layer_interface_class;
  _crude_jph_object_vs_broad_phase_layer_filter               *jph_object_vs_broadphase_layer_filter_class;
  _crude_jph_object_layer_pair_filter_class                   *jph_object_vs_object_layer_filter_class;
  _crude_jph_body_activation_listener_class                   *jph_body_activation_listener_class;
  _crude_jph_contact_listener_class                           *jph_contact_listener_class;
  JPH::Body                                               *jph_floor_class;
  JPH::BodyID                                              jph_sphere_id_class;
} crude_physics;

CRUDE_API void
crude_physics_initialize
(
  _In_ crude_physics                                      *physics,
  _In_ crude_physics_creation const                       *creation,
  _In_ crude_ecs                                          *world
);

CRUDE_API void
crude_physics_deinitialize
(
  _In_ crude_physics                                      *physics
);

CRUDE_API void
crude_physics_update
(
  _In_ crude_physics                                      *physics
);

CRUDE_API void
crude_physics_enable_simulation
(
  _In_ crude_physics                                      *physics,
  _In_ bool                                                enable
);

CRUDE_API void
crude_physics_enable_reset_velocity
(
  _In_ crude_physics                                      *physics,
  _In_ crude_ecs                                          *world
);

CRUDE_API bool
crude_physics_cast_ray
(
  _In_ crude_physics                                      *physics,
  _In_ crude_ecs                                          *world,
  _In_ XMVECTOR                                            ray_origin,
  _In_ XMVECTOR                                            ray_direction,
  _In_ uint32                                              mask,
  _Out_opt_ crude_physics_raycast_result                  *result
);