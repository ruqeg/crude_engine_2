#include <engine/core/ecs.h>

ECS_DECLARE( crude_entity_tag );

/************************************************
 *
 * ECS World Functions Implementatino
 * 
 ***********************************************/
CRUDE_API void
crude_ecs_initialize
(
  _In_ crude_ecs                                          *world
)
{
  mtx_init( &world->mutex, mtx_plain );
  world->handle = ecs_init( );
}

int
crude_ecs_deinitalize
(
  _In_ crude_ecs                                          *world
)
{
  mtx_lock( &world->mutex );
  mtx_destroy( &world->mutex );
  int result = ecs_fini( world->handle );
  mtx_unlock( &world->mutex );
  return result;
}

bool
crude_ecs_should_quit
(
  _In_ crude_ecs                                          *world
)
{
  return ecs_should_quit( world->handle );
}

bool
crude_ecs_progress
(
  _In_ crude_ecs                                          *world,
  _In_ float32                                             delta_time
)
{
  mtx_lock( &world->mutex );
  bool result = ecs_progress( world->handle, delta_time );
  mtx_unlock( &world->mutex );
  return result;
}

crude_entity
crude_ecs_lookup_entity
(
  _In_ crude_ecs                                          *world,
  _In_ char const                                         *path
)
{
  crude_entity entity;
  entity.world = world;
  entity.handle = ecs_lookup( world->handle, path );
  return entity;
}

crude_entity
crude_ecs_lookup_entity_from_parent
(
  _In_ crude_entity                                        parent,
  _In_ char const                                         *path
)
{
  crude_entity entity;
  entity.world = parent.world;
  entity.handle = ecs_lookup_from( parent.world->handle, parent.handle, path );
  return entity;
}

void
crude_ecs_set_threads
(
  _In_ crude_ecs                                          *world,
  _In_ uint64                                              threads_count
)
{
  mtx_lock( &world->mutex );
  ecs_set_threads( world->handle, threads_count );
  mtx_unlock( &world->mutex );
}

ecs_query_t*
crude_ecs_query_create
(
  _In_ crude_ecs                                          *world,
  _In_ ecs_query_desc_t const                             *const_desc
)
{
  mtx_lock( &world->mutex );
  ecs_query_t *query = ecs_query_init( world->handle, const_desc );
  mtx_unlock( &world->mutex );
  return query;
}

ecs_iter_t
crude_ecs_children
(
  _In_ crude_entity                                        parent
)
{
  return ecs_children( parent.world->handle, parent.handle );
}

/************************************************
 *
 * ECS Entity Functions Implementatino
 * 
 ***********************************************/
crude_entity
crude_entity_from_iterator
(
  _In_ ecs_iter_t                                         *it,
  _In_ crude_ecs                                          *world,
  _In_ uint64                                              index
)
{
  return CRUDE_COMPOUNT( crude_entity, { it->entities[ index ], world } );
}

crude_entity
crude_entity_create_empty_without_name
(
  _In_ crude_ecs                                          *world
)
{
  mtx_lock( &world->mutex );
  crude_entity entity = CRUDE_COMPOUNT( crude_entity, { .handle = ecs_new( world->handle ), .world = world } );
  ecs_add( world->handle, entity.handle, crude_entity_tag );
  mtx_unlock( &world->mutex );
  return entity;
}

crude_entity
crude_entity_create_empty
(
  _In_ crude_ecs                                          *world,
  _In_ const char                                         *name
)
{
  mtx_lock( &world->mutex );
  crude_entity entity = CRUDE_COMPOUNT( crude_entity, { .handle = ecs_new( world->handle ), .world = world } );
  entity.handle = ecs_set_name( world->handle, entity.handle, name == NULL || name[0] == '\0' ? "entity" : name );
  ecs_add( world->handle, entity.handle, crude_entity_tag );
  mtx_unlock( &world->mutex );
  return entity;
}

void
crude_entity_destroy
(
  _In_ crude_entity                                        entity
)
{
  mtx_lock( &entity.world->mutex );
  ecs_delete( entity.world->handle, entity.handle );
  mtx_unlock( &entity.world->mutex );
}

bool
crude_entity_valid
(
  _In_ crude_entity                                        entity
)
{
  return entity.world != NULL && ecs_is_valid( entity.world->handle, entity.handle );
}

void
crude_entity_set_parent
(
  _In_ crude_entity                                        entity,
  _In_ crude_entity                                        parent
)
{
  mtx_lock( &entity.world->mutex );
  ecs_add_pair( entity.world->handle, entity.handle, EcsChildOf, parent.handle );
  mtx_unlock( &entity.world->mutex );
}

crude_entity
crude_entity_get_parent
(
  _In_ crude_entity                                        entity
)
{
  ecs_entity_t parent = ecs_get_parent( entity.world->handle, entity.handle );
  return CRUDE_COMPOUNT( crude_entity, { .handle = parent, .world = entity.world } );
}

char const*
crude_entity_get_name
(
  _In_ crude_entity                                        entity
)
{
  return ecs_get_name( entity.world->handle, entity.handle );
}

void
crude_entity_set_name
(
  _In_ crude_entity                                        entity,
  _In_ char const                                         *name
)
{
  mtx_lock( &entity.world->mutex );
  ecs_set_name( entity.world->handle, entity.handle, name );
  mtx_unlock( &entity.world->mutex );
}

