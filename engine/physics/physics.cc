#include <thirdparty/flecs/flecs.h>
#include <stdarg.h>

#include <engine/core/time.h>
#include <engine/core/memory.h>
#include <engine/core/log.h>
#include <engine/core/array.h>
#include <engine/core/hashmapstr.h>
#include <engine/core/assert.h>
#include <engine/scene/scene_ecs.h>
#include <engine/physics/physics_ecs.h>

#include <engine/physics/physics.h>

JPH_SUPPRESS_WARNINGS

crude_heap_allocator                                      *g_physics_heap_allocator_;

static void*
crude_physics_jph_allocate_implementation
(
  _In_ size_t                                              size
)
{
  JPH_ASSERT( size > 0 );
  return crude_heap_allocator_allocate_align( g_physics_heap_allocator_, size, 16 );
}

static void*
crude_physics_jph_reallocate_implementation
(
  _In_ void                                               *block,
  _In_ [[maybe_unused]] size_t                             old_size,
  _In_ size_t                                              new_size
)
{
  JPH_ASSERT( new_size > 0 );
  return crude_heap_allocator_reallocate( g_physics_heap_allocator_, block, new_size );
}

static void
crude_physics_jph_free_implementation
(
  _In_ void                                               *block
)
{
  crude_heap_allocator_deallocate( g_physics_heap_allocator_, block );
}

static void*
crude_physics_jph_aligned_allocate_implementation
(
  _In_ size_t                                              size,
  _In_ size_t                                              alignment
)
{
  JPH_ASSERT( size > 0 && alignment > 0 );
  return crude_heap_allocator_allocate_align( g_physics_heap_allocator_, size, alignment );
}

static void
crude_physics_jph_aligned_free_implementation
(
  _In_ void                                               *block
)
{
  crude_heap_allocator_deallocate( g_physics_heap_allocator_, block );
}

#if defined(JPH_ENABLE_ASSERTS)

static void
crude_physics_jolt_trace_impl_
(
  _In_ char const                                          *fmt,
  _In_ ...
);

static bool
crude_physics_jolt_assert_failed_impl_
(
  _In_ char const                                          *expression,
  _In_ char const                                          *message,
  _In_ char const                                          *file,
  _In_ uint32                                               line
);

#endif /* JPH_ENABLE_ASSERTS */

bool
_crude_jph_object_layer_pair_filter_class::ShouldCollide
(
  _In_ JPH::ObjectLayer                                    object1,
  _In_ JPH::ObjectLayer                                    object2
) const
{
  switch ( object1 )
  {
  case g_crude_jph_layer_non_moving:
  {
    return object2 == g_crude_jph_layer_moving;
  }
  case g_crude_jph_layer_moving:
  {
    return true;
  }
  default:
  {
    JPH_ASSERT(false);
    return false;
  }
  }
}

_crude_jph_bp_layer_interface_class::_crude_jph_bp_layer_interface_class
(
)
{
  this->object_to_broad_phase[ g_crude_jph_layer_non_moving ] = g_crude_jph_broad_phase_layer_non_moving_class;
  this->object_to_broad_phase[ g_crude_jph_layer_moving ] = g_crude_jph_broad_phase_layer_moving_class;
}

JPH::uint
_crude_jph_bp_layer_interface_class::GetNumBroadPhaseLayers
(
) const
{
  return g_crude_jph_broad_phase_layer_num_layers;
}

JPH::BroadPhaseLayer
_crude_jph_bp_layer_interface_class::GetBroadPhaseLayer
(
  _In_ JPH::ObjectLayer                                    layer
) const
{
  JPH_ASSERT( layer < g_crude_jph_num_layers );
  return this->object_to_broad_phase[ layer ];
}

#if defined( JPH_EXTERNAL_PROFILE ) || defined( JPH_PROFILE_ENABLED )
char const*
_crude_jph_bp_layer_interface_class::GetBroadPhaseLayerName
(
  _In_ JPH::BroadPhaseLayer                              layer
) const
{
  switch ( CRUDE_CAST( JPH::BroadPhaseLayer::Type, layer ))
  {
  case CRUDE_CAST( JPH::BroadPhaseLayer::Type, g_crude_jph_broad_phase_layer_non_moving_class ):
  {
    return "NON_MOVING";
  }
  case CRUDE_CAST( JPH::BroadPhaseLayer::Type, g_crude_jph_broad_phase_layer_moving_class ):
  {
    return "MOVING";
  }
  default:
  {
    JPH_ASSERT(false);
    return "INVALID";
  }
  }
}
#endif /* JPH_EXTERNAL_PROFILE || JPH_PROFILE_ENABLED */

