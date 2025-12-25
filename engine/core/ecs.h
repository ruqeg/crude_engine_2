#pragma once

#include <flecs.h>
#include <threads.h>

#include <engine/core/alias.h>

typedef struct crude_ecs
{
  mtx_t                                                    mutex;
  ecs_world_t                                             *handle;
} crude_ecs;

typedef struct crude_entity
{
  ecs_entity_t                                             handle;
  crude_ecs                                               *world;
} crude_entity;

#define CRUDE_ECS_MODULE_IMPORT_DECL( fnname ) CRUDE_API void fnname##Import( crude_ecs* world )
#define CRUDE_ECS_MODULE_IMPORT_IMPL( fnname ) void fnname##Import( crude_ecs* world )

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
CRUDE_API void
crude_ecs_initialize
(
  _In_ crude_ecs                                          *world
);

CRUDE_API int
crude_ecs_deinitalize
(
  _In_ crude_ecs                                          *world
);

CRUDE_API bool
crude_ecs_should_quit
(
  _In_ crude_ecs                                          *world
);

CRUDE_API bool
crude_ecs_progress
(
  _In_ crude_ecs                                          *world,
  _In_ float32                                             delta_time
);

CRUDE_API crude_entity
crude_ecs_lookup_entity
(
  _In_ crude_ecs                                          *world,
  _In_ char const                                         *path
);

CRUDE_API crude_entity
crude_ecs_lookup_entity_from_parent
(
  _In_ crude_entity                                        parent,
  _In_ char const                                         *path
);

CRUDE_API void
crude_ecs_set_threads
(
  _In_ crude_ecs                                          *world,
  _In_ uint64                                              threads_count
);

CRUDE_API ecs_query_t*
crude_ecs_query_create
(
  _In_ crude_ecs                                          *world,
  _In_ ecs_query_desc_t const                             *const_desc
);

CRUDE_API ecs_iter_t
crude_ecs_children
(
  _In_ crude_entity                                        parent
);

/************************************************
 *
 * ECS Entity Functions Declaration 
 * 
 ***********************************************/
CRUDE_API crude_entity
crude_entity_from_iterator
(
  _In_ ecs_iter_t                                         *it,
  _In_ crude_ecs                                          *world,
  _In_ uint64                                              index
);

CRUDE_API crude_entity
crude_entity_create_empty_without_name
(
  _In_ crude_ecs                                          *world
);