void
crude_entity_destroy_hierarchy
(
  _In_ crude_entity                                        entity
)
{
  if ( !crude_entity_valid( entity ) )
  {
    return;
  }

  ecs_iter_t it = crude_ecs_children( entity );

  while ( ecs_children_next( &it ) )
  {
    for ( size_t i = 0; i < it.count; ++i )
    {
      crude_entity child = CRUDE_COMPOUNT( crude_entity, { .handle = it.entities[ i ], .world = entity.world } );
      crude_entity_destroy_hierarchy( child );
    }
  }
  crude_entity_destroy( entity );
}

crude_entity
crude_entity_copy
(
  _In_ crude_entity                                        src,
  _In_ bool                                                copy_value
)
{
  crude_entity dst = CRUDE_COMPOUNT_EMPTY( crude_entity );
  dst.world = src.world;
  dst.handle = ecs_clone( src.world->handle, dst.handle, src.handle, copy_value );
  return dst;
}

crude_entity
crude_entity_copy_hierarchy
(
  _In_ crude_entity                                        src,
  _In_ char const                                         *name,
  _In_ bool                                                copy_value,
  _In_ bool                                                enabled
)
{
  if ( !crude_entity_valid( src ) )
  {
    return CRUDE_COMPOUNT_EMPTY( crude_entity );
  }
  
  crude_entity new_node = crude_entity_copy( src, copy_value );

  ecs_iter_t it = crude_ecs_children( src );

  while ( ecs_children_next( &it ) )
  {
    for ( size_t i = 0; i < it.count; ++i )
    {
      crude_entity child = CRUDE_COMPOUNT( crude_entity, { .handle = it.entities[ i ], .world = src.world } );
      crude_entity new_child = crude_entity_copy_hierarchy( child, crude_entity_get_name( child ), copy_value, enabled );

      crude_entity_set_parent( new_child, new_node );
    }
  }
  
  crude_entity_enable( new_node, enabled );
  crude_entity_set_name( new_node, name );

  return new_node;
}

void
crude_entity_enable
(
  _In_ crude_entity                                        entity,
  _In_ bool                                                enabled
)
{
  ecs_enable( entity.world->handle, entity.handle, enabled );
}

void
crude_entity_enable_hierarchy
(
  _In_ crude_entity                                        entity,
  _In_ bool                                                enabled
)
{
  if ( !crude_entity_valid( entity ) )
  {
    return;
  }
  
  ecs_iter_t it = crude_ecs_children( entity );

  while ( ecs_children_next( &it ) )
  {
    for ( size_t i = 0; i < it.count; ++i )
    {
      crude_entity child = CRUDE_COMPOUNT( crude_entity, { .handle = it.entities[ i ], .world = entity.world } );
      crude_entity_enable_hierarchy( child, enabled );
    }
  }
  
  crude_entity_enable( entity, enabled );
}

void
crude_entity_add_component
(
  _In_ crude_entity                                        entity,
  _In_ ecs_id_t                                            id
)
{
  mtx_lock( &entity.world->mutex );
  ecs_add_id( entity.world->handle, entity.handle, id );
  mtx_unlock( &entity.world->mutex );
}

void
crude_entity_set_component
(
  _In_ crude_entity                                        entity,
  _In_ ecs_id_t                                            id,
  _In_ uint64                                              size,
  _In_ void const                                         *ptr
)
{
  mtx_lock( &entity.world->mutex );
  ecs_set_id( entity.world->handle, entity.handle, id, size, ptr );
  mtx_unlock( &entity.world->mutex );
}

void*
crude_entity_get_or_add_component
(
  _In_ crude_entity                                        entity,
  _In_ ecs_id_t                                            id
)
{
  mtx_lock( &entity.world->mutex );
  void *data = ecs_ensure_id( entity.world->handle, entity.handle, id );
  mtx_unlock( &entity.world->mutex );
  return data;
}

void const*
crude_entity_get_immutable_component
(
  _In_ crude_entity                                        entity,
  _In_ ecs_id_t                                            id
)
{
  return ecs_get_id( entity.world->handle, entity.handle, id );
}

void*
crude_entity_get_mutable_component
(
  _In_ crude_entity                                        entity,
  _In_ ecs_id_t                                            id
)
{
  return ecs_get_mut_id( entity.world->handle, entity.handle, id );
}

bool
crude_entity_has_component
(
  _In_ crude_entity                                        entity,
  _In_ ecs_id_t                                            id
)
{
  return ecs_has_id( entity.world->handle, entity.handle, id );
}

void
crude_entity_remove_component
(
  _In_ crude_entity                                        entity,
  _In_ ecs_id_t                                            id
)
{
  mtx_lock( &entity.world->mutex );
  ecs_remove_id( entity.world->handle, entity.handle, id );
  mtx_unlock( &entity.world->mutex );
}

void
crude_entity_modified_component
(
  _In_ crude_entity                                        entity,
  _In_ ecs_id_t                                            id
)
{
  mtx_lock( &entity.world->mutex );
  ecs_modified_id( entity.world->handle, entity.handle, id );
  mtx_unlock( &entity.world->mutex );
}

bool
crude_entity_has_id
(
  _In_ crude_entity                                        entity,
  _In_ ecs_id_t                                            id
)
{
  return ecs_has_id( entity.world->handle, entity.handle, id );
}

void
crude_entity_add_id
(
  _In_ crude_entity                                        entity,
  _In_ ecs_id_t                                            id
)
{
  mtx_lock( &entity.world->mutex );
  ecs_add_id( entity.world->handle, entity.handle, id );
  mtx_unlock( &entity.world->mutex );
}

void
crude_entity_remove_id
(
  _In_ crude_entity                                        entity,
  _In_ ecs_id_t                                            id
)
{
  mtx_lock( &entity.world->mutex );
  ecs_remove_id( entity.world->handle, entity.handle, id );
  mtx_unlock( &entity.world->mutex );
}