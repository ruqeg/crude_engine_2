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

#include <game/teleport_station_system.h>

/*****
 * 
 * Constant
 * 
 *******/

CRUDE_ECS_SYSTEM_DECLARE( crude_teleport_station_update_system_ );

void
crude_teleport_station_set_serum
(
  _In_ crude_teleport_station                             *teleport_station,
  _In_ crude_player                                       *player,
  _In_ crude_entity                                        teleport_station_node
)
{
  game_t                                                  *game;
  ecs_iter_t                                               serums_it;
  uint32                                                   serums_count;
  
  game = game_instance( );
  serums_it = ecs_children( teleport_station_node.world, crude_ecs_lookup_entity_from_parent( teleport_station_node, "serums" ).handle );
  while ( ecs_children_next( &serums_it ) )
  {
    for ( size_t i = 0; i < serums_it.count; ++i )
    {
      crude_gltf                                        *serum_gltf;
      crude_entity                                       serum_node;
  
      serum_node = CRUDE_COMPOUNT( crude_entity, { .handle = serums_it.entities[ i ], .world = serums_it.world } );
      serum_gltf = CRUDE_ENTITY_GET_MUTABLE_COMPONENT( serum_node, crude_gltf );

      if ( serum_gltf->hidden )
      {
        for ( uint32 i = 0; i < CRUDE_GAME_PLAYER_ITEMS_MAX_COUNT; ++i )
        {
          if ( player->inventory_items[ i ] == CRUDE_GAME_ITEM_SERUM )
          {
            crude_transform                               *serum_transform;
            serum_transform = CRUDE_ENTITY_GET_MUTABLE_COMPONENT( serum_node, crude_transform );
            crude_audio_device_sound_set_translation( &game->audio_device, game->recycle_interaction_sound_handle, crude_transform_node_to_world( serum_node, serum_transform ).r[ 3 ] );
            crude_audio_device_sound_start( &game->audio_device, game->recycle_interaction_sound_handle );
            game_player_set_item( game, player, i, CRUDE_GAME_ITEM_NONE );
            serum_gltf->hidden = false;
            break;
          }
        }
        return;
      }
    }
  }

  CRUDE_LOG_INFO( CRUDE_CHANNEL_ALL, "Done" );
}

static void
crude_teleport_station_update_system_
(
  _In_ ecs_iter_t *it
)
{
  CRUDE_PROFILER_ZONE_NAME( "crude_teleport_station_update_system_" );
  game_t *game = game_instance( );
  crude_teleport_station *teleport_station_per_entity = ecs_field( it, crude_teleport_station, 0 );
  
  for ( uint32 i = 0; i < it->count; ++i )
  {
    crude_teleport_station                                *teleport_station;
    crude_entity                                           teleport_station_node;

    teleport_station = &teleport_station_per_entity[ i ];
    teleport_station_node = CRUDE_COMPOUNT( crude_entity, { it->entities[ i ], it->world } );

    CRUDE_ASSERT( crude_entity_valid( game->player_node ) );
  }
  CRUDE_PROFILER_END;
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

CRUDE_ECS_MODULE_IMPORT_IMPL( crude_teleport_station_system )
{
  ECS_MODULE( world, crude_teleport_station_system );

  ECS_IMPORT( world, crude_scene_components );
  ECS_IMPORT( world, crude_physics_components );
  ECS_IMPORT( world, crude_game_components );

  CRUDE_ECS_SYSTEM_DEFINE( world, crude_teleport_station_update_system_, EcsOnUpdate, NULL, {
    { .id = ecs_id( crude_teleport_station ) }
  } );
}