#include <SDL3/SDL.h>

#include <engine/core/memory.h>
#include <engine/external/game_components.h>
#include <engine/platform/platform.h>
#include <engine/audio/audio_device.h>
#include <game/game.h>

#include <game/level_01_system.h>

CRUDE_ECS_SYSTEM_DECLARE( crude_level_01_update_system_ );
CRUDE_ECS_OBSERVER_DECLARE( crude_level_01_creation_observer_ );
CRUDE_ECS_OBSERVER_DECLARE( crude_level_01_destroy_observer_ );

static void
crude_level_01_destroy_observer_
(
  ecs_iter_t *it
)
{
  game_t                                                  *game;
  crude_level_01                                          *enemies_per_entity;

  game = game_instance( );
  enemies_per_entity = ecs_field( it, crude_level_01, 0 );

  for ( uint32 i = 0; i < it->count; ++i )
  {
    crude_level_01                                        *level;
    crude_window_handle                                   *window_handle;
    crude_entity                                           level_node;
    
    level = &enemies_per_entity[ i ];
    level_node = CRUDE_COMPOUNT( crude_entity, { it->entities[ i ], it->world } );

    crude_audio_device_destroy_sound( &game->audio_device, level->ambient_sound_handle );
    crude_audio_device_destroy_sound( &game->audio_device, level->save_theme_sound_handle );
    crude_audio_device_destroy_sound( &game->audio_device, level->shot_sound_handle );
    crude_audio_device_destroy_sound( &game->audio_device, level->shot_without_ammo_sound_handle );
    crude_audio_device_destroy_sound( &game->audio_device, level->recycle_sound_handle );
    crude_audio_device_destroy_sound( &game->audio_device, level->hit_critical_sound_handle );
    crude_audio_device_destroy_sound( &game->audio_device, level->hit_basic_sound_handle );
    crude_audio_device_destroy_sound( &game->audio_device, level->take_serum_sound_handle );
    crude_audio_device_destroy_sound( &game->audio_device, level->recycle_interaction_sound_handle );
  }
}
static void
crude_level_01_creation_observer_
(
  ecs_iter_t *it
)
{
  game_t                                                  *game;
  crude_level_01                                          *enemies_per_entity;

  game = game_instance( );
  enemies_per_entity = ecs_field( it, crude_level_01, 0 );

  for ( uint32 i = 0; i < it->count; ++i )
  {
    crude_level_01                                        *level;
    crude_window_handle                                   *window_handle;
    crude_entity                                           level_node;
    
    level = &enemies_per_entity[ i ];
    level_node = CRUDE_COMPOUNT( crude_entity, { it->entities[ i ], it->world } );

    level->editor_camera_controller_enabled = true;
    
    /* Preload models */
    crude_gfx_model_renderer_resources_manager_get_gltf_model( &game->model_renderer_resources_manager, game->serum_model_absolute_filepath , NULL );
    crude_gfx_model_renderer_resources_manager_get_gltf_model( &game->model_renderer_resources_manager, game->syringe_drug_model_absolute_filepath, NULL );
    crude_gfx_model_renderer_resources_manager_get_gltf_model( &game->model_renderer_resources_manager, game->syringe_health_model_absolute_filepath, NULL );
    crude_gfx_model_renderer_resources_manager_get_gltf_model( &game->model_renderer_resources_manager, game->serum_station_enabled_model_absolute_filepath, NULL );
    crude_gfx_model_renderer_resources_manager_get_gltf_model( &game->model_renderer_resources_manager, game->serum_station_disabled_model_absolute_filepath, NULL );
    crude_gfx_model_renderer_resources_manager_get_gltf_model( &game->model_renderer_resources_manager, game->ammo_box_model_absolute_filepath, NULL );
    
#if CRUDE_DEVELOP
    crude_gfx_model_renderer_resources_manager_get_gltf_model( &game->model_renderer_resources_manager, game->syringe_spawnpoint_debug_model_absolute_filepath, NULL );
    crude_gfx_model_renderer_resources_manager_get_gltf_model( &game->model_renderer_resources_manager, game->enemy_spawnpoint_debug_model_absolute_filepath, NULL );
    crude_gfx_model_renderer_resources_manager_get_gltf_model( &game->model_renderer_resources_manager, game->syringe_serum_station_active_debug_model_absolute_filepath, NULL );
#endif

    /* Setup sounds */
    {
      crude_sound_creation sound_creation = crude_sound_creation_empty( );
      sound_creation.looping = true;
      sound_creation.stream = true;
      sound_creation.decode = true;
      sound_creation.absolute_filepath = game->ambient_sound_absolute_filepath;
      sound_creation.positioning = CRUDE_AUDIO_SOUND_POSITIONING_RELATIVE;
      level->ambient_sound_handle = crude_audio_device_create_sound( &game->audio_device, &sound_creation );
      
      sound_creation = crude_sound_creation_empty( );
      sound_creation.looping = true;
      sound_creation.stream = true;
      sound_creation.decode = true;
      sound_creation.absolute_filepath = game->save_theme_sound_absolute_filepath;
      sound_creation.positioning = CRUDE_AUDIO_SOUND_POSITIONING_RELATIVE;
      level->save_theme_sound_handle = crude_audio_device_create_sound( &game->audio_device, &sound_creation );

      sound_creation = crude_sound_creation_empty( );
      sound_creation.async_loading = true;
      sound_creation.absolute_filepath = game->shot_sound_absolute_filepath;
      sound_creation.positioning = CRUDE_AUDIO_SOUND_POSITIONING_RELATIVE;
      level->shot_sound_handle = crude_audio_device_create_sound( &game->audio_device, &sound_creation );

      sound_creation = crude_sound_creation_empty( );
      sound_creation.async_loading = true;
      sound_creation.absolute_filepath = game->shot_without_ammo_sound_absolute_filepath;
      sound_creation.positioning = CRUDE_AUDIO_SOUND_POSITIONING_RELATIVE;
      level->shot_without_ammo_sound_handle = crude_audio_device_create_sound( &game->audio_device, &sound_creation );
      
      sound_creation = crude_sound_creation_empty( );
      sound_creation.async_loading = true;
      sound_creation.absolute_filepath = game->recycle_sound_absolute_filepath;
      sound_creation.positioning = CRUDE_AUDIO_SOUND_POSITIONING_RELATIVE;
      level->recycle_sound_handle = crude_audio_device_create_sound( &game->audio_device, &sound_creation );
      crude_audio_device_sound_set_volume( &game->audio_device, level->recycle_sound_handle, 1.5f );

      sound_creation = crude_sound_creation_empty( );
      sound_creation.async_loading = true;
      sound_creation.absolute_filepath = game->hit_critical_sound_absolute_filepath;
      sound_creation.rolloff = 0.15;
      level->hit_critical_sound_handle = crude_audio_device_create_sound( &game->audio_device, &sound_creation );
      crude_audio_device_sound_set_volume( &game->audio_device, level->hit_critical_sound_handle, 5.0f );
      
      sound_creation = crude_sound_creation_empty( );
      sound_creation.async_loading = true;
      sound_creation.absolute_filepath = game->hit_basic_sound_absolute_filepath;
      sound_creation.rolloff = 0.15;
      level->hit_basic_sound_handle = crude_audio_device_create_sound( &game->audio_device, &sound_creation );
      crude_audio_device_sound_set_volume( &game->audio_device, level->hit_basic_sound_handle, 5.0f );

      sound_creation = crude_sound_creation_empty( );
      sound_creation.async_loading = true;
      sound_creation.absolute_filepath = game->take_serum_sound_absolute_filepath;
      sound_creation.max_distance = CRUDE_GAME_PLAYER_MAX_FOG_DISTANCE;
      level->take_serum_sound_handle = crude_audio_device_create_sound( &game->audio_device, &sound_creation );

      sound_creation = crude_sound_creation_empty( );
      sound_creation.async_loading = true;
      sound_creation.absolute_filepath = game->recycle_interaction_sound_absolute_filepath;
      sound_creation.min_distance = 1.0;
      sound_creation.max_distance = CRUDE_GAME_PLAYER_MAX_FOG_DISTANCE;
      sound_creation.rolloff = 0.25;
      level->recycle_interaction_sound_handle = crude_audio_device_create_sound( &game->audio_device, &sound_creation );

      crude_audio_device_sound_start( &game->audio_device, level->ambient_sound_handle );
      crude_audio_device_sound_start( &game->audio_device, level->save_theme_sound_handle );
      crude_audio_device_sound_set_volume( &game->audio_device, level->save_theme_sound_handle, 0.f );
      crude_audio_device_sound_set_volume( &game->audio_device, level->ambient_sound_handle, 1.f );
    }

    /* Setup enemies*/
    {
      ecs_iter_t                                           entity_swapnpoint_it;
      uint32                                               enemy_count;

      level->enemies_spawn_points_parent_node = crude_ecs_lookup_entity_from_parent( level_node, "enemies_spawnpoints" );

      enemy_count = 0;
      entity_swapnpoint_it = ecs_children( it->world, level->enemies_spawn_points_parent_node.handle );
      while ( ecs_children_next( &entity_swapnpoint_it ) )
      {
        for ( size_t i = 0; i < entity_swapnpoint_it.count; ++i )
        {
          crude_transform const                             *entity_spawn_point_transform;
          crude_enemy                                       *enemy;
          crude_transform                                    enemy_transform;
          crude_entity                                       entity_swapnpoint_node, enemy_node;
          crude_enemy                                        enemy_component;
          XMMATRIX                                           entity_swapn_point_to_world;
          XMVECTOR                                           translation, scale, rotation;
          char                                               enemy_node_name_buffer[ 512 ];

          entity_swapnpoint_node = CRUDE_COMPOUNT( crude_entity, { .handle = entity_swapnpoint_it.entities[ i ], .world = it->world } );

          entity_spawn_point_transform = CRUDE_ENTITY_GET_IMMUTABLE_COMPONENT( entity_swapnpoint_node, crude_transform );
          entity_swapn_point_to_world = crude_transform_node_to_world( entity_swapnpoint_node, entity_spawn_point_transform );

          enemy_transform = crude_transform_empty( );
          enemy_transform.translation.x = XMVectorGetX( entity_swapn_point_to_world.r[ 3 ] );
          enemy_transform.translation.z = XMVectorGetZ( entity_swapn_point_to_world.r[ 3 ] );
          crude_snprintf( enemy_node_name_buffer, sizeof( enemy_node_name_buffer ), "enemy_%i", enemy_count );
          enemy_node = crude_entity_copy_hierarchy( game->template_enemy_node, enemy_node_name_buffer, true, true );
          crude_entity_set_parent( enemy_node, level_node );
          CRUDE_ENTITY_ADD_COMPONENT( enemy_node, crude_node_runtime );

          enemy_component = CRUDE_COMPOUNT_EMPTY( crude_enemy );
          enemy_component.moving_speed = 5;
          enemy_component.spawn_node_translation = enemy_transform.translation;
          CRUDE_ENTITY_SET_COMPONENT( enemy_node, crude_enemy, { enemy_component } );
          CRUDE_ENTITY_SET_COMPONENT( enemy_node, crude_transform, { enemy_transform } );

          ++enemy_count;
        }
      }
    }

    /* Setup serum stations */
    {
      ecs_iter_t                                           serum_stations_swapn_points_it;
      uint32                                               serum_stations_count;
      
      level->serum_stations_spawn_points_parent_node = crude_ecs_lookup_entity_from_parent( level_node, "serum_station_spawn_points" );

      serum_stations_count = 0;
      serum_stations_swapn_points_it = ecs_children( it->world, level->serum_stations_spawn_points_parent_node.handle );
      while ( ecs_children_next( &serum_stations_swapn_points_it ) )
      {
        for ( size_t i = 0; i < serum_stations_swapn_points_it.count; ++i )
        {
          crude_transform const                             *serum_station_spawn_point_transform;
          crude_enemy                                       *serum_station;
          crude_transform                                    serum_station_transform;
          crude_entity                                       serum_station_spawn_point_node, serum_station_node;
          crude_serum_station                                serum_station_component;
          XMMATRIX                                           serum_station_spawn_point_to_world;
          XMVECTOR                                           translation, scale, rotation;
          char                                               serum_station_node_name_buffer[ 512 ];

          serum_station_spawn_point_node = CRUDE_COMPOUNT( crude_entity, { .handle = serum_stations_swapn_points_it.entities[ i ], .world = it->world } );

          serum_station_spawn_point_transform = CRUDE_ENTITY_GET_IMMUTABLE_COMPONENT( serum_station_spawn_point_node, crude_transform );
          serum_station_spawn_point_to_world = crude_transform_node_to_world( serum_station_spawn_point_node, serum_station_spawn_point_transform );

          serum_station_transform = crude_transform_empty( );
          serum_station_transform.translation.x = XMVectorGetX( serum_station_spawn_point_to_world.r[ 3 ] );
          serum_station_transform.translation.z = XMVectorGetZ( serum_station_spawn_point_to_world.r[ 3 ] );
          
          crude_snprintf( serum_station_node_name_buffer, sizeof( serum_station_node_name_buffer ), "serum_station_%i", serum_stations_count );
          serum_station_node = crude_entity_copy_hierarchy( game->template_serum_station_node, serum_station_node_name_buffer, true, true );
          crude_entity_set_parent( serum_station_node, level_node );
          CRUDE_ENTITY_ADD_COMPONENT( serum_station_node, crude_node_runtime );

          serum_station_component = CRUDE_COMPOUNT_EMPTY( crude_serum_station );
          CRUDE_ENTITY_SET_COMPONENT( serum_station_node, crude_serum_station, { serum_station_component } );
          CRUDE_ENTITY_SET_COMPONENT( serum_station_node, crude_transform, { serum_station_transform } );
          ++serum_stations_count;
        }
      }
    }

    for ( uint32 i = 0; i < CRUDE_GAME_SERUM_STATION_MAX_ACTIVE_COUNT; ++i )
    {
      game_push_enable_random_serum_station_command( game, CRUDE_COMPOUNT_EMPTY( crude_entity ) );
    }

    window_handle = CRUDE_ENTITY_GET_MUTABLE_COMPONENT( game->platform_node, crude_window_handle );
    crude_platform_hide_cursor( *window_handle );
  }
}

static void
crude_level_01_update_system_
(
  _In_ ecs_iter_t *it
)
{
  game_t *game = game_instance( );
  crude_level_01 *leveles_per_entity = ecs_field( it, crude_level_01, 0 );

  for ( uint32 i = 0; i < it->count; ++i )
  {
    crude_player const                                    *player;
    crude_level_01                                        *level;
    crude_entity                                           level_node;
    
    //game->game_controller_node = crude_ecs_lookup_entity_from_parent( game->scene.main_node, "player" );
    //game->game_camera_node = crude_ecs_lookup_entity_from_parent( game->scene.main_node, "player.pivot.camera" );

    level = &leveles_per_entity[ i ];
    level_node = CRUDE_COMPOUNT( crude_entity, { it->entities[ i ], it->world } );

    player = CRUDE_ENTITY_GET_IMMUTABLE_COMPONENT( game->player_node, crude_player );
    //game-> player->drug_withdrawal
  }
}

CRUDE_ECS_MODULE_IMPORT_IMPL( crude_level_01_system )
{
  ECS_MODULE( world, crude_level_01_system );
  
  ECS_IMPORT( world, crude_game_components );

  CRUDE_ECS_OBSERVER_DEFINE( world, crude_level_01_creation_observer_, EcsOnSet, NULL, { 
    { .id = ecs_id( crude_level_01 ) }
  } );

  CRUDE_ECS_OBSERVER_DEFINE( world, crude_level_01_destroy_observer_, EcsOnRemove, NULL, { 
    { .id = ecs_id( crude_level_01 ) }
  } );

  CRUDE_ECS_SYSTEM_DEFINE( world, crude_level_01_update_system_, EcsOnUpdate, NULL, {
    { .id = ecs_id( crude_level_01 ) }
  } );
}