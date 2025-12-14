#include <SDL3/SDL.h>

#include <engine/core/memory.h>
#include <engine/external/game_components.h>
#include <engine/platform/platform.h>
#include <engine/audio/audio_device.h>
#include <game/game.h>

#include <game/level_boss_fight_system.h>

CRUDE_ECS_SYSTEM_DECLARE( crude_level_boss_fight_update_system_ );
CRUDE_ECS_OBSERVER_DECLARE( crude_level_boss_fight_creation_observer_ );
CRUDE_ECS_OBSERVER_DECLARE( crude_level_boss_fight_destroy_observer_ );

static void
crude_level_boss_fight_destroy_observer_
(
  ecs_iter_t *it
)
{
  game_t                                                  *game;
  crude_level_boss_fight                                  *level_per_entity;

  game = game_instance( );
  level_per_entity = ecs_field( it, crude_level_boss_fight, 0 );

  for ( uint32 i = 0; i < it->count; ++i )
  {
    crude_level_boss_fight                                *level;
    crude_window_handle                                   *window_handle;
    crude_entity                                           level_node;
    
    level = &level_per_entity[ i ];
    level_node = CRUDE_COMPOUNT( crude_entity, { it->entities[ i ], it->world } );

    crude_audio_device_destroy_sound( &game->audio_device, level->background_sound_handle );
  }
}
static void
crude_level_boss_fight_creation_observer_
(
  ecs_iter_t *it
)
{
  game_t                                                  *game;
  crude_level_boss_fight                                  *levels_per_entity;

  game = game_instance( );
  levels_per_entity = ecs_field( it, crude_level_boss_fight, 0 );

  for ( uint32 i = 0; i < it->count; ++i )
  {
    crude_level_boss_fight                                *level;
    crude_window_handle                                   *window_handle;
    crude_entity                                           level_node;
    crude_sound_creation                                   sound_creation;
    crude_player                                          *player;

    level = &levels_per_entity[ i ];
    level_node = CRUDE_COMPOUNT( crude_entity, { it->entities[ i ], it->world } );
    
    sound_creation = crude_sound_creation_empty( );
    sound_creation.stream = true;
    sound_creation.looping = true;
    sound_creation.decode = true;
    sound_creation.absolute_filepath = game->starting_room_background_sound_absolute_filepath;
    sound_creation.positioning = CRUDE_AUDIO_SOUND_POSITIONING_RELATIVE;
    level->background_sound_handle = crude_audio_device_create_sound( &game->audio_device, &sound_creation );
    crude_audio_device_sound_start( &game->audio_device, level->background_sound_handle );

    window_handle = CRUDE_ENTITY_GET_MUTABLE_COMPONENT( game->platform_node, crude_window_handle );
    crude_platform_hide_cursor( *window_handle );
  }
}

static void
crude_level_boss_fight_update_system_
(
  _In_ ecs_iter_t *it
)
{
  game_t *game = game_instance( );
  crude_level_boss_fight *leveles_per_entity = ecs_field( it, crude_level_boss_fight, 0 );

  for ( uint32 i = 0; i < it->count; ++i )
  {
    crude_player                                          *player;
    crude_level_boss_fight                                *level;
    crude_entity                                           level_node;
    
    level = &leveles_per_entity[ i ];
    level_node = CRUDE_COMPOUNT( crude_entity, { it->entities[ i ], it->world } );

    player = CRUDE_ENTITY_GET_MUTABLE_COMPONENT( game->player_node, crude_player );
  }
}

CRUDE_ECS_MODULE_IMPORT_IMPL( crude_level_boss_fight_system )
{
  ECS_MODULE( world, crude_level_boss_fight_system );
  
  ECS_IMPORT( world, crude_game_components );

  CRUDE_ECS_OBSERVER_DEFINE( world, crude_level_boss_fight_creation_observer_, EcsOnSet, NULL, { 
    { .id = ecs_id( crude_level_boss_fight ) }
  } );

  CRUDE_ECS_OBSERVER_DEFINE( world, crude_level_boss_fight_destroy_observer_, EcsOnRemove, NULL, { 
    { .id = ecs_id( crude_level_boss_fight ) }
  } );

  CRUDE_ECS_SYSTEM_DEFINE( world, crude_level_boss_fight_update_system_, EcsOnUpdate, NULL, {
    { .id = ecs_id( crude_level_boss_fight ) }
  } );
}