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

#include <game/recycle_station_system.h>

CRUDE_ECS_SYSTEM_DECLARE( crude_recycle_station_update_system_ );
CRUDE_ECS_OBSERVER_DECLARE( crude_recycle_station_creation_observer_ );

void
crude_recycle_station_start_recycle
(
  _In_ crude_recycle_station                              *station,
  _In_ crude_player                                       *player,
  _In_ crude_entity                                        station_node
)
{
  game_t *game = game_instance( );

  crude_transform *station_transform = CRUDE_ENTITY_GET_MUTABLE_COMPONENT( station_node, crude_transform );
  XMMATRIX station_node_to_world = crude_transform_node_to_world( station_node, station_transform );

  if ( station->state == CRUDE_RECYCLE_STATION_STATE_DONE )
  {
    for ( uint32 i = 0; i < CRUDE_GAME_PLAYER_ITEMS_MAX_COUNT; ++i )
    {
      if ( player->inventory_items[ i ] == CRUDE_GAME_ITEM_NONE )
      {
        station->state = CRUDE_RECYCLE_STATION_STATE_EMPTY;
        game_player_set_item( game, player, i, station->game_item );
        
        crude_level_01 *level = CRUDE_ENTITY_GET_MUTABLE_COMPONENT( game->main_node, crude_level_01 );
        crude_audio_device_sound_set_translation( &game->audio_device, level->recycle_interaction_sound_handle, station_node_to_world.r[ 3 ] );
        crude_audio_device_sound_start( &game->audio_device, level->recycle_interaction_sound_handle );

        crude_gltf *serum_loaded_gltf = CRUDE_ENTITY_GET_MUTABLE_COMPONENT( crude_ecs_lookup_entity_from_parent( station_node, "serum_loaded" ), crude_gltf );
        crude_gltf *serum_empty_gltf = CRUDE_ENTITY_GET_MUTABLE_COMPONENT( crude_ecs_lookup_entity_from_parent( station_node, "serum_empty" ), crude_gltf );
        serum_loaded_gltf->hidden = true;
        serum_empty_gltf->hidden = true;
        break;
      }
    }
  }
  else if ( station->state == CRUDE_RECYCLE_STATION_STATE_EMPTY )
  {
    for ( uint32 i = 0; i < CRUDE_GAME_PLAYER_ITEMS_MAX_COUNT; ++i )
    {
      if ( player->inventory_items[ i ] == CRUDE_GAME_ITEM_SERUM )
      {
        station->last_recycle_time = 0.f;
        station->state = CRUDE_RECYCLE_STATION_STATE_ACTIVE;
        
        game_player_set_item( game, player, i, CRUDE_GAME_ITEM_NONE );
        
        crude_level_01 *level = CRUDE_ENTITY_GET_MUTABLE_COMPONENT( game->main_node, crude_level_01 );
        crude_audio_device_sound_start( &game->audio_device, level->recycle_sound_handle );

        crude_audio_device_sound_set_translation( &game->audio_device, level->recycle_interaction_sound_handle, station_node_to_world.r[ 3 ] );
        crude_audio_device_sound_start( &game->audio_device, level->recycle_interaction_sound_handle );
        
        crude_gltf *serum_loaded_gltf = CRUDE_ENTITY_GET_MUTABLE_COMPONENT( crude_ecs_lookup_entity_from_parent( station_node, "serum_loaded" ), crude_gltf );
        crude_gltf *serum_empty_gltf = CRUDE_ENTITY_GET_MUTABLE_COMPONENT( crude_ecs_lookup_entity_from_parent( station_node, "serum_empty" ), crude_gltf );
        serum_loaded_gltf->hidden = false;
        serum_empty_gltf->hidden = true;
        break;
      }
    }
  }
}

static void
crude_recycle_station_creation_observer_
(
  ecs_iter_t *it
)
{
  game_t *game = game_instance( );

  crude_recycle_station *recycle_station_per_entity = ecs_field( it, crude_recycle_station, 0 );

  for ( uint32 i = 0; i < it->count; ++i )
  {
    crude_recycle_station                                 *recycle_station;
    crude_entity                                           recycle_station_node;

    recycle_station = &recycle_station_per_entity[ i ];
    recycle_station_node = CRUDE_COMPOUNT( crude_entity, { it->entities[ i ], it->world } );

    recycle_station->state = CRUDE_RECYCLE_STATION_STATE_EMPTY;
    recycle_station->last_recycle_time = 100000000.f;
  }
}

static void
crude_recycle_station_update_system_
(
  _In_ ecs_iter_t *it
)
{
  CRUDE_PROFILER_ZONE_NAME( "crude_recycle_station_update_system_" );
  game_t *game = game_instance( );
  crude_recycle_station *recycle_station_per_entity = ecs_field( it, crude_recycle_station, 0 );
  
  for ( uint32 i = 0; i < it->count; ++i )
  {
    crude_recycle_station                                 *recycle_station;
    crude_entity                                           recycle_station_node;

    recycle_station = &recycle_station_per_entity[ i ];
    recycle_station_node = CRUDE_COMPOUNT( crude_entity, { it->entities[ i ], it->world } );

    recycle_station->last_recycle_time += it->delta_time;
    if ( recycle_station->last_recycle_time > 6.0 && recycle_station->state == CRUDE_RECYCLE_STATION_STATE_ACTIVE )
    {
      crude_gltf *serum_loaded_gltf = CRUDE_ENTITY_GET_MUTABLE_COMPONENT( crude_ecs_lookup_entity_from_parent( recycle_station_node, "serum_loaded" ), crude_gltf );
      crude_gltf *serum_empty_gltf = CRUDE_ENTITY_GET_MUTABLE_COMPONENT( crude_ecs_lookup_entity_from_parent( recycle_station_node, "serum_empty" ), crude_gltf );
      serum_loaded_gltf->hidden = true;
      serum_empty_gltf->hidden = false;
      recycle_station->state = CRUDE_RECYCLE_STATION_STATE_DONE;
    }
  }
  CRUDE_PROFILER_ZONE_END;
}

CRUDE_ECS_MODULE_IMPORT_IMPL( crude_recycle_station_system )
{
  ECS_MODULE( world, crude_recycle_station_system );

  ECS_IMPORT( world, crude_scene_components );
  ECS_IMPORT( world, crude_physics_components );
  ECS_IMPORT( world, crude_game_components );

  CRUDE_ECS_OBSERVER_DEFINE( world, crude_recycle_station_creation_observer_, EcsOnSet, NULL, {
    { .id = ecs_id( crude_recycle_station ) }
  } );

  CRUDE_ECS_SYSTEM_DEFINE( world, crude_recycle_station_update_system_, EcsOnUpdate, NULL, {
    { .id = ecs_id( crude_recycle_station ) }
  } );
}