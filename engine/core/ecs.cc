#include <core/ecs.h>

ECS_DECLARE( crude_entity_tag );

/************************************************
 *
 * ECS World Functions Implementatino
 * 
 ***********************************************/
ecs_world_t*
crude_ecs_init
(
)
{
  return ecs_init( );
}

int
crude_ecs_fini
(
  _In_ ecs_world_t                                        *world
)
{
  return ecs_fini( world );
}

bool
crude_ecs_should_quit
(
  _In_ ecs_world_t                                        *world
)
{
  return ecs_should_quit( world );
}

bool
crude_ecs_progress
(
  _In_ ecs_world_t                                        *world,
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
  _In_ ecs_world_t                                        *world,
  _In_ const char                                         *name
)
{
  crude_entity entity = CRUDE_COMPOUNT( crude_entity, { .handle = ecs_new( world ), .world = world } );
  ecs_doc_set_name( world, entity.handle, name == NULL || name[0] == '\0' ? "entity" : name );
  ecs_add( world, entity.handle, crude_entity_tag );
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
  ecs_entity_t parent = ecs_get_parent( entity.world, entity.handle );
  return CRUDE_COMPOUNT( crude_entity, { .handle = parent, .world = entity.world } );
}

char const*
crude_entity_get_name
(
  _In_ crude_entity                                        entity
)
{
  return ecs_doc_get_name( entity.world, entity.handle );
}