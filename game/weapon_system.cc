#include <engine/core/log.h>
#include <engine/core/profiler.h>
#include <engine/scene/scripts_components.h>
#include <engine/scene/scene_components.h>
#include <engine/platform/platform_components.h>
#include <engine/physics/physics_components.h>
#include <engine/physics/physics.h>
#include <engine/external/game_components.h>
#include <game/game.h>
#include <game/enemy_system.h>

#include <game/weapon_system.h>

CRUDE_ECS_OBSERVER_DECLARE( crude_weapon_creation_observer_ );
CRUDE_ECS_SYSTEM_DECLARE( crude_weapon_update_system_ );

static void
crude_weapon_creation_observer_
(
  _In_ ecs_iter_t *it
)
{
  game_t *game = game_instance( );
  crude_weapon *weapon_per_entity = ecs_field( it, crude_weapon, 0 );

  for ( uint32 i = 0; i < it->count; ++i )
  {
    crude_weapon *weapon = &weapon_per_entity[ i ];
    crude_entity node = CRUDE_COMPOUNT( crude_entity, { it->entities[ i ], it->world } );
    weapon->weapon_basic_node  = crude_ecs_lookup_entity_from_parent( node, "weapon_basic_position" );
    weapon->weapon_scoped_node = crude_ecs_lookup_entity_from_parent( node, "weapon_scoped_position" );
    weapon->weapon_shot_node = crude_ecs_lookup_entity_from_parent( node, "weapon_shot_node" );
    weapon->last_shot_timer = 100.f;
    weapon->ammo = weapon->max_ammo;
  }
}

void
crude_weapon_update_system_
(
  _In_ ecs_iter_t                                         *it
)
{
  CRUDE_PROFILER_ZONE_NAME( "crude_weapon_update_system_" );
  game_t *game = game_instance( );

  crude_input *input = CRUDE_ENTITY_GET_MUTABLE_COMPONENT( game->platform_node, crude_input );
  crude_weapon *weapon_per_entity = ecs_field( it, crude_weapon, 0 );
  crude_transform *transforms_per_entity = ecs_field( it, crude_transform, 1 );
  
  if ( game->death_screen )
  {
    return;
  }

  for ( uint32 i = 0; i < it->count; ++i )
  {
    crude_weapon                                          *weapon;
    crude_transform                                       *transform;
    XMVECTOR                                               breath_offset;
    
    weapon = &weapon_per_entity[ i ];
    transform = &transforms_per_entity[ i ];

    breath_offset = XMVectorSet( 0.005 * sin( 2 * game->time ), 0.003 * cos( 2 * game->time ), 0, 0 );

    if ( input->mouse.right.current )
    {
      crude_transform                                     *weapon_scoped_transform;
      XMVECTOR                                             weapon_target_translation;
      
      weapon_scoped_transform = CRUDE_ENTITY_GET_MUTABLE_COMPONENT( weapon->weapon_scoped_node, crude_transform );
      weapon_target_translation = XMVectorAdd( XMLoadFloat3( &weapon_scoped_transform->translation ), breath_offset );
      XMStoreFloat3( &transform->translation, XMVectorLerp( XMLoadFloat3( &transform->translation ), weapon_target_translation, CRUDE_GAME_WEAPON_MODEL_MOVE_SPEED * it->delta_time ) ); 
      XMStoreFloat4( &transform->rotation, XMVectorLerp( XMLoadFloat4( &transform->rotation ), XMLoadFloat4( &weapon_scoped_transform->rotation ), CRUDE_GAME_WEAPON_MODEL_MOVE_SPEED * it->delta_time ) ); 
    }
    else
    {
      crude_transform *weapon_basic_transform = CRUDE_ENTITY_GET_MUTABLE_COMPONENT( weapon->weapon_basic_node, crude_transform );
      XMStoreFloat3( &transform->translation, XMVectorLerp( XMLoadFloat3( &transform->translation ), XMLoadFloat3( &weapon_basic_transform->translation ), CRUDE_GAME_WEAPON_MODEL_MOVE_SPEED * it->delta_time ) );
      XMStoreFloat4( &transform->rotation, XMVectorLerp( XMLoadFloat4( &transform->rotation ), XMLoadFloat4( &weapon_basic_transform->rotation ), 2 * it->delta_time ) ); 
    }

    crude_player_controller const *player_controller = CRUDE_ENTITY_GET_IMMUTABLE_COMPONENT( game->player_node, crude_player_controller );
    if ( player_controller->input_enabled && input->mouse.left.current && ( weapon->last_shot_timer > CRUDE_GAME_WEAPON_SHOT_INTEVAL ) )
    {
      crude_level_01 *level = CRUDE_ENTITY_GET_MUTABLE_COMPONENT( game->main_node, crude_level_01 );
      if ( weapon->ammo > 0 )
      {
        crude_transform const                             *weapon_shot_node_transform;
        XMMATRIX                                           weapon_shot_to_world;
        crude_physics_raycast_result                       raycast_result;
        XMVECTOR                                           shot_point_translation;
        XMVECTOR                                           ray_origin;
        XMVECTOR                                           ray_direction;

        weapon_shot_node_transform = CRUDE_ENTITY_GET_IMMUTABLE_COMPONENT( weapon->weapon_shot_node, crude_transform );
        weapon_shot_to_world = crude_transform_node_to_world( weapon->weapon_shot_node, weapon_shot_node_transform ); 

        ray_origin = weapon_shot_to_world.r[ 3 ];
        ray_direction = XMVector3TransformNormal( XMVectorSet( 0, 0, 1, 0 ), weapon_shot_to_world );
        
        if ( crude_physics_cast_ray( &game_instance( )->physics, ray_origin, ray_direction, 1 | ( 1 << 5 ), &raycast_result ) && raycast_result.body_layer & ( 1 << 5 ) )
        {
          crude_player *player = CRUDE_ENTITY_GET_MUTABLE_COMPONENT( game->player_node, crude_player );
          crude_enemy *enemy = CRUDE_ENTITY_GET_MUTABLE_COMPONENT( crude_entity_get_parent( crude_entity_get_parent( raycast_result.node ) ), crude_enemy );
          crude_enemy_receive_damage( enemy, CRUDE_GAME_WEAPON_CRITICAL_DAMAGE, true );
          crude_audio_device_sound_set_translation( &game->audio_device, level->hit_critical_sound_handle, raycast_result.raycast_result.point );
          crude_audio_device_sound_start( &game->audio_device, level->hit_critical_sound_handle );
          player->sanity = CRUDE_MIN( player->sanity + 0.15, 1.f );
          CRUDE_LOG_INFO( CRUDE_CHANNEL_ALL, "Critical hit" );
        }
        else if ( crude_physics_cast_ray( &game_instance( )->physics, ray_origin, ray_direction, 1 | ( 1 << 6 ), &raycast_result ) && raycast_result.body_layer & ( 1 << 6 ) )
        {
          crude_player *player = CRUDE_ENTITY_GET_MUTABLE_COMPONENT( game->player_node, crude_player );
          player->sanity = CRUDE_MIN( player->sanity + 0.05, 1.f );
          crude_enemy *enemy = CRUDE_ENTITY_GET_MUTABLE_COMPONENT( crude_entity_get_parent( crude_entity_get_parent( raycast_result.node ) ), crude_enemy );
          crude_enemy_receive_damage( enemy, CRUDE_GAME_WEAPON_DAMAGE, false );
          crude_audio_device_sound_set_translation( &game->audio_device, level->hit_basic_sound_handle, raycast_result.raycast_result.point );
          crude_audio_device_sound_start( &game->audio_device, level->hit_basic_sound_handle );
          CRUDE_LOG_INFO( CRUDE_CHANNEL_ALL, "Default Hit" );
        }

        weapon->last_shot_timer = 0.f;
        XMStoreFloat4( &transform->rotation, XMQuaternionMultiply( XMLoadFloat4( &transform->rotation ), XMQuaternionRotationAxis( XMVectorSet( 1, 0, 0, 0 ), -XM_PIDIV2 ) ) );
        --weapon->ammo;

        crude_audio_device_sound_start( &game->audio_device, level->shot_sound_handle );
      }
      else
      {
        crude_audio_device_sound_start( &game->audio_device, level->shot_without_ammo_sound_handle );
      }
    }

    weapon->last_shot_timer += it->delta_time;
  }
  CRUDE_PROFILER_ZONE_END;
}

CRUDE_ECS_MODULE_IMPORT_IMPL( crude_weapon_system )
{
  ECS_MODULE( world, crude_weapon_system );
  
  ECS_IMPORT( world, crude_scene_components );
  ECS_IMPORT( world, crude_physics_components );
  ECS_IMPORT( world, crude_game_components );
  
  CRUDE_ECS_SYSTEM_DEFINE( world, crude_weapon_update_system_, EcsOnUpdate, NULL, {
    { .id = ecs_id( crude_weapon ) },
    { .id = ecs_id( crude_transform ) },
  } );
  
  CRUDE_ECS_OBSERVER_DEFINE( world, crude_weapon_creation_observer_, EcsOnSet, NULL, { 
    { .id = ecs_id( crude_weapon ) }
  } );
}