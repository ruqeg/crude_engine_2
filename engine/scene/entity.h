#pragma once

#include <core/alias.h>
#include <flecs.h>

extern ECS_DECLARE( crude_entity_tag );

typedef struct crude_entity
{
  ecs_entity_t   handle;
  void          *world;
} crude_entity;

crude_entity crude_entity_create_empty( void *world, const char *name );
void crude_entity_destroy( crude_entity entity );
bool crude_entity_valid( crude_entity entity );

#define CRUDE_ENTITY_CREATE( ecs_world, name, component, ...)\
  ({\
    crude_entity entity = (crude_entity){ ecs_new( ecs_world, 0 ), ecs_world };\
    ecs_doc_set_name( entity.world, entity.handle, name == NULL || name[0] == '\0' ? "entity" : name );\
    ecs_set( entity.world, entity.handle, component, __VA_ARGS__ );\
    ecs_add( entity.world, entity.handle, crude_entity_tag );\
  })
#define CRUDE_ENTITY_ADD_COMPONENT( entity, component, ... )  ecs_set( entity.world, entity.handle, component, __VA_ARGS__ )
#define CRUDE_ENTITY_GET_OR_ADD_COMPONENT( entity, T )        ecs_get_mut( entity.world, entity.handle, T )
#define CRUDE_ENTITY_GET_NAME(entity)                         ecs_doc_get_name( entity.world, entity.handle )
#define CRUDE_ENTITY_GET_READ_ONLY_COMPONENT(entity, T)       ecs_get( entity.world, entity.handle, T )
#define CRUDE_ENTITY_HAS_COMPONENT(entity, T)                 ecs_has( entity.world, entity.handle, T )
#define CRUDE_ENTITY_REMOVE_COMPONENT(entity, T)              ecs_remove( entity.world, entity.handle, T )
#define CRUDE_ENTITY_REMOVE_COMPONENT_ID(entity, id)          ecs_remove_id( entity.world, entity.handle, id )