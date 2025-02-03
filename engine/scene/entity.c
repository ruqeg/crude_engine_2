#include <scene/entity.h>
#include <core/utils.h>

ECS_DECLARE( crude_entity_tag );

crude_entity crude_entity_create_empty( void *world, const char *name )
{
  crude_entity entity = ( crude_entity ){ ecs_new( world, 0 ), world };
  ecs_doc_set_name( world, entity.handle, name == NULL || name[0] == '\0' ? "entity" : name );
  ecs_add( entity.world, entity.handle, crude_entity_tag );
  return entity;
}

void crude_entity_destroy( crude_entity entity )
{
  ecs_delete( entity.world, entity.handle );
}

bool crude_entity_valid( crude_entity entity )
{
  return entity.world != NULL && ecs_is_valid( entity.world, entity.handle );
}