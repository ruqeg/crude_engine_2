#include <engine/core/log.h>
#include <engine/core/profiler.h>
#include <engine/scene/scene_ecs.h>
#include <engine/platform/platform.h>
#include <engine/graphics/imgui.h>
#include <game/game.h>

#include <game/player_ecs.h>

/**********************************************************
 *
 *                 Component
 *
 *********************************************************/
ECS_COMPONENT_DECLARE( crude_player );
CRUDE_COMPONENT_STRING_DEFINE( crude_player, "crude_player" );

CRUDE_PARSE_JSON_TO_COMPONENT_FUNC_IMPLEMENTATION( crude_player )
{
  crude_memory_set( component, 0, sizeof( crude_player ) );
  return true;
}

CRUDE_PARSE_COMPONENT_TO_JSON_FUNC_IMPLEMENTATION( crude_player )
{
  cJSON *free_camera_json = cJSON_CreateObject( );
  cJSON_AddItemToObject( free_camera_json, "type", cJSON_CreateString( CRUDE_COMPONENT_STRING( crude_player ) ) );
  return free_camera_json;
}

CRUDE_PARSE_COMPONENT_TO_IMGUI_FUNC_IMPLEMENTATION( crude_player )
{
  CRUDE_IMGUI_START_OPTIONS;
}

/**********************************************************
 *
 *                 System
 *
 *********************************************************/
CRUDE_ECS_OBSERVER_DECLARE( crude_player_create_observer_ );
CRUDE_ECS_SYSTEM_DECLARE( crude_player_game_update_system_ );

void
crude_player_create_observer_
(
  _In_ ecs_iter_t                                         *it
);

void
crude_player_system_import
(
  _In_ crude_ecs                                          *world,
  _In_ crude_components_serialization_manager             *manager,
  _In_ crude_player_system_context                        *ctx
)
{
  CRUDE_ECS_MODULE( world, crude_player_system );
  
  CRUDE_ECS_COMPONENT_DEFINE( world, crude_player );
  CRUDE_PARSE_JSON_TO_COMPONENT_FUNC_DEFINE( manager, crude_player );
  CRUDE_PARSE_COMPONENT_TO_JSON_FUNC_DEFINE( manager, crude_player );
  CRUDE_PARSE_COMPONENT_TO_IMGUI_FUNC_DEFINE( manager, crude_player );

  crude_scene_components_import( world, manager );
  
  CRUDE_ECS_OBSERVER_DEFINE( world, crude_player_game_update_system_, EcsOnSet, ctx, { 
    { .id = ecs_id( crude_player ), .oper = EcsAnd }
  } );
  
  CRUDE_ECS_OBSERVER_DEFINE( world, crude_player_create_observer_, EcsOnSet, ctx, { 
    { .id = ecs_id( crude_player ), .oper = EcsAnd }
  } );
}

void
crude_player_game_update_system_
(
  _In_ ecs_iter_t                                         *it
)
{
  CRUDE_PROFILER_ZONE_NAME( "crude_player_game_update_system_" );

  crude_game                                              *game;
  crude_player_system_context                             *ctx;
  crude_player                                            *player_per_entity;

  game = crude_game_instance( );
  ctx = CRUDE_CAST( crude_player_system_context*, it->ctx );
  player_per_entity = ecs_field( it, crude_player, 0 );
  
  for ( uint32 i = 0; i < it->count; ++i )
  {
  }
  CRUDE_PROFILER_ZONE_END;
}

void
crude_player_create_observer_
(
  _In_ ecs_iter_t                                         *it
)
{
  CRUDE_PROFILER_ZONE_NAME( "crude_player_create_observer_" );

  crude_game                                              *game;
  crude_player_system_context                             *ctx;
  crude_player                                            *player_per_entity;

  game = crude_game_instance( );
  ctx = CRUDE_CAST( crude_player_system_context*, it->ctx );
  player_per_entity = ecs_field( it, crude_player, 0 );
  
  for ( uint32 i = 0; i < it->count; ++i )
  {
    crude_player                                          *player;

    player = &player_per_entity[ i ];

    /* Weapon */
    {
      crude_weapon                                        *weapon;
      crude_entity                                         player_character_entity;
      crude_entity                                         player_orientation_entity;
      crude_entity                                         weapon_entity;

      player_character_entity = crude_ecs_lookup_entity_from_parent( it->world, it->entities[ i ], "character" );
      player_orientation_entity = crude_ecs_lookup_entity_from_parent( it->world, player_character_entity, "orientation" );
      weapon_entity = crude_ecs_lookup_entity_from_parent( it->world, player_orientation_entity, "weapon" );

      weapon = CRUDE_ENTITY_GET_MUTABLE_COMPONENT( it->world, weapon_entity, crude_weapon );

      crude_weapon_fill_ammo( weapon );
    }
  }
  CRUDE_PROFILER_ZONE_END;
}