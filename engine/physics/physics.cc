#include <flecs.h>
#include <stdarg.h>

#include <core/log.h>
#include <core/array.h>
#include <core/hash_map.h>
#include <core/assert.h>
#include <scene/scene_components.h>
#include <physics/physics_components.h>

#include <physics/physics.h>

crude_physics                                             *physics_instance_;

void
crude_physics_initialize
(
  _In_ crude_physics                                      *physics,
  _In_ crude_physics_creation const                       *creation
)
{
  physics->allocator = creation->allocator;

  physics->simulation_enabled = false;

  CRUDE_ARRAY_INITIALIZE_WITH_CAPACITY( physics->character_bodies, 0, crude_heap_allocator_pack( physics->allocator ) );
  CRUDE_ARRAY_INITIALIZE_WITH_CAPACITY( physics->static_bodies, 0, crude_heap_allocator_pack( physics->allocator ) );

  crude_resource_pool_initialize( &physics->character_bodies_resource_pool, crude_heap_allocator_pack( physics->allocator ), CRUDE_PHYSICS_MAX_BODIES_COUNT, sizeof( crude_physics_character_body ) );
  crude_resource_pool_initialize( &physics->static_bodies_resource_pool, crude_heap_allocator_pack( physics->allocator ), CRUDE_PHYSICS_MAX_BODIES_COUNT, sizeof( crude_physics_static_body ) );
}

void
crude_physics_deinitialize
(
  _In_ crude_physics                                      *physics
)
{
  CRUDE_ARRAY_DEINITIALIZE( physics->character_bodies );
  CRUDE_ARRAY_DEINITIALIZE( physics->static_bodies );

  crude_resource_pool_deinitialize( &physics->character_bodies_resource_pool );
  crude_resource_pool_deinitialize( &physics->static_bodies_resource_pool );
}

crude_physics_character_body_handle
crude_physics_create_character_body
(
  _In_ crude_physics                                      *physics,
  _In_ crude_entity                                        node
)
{
  crude_physics_character_body_handle dynamic_body_handle = CRUDE_COMPOUNT( crude_physics_character_body_handle, { crude_resource_pool_obtain_resource( &physics->character_bodies_resource_pool ) } );
  crude_physics_character_body *dynamic_body = crude_physics_access_character_body( physics, dynamic_body_handle );
  dynamic_body->node = node;

  CRUDE_ARRAY_PUSH( physics->character_bodies, dynamic_body_handle ); 
  return dynamic_body_handle;
}

CRUDE_API crude_physics_static_body_handle
crude_physics_create_static_body
(
  _In_ crude_physics                                      *physics,
  _In_ crude_entity                                        node
)
{
  crude_physics_static_body_handle static_body_handle = CRUDE_COMPOUNT( crude_physics_static_body_handle, { crude_resource_pool_obtain_resource( &physics->static_bodies_resource_pool ) } );
  crude_physics_static_body *static_body = crude_physics_access_static_body( physics, static_body_handle );
  static_body->node = node;
  
  CRUDE_ARRAY_PUSH( physics->static_bodies, static_body_handle ); 
  return static_body_handle;
}

void
crude_physics_destroy_character_body
(
  _In_ crude_physics                                      *physics,
  _In_ crude_physics_character_body_handle                 handle
)
{
  // !TODO :D
  for ( uint32 i = 0; i < CRUDE_ARRAY_LENGTH( physics->character_bodies ); ++i )
  {
    if ( physics->character_bodies[ i ].index == handle.index )
    {
      CRUDE_ARRAY_DELSWAP( physics->character_bodies, i );
      break;
    }
  }
  crude_resource_pool_release_resource( &physics->character_bodies_resource_pool, handle.index );
}

void
crude_physics_destroy_static_body
(
  _In_ crude_physics                                      *physics,
  _In_ crude_physics_static_body_handle                    handle
)
{
  // !TODO :D
  for ( uint32 i = 0; i < CRUDE_ARRAY_LENGTH( physics->static_bodies ); ++i )
  {
    if ( physics->static_bodies[ i ].index == handle.index )
    {
      CRUDE_ARRAY_DELSWAP( physics->static_bodies, i );
      break;
    }
  }
  crude_resource_pool_release_resource( &physics->static_bodies_resource_pool, handle.index );
}

crude_physics_character_body*
crude_physics_access_character_body
(
  _In_ crude_physics                                      *physics,
  _In_ crude_physics_character_body_handle                 handle
)
{
  return CRUDE_CAST( crude_physics_character_body*, crude_resource_pool_access_resource( &physics->character_bodies_resource_pool, handle.index ) );
}

crude_physics_static_body*
crude_physics_access_static_body
(
  _In_ crude_physics                                      *physics,
  _In_ crude_physics_static_body_handle                    handle
)
{
  return CRUDE_CAST( crude_physics_static_body*, crude_resource_pool_access_resource( &physics->static_bodies_resource_pool, handle.index ) );
}

XMVECTOR
crude_physics_character_body_get_velocity
(
  _In_ crude_physics                                      *physics,
  _In_ crude_physics_character_body_handle                 handle
)
{
  crude_physics_character_body *body = CRUDE_CAST( crude_physics_character_body*, crude_resource_pool_access_resource( &physics->character_bodies_resource_pool, handle.index ) );
  return XMLoadFloat3( &body->velocity );
}

void
crude_physics_character_body_set_velocity
(
  _In_ crude_physics                                      *physics,
  _In_ crude_physics_character_body_handle                 handle,
  _In_ XMVECTOR                                            velocity
)
{
  crude_physics_character_body *body = CRUDE_CAST( crude_physics_character_body*, crude_resource_pool_access_resource( &physics->character_bodies_resource_pool, handle.index ) );
  XMStoreFloat3( &body->velocity, velocity );
}

bool
crude_physics_character_body_on_floor
(
  _In_ crude_physics                                      *physics,
  _In_ crude_physics_character_body_handle                 handle
)
{
  crude_physics_character_body *body = CRUDE_CAST( crude_physics_character_body*, crude_resource_pool_access_resource( &physics->character_bodies_resource_pool, handle.index ) );
  return body->on_floor;
}

void
crude_physics_enable_simulation
(
  _In_ crude_physics                                      *physics,
  _In_ bool                                                enable
)
{
  physics->simulation_enabled = enable;

  if ( enable )
  {
    for ( uint32 i = 0; i < CRUDE_ARRAY_LENGTH( physics->character_bodies ); ++i )
    {
      crude_physics_character_body *dynamic_body = crude_physics_access_character_body( physics, physics->character_bodies[ i ] );
      XMStoreFloat3( &dynamic_body->velocity, XMVectorZero( ) );
      dynamic_body->on_floor = false;
    }
  }
}

void
crude_physics_instance_allocate
(
  _In_ crude_allocator_container                           allocator_container
)
{
  physics_instance_ = CRUDE_CAST( crude_physics*, CRUDE_ALLOCATE( allocator_container, sizeof( crude_physics ) ) );
  *physics_instance_ = CRUDE_COMPOUNT_EMPTY( crude_physics );
}

void
crude_physics_instance_deallocate
(
  _In_ crude_allocator_container                           allocator_container
)
{
  CRUDE_DEALLOCATE( allocator_container, physics_instance_ );
}

crude_physics*
crude_physics_instance
(
)
{
  return physics_instance_;
}