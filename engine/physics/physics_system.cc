
#include <flecs.h>
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

JPH_SUPPRESS_WARNINGS

#define CRUDE_PHYSICS_BODY_LAYERS_COLLIDING_DYNAMIC 1
#define CRUDE_PHYSICS_BODY_LAYERS_COLLIDING_STATIC 2

constexpr static JPH::BroadPhaseLayer jph_non_moving_broad_phase_layer_ { 0 };
constexpr static JPH::BroadPhaseLayer jph_moving_broad_phase_layer_ { 1 };
constexpr static JPH::BroadPhaseLayer jph_sensor_broad_phase_layer_ { 2 };
constexpr static uint32 jph_broad_phase_layers_count_ = 3;

static void
jolt_trace_implementation_
(
  _In_ char const                                           *fmt,
  ...
);

#ifdef JPH_ENABLE_ASSERTS
static bool
jolt_assert_failed_impl
(
  _In_ char const                                           *expression,
  _In_ char const                                           *message,
  _In_ char const                                           *file,
  _In_ uint32                                                line
);
#endif /* JPH_ENABLE_ASSERTS */

struct ObjectLayerPairFilterImpl : public JPH::ObjectLayerPairFilter
{
virtual bool
ShouldCollide
(
  _In_ JPH::ObjectLayer                                    in_object1, 
  _In_ JPH::ObjectLayer                                    in_object2
) const override;
};

struct ContactListenerImpl : public JPH::ContactListener
{
virtual JPH::ValidateResult
OnContactValidate
(
  _In_ JPH::Body const                                  &in_body1,
  _In_ JPH::Body const                                  &in_body2,
  _In_ JPH::RVec3Arg                                     in_base_offset,
  _In_ JPH::CollideShapeResult const                    &in_collision_result
) override;

virtual void
OnContactAdded
(
  _In_ JPH::Body const                                  &in_body1,
  _In_ JPH::Body const                                  &in_body2,
  _In_ JPH::ContactManifold const                       &in_manifold,
  _In_ JPH::ContactSettings                             &io_settings
) override;

virtual void
OnContactPersisted
(
  _In_ JPH::Body const                                  &in_body1,
  _In_ JPH::Body const                                  &in_body2,
  _In_ JPH::ContactManifold const                       &in_manifold,
  _In_ JPH::ContactSettings                             &io_settings
) override;

virtual void
OnContactRemoved
(
  _In_ JPH::SubShapeIDPair const                        &in_sub_shape_pair
) override;
};

struct BPLayerInterfaceImpl final : public JPH::BroadPhaseLayerInterface
{
public:
virtual JPH::uint
GetNumBroadPhaseLayers
(
) const override
{
  return jph_broad_phase_layers_count_;
}

virtual JPH::BroadPhaseLayer
GetBroadPhaseLayer
(
  _In_ JPH::ObjectLayer                                    in_layer
) const override
{
  if ( in_layer & CRUDE_PHYSICS_BODY_LAYERS_COLLIDING_DYNAMIC )
  {
    return jph_moving_broad_phase_layer_;
  }
  else if ( in_layer & CRUDE_PHYSICS_BODY_LAYERS_COLLIDING_STATIC )
  {
    return jph_non_moving_broad_phase_layer_;
  }

  return jph_sensor_broad_phase_layer_;
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
  case ( JPH::BroadPhaseLayer::Type ) jph_non_moving_broad_phase_layer_:
  {
    return "NON_MOVING";
  }
  case ( JPH::BroadPhaseLayer::Type ) jph_moving_broad_phase_layer_:
  {
    return "MOVING";
  }
  case ( JPH::BroadPhaseLayer::Type ) jph_sensor_broad_phase_layer_:
  {
    return "SENSOR";
  }
  default:
  {
    CRUDE_ASSERT( false ); 
    return "INVALID";
  }
  };
}
#endif // JPH_EXTERNAL_PROFILE || JPH_PROFILE_ENABLED
};

