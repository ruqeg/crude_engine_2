
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

#include <core/log.h>
#include <core/assert.h>
#include <scene/scene_components.h>
#include <physics/physics_components.h>

#include <physics/physics_system.h>

/* Disable common warnings triggered by Jolt, you can use JPH_SUPPRESS_WARNING_PUSH / JPH_SUPPRESS_WARNING_POP to store and restore the warning state */
JPH_SUPPRESS_WARNINGS

// Callback for traces, connect this to your own trace function if you have one
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
/* Callback for asserts, connect this to your own assert handler if you have one */
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
  /* Breakpoint */
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
    /* Create a mapping table from object to broad phase layer */
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

/* An example contact listener */
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

// An example activation listener
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
CRUDE_ECS_OBSERVER_DECLARE( crude_physics_dynamic_body_destrotion_observer_ );
CRUDE_ECS_OBSERVER_DECLARE( crude_physics_static_body_creation_observer_ );
CRUDE_ECS_OBSERVER_DECLARE( crude_physics_static_body_destrotion_observer_ );

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
  crude_physics_static_body *static_bodies_per_entity = ecs_field( it, crude_physics_static_body, 0 );

  // The main way to interact with the bodies in the physics system is through the body interface. There is a locking and a non-locking
  // variant of this. We're going to use the locking version (even though we're not planning to access bodies from multiple threads)
  JPH::BodyInterface &body_interface = jph_physics_system_.GetBodyInterface();

  for ( uint32 i = 0; i < it->count; ++i )
  {
    ecs_world_t                                           *world;
    crude_physics_static_body                             *static_body;
    crude_transform                                       *transform;
    crude_physics_static_body_handle                       *static_body_handle;
    JPH::ShapeSettings::ShapeResult                        shape_result;
    crude_entity                                           entity;

    world = it->world;
    static_body = &static_bodies_per_entity[ i ];
    entity = CRUDE_COMPOUNT( crude_entity, { it->entities[ i ], world } );

    CRUDE_ASSERT( CRUDE_ENTITY_HAS_COMPONENT( entity, crude_transform ) );
    transform = CRUDE_ENTITY_GET_MUTABLE_COMPONENT( entity, crude_transform );
    CRUDE_ASSERT( transform );

    static_body_handle = ecs_ensure( world, entity.handle, crude_physics_static_body_handle );

    // Next we can create a rigid body to serve as the floor, we make a large box
    // Create the settings for the collision volume (the shape).
    // Note that for simple shapes (like boxes) you can also directly construct a BoxShape.
    if ( static_body->collision_shape_type == CRUDE_PHYSICS_COLLISION_SHAPE_TYPE_BOX )
    {
      crude_physics_box_collision_shape                   *collision_shape;
      JPH::Body                                           *body;
      JPH::BoxShapeSettings                                jph_shape_settings;
      JPH::ShapeSettings::ShapeResult                      shape_result;
      JPH::ShapeRefC                                       shape_ref;
      JPH::BodyCreationSettings                            settings;

      collision_shape = &static_body->box_shape;

      jph_shape_settings = JPH::BoxShapeSettings( JPH::Vec3( collision_shape->half_extent.x, collision_shape->half_extent.y, collision_shape->half_extent.z ) );
      jph_shape_settings.SetEmbedded( ); // A ref counted object on the stack (base class RefTarget) should be marked as such to prevent it from being freed when its reference count goes to 0.
      
      // Create the shape
      shape_result = jph_shape_settings.Create( );
      shape_ref = shape_result.Get( ); // We don't expect an error here, but you can check floor_shape_result for HasError() / GetError()

      // Create the settings for the body itself. Note that here you can also set other properties like the restitution / friction.
      settings = JPH::BodyCreationSettings( shape_ref, JPH::RVec3( transform->translation.x, transform->translation.y, transform->translation.z ), JPH::Quat( transform->rotation.x, transform->rotation.y, transform->rotation.z, transform->rotation.w ), JPH::EMotionType::Static, non_moving_object_layer_ );

      // Create the actual rigid body
      body = body_interface.CreateBody( settings ); // Note that if we run out of bodies this can return nullptr

      // Add it to the world
      body_interface.AddBody( body->GetID( ), JPH::EActivation::DontActivate );

      static_body_handle->static_body_index = body->GetID( ).GetIndexAndSequenceNumber( );
    }
    else
    {
      CRUDE_ASSERT( false );
    }
  }
}

