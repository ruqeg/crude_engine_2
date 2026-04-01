#include <thirdparty/flecs/flecs.h>
#include <stdarg.h>

#include <engine/core/memory.h>
#include <engine/core/log.h>
#include <engine/core/array.h>
#include <engine/core/hashmapstr.h>
#include <engine/core/assert.h>
#include <engine/scene/scene_ecs.h>
#include <engine/physics/physics_ecs.h>

#include <engine/physics/physics.h>

JPH_SUPPRESS_WARNINGS

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
  JPH::BodyInterface                                      *jph_body_interface_class;
  JPH::BoxShapeSettings                                    jph_floor_shape_settings_class;
  JPH::ShapeSettings::ShapeResult                          jph_floor_shape_result_class;
  JPH::ShapeRefC                                           jph_floor_shape_class;
  JPH::BodyCreationSettings                                jph_floor_settings_class;
  JPH::BodyCreationSettings                                jph_sphere_settings_class;

  ecs_query_desc_t                                         query_desc;

  physics->allocator = creation->allocator;
  physics->allocator_container = crude_heap_allocator_pack( creation->allocator );

  JPH::RegisterDefaultAllocator( );

  JPH::Trace = crude_physics_jolt_trace_impl_;
  JPH_IF_ENABLE_ASSERTS( JPH::AssertFailed = crude_physics_jolt_assert_failed_impl_ ;);

  JPH::Factory::sInstance = new JPH::Factory( );

  JPH::RegisterTypes( );
  
  physics->jph_temporary_allocator_class = CRUDE_ALLOCATE_AND_CONSTRUCT( physics->allocator_container, JPH::TempAllocatorImpl, 10 * 1024 * 1024 );
  physics->jph_job_system_class = CRUDE_ALLOCATE_AND_CONSTRUCT( physics->allocator_container, JPH::JobSystemThreadPool, JPH::cMaxPhysicsJobs, JPH::cMaxPhysicsBarriers, 1u );
  physics->jph_physics_system_class = CRUDE_ALLOCATE_AND_CONSTRUCT( physics->allocator_container, JPH::PhysicsSystem );

  physics->jph_broad_phase_layer_interface_class = CRUDE_ALLOCATE_AND_CONSTRUCT( physics->allocator_container, _crude_jph_bp_layer_interface_class );
  physics->jph_object_vs_broadphase_layer_filter_class = CRUDE_ALLOCATE_AND_CONSTRUCT( physics->allocator_container, _crude_jph_object_vs_broad_phase_layer_filter );
  physics->jph_object_vs_object_layer_filter_class = CRUDE_ALLOCATE_AND_CONSTRUCT( physics->allocator_container, _crude_jph_object_layer_pair_filter_class );
  
  physics->jph_body_activation_listener_class = CRUDE_ALLOCATE_AND_CONSTRUCT( physics->allocator_container, _crude_jph_body_activation_listener_class );
  physics->jph_contact_listener_class = CRUDE_ALLOCATE_AND_CONSTRUCT( physics->allocator_container, _crude_jph_contact_listener_class );

  physics->jph_physics_system_class->Init(
    CRUDE_PHYSICS_JOLT_MAX_BODIES, CRUDE_PHYSICS_JOLT_NUM_BODIES_MUTEXES, CRUDE_PHYSICS_JOLT_MAX_BODIES_PAIRS, CRUDE_PHYSICS_JOLT_MAX_CONTACT_CONSTRAINTS,
    *physics->jph_broad_phase_layer_interface_class, *physics->jph_object_vs_broadphase_layer_filter_class, *physics->jph_object_vs_object_layer_filter_class );

  physics->jph_physics_system_class->SetBodyActivationListener( physics->jph_body_activation_listener_class );
  physics->jph_physics_system_class->SetContactListener( physics->jph_contact_listener_class );

  jph_body_interface_class = &physics->jph_physics_system_class->GetBodyInterface( );
  
  jph_floor_shape_settings_class = CRUDE_COMPOUNT( JPH::BoxShapeSettings, { JPH::Vec3( 100.0f, 1.0f, 100.0f ) } );
  jph_floor_shape_settings_class.SetEmbedded( );
  
  jph_floor_shape_result_class = jph_floor_shape_settings_class.Create( );
  jph_floor_shape_class = jph_floor_shape_result_class.Get( );
  
  jph_floor_settings_class = CRUDE_COMPOUNT( JPH::BodyCreationSettings,{
    jph_floor_shape_class, JPH::RVec3( 0.0, -1.0, 0.0 ), JPH::Quat::sIdentity( ), JPH::EMotionType::Static, g_crude_jph_layer_non_moving } );
  
  physics->jph_floor_class = jph_body_interface_class->CreateBody( jph_floor_settings_class );
  
  jph_body_interface_class->AddBody( physics->jph_floor_class->GetID( ), JPH::EActivation::DontActivate );
  
  jph_sphere_settings_class = CRUDE_COMPOUNT( JPH::BodyCreationSettings, { new JPH::SphereShape(0.5f), JPH::RVec3(0.0, 2.0, 0.0), JPH::Quat::sIdentity(), JPH::EMotionType::Dynamic, g_crude_jph_layer_moving } );
  physics->jph_sphere_id_class = jph_body_interface_class->CreateAndAddBody( jph_sphere_settings_class, JPH::EActivation::Activate );
  
  jph_body_interface_class->SetLinearVelocity( physics->jph_sphere_id_class, JPH::Vec3( 0.0f, -5.0f, 0.0f ) );
  
  physics->jph_physics_system_class->OptimizeBroadPhase( );

  physics->collision_manager = creation->collision_manager;
  physics->simulation_enabled = false;
  
  query_desc = CRUDE_COMPOUNT_EMPTY( ecs_query_desc_t );
  query_desc.terms[ 0 ].id = ecs_id( crude_physics_static_body );
  query_desc.terms[ 1 ].id = ecs_id( crude_physics_collision_shape );
  query_desc.terms[ 2 ].id = ecs_id( crude_transform );
  physics->static_body_handle_query = crude_ecs_query_create( world, &query_desc );
  
  query_desc = CRUDE_COMPOUNT_EMPTY( ecs_query_desc_t );
  query_desc.terms[ 0 ].id = ecs_id( crude_physics_character_body );
  query_desc.terms[ 1 ].id = ecs_id( crude_physics_collision_shape );
  query_desc.terms[ 2 ].id = ecs_id( crude_transform );
  physics->character_body_handle_query = crude_ecs_query_create( world, &query_desc );
}

