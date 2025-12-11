#include <engine/core/log.h>
#include <engine/core/assert.h>
#include <engine/core/profiler.h>
#include <engine/scene/scripts_components.h>
#include <engine/scene/scene_components.h>
#include <engine/platform/platform_components.h>
#include <engine/physics/physics_components.h>
#include <engine/external/game_components.h>
#include <engine/physics/physics.h>
#include <game/game.h>

#include <game/serum_station_system.h>

/*****
 * 
 * Constant
 * 
 *******/

CRUDE_ECS_SYSTEM_DECLARE( crude_serum_station_update_system_ );
CRUDE_ECS_OBSERVER_DECLARE( crude_serum_station_creation_observer_ );
CRUDE_ECS_OBSERVER_DECLARE( crude_serum_station_enabled_creation_observer_ );
CRUDE_ECS_OBSERVER_DECLARE( crude_serum_station_enabled_destroy_observer_ );

static void
crude_serum_station_creation_observer_
(
  ecs_iter_t *it
)
{
  game_t *game = game_instance( );

  crude_serum_station *serum_station_per_entity = ecs_field( it, crude_serum_station, 0 );

  for ( uint32 i = 0; i < it->count; ++i )
  {
    crude_serum_station                                   *serum_station;
    crude_entity                                           serum_station_node;

    serum_station = &serum_station_per_entity[ i ];
    serum_station_node = CRUDE_COMPOUNT( crude_entity, { it->entities[ i ], it->world } );
    CRUDE_ENTITY_SET_COMPONENT( serum_station_node, crude_gltf, { game->serum_station_disabled_model_absolute_filepath } );
  }
}

static void
crude_serum_station_update_system_
(
  _In_ ecs_iter_t *it
)
{
  CRUDE_PROFILER_ZONE_NAME( "crude_serum_station_update_system_" );
  game_t *game = game_instance( );
  crude_serum_station *serum_station_per_entity = ecs_field( it, crude_serum_station, 0 );
  crude_physics_static_body_handle *static_body_handle_per_entity = ecs_field( it, crude_physics_static_body_handle, 1 );
  crude_transform *transform_per_entity = ecs_field( it, crude_transform, 2 );
  
  for ( uint32 i = 0; i < it->count; ++i )
  {
    crude_serum_station                                   *serum_station;
    crude_transform                                       *serum_station_transform;
    crude_physics_static_body_handle                      *serum_station_static_body_handle;
    crude_physics_static_body                             *serum_station_static_body;
    crude_entity                                           serum_station_node;

    serum_station = &serum_station_per_entity[ i ];
    serum_station_static_body_handle = &static_body_handle_per_entity[ i ];
    serum_station_transform = &transform_per_entity[ i ];
    serum_station_node = CRUDE_COMPOUNT( crude_entity, { it->entities[ i ], it->world } );

    CRUDE_ASSERT( crude_entity_valid( game->player_node ) );
    
    serum_station_static_body = crude_physics_resources_manager_access_static_body( &game->physics_resources_manager, *serum_station_static_body_handle );
  }
  CRUDE_PROFILER_ZONE_END;
}

static void
crude_serum_station_enabled_creation_observer_
(
  ecs_iter_t *it
)
{
  game_t *game = game_instance( );

  for ( uint32 i = 0; i < it->count; ++i )
  {
    crude_entity                                           serum_station_node;
    
    serum_station_node = CRUDE_COMPOUNT( crude_entity, { it->entities[ i ], it->world } );

    CRUDE_ENTITY_SET_COMPONENT( serum_station_node, crude_gltf, { game->serum_station_enabled_model_absolute_filepath } );
  }
}

static void
crude_serum_station_enabled_destroy_observer_
(
  ecs_iter_t *it
)
{
  game_t *game = game_instance( );

  for ( uint32 i = 0; i < it->count; ++i )
  {
    crude_entity                                           serum_station_node;
    
    serum_station_node = CRUDE_COMPOUNT( crude_entity, { it->entities[ i ], it->world } );
    
    CRUDE_ENTITY_SET_COMPONENT( serum_station_node, crude_gltf, { game->serum_station_disabled_model_absolute_filepath } );
  }
}

CRUDE_ECS_MODULE_IMPORT_IMPL( crude_serum_station_system )
{
  ECS_MODULE( world, crude_serum_station_system );

  ECS_IMPORT( world, crude_scene_components );
  ECS_IMPORT( world, crude_physics_components );
  ECS_IMPORT( world, crude_game_components );

  CRUDE_ECS_OBSERVER_DEFINE( world, crude_serum_station_creation_observer_, EcsOnSet, NULL, {
    { .id = ecs_id( crude_serum_station ) }
  } );

  CRUDE_ECS_SYSTEM_DEFINE( world, crude_serum_station_update_system_, EcsOnUpdate, NULL, {
    { .id = ecs_id( crude_serum_station ) },
    { .id = ecs_id( crude_physics_static_body_handle ) },
    { .id = ecs_id( crude_transform ) }
  } );

  CRUDE_ECS_OBSERVER_DEFINE( world, crude_serum_station_enabled_creation_observer_, EcsOnSet, NULL, { 
    { .id = ecs_id( crude_serum_station_enabled ) }
  } );

  CRUDE_ECS_OBSERVER_DEFINE( world, crude_serum_station_enabled_destroy_observer_, EcsOnRemove, NULL, { 
    { .id = ecs_id( crude_serum_station_enabled ) }
  } );
}