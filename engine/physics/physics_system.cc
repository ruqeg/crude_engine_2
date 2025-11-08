
#include <stdarg.h>
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
#include <Jolt/Physics/Collision/Shape/CapsuleShape.h>
#include <Jolt/Physics/Character/CharacterVirtual.h>
#include <Jolt/Physics/Character/Character.h>
#include <Jolt/Physics/Collision/Shape/RotatedTranslatedShape.h>

#include <core/log.h>
#include <core/assert.h>
#include <scene/scene_components.h>
#include <physics/physics_components.h>

#include <physics/physics_system.h>

/* Disable common warnings triggered by Jolt, you can use JPH_SUPPRESS_WARNING_PUSH / JPH_SUPPRESS_WARNING_POP to store and restore the warning state */
JPH_SUPPRESS_WARNINGS

static void
jolt_trace_implementation_
(
  _In_ char const                                           *fmt,
  ...
)
{
  va_list list;
  va_start( list, fmt );
  crude_log_common_va( __FILE__, __LINE__, CRUDE_CHANNEL_PHYSICS, CRUDE_VERBOSITY_ALL, fmt, list );
  va_end( list );
}

#ifdef JPH_ENABLE_ASSERTS
static bool
jolt_assert_failed_impl
(
  _In_ char const                                           *expression,
  _In_ char const                                           *message,
  _In_ char const                                           *file,
  _In_ uint32                                                line
)
{
  CRUDE_LOG_ERROR( CRUDE_CHANNEL_PHYSICS, "%s:%s: (%s) %s", file, line, expression, message ? message : "" );
  return true;
};
#endif /* JPH_ENABLE_ASSERTS */

/**
 * Layer that objects can be in, determines which other objects it can collide with
 * Typically you at least want to have 1 layer for moving bodies and 1 layer for static bodies, but you can have more
 * layers if you want. E.g. you could have a layer for high detail collision (which is not used by the physics simulation
 * but only if you do collision testing).
 */
constexpr JPH::ObjectLayer non_moving_object_layer_ = 0;
constexpr JPH::ObjectLayer moving_object_layer_ = 1;
constexpr uint32 num_layers_ = 2;

/**
 * Class that determines if two object layers can collide
 */
class ObjectLayerPairFilterImpl
  : public JPH::ObjectLayerPairFilter
{
public:
  virtual bool
  ShouldCollide
  (
    _In_ JPH::ObjectLayer                                    in_object1, 
    _In_ JPH::ObjectLayer                                    in_object2
  ) const override
  {
    switch ( in_object1 )
    {
    case non_moving_object_layer_:
    {
      /* Non moving only collides with moving */
      return in_object2 == moving_object_layer_;
    }
    case moving_object_layer_:
    {
      /* Moving collides with everything */
      return true;
    }
    default:
    {
      JPH_ASSERT( false );
      return false;
    }
    }
  }
};

/**
 * Each broadphase layer results in a separate bounding volume tree in the broad phase. You at least want to have
 * a layer for non-moving and moving objects to avoid having to update a tree full of static objects every frame.
 * You can have a 1-on-1 mapping between object layers and broadphase layers (like in this case) but if you have
 * many object layers you'll be creating many broad phase trees, which is not efficient. If you want to fine tune
 * your broadphase layers define JPH_TRACK_BROADPHASE_STATS and look at the stats reported on the TTY.
 * */
constexpr static JPH::BroadPhaseLayer non_moving_broad_phase_layer_ { 0 };
constexpr static JPH::BroadPhaseLayer moving_broad_phase_layer_ { 1 };
constexpr static uint32 num_broad_phase_layers_ = 2;

/**
 * BroadPhaseLayerInterface implementation
 * This defines a mapping between object and broadphase layers.
 **/
