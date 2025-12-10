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

#include <game/enemy_system.h>

CRUDE_ECS_SYSTEM_DECLARE( crude_enemy_update_system_ );
CRUDE_ECS_OBSERVER_DECLARE( crude_enemy_creation_observer_ );
CRUDE_ECS_OBSERVER_DECLARE( crude_enemy_destroy_observer_ );

void
crude_enemy_deal_damage_to_player
(
  _In_ crude_enemy                                        *enemy,
  _In_ crude_player                                       *player,
  _In_ XMVECTOR                                            enemy_translation
)
{
  game_t *game = game_instance( );

  if ( enemy->state != CRUDE_ENEMY_STATE_STANNED )
  {
    enemy->state = CRUDE_ENEMY_STATE_STANNED;
    enemy->stanned_time_left = CRUDE_GAME_ENEMY_HIT_DEALE_STANNE_TIME;
  
    player->health -= CRUDE_GAME_PLAYER_HEALTH_DAMAGE_FROM_ENEMY;
    player->sanity -= CRUDE_GAME_PLAYER_SANITY_DAMAGE_FROM_ENEMY;

    crude_audio_device_sound_set_translation( &game->audio_device, enemy->enemy_attack_sound_handle, enemy_translation );
    crude_audio_device_sound_start( &game->audio_device, enemy->enemy_attack_sound_handle );

  }
}

void
crude_enemy_receive_damage
(
  _In_ crude_enemy                                        *enemy,
  _In_ float64                                             damage,
  _In_ bool                                                critical
)
{
  enemy->state = CRUDE_ENEMY_STATE_STANNED;
  enemy->stanned_time_left = critical ? CRUDE_GAME_ENEMY_CRITICAL_HIT_RECEIVE_STANNE_TIME : CRUDE_GAME_ENEMY_HIT_RECEIVE_STANNE_TIME;
  enemy->health -= damage;
}

static bool
crude_enemy_search_player
(
  _In_ crude_enemy                                        *enemy
)
{
  game_t                                                  *game;
  crude_transform const                                   *player_transform;
  crude_transform const                                   *player_look_ray_origin_transform;
  XMMATRIX                                                 player_look_ray_origin_to_world;
  XMVECTOR                                                 player_look_ray_origin_world_translation;
  XMVECTOR                                                 player_look_ray_origin_to_player, player_look_ray_direction;
  XMVECTOR                                                 player_translation;
  crude_physics_raycast_result                             raycast_result;
  
  game = game_instance( );

  player_look_ray_origin_transform = CRUDE_ENTITY_GET_IMMUTABLE_COMPONENT( enemy->player_look_ray_origin_node, crude_transform );
  player_look_ray_origin_to_world = crude_transform_node_to_world( enemy->player_look_ray_origin_node, player_look_ray_origin_transform );
  player_look_ray_origin_world_translation = player_look_ray_origin_to_world.r[ 3 ];
  
  player_transform = CRUDE_ENTITY_GET_IMMUTABLE_COMPONENT( game->player_node, crude_transform );
  player_translation = XMLoadFloat3( &player_transform->translation );
  player_look_ray_origin_to_player = XMVectorSubtract( player_translation, player_look_ray_origin_world_translation );
  player_look_ray_direction = XMVector3Normalize( player_look_ray_origin_to_player );
  
  bool player_visible_in_fog = XMVectorGetX( XMVector3Length( XMVectorSubtract( player_look_ray_origin_world_translation, player_translation ) ) ) < CRUDE_GAME_PLAYER_MAX_FOG_DISTANCE;
  if ( player_visible_in_fog )
  {
    bool enemy_see_body = crude_physics_cast_ray( &game_instance( )->physics, player_look_ray_origin_world_translation, player_look_ray_direction, 1, &raycast_result );
    if ( enemy_see_body )
    {
      bool enemy_can_see_player = XMVectorGetX( XMVector3LengthSq( player_look_ray_origin_to_player ) ) < XMVectorGetX( XMVector3LengthSq( XMVectorSubtract( player_look_ray_origin_world_translation, raycast_result.raycast_result.point ) ) );
      if ( enemy_can_see_player )
      {
        return true;
      }
    }
  }
  return false;
}

