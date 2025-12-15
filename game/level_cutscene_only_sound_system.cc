#include <SDL3/SDL.h>

#include <engine/core/time.h>
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

    crude_audio_device_destroy_sound( &game->audio_device, level->sound_handle );
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
    
    level->first_system_run = true;

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
      
      game->focused_camera_node = crude_ecs_lookup_entity_from_parent( level_node, "camera" );

      game->game_postprocessing_pass.options.fog_distance = 10.f;
      break;
    }
    case CRUDE_LEVEL_CUTSCENE_ONLY_SOUND_TYPE_CUTSCENE0:
    {
      sound_creation = crude_sound_creation_empty( );
      sound_creation.stream = true;
      sound_creation.looping = false;
      sound_creation.absolute_filepath = game->level_cutscene0_sound_absolute_filepath;
      sound_creation.positioning = CRUDE_AUDIO_SOUND_POSITIONING_RELATIVE;
      level->sound_handle = crude_audio_device_create_sound( &game->audio_device, &sound_creation );
      
      game->focused_camera_node = crude_ecs_lookup_entity_from_parent( level_node, "camera" );

      game->game_postprocessing_pass.options.fog_distance = 15.f;
      break;
    }
    case CRUDE_LEVEL_CUTSCENE_ONLY_SOUND_TYPE_CUTSCENE1:
    {
      sound_creation = crude_sound_creation_empty( );
      sound_creation.stream = true;
      sound_creation.looping = false;
      sound_creation.absolute_filepath = game->level_cutscene1_sound_absolute_filepath;
      sound_creation.positioning = CRUDE_AUDIO_SOUND_POSITIONING_RELATIVE;
      level->sound_handle = crude_audio_device_create_sound( &game->audio_device, &sound_creation );
      
      game->focused_camera_node = crude_ecs_lookup_entity_from_parent( level_node, "camera" );

      game->game_postprocessing_pass.options.fog_distance = 20.f;
      
      ecs_iter_t                                               serums_it;
      uint32                                                   serums_count;
      crude_entity                                             teleport_station_node;
      
      teleport_station_node = crude_ecs_lookup_entity_from_parent( level_node, "teleport_station" );
      serums_it = ecs_children( teleport_station_node.world, crude_ecs_lookup_entity_from_parent( teleport_station_node, "models_node.serums" ).handle );
      while ( ecs_children_next( &serums_it ) )
      {
        for ( size_t i = 0; i < serums_it.count; ++i )
        {
          crude_gltf                                        *serum_gltf;
          crude_entity                                       serum_node;
      
          serum_node = CRUDE_COMPOUNT( crude_entity, { .handle = serums_it.entities[ i ], .world = serums_it.world } );
          serum_gltf = CRUDE_ENTITY_GET_MUTABLE_COMPONENT( serum_node, crude_gltf );
          serum_gltf->hidden = false;
        }
      }
      break;
    }
    case CRUDE_LEVEL_CUTSCENE_ONLY_SOUND_TYPE_CUTSCENE2:
    {
      sound_creation = crude_sound_creation_empty( );
      sound_creation.stream = true;
      sound_creation.looping = false;
      sound_creation.absolute_filepath = game->level_cutscene2_sound_absolute_filepath;
      sound_creation.positioning = CRUDE_AUDIO_SOUND_POSITIONING_RELATIVE;
      level->sound_handle = crude_audio_device_create_sound( &game->audio_device, &sound_creation );
      game->focused_camera_node = crude_ecs_lookup_entity_from_parent( level_node, "camera" );
      game->game_postprocessing_pass.options.fog_distance = 1000.f;
      break;
    }
    case CRUDE_LEVEL_CUTSCENE_ONLY_SOUND_TYPE_CUTSCENE3:
    {
      sound_creation = crude_sound_creation_empty( );
      sound_creation.stream = true;
      sound_creation.looping = false;
      sound_creation.absolute_filepath = game->level_cutscene3_sound_absolute_filepath;
      sound_creation.positioning = CRUDE_AUDIO_SOUND_POSITIONING_RELATIVE;
      level->sound_handle = crude_audio_device_create_sound( &game->audio_device, &sound_creation );
      game->focused_camera_node = crude_ecs_lookup_entity_from_parent( level_node, "camera" );
      game->game_postprocessing_pass.options.fog_distance = 1000.f;
      break;
    }
    case CRUDE_LEVEL_CUTSCENE_ONLY_SOUND_TYPE_CUTSCENE4:
    {
      sound_creation = crude_sound_creation_empty( );
      sound_creation.stream = true;
      sound_creation.looping = false;
      sound_creation.absolute_filepath = game->level_cutscene4_sound_absolute_filepath;
      sound_creation.positioning = CRUDE_AUDIO_SOUND_POSITIONING_RELATIVE;
      level->sound_handle = crude_audio_device_create_sound( &game->audio_device, &sound_creation );
      game->focused_camera_node = crude_ecs_lookup_entity_from_parent( level_node, "camera" );
      game->game_postprocessing_pass.options.fog_distance = 1000.f;
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
    
    if ( level->first_system_run )
    {
      switch ( level->type )
      {
      case CRUDE_LEVEL_CUTSCENE_ONLY_SOUND_TYPE_INTRO:
      {
        crude_audio_device_sound_start( &game->audio_device, level->sound_handle );
        break;
      }
      case CRUDE_LEVEL_CUTSCENE_ONLY_SOUND_TYPE_CUTSCENE0:
      {
        crude_audio_device_sound_start( &game->audio_device, level->sound_handle );
        break;
      }
      case CRUDE_LEVEL_CUTSCENE_ONLY_SOUND_TYPE_CUTSCENE1:
      {
        crude_audio_device_sound_start( &game->audio_device, level->sound_handle );
        break;
      }
      case CRUDE_LEVEL_CUTSCENE_ONLY_SOUND_TYPE_CUTSCENE2:
      {
        crude_audio_device_sound_start( &game->audio_device, level->sound_handle );
        break;
      }
      case CRUDE_LEVEL_CUTSCENE_ONLY_SOUND_TYPE_CUTSCENE3:
      {
        crude_audio_device_sound_start( &game->audio_device, level->sound_handle );
        break;
      }
      case CRUDE_LEVEL_CUTSCENE_ONLY_SOUND_TYPE_CUTSCENE4:
      {
        crude_entity yellow_balls_node = crude_ecs_lookup_entity_from_parent( game->main_node, "yellow_balls_node" );
        crude_transform *yellow_balls_node_transform = CRUDE_ENTITY_GET_MUTABLE_COMPONENT( yellow_balls_node, crude_transform );
        yellow_balls_node_transform->translation.y = -5;
        crude_audio_device_sound_start( &game->audio_device, level->sound_handle );
        break;
      }
      }
      level->time = crude_time_now( );
      level->first_system_run = false;
      continue;
    }

    switch ( level->type )
    {
    case CRUDE_LEVEL_CUTSCENE_ONLY_SOUND_TYPE_INTRO:
    {
      if ( !crude_audio_device_sound_is_playing( &game->audio_device, level->sound_handle ) )
      {
        game_push_load_scene_command( game, game->level_starting_room_node_absolute_filepath );
      }
      crude_transform *camera_transform = CRUDE_ENTITY_GET_MUTABLE_COMPONENT( game->focused_camera_node, crude_transform );
      float32 time_from_start = crude_time_delta_seconds( level->time, crude_time_now( ) );
      if ( time_from_start < 17.f )
      {
        crude_entity from_entity = crude_ecs_lookup_entity_from_parent( game->main_node, "ap0" );
        crude_entity to_entity = crude_ecs_lookup_entity_from_parent( game->main_node, "ap1" );
        crude_transform *from_transform = CRUDE_ENTITY_GET_MUTABLE_COMPONENT( from_entity, crude_transform );
        crude_transform *to_transform = CRUDE_ENTITY_GET_MUTABLE_COMPONENT( to_entity, crude_transform );
        float32 t = time_from_start / 17;
        XMStoreFloat3( &camera_transform->translation, XMVectorLerp( XMLoadFloat3( &from_transform->translation ), XMLoadFloat3( &to_transform->translation ), t ) );
        XMStoreFloat4( &camera_transform->rotation, XMQuaternionSlerp( XMLoadFloat4( &from_transform->rotation ), XMLoadFloat4( &to_transform->rotation ), t ) );
      }
      else if ( time_from_start > 32.f && time_from_start < 45.f )
      {
        crude_entity from_entity = crude_ecs_lookup_entity_from_parent( game->main_node, "ap1" );
        crude_entity to_entity = crude_ecs_lookup_entity_from_parent( game->main_node, "ap2" );
        crude_transform *from_transform = CRUDE_ENTITY_GET_MUTABLE_COMPONENT( from_entity, crude_transform );
        crude_transform *to_transform = CRUDE_ENTITY_GET_MUTABLE_COMPONENT( to_entity, crude_transform );
        float32 t = ( time_from_start - 32 ) / (45 - 32);
        XMStoreFloat3( &camera_transform->translation, XMVectorLerp( XMLoadFloat3( &from_transform->translation ), XMLoadFloat3( &to_transform->translation ), t ) );
        XMStoreFloat4( &camera_transform->rotation, XMQuaternionSlerp( XMLoadFloat4( &from_transform->rotation ), XMLoadFloat4( &to_transform->rotation ), t ) );
      }
      break;
    }
    case CRUDE_LEVEL_CUTSCENE_ONLY_SOUND_TYPE_CUTSCENE0:
    {
      if ( !crude_audio_device_sound_is_playing( &game->audio_device, level->sound_handle ) )
      {
        game_push_load_scene_command( game, game->level_0_node_absolute_filepath );
      }
      crude_transform *camera_transform = CRUDE_ENTITY_GET_MUTABLE_COMPONENT( game->focused_camera_node, crude_transform );
      float32 time_from_start = crude_time_delta_seconds( level->time, crude_time_now( ) );
      if ( time_from_start < 20.f )
      {
        crude_entity from_entity = crude_ecs_lookup_entity_from_parent( game->main_node, "ap0" );
        crude_entity to_entity = crude_ecs_lookup_entity_from_parent( game->main_node, "ap1" );
        crude_transform *from_transform = CRUDE_ENTITY_GET_MUTABLE_COMPONENT( from_entity, crude_transform );
        crude_transform *to_transform = CRUDE_ENTITY_GET_MUTABLE_COMPONENT( to_entity, crude_transform );
        float32 t = time_from_start / 20;
        XMStoreFloat3( &camera_transform->translation, XMVectorLerp( XMLoadFloat3( &from_transform->translation ), XMLoadFloat3( &to_transform->translation ), t ) );
        XMStoreFloat4( &camera_transform->rotation, XMQuaternionSlerp( XMLoadFloat4( &from_transform->rotation ), XMLoadFloat4( &to_transform->rotation ), t ) );
      }
      else if ( time_from_start > 29.f && time_from_start < 34.f )
      {
        crude_entity from_entity = crude_ecs_lookup_entity_from_parent( game->main_node, "ap1" );
        crude_entity to_entity = crude_ecs_lookup_entity_from_parent( game->main_node, "ap2" );
        crude_transform *from_transform = CRUDE_ENTITY_GET_MUTABLE_COMPONENT( from_entity, crude_transform );
        crude_transform *to_transform = CRUDE_ENTITY_GET_MUTABLE_COMPONENT( to_entity, crude_transform );
        float32 t = ( time_from_start - 29.f ) / (34 - 29);
        XMStoreFloat3( &camera_transform->translation, XMVectorLerp( XMLoadFloat3( &from_transform->translation ), XMLoadFloat3( &to_transform->translation ), t ) );
        XMStoreFloat4( &camera_transform->rotation, XMQuaternionSlerp( XMLoadFloat4( &from_transform->rotation ), XMLoadFloat4( &to_transform->rotation ), t ) );
      }
      else if ( time_from_start > 34.f && time_from_start < 63.f )
      {
        crude_entity from_entity = crude_ecs_lookup_entity_from_parent( game->main_node, "ap2" );
        crude_entity to_entity = crude_ecs_lookup_entity_from_parent( game->main_node, "ap3" );
        crude_transform *from_transform = CRUDE_ENTITY_GET_MUTABLE_COMPONENT( from_entity, crude_transform );
        crude_transform *to_transform = CRUDE_ENTITY_GET_MUTABLE_COMPONENT( to_entity, crude_transform );
        float32 t = ( time_from_start - 34 ) / (64 - 34);
        XMStoreFloat3( &camera_transform->translation, XMVectorLerp( XMLoadFloat3( &from_transform->translation ), XMLoadFloat3( &to_transform->translation ), t ) );
        XMStoreFloat4( &camera_transform->rotation, XMQuaternionSlerp( XMLoadFloat4( &from_transform->rotation ), XMLoadFloat4( &to_transform->rotation ), t ) );
      }
      else if ( time_from_start > 63.f && time_from_start < 72.f )
      {
        crude_entity from_entity = crude_ecs_lookup_entity_from_parent( game->main_node, "ap3" );
        crude_entity to_entity = crude_ecs_lookup_entity_from_parent( game->main_node, "ap4" );
        crude_transform *from_transform = CRUDE_ENTITY_GET_MUTABLE_COMPONENT( from_entity, crude_transform );
        crude_transform *to_transform = CRUDE_ENTITY_GET_MUTABLE_COMPONENT( to_entity, crude_transform );
        float32 t = ( time_from_start - 63 ) / (72 - 63);
        XMStoreFloat3( &camera_transform->translation, XMVectorLerp( XMLoadFloat3( &from_transform->translation ), XMLoadFloat3( &to_transform->translation ), t ) );
        XMStoreFloat4( &camera_transform->rotation, XMQuaternionSlerp( XMLoadFloat4( &from_transform->rotation ), XMLoadFloat4( &to_transform->rotation ), t ) );
      }
      else if ( time_from_start > 72.f && time_from_start < 90.f )
      {
        crude_entity from_entity = crude_ecs_lookup_entity_from_parent( game->main_node, "ap4" );
        crude_entity to_entity = crude_ecs_lookup_entity_from_parent( game->main_node, "ap5" );
        crude_transform *from_transform = CRUDE_ENTITY_GET_MUTABLE_COMPONENT( from_entity, crude_transform );
        crude_transform *to_transform = CRUDE_ENTITY_GET_MUTABLE_COMPONENT( to_entity, crude_transform );
        float32 t = ( time_from_start - 72.f ) / (90 - 72);
        XMStoreFloat3( &camera_transform->translation, XMVectorLerp( XMLoadFloat3( &from_transform->translation ), XMLoadFloat3( &to_transform->translation ), t ) );
        XMStoreFloat4( &camera_transform->rotation, XMQuaternionSlerp( XMLoadFloat4( &from_transform->rotation ), XMLoadFloat4( &to_transform->rotation ), t ) );
      }
      break;
    }
    case CRUDE_LEVEL_CUTSCENE_ONLY_SOUND_TYPE_CUTSCENE1:
    {
      if ( !crude_audio_device_sound_is_playing( &game->audio_device, level->sound_handle ) )
      {
        game_push_load_scene_command( game, game->level_cutscene2_node_absolute_filepath );
      }

      crude_transform *camera_transform = CRUDE_ENTITY_GET_MUTABLE_COMPONENT( game->focused_camera_node, crude_transform );
      float32 time_from_start = crude_time_delta_seconds( level->time, crude_time_now( ) );
      if ( time_from_start < 11.f )
      {
        crude_entity from_entity = crude_ecs_lookup_entity_from_parent( game->main_node, "ap0" );
        crude_entity to_entity = crude_ecs_lookup_entity_from_parent( game->main_node, "ap1" );
        crude_transform *from_transform = CRUDE_ENTITY_GET_MUTABLE_COMPONENT( from_entity, crude_transform );
        crude_transform *to_transform = CRUDE_ENTITY_GET_MUTABLE_COMPONENT( to_entity, crude_transform );
        float32 t = time_from_start / 11;
        XMStoreFloat3( &camera_transform->translation, XMVectorLerp( XMLoadFloat3( &from_transform->translation ), XMLoadFloat3( &to_transform->translation ), t ) );
        XMStoreFloat4( &camera_transform->rotation, XMQuaternionSlerp( XMLoadFloat4( &from_transform->rotation ), XMLoadFloat4( &to_transform->rotation ), t ) );
      }
      else if ( time_from_start > 15 && time_from_start < 25 )
      {
        crude_entity from_entity = crude_ecs_lookup_entity_from_parent( game->main_node, "ap1" );
        crude_entity to_entity = crude_ecs_lookup_entity_from_parent( game->main_node, "ap2" );
        crude_transform *from_transform = CRUDE_ENTITY_GET_MUTABLE_COMPONENT( from_entity, crude_transform );
        crude_transform *to_transform = CRUDE_ENTITY_GET_MUTABLE_COMPONENT( to_entity, crude_transform );
        float32 t = ( time_from_start - 15 ) / (25 - 15);
        XMStoreFloat3( &camera_transform->translation, XMVectorLerp( XMLoadFloat3( &from_transform->translation ), XMLoadFloat3( &to_transform->translation ), t ) );
        XMStoreFloat4( &camera_transform->rotation, XMQuaternionSlerp( XMLoadFloat4( &from_transform->rotation ), XMLoadFloat4( &to_transform->rotation ), t ) );
      }
      else if ( time_from_start > 25 && time_from_start < 30 )
      {
        crude_entity from_entity = crude_ecs_lookup_entity_from_parent( game->main_node, "ap2" );
        crude_entity to_entity = crude_ecs_lookup_entity_from_parent( game->main_node, "ap3" );
        crude_transform *from_transform = CRUDE_ENTITY_GET_MUTABLE_COMPONENT( from_entity, crude_transform );
        crude_transform *to_transform = CRUDE_ENTITY_GET_MUTABLE_COMPONENT( to_entity, crude_transform );
        float32 t = ( time_from_start - 25 ) / (30 - 25);
        XMStoreFloat3( &camera_transform->translation, XMVectorLerp( XMLoadFloat3( &from_transform->translation ), XMLoadFloat3( &to_transform->translation ), t ) );
        XMStoreFloat4( &camera_transform->rotation, XMQuaternionSlerp( XMLoadFloat4( &from_transform->rotation ), XMLoadFloat4( &to_transform->rotation ), t ) );
      }
      else if ( time_from_start > 30 && time_from_start < 40 )
      {
        crude_entity from_entity = crude_ecs_lookup_entity_from_parent( game->main_node, "ap3" );
        crude_entity to_entity = crude_ecs_lookup_entity_from_parent( game->main_node, "ap4" );
        crude_transform *from_transform = CRUDE_ENTITY_GET_MUTABLE_COMPONENT( from_entity, crude_transform );
        crude_transform *to_transform = CRUDE_ENTITY_GET_MUTABLE_COMPONENT( to_entity, crude_transform );
        float32 t = ( time_from_start - 30 ) / (40 - 30);
        XMStoreFloat3( &camera_transform->translation, XMVectorLerp( XMLoadFloat3( &from_transform->translation ), XMLoadFloat3( &to_transform->translation ), t ) );
        XMStoreFloat4( &camera_transform->rotation, XMQuaternionSlerp( XMLoadFloat4( &from_transform->rotation ), XMLoadFloat4( &to_transform->rotation ), t ) );
      }
      else if ( time_from_start > 50 && time_from_start < 60 )
      {
        crude_entity from_entity = crude_ecs_lookup_entity_from_parent( game->main_node, "ap4" );
        crude_entity to_entity = crude_ecs_lookup_entity_from_parent( game->main_node, "ap5" );
        crude_transform *from_transform = CRUDE_ENTITY_GET_MUTABLE_COMPONENT( from_entity, crude_transform );
        crude_transform *to_transform = CRUDE_ENTITY_GET_MUTABLE_COMPONENT( to_entity, crude_transform );
        float32 t = ( time_from_start - 50 ) / (60 - 50);
        XMStoreFloat3( &camera_transform->translation, XMVectorLerp( XMLoadFloat3( &from_transform->translation ), XMLoadFloat3( &to_transform->translation ), t ) );
        XMStoreFloat4( &camera_transform->rotation, XMQuaternionSlerp( XMLoadFloat4( &from_transform->rotation ), XMLoadFloat4( &to_transform->rotation ), t ) );
      }
      else if ( time_from_start > 60 && time_from_start < 80 )
      {
        crude_entity from_entity = crude_ecs_lookup_entity_from_parent( game->main_node, "ap5" );
        crude_entity to_entity = crude_ecs_lookup_entity_from_parent( game->main_node, "ap6" );
        crude_transform *from_transform = CRUDE_ENTITY_GET_MUTABLE_COMPONENT( from_entity, crude_transform );
        crude_transform *to_transform = CRUDE_ENTITY_GET_MUTABLE_COMPONENT( to_entity, crude_transform );
        float32 t = ( time_from_start - 60 ) / (80 - 60);
        XMStoreFloat3( &camera_transform->translation, XMVectorLerp( XMLoadFloat3( &from_transform->translation ), XMLoadFloat3( &to_transform->translation ), t ) );
        XMStoreFloat4( &camera_transform->rotation, XMQuaternionSlerp( XMLoadFloat4( &from_transform->rotation ), XMLoadFloat4( &to_transform->rotation ), t ) );
      }
      else if ( time_from_start > 90 && time_from_start < 110  )
      {
        crude_entity from_entity = crude_ecs_lookup_entity_from_parent( game->main_node, "ap6" );
        crude_entity to_entity = crude_ecs_lookup_entity_from_parent( game->main_node, "ap7" );
        crude_transform *from_transform = CRUDE_ENTITY_GET_MUTABLE_COMPONENT( from_entity, crude_transform );
        crude_transform *to_transform = CRUDE_ENTITY_GET_MUTABLE_COMPONENT( to_entity, crude_transform );
        float32 t = ( time_from_start - 90 ) / (110 - 90);
        XMStoreFloat3( &camera_transform->translation, XMVectorLerp( XMLoadFloat3( &from_transform->translation ), XMLoadFloat3( &to_transform->translation ), t ) );
        XMStoreFloat4( &camera_transform->rotation, XMQuaternionSlerp( XMLoadFloat4( &from_transform->rotation ), XMLoadFloat4( &to_transform->rotation ), t ) );
      }
      else if ( time_from_start > 120 && time_from_start < 127 )
      {
        crude_entity from_entity = crude_ecs_lookup_entity_from_parent( game->main_node, "ap7" );
        crude_entity to_entity = crude_ecs_lookup_entity_from_parent( game->main_node, "ap8" );
        crude_transform *from_transform = CRUDE_ENTITY_GET_MUTABLE_COMPONENT( from_entity, crude_transform );
        crude_transform *to_transform = CRUDE_ENTITY_GET_MUTABLE_COMPONENT( to_entity, crude_transform );
        float32 t = ( time_from_start - 120 ) / (127 - 120);
        XMStoreFloat3( &camera_transform->translation, XMVectorLerp( XMLoadFloat3( &from_transform->translation ), XMLoadFloat3( &to_transform->translation ), t ) );
        XMStoreFloat4( &camera_transform->rotation, XMQuaternionSlerp( XMLoadFloat4( &from_transform->rotation ), XMLoadFloat4( &to_transform->rotation ), t ) );
      }
      break;
    }
    case CRUDE_LEVEL_CUTSCENE_ONLY_SOUND_TYPE_CUTSCENE2:
    {
      crude_entity apollon_node = crude_ecs_lookup_entity_from_parent( game->main_node, "Apollon" );
      crude_transform *apollon_transform = CRUDE_ENTITY_GET_MUTABLE_COMPONENT( apollon_node, crude_transform );

      XMStoreFloat4( &apollon_transform->rotation, XMQuaternionMultiply( XMLoadFloat4( &apollon_transform->rotation ), XMQuaternionRotationAxis( XMVectorSet( 1, 0, 0, 0 ), 0.1 * it->delta_time ) ) );

      if ( !crude_audio_device_sound_is_playing( &game->audio_device, level->sound_handle ) )
      {
        game_push_load_scene_command( game, game->level_cutscene3_node_absolute_filepath );
      }

      crude_transform *camera_transform = CRUDE_ENTITY_GET_MUTABLE_COMPONENT( game->focused_camera_node, crude_transform );
      float32 time_from_start = crude_time_delta_seconds( level->time, crude_time_now( ) );
      if ( time_from_start > 5.f && time_from_start < 33.4f )
      {
        crude_entity from_entity = crude_ecs_lookup_entity_from_parent( game->main_node, "ap0" );
        crude_entity to_entity = crude_ecs_lookup_entity_from_parent( game->main_node, "ap1" );
        crude_transform *from_transform = CRUDE_ENTITY_GET_MUTABLE_COMPONENT( from_entity, crude_transform );
        crude_transform *to_transform = CRUDE_ENTITY_GET_MUTABLE_COMPONENT( to_entity, crude_transform );
        float32 t = ( time_from_start - 5 ) / (33.4 - 5);
        XMStoreFloat3( &camera_transform->translation, XMVectorLerp( XMLoadFloat3( &from_transform->translation ), XMLoadFloat3( &to_transform->translation ), t ) );
        XMStoreFloat4( &camera_transform->rotation, XMQuaternionSlerp( XMLoadFloat4( &from_transform->rotation ), XMLoadFloat4( &to_transform->rotation ), t ) );
      }
      else if ( time_from_start > 50 && time_from_start < 70 )
      {
        crude_entity from_entity = crude_ecs_lookup_entity_from_parent( game->main_node, "ap1" );
        crude_entity to_entity = crude_ecs_lookup_entity_from_parent( game->main_node, "ap2" );
        crude_transform *from_transform = CRUDE_ENTITY_GET_MUTABLE_COMPONENT( from_entity, crude_transform );
        crude_transform *to_transform = CRUDE_ENTITY_GET_MUTABLE_COMPONENT( to_entity, crude_transform );
        float32 t = ( time_from_start - 50 ) / (70 - 50);
        XMStoreFloat3( &camera_transform->translation, XMVectorLerp( XMLoadFloat3( &from_transform->translation ), XMLoadFloat3( &to_transform->translation ), t ) );
        XMStoreFloat4( &camera_transform->rotation, XMQuaternionSlerp( XMLoadFloat4( &from_transform->rotation ), XMLoadFloat4( &to_transform->rotation ), t ) );
      }
      else if ( time_from_start > 80 && time_from_start < 85 )
      {
        crude_entity from_entity = crude_ecs_lookup_entity_from_parent( game->main_node, "ap2" );
        crude_entity to_entity = crude_ecs_lookup_entity_from_parent( game->main_node, "ap3" );
        crude_transform *from_transform = CRUDE_ENTITY_GET_MUTABLE_COMPONENT( from_entity, crude_transform );
        crude_transform *to_transform = CRUDE_ENTITY_GET_MUTABLE_COMPONENT( to_entity, crude_transform );
        float32 t = ( time_from_start - 80 ) / (85 - 80);
        XMStoreFloat3( &camera_transform->translation, XMVectorLerp( XMLoadFloat3( &from_transform->translation ), XMLoadFloat3( &to_transform->translation ), t ) );
        XMStoreFloat4( &camera_transform->rotation, XMQuaternionSlerp( XMLoadFloat4( &from_transform->rotation ), XMLoadFloat4( &to_transform->rotation ), t ) );
      }
      else if ( time_from_start > 92 && time_from_start < 105 )
      {
        crude_entity from_entity = crude_ecs_lookup_entity_from_parent( game->main_node, "ap3" );
        crude_entity to_entity = crude_ecs_lookup_entity_from_parent( game->main_node, "ap4" );
        crude_transform *from_transform = CRUDE_ENTITY_GET_MUTABLE_COMPONENT( from_entity, crude_transform );
        crude_transform *to_transform = CRUDE_ENTITY_GET_MUTABLE_COMPONENT( to_entity, crude_transform );
        float32 t = ( time_from_start - 92 ) / (105 - 92);
        XMStoreFloat3( &camera_transform->translation, XMVectorLerp( XMLoadFloat3( &from_transform->translation ), XMLoadFloat3( &to_transform->translation ), t ) );
        XMStoreFloat4( &camera_transform->rotation, XMQuaternionSlerp( XMLoadFloat4( &from_transform->rotation ), XMLoadFloat4( &to_transform->rotation ), t ) );
      }
      break;
    }
    case CRUDE_LEVEL_CUTSCENE_ONLY_SOUND_TYPE_CUTSCENE3:
    {
      crude_entity apollon_node = crude_ecs_lookup_entity_from_parent( game->main_node, "Apollon" );
      crude_transform *apollon_transform = CRUDE_ENTITY_GET_MUTABLE_COMPONENT( apollon_node, crude_transform );

      XMStoreFloat4( &apollon_transform->rotation, XMQuaternionMultiply( XMLoadFloat4( &apollon_transform->rotation ), XMQuaternionRotationAxis( XMVectorSet( 1, 0, 0, 0 ), 0.1 * it->delta_time ) ) );

      if ( !crude_audio_device_sound_is_playing( &game->audio_device, level->sound_handle ) )
      {
        game_push_load_scene_command( game, game->level_cutscene4_node_absolute_filepath );
      }

      crude_transform *camera_transform = CRUDE_ENTITY_GET_MUTABLE_COMPONENT( game->focused_camera_node, crude_transform );
      float32 time_from_start = crude_time_delta_seconds( level->time, crude_time_now( ) );
      if ( time_from_start > 0.f && time_from_start < 25 )
      {
        crude_entity from_entity = crude_ecs_lookup_entity_from_parent( game->main_node, "ap0" );
        crude_entity to_entity = crude_ecs_lookup_entity_from_parent( game->main_node, "ap1" );
        crude_transform *from_transform = CRUDE_ENTITY_GET_MUTABLE_COMPONENT( from_entity, crude_transform );
        crude_transform *to_transform = CRUDE_ENTITY_GET_MUTABLE_COMPONENT( to_entity, crude_transform );
        float32 t = ( time_from_start - 0 ) / (25 - 0);
        XMStoreFloat3( &camera_transform->translation, XMVectorLerp( XMLoadFloat3( &from_transform->translation ), XMLoadFloat3( &to_transform->translation ), t ) );
        XMStoreFloat4( &camera_transform->rotation, XMQuaternionSlerp( XMLoadFloat4( &from_transform->rotation ), XMLoadFloat4( &to_transform->rotation ), t ) );
      }
      else if ( time_from_start > 25 && time_from_start < 38 )
      {
        crude_entity from_entity = crude_ecs_lookup_entity_from_parent( game->main_node, "ap1" );
        crude_entity to_entity = crude_ecs_lookup_entity_from_parent( game->main_node, "ap2" );
        crude_transform *from_transform = CRUDE_ENTITY_GET_MUTABLE_COMPONENT( from_entity, crude_transform );
        crude_transform *to_transform = CRUDE_ENTITY_GET_MUTABLE_COMPONENT( to_entity, crude_transform );
        float32 t = ( time_from_start - 38 ) / (38 - 25);
        XMStoreFloat3( &camera_transform->translation, XMVectorLerp( XMLoadFloat3( &from_transform->translation ), XMLoadFloat3( &to_transform->translation ), t ) );
        XMStoreFloat4( &camera_transform->rotation, XMQuaternionSlerp( XMLoadFloat4( &from_transform->rotation ), XMLoadFloat4( &to_transform->rotation ), t ) );
      }
      else if ( time_from_start > 50 && time_from_start < 70 )
      {
        crude_entity from_entity = crude_ecs_lookup_entity_from_parent( game->main_node, "ap2" );
        crude_entity to_entity = crude_ecs_lookup_entity_from_parent( game->main_node, "ap3" );
        crude_transform *from_transform = CRUDE_ENTITY_GET_MUTABLE_COMPONENT( from_entity, crude_transform );
        crude_transform *to_transform = CRUDE_ENTITY_GET_MUTABLE_COMPONENT( to_entity, crude_transform );
        float32 t = ( time_from_start - 50 ) / (70 - 50);
        XMStoreFloat3( &camera_transform->translation, XMVectorLerp( XMLoadFloat3( &from_transform->translation ), XMLoadFloat3( &to_transform->translation ), t ) );
        XMStoreFloat4( &camera_transform->rotation, XMQuaternionSlerp( XMLoadFloat4( &from_transform->rotation ), XMLoadFloat4( &to_transform->rotation ), t ) );
      }
      break;
    }
    case CRUDE_LEVEL_CUTSCENE_ONLY_SOUND_TYPE_CUTSCENE4:
    {
      //crude_entity apollon_node = crude_ecs_lookup_entity_from_parent( game->main_node, "Apollon" );
      //crude_transform *apollon_transform = CRUDE_ENTITY_GET_MUTABLE_COMPONENT( apollon_node, crude_transform );
      //
      //XMStoreFloat4( &apollon_transform->rotation, XMQuaternionMultiply( XMLoadFloat4( &apollon_transform->rotation ), XMQuaternionRotationAxis( XMVectorSet( 1, 0, 0, 0 ), 0.1 * it->delta_time ) ) );

      if ( !crude_audio_device_sound_is_playing( &game->audio_device, level->sound_handle ) )
      {
        game_push_load_scene_command( game, game->level_cutscene4_node_absolute_filepath );
      }

      crude_transform *camera_transform = CRUDE_ENTITY_GET_MUTABLE_COMPONENT( game->focused_camera_node, crude_transform );
      float32 time_from_start = crude_time_delta_seconds( level->time, crude_time_now( ) );
      if ( time_from_start > 0.f && time_from_start < 10 )
      {
        crude_entity from_entity = crude_ecs_lookup_entity_from_parent( game->main_node, "ap0" );
        crude_entity to_entity = crude_ecs_lookup_entity_from_parent( game->main_node, "ap1" );
        crude_transform *from_transform = CRUDE_ENTITY_GET_MUTABLE_COMPONENT( from_entity, crude_transform );
        crude_transform *to_transform = CRUDE_ENTITY_GET_MUTABLE_COMPONENT( to_entity, crude_transform );
        float32 t = ( time_from_start - 0 ) / (10 - 0);
        XMStoreFloat3( &camera_transform->translation, XMVectorLerp( XMLoadFloat3( &from_transform->translation ), XMLoadFloat3( &to_transform->translation ), t ) );
        XMStoreFloat4( &camera_transform->rotation, XMQuaternionSlerp( XMLoadFloat4( &from_transform->rotation ), XMLoadFloat4( &to_transform->rotation ), t ) );
      }
      else if ( time_from_start > 10 && time_from_start < 40 )
      {
        crude_entity from_entity = crude_ecs_lookup_entity_from_parent( game->main_node, "ap1" );
        crude_entity to_entity = crude_ecs_lookup_entity_from_parent( game->main_node, "ap2" );
        crude_transform *from_transform = CRUDE_ENTITY_GET_MUTABLE_COMPONENT( from_entity, crude_transform );
        crude_transform *to_transform = CRUDE_ENTITY_GET_MUTABLE_COMPONENT( to_entity, crude_transform );
        float32 t = ( time_from_start - 10 ) / (40 - 10);
        XMStoreFloat3( &camera_transform->translation, XMVectorLerp( XMLoadFloat3( &from_transform->translation ), XMLoadFloat3( &to_transform->translation ), t ) );
        XMStoreFloat4( &camera_transform->rotation, XMQuaternionSlerp( XMLoadFloat4( &from_transform->rotation ), XMLoadFloat4( &to_transform->rotation ), t ) );
      }
      else if ( time_from_start > 40 && time_from_start < 45 )
      {
        crude_entity yellow_balls_node = crude_ecs_lookup_entity_from_parent( game->main_node, "yellow_balls_node" );
        crude_transform *yellow_balls_node_transform = CRUDE_ENTITY_GET_MUTABLE_COMPONENT( yellow_balls_node, crude_transform );
        yellow_balls_node_transform->translation.y = 0;
        crude_entity from_entity = crude_ecs_lookup_entity_from_parent( game->main_node, "ap2" );
        crude_entity to_entity = crude_ecs_lookup_entity_from_parent( game->main_node, "ap3" );
        crude_transform *from_transform = CRUDE_ENTITY_GET_MUTABLE_COMPONENT( from_entity, crude_transform );
        crude_transform *to_transform = CRUDE_ENTITY_GET_MUTABLE_COMPONENT( to_entity, crude_transform );
        float32 t = ( time_from_start - 40 ) / (45 - 40);
        XMStoreFloat3( &camera_transform->translation, XMVectorLerp( XMLoadFloat3( &from_transform->translation ), XMLoadFloat3( &to_transform->translation ), t ) );
        XMStoreFloat4( &camera_transform->rotation, XMQuaternionSlerp( XMLoadFloat4( &from_transform->rotation ), XMLoadFloat4( &to_transform->rotation ), t ) );
      }
      else if ( time_from_start > 45 && time_from_start < 90 )
      {
        crude_entity from_entity = crude_ecs_lookup_entity_from_parent( game->main_node, "ap3" );
        crude_entity to_entity = crude_ecs_lookup_entity_from_parent( game->main_node, "ap4" );
        crude_transform *from_transform = CRUDE_ENTITY_GET_MUTABLE_COMPONENT( from_entity, crude_transform );
        crude_transform *to_transform = CRUDE_ENTITY_GET_MUTABLE_COMPONENT( to_entity, crude_transform );
        float32 t = ( time_from_start - 45 ) / (90 - 45);
        XMStoreFloat3( &camera_transform->translation, XMVectorLerp( XMLoadFloat3( &from_transform->translation ), XMLoadFloat3( &to_transform->translation ), t ) );
        XMStoreFloat4( &camera_transform->rotation, XMQuaternionSlerp( XMLoadFloat4( &from_transform->rotation ), XMLoadFloat4( &to_transform->rotation ), t ) );
      }
      else if ( time_from_start > 90 && time_from_start < 100 )
      {
        crude_entity from_entity = crude_ecs_lookup_entity_from_parent( game->main_node, "ap4" );
        crude_entity to_entity = crude_ecs_lookup_entity_from_parent( game->main_node, "ap5" );
        crude_transform *from_transform = CRUDE_ENTITY_GET_MUTABLE_COMPONENT( from_entity, crude_transform );
        crude_transform *to_transform = CRUDE_ENTITY_GET_MUTABLE_COMPONENT( to_entity, crude_transform );
        float32 t = ( time_from_start - 90 ) / (100 - 90);
        XMStoreFloat3( &camera_transform->translation, XMVectorLerp( XMLoadFloat3( &from_transform->translation ), XMLoadFloat3( &to_transform->translation ), t ) );
        XMStoreFloat4( &camera_transform->rotation, XMQuaternionSlerp( XMLoadFloat4( &from_transform->rotation ), XMLoadFloat4( &to_transform->rotation ), t ) );
      }
      else if ( time_from_start > 100 && time_from_start < 112 )
      {
        crude_entity yellow_balls_node = crude_ecs_lookup_entity_from_parent( game->main_node, "yellow_balls_node" );
        crude_transform *yellow_balls_node_transform = CRUDE_ENTITY_GET_MUTABLE_COMPONENT( yellow_balls_node, crude_transform );
        yellow_balls_node_transform->translation.y = -5;
        crude_entity redball_model = crude_ecs_lookup_entity_from_parent( game->main_node, "redball_model" );
        crude_transform *redball_model_transform = CRUDE_ENTITY_GET_MUTABLE_COMPONENT( redball_model, crude_transform );
        redball_model_transform->translation.y = -5;

        crude_entity from_entity = crude_ecs_lookup_entity_from_parent( game->main_node, "ap5" );
        crude_entity to_entity = crude_ecs_lookup_entity_from_parent( game->main_node, "ap6" );
        crude_transform *from_transform = CRUDE_ENTITY_GET_MUTABLE_COMPONENT( from_entity, crude_transform );
        crude_transform *to_transform = CRUDE_ENTITY_GET_MUTABLE_COMPONENT( to_entity, crude_transform );
        float32 t = ( time_from_start - 100 ) / (112 - 100);
        XMStoreFloat3( &camera_transform->translation, XMVectorLerp( XMLoadFloat3( &from_transform->translation ), XMLoadFloat3( &to_transform->translation ), t ) );
        XMStoreFloat4( &camera_transform->rotation, XMQuaternionSlerp( XMLoadFloat4( &from_transform->rotation ), XMLoadFloat4( &to_transform->rotation ), t ) );
      }
      else if ( time_from_start > 112 && time_from_start < 122 )
      {
        crude_entity from_entity = crude_ecs_lookup_entity_from_parent( game->main_node, "ap6" );
        crude_entity to_entity = crude_ecs_lookup_entity_from_parent( game->main_node, "ap7" );
        crude_transform *from_transform = CRUDE_ENTITY_GET_MUTABLE_COMPONENT( from_entity, crude_transform );
        crude_transform *to_transform = CRUDE_ENTITY_GET_MUTABLE_COMPONENT( to_entity, crude_transform );
        float32 t = ( time_from_start - 112 ) / (122 - 112);
        XMStoreFloat3( &camera_transform->translation, XMVectorLerp( XMLoadFloat3( &from_transform->translation ), XMLoadFloat3( &to_transform->translation ), t ) );
        XMStoreFloat4( &camera_transform->rotation, XMQuaternionSlerp( XMLoadFloat4( &from_transform->rotation ), XMLoadFloat4( &to_transform->rotation ), t ) );

        crude_entity apollo_node = crude_ecs_lookup_entity_from_parent( game->main_node, "Apollon_Landing_Module" );
        crude_transform *apollo_node_transform = CRUDE_ENTITY_GET_MUTABLE_COMPONENT( apollo_node, crude_transform );
        XMStoreFloat3( &apollo_node_transform->translation, XMVectorLerp( XMVectorSet( -6.437, 84.051, 0, 0 ), XMVectorSet( -6.437, 63.751, 0.000, 0 ), t ) );
      }
      else if ( time_from_start > 122 && time_from_start < 180 )
      {
        crude_entity from_entity = crude_ecs_lookup_entity_from_parent( game->main_node, "ap7" );
        crude_entity to_entity = crude_ecs_lookup_entity_from_parent( game->main_node, "ap8" );
        crude_transform *from_transform = CRUDE_ENTITY_GET_MUTABLE_COMPONENT( from_entity, crude_transform );
        crude_transform *to_transform = CRUDE_ENTITY_GET_MUTABLE_COMPONENT( to_entity, crude_transform );
        float32 t = ( time_from_start - 122 ) / (180 - 122);
        XMStoreFloat3( &camera_transform->translation, XMVectorLerp( XMLoadFloat3( &from_transform->translation ), XMLoadFloat3( &to_transform->translation ), t ) );
        XMStoreFloat4( &camera_transform->rotation, XMQuaternionSlerp( XMLoadFloat4( &from_transform->rotation ), XMLoadFloat4( &to_transform->rotation ), t ) );
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