class BPLayerInterfaceImpl final
  : public JPH::BroadPhaseLayerInterface
{
public:
  BPLayerInterfaceImpl
  (
  )
  {
    mObjectToBroadPhase[ non_moving_object_layer_ ] = non_moving_broad_phase_layer_;
    mObjectToBroadPhase[ moving_object_layer_ ] = moving_broad_phase_layer_;
  }

  virtual JPH::uint
  GetNumBroadPhaseLayers
  (
  ) const override
  {
    return num_broad_phase_layers_;
  }

  virtual JPH::BroadPhaseLayer
  GetBroadPhaseLayer
  (
    _In_ JPH::ObjectLayer                                    in_layer
  ) const override
  {
    JPH_ASSERT( in_layer < num_layers_ );
    return mObjectToBroadPhase[ in_layer ];
  }

#if defined(JPH_EXTERNAL_PROFILE) || defined( JPH_PROFILE_ENABLED )
  virtual char const*
  GetBroadPhaseLayerName
  (
    _In_ JPH::BroadPhaseLayer                                in_layer
  ) const override
  {
    switch ( ( JPH::BroadPhaseLayer::Type )in_layer )
    {
    case ( JPH::BroadPhaseLayer::Type ) non_moving_broad_phase_layer_:  return "NON_MOVING";
    case ( JPH::BroadPhaseLayer::Type ) moving_broad_phase_layer_:      return "MOVING";
    default:                          JPH_ASSERT(false); return "INVALID";
    }
  }
#endif // JPH_EXTERNAL_PROFILE || JPH_PROFILE_ENABLED
private:
  JPH::BroadPhaseLayer          mObjectToBroadPhase[ num_layers_ ];
};

/* Class that determines if an object layer can collide with a broadphase layer */
class ObjectVsBroadPhaseLayerFilterImpl
  : public JPH::ObjectVsBroadPhaseLayerFilter
{
public:
  virtual bool
  ShouldCollide
  (
    _In_ JPH::ObjectLayer                                                                       in_layer1,
    _In_ JPH::BroadPhaseLayer                                                                   in_layer2
  ) const override
  {
    switch ( in_layer1 )
    {
    case non_moving_object_layer_:
    {
      return in_layer2 == moving_broad_phase_layer_;
    }
    case moving_object_layer_:
    {
      return true;
    }
    default:
    {
      JPH_ASSERT( false );
      return false;
    }
    }
  }
};

class MyContactListener
  : public JPH::ContactListener
{
public:
  virtual JPH::ValidateResult
  OnContactValidate
  (
    _In_ JPH::Body const                                  &in_body1,
    _In_ JPH::Body const                                  &in_body2,
    _In_ JPH::RVec3Arg                                     in_base_offset,
    _In_ JPH::CollideShapeResult const                    &in_collision_result
  ) override
  {
    //cout << "Contact validate callback" << endl;

    // Allows you to ignore a contact before it is created (using layers to not make objects collide is cheaper!)
    return JPH::ValidateResult::AcceptAllContactsForThisBodyPair;
  }

  virtual void
  OnContactAdded(const JPH::Body &inBody1, const JPH::Body &inBody2, const JPH::ContactManifold &inManifold, JPH::ContactSettings &ioSettings) override
  {
    //cout << "A contact was added" << endl;
  }

  virtual void      OnContactPersisted(const JPH::Body &inBody1, const JPH::Body &inBody2, const JPH::ContactManifold &inManifold, JPH::ContactSettings &ioSettings) override
  {
    //cout << "A contact was persisted" << endl;
  }

  virtual void      OnContactRemoved(const JPH::SubShapeIDPair &inSubShapePair) override
  {
    //cout << "A contact was removed" << endl;
  }
};

class MyBodyActivationListener 
  : public JPH::BodyActivationListener
{
public:
  virtual void    OnBodyActivated(const JPH::BodyID &inBodyID, uint64 inBodyUserData) override
  {
    //cout << "A body got activated" << endl;
  }

  virtual void    OnBodyDeactivated(const JPH::BodyID &inBodyID, uint64 inBodyUserData) override
  {
    //cout << "A body went to sleep" << endl;
  }
};