struct ObjectVsBroadPhaseLayerFilterImpl : public JPH::ObjectVsBroadPhaseLayerFilter
{
public:
virtual bool
ShouldCollide
(
  _In_ JPH::ObjectLayer                                                                       in_layer1,
  _In_ JPH::BroadPhaseLayer                                                                   in_layer2
) const override
{
  if ( in_layer1 & CRUDE_PHYSICS_BODY_LAYERS_COLLIDING_STATIC )
  {
    return in_layer2 == jph_moving_broad_phase_layer_;
  }
  
  if ( in_layer1 & CRUDE_PHYSICS_BODY_LAYERS_COLLIDING_DYNAMIC )
  {
    return true;
  }
  
  return false;
}
};

CRUDE_ECS_SYSTEM_DECLARE( crude_process_physics_system_ );
CRUDE_ECS_OBSERVER_DECLARE( crude_physics_dynamic_body_creation_observer_ );
CRUDE_ECS_OBSERVER_DECLARE( crude_physics_static_body_creation_observer_ );
CRUDE_ECS_OBSERVER_DECLARE( crude_physics_body_destrotion_observer_ );
CRUDE_ECS_OBSERVER_DECLARE( crude_physics_body_transform_set_observer_ );
CRUDE_ECS_OBSERVER_DECLARE( crude_physics_body_collision_shape_set_observer_ );
CRUDE_ECS_SYSTEM_DECLARE( crude_physics_body_update_transform_system_ );