bool
_crude_jph_object_vs_broad_phase_layer_filter::ShouldCollide
(
  _In_ JPH::ObjectLayer                                  layer1,
  _In_ JPH::BroadPhaseLayer                              layer2
) const
{
  switch ( layer1 )
  {
  case g_crude_jph_layer_non_moving:
  {
    return layer2 == g_crude_jph_broad_phase_layer_moving_class;
  }
  case g_crude_jph_layer_moving:
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

JPH::ValidateResult
_crude_jph_contact_listener_class::OnContactValidate
(
  _In_ JPH::Body const                                    &body1,
  _In_ JPH::Body const                                    &body2,
  _In_ JPH::RVec3Arg                                       in_base_offset,
  _In_ JPH::CollideShapeResult const                      &in_collision_result
)
{
  //cout << "Contact validate callback" << endl;
  // Allows you to ignore a contact before it is created (using layers to not make objects collide is cheaper!)
  return JPH::ValidateResult::AcceptAllContactsForThisBodyPair;
}

void
_crude_jph_contact_listener_class::OnContactAdded
(
  _In_ const JPH::Body                                    &body1,
  _In_ const JPH::Body                                    &body2,
  _In_ const JPH::ContactManifold                         &manifold,
  _In_ JPH::ContactSettings                               &settings
)
{
  //cout << "A contact was added" << endl;
}

void
_crude_jph_contact_listener_class::OnContactPersisted
(
  _In_ const JPH::Body                                    &body1,
  _In_ const JPH::Body                                    &body2,
  _In_ const JPH::ContactManifold                         &manifold,
  _In_ JPH::ContactSettings                               &settings
)
{
  //cout << "A contact was persisted" << endl;
}

void
_crude_jph_contact_listener_class::OnContactRemoved
(
  _In_ const JPH::SubShapeIDPair                        &subshapepair
)
{
  //cout << "A contact was removed" << endl;
}

void
_crude_jph_body_activation_listener_class::OnBodyActivated
(
  _In_ const JPH::BodyID                                  &body_id,
  _In_ uint64                                              body_user_data
)
{
  //cout << "A body got activated" << endl;
}

void
_crude_jph_body_activation_listener_class::OnBodyDeactivated
(
  _In_ const JPH::BodyID                                  &body_id,
  _In_ uint64                                              body_user_data
)
{
  //cout << "A body went to sleep" << endl;
}

void
crude_physics_initialize
(
  _In_ crude_physics                                      *physics,
  _In_ crude_physics_creation const                       *creation,
  _In_ crude_ecs                                          *world
)
{
  physics->physics_allocator = creation->physics_allocator;
  physics->physics_allocator_container = crude_heap_allocator_pack( creation->physics_allocator );

  physics->last_update_time = crude_time_now( );
  physics->simulation_enabled = true;

  crude_resource_pool_initialize( &physics->characters_resource_pool, physics->physics_allocator_container, 16, sizeof( crude_physics_character_container ) );
  crude_resource_pool_initialize( &physics->static_body_resource_pool, physics->physics_allocator_container, 256, sizeof( crude_physics_static_body_container ) );

  g_physics_heap_allocator_ = creation->physics_allocator;
  JPH::Allocate = crude_physics_jph_allocate_implementation;
  JPH::Reallocate = crude_physics_jph_reallocate_implementation;
  JPH::Free = crude_physics_jph_free_implementation;
  JPH::AlignedAllocate = crude_physics_jph_aligned_allocate_implementation;
  JPH::AlignedFree = crude_physics_jph_aligned_free_implementation;
  
  JPH::Trace = crude_physics_jolt_trace_impl_;

  JPH_IF_ENABLE_ASSERTS( JPH::AssertFailed = crude_physics_jolt_assert_failed_impl_ ;);

  JPH::Factory::sInstance = new JPH::Factory( );

  JPH::RegisterTypes( );
  
  physics->jph_temporary_allocator_class = CRUDE_JOLT_OVERRIDEN_NEW JPH::TempAllocatorImpl( 10 * 1024 * 1024 );
  physics->jph_job_system_class = CRUDE_JOLT_OVERRIDEN_NEW JPH::JobSystemThreadPool( JPH::cMaxPhysicsJobs, JPH::cMaxPhysicsBarriers, 1u );
  physics->jph_physics_system_class = CRUDE_JOLT_OVERRIDEN_NEW JPH::PhysicsSystem( );

  physics->jph_broad_phase_layer_interface_class = CRUDE_ALLOCATE_AND_CONSTRUCT( physics->physics_allocator_container, _crude_jph_bp_layer_interface_class );
  physics->jph_object_vs_broadphase_layer_filter_class = CRUDE_ALLOCATE_AND_CONSTRUCT( physics->physics_allocator_container, _crude_jph_object_vs_broad_phase_layer_filter );
  physics->jph_object_vs_object_layer_filter_class = CRUDE_ALLOCATE_AND_CONSTRUCT( physics->physics_allocator_container, _crude_jph_object_layer_pair_filter_class );
  
  physics->jph_body_activation_listener_class = CRUDE_ALLOCATE_AND_CONSTRUCT( physics->physics_allocator_container, _crude_jph_body_activation_listener_class );
  physics->jph_contact_listener_class = CRUDE_ALLOCATE_AND_CONSTRUCT( physics->physics_allocator_container, _crude_jph_contact_listener_class );

  physics->jph_physics_system_class->Init(
    CRUDE_PHYSICS_JOLT_MAX_BODIES, CRUDE_PHYSICS_JOLT_NUM_BODIES_MUTEXES, CRUDE_PHYSICS_JOLT_MAX_BODIES_PAIRS, CRUDE_PHYSICS_JOLT_MAX_CONTACT_CONSTRAINTS,
    *physics->jph_broad_phase_layer_interface_class, *physics->jph_object_vs_broadphase_layer_filter_class, *physics->jph_object_vs_object_layer_filter_class );

  physics->jph_physics_system_class->SetBodyActivationListener( physics->jph_body_activation_listener_class );
  physics->jph_physics_system_class->SetContactListener( physics->jph_contact_listener_class );
  
  //physics->jph_physics_system_class->OptimizeBroadPhase( );

  physics->physics_shapes_manager = creation->physics_shapes_manager;
}

void
crude_physics_deinitialize
(
  _In_ crude_physics                                      *physics
)
{
  JPH::BodyInterface                                      *jph_body_interface_class;

  jph_body_interface_class = &physics->jph_physics_system_class->GetBodyInterface( );
  
  JPH::UnregisterTypes();
  
  delete JPH::Factory::sInstance;
  JPH::Factory::sInstance = nullptr;
  
  CRUDE_JOLT_OVERRIDEN_FREE physics->jph_temporary_allocator_class;
  CRUDE_JOLT_OVERRIDEN_FREE physics->jph_job_system_class;
  CRUDE_JOLT_OVERRIDEN_FREE physics->jph_physics_system_class;

  CRUDE_DEALLOCATE_AND_DECONSTRUCT( physics->physics_allocator_container, physics->jph_broad_phase_layer_interface_class, _crude_jph_bp_layer_interface_class );
  CRUDE_DEALLOCATE_AND_DECONSTRUCT( physics->physics_allocator_container, physics->jph_object_vs_broadphase_layer_filter_class, _crude_jph_object_vs_broad_phase_layer_filter );
  CRUDE_DEALLOCATE_AND_DECONSTRUCT( physics->physics_allocator_container, physics->jph_object_vs_object_layer_filter_class, _crude_jph_object_layer_pair_filter_class );
  CRUDE_DEALLOCATE_AND_DECONSTRUCT( physics->physics_allocator_container, physics->jph_body_activation_listener_class, _crude_jph_body_activation_listener_class );
  CRUDE_DEALLOCATE_AND_DECONSTRUCT( physics->physics_allocator_container, physics->jph_contact_listener_class, _crude_jph_contact_listener_class );
  
  crude_resource_pool_deinitialize( &physics->characters_resource_pool );
  crude_resource_pool_deinitialize( &physics->static_body_resource_pool );
}

void
crude_physics_update
(
  _In_ crude_physics                                      *physics,
  _In_ int64                                               current_time
)
{
  if ( !physics->simulation_enabled )
  {
    return;
  }

  if ( crude_time_delta_seconds( physics->last_update_time, current_time ) < CRUDE_PHYSICS_JOLT_DELTA_TIME )
  {
    return;
  }

  physics->jph_physics_system_class->Update( CRUDE_PHYSICS_JOLT_DELTA_TIME, CRUDE_PHYSICS_JOLT_COLLISIONS_STEPS, physics->jph_temporary_allocator_class, physics->jph_job_system_class );
  physics->last_update_time = current_time;
}

void
crude_physics_enable_simulation
(
  _In_ crude_physics                                      *physics,
  _In_ ecs_world_t                                        *world,
  _In_ bool                                                enable
)
{
  physics->simulation_enabled = enable;
  crude_entity_enable( world, crude_ecs_on_pre_physics_update, enable );
  crude_entity_enable( world, crude_ecs_on_post_physics_update, enable );
}

crude_physics_character_creation
crude_physics_character_creation_empty
(
)
{
  crude_physics_character_creation creation = CRUDE_COMPOUNT_EMPTY( crude_physics_character_creation );
  return creation;
}

crude_physics_static_body_creation
crude_physics_static_body_creation_empty
(
)
{
  crude_physics_static_body_creation creation = CRUDE_COMPOUNT_EMPTY( crude_physics_static_body_creation );
  return creation;
}

crude_physics_character_handle
crude_physics_create_character
(
  _In_ crude_physics                                      *physics,
  _In_ crude_physics_character_creation const             *creation
)
{
  crude_physics_character_container                       *character_container;
  crude_physics_character_handle                           handle;
  JPH::Ref< JPH::CharacterSettings >                       jph_settings_class;
  JPH::RefConst< JPH::Shape >                              jph_standing_shape;
    
  jph_standing_shape = JPH::RotatedTranslatedShapeSettings(
    JPH::Vec3( 0, 0.5f * creation->character_height_standing + creation->character_radius_standing, 0 ),
    JPH::Quat::sIdentity( ),
    CRUDE_JOLT_OVERRIDEN_NEW JPH::CapsuleShape( 0.5f * creation->character_height_standing, creation->character_radius_standing )
  ).Create( ).Get( );
    
  jph_settings_class = CRUDE_JOLT_OVERRIDEN_NEW JPH::CharacterSettings( );
  jph_settings_class->mMaxSlopeAngle = creation->max_slop_angle;
  jph_settings_class->mLayer = g_crude_jph_layer_moving;
  jph_settings_class->mShape = jph_standing_shape;
  jph_settings_class->mFriction = creation->friction;
  jph_settings_class->mSupportingVolume = JPH::Plane( JPH::Vec3::sAxisY( ), -creation->character_radius_standing );
    
  handle.index = crude_resource_pool_obtain_resource( &physics->characters_resource_pool );

  character_container = crude_physics_access_character( physics, handle );
  CRUDE_CXX_CONSTRUCTOR( &character_container->jph_character_class, JPH::Ref< JPH::Character > );

  character_container->jph_character_class = CRUDE_JOLT_OVERRIDEN_NEW JPH::Character(
    jph_settings_class,
    JPH::RVec3::sZero( ),
    JPH::Quat::sIdentity( ),
    0,
    physics->jph_physics_system_class );

  character_container->jph_character_class->AddToPhysicsSystem( JPH::EActivation::Activate );
  
  CRUDE_CXX_CONSTRUCTOR( &character_container->manually_stored_transform, JPH::Mat44 );

  return handle;
}

crude_physics_character_container*
crude_physics_access_character
(
  _In_ crude_physics                                      *physics,
  _In_ crude_physics_character_handle                      handle
)
{
  return CRUDE_CAST( crude_physics_character_container*, crude_resource_pool_access_resource( &physics->characters_resource_pool, handle.index ) );
}

CRUDE_API void
crude_physics_destroy_character_instant
(
  _In_ crude_physics                                      *physics,
  _In_ crude_physics_character_handle                      handle
)
{
  crude_physics_character_container                       *character_container;

  character_container = crude_physics_access_character( physics, handle );

  character_container->jph_character_class->RemoveFromPhysicsSystem( );
  character_container->jph_character_class.~Ref( );

  crude_resource_pool_release_resource( &physics->characters_resource_pool, handle.index );
}


crude_physics_static_body_handle
crude_physics_create_static_body
(
  _In_ crude_physics                                      *physics,
  _In_ crude_physics_static_body_creation const           *creation
)
{
  crude_physics_static_body_container                     *static_body_container;
  JPH::BodyInterface                                      *jph_body_interface_class;
  JPH::ShapeSettings::ShapeResult                          jph_shape_result_class;
  JPH::ShapeRefC                                           jph_shape_class;
  JPH::BodyCreationSettings                                jph_settings_class;
  crude_physics_static_body_handle                         handle;
    
  handle.index = crude_resource_pool_obtain_resource( &physics->static_body_resource_pool );

  static_body_container = crude_physics_access_static_body( physics, handle );

  jph_body_interface_class = &physics->jph_physics_system_class->GetBodyInterface( );
  
  if ( creation->type == CRUDE_PHYSICS_STATIC_BODY_SHAPE_TYPE_BOX )
  {
    JPH::BoxShapeSettings                                    jph_shape_settings_class;
    jph_shape_settings_class = CRUDE_COMPOUNT( JPH::BoxShapeSettings, { JPH::Vec3( creation->box.extent.x, creation->box.extent.y, creation->box.extent.z ) } );
    jph_shape_settings_class.SetEmbedded( );
    jph_shape_result_class = jph_shape_settings_class.Create( );
    jph_shape_class = jph_shape_result_class.Get( );
  }
  else if ( creation->type == CRUDE_PHYSICS_STATIC_BODY_SHAPE_TYPE_MESH )
  {
    jph_shape_class = crude_physics_shapes_manager_access_mesh_shape( physics->physics_shapes_manager, creation->mesh.handle )->jph_shape_class;
  }

  jph_settings_class = JPH::BodyCreationSettings( jph_shape_class, JPH::RVec3( 0.0, 0.0, 0.0 ), JPH::Quat::sIdentity( ), JPH::EMotionType::Static, g_crude_jph_layer_non_moving | creation->layers );

  CRUDE_CXX_CONSTRUCTOR( &static_body_container->jph_body_class, JPH::BodyID, jph_body_interface_class->CreateAndAddBody( jph_settings_class, JPH::EActivation::DontActivate ) );

  return handle;
}

crude_physics_static_body_container*
crude_physics_access_static_body
(
  _In_ crude_physics                                      *physics,
  _In_ crude_physics_static_body_handle                    handle
)
{ 
  return CRUDE_CAST( crude_physics_static_body_container*, crude_resource_pool_access_resource( &physics->static_body_resource_pool, handle.index ) );
}

void
crude_physics_destroy_static_body_instant
(
  _In_ crude_physics                                      *physics,
  _In_ crude_physics_static_body_handle                    handle
)
{
  JPH::BodyInterface                                      *jph_body_interface_class;
  crude_physics_static_body_container                     *static_body_container;

  static_body_container = crude_physics_access_static_body( physics, handle );
  
  jph_body_interface_class = &physics->jph_physics_system_class->GetBodyInterface( );

  jph_body_interface_class->RemoveBody( static_body_container->jph_body_class );
  jph_body_interface_class->DestroyBody( static_body_container->jph_body_class );

  crude_resource_pool_release_resource( &physics->static_body_resource_pool, handle.index );
}

#if defined(JPH_ENABLE_ASSERTS)

static void
crude_physics_jolt_trace_impl_
(
  _In_ char const                                          *fmt,
  _In_ ...
)
{
  char                                                    buffer[ 1024 ];
  va_list                                                  list;

  va_start( list, fmt );
  vsnprintf( buffer, sizeof( buffer ), fmt, list );
  va_end( list );

  CRUDE_LOG_ERROR( CRUDE_CHANNEL_PHYSICS, buffer );
}

static bool
crude_physics_jolt_assert_failed_impl_
(
  _In_ char const                                          *expression,
  _In_ char const                                          *message,
  _In_ char const                                          *file,
  _In_ uint32                                               line
)
{
  CRUDE_LOG_ERROR( CRUDE_CHANNEL_PHYSICS, "%s:%i: (%s) %s",  file, line, expression, message ? message : "" );
  return true;
}

#endif /* JPH_ENABLE_ASSERTS */