CRUDE_ECS_SYSTEM_DECLARE( crude_process_physics_system_ );
CRUDE_ECS_OBSERVER_DECLARE( crude_physics_dynamic_body_creation_observer_ );
CRUDE_ECS_OBSERVER_DECLARE( crude_physics_static_body_creation_observer_ );
CRUDE_ECS_OBSERVER_DECLARE( crude_physics_body_destrotion_observer_ );
CRUDE_ECS_OBSERVER_DECLARE( crude_physics_body_transform_set_observer_ );
CRUDE_ECS_SYSTEM_DECLARE( crude_physics_body_update_transform_system_ );

// We need a temp allocator for temporary allocations during the physics update. We're
// pre-allocating 10 MB to avoid having to do allocations during the physics update.
// B.t.w. 10 MB is way too much for this example but it is a typical value you can use.
// If you don't want to pre-allocate you can also use TempAllocatorMalloc to fall back to
// malloc / free.
JPH::TempAllocatorImpl                                    *jph_temp_allocator_;

// We need a job system that will execute physics jobs on multiple threads. Typically
// you would implement the JobSystem interface yourself and let Jolt Physics run on top
// of your own job scheduler. JobSystemThreadPool is an example implementation.
JPH::JobSystemThreadPool                                  *jph_job_system_;

// Create mapping table from object layer to broadphase layer
// Note: As this is an interface, PhysicsSystem will take a reference to this so this instance needs to stay alive!
// Also have a look at BroadPhaseLayerInterfaceTable or BroadPhaseLayerInterfaceMask for a simpler interface.
BPLayerInterfaceImpl                                       jph_broad_phase_layer_interface_;

// Create class that filters object vs broadphase layers
// Note: As this is an interface, PhysicsSystem will take a reference to this so this instance needs to stay alive!
// Also have a look at ObjectVsBroadPhaseLayerFilterTable or ObjectVsBroadPhaseLayerFilterMask for a simpler interface.
ObjectVsBroadPhaseLayerFilterImpl                          jph_object_vs_broadphase_layer_filter_;

// Create class that filters object vs object layers
// Note: As this is an interface, PhysicsSystem will take a reference to this so this instance needs to stay alive!
// Also have a look at ObjectLayerPairFilterTable or ObjectLayerPairFilterMask for a simpler interface.
ObjectLayerPairFilterImpl                                  jph_object_vs_object_layer_filter_;

JPH::PhysicsSystem                                         jph_physics_system_;

// A body activation listener gets notified when bodies activate and go to sleep
// Note that this is called from a job so whatever you do here needs to be thread safe.
// Registering one is entirely optional.
MyBodyActivationListener                                   jph_body_activation_listener_;

// A contact listener gets notified when bodies (are about to) collide, and when they separate again.
// Note that this is called from a job so whatever you do here needs to be thread safe.
// Registering one is entirely optional.
MyContactListener                                          jph_contact_listener_;

static void
crude_physics_static_body_creation_observer_
(
  ecs_iter_t *it
)
{
  crude_physics_static_body                               *static_bodies_per_entity;
  crude_collision_shape                                   *collision_shapes_per_entity;
  JPH::BodyInterface                                      *body_interface;

  static_bodies_per_entity = ecs_field( it, crude_physics_static_body, 0 );
  collision_shapes_per_entity = ecs_field( it, crude_collision_shape, 1 );

  body_interface = &jph_physics_system_.GetBodyInterface();

  for ( uint32 i = 0; i < it->count; ++i )
  {
    ecs_world_t                                           *world;
    crude_physics_static_body                             *static_body;
    crude_collision_shape                                 *collision_shape;
    crude_transform const                                 *transform;
    crude_physics_body_handle                             *body_handle;
    crude_entity                                           entity;
    
    world = it->world;
    static_body = &static_bodies_per_entity[ i ];
    collision_shape = &collision_shapes_per_entity[ i ];
    entity = CRUDE_COMPOUNT( crude_entity, { it->entities[ i ], world } );
    
    body_handle = ecs_ensure( world, it->entities[ i ], crude_physics_body_handle );

    CRUDE_ASSERT( CRUDE_ENTITY_HAS_COMPONENT( entity, crude_transform ) );
    transform = CRUDE_ENTITY_GET_IMMUTABLE_COMPONENT( entity, crude_transform );

    if ( collision_shape->type == CRUDE_COLLISION_SHAPE_TYPE_BOX )
    {
      JPH::BodyID                                          jph_body_id;
      JPH::BodyCreationSettings                            jph_box_creation_settings;
      
      jph_box_creation_settings = JPH::BodyCreationSettings( 
        new JPH::BoxShape( JPH::Vec3( collision_shape->box.extent.x * 0.5f, collision_shape->box.extent.y * 0.5f, collision_shape->box.extent.z  * 0.5f ) ), 
        JPH::RVec3( transform->translation.x, transform->translation.y, transform->translation.z ),
        JPH::Quat( transform->rotation.x, transform->rotation.y, transform->rotation.z, transform->rotation.w ),
        JPH::EMotionType::Static,
        non_moving_object_layer_
      );
      jph_body_id = body_interface->CreateAndAddBody( jph_box_creation_settings, JPH::EActivation::DontActivate );
      body_handle->body_index = jph_body_id.GetIndexAndSequenceNumber( );
    }
    else
    {
      CRUDE_ASSERT( false );
    }
  }
}

