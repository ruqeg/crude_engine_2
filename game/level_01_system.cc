#include <core/memory.h>

#include <game_components.h>

#include <level_01_system.h>

CRUDE_ECS_SYSTEM_DECLARE( crude_level_01_update_system_ );
CRUDE_ECS_OBSERVER_DECLARE( crude_level_01_creation_observer_ );

static void
crude_level_01_creation_observer_
(
  ecs_iter_t *it
)
{
  crude_level_01 *enemies_per_entity = ecs_field( it, crude_level_01, 0 );

  for ( uint32 i = 0; i < it->count; ++i )
  {
    crude_level_01                                        *level;

    level = &enemies_per_entity[ i ];
    level->editor_camera_controller_enabled = true;
  }
}

static void
crude_level_01_update_system_
(
  _In_ ecs_iter_t *it
)
{
  crude_level_01 *leveles_per_entity = ecs_field( it, crude_level_01, 0 );

  for ( uint32 i = 0; i < it->count; ++i )
  {
    crude_level_01                                        *level;
    crude_entity                                           level_node;
    
    //game->game_controller_node = crude_ecs_lookup_entity_from_parent( game->scene.main_node, "player" );
    //game->game_camera_node = crude_ecs_lookup_entity_from_parent( game->scene.main_node, "player.pivot.camera" );

    level = &leveles_per_entity[ i ];
    level_node = CRUDE_COMPOUNT( crude_entity, { it->entities[ i ], it->world } );
  }
}

CRUDE_ECS_MODULE_IMPORT_IMPL( crude_level_01_system )
{
  ECS_MODULE( world, crude_level_01_system );
  
  ECS_IMPORT( world, crude_game_components );

  CRUDE_ECS_OBSERVER_DEFINE( world, crude_level_01_creation_observer_, EcsOnSet, { 
    { .id = ecs_id( crude_level_01 ) }
  } );

  CRUDE_ECS_SYSTEM_DEFINE( world, crude_level_01_update_system_, EcsOnUpdate, NULL, {
    { .id = ecs_id( crude_level_01 ) }
  } );
}