CRUDE_API crude_entity
crude_entity_create_empty
(
  _In_ crude_ecs                                          *world,
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
crude_entity_set_name
(
  _In_ crude_entity                                        entity,
  _In_ char const                                         *name
);

CRUDE_API void
crude_entity_destroy_hierarchy
(
  _In_ crude_entity                                        entity
);

CRUDE_API crude_entity
crude_entity_copy
(
  _In_ crude_entity                                        src,
  _In_ bool                                                copy_value
);

CRUDE_API crude_entity
crude_entity_copy_hierarchy
(
  _In_ crude_entity                                        src,
  _In_ char const                                         *name,
  _In_ bool                                                copy_value,
  _In_ bool                                                enabled
);

CRUDE_API void
crude_entity_enable
(
  _In_ crude_entity                                        entity,
  _In_ bool                                                enabled
);

CRUDE_API void
crude_entity_enable_hierarchy
(
  _In_ crude_entity                                        entity,
  _In_ bool                                                enabled
);

CRUDE_API void
crude_entity_add_component
(
  _In_ crude_entity                                        entity,
  _In_ ecs_id_t                                            id
);

CRUDE_API void
crude_entity_set_component
(
  _In_ crude_entity                                        entity,
  _In_ ecs_id_t                                            id,
  _In_ uint64                                              size,
  _In_ void const                                         *ptr
);

CRUDE_API void*
crude_entity_get_or_add_component
(
  _In_ crude_entity                                        entity,
  _In_ ecs_id_t                                            id
);

CRUDE_API void const*
crude_entity_get_immutable_component
(
  _In_ crude_entity                                        entity,
  _In_ ecs_id_t                                            id
);

CRUDE_API void*
crude_entity_get_mutable_component
(
  _In_ crude_entity                                        entity,
  _In_ ecs_id_t                                            id
);

CRUDE_API bool
crude_entity_has_component
(
  _In_ crude_entity                                        entity,
  _In_ ecs_id_t                                            id
);

CRUDE_API void
crude_entity_remove_component
(
  _In_ crude_entity                                        entity,
  _In_ ecs_id_t                                            id
);

CRUDE_API void
crude_entity_modified_component
(
  _In_ crude_entity                                        entity,
  _In_ ecs_id_t                                            id
);

CRUDE_API bool
crude_entity_has_id
(
  _In_ crude_entity                                        entity,
  _In_ ecs_id_t                                            id
);

CRUDE_API void
crude_entity_add_id
(
  _In_ crude_entity                                        entity,
  _In_ ecs_id_t                                            id
);

CRUDE_API void
crude_entity_remove_id
(
  _In_ crude_entity                                        entity,
  _In_ ecs_id_t                                            id
);

/************************************************
 *
 * ECS Macros
 * 
 ***********************************************/
#define CRUDE_ENTITY_ADD_COMPONENT( entity, component )\
{\
  crude_entity_add_component( entity, ecs_id( component ) );\
}

#define CRUDE_ENTITY_SET_COMPONENT( entity, component, ... )\
{\
  component tmp = CRUDE_COMPOUNT( component, ##__VA_ARGS__ );\
  crude_entity_set_component( entity, ecs_id( component ), sizeof( component ), &tmp );\
}

#define CRUDE_ENTITY_GET_OR_ADD_COMPONENT( entity, component )\
(\
  ( component* )( crude_entity_get_or_add_component( entity, ecs_id( component ) ) )\
)

#define CRUDE_ENTITY_GET_IMMUTABLE_COMPONENT( entity, component )\
(\
  ( component const* )( crude_entity_get_immutable_component( entity, ecs_id( component ) ) )\
)

#define CRUDE_ENTITY_GET_MUTABLE_COMPONENT( entity, component )\
(\
  ( component* )( crude_entity_get_mutable_component( entity, ecs_id( component ) ) )\
)

#define CRUDE_ENTITY_HAS_COMPONENT( entity, component )\
(\
  crude_entity_has_component( entity, ecs_id( component ) )\
)

#define CRUDE_ENTITY_REMOVE_COMPONENT( entity, component )\
(\
  crude_entity_remove_component( entity, ecs_id( component ) )\
)

#define CRUDE_ENTITY_COMPONENT_MODIFIED( entity, component )\
{\
  crude_entity_modified_component( entity, ecs_id( component ) );\
}

#define CRUDE_ENTITY_HAS_TAG( entity, tag )\
(\
  crude_entity_has_id( entity, tag )\
)

#define CRUDE_ENTITY_ADD_TAG( entity, tag )\
(\
  crude_entity_add_id( entity, tag )\
)

#define CRUDE_ENTITY_REMOVE_TAG( entity, tag )\
(\
  crude_entity_remove_id( entity, tag )\
)

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
  mtx_lock( &(world)->mutex );\
  desc.entity = ecs_entity_init( (world)->handle, &edesc );\
  desc.callback = id_;\
  desc.ctx = ctx_;\
  ecs_id(id_) = ecs_system_init( (world)->handle, &desc );\
  ecs_assert(ecs_id(id_) != 0, ECS_INVALID_PARAMETER, "failed to create system %s", #id_);\
  mtx_unlock( &(world)->mutex );\
}

#define CRUDE_ECS_OBSERVER_DECLARE( id )\
  ecs_entity_t ecs_id(id)

#define CRUDE_ECS_OBSERVER_DEFINE( world, id_, kind, ctx_, ... )\
{\
  ecs_entity_desc_t edesc = { 0 };\
  edesc.id = ecs_id( id_ ); \
  edesc.name = #id_; \
  ecs_observer_desc_t desc = {\
    .query = {\
      .terms = ##__VA_ARGS__\
    }\
  };\
  mtx_lock( &(world)->mutex );\
  desc.entity = ecs_entity_init( (world)->handle, &edesc ); \
  desc.callback = id_;\
  desc.events[0] = kind;\
  desc.ctx = ctx_;\
  ecs_id( id_ ) = ecs_observer_init( (world)->handle, &desc );\
  ecs_assert( ecs_id( id_ ) != 0, ECS_INVALID_PARAMETER, "failed to create observer %s", #id_ );\
  mtx_unlock( &(world)->mutex );\
}

#define CRUDE_ECS_IMPORT(world, id)\
{\
  mtx_lock( &(world)->mutex );\
  ECS_IMPORT((world)->handle, id);\
  mtx_unlock( &(world)->mutex );\
}

#define CRUDE_ECS_MODULE(world, id)\
{\
  mtx_lock( &(world)->mutex );\
  ECS_MODULE((world)->handle, id);\
  mtx_unlock( &(world)->mutex );\
}

#define CRUDE_ECS_TAG_DEFINE(world, id)\
{\
  mtx_lock( &(world)->mutex );\
  ECS_TAG_DEFINE((world)->handle, id);\
  mtx_unlock( &(world)->mutex );\
}

#define CRUDE_ECS_COMPONENT_DEFINE(world, id)\
{\
  mtx_lock( &(world)->mutex );\
  ECS_COMPONENT_DEFINE((world)->handle, id);\
  mtx_unlock( &(world)->mutex );\
}