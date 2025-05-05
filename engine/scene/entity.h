#pragma once

#include <flecs.h>

#include <core/alias.h>

CRUDE_API extern ECS_DECLARE( Entity );

typedef struct crude_entity
{
  ecs_entity_t          handle;
  void                 *world;
} crude_entity;

CRUDE_API crude_entity
crude_entity_create_empty
(
  _In_ void            *world,
  _In_ const char      *name
);

CRUDE_API void
crude_entity_destroy
(
  _In_ crude_entity     entity
);

CRUDE_API bool
crude_entity_valid
(
  _In_ crude_entity     entity
);

#define CRUDE_ENTITY_CREATE( ecs_world, name, component, ... )\
({\
  crude_entity entity = (crude_entity){ ecs_new( ecs_world, 0 ), ecs_world };\
  ecs_doc_set_name( entity.world, entity.handle, name == NULL || name[0] == '\0' ? "entity" : name);\
  ecs_set( entity.world, entity.handle, component, __VA_ARGS__ );\
  ecs_add( entity.world, entity.handle, Entity );\
  entity;\
})

#define CRUDE_ENTITY_SET_COMPONENT( entity, component, ... )\
{\
  ecs_set( entity.world, entity.handle, component, __VA_ARGS__ );\
}

#define CRUDE_ENTITY_GET_OR_ADD_COMPONENT( entity, component )\
(\
  ecs_ensure( entity.world, entity.handle, component )\
)

#define CRUDE_ENTITY_GET_NAME( entity )\
(\
  ecs_doc_get_name( entity.world, entity.handle )\
)

#define CRUDE_ENTITY_GET_IMMUTABLE_COMPONENT( entity, component )\
(\
  ecs_get( entity.world, entity.handle, component )\
)

#define CRUDE_ENTITY_GET_MUTABLE_COMPONENT( entity, component )\
(\
  ecs_get_mut( entity.world, entity.handle, component )\
)

#define CRUDE_ENTITY_HAS_COMPONENT( entity, component )\
(\
  ecs_has( entity.world, entity.handle, component )\
)

#define CRUDE_ENTITY_REMOVE_COMPONENT( entity, component )\
(\
  ecs_remove( entity.world, entity.handle, component )\
)

#define CRUDE_ENTITY_REMOVE_COMPONEN_ID( entity, id )\
(\
  ecs_remove_id( entity.world, entity.handle, id )\
)

#define CRUDE_ENTITY_GET_PARENT( entity )\
(\
  (crude_entity){ ecs_get_parent( entity.world, entity.handle ), entity.world }\
)

#define CRUDE_ENTITY_VALID( entity )\
(\
  crude_entity_valid( entity )\
)