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

/*****
 * 
 * Constant
 * 
 *******/
#define CRUDE_GAME_ENEMY_RESET_ENEMY_POSITION_TIMER 10
#define CRUDE_GAME_ENEMY_RESET_ENEMY_MIN_DISTANCE_POSITION_TIMER_ACTIVATION 10
#define CRUDE_GAME_ENEMY_ROTATION_SPEED 10

CRUDE_ECS_SYSTEM_DECLARE( crude_enemy_update_system_ );
CRUDE_ECS_OBSERVER_DECLARE( crude_enemy_creation_observer_ );

static void
crude_enemy_creation_observer_
(
  ecs_iter_t *it
)
{
  crude_enemy *enemies_per_entity = ecs_field( it, crude_enemy, 0 );

  for ( uint32 i = 0; i < it->count; ++i )
  {
    crude_enemy                                           *enemy;

    enemy = &enemies_per_entity[ i ];

    enemy->time_near_last_player_visible_translaion = 1000000.f;
    enemy->looking_angle = 0.f;
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

  for ( uint32 i = 0; i < it->count; ++i )
  {
    crude_enemy                                           *enemy;
    crude_transform                                       *enemy_transform;
    crude_transform const                                 *player_transform;
    XMVECTOR                                               player_translation;
    XMVECTOR                                               enemy_translation, enemy_new_translation;
    XMVECTOR                                               enemy_to_player, enemy_to_player_normalized;
    XMVECTOR                                               enemy_to_last_player_visible_translation, enemy_to_last_player_visible_translation_normalized;
    crude_entity                                           enemy_node;
    crude_physics_raycast_result                           raycast_result;
    float32                                                enemy_to_last_player_visible_translation_distance;

    enemy = &enemies_per_entity[ i ];
    enemy_node = CRUDE_COMPOUNT( crude_entity, { it->entities[ i ], it->world } );

    CRUDE_ASSERT( crude_entity_valid( game->player_node ) );
    
    if ( enemy->time_near_last_player_visible_translaion > CRUDE_GAME_ENEMY_RESET_ENEMY_POSITION_TIMER ) /* reset when enemy don't see player for a while */
    {
      enemy->time_near_last_player_visible_translaion = 0.f;
      enemy->last_player_visible_translation = enemy->spawn_node_translation;
      enemy_new_translation = XMLoadFloat3( &enemy->spawn_node_translation );
    }

    enemy_transform = CRUDE_ENTITY_GET_MUTABLE_COMPONENT( enemy_node, crude_transform );
    player_transform = CRUDE_ENTITY_GET_IMMUTABLE_COMPONENT( game->player_node, crude_transform );
    
    player_translation = XMLoadFloat3( &player_transform->translation );
    enemy_translation = XMLoadFloat3( &enemy_transform->translation );
    enemy_to_player = XMVectorSubtract( player_translation, enemy_translation );
    enemy_to_player_normalized = XMVector3Normalize( enemy_to_player );
    
    if ( crude_physics_cast_ray( &game_instance( )->physics, enemy_translation, enemy_to_player_normalized, 1, &raycast_result ) )
    {
      if ( XMVectorGetX( XMVector3LengthSq( enemy_to_player ) ) < XMVectorGetX( XMVector3LengthSq( XMVectorSubtract( enemy_translation, raycast_result.raycast_result.point ) ) ) )
      {
        enemy->last_player_visible_translation = player_transform->translation;
      }
    }
    enemy_to_last_player_visible_translation = XMVectorSubtract( XMLoadFloat3( &enemy->last_player_visible_translation ), enemy_translation );
    enemy_to_last_player_visible_translation_distance = XMVectorGetX( XMVector3Length( enemy_to_last_player_visible_translation ) );
    enemy_to_last_player_visible_translation_normalized = XMVector3Normalize( enemy_to_last_player_visible_translation );
    if ( enemy_to_last_player_visible_translation_distance < CRUDE_GAME_ENEMY_RESET_ENEMY_MIN_DISTANCE_POSITION_TIMER_ACTIVATION )
    {
      enemy->time_near_last_player_visible_translaion += it->delta_time;
    }
    else
    {
      enemy->time_near_last_player_visible_translaion = 0.f;
    }

    enemy_new_translation = XMVectorAdd( enemy_translation, XMVectorScale( enemy_to_last_player_visible_translation_normalized, enemy->moving_speed * it->delta_time ) );
    enemy_new_translation = XMVectorSetY( enemy_new_translation, XMVectorGetY( enemy_translation ) );
    XMStoreFloat3( &enemy_transform->translation, enemy_new_translation );
    if ( enemy_to_last_player_visible_translation_distance > 0.001f )
    {
      XMVECTOR                                             enemy_to_last_player_visible_translation_2d;
      float32                                              angle;
      
      enemy_to_last_player_visible_translation_2d = XMVectorSet( XMVectorGetX( enemy_to_last_player_visible_translation ), XMVectorGetZ( enemy_to_last_player_visible_translation ), 0.f, 0.f );
      angle = atan2f( XMVectorGetX( enemy_to_last_player_visible_translation_2d ), XMVectorGetY( enemy_to_last_player_visible_translation_2d ) );
      enemy->looking_angle = crude_lerp_angle( enemy->looking_angle, angle, CRUDE_GAME_ENEMY_ROTATION_SPEED * it->delta_time );
      XMStoreFloat4( &enemy_transform->rotation, XMQuaternionRotationAxis( g_XMIdentityR1, enemy->looking_angle ) );
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
    { .id = ecs_id( crude_enemy ) }
  } );

  CRUDE_ECS_SYSTEM_DEFINE( world, crude_enemy_update_system_, EcsOnUpdate, NULL, {
    { .id = ecs_id( crude_enemy ) }
  } );
}