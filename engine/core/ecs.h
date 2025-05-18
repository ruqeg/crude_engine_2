#pragma once

#include <core/alias.h>

/************************************************
 *
 * ECS Typedef 
 * 
 ***********************************************/
typedef uint64 crude_ecs_id;
typedef uint64 crude_entity_handle;

/************************************************
 *
 * ECS Entity Declaration 
 * 
 ***********************************************/
typedef struct crude_entity
{
  crude_entity_handle                                      handle;
  void                                                    *world;
} crude_entity;

/************************************************
 *
 * ECS Helper Macro 
 * Some of them was copied from flecs.h in hardcode way......
 * Hope it will not be broken in the future... :D
 * 
 ***********************************************/
/* ecs_id */
#define CRUDE_ECS_ID( T ) FLECS_ID##T##ID_
/* ECS_DECLARE */
#define CRUDE_ECS_DECLARE( T ) crude_entity_handle T, CRUDE_ECS_ID( T )
#define CRUDE_ECS_TAG_DECLARE CRUDE_ECS_DECLARE
/* ECS_COMPONENT_DECLARE */
#define CRUDE_ECS_COMPONENT_DECLARE( T ) crude_entity_handle CRUDE_ECS_ID( T )

#define CRUDE_ECS_MODULE_IMPORT_DECL( fnname ) CRUDE_API void fnname##Import( void* world )
#define CRUDE_ECS_MODULE_IMPORT_IMPL( fnname ) void fnname##Import( void* world )

/************************************************
 *
 * ECS Entity Tag Declaration 
 * 
 ***********************************************/
CRUDE_API CRUDE_ECS_TAG_DECLARE( crude_entity_tag );

/************************************************
 *
 * ECS World Functions Declaration 
 * 
 ***********************************************/
CRUDE_API void*
crude_ecs_init
(
);

CRUDE_API int
crude_ecs_fini
(
  _In_ void                                               *world
);

CRUDE_API bool
crude_ecs_should_quit
(
  _In_ void                                               *world
);

CRUDE_API bool
crude_ecs_progress
(
  _In_ void                                               *world,
  _In_ float32                                             delta_time
);

/************************************************
 *
 * ECS Entity Functions Declaration 
 * 
 ***********************************************/
CRUDE_API crude_entity
crude_entity_create_empty
(
  _In_ void                                               *world,
  _In_ char const                                         *name
);

CRUDE_API void
crude_entity_destroy
(
  _In_ crude_entity                                        entity
);

CRUDE_API bool
crude_entity_valid
(
  _In_ crude_entity                                        entity
);

CRUDE_API void
crude_entity_set
(
  _In_ crude_entity                                        entity,
  _In_ crude_ecs_id                                        id,
  _In_ size_t                                              size,
  _In_ void const                                         *ptr
);

CRUDE_API void
crude_entity_add
(
  _In_ crude_entity                                        entity,
  _In_ crude_ecs_id                                        id
);

CRUDE_API void
crude_entity_doc_set_name
(
  _In_ crude_entity                                        entity,
  _In_ char const                                         *name
);

CRUDE_API void*
crude_entity_ensure
(
  _In_ crude_entity                                        entity,
  _In_ crude_ecs_id                                        id
);

CRUDE_API char const*
crude_entity_doc_get_name
(
  _In_ crude_entity                                        entity
);

CRUDE_API void const*
crude_entity_get
(
  _In_ crude_entity                                        entity,
  _In_ crude_ecs_id                                        id
);

CRUDE_API void*
crude_entity_get_mut
(
  _In_ crude_entity                                        entity,
  _In_ crude_ecs_id                                        id
);

CRUDE_API bool
crude_entity_has
(
  _In_ crude_entity                                        entity,
  _In_ crude_ecs_id                                        id
);

CRUDE_API void
crude_entity_remove
(
  _In_ crude_entity                                        entity,
  _In_ crude_ecs_id                                        id
);

CRUDE_API void
crude_entity_set_parent
(
  _In_ crude_entity                                        entity,
  _In_ crude_entity                                        parent
);

CRUDE_API crude_entity
crude_entity_get_parent
(
  _In_ crude_entity                                        entity
);

/************************************************
 *
 * ECS Entity Macros
 * 
 ***********************************************/
#define CRUDE_ENTITY_CREATE( world, name, component, ... )\
({\
  crude_entity entity = crude_entity_create_empty( world, name == NULL || name[0] == '\0' ? "entity" : name );\
  crude_entity_set( entity, CRUDE_ECS_ID( component ), sizeof( component ), &(component)__VA_ARGS__ );\
  entity;\
})

#define CRUDE_ENTITY_SET_COMPONENT( entity, component, ... )\
{\
  crude_entity_set( entity, CRUDE_ECS_ID( component ), sizeof( component ), &( component )__VA_ARGS__ );\
}

#define CRUDE_ENTITY_GET_OR_ADD_COMPONENT( entity, component )\
(\
  ( component* )( crude_entity_ensure( entity, CRUDE_ECS_ID( component ) ) )\
)

#define CRUDE_ENTITY_GET_IMMUTABLE_COMPONENT( entity, component )\
(\
  ( component const* )( crude_entity_get( entity, CRUDE_ECS_ID( component ) ) )\
)

#define CRUDE_ENTITY_GET_MUTABLE_COMPONENT( entity, component )\
(\
  ( component* )( crude_entity_get_mut( entity, CRUDE_ECS_ID( component ) ) )\
)

#define CRUDE_ENTITY_HAS_COMPONENT( entity, component )\
(\
  crude_entity_has( entity, CRUDE_ECS_ID( component ) )\
)

#define CRUDE_ENTITY_REMOVE_COMPONENT( entity, component )\
(\
  crude_entity_remove( entity, CRUDE_ECS_ID( component ) )\
)