JPH::TempAllocatorImpl                                    *jph_temp_allocator_;
JPH::JobSystemThreadPool                                  *jph_job_system_;
JPH::PhysicsSystem                                         jph_physics_system_;
ObjectLayerPairFilterImpl                                  jph_object_vs_object_layer_filter_;
BPLayerInterfaceImpl                                       jph_broad_phase_layer_interface_;
ObjectVsBroadPhaseLayerFilterImpl                          jph_object_vs_broadphase_layer_filter_;
ContactListenerImpl                                        jph_contact_listener_;

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
      JPH::ObjectLayer                                     jph_object_layer;

      jph_object_layer = static_body->layers;
      if ( jph_object_layer & CRUDE_PHYSICS_BODY_LAYERS_COLLIDING )
      {
        jph_object_layer = jph_object_layer | CRUDE_PHYSICS_BODY_LAYERS_COLLIDING_STATIC;
      }

      jph_box_creation_settings = JPH::BodyCreationSettings( 
        new JPH::BoxShape( JPH::Vec3( collision_shape->box.extent.x * 0.5f, collision_shape->box.extent.y * 0.5f, collision_shape->box.extent.z  * 0.5f ) ), 
        JPH::RVec3( transform->translation.x, transform->translation.y, transform->translation.z ),
        JPH::Quat( transform->rotation.x, transform->rotation.y, transform->rotation.z, transform->rotation.w ),
        JPH::EMotionType::Static,
        jph_object_layer
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
      JPH::ObjectLayer                                     jph_object_layer;

      jph_object_layer = dynamic_body->layers;
      if ( jph_object_layer & CRUDE_PHYSICS_BODY_LAYERS_COLLIDING )
      {
        jph_object_layer = jph_object_layer | CRUDE_PHYSICS_BODY_LAYERS_COLLIDING_DYNAMIC;
      }

      jph_body_creation_settings = JPH::BodyCreationSettings( 
        new JPH::SphereShape( collision_shape->sphere.radius ), 
        JPH::RVec3( transform->translation.x, transform->translation.y, transform->translation.z ),
        JPH::Quat( transform->rotation.x, transform->rotation.y, transform->rotation.z, transform->rotation.w ),
        JPH::EMotionType::Dynamic, jph_object_layer
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

static void
crude_physics_body_collision_shape_set_observer_
(
  ecs_iter_t *it
)
{
  crude_physics_body_handle                               *bodies_per_entity;
  crude_collision_shape                                   *collision_shapes_per_entity;
  JPH::BodyInterface                                      *body_interface;

  bodies_per_entity = ecs_field( it, crude_physics_body_handle, 0 );
  collision_shapes_per_entity = ecs_field( it, crude_collision_shape, 1 );

  body_interface = &jph_physics_system_.GetBodyInterface();

  for ( uint32 i = 0; i < it->count; ++i )
  {
    crude_physics_body_handle                             *physics_body_handle;
    crude_collision_shape                                 *collision_shape;
    JPH::BodyID                                            jph_body_id;
    JPH::ShapeRefC                                         jph_shape;

    physics_body_handle = &bodies_per_entity[ i ];
    collision_shape = &collision_shapes_per_entity[ i ];
    
    crude_physics_body_set_collision( physics_body_handle, collision_shape );
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
  
  CRUDE_ECS_SYSTEM_DEFINE( world, crude_physics_body_collision_shape_set_observer_, EcsOnSet, NULL, {
    { .id = ecs_id( crude_physics_body_handle ) },
    { .id = ecs_id( crude_collision_shape ) }
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

void
crude_physics_body_set_body_layer
(
  _In_ crude_physics_body_handle const                    *body,
  _In_ uint32                                              layers
)
{
  JPH::BodyInterface                                      *jph_body_interface;
  JPH::BodyID                                              jph_body_id;

  jph_body_interface = &jph_physics_system_.GetBodyInterface( );
  jph_body_id = CRUDE_COMPOUNT( JPH::BodyID, { body->body_index } );
  
  if ( layers & CRUDE_PHYSICS_BODY_LAYERS_COLLIDING )
  {
    switch ( jph_body_interface->GetMotionType( jph_body_id ) )
    {
    case JPH::EMotionType::Static:
    {
      layers = layers | CRUDE_PHYSICS_BODY_LAYERS_COLLIDING_STATIC;
      break;
    }
    case JPH::EMotionType::Dynamic:
    {
      layers = layers | CRUDE_PHYSICS_BODY_LAYERS_COLLIDING_DYNAMIC;
      break;
    }
    }
  }

  jph_body_interface->SetObjectLayer( jph_body_id, layers );
}

void
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
bool
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
}
#endif /* JPH_ENABLE_ASSERTS */

bool
ObjectLayerPairFilterImpl::ShouldCollide
(
  _In_ JPH::ObjectLayer                                    in_object1, 
  _In_ JPH::ObjectLayer                                    in_object2
) const
{
  if ( in_object1 & CRUDE_PHYSICS_BODY_LAYERS_COLLIDING_DYNAMIC )
  {
    return true;
  }

  if ( in_object1 & CRUDE_PHYSICS_BODY_LAYERS_COLLIDING_STATIC )
  {
    in_object2 & CRUDE_PHYSICS_BODY_LAYERS_COLLIDING_DYNAMIC;
  }

  return false;
}

JPH::ValidateResult
ContactListenerImpl::OnContactValidate
(
  _In_ JPH::Body const                                  &in_body1,
  _In_ JPH::Body const                                  &in_body2,
  _In_ JPH::RVec3Arg                                     in_base_offset,
  _In_ JPH::CollideShapeResult const                    &in_collision_result
)
{
  //cout << "Contact validate callback" << endl;

  // Allows you to ignore a contact before it is created (using layers to not make objects collide is cheaper!)
  return JPH::ValidateResult::AcceptAllContactsForThisBodyPair;
}


void
ContactListenerImpl::OnContactAdded
(
  _In_ JPH::Body const                                  &in_body1,
  _In_ JPH::Body const                                  &in_body2,
  _In_ JPH::ContactManifold const                       &in_manifold,
  _In_ JPH::ContactSettings                             &io_settings
) 
{
  //cout << "A contact was added" << endl;
}

void
ContactListenerImpl::OnContactPersisted
(
  _In_ JPH::Body const                                  &in_body1,
  _In_ JPH::Body const                                  &in_body2,
  _In_ JPH::ContactManifold const                       &in_manifold,
  _In_ JPH::ContactSettings                             &io_settings
)
{
  //cout << "A contact was persisted" << endl;
}

void
ContactListenerImpl::OnContactRemoved
(
  _In_ JPH::SubShapeIDPair const                        &in_sub_shape_pair
)
{
  //cout << "A contact was removed" << endl;
}