static void
crude_physics_dynamic_body_creation_observer_
(
  ecs_iter_t *it
)
{
  crude_physics_dynamic_body                              *dynamic_bodies_per_entity;
  crude_collision_shape                                   *collision_shapes_per_entity;
  JPH::BodyInterface                                      *body_interface;

  dynamic_bodies_per_entity = ecs_field( it, crude_physics_dynamic_body, 0 );
  collision_shapes_per_entity = ecs_field( it, crude_collision_shape, 1 );

  body_interface = &jph_physics_system_.GetBodyInterface();

  for ( uint32 i = 0; i < it->count; ++i )
  {
    ecs_world_t                                           *world;
    crude_physics_dynamic_body                            *dynamic_body;
    crude_transform                                       *transform;
    crude_collision_shape                                 *collision_shape;
    crude_physics_body_handle                             *body_handle;
    JPH::ShapeSettings::ShapeResult                        shape_result;
    crude_entity                                           entity;

    world = it->world;
    dynamic_body = &dynamic_bodies_per_entity[ i ];
    entity = CRUDE_COMPOUNT( crude_entity, { it->entities[ i ], world } );
    collision_shape = &collision_shapes_per_entity[ i ];

    body_handle = ecs_ensure( world, it->entities[ i ], crude_physics_body_handle );

    CRUDE_ASSERT( CRUDE_ENTITY_HAS_COMPONENT( entity, crude_transform ) );
    transform = CRUDE_ENTITY_GET_MUTABLE_COMPONENT( entity, crude_transform );
    CRUDE_ASSERT( transform );
    
    if ( collision_shape->type == CRUDE_COLLISION_SHAPE_TYPE_SPHERE )
    {
      JPH::BodyID                                          jph_sphere_id;
      JPH::BodyCreationSettings                            jph_body_creation_settings;

      jph_body_creation_settings = JPH::BodyCreationSettings( 
        new JPH::SphereShape( collision_shape->sphere.radius ), 
        JPH::RVec3( transform->translation.x, transform->translation.y, transform->translation.z ),
        JPH::Quat( transform->rotation.x, transform->rotation.y, transform->rotation.z, transform->rotation.w ),
        JPH::EMotionType::Dynamic, moving_object_layer_
      );

      if ( dynamic_body->lock_rotation )
      {
        jph_body_creation_settings.mAllowedDOFs = JPH::EAllowedDOFs::TranslationX | JPH::EAllowedDOFs::TranslationY | JPH::EAllowedDOFs::TranslationZ;
      }

      jph_sphere_id = body_interface->CreateAndAddBody( jph_body_creation_settings, JPH::EActivation::Activate );

      body_handle->body_index = jph_sphere_id.GetIndexAndSequenceNumber( );
    }
    else
    {
      CRUDE_ASSERT( false );
    }
  }
}