void
crude_physics_deinitialize
(
  _In_ crude_physics                                      *physics
)
{
  JPH::BodyInterface                                      *jph_body_interface_class;

  jph_body_interface_class = &physics->jph_physics_system_class->GetBodyInterface( );
  
  jph_body_interface_class->RemoveBody( physics->jph_sphere_id_class );
  jph_body_interface_class->DestroyBody( physics->jph_sphere_id_class );
  
  jph_body_interface_class->RemoveBody( physics->jph_floor_class->GetID( ) );
  jph_body_interface_class->DestroyBody( physics->jph_floor_class->GetID( ) );
  
  JPH::UnregisterTypes();
  
  delete JPH::Factory::sInstance;
  JPH::Factory::sInstance = nullptr;
  
  CRUDE_DEALLOCATE_AND_DECONSTRUCT( physics->allocator_container, physics->jph_temporary_allocator_class, TempAllocatorImpl );
  CRUDE_DEALLOCATE_AND_DECONSTRUCT( physics->allocator_container, physics->jph_job_system_class, JobSystemThreadPool );
  CRUDE_DEALLOCATE_AND_DECONSTRUCT( physics->allocator_container, physics->jph_physics_system_class, PhysicsSystem );

  CRUDE_DEALLOCATE_AND_DECONSTRUCT( physics->allocator_container, physics->jph_broad_phase_layer_interface_class, _crude_jph_bp_layer_interface_class );
  CRUDE_DEALLOCATE_AND_DECONSTRUCT( physics->allocator_container, physics->jph_object_vs_broadphase_layer_filter_class, _crude_jph_object_vs_broad_phase_layer_filter );
  CRUDE_DEALLOCATE_AND_DECONSTRUCT( physics->allocator_container, physics->jph_object_vs_object_layer_filter_class, _crude_jph_object_layer_pair_filter_class );
  
  CRUDE_DEALLOCATE_AND_DECONSTRUCT( physics->allocator_container, physics->jph_body_activation_listener_class, _crude_jph_body_activation_listener_class );
  CRUDE_DEALLOCATE_AND_DECONSTRUCT( physics->allocator_container, physics->jph_contact_listener_class, _crude_jph_contact_listener_class );
}

void
crude_physics_update
(
  _In_ crude_physics                                      *physics
)
{
  physics->jph_physics_system_class->Update( CRUDE_PHYSICS_JOLT_DELTA_TIME, CRUDE_PHYSICS_JOLT_COLLISIONS_STEPS, physics->jph_temporary_allocator_class, physics->jph_job_system_class );
}

void
crude_physics_enable_simulation
(
  _In_ crude_physics                                      *physics,
  _In_ bool                                                enable
)
{
  physics->simulation_enabled = enable;
}

