#pragma once

#include <flecs.h>
#include <core/alias.h>

typedef struct crude_entity
{
  ecs_entity_t                                             handle;
  ecs_world_t                                             *world;
} crude_entity;

#define CRUDE_ECS_MODULE_IMPORT_DECL( fnname ) CRUDE_API void fnname##Import( ecs_world_t* world )
#define CRUDE_ECS_MODULE_IMPORT_IMPL( fnname ) void fnname##Import( ecs_world_t* world )

/************************************************
 *
 * ECS Entity Tag Declaration 
 * 
 ***********************************************/
CRUDE_API ECS_TAG_DECLARE( crude_entity_tag );

/************************************************
 *
 * ECS World Functions Declaration 
 * 
 ***********************************************/
CRUDE_API ecs_world_t*
crude_ecs_init
(
);

CRUDE_API int
crude_ecs_fini
(
  _In_ ecs_world_t                                        *world
);

CRUDE_API bool
crude_ecs_should_quit
(
  _In_ ecs_world_t                                        *world
);

CRUDE_API bool
crude_ecs_progress
(
  _In_ ecs_world_t                                        *world,
  _In_ float32                                             delta_time
);

CRUDE_API crude_entity
crude_ecs_lookup_entity
(
  _In_ ecs_world_t                                        *world,
  _In_ char const                                         *path
);

CRUDE_API crude_entity
crude_ecs_lookup_entity_from_parent
(
  _In_ crude_entity                                        parent,
  _In_ char const                                         *path
);

/************************************************
 *
 * ECS Entity Functions Declaration 
 * 
 ***********************************************/
CRUDE_API crude_entity
crude_entity_create_empty
(
  _In_ ecs_world_t                                        *world,
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

CRUDE_API char const*
crude_entity_get_name
(
  _In_ crude_entity                                        entity
);

CRUDE_API void
crude_entity_destroy_hierarchy
(
  _In_ crude_entity                                        entity
);

/************************************************
 *
 * ECS Macros
 * 
 ***********************************************/
#define CRUDE_ENTITY_CREATE( world, name, component, ... )\
({\
  crude_entity entity = crude_entity_create_empty( world, name == NULL || name[0] == '\0' ? "entity" : name );\
  CRUDE_ENTITY_SET_COMPONENT( entity, component, ##__VA_ARGS__ );\
  entity;\
})

#define CRUDE_ENTITY_SET_COMPONENT( entity, component, ... )\
{\
  component tmp = CRUDE_COMPOUNT( component, ##__VA_ARGS__ );\
  ecs_set_id( entity.world, entity.handle, ecs_id( component ), sizeof( component ), &tmp );\
}

#define CRUDE_ENTITY_GET_OR_ADD_COMPONENT( entity, component )\
(\
  ( component* )( ecs_ensure( entity.world, entity.handle, component ) )\
)

#define CRUDE_ENTITY_GET_IMMUTABLE_COMPONENT( entity, component )\
(\
  ( component const* )( ecs_get( entity.world, entity.handle, component ) )\
)

#define CRUDE_ENTITY_GET_MUTABLE_COMPONENT( entity, component )\
(\
  ( component* )( ecs_get_mut( ( entity ).world, ( entity ).handle, component ) )\
)

#define CRUDE_ENTITY_HAS_COMPONENT( entity, component )\
(\
  ecs_has( entity.world, entity.handle, component )\
)

#define CRUDE_ENTITY_REMOVE_COMPONENT( entity, component )\
(\
  ecs_remove( entity.world, entity.handle, component )\
)

#define CRUDE_ENTITY_COMPONENT_MODIFIED( entity, component )\
{\
  ecs_modified_id( entity.world, entity.handle, ecs_id( component ) );\
}

#define CRUDE_ECS_SYSTEM_DECLARE( id )\
  ecs_entity_t ecs_id( id )

#define CRUDE_ECS_SYSTEM_DEFINE( world, id_, phase, ctx_, ... )\
{\
  ecs_entity_desc_t edesc = { 0 };\
  ecs_id_t add_ids[3] = {\
    ( ( phase ) ? ecs_pair( EcsDependsOn, ( phase ) ) : 0 ),\
    ( phase ),\
    0\
  };\
  edesc.id = ecs_id( id_ );\
  edesc.name = #id_;\
  edesc.add = add_ids;\
  ecs_system_desc_t desc = {\
    .query = {\
      .terms = ##__VA_ARGS__\
    }\
  };\
  desc.entity = ecs_entity_init( world, &edesc );\
  desc.callback = id_;\
  desc.ctx = ctx_;\
  ecs_id(id_) = ecs_system_init( world, &desc );\
  ecs_assert(ecs_id(id_) != 0, ECS_INVALID_PARAMETER, "failed to create system %s", #id_);\
}

#define CRUDE_ECS_OBSERVER_DECLARE( id )\
  ecs_entity_t ecs_id(id)

#define CRUDE_ECS_OBSERVER_DEFINE( world, id_, kind, ... )\
{\
  ecs_entity_desc_t edesc = { 0 };\
  edesc.id = ecs_id( id_ ); \
  edesc.name = #id_; \
  ecs_observer_desc_t desc = {\
    .query = {\
      .terms = ##__VA_ARGS__\
    }\
  };\
  desc.entity = ecs_entity_init( world, &edesc ); \
  desc.callback = id_;\
  desc.events[0] = kind;\
  ecs_id( id_ ) = ecs_observer_init( world, &desc );\
  ecs_assert( ecs_id( id_ ) != 0, ECS_INVALID_PARAMETER, "failed to create observer %s", #id_ );\
}