#include <SDL3/SDL.h>

#include <engine/core/memory.h>
#include <engine/external/game_components.h>
#include <engine/platform/platform.h>
#include <engine/audio/audio_device.h>
#include <game/game.h>

#include <game/level_cutscene_only_sound_system.h>

CRUDE_ECS_SYSTEM_DECLARE( crude_level_cutscene_only_sound_update_system_ );
CRUDE_ECS_OBSERVER_DECLARE( crude_level_cutscene_only_sound_creation_observer_ );
CRUDE_ECS_OBSERVER_DECLARE( crude_level_cutscene_only_sound_destroy_observer_ );

static void
crude_level_cutscene_only_sound_destroy_observer_
(
  ecs_iter_t *it
)
{
  game_t                                                  *game;
  crude_level_cutscene_only_sound                         *levels_per_entity;

  game = game_instance( );
  levels_per_entity = ecs_field( it, crude_level_cutscene_only_sound, 0 );

  for ( uint32 i = 0; i < it->count; ++i )
  {
    crude_level_cutscene_only_sound                       *level;
    crude_window_handle                                   *window_handle;
    crude_entity                                           level_node;
    
    level = &levels_per_entity[ i ];
    level_node = CRUDE_COMPOUNT( crude_entity, { it->entities[ i ], it->world } );

    switch ( level->type )
    {
    case CRUDE_LEVEL_CUTSCENE_ONLY_SOUND_TYPE_INTRO:
    {
      crude_audio_device_destroy_sound( &game->audio_device, level->sound_handle );
      break;
    }
    }
  }
}
static void
crude_level_cutscene_only_sound_creation_observer_
(
  ecs_iter_t *it
)
{
  game_t                                                  *game;
  crude_level_cutscene_only_sound                         *levels_per_entity;

  game = game_instance( );
  levels_per_entity = ecs_field( it, crude_level_cutscene_only_sound, 0 );

  for ( uint32 i = 0; i < it->count; ++i )
  {
    crude_level_cutscene_only_sound                       *level;
    crude_window_handle                                   *window_handle;
    crude_entity                                           level_node;
    crude_sound_creation                                   sound_creation;
    crude_player                                          *player;

    level = &levels_per_entity[ i ];
    level_node = CRUDE_COMPOUNT( crude_entity, { it->entities[ i ], it->world } );

    switch ( level->type )
    {
    case CRUDE_LEVEL_CUTSCENE_ONLY_SOUND_TYPE_INTRO:
    {
      sound_creation = crude_sound_creation_empty( );
      sound_creation.stream = true;
      sound_creation.looping = false;
      sound_creation.absolute_filepath = game->level_intro_sound_absolute_filepath;
      sound_creation.positioning = CRUDE_AUDIO_SOUND_POSITIONING_RELATIVE;
      level->sound_handle = crude_audio_device_create_sound( &game->audio_device, &sound_creation );
      crude_audio_device_sound_start( &game->audio_device, level->sound_handle );
      level->time_left = 44;
      break;
    }
    }

    window_handle = CRUDE_ENTITY_GET_MUTABLE_COMPONENT( game->platform_node, crude_window_handle );
    crude_platform_hide_cursor( *window_handle );
  }
}

static void
crude_level_cutscene_only_sound_update_system_
(
  _In_ ecs_iter_t *it
)
{
  game_t *game = game_instance( );
  crude_level_cutscene_only_sound *leveles_per_entity = ecs_field( it, crude_level_cutscene_only_sound, 0 );
  
  for ( uint32 i = 0; i < it->count; ++i )
  {
    crude_player                                          *player;
    crude_level_cutscene_only_sound                       *level;
    crude_entity                                           level_node;
    
    level = &leveles_per_entity[ i ];
    level_node = CRUDE_COMPOUNT( crude_entity, { it->entities[ i ], it->world } );
  
    level->time_left -= it->delta_time;

    switch ( level->type )
    {
    case CRUDE_LEVEL_CUTSCENE_ONLY_SOUND_TYPE_INTRO:
    {
      if ( !crude_audio_device_sound_is_playing( &game->audio_device, level->sound_handle ) )
      {
        game_push_load_scene_command( game, game->level_starting_room_node_absolute_filepath );
      }
      break;
    }
    }
  }
}

CRUDE_ECS_MODULE_IMPORT_IMPL( crude_level_cutscene_only_sound_system )
{
  ECS_MODULE( world, crude_level_cutscene_only_sound_system );
  
  ECS_IMPORT( world, crude_game_components );

  CRUDE_ECS_OBSERVER_DEFINE( world, crude_level_cutscene_only_sound_creation_observer_, EcsOnSet, NULL, { 
    { .id = ecs_id( crude_level_cutscene_only_sound ) }
  } );

  CRUDE_ECS_OBSERVER_DEFINE( world, crude_level_cutscene_only_sound_destroy_observer_, EcsOnRemove, NULL, { 
    { .id = ecs_id( crude_level_cutscene_only_sound ) }
  } );

  CRUDE_ECS_SYSTEM_DEFINE( world, crude_level_cutscene_only_sound_update_system_, EcsOnUpdate, NULL, {
    { .id = ecs_id( crude_level_cutscene_only_sound ) }
  } );
}