void
crude_physics_enable_reset_velocity
(
  _In_ crude_physics                                      *physics,
  _In_ crude_ecs                                          *world
)
{
  ecs_iter_t                                               character_body_handle_it;

  character_body_handle_it = ecs_query_iter( world, physics->character_body_handle_query );
  while ( ecs_query_next( &character_body_handle_it ) )
  {
    crude_physics_character_body                          *character_body_per_entity;
    crude_physics_collision_shape                         *collision_shape_per_entity;
    crude_transform                                       *transform_per_entity;

    character_body_per_entity = ecs_field( &character_body_handle_it, crude_physics_character_body, 0 );
    collision_shape_per_entity = ecs_field( &character_body_handle_it, crude_physics_collision_shape, 1 );
    transform_per_entity = ecs_field( &character_body_handle_it, crude_transform, 2 );

    for ( uint32 i = 0; i < character_body_handle_it.count; ++i )
    {
      XMStoreFloat3( &character_body_per_entity[ i ].velocity, XMVectorZero( ) );
      character_body_per_entity[ i ].on_floor = false;
    }
  }
}

bool
crude_physics_cast_ray
(
  _In_ crude_physics                                      *physics,
  _In_ crude_ecs                                          *world,
  _In_ XMVECTOR                                            ray_origin,
  _In_ XMVECTOR                                            ray_direction,
  _In_ uint32                                              mask,
  _Out_opt_ crude_physics_raycast_result                  *result
)
{
  ecs_iter_t                                               static_body_it;
  float32                                                  nearest_t;

  nearest_t = FLT_MAX;

  static_body_it = ecs_query_iter( world, physics->static_body_handle_query );
  while ( ecs_query_next( &static_body_it ) )
  {
    crude_physics_collision_shape                         *second_collision_shape_per_entity;
    crude_transform                                       *second_transform_per_entity;
    crude_physics_static_body                             *second_body_handle_per_entity;

    second_body_handle_per_entity = ecs_field( &static_body_it, crude_physics_static_body, 0 );
    second_collision_shape_per_entity = ecs_field( &static_body_it, crude_physics_collision_shape, 1 );
    second_transform_per_entity = ecs_field( &static_body_it, crude_transform, 2 );

    for ( uint32 static_body_index = 0; static_body_index < static_body_it.count; ++static_body_index )
    {
      crude_physics_collision_shape                       *second_collision_shape;
      crude_transform                                     *second_transform;
      crude_physics_static_body                           *second_body;
      crude_entity                                         static_body_node;
      XMMATRIX                                             second_transform_mesh_to_world;
      XMVECTOR                                             second_translation;
      crude_raycast_result                                 current_result;
      bool                                                 intersected;
      
      second_collision_shape = &second_collision_shape_per_entity[ static_body_index ];
      second_transform = &second_transform_per_entity[ static_body_index ];
      second_body = &second_body_handle_per_entity[ static_body_index ];

      static_body_node = crude_entity_from_iterator( &static_body_it, static_body_index );
  
      if ( !second_body->enabeld )
      {
        continue;
      }

      if ( !( mask & second_body->layer ) )
      {
        continue;
      }
  
      second_collision_shape = CRUDE_ENTITY_GET_MUTABLE_COMPONENT( world, static_body_node, crude_physics_collision_shape );
      second_transform = CRUDE_ENTITY_GET_MUTABLE_COMPONENT( world, static_body_node, crude_transform );
      second_transform_mesh_to_world = crude_transform_node_to_world( world, static_body_node, second_transform );
      second_translation = second_transform_mesh_to_world.r[ 3 ];
  
      if ( second_collision_shape->type == CRUDE_PHYSICS_COLLISION_SHAPE_TYPE_BOX )
      {
        XMVECTOR second_box_half_extent = XMLoadFloat3( &second_collision_shape->box.half_extent );
        intersected = crude_raycast_obb( ray_origin, ray_direction, second_translation, second_box_half_extent, second_transform_mesh_to_world, &current_result );
      }
      else if ( second_collision_shape->type == CRUDE_PHYSICS_COLLISION_SHAPE_TYPE_MESH )
      {
        crude_octree *octree = crude_collisions_resources_manager_access_octree( physics->collision_manager, second_collision_shape->mesh.octree_handle );
        intersected = crude_octree_cast_ray( octree, ray_origin, ray_direction, &current_result );
      }
      else
      {
        CRUDE_ASSERT( false );
      }
        
      if ( intersected && current_result.t < nearest_t )
      {
        nearest_t = current_result.t;
        if ( result )
        {
          result->raycast_result = current_result;
          result->node = static_body_node;
          result->body_layer = second_body->layer;
        }
      }
    }
  }

  return nearest_t != FLT_MAX;
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