static void
crude_physics_body_destrotion_observer_
(
  ecs_iter_t *it
)
{
  crude_physics_body_handle                               *bodies_per_entity;
  JPH::BodyInterface                                      *body_interface;

  bodies_per_entity = ecs_field( it, crude_physics_body_handle, 0 );
  body_interface = &jph_physics_system_.GetBodyInterface();

  for ( uint32 i = 0; i < it->count; ++i )
  {
    crude_physics_body_handle                             *body;
    JPH::BodyID                                            body_id;

    body = &bodies_per_entity[ i ];

    body_id = JPH::BodyID{ body->body_index };
    body_interface->RemoveBody( body_id );
    body_interface->DestroyBody( body_id );
  }
}

static void
crude_physics_body_transform_set_observer_
(
  ecs_iter_t *it
)
{
  crude_physics_body_handle                               *bodies_per_entity;
  crude_transform                                         *transform_per_entity;
  JPH::BodyInterface                                      *body_interface;

  bodies_per_entity = ecs_field( it, crude_physics_body_handle, 0 );
  transform_per_entity = ecs_field( it, crude_transform, 1 );

  body_interface = &jph_physics_system_.GetBodyInterface();

  for ( uint32 i = 0; i < it->count; ++i )
  {
    crude_physics_body_handle                             *body;
    crude_transform                                       *transform;
    JPH::BodyID                                            jph_body_id;
    JPH::ShapeRefC                                         jph_shape;

    body = &bodies_per_entity[ i ];
    transform = &transform_per_entity[ i ];
    crude_physics_body_set_translation( body, XMLoadFloat3( &transform->translation ) );
    crude_physics_body_set_rotation( body, XMLoadFloat4( &transform->rotation ) );
    crude_physics_body_set_linear_velocity( body, XMVectorZero( ) );
  }
}

void
crude_physics_update
(
  _In_ float32                                             delta_time
)
{
  JPH::BodyInterface &body_interface = jph_physics_system_.GetBodyInterface( );

  // Optional step: Before starting the physics simulation you can optimize the broad phase. This improves collision detection performance (it's pointless here because we only have 2 bodies).
  // You should definitely not call this every frame or when e.g. streaming in a new level section as it is an expensive operation.
  // Instead insert all new objects in batches instead of 1 at a time to keep the broad phase efficient.
  //jph_physics_system_.OptimizeBroadPhase( );

  // If you take larger steps than 1 / 60th of a second you need to do multiple collision steps in order to keep the simulation stable. Do 1 collision step per 1 / 60th of a second (round up).
  const int cCollisionSteps = 1;
  jph_physics_system_.Update( delta_time, cCollisionSteps, jph_temp_allocator_, jph_job_system_ );
}



static void
crude_physics_body_update_transform_system_
(
  ecs_iter_t *it
)
{
  crude_physics_body_handle                               *bodies_per_entity;
  crude_transform                                         *transform_per_entity;

  bodies_per_entity = ecs_field( it, crude_physics_body_handle, 0 );
  transform_per_entity = ecs_field( it, crude_transform, 1 );

  for ( uint32 i = 0; i < it->count; ++i )
  {
    crude_physics_body_handle                             *body;
    crude_transform                                       *transform;

    body = &bodies_per_entity[ i ];
    transform = &transform_per_entity[ i ];

    XMStoreFloat3( &transform->translation, crude_physics_body_get_center_of_mass_translation( body ) );
    XMStoreFloat4( &transform->rotation, crude_physics_body_get_rotation( body ) );
  }
}