static void
crude_enemy_go_to_point
(
  _In_ crude_enemy                                        *enemy,
  _In_ crude_transform                                    *enemy_transform,
  _In_ XMVECTOR                                            point,
  _In_ float32                                             delta_time
)
{
  XMVECTOR                                             enemy_translation, enemy_new_translation;
  XMVECTOR                                             enemy_to_point, enemy_to_point_normalized, enemy_to_point_translation_2d;
  float32                                              enemy_to_point_distance, angle;
  
  enemy_translation = XMLoadFloat3( &enemy_transform->translation );
  enemy_to_point = XMVectorSubtract( point, enemy_translation );
  enemy_to_point_distance = XMVectorGetX( XMVector3Length( enemy_to_point ) );
  enemy_to_point_normalized = XMVector3Normalize( enemy_to_point );
  
  enemy_new_translation = XMVectorAdd( enemy_translation, XMVectorScale( enemy_to_point_normalized, enemy->moving_speed * delta_time ) );
  enemy_new_translation = XMVectorSetY( enemy_new_translation, XMVectorGetY( enemy_translation ) );

  enemy_to_point_translation_2d = XMVectorSet( XMVectorGetX( enemy_to_point ), XMVectorGetZ( enemy_to_point ), 0.f, 0.f );
  angle = atan2f( XMVectorGetX( enemy_to_point_translation_2d ), XMVectorGetY( enemy_to_point_translation_2d ) );
  enemy->target_looking_angle = crude_lerp_angle( enemy->target_looking_angle, angle, CRUDE_GAME_ENEMY_ROTATION_FOLLOWING_SPEED * delta_time );

  XMStoreFloat3( &enemy_transform->translation, enemy_new_translation );
  XMStoreFloat4( &enemy_transform->rotation, XMQuaternionRotationAxis( g_XMIdentityR1, enemy->target_looking_angle ) );
}

static void
crude_enemy_creation_observer_
(
  ecs_iter_t *it
)
{
  game_t *game = game_instance( );
  crude_enemy *enemies_per_entity = ecs_field( it, crude_enemy, 0 );

  for ( uint32 i = 0; i < it->count; ++i )
  {
    crude_enemy                                           *enemy;
    crude_transform const                                 *enemy_transform;
    crude_entity                                           enemy_node;
    crude_sound_creation                                   sound_creation;  

    enemy = &enemies_per_entity[ i ];
    enemy_node = CRUDE_COMPOUNT( crude_entity, { it->entities[ i ], it->world } );

    enemy->state = CRUDE_ENEMY_STATE_RETURN_TO_SPAWN;
    enemy->player_look_ray_origin_node = crude_ecs_lookup_entity_from_parent( enemy_node, "player_look_ray_origin" );
    enemy->player_last_visible_time = 1000000.f;
    enemy->target_looking_angle = 0.f;
    enemy->stanned_time_left = 0.f;
    enemy->health = 1.f;
    
    enemy_transform = CRUDE_ENTITY_GET_IMMUTABLE_COMPONENT( enemy_node, crude_transform );

    sound_creation = crude_sound_creation_empty( );
    sound_creation.stream = true;
    sound_creation.looping = true;
    sound_creation.absolute_filepath = game->enemy_idle_sound_absolute_filepath;
    sound_creation.rolloff = 0.25;
    sound_creation.min_distance = 2.0;
    sound_creation.max_distance = CRUDE_GAME_PLAYER_MAX_FOG_DISTANCE;
    enemy->enemy_idle_sound_handle = crude_audio_device_create_sound( &game->audio_device, &sound_creation );

    sound_creation = crude_sound_creation_empty( );
    sound_creation.async_loading = true;
    sound_creation.absolute_filepath = game->enemy_notice_sound_absolute_filepath;
    sound_creation.rolloff = 0.5;
    sound_creation.min_distance = 2.0;
    enemy->enemy_notice_sound_handle = crude_audio_device_create_sound( &game->audio_device, &sound_creation );
    crude_audio_device_sound_set_volume( &game->audio_device, enemy->enemy_notice_sound_handle, 2.0 );
    
    sound_creation = crude_sound_creation_empty( );
    sound_creation.async_loading = true;
    sound_creation.absolute_filepath = game->enemy_attack_sound_absolute_filepath;
    sound_creation.rolloff = 0.25;
    sound_creation.min_distance = 2.0;
    sound_creation.max_distance = CRUDE_GAME_PLAYER_MAX_FOG_DISTANCE;
    enemy->enemy_attack_sound_handle = crude_audio_device_create_sound( &game->audio_device, &sound_creation );
  }
}

