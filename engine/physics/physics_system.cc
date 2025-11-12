
#include <flecs.h>
#include <stdarg.h>

#include <core/log.h>
#include <core/assert.h>
#include <scene/scene_components.h>
#include <physics/physics_components.h>

#include <physics/physics_system.h>

#define CRUDE_PHYSICS_BODY_LAYERS_COLLIDING_DYNAMIC 1
#define CRUDE_PHYSICS_BODY_LAYERS_COLLIDING_STATIC 2

constexpr static uint32 jph_broad_phase_layers_count_ = 3;

CRUDE_ECS_SYSTEM_DECLARE( crude_process_physics_system_ );
CRUDE_ECS_OBSERVER_DECLARE( crude_physics_dynamic_body_creation_observer_ );
CRUDE_ECS_OBSERVER_DECLARE( crude_physics_static_body_creation_observer_ );
CRUDE_ECS_OBSERVER_DECLARE( crude_physics_body_destrotion_observer_ );
CRUDE_ECS_OBSERVER_DECLARE( crude_physics_body_transform_set_observer_ );
CRUDE_ECS_OBSERVER_DECLARE( crude_physics_body_collision_shape_set_observer_ );
CRUDE_ECS_SYSTEM_DECLARE( crude_physics_body_update_transform_system_ );

static void
crude_physics_static_body_creation_observer_
(
  ecs_iter_t *it
)
{
  crude_physics_static_body                               *static_bodies_per_entity;
  crude_collision_shape                                   *collision_shapes_per_entity;

  static_bodies_per_entity = ecs_field( it, crude_physics_static_body, 0 );
  collision_shapes_per_entity = ecs_field( it, crude_collision_shape, 1 );

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

  dynamic_bodies_per_entity = ecs_field( it, crude_physics_dynamic_body, 0 );
  collision_shapes_per_entity = ecs_field( it, crude_collision_shape, 1 );

  for ( uint32 i = 0; i < it->count; ++i )
  {
    ecs_world_t                                           *world;
    crude_physics_dynamic_body                            *dynamic_body;
    crude_transform                                       *transform;
    crude_collision_shape                                 *collision_shape;
    crude_physics_body_handle                             *body_handle;
    crude_entity                                           entity;

    world = it->world;
    dynamic_body = &dynamic_bodies_per_entity[ i ];
    entity = CRUDE_COMPOUNT( crude_entity, { it->entities[ i ], world } );
    collision_shape = &collision_shapes_per_entity[ i ];

    body_handle = ecs_ensure( world, it->entities[ i ], crude_physics_body_handle );

    CRUDE_ASSERT( CRUDE_ENTITY_HAS_COMPONENT( entity, crude_transform ) );
    transform = CRUDE_ENTITY_GET_MUTABLE_COMPONENT( entity, crude_transform );
    CRUDE_ASSERT( transform );
    
  }
}

static void
crude_physics_body_destrotion_observer_
(
  ecs_iter_t *it
)
{
  crude_physics_body_handle                               *bodies_per_entity;

  bodies_per_entity = ecs_field( it, crude_physics_body_handle, 0 );

  for ( uint32 i = 0; i < it->count; ++i )
  {
    crude_physics_body_handle                             *body;

    body = &bodies_per_entity[ i ];

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

  bodies_per_entity = ecs_field( it, crude_physics_body_handle, 0 );
  transform_per_entity = ecs_field( it, crude_transform, 1 );

  for ( uint32 i = 0; i < it->count; ++i )
  {
    crude_physics_body_handle                             *body;
    crude_transform                                       *transform;

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

  bodies_per_entity = ecs_field( it, crude_physics_body_handle, 0 );
  collision_shapes_per_entity = ecs_field( it, crude_collision_shape, 1 );

  for ( uint32 i = 0; i < it->count; ++i )
  {
    crude_physics_body_handle                             *physics_body_handle;
    crude_collision_shape                                 *collision_shape;

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
}

void
crude_physics_deinitialize
(
)
{
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
  return XMVectorSet( 0, 0, 0, 1 );
}

void
crude_physics_body_set_translation
(
  _In_ crude_physics_body_handle const                    *dynamic_body,
  _In_ XMVECTOR                                            position
)
{
}

void
crude_physics_body_set_rotation
(
  _In_ crude_physics_body_handle const                    *dynamic_body,
  _In_ XMVECTOR                                            rotation
)
{
}

void
crude_physics_body_set_linear_velocity
(
  _In_ crude_physics_body_handle const                    *dynamic_body,
  _In_ XMVECTOR                                            velocity
)
{
}

void
crude_physics_body_add_linear_velocity
(
  _In_ crude_physics_body_handle const                    *dynamic_body,
  _In_ XMVECTOR                                            velocity
)
{
}

XMVECTOR
crude_physics_body_get_linear_velocity
(
  _In_ crude_physics_body_handle const                    *dynamic_body
)
{
  return XMVectorSet( 0, 0, 0, 1 );
}

XMVECTOR
crude_physics_body_get_rotation
(
  _In_ crude_physics_body_handle const                    *body
)
{
  return XMVectorSet( 0,0,0,0 );
}

void
crude_physics_body_set_collision
(
  _In_ crude_physics_body_handle const                    *body,
  _In_ crude_collision_shape const                        *shape
)
{
}

void
crude_physics_body_set_body_layer
(
  _In_ crude_physics_body_handle const                    *body,
  _In_ uint32                                              layers
)
{
}