CRUDE_ECS_MODULE_IMPORT_IMPL( crude_physics_system )
{
  ECS_MODULE( world, crude_physics_system );
  ECS_IMPORT( world, crude_physics_components );
  ECS_IMPORT( world, crude_scene_components );
 
  CRUDE_ECS_OBSERVER_DEFINE( world, crude_physics_static_body_creation_observer_, EcsOnSet, { 
    { .id = ecs_id( crude_physics_static_body ) },
    { .id = ecs_id( crude_collision_shape ) },
    { .id = ecs_id( crude_physics_body_handle ), .oper = EcsNot }
  } );
  
  CRUDE_ECS_OBSERVER_DEFINE( world, crude_physics_dynamic_body_creation_observer_, EcsOnSet, { 
    { .id = ecs_id( crude_physics_dynamic_body ) },
    { .id = ecs_id( crude_collision_shape ) },
    { .id = ecs_id( crude_physics_body_handle ), .oper = EcsNot }
  } );

  CRUDE_ECS_OBSERVER_DEFINE( world, crude_physics_body_transform_set_observer_, EcsOnSet, { 
    { .id = ecs_id( crude_physics_body_handle ) },
    { .id = ecs_id( crude_transform ) }
  } );

  CRUDE_ECS_OBSERVER_DEFINE( world, crude_physics_body_destrotion_observer_, EcsOnRemove, { 
    { .id = ecs_id( crude_physics_body_handle ) },
  } );
  
  CRUDE_ECS_SYSTEM_DEFINE( world, crude_physics_body_update_transform_system_, EcsOnUpdate, NULL, {
    { .id = ecs_id( crude_physics_body_handle ) },
    { .id = ecs_id( crude_transform ) }
  } );
}

void
crude_physics_initialize
(
  _In_ crude_physics_creation const                                                *creation
)
{
  JPH::RegisterDefaultAllocator( );

  JPH::Trace = jolt_trace_implementation_;
  JPH_IF_ENABLE_ASSERTS( JPH::AssertFailed = jolt_assert_failed_impl; )

  JPH::Factory::sInstance = new JPH::Factory( );

  JPH::RegisterTypes( );

  jph_temp_allocator_ = new JPH::TempAllocatorImpl( creation->temporary_allocator_size );
  jph_job_system_ = new JPH::JobSystemThreadPool( JPH::cMaxPhysicsJobs, JPH::cMaxPhysicsBarriers, creation->num_threads );
  jph_physics_system_.Init( creation->max_rigid_bodies, creation->num_body_mutexes, creation->max_body_pairs, creation->max_contact_constraints, jph_broad_phase_layer_interface_, jph_object_vs_broadphase_layer_filter_, jph_object_vs_object_layer_filter_ );
  jph_physics_system_.SetBodyActivationListener( &jph_body_activation_listener_ );
  jph_physics_system_.SetContactListener( &jph_contact_listener_ );
}

void
crude_physics_deinitialize
(
)
{
  delete jph_temp_allocator_;

  JPH::UnregisterTypes();

  delete JPH::Factory::sInstance;
  JPH::Factory::sInstance = nullptr;
}

crude_physics_creation
crude_physics_creation_empty
(
)
{
  crude_physics_creation creation = CRUDE_COMPOUNT_EMPTY( crude_physics_creation );
  creation.max_rigid_bodies = 1024;
  creation.num_threads = 1;
  creation.max_body_pairs = 1024;
  creation.max_contact_constraints = 1024;
  creation.temporary_allocator_size = 10u * 1024u * 1024u;
  return creation;
}

XMVECTOR
crude_physics_body_get_center_of_mass_translation
(
  _In_ crude_physics_body_handle const                    *dynamic_body
)
{
  JPH::BodyInterface                                      *jph_body_interface;
  JPH::RVec3                                               jph_position;

  jph_body_interface = &jph_physics_system_.GetBodyInterface( );
  jph_position = jph_body_interface->GetCenterOfMassPosition( JPH::BodyID( dynamic_body->body_index ) );
  return XMVectorSet( jph_position.GetX( ), jph_position.GetY( ), jph_position.GetZ( ), 1 );
}

void
crude_physics_body_set_translation
(
  _In_ crude_physics_body_handle const                    *dynamic_body,
  _In_ XMVECTOR                                            position
)
{
  JPH::BodyInterface                                      *jph_body_interface;
  XMFLOAT3                                                 position_f;

  XMStoreFloat3( &position_f, position );
  jph_body_interface = &jph_physics_system_.GetBodyInterface( );
  jph_body_interface->SetPosition( JPH::BodyID( dynamic_body->body_index ), JPH::Vec3( position_f.x, position_f.y, position_f.z ), JPH::EActivation::Activate );
}

