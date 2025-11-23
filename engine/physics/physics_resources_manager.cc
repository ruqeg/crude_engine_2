#include <engine/core/array.h>

#include <engine/physics/physics_resources_manager.h>

void
crude_physics_resources_manager_initialize
(
  _In_ crude_physics_resources_manager                    *manager,
  _In_ crude_physics_resources_manager_creation const     *creation
)
{
  manager->allocator = creation->allocator;

  CRUDE_ARRAY_INITIALIZE_WITH_CAPACITY( manager->character_bodies, 0, crude_heap_allocator_pack( manager->allocator ) );
  CRUDE_ARRAY_INITIALIZE_WITH_CAPACITY( manager->static_bodies, 0, crude_heap_allocator_pack( manager->allocator ) );

  crude_resource_pool_initialize( &manager->character_bodies_resource_pool, crude_heap_allocator_pack( manager->allocator ), CRUDE_PHYSICS_MAX_BODIES_COUNT, sizeof( crude_physics_character_body ) );
  crude_resource_pool_initialize( &manager->static_bodies_resource_pool, crude_heap_allocator_pack( manager->allocator ), CRUDE_PHYSICS_MAX_BODIES_COUNT, sizeof( crude_physics_static_body ) );
}

void
crude_physics_resources_manager_deinitialize
(
  _In_ crude_physics_resources_manager                    *manager
)
{
  for ( int32 i = CRUDE_ARRAY_LENGTH( manager->character_bodies ) - 1; i >= 0; --i )
  {
    crude_physics_resources_manager_destroy_character_body( manager, manager->character_bodies[ i ] );
  }

  for ( int32 i = CRUDE_ARRAY_LENGTH( manager->static_bodies ) - 1; i >= 0; --i )
  {
    crude_physics_resources_manager_destroy_static_body( manager, manager->static_bodies[ i ] );
  }

  CRUDE_ARRAY_DEINITIALIZE( manager->character_bodies );
  CRUDE_ARRAY_DEINITIALIZE( manager->static_bodies );

  crude_resource_pool_deinitialize( &manager->character_bodies_resource_pool );
  crude_resource_pool_deinitialize( &manager->static_bodies_resource_pool );
}

crude_physics_character_body_handle
crude_physics_resources_manager_create_character_body
(
  _In_ crude_physics_resources_manager                    *manager,
  _In_ crude_entity                                        node
)
{
  crude_physics_character_body_handle character_body_handle = CRUDE_COMPOUNT( crude_physics_character_body_handle, { crude_resource_pool_obtain_resource( &manager->character_bodies_resource_pool ) } );
  crude_physics_character_body *character_body = crude_physics_resources_manager_access_character_body( manager, character_body_handle );
  character_body->callback_container = crude_physics_collision_callback_container_empty( );
  character_body->node = node;

  CRUDE_ARRAY_PUSH( manager->character_bodies, character_body_handle ); 
  return character_body_handle;
}

CRUDE_API crude_physics_static_body_handle
crude_physics_resources_manager_create_static_body
(
  _In_ crude_physics_resources_manager                    *manager,
  _In_ crude_entity                                        node
)
{
  crude_physics_static_body_handle static_body_handle = CRUDE_COMPOUNT( crude_physics_static_body_handle, { crude_resource_pool_obtain_resource( &manager->static_bodies_resource_pool ) } );
  crude_physics_static_body *static_body = crude_physics_resources_manager_access_static_body( manager, static_body_handle );
  static_body->node = node;
  
  CRUDE_ARRAY_PUSH( manager->static_bodies, static_body_handle ); 
  return static_body_handle;
}

void
crude_physics_resources_manager_destroy_character_body
(
  _In_ crude_physics_resources_manager                    *manager,
  _In_ crude_physics_character_body_handle                 handle
)
{
  // !TODO :D
  for ( uint32 i = 0; i < CRUDE_ARRAY_LENGTH( manager->character_bodies ); ++i )
  {
    if ( manager->character_bodies[ i ].index == handle.index )
    {
      CRUDE_ARRAY_DELSWAP( manager->character_bodies, i );
      break;
    }
  }
  crude_resource_pool_release_resource( &manager->character_bodies_resource_pool, handle.index );
}

void
crude_physics_resources_manager_destroy_static_body
(
  _In_ crude_physics_resources_manager                    *manager,
  _In_ crude_physics_static_body_handle                    handle
)
{
  // !TODO :D
  for ( uint32 i = 0; i < CRUDE_ARRAY_LENGTH( manager->static_bodies ); ++i )
  {
    if ( manager->static_bodies[ i ].index == handle.index )
    {
      CRUDE_ARRAY_DELSWAP( manager->static_bodies, i );
      break;
    }
  }
  crude_resource_pool_release_resource( &manager->static_bodies_resource_pool, handle.index );
}

crude_physics_character_body*
crude_physics_resources_manager_access_character_body
(
  _In_ crude_physics_resources_manager                    *manager,
  _In_ crude_physics_character_body_handle                 handle
)
{
  return CRUDE_CAST( crude_physics_character_body*, crude_resource_pool_access_resource( &manager->character_bodies_resource_pool, handle.index ) );
}

crude_physics_static_body*
crude_physics_resources_manager_access_static_body
(
  _In_ crude_physics_resources_manager                    *manager,
  _In_ crude_physics_static_body_handle                    handle
)
{
  return CRUDE_CAST( crude_physics_static_body*, crude_resource_pool_access_resource( &manager->static_bodies_resource_pool, handle.index ) );
}