static void
crude_physics_static_body_destrotion_observer_
(
  ecs_iter_t *it
)
{
  crude_physics_static_body *static_bodies_per_entity = ecs_field( it, crude_physics_static_body, 0 );
  crude_physics_static_body_handle *static_bodies_values_per_entity = ecs_field( it, crude_physics_static_body_handle, 1 );

  JPH::BodyInterface &body_interface = jph_physics_system_.GetBodyInterface();

  for ( uint32 i = 0; i < it->count; ++i )
  {
    crude_physics_static_body                             *static_body;
    crude_physics_static_body_handle                       *static_body_value;
    ecs_world_t                                           *world;
    JPH::BodyID                                            body_id;

    world = it->world;
    static_body = &static_bodies_per_entity[ i ];
    static_body_value = &static_bodies_values_per_entity[ i ];

    if ( static_body->collision_shape_type == CRUDE_PHYSICS_COLLISION_SHAPE_TYPE_SPHERE )
    {
      body_id = JPH::BodyID{ static_body_value->static_body_index };
      body_interface.RemoveBody( body_id );
      body_interface.DestroyBody( body_id );
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
  crude_physics_dynamic_body *dynamic_bodies_per_entity = ecs_field( it, crude_physics_dynamic_body, 0 );

  // The main way to interact with the bodies in the physics system is through the body interface. There is a locking and a non-locking
  // variant of this. We're going to use the locking version (even though we're not planning to access bodies from multiple threads)
  JPH::BodyInterface &body_interface = jph_physics_system_.GetBodyInterface();

  for ( uint32 i = 0; i < it->count; ++i )
  {
    ecs_world_t                                           *world;
    crude_physics_dynamic_body                            *dynamic_body;
    crude_transform                                       *transform;
    crude_physics_dynamic_body_handle                      *dynamic_body_handle;
    JPH::ShapeSettings::ShapeResult                        shape_result;
    crude_entity                                           entity;

    world = it->world;
    dynamic_body = &dynamic_bodies_per_entity[ i ];
    entity = CRUDE_COMPOUNT( crude_entity, { it->entities[ i ], world } );

    CRUDE_ASSERT( CRUDE_ENTITY_HAS_COMPONENT( entity, crude_transform ) );
    transform = CRUDE_ENTITY_GET_MUTABLE_COMPONENT( entity, crude_transform );
    CRUDE_ASSERT( transform );

    dynamic_body_handle = ecs_ensure( world, entity.handle, crude_physics_dynamic_body_handle );

    // Next we can create a rigid body to serve as the floor, we make a large box
    // Create the settings for the collision volume (the shape).
    // Note that for simple shapes (like boxes) you can also directly construct a BoxShape.
    if ( dynamic_body->collision_shape_type == CRUDE_PHYSICS_COLLISION_SHAPE_TYPE_SPHERE )
    {
      JPH::BodyCreationSettings                            sphere_settings;
      JPH::BodyID                                          sphere_id;
      
      sphere_settings = JPH::BodyCreationSettings( new JPH::SphereShape( dynamic_body->sphere_shape.radius ), JPH::RVec3( transform->translation.x, transform->translation.y, transform->translation.z ), JPH::Quat( transform->rotation.x, transform->rotation.y, transform->rotation.z, transform->rotation.w ), JPH::EMotionType::Dynamic, moving_object_layer_ );
      sphere_id = body_interface.CreateAndAddBody( sphere_settings, JPH::EActivation::Activate );
      dynamic_body_handle->dynamic_body_index = sphere_id.GetIndexAndSequenceNumber( );
    }
    else
    {
      CRUDE_ASSERT( false );
    }
  }
}

static void
crude_physics_dynamic_body_destrotion_observer_
(
  ecs_iter_t *it
)
{
  crude_physics_dynamic_body *dynamic_bodies_per_entity = ecs_field( it, crude_physics_dynamic_body, 0 );
  crude_physics_dynamic_body_handle *dynamic_bodies_values_per_entity = ecs_field( it, crude_physics_dynamic_body_handle, 1 );

  JPH::BodyInterface &body_interface = jph_physics_system_.GetBodyInterface();

  for ( uint32 i = 0; i < it->count; ++i )
  {
    crude_physics_dynamic_body                            *dynamic_body;
    crude_physics_dynamic_body_handle                      *dynamic_body_value;
    ecs_world_t                                           *world;
    JPH::BodyID                                            body_id;

    world = it->world;
    dynamic_body = &dynamic_bodies_per_entity[ i ];
    dynamic_body_value = &dynamic_bodies_values_per_entity[ i ];

    if ( dynamic_body->collision_shape_type == CRUDE_PHYSICS_COLLISION_SHAPE_TYPE_SPHERE )
    {
      body_id = JPH::BodyID{ dynamic_body_value->dynamic_body_index };
      body_interface.RemoveBody( body_id );
      body_interface.DestroyBody( body_id );
    }
    else
    {
      CRUDE_ASSERT( false );
    }
  }
}

void
crude_physics_update
(
  _In_ float32                                             delta_time
)
{
  // The main way to interact with the bodies in the physics system is through the body interface. There is a locking and a non-locking
  // variant of this. We're going to use the locking version (even though we're not planning to access bodies from multiple threads)
  JPH::BodyInterface &body_interface = jph_physics_system_.GetBodyInterface( );


  // Now you can interact with the dynamic body, in this case we're going to give it a velocity.
  // (note that if we had used CreateBody then we could have set the velocity straight on the body before adding it to the physics system)
  //body_interface.SetLinearVelocity(sphere_id, JPH::Vec3(0.0f, -5.0f, 0.0f));

  // We simulate the physics world in discrete time steps. 60 Hz is a good rate to update the physics system.
  //const float cDeltaTime = 1.0f / 60.0f;

  // Optional step: Before starting the physics simulation you can optimize the broad phase. This improves collision detection performance (it's pointless here because we only have 2 bodies).
  // You should definitely not call this every frame or when e.g. streaming in a new level section as it is an expensive operation.
  // Instead insert all new objects in batches instead of 1 at a time to keep the broad phase efficient.
  //jph_physics_system_.OptimizeBroadPhase( );

  // Now we're ready to simulate the body, keep simulating until it goes to sleep

  // Output current position and velocity of the sphere
  //JPH::RVec3 position = body_interface.GetCenterOfMassPosition( sphere_id );
  //JPH::Vec3 velocity = body_interface.GetLinearVelocity( sphere_id );
  //cout << "Step " << step << ": Position = (" << position.GetX() << ", " << position.GetY() << ", " << position.GetZ() << "), Velocity = (" << velocity.GetX() << ", " << velocity.GetY() << ", " << velocity.GetZ() << ")" << endl;

  // If you take larger steps than 1 / 60th of a second you need to do multiple collision steps in order to keep the simulation stable. Do 1 collision step per 1 / 60th of a second (round up).
  const int cCollisionSteps = 1;
  jph_physics_system_.Update( delta_time, cCollisionSteps, jph_temp_allocator_, jph_job_system_ );
}

CRUDE_ECS_MODULE_IMPORT_IMPL( crude_physics_system )
{
  ECS_MODULE( world, crude_physics_system );
  ECS_IMPORT( world, crude_physics_components );
  
  CRUDE_ECS_OBSERVER_DEFINE( world, crude_physics_static_body_creation_observer_, EcsOnSet, { 
    { .id = ecs_id( crude_physics_static_body ) },
    { .id = ecs_id( crude_physics_static_body_handle ), .oper = EcsNot }
  } );

  CRUDE_ECS_OBSERVER_DEFINE( world, crude_physics_static_body_destrotion_observer_, EcsOnRemove, { 
    { .id = ecs_id( crude_physics_static_body_handle ) }
  } );
  
  CRUDE_ECS_OBSERVER_DEFINE( world, crude_physics_dynamic_body_creation_observer_, EcsOnSet, { 
    { .id = ecs_id( crude_physics_dynamic_body ) },
    { .id = ecs_id( crude_physics_dynamic_body_handle ), .oper = EcsNot }
  } );

  CRUDE_ECS_OBSERVER_DEFINE( world, crude_physics_dynamic_body_destrotion_observer_, EcsOnRemove, { 
    { .id = ecs_id( crude_physics_dynamic_body_handle ) }
  } );
}

void
crude_physics_initialize
(
  _In_ crude_physics_creation const                                                *creation
)
{
  // Register allocation hook. In this example we'll just let Jolt use malloc / free but you can override these if you want (see Memory.h).
  // This needs to be done before any other Jolt function is called.
  JPH::RegisterDefaultAllocator( );

  // Install trace and assert callbacks
  JPH::Trace = jolt_trace_implementation_;
  JPH_IF_ENABLE_ASSERTS( JPH::AssertFailed = jolt_assert_failed_impl; )

  // Create a factory, this class is responsible for creating instances of classes based on their name or hash and is mainly used for deserialization of saved data.
  // It is not directly used in this example but still required.
  JPH::Factory::sInstance = new JPH::Factory( );

  // Register all physics types with the factory and install their collision handlers with the CollisionDispatch class.
  // If you have your own custom shape types you probably need to register their handlers with the CollisionDispatch before calling this function.
  // If you implement your own default material (PhysicsMaterial::sDefault) make sure to initialize it before this function or else this function will create one for you.
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

  // Unregisters all types with the factory and cleans up the default material
  JPH::UnregisterTypes();

  // Destroy the factory
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