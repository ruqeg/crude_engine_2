#include <engine/scene/scene_debug_components.h>
#include <engine/external/game_components.h>

#include <engine/external/game_debug_system.h>

CRUDE_ECS_OBSERVER_DECLARE( crude_level_01_creation_observer_ );

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
    level_node = CRUDE_COMPOUNT( crude_entity, { it->entities[ i ], it->world } );

    level->editor_camera_controller_enabled = true;
    level->enemies_spawn_points_parent_node = crude_ecs_lookup_entity_from_parent( level_node, "enemies_spawnpoints" );
    
    ecs_iter_t entity_swapnpoint_it = ecs_children( it->world, level->enemies_spawn_points_parent_node.handle );
    while ( ecs_children_next( &entity_swapnpoint_it ) )
    {
      for ( size_t i = 0; i < entity_swapnpoint_it.count; ++i )
      {
        crude_entity                                       entity_swapnpoint_node;
        entity_swapnpoint_node = CRUDE_COMPOUNT( crude_entity, { .handle = entity_swapnpoint_it.entities[ i ], .world = it->world } );
        CRUDE_ENTITY_SET_COMPONENT( entity_swapnpoint_node, crude_debug_gltf, { ctx->enemy_spawnpoint_model_absolute_filepath, true } );
      }
    }
  }
}

void
crude_game_debug_system_import
(
  _In_ ecs_world_t                                        *world,
	_In_ crude_game_debug_system_context									  *ctx
)
{
  if ( !ctx->enemy_spawnpoint_model_absolute_filepath )
  {
    ctx->enemy_spawnpoint_model_absolute_filepath = crude_string_buffer_append_use_f( ctx->string_bufffer, "%s%s", ctx->resources_absolute_directory, "debug\\models\\enemy_spawnpoint_model.gltf" );
  }
  CRUDE_ECS_OBSERVER_DEFINE( world, crude_level_01_creation_observer_, EcsOnSet, ctx, { 
    { .id = ecs_id( crude_level_01 ) }
  } );
}