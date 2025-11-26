#include <engine/core/ecs.h>

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

crude_entity
crude_ecs_lookup_entity
(
  _In_ ecs_world_t                                        *world,
  _In_ char const                                         *path
)
{
  crude_entity entity;
  entity.world = world;
  entity.handle = ecs_lookup( world, path );
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
  entity.handle = ecs_lookup_from( parent.world, parent.handle, path );
  return entity;
}

/************************************************
 *
 * ECS Entity Functions Implementatino
 * 
 ***********************************************/
crude_entity
crude_entity_create_empty_without_name
(
  _In_ ecs_world_t                                        *world
)
{
  crude_entity entity = CRUDE_COMPOUNT( crude_entity, { .handle = ecs_new( world ), .world = world } );
  ecs_add( world, entity.handle, crude_entity_tag );
  return entity;
}

crude_entity
crude_entity_create_empty
(
  _In_ ecs_world_t                                        *world,
  _In_ const char                                         *name
)
{
  crude_entity entity = CRUDE_COMPOUNT( crude_entity, { .handle = ecs_new( world ), .world = world } );
  entity.handle = ecs_set_name( world, entity.handle, name == NULL || name[0] == '\0' ? "entity" : name );
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
  return ecs_get_name( entity.world, entity.handle );
}

void
crude_entity_set_name
(
  _In_ crude_entity                                        entity,
  _In_ char const                                         *name
)
{
  ecs_set_name( entity.world, entity.handle, name );
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

  ecs_iter_t it = ecs_children( entity.world, entity.handle );

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
crude_entity_clone
(
  _In_ crude_entity                                        dst,
  _In_ crude_entity                                        src,
  _In_ bool                                                copy_value
)
{
  dst.world = src.world;
  dst.handle = ecs_clone( src.world, dst.handle, src.handle, copy_value );
  return dst;
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
  dst.handle = ecs_clone( src.world, dst.handle, src.handle, copy_value );
  return dst;
}

crude_entity
crude_entity_copy_hierarchy
(
  _In_ crude_entity                                        src,
  _In_ char const                                         *name,
  _In_ bool                                                copy_value
)
{
  if ( !crude_entity_valid( src ) )
  {
    return CRUDE_COMPOUNT_EMPTY( crude_entity );
  }
  
  crude_entity new_node = crude_entity_copy( src, copy_value );

  ecs_iter_t it = ecs_children( src.world, src.handle );

  while ( ecs_children_next( &it ) )
  {
    for ( size_t i = 0; i < it.count; ++i )
    {
      crude_entity child = CRUDE_COMPOUNT( crude_entity, { .handle = it.entities[ i ], .world = src.world } );
      crude_entity new_child = crude_entity_copy_hierarchy( child, crude_entity_get_name( child ), copy_value );

      crude_entity_set_parent( new_child, new_node );
    }
  }
  
  crude_entity_set_name( new_node, name );

  return new_node;
}