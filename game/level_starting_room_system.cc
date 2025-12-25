#include <SDL3/SDL.h>

#include <engine/core/memory.h>
#include <engine/external/game_components.h>
#include <engine/platform/platform.h>
#include <engine/audio/audio_device.h>
#include <game/game.h>

#include <game/level_starting_room_system.h>

CRUDE_ECS_SYSTEM_DECLARE( crude_level_starting_room_update_system_ );
CRUDE_ECS_OBSERVER_DECLARE( crude_level_starting_room_creation_observer_ );
CRUDE_ECS_OBSERVER_DECLARE( crude_level_starting_room_destroy_observer_ );

static void
crude_level_starting_room_destroy_observer_
(
  ecs_iter_t *it
)
{
  game_t                                                  *game;
  crude_level_starting_room                               *level_starting_room_per_entity;

  game = game_instance( );
  level_starting_room_per_entity = ecs_field( it, crude_level_starting_room, 0 );

  for ( uint32 i = 0; i < it->count; ++i )
  {
    crude_level_starting_room                             *level;
    //crude_window_handle                                   *window_handle;
    //crude_entity                                           level_node;
    //
    //level = &level_starting_room_per_entity[ i ];
    //level_node = CRUDE_COMPOUNT( crude_entity, { it->entities[ i ], it->world } );
    //
    //crude_audio_device_destroy_sound( &game->audio_device, level->background_sound_handle );
    //crude_audio_device_destroy_sound( &game->audio_device, level->voiceline0_sound_handle );
    //crude_audio_device_destroy_sound( &game->audio_device, level->voiceline1_sound_handle );
    //crude_audio_device_destroy_sound( &game->audio_device, level->voiceline2_sound_handle );
    //crude_audio_device_destroy_sound( &game->audio_device, level->voiceline3_sound_handle );
    //crude_audio_device_destroy_sound( &game->audio_device, level->hit_0_sound_handle );
    //crude_audio_device_destroy_sound( &game->audio_device, level->hit_1_sound_handle );
    //crude_audio_device_destroy_sound( &game->audio_device, level->hit_2_sound_handle );
  }
}
static void
crude_level_starting_room_creation_observer_
(
  ecs_iter_t *it
)
{
  game_t                                                  *game;
  crude_level_starting_room                               *level_starting_room_per_entity;

  game = game_instance( );
  level_starting_room_per_entity = ecs_field( it, crude_level_starting_room, 0 );

  for ( uint32 i = 0; i < it->count; ++i )
  {
    crude_level_starting_room                             *level;
    crude_entity                                           level_node;
    crude_sound_creation                                   sound_creation;
    crude_player                                          *player;

    level = &level_starting_room_per_entity[ i ];
    level_node = CRUDE_COMPOUNT( crude_entity, { it->entities[ i ], it->world } );
    
    crude_gfx_model_renderer_resources_manager_get_gltf_model( &game->engine->model_renderer_resources_manager, game->starting_room_modern_syringe_health_model_absolute_filepath, NULL );
    crude_gfx_model_renderer_resources_manager_get_gltf_model( &game->engine->model_renderer_resources_manager, game->starting_room_modern_syringe_drug_model_absolute_filepath, NULL );

    level->state = CRUDE_LEVEL_STARTING_ROOM_STATE_NEED_HIT;

    sound_creation = crude_sound_creation_empty( );
    sound_creation.stream = true;
    sound_creation.looping = true;
    sound_creation.decode = true;
    sound_creation.absolute_filepath = game->starting_room_background_sound_absolute_filepath;
    sound_creation.positioning = CRUDE_AUDIO_SOUND_POSITIONING_RELATIVE;
    level->background_sound_handle = crude_audio_device_create_sound( &game->engine->audio_device, &sound_creation );
    crude_audio_device_sound_start( &game->engine->audio_device, level->background_sound_handle );
    
    sound_creation = crude_sound_creation_empty( );
    sound_creation.async_loading = true;
    sound_creation.absolute_filepath = game->starting_room_voiceline0_sound_absolute_filepath;
    sound_creation.positioning = CRUDE_AUDIO_SOUND_POSITIONING_RELATIVE;
    level->voiceline0_sound_handle = crude_audio_device_create_sound( &game->engine->audio_device, &sound_creation );
    crude_audio_device_sound_start( &game->engine->audio_device, level->voiceline0_sound_handle );

    sound_creation.absolute_filepath = game->starting_room_voiceline1_sound_absolute_filepath;
    level->voiceline1_sound_handle = crude_audio_device_create_sound( &game->engine->audio_device, &sound_creation );

    sound_creation.absolute_filepath = game->starting_room_voiceline2_sound_absolute_filepath;
    level->voiceline2_sound_handle = crude_audio_device_create_sound( &game->engine->audio_device, &sound_creation );

    sound_creation.absolute_filepath = game->starting_room_voiceline3_sound_absolute_filepath;
    level->voiceline3_sound_handle = crude_audio_device_create_sound( &game->engine->audio_device, &sound_creation );

    sound_creation = crude_sound_creation_empty( );
    sound_creation.async_loading = true;
    sound_creation.absolute_filepath = game->hit_0_sound_absolute_filepath;
    sound_creation.positioning = CRUDE_AUDIO_SOUND_POSITIONING_RELATIVE;
    level->hit_0_sound_handle = crude_audio_device_create_sound( &game->engine->audio_device, &sound_creation );

    sound_creation = crude_sound_creation_empty( );
    sound_creation.async_loading = true;
    sound_creation.absolute_filepath = game->hit_1_sound_absolute_filepath;
    sound_creation.positioning = CRUDE_AUDIO_SOUND_POSITIONING_RELATIVE;
    level->hit_1_sound_handle = crude_audio_device_create_sound( &game->engine->audio_device, &sound_creation );

    sound_creation = crude_sound_creation_empty( );
    sound_creation.async_loading = true;
    sound_creation.absolute_filepath = game->hit_2_sound_absolute_filepath;
    sound_creation.positioning = CRUDE_AUDIO_SOUND_POSITIONING_RELATIVE;
    level->hit_2_sound_handle = crude_audio_device_create_sound( &game->engine->audio_device, &sound_creation );

    player = CRUDE_ENTITY_GET_MUTABLE_COMPONENT( crude_ecs_lookup_entity_from_parent( level_node, "player" ), crude_player );
    player->sanity = 0.2;

    //window_handle = CRUDE_ENTITY_GET_MUTABLE_COMPONENT( game->engine->platform_node, crude_window_handle );
    //crude_platform_hide_cursor( *window_handle );
  }
}