static void
crude_enemy_destroy_observer_
(
  ecs_iter_t *it
)
{
  game_t *game = game_instance( );
  crude_enemy *enemies_per_entity = ecs_field( it, crude_enemy, 0 );

  for ( uint32 i = 0; i < it->count; ++i )
  {
    crude_enemy                                           *enemy;
    crude_transform const                                 *enemy_transform;
    crude_entity                                           enemy_node;
    crude_sound_creation                                   sound_creation;  

    enemy = &enemies_per_entity[ i ];
    enemy_node = CRUDE_COMPOUNT( crude_entity, { it->entities[ i ], it->world } );

    crude_audio_device_destroy_sound( &game->audio_device, enemy->enemy_idle_sound_handle );
    crude_audio_device_destroy_sound( &game->audio_device, enemy->enemy_notice_sound_handle );
    crude_audio_device_destroy_sound( &game->audio_device, enemy->enemy_attack_sound_handle );
  }
}

static void
crude_enemy_update_system_
(
  _In_ ecs_iter_t *it
)
{
  CRUDE_PROFILER_ZONE_NAME( "crude_enemy_update_system_" );
  game_t *game = game_instance( );
  crude_enemy *enemies_per_entity = ecs_field( it, crude_enemy, 0 );
  crude_transform *transform_per_entity = ecs_field( it, crude_transform, 1 );
    
    
  if ( game->death_screen )
  {
    return;
  }

  for ( uint32 i = 0; i < it->count; ++i )
  {
    crude_enemy                                           *enemy;
    crude_transform                                       *enemy_transform;
    crude_player const                                    *player;
    crude_transform const                                 *player_transform;
    XMVECTOR                                               enemy_new_translation;
    crude_entity                                           enemy_node;
    float32                                                distance_to_player;

    enemy = &enemies_per_entity[ i ];
    enemy_transform = &transform_per_entity[ i ];
    enemy_node = CRUDE_COMPOUNT( crude_entity, { it->entities[ i ], it->world } );

    CRUDE_ASSERT( crude_entity_valid( game->player_node ) );
    
    player = CRUDE_ENTITY_GET_IMMUTABLE_COMPONENT( game->player_node, crude_player );
    player_transform = CRUDE_ENTITY_GET_IMMUTABLE_COMPONENT( game->player_node, crude_transform );
    distance_to_player = XMVectorGetX( XMVector2Length( XMVectorSubtract( XMVectorSet( enemy_transform->translation.x, enemy_transform->translation.z, 0, 0 ), XMVectorSet( player_transform->translation.x, player_transform->translation.z, 0, 0 ) ) ) );

    if ( ( game->death_screen || player->inside_safe_zone || distance_to_player > CRUDE_GAME_PLAYER_MAX_FOG_DISTANCE ) && crude_audio_device_sound_is_playing( &game->audio_device, enemy->enemy_idle_sound_handle ) )
    {
      crude_audio_device_sound_stop( &game->audio_device, enemy->enemy_idle_sound_handle );
    }
    
    if ( ( !game->death_screen && !player->inside_safe_zone && distance_to_player < CRUDE_GAME_PLAYER_MAX_FOG_DISTANCE ) && !crude_audio_device_sound_is_playing( &game->audio_device, enemy->enemy_idle_sound_handle ) )
    {
      crude_audio_device_sound_start( &game->audio_device, enemy->enemy_idle_sound_handle );
    }

    if ( crude_audio_device_sound_is_playing( &game->audio_device, enemy->enemy_idle_sound_handle ) && distance_to_player < CRUDE_GAME_PLAYER_MAX_FOG_DISTANCE )
    {
      crude_audio_device_sound_set_translation( &game->audio_device, enemy->enemy_idle_sound_handle, XMLoadFloat3( &enemy_transform->translation ) );
    }
    
    switch ( enemy->state )
    {
    case CRUDE_ENEMY_STATE_IDLE:
    {
      if ( crude_enemy_search_player( enemy ) )
      {
        enemy->state = CRUDE_ENEMY_STATE_FOLLOW_PLAYER;
        crude_audio_device_sound_set_translation( &game->audio_device, enemy->enemy_notice_sound_handle, XMLoadFloat3( &enemy_transform->translation ) );
        crude_audio_device_sound_start( &game->audio_device, enemy->enemy_notice_sound_handle );
      }
      else
      {
        enemy->target_looking_angle += 0.6 * it->delta_time;
        XMStoreFloat4( &enemy_transform->rotation, XMQuaternionRotationAxis( g_XMIdentityR1, enemy->target_looking_angle ) );
      }
      break;
    }
    case CRUDE_ENEMY_STATE_FOLLOW_PLAYER:
    {
      if ( crude_enemy_search_player( enemy ) )
      {
        crude_transform const *player_transform = CRUDE_ENTITY_GET_IMMUTABLE_COMPONENT( game->player_node, crude_transform );

        if ( enemy->player_last_visible_translation_updated_time > 0.75f )
        {
          crude_audio_device_sound_set_translation( &game->audio_device, enemy->enemy_notice_sound_handle, XMLoadFloat3( &enemy_transform->translation ) );
          crude_audio_device_sound_start( &game->audio_device, enemy->enemy_notice_sound_handle );
        }
        enemy->player_last_visible_time = 0.f;
        enemy->player_last_visible_translation_updated_time = 0.f;
        enemy->player_last_visible_translation = player_transform->translation;
      }

      enemy->player_last_visible_translation_updated_time += it->delta_time;

      bool enemy_reach_player_last_visible_translation = ( XMVectorGetX( XMVector2Length( XMVectorSubtract( XMVectorSet( enemy_transform->translation.x, enemy_transform->translation.z, 0, 0 ), XMVectorSet( enemy->player_last_visible_translation.x, enemy->player_last_visible_translation.z, 0, 0 ) ) ) ) < 0.01 );
      if ( enemy_reach_player_last_visible_translation )
      {
        enemy->player_last_visible_time += it->delta_time;
        enemy->target_looking_angle += 0.6 * it->delta_time;
        XMStoreFloat4( &enemy_transform->rotation, XMQuaternionRotationAxis( g_XMIdentityR1, enemy->target_looking_angle ) );      }
      else
      {
        crude_enemy_go_to_point( enemy, enemy_transform, XMLoadFloat3( &enemy->player_last_visible_translation ), it->delta_time );
      }

      if ( enemy->player_last_visible_time > CRUDE_GAME_ENEMY_RESET_ENEMY_POSITION_TIMER )
      {
        enemy->state = CRUDE_ENEMY_STATE_RETURN_TO_SPAWN;
      }
      break;
    }
    case CRUDE_ENEMY_STATE_RETURN_TO_SPAWN:
    {
      if ( crude_enemy_search_player( enemy ) )
      {
        enemy->state = CRUDE_ENEMY_STATE_FOLLOW_PLAYER;
        crude_audio_device_sound_set_translation( &game->audio_device, enemy->enemy_notice_sound_handle, XMLoadFloat3( &enemy_transform->translation ) );
        crude_audio_device_sound_start( &game->audio_device, enemy->enemy_notice_sound_handle );
      }
      else
      {
        crude_enemy_go_to_point( enemy, enemy_transform, XMLoadFloat3( &enemy->spawn_node_translation ), it->delta_time );
        
        bool enemy_reach_spawn = ( XMVectorGetX( XMVector2Length( XMVectorSubtract( XMVectorSet( enemy_transform->translation.x, enemy_transform->translation.z, 0, 0 ), XMVectorSet( enemy->spawn_node_translation.x, enemy->spawn_node_translation.z, 0, 0 ) ) ) ) < 0.01 );
        if ( enemy_reach_spawn )
        {
          enemy->state = CRUDE_ENEMY_STATE_IDLE;
        }
      }
      break;
    }
    case CRUDE_ENEMY_STATE_STANNED:
    {
      enemy->stanned_time_left -= it->delta_time;
      if ( enemy->stanned_time_left < 0.f )
      {
        enemy->state = CRUDE_ENEMY_STATE_FOLLOW_PLAYER;
        crude_audio_device_sound_set_translation( &game->audio_device, enemy->enemy_notice_sound_handle, XMLoadFloat3( &enemy_transform->translation ) );
        crude_audio_device_sound_start( &game->audio_device, enemy->enemy_notice_sound_handle );
      }
      break;
    }
    }
  }
  CRUDE_PROFILER_END( "crude_enemy_update_system_" );
}

CRUDE_ECS_MODULE_IMPORT_IMPL( crude_enemy_system )
{
  ECS_MODULE( world, crude_enemy_system );
  
  ECS_IMPORT( world, crude_scene_components );
  ECS_IMPORT( world, crude_physics_components );
  ECS_IMPORT( world, crude_game_components );
  
  CRUDE_ECS_OBSERVER_DEFINE( world, crude_enemy_creation_observer_, EcsOnSet, NULL, { 
    { .id = ecs_id( crude_enemy ) },
    { .id = ecs_id( crude_node_runtime ) }
  } );
  
  CRUDE_ECS_OBSERVER_DEFINE( world, crude_enemy_destroy_observer_, EcsOnRemove, NULL, { 
    { .id = ecs_id( crude_enemy ) },
    { .id = ecs_id( crude_node_runtime ) }
  } );

  CRUDE_ECS_SYSTEM_DEFINE( world, crude_enemy_update_system_, EcsOnUpdate, NULL, {
    { .id = ecs_id( crude_enemy ) },
    { .id = ecs_id( crude_transform ) }
  } );
}