#if CRUDE_DEVELOP

#include <engine/scene/scene_debug_ecs.h>
#include <engine/external/game_ecs.h>

#include <engine/external/game_debug_ecs.h>

/**********************************************************
 *
 *                 System
 *
 *********************************************************/
CRUDE_ECS_OBSERVER_DECLARE( crude_level_01_creation_observer_ );
CRUDE_ECS_OBSERVER_DECLARE( crude_serum_station_enabled_creation_observer_ );
CRUDE_ECS_OBSERVER_DECLARE( crude_serum_station_enabled_destroy_observer_ );

static void
crude_level_01_creation_observer_
(
  ecs_iter_t *it
)
{
  crude_game_debug_system_context *ctx = CRUDE_CAST( crude_game_debug_system_context*, it->ctx );
  crude_level_01 *enemies_per_entity = ecs_field( it, crude_level_01, 0 );

  for ( uint32 i = 0; i < it->count; ++i )
  {
    crude_level_01                                        *level;
    crude_entity                                           level_node;
    
    level = &enemies_per_entity[ i ];
    level_node = crude_entity_from_iterator( it, i );

    level->editor_camera_controller_enabled = true;
    level->enemies_spawn_points_parent_node = crude_ecs_lookup_entity_from_parent( it->world, level_node, "enemies_spawnpoints" );
    level->serum_stations_spawn_points_parent_node = crude_ecs_lookup_entity_from_parent( it->world, level_node, "serum_station_spawn_points" );

    ecs_iter_t entity_swapnpoint_it = crude_ecs_children( it->world, level->enemies_spawn_points_parent_node );
    while ( ecs_children_next( &entity_swapnpoint_it ) )
    {
      for ( size_t j = 0; j < entity_swapnpoint_it.count; ++j )
      {
        crude_entity                                       entity_swapnpoint_node;
        entity_swapnpoint_node = crude_entity_from_iterator( &entity_swapnpoint_it, j );
        CRUDE_ENTITY_SET_COMPONENT( it->world, entity_swapnpoint_node, crude_debug_gltf, { ctx->enemy_spawnpoint_model_absolute_filepath, true } );
      }
    }

    ecs_iter_t entity_syringe_spawn_point_it = crude_ecs_children( it->world, level->serum_stations_spawn_points_parent_node );
    while ( ecs_children_next( &entity_syringe_spawn_point_it ) )
    {
      for ( size_t j = 0; j < entity_syringe_spawn_point_it.count; ++j )
      {
        crude_entity                                       entity_spawn_point_node;
        entity_spawn_point_node = crude_entity_from_iterator( &entity_syringe_spawn_point_it, j );
        CRUDE_ENTITY_SET_COMPONENT( it->world, entity_spawn_point_node, crude_debug_gltf, { ctx->syringe_spawnpoint_model_absolute_filepath, true } );
      }
    }
  }
}

static void
crude_serum_station_enabled_creation_observer_
(
  ecs_iter_t *it
)
{
  crude_game_debug_system_context *ctx = CRUDE_CAST( crude_game_debug_system_context*, it->ctx );

  for ( uint32 i = 0; i < it->count; ++i )
  {
    crude_entity                                           serum_station_node;
    
    serum_station_node = crude_entity_from_iterator( it, i );

    CRUDE_ENTITY_SET_COMPONENT( it->world, serum_station_node, crude_debug_gltf, { ctx->syringe_serum_station_active_model_absolute_filepath, true } );
  }
}

static void
crude_serum_station_enabled_destroy_observer_
(
  ecs_iter_t *it
)
{
  crude_game_debug_system_context *ctx = CRUDE_CAST( crude_game_debug_system_context*, it->ctx );

  for ( uint32 i = 0; i < it->count; ++i )
  {
    crude_entity                                           serum_station_node;
    
    serum_station_node = crude_entity_from_iterator( it, i );

    CRUDE_ENTITY_REMOVE_COMPONENT( it->world, serum_station_node, crude_debug_gltf );
  }
}

void
crude_game_debug_system_import
(
  _In_ crude_ecs                                          *world,
  _In_ crude_game_debug_system_context                    *ctx
)
{
  CRUDE_ASSERT( ctx->syringe_spawnpoint_model_absolute_filepath );
  CRUDE_ASSERT( ctx->enemy_spawnpoint_model_absolute_filepath );
    
  CRUDE_ECS_OBSERVER_DEFINE( world, crude_level_01_creation_observer_, EcsOnSet, ctx, { 
    { .id = ecs_id( crude_level_01 ) }
  } );

  if ( ctx->syringe_serum_station_active_model_absolute_filepath )
  {
    CRUDE_ECS_OBSERVER_DEFINE( world, crude_serum_station_enabled_creation_observer_, EcsOnSet, ctx, { 
      { .id = ecs_id( crude_serum_station_enabled ) }
    } );

    CRUDE_ECS_OBSERVER_DEFINE( world, crude_serum_station_enabled_destroy_observer_, EcsOnRemove, ctx, { 
      { .id = ecs_id( crude_serum_station_enabled ) }
    } );
  }
}

#endif