#pragma once

#include <engine/core/ecs.h>
#include <engine/core/math.h>
#include <engine/physics/physics_shapes_manager.h>
#include <engine/physics/physics_resource.h>

/*********************
 * Use only when JPH class override new/free!
 ********************/
#define CRUDE_JOLT_OVERRIDEN_NEW                                     new
#define CRUDE_JOLT_OVERRIDEN_FREE                                    delete

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

typedef struct crude_physics_creation
{
  crude_physics_shapes_manager                            *physics_shapes_manager;
  crude_heap_allocator                                    *physics_allocator;
} crude_physics_creation;

typedef struct crude_physics
{
  /* Context */
  crude_physics_shapes_manager                            *physics_shapes_manager;

  crude_heap_allocator                                    *physics_allocator;
  crude_allocator_container                                physics_allocator_container;
  
  /* Commonm */
  crude_resource_pool                                      characters_resource_pool;
  crude_resource_pool                                      static_body_resource_pool;
  bool                                                     simulation_enabled;
  int64                                                    last_update_time;
    
  /* JPH */
  JPH::PhysicsSystem                                      *jph_physics_system_class;
  JPH::TempAllocatorImpl                                  *jph_temporary_allocator_class;
  JPH::JobSystemThreadPool                                *jph_job_system_class;
  _crude_jph_bp_layer_interface_class                     *jph_broad_phase_layer_interface_class;
  _crude_jph_object_vs_broad_phase_layer_filter           *jph_object_vs_broadphase_layer_filter_class;
  _crude_jph_object_layer_pair_filter_class               *jph_object_vs_object_layer_filter_class;
  _crude_jph_body_activation_listener_class               *jph_body_activation_listener_class;
  _crude_jph_contact_listener_class                       *jph_contact_listener_class;
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
  _In_ crude_physics                                      *physics,
  _In_ int64                                               current_time
);

CRUDE_API void
crude_physics_enable_simulation
(
  _In_ crude_physics                                      *physics,
  _In_ ecs_world_t                                        *world,
  _In_ bool                                                enable
);

CRUDE_API crude_physics_character_creation
crude_physics_character_creation_empty
(
);

CRUDE_API crude_physics_static_body_creation
crude_physics_static_body_creation_empty
(
);

CRUDE_API crude_physics_character_handle
crude_physics_create_character
(
  _In_ crude_physics                                      *physics,
  _In_ crude_physics_character_creation const             *creation
);

CRUDE_API crude_physics_character_container*
crude_physics_access_character
(
  _In_ crude_physics                                      *physics,
  _In_ crude_physics_character_handle                      handle
);

CRUDE_API void
crude_physics_destroy_character_instant
(
  _In_ crude_physics                                      *physics,
  _In_ crude_physics_character_handle                      handle
);

CRUDE_API crude_physics_static_body_handle
crude_physics_create_static_body
(
  _In_ crude_physics                                      *physics,
  _In_ crude_physics_static_body_creation const           *creation
);

CRUDE_API crude_physics_static_body_container*
crude_physics_access_static_body
(
  _In_ crude_physics                                      *physics,
  _In_ crude_physics_static_body_handle                    handle
);

CRUDE_API void
crude_physics_destroy_static_body_instant
(
  _In_ crude_physics                                      *physics,
  _In_ crude_physics_static_body_handle                    handle
);