void
crude_physics_body_set_rotation
(
  _In_ crude_physics_body_handle const                    *dynamic_body,
  _In_ XMVECTOR                                            rotation
)
{
  JPH::BodyInterface                                      *jph_body_interface;

  jph_body_interface = &jph_physics_system_.GetBodyInterface( );
  jph_body_interface->SetRotation( JPH::BodyID( dynamic_body->body_index ), JPH::Quat( XMVectorGetX( rotation ), XMVectorGetY( rotation ), XMVectorGetZ( rotation ), XMVectorGetW( rotation ) ), JPH::EActivation::Activate );
}

void
crude_physics_body_set_linear_velocity
(
  _In_ crude_physics_body_handle const                    *dynamic_body,
  _In_ XMVECTOR                                            velocity
)
{
  JPH::BodyInterface                                      *jph_body_interface;
  XMFLOAT3                                                 velocity_f;

  XMStoreFloat3( &velocity_f, velocity );
  jph_body_interface = &jph_physics_system_.GetBodyInterface( );
  jph_body_interface->SetLinearVelocity( JPH::BodyID( dynamic_body->body_index ), JPH::Vec3( velocity_f.x, velocity_f.y, velocity_f.z ) );
}

void
crude_physics_body_add_linear_velocity
(
  _In_ crude_physics_body_handle const                    *dynamic_body,
  _In_ XMVECTOR                                            velocity
)
{
  JPH::BodyInterface                                      *jph_body_interface;
  XMFLOAT3                                                 velocity_f;

  XMStoreFloat3( &velocity_f, velocity );
  jph_body_interface = &jph_physics_system_.GetBodyInterface( );
  jph_body_interface->AddLinearVelocity( JPH::BodyID( dynamic_body->body_index ), JPH::Vec3( velocity_f.x, velocity_f.y, velocity_f.z ) );
}

XMVECTOR
crude_physics_body_get_linear_velocity
(
  _In_ crude_physics_body_handle const                    *dynamic_body
)
{
  JPH::BodyInterface                                      *jph_body_interface;
  JPH::RVec3                                               jph_position;

  jph_body_interface = &jph_physics_system_.GetBodyInterface( );
  jph_position = jph_body_interface->GetLinearVelocity( JPH::BodyID( dynamic_body->body_index ) );
  return XMVectorSet( jph_position.GetX( ), jph_position.GetY( ), jph_position.GetZ( ), 1 );
}

XMVECTOR
crude_physics_body_get_rotation
(
  _In_ crude_physics_body_handle const                    *body
)
{
  JPH::BodyInterface                                      *jph_body_interface;
  JPH::Quat                                                jph_rotation;

  jph_body_interface = &jph_physics_system_.GetBodyInterface( );
  jph_rotation = jph_body_interface->GetRotation( JPH::BodyID( body->body_index ) );
  return XMVectorSet( jph_rotation.GetX( ), jph_rotation.GetY( ), jph_rotation.GetZ( ), jph_rotation.GetW( ) );
}

void
crude_physics_body_set_collision
(
  _In_ crude_physics_body_handle const                    *body,
  _In_ crude_collision_shape const                        *shape
)
{
  JPH::BodyInterface                                      *jph_body_interface;
  JPH::BodyID                                              jph_body_id;

  jph_body_interface = &jph_physics_system_.GetBodyInterface( );
  jph_body_id = CRUDE_COMPOUNT( JPH::BodyID, { body->body_index } );

  if ( shape->type == CRUDE_COLLISION_SHAPE_TYPE_BOX )
  {
    jph_body_interface->SetShape( jph_body_id, new JPH::BoxShape( JPH::Vec3( shape->box.extent.x * 0.5f, shape->box.extent.y * 0.5f, shape->box.extent.z  * 0.5f ) ), false, JPH::EActivation::DontActivate );
  }
  else if ( shape->type == CRUDE_COLLISION_SHAPE_TYPE_SPHERE )
  {
    jph_body_interface->SetShape( jph_body_id, new JPH::SphereShape( shape->sphere.radius ), false, JPH::EActivation::DontActivate );
  }
}