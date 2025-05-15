#include <flecs.h>

#include <core/ecs.h>

/************************************************
 *
 * Some asserts since I copied some types from flecs.h
 * 
 ***********************************************/
CRUDE_STATIC_ASSERT( sizeof( ecs_entity_t ) == sizeof( crude_entity_handle ), "crude_entity_handle != ecs_entity_t" );
CRUDE_STATIC_ASSERT( sizeof( ecs_id_t ) == sizeof( crude_ecs_id ), "crude_entity_handle != ecs_entity_t" );

ECS_DECLARE(Entity);

/************************************************
 *
 * ECS World Functions Implementatino
 * 
 ***********************************************/
void*
crude_ecs_init
(
)
{
  return ecs_init( );
}

int
crude_ecs_fini
(
  _In_ void                                               *world
)
{
  ecs_fini( world );
}

bool
crude_ecs_should_quit
(
  _In_ void                                               *world
)
{
  return ecs_should_quit( world );
}

bool
crude_ecs_progress
(
  _In_ void                                               *world,
  _In_ float32                                             delta_time
)
{
  return ecs_progress( world, delta_time );
}

/************************************************
 *
 * ECS Entity Functions Implementatino
 * 
 ***********************************************/
crude_entity
crude_entity_create_empty
(
  _In_ void                                               *world,
  _In_ const char                                         *name
)
{
  crude_entity entity = ( crude_entity ){ ecs_new( world, 0 ), world };
  ecs_doc_set_name( world, entity.handle, name == NULL || name[0] == '\0' ? "entity" : name );
  ecs_add( entity.world, entity.handle, Entity );
  return entity;
}

void
crude_entity_destroy
(
  _In_ crude_entity                                        entity
)
{
  ecs_delete( entity.world, entity.handle );
}

bool
crude_entity_valid
(
  _In_ crude_entity                                        entity
)
{
  return entity.world != NULL && ecs_is_valid( entity.world, entity.handle );
}

void
crude_entity_set
(
  _In_ crude_entity                                        entity,
  _In_ crude_ecs_id                                        id,
  _In_ size_t                                              size,
  _In_ void const                                         *ptr
)
{
  ecs_set_id( entity.world, entity.handle, id, size, ptr );
}

void
crude_entity_add
(
  _In_ crude_entity                                        entity,
  _In_ crude_ecs_id                                        id
)
{
  ecs_add_id( entity.world, entity.handle, id );
}

void
crude_entity_doc_set_name
(
  _In_ crude_entity                                        entity,
  _In_ char const                                         *name
)
{
  ecs_doc_set_name( entity.world, entity.handle, name );
}

void*
crude_entity_ensure
(
  _In_ crude_entity                                        entity,
  _In_ crude_ecs_id                                        id
)
{
  return ecs_ensure_id( entity.world, entity.handle, id );
}

char const*
crude_entity_doc_get_name
(
  _In_ crude_entity                                        entity
)
{
  return ecs_doc_get_name( entity.world, entity.handle );
}

void const*
crude_entity_get
(
  _In_ crude_entity                                        entity,
  _In_ crude_ecs_id                                        id
)
{
  return ecs_get_id( entity.world, entity.handle, id );
}

void*
crude_entity_get_mut
(
  _In_ crude_entity                                        entity,
  _In_ crude_ecs_id                                        id
)
{
  return ecs_get_mut_id( entity.world, entity.handle, id );
}

bool
crude_entity_has
(
  _In_ crude_entity                                        entity,
  _In_ crude_ecs_id                                        id
)
{
  return ecs_has_id( entity.world, entity.handle, id );
}

void
crude_entity_remove
(
  _In_ crude_entity                                        entity,
  _In_ crude_ecs_id                                        id
)
{
  return ecs_remove_id( entity.world, entity.handle, id );
}

void
crude_entity_set_parent
(
  _In_ crude_entity                                        entity,
  _In_ crude_entity                                        parent
)
{
  ecs_add_pair( entity.world, entity.handle, EcsChildOf, parent.handle );
}

crude_entity
crude_entity_get_parent
(
  _In_ crude_entity                                        entity
)
{
  return ( crude_entity ){ .world = entity.world, .handle = ecs_get_parent( entity.world, entity.handle ) };
}