static void
crude_level_starting_room_update_system_
(
  _In_ ecs_iter_t *it
)
{
  game_t *game = game_instance( );
  crude_level_starting_room *leveles_per_entity = ecs_field( it, crude_level_starting_room, 0 );

  for ( uint32 i = 0; i < it->count; ++i )
  {
    crude_player                                          *player;
    crude_level_starting_room                             *level;
    crude_entity                                           level_node;
    
    level = &leveles_per_entity[ i ];
    level_node = CRUDE_COMPOUNT( crude_entity, { it->entities[ i ], it->world } );

    player = CRUDE_ENTITY_GET_MUTABLE_COMPONENT( game->player_node, crude_player );
    if ( level->state == CRUDE_LEVEL_STARTING_ROOM_STATE_NEED_HIT )
    {
      if ( player->sanity > 1.0 )
      {
        level->state = CRUDE_LEVEL_STARTING_ROOM_STATE_NEED_HEALTH;
        player->health = 0.2;
        crude_audio_device_sound_start( &game->engine->audio_device, level->voiceline1_sound_handle );
      }
      else
      {
        player->drug_withdrawal = 0.0;
      }
    }
    if ( level->state == CRUDE_LEVEL_STARTING_ROOM_STATE_NEED_HEALTH )
    {
      if ( player->health > 0.99 && !crude_audio_device_sound_is_playing( &game->engine->audio_device, level->voiceline1_sound_handle ) )
      {
        level->state = CRUDE_LEVEL_STARTING_ROOM_STATE_NEED_DRUG;
        player->drug_withdrawal = 0.85;
        crude_audio_device_sound_start( &game->engine->audio_device, level->voiceline2_sound_handle );
      }
      else
      {
        player->sanity = 1.0;
        player->drug_withdrawal = 0.0;
      }
    }
    if ( level->state == CRUDE_LEVEL_STARTING_ROOM_STATE_NEED_DRUG )
    {
      if ( player->drug_withdrawal < 0.01 && !crude_audio_device_sound_is_playing( &game->engine->audio_device, level->voiceline2_sound_handle ) )
      {
        crude_audio_device_sound_start( &game->engine->audio_device, level->voiceline3_sound_handle );
        level->state = CRUDE_LEVEL_STARTING_ROOM_STATE_NEED_CAN_MOVE;
      }
      else
      {
        player->sanity = 1.0;
      }
    }
  }
}

CRUDE_ECS_MODULE_IMPORT_IMPL( crude_level_starting_room_system )
{
  ECS_MODULE( world, crude_level_starting_room_system );
  
  ECS_IMPORT( world, crude_game_components );

  CRUDE_ECS_OBSERVER_DEFINE( world, crude_level_starting_room_creation_observer_, EcsOnSet, NULL, { 
    { .id = ecs_id( crude_level_starting_room ) }
  } );

  CRUDE_ECS_OBSERVER_DEFINE( world, crude_level_starting_room_destroy_observer_, EcsOnRemove, NULL, { 
    { .id = ecs_id( crude_level_starting_room ) }
  } );

  CRUDE_ECS_SYSTEM_DEFINE( world, crude_level_starting_room_update_system_, EcsOnUpdate, NULL, {
    { .id = ecs_id( crude_level_starting_room ) }
  } );
}