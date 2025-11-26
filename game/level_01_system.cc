#include <SDL3/SDL.h>

#include <engine/core/memory.h>
#include <engine/external/game_components.h>
#include <engine/platform/platform.h>
#include <game/game.h>

#include <game/level_01_system.h>

CRUDE_ECS_SYSTEM_DECLARE( crude_level_01_update_system_ );
CRUDE_ECS_OBSERVER_DECLARE( crude_level_01_creation_observer_ );

static void
crude_level_01_creation_observer_
(
  ecs_iter_t *it
)
{
  game_t                                                  *game;
  crude_level_01                                          *enemies_per_entity;
  char                                                     enemy_node_name_buffer[ 512 ];

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
    level->enemies_spawn_points_parent_node = crude_ecs_lookup_entity_from_parent( level_node, "enemies_spawnpoints" );
    
    uint32 enemy_count = 0;
    ecs_iter_t entity_swapnpoint_it = ecs_children( it->world, level->enemies_spawn_points_parent_node.handle );
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

        entity_swapnpoint_node = CRUDE_COMPOUNT( crude_entity, { .handle = entity_swapnpoint_it.entities[ i ], .world = it->world } );

        entity_spawn_point_transform = CRUDE_ENTITY_GET_IMMUTABLE_COMPONENT( entity_swapnpoint_node, crude_transform );
        entity_swapn_point_to_world = crude_transform_node_to_world( entity_swapnpoint_node, entity_spawn_point_transform );

        enemy_transform = crude_transform_empty( );
        enemy_transform.translation.x = XMVectorGetX( entity_swapn_point_to_world.r[ 3 ] );
        enemy_transform.translation.z = XMVectorGetZ( entity_swapn_point_to_world.r[ 3 ] );
        
        crude_snprintf( enemy_node_name_buffer, sizeof( enemy_node_name_buffer ), "enemy_%i", enemy_count );
        enemy_node = crude_entity_copy_hierarchy( game->template_enemy_node, enemy_node_name_buffer, true );
        crude_entity_set_parent( enemy_node, level_node );
        CRUDE_ENTITY_ENABLE( enemy_node );
        CRUDE_ENTITY_ADD_COMPONENT( enemy_node, crude_node_runtime );

        enemy_component = CRUDE_COMPOUNT_EMPTY( crude_enemy );
        enemy_component.moving_speed = 5;
        enemy_component.spawn_node_translation = enemy_transform.translation;
        CRUDE_ENTITY_SET_COMPONENT( enemy_node, crude_enemy, { enemy_component } );
        CRUDE_ENTITY_SET_COMPONENT( enemy_node, crude_transform, { enemy_transform } );
        ++enemy_count;
      }
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
  crude_level_01 *leveles_per_entity = ecs_field( it, crude_level_01, 0 );

  for ( uint32 i = 0; i < it->count; ++i )
  {
    crude_level_01                                        *level;
    crude_entity                                           level_node;
    
    //game->game_controller_node = crude_ecs_lookup_entity_from_parent( game->scene.main_node, "player" );
    //game->game_camera_node = crude_ecs_lookup_entity_from_parent( game->scene.main_node, "player.pivot.camera" );

    level = &leveles_per_entity[ i ];
    level_node = CRUDE_COMPOUNT( crude_entity, { it->entities[ i ], it->world } );
  }
}

CRUDE_ECS_MODULE_IMPORT_IMPL( crude_level_01_system )
{
  ECS_MODULE( world, crude_level_01_system );
  
  ECS_IMPORT( world, crude_game_components );

  CRUDE_ECS_OBSERVER_DEFINE( world, crude_level_01_creation_observer_, EcsOnSet, NULL, { 
    { .id = ecs_id( crude_level_01 ) }
  } );

  CRUDE_ECS_SYSTEM_DEFINE( world, crude_level_01_update_system_, EcsOnUpdate, NULL, {
    { .id = ecs_id( crude_level_01 ) }
  } );
}