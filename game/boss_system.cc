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

#include <game/boss_system.h>

CRUDE_ECS_SYSTEM_DECLARE( crude_boss_bullet_update_system_ );
CRUDE_ECS_SYSTEM_DECLARE( crude_boss_update_system_ );
CRUDE_ECS_OBSERVER_DECLARE( crude_boss_creation_observer_ );
CRUDE_ECS_OBSERVER_DECLARE( crude_boss_destroy_observer_ );

void
crude_boss_receive_damage
(
  _In_ crude_boss                                         *boss,
  _In_ crude_entity                                        node
)
{
  game_t *game = game_instance( );
  crude_level_boss_fight *boss_fight = CRUDE_ENTITY_GET_MUTABLE_COMPONENT( game->main_node, crude_level_boss_fight );

  if ( boss_fight->type == 1 )
  {
    boss->health -= 0.025;
  }
  else
  {
    crude_transform *transform = CRUDE_ENTITY_GET_MUTABLE_COMPONENT( node, crude_transform );
    XMVECTOR translation = crude_transform_node_to_world( node, transform ).r[ 3 ];
    if ( crude_string_cmp( crude_entity_get_name( node ), "eye_collision0" ) == 0 )
    {
      boss->health_eye_0 -= 0.15;
      if ( boss->health_eye_0 < 0 )
      {
        crude_audio_device_sound_set_translation( &game->audio_device, boss_fight->destroy_critical_sound_handle, translation );
        crude_audio_device_sound_start( &game->audio_device, boss_fight->destroy_critical_sound_handle );
        crude_entity_destroy_hierarchy( node );
      }
    }
    else if ( crude_string_cmp( crude_entity_get_name( node ), "eye_collision1" ) == 0 )
    {
      boss->health_eye_1 -= 0.15;
      if ( boss->health_eye_1 < 0 )
      {
        crude_audio_device_sound_set_translation( &game->audio_device, boss_fight->destroy_critical_sound_handle, translation );
        crude_audio_device_sound_start( &game->audio_device, boss_fight->destroy_critical_sound_handle );
        crude_entity_destroy_hierarchy( node );
      }
    }
    else if ( crude_string_cmp( crude_entity_get_name( node ), "eye_collision2" ) == 0 )
    {
      boss->health_eye_2 -= 0.15;
      if ( boss->health_eye_2 < 0 )
      {
        crude_audio_device_sound_set_translation( &game->audio_device, boss_fight->destroy_critical_sound_handle, translation );
        crude_audio_device_sound_start( &game->audio_device, boss_fight->destroy_critical_sound_handle );
        crude_entity_destroy_hierarchy( node );
      }
    }
  }

  if ( boss->health_eye_0 < 0 && boss->health_eye_1 < 0 && boss->health_eye_2 < 0 && boss_fight->type == 0 )
  {
    game_push_load_scene_command( game, game->level_cutscene3_node_absolute_filepath );
  }
  if ( boss->health < 0 && boss_fight->type == 1 )
  {
    game_push_load_scene_command( game, game->level_cutscene4_node_absolute_filepath );
  }
}

static void
crude_boss_creation_observer_
(
  ecs_iter_t *it
)
{
  game_t *game = game_instance( );
  crude_boss *boss_per_entity = ecs_field( it, crude_boss, 0 );

  for ( uint32 i = 0; i < it->count; ++i )
  {
    crude_boss                                            *boss;
    crude_entity                                           boss_node;

    boss = &boss_per_entity[ i ];
    boss_node = CRUDE_COMPOUNT( crude_entity, { it->entities[ i ], it->world } );
    
    boss->health = 1.f;
    boss->health_eye_0 = 1.f;
    boss->health_eye_1 = 1.f;
    boss->health_eye_2 = 1.f;
    boss->last_shot_time =222.f;
  }
}

static void
crude_boss_destroy_observer_
(
  ecs_iter_t *it
)
{
  game_t *game = game_instance( );
  crude_boss *boss_per_entity = ecs_field( it, crude_boss, 0 );

  for ( uint32 i = 0; i < it->count; ++i )
  {
    crude_boss                                            *boss;
    crude_entity                                           boss_node;

    boss = &boss_per_entity[ i ];
    boss_node = CRUDE_COMPOUNT( crude_entity, { it->entities[ i ], it->world } );
  }
}

static void
crude_boss_bullet_update_system_
(
  _In_ ecs_iter_t *it
)
{
  CRUDE_PROFILER_ZONE_NAME( "crude_boss_bullet_update_system_" );
  game_t *game = game_instance( );
  crude_boss_bullet *boss_bullet_per_entity = ecs_field( it, crude_boss_bullet, 0 );
  crude_transform *transform_per_entity = ecs_field( it, crude_transform, 1 );

  if ( game->death_screen )
  {
    return;
  }
  
  crude_level_boss_fight *boss_fight = CRUDE_ENTITY_GET_MUTABLE_COMPONENT( game->main_node, crude_level_boss_fight );
  for ( uint32 i = 0; i < it->count; ++i )
  {
    crude_boss_bullet                                     *boss_bullet;
    crude_transform                                       *boss_bullet_transform;
    crude_entity                                           boss_bullet_node;

    boss_bullet = &boss_bullet_per_entity[ i ];
    boss_bullet_transform = &transform_per_entity[ i ];
    boss_bullet_node = CRUDE_COMPOUNT( crude_entity, { it->entities[ i ], it->world } );

    XMVECTOR dir = XMVector3Normalize( XMVectorSubtract( XMLoadFloat3( &boss_bullet->target ), XMLoadFloat3( &boss_bullet_transform->translation ) ) );
    XMStoreFloat3( &boss_bullet_transform->translation, XMVectorAdd( XMLoadFloat3( &boss_bullet_transform->translation ), XMVectorScale( dir, boss_fight->type ? it->delta_time * 75.0f : it->delta_time * 50.0f ) ) ); 
    boss_bullet->lifetime += it->delta_time;

    if ( boss_bullet->lifetime > 5.f )
    {
      crude_entity_destroy_hierarchy( boss_bullet_node );
    }
  }
  CRUDE_PROFILER_ZONE_END;
}

static void
crude_boss_update_system_
(
  _In_ ecs_iter_t *it
)
{
  CRUDE_PROFILER_ZONE_NAME( "crude_boss_update_system_" );
  game_t *game = game_instance( );
  crude_boss *boss_per_entity = ecs_field( it, crude_boss, 0 );
  crude_transform *transform_per_entity = ecs_field( it, crude_transform, 1 );

  if ( game->death_screen )
  {
    return;
  }

  for ( uint32 i = 0; i < it->count; ++i )
  {
    crude_boss                                            *boss;
    crude_transform                                       *boss_transform;
    crude_player const                                    *player;
    crude_transform const                                 *player_transform;
    crude_entity                                           boss_node;
    XMVECTOR                                               boss_translation;
    XMVECTOR                                               boss_to_point, boss_to_point_normalized, boss_to_point_translation_2d;
    float32                                                boss_to_point_distance, angle;

    boss = &boss_per_entity[ i ];
    boss_transform = &transform_per_entity[ i ];
    boss_node = CRUDE_COMPOUNT( crude_entity, { it->entities[ i ], it->world } );

    CRUDE_ASSERT( crude_entity_valid( game->player_node ) );
    
    player = CRUDE_ENTITY_GET_IMMUTABLE_COMPONENT( game->player_node, crude_player );
    player_transform = CRUDE_ENTITY_GET_IMMUTABLE_COMPONENT( game->player_node, crude_transform );

    boss_translation = XMLoadFloat3( &boss_transform->translation );
    boss_to_point = XMVectorSubtract( XMLoadFloat3( &player_transform->translation ), boss_translation );
    boss_to_point_distance = XMVectorGetX( XMVector3Length( boss_to_point ) );
    boss_to_point_normalized = XMVector3Normalize( boss_to_point );
    
    boss_to_point_translation_2d = XMVectorSet( XMVectorGetX( boss_to_point ), XMVectorGetZ( boss_to_point ), 0.f, 0.f );
    angle = atan2f( XMVectorGetX( boss_to_point_translation_2d ), XMVectorGetY( boss_to_point_translation_2d ) );
    boss->target_looking_angle = crude_lerp_angle( boss->target_looking_angle, angle, 3 * it->delta_time );

    XMStoreFloat4( &boss_transform->rotation, XMQuaternionRotationAxis( g_XMIdentityR1, boss->target_looking_angle ) );

    if ( boss->last_shot_time > 0.75f )
    {
      char                                                 bullet_node_name_buffer[ 512 ];
      static int                                           bullet_id = 0;

      boss->last_shot_time = 0.f;
     
      crude_entity bullet_spawnpoint_node = crude_ecs_lookup_entity_from_parent( boss_node, "bullet_spawnpoint" );
      XMFLOAT3 translation;
      XMStoreFloat3( &translation, crude_transform_node_to_world( bullet_spawnpoint_node, CRUDE_ENTITY_GET_IMMUTABLE_COMPONENT( bullet_spawnpoint_node, crude_transform ) ).r[ 3 ] );

      crude_snprintf( bullet_node_name_buffer, sizeof( bullet_node_name_buffer ), "bullet_%i", bullet_id++ );
      crude_entity new_bollet_node = crude_entity_copy_hierarchy( game->template_boss_bullet_node, bullet_node_name_buffer, true, true );
      crude_entity_set_parent( new_bollet_node, game->main_node );
      crude_transform new_transform = crude_transform_empty( );
      new_transform.translation = translation;
      CRUDE_ENTITY_ADD_COMPONENT( new_bollet_node, crude_node_runtime );
      CRUDE_ENTITY_SET_COMPONENT( new_bollet_node, crude_boss_bullet, { player_transform->translation } );
      CRUDE_ENTITY_SET_COMPONENT( new_bollet_node, crude_transform, { new_transform } );

    }

    boss->last_shot_time += it->delta_time;
  }
  CRUDE_PROFILER_ZONE_END;
}

CRUDE_ECS_MODULE_IMPORT_IMPL( crude_boss_system )
{
  ECS_MODULE( world, crude_boss_system );
  
  ECS_IMPORT( world, crude_scene_components );
  ECS_IMPORT( world, crude_physics_components );
  ECS_IMPORT( world, crude_game_components );
  
  CRUDE_ECS_OBSERVER_DEFINE( world, crude_boss_creation_observer_, EcsOnSet, NULL, { 
    { .id = ecs_id( crude_boss ) }
  } );
  
  CRUDE_ECS_OBSERVER_DEFINE( world, crude_boss_destroy_observer_, EcsOnRemove, NULL, { 
    { .id = ecs_id( crude_boss ) }
  } );

  CRUDE_ECS_SYSTEM_DEFINE( world, crude_boss_update_system_, EcsOnUpdate, NULL, {
    { .id = ecs_id( crude_boss ) },
    { .id = ecs_id( crude_transform ) }
  } );

  CRUDE_ECS_SYSTEM_DEFINE( world, crude_boss_bullet_update_system_, EcsOnUpdate, NULL, {
    { .id = ecs_id( crude_boss_bullet ) },
    { .id = ecs_id( crude_transform ) }
  } );
}