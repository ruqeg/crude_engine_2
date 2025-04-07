#include <scene/entity.h>

ECS_DECLARE(Entity);

crude_entity
crude_entity_create_empty
(
  _In_ void            *world,
  _In_ const char      *name
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
  _In_ crude_entity     entity
)
{
  ecs_delete( entity.world, entity.handle );
}

bool
crude_entity_valid
(
  _In_ crude_entity     entity
)
{
  return entity.world != NULL && ecs_is_valid( entity.world, entity.handle );
}