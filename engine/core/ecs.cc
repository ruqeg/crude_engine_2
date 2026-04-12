#include <engine/core/profiler.h>

#include <engine/core/ecs.h>

/************************************************
 *
 * ECS World Functions Implementatino
 * 
 ***********************************************/
CRUDE_API crude_ecs*
crude_ecs_create
(
)
{
  return ecs_init( );
}

int
crude_ecs_destroy
(
  _In_ crude_ecs                                          *world
)
{
  int result = ecs_fini( world );
  return result;
}

bool
crude_ecs_should_quit
(
  _In_ crude_ecs                                          *world
)
{
  return ecs_should_quit( world );
}

bool
crude_ecs_progress
(
  _In_ crude_ecs                                          *world,
  _In_ float32                                             delta_time
)
{
  CRUDE_PROFILER_ZONE_NAME( "crude_ecs_progress" );
  bool result = ecs_progress( world, delta_time );
  CRUDE_PROFILER_ZONE_END;
  return result;
}

crude_entity
crude_ecs_lookup_entity
(
  _In_ crude_ecs                                          *world,
  _In_ char const                                         *path
)
{
  return ecs_lookup( world, path );
}

crude_entity
crude_ecs_lookup_entity_from_parent
(
  _In_ crude_ecs                                          *world,
  _In_ crude_entity                                        parent,
  _In_ char const                                         *path
)
{
  return ecs_lookup_from( world, parent, path );
}

void
crude_ecs_set_threads
(
  _In_ crude_ecs                                          *world,
  _In_ uint64                                              threads_count
)
{
  ecs_set_threads( world, threads_count );
}

ecs_query_t*
crude_ecs_query_create
(
  _In_ crude_ecs                                          *world,
  _In_ ecs_query_desc_t const                             *const_desc
)
{
  return ecs_query_init( world, const_desc );
}

ecs_iter_t
crude_ecs_children
(
  _In_ crude_ecs                                          *world,
  _In_ crude_entity                                        parent
)
{
  return ecs_children( world, parent );
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
  _In_ uint64                                              index
)
{
  return it->entities[ index ];
}

crude_entity
crude_entity_create_empty_without_name
(
  _In_ crude_ecs                                          *world
)
{
  crude_entity entity = ecs_new( world );
  return entity;
}

crude_entity
crude_entity_create_empty
(
  _In_ crude_ecs                                          *world,
  _In_ const char                                         *name
)
{
  crude_entity entity = ecs_new( world );
  entity = ecs_set_name( world, entity, name == NULL || name[0] == '\0' ? "entity" : name );
  return entity;
}

void
crude_entity_destroy
(
  _In_ crude_ecs                                          *world,
  _In_ crude_entity                                        entity
)
{
  ecs_delete( world, entity );
}

bool
crude_entity_valid
(
  _In_ crude_ecs                                          *world,
  _In_ crude_entity                                        entity
)
{
  return ( world != NULL && ecs_is_valid( world, entity ) );
}

void
crude_entity_set_parent
(
  _In_ crude_ecs                                          *world,
  _In_ crude_entity                                        entity,
  _In_ crude_entity                                        parent
)
{
  ecs_add_pair( world, entity, EcsChildOf, parent );
}

crude_entity
crude_entity_get_parent
(
  _In_ crude_ecs                                          *world,
  _In_ crude_entity                                        entity
)
{
  return ecs_get_parent( world, entity );
}

char const*
crude_entity_get_name
(
  _In_ crude_ecs                                          *world,
  _In_ crude_entity                                        entity
)
{
  return ecs_get_name( world, entity );
}

void
crude_entity_allocate_path
(
  _In_ crude_ecs                                          *world,
  _In_ crude_entity                                        entity,
  _Out_ char                                             **path
)
{
  *path = ecs_get_path( world, entity );
}

void
crude_entity_free_path
(
  _In_ char                                               *path
)
{
  ecs_os_free( path );
}

void
crude_entity_set_name
(
  _In_ crude_ecs                                          *world,
  _In_ crude_entity                                        entity,
  _In_ char const                                         *name
)
{
  ecs_set_name( world, entity, name );
}

void
crude_entity_destroy_hierarchy
(
  _In_ crude_ecs                                          *world,
  _In_ crude_entity                                        entity
)
{
  if ( !crude_entity_valid( world, entity ) )
  {
    return;
  }

  ecs_iter_t it = crude_ecs_children( world, entity );

  while ( ecs_children_next( &it ) )
  {
    for ( size_t i = 0; i < it.count; ++i )
    {
      crude_entity child = crude_entity_from_iterator( &it, i );
      crude_entity_destroy_hierarchy( world, child );
    }
  }
  crude_entity_destroy( world, entity );
}

crude_entity
crude_entity_copy
(
  _In_ crude_ecs                                          *world,
  _In_ crude_entity                                        src,
  _In_ bool                                                copy_value
)
{
  return ecs_clone( world, 0, src, copy_value );
}

crude_entity
crude_entity_copy_to_entity
(
  _In_ crude_ecs                                          *world,
  _In_ crude_entity                                        dst,
  _In_ crude_entity                                        src,
  _In_ bool                                                copy_value
)
{
  return ecs_clone( world, dst, src, copy_value );
}

void
crude_entity_enable
(
  _In_ crude_ecs                                          *world,
  _In_ crude_entity                                        entity,
  _In_ bool                                                enabled
)
{
  ecs_enable( world, entity, enabled );
}

bool
crude_entity_is_enable
(
  _In_ crude_ecs                                          *world,
  _In_ crude_entity                                        entity
)
{
  
  return !ecs_has_id( world, entity, EcsDisabled );
}

void
crude_entity_add_component
(
  _In_ crude_ecs                                          *world,
  _In_ crude_entity                                        entity,
  _In_ ecs_id_t                                            id
)
{
  ecs_add_id( world, entity, id );
}

void
crude_entity_set_component
(
  _In_ crude_ecs                                          *world,
  _In_ crude_entity                                        entity,
  _In_ ecs_id_t                                            id,
  _In_ uint64                                              size,
  _In_ void const                                         *ptr
)
{
  ecs_set_id( world, entity, id, size, ptr );
}

void*
crude_entity_find_component_from_parents
(
  _In_ crude_ecs                                          *world,
  _Out_ crude_entity                                      *entity,
  _In_ ecs_id_t                                            id
)
{
  while ( *entity = crude_entity_get_parent( world, *entity ) )
  {
    if ( crude_entity_valid( world, *entity ) )
    {
      void                                                *component;

      component = crude_entity_get_mutable_component( world, *entity, id );
  
      if ( component )
      {
        return component;
      }
    }
  }

  return NULL;
}

void*
crude_entity_get_or_add_component
(
  _In_ crude_ecs                                          *world,
  _In_ crude_entity                                        entity,
  _In_ ecs_id_t                                            id,
  _In_ uint64                                              size
)
{
  return ecs_ensure_id( world, entity, id, size );
}

void const*
crude_entity_get_immutable_component
(
  _In_ crude_ecs                                          *world,
  _In_ crude_entity                                        entity,
  _In_ ecs_id_t                                            id
)
{
  return ecs_get_id( world, entity, id );
}

void*
crude_entity_get_mutable_component
(
  _In_ crude_ecs                                          *world,
  _In_ crude_entity                                        entity,
  _In_ ecs_id_t                                            id
)
{
  return ecs_get_mut_id( world, entity, id );
}

bool
crude_entity_has_component
(
  _In_ crude_ecs                                          *world,
  _In_ crude_entity                                        entity,
  _In_ ecs_id_t                                            id
)
{
  return ecs_has_id( world, entity, id );
}

void
crude_entity_remove_component
(
  _In_ crude_ecs                                          *world,
  _In_ crude_entity                                        entity,
  _In_ ecs_id_t                                            id
)
{
  ecs_remove_id( world, entity, id );
}

void
crude_entity_modified_component
(
  _In_ crude_ecs                                          *world,
  _In_ crude_entity                                        entity,
  _In_ ecs_id_t                                            id
)
{
  ecs_modified_id( world, entity, id );
}

bool
crude_entity_has_id
(
  _In_ crude_ecs                                          *world,
  _In_ crude_entity                                        entity,
  _In_ ecs_id_t                                            id
)
{
  return ecs_has_id( world, entity, id );
}

void
crude_entity_add_id
(
  _In_ crude_ecs                                          *world,
  _In_ crude_entity                                        entity,
  _In_ ecs_id_t                                            id
)
{
  ecs_add_id( world, entity, id );
}

void
crude_entity_remove_id
(
  _In_ crude_ecs                                          *world,
  _In_ crude_entity                                        entity,
  _In_ ecs_id_t                                            id
)
{
  ecs_remove_id( world, entity, id );
}