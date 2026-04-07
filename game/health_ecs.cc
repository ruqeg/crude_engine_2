#include <engine/core/log.h>
#include <engine/core/profiler.h>
#include <engine/scene/scene_ecs.h>
#include <engine/platform/platform.h>
#include <engine/graphics/imgui.h>
#include <game/game.h>

#include <game/health_ecs.h>

/**********************************************************
 *
 *                 Component
 *
 *********************************************************/
ECS_COMPONENT_DECLARE( crude_health );
CRUDE_COMPONENT_STRING_DEFINE( crude_health, "crude_health" );

CRUDE_PARSE_JSON_TO_COMPONENT_FUNC_IMPLEMENTATION( crude_health )
{
  crude_memory_set( component, 0, sizeof( crude_health ) );
  component->max_health = cJSON_GetNumberValue( cJSON_GetObjectItemCaseSensitive( component_json, "max_health" ) );
  return true;
}

CRUDE_PARSE_COMPONENT_TO_JSON_FUNC_IMPLEMENTATION( crude_health )
{
  cJSON *free_camera_json = cJSON_CreateObject( );
  cJSON_AddItemToObject( free_camera_json, "type", cJSON_CreateString( CRUDE_COMPONENT_STRING( crude_health ) ) );
  cJSON_AddItemToObject( free_camera_json, "max_health", cJSON_CreateNumber( component->max_health ) );
  return free_camera_json;
}

CRUDE_PARSE_COMPONENT_TO_IMGUI_FUNC_IMPLEMENTATION( crude_health )
{
  CRUDE_IMGUI_START_OPTIONS;

  CRUDE_IMGUI_OPTION( "Max health", {
    ImGui::DragInt( "##Max health", &component->max_health );
    });
}

/**********************************************************
 *
 *                 API
 *
 *********************************************************/
void
crude_heal_receive_damage 
(
  _In_ crude_entity                                        health_entity,
  _In_ crude_health                                       *health,
  _In_ int32                                               damage
)
{
  health->total_damage -= damage;

  if ( health->callback_container.damage_callback )
  {
    health->callback_container.damage_callback( health_entity, health, damage );
  }
}

/**********************************************************
 *
 *                 System
 *
 *********************************************************/
CRUDE_ECS_OBSERVER_DECLARE( crude_health_create_observer );
CRUDE_ECS_SYSTEM_DECLARE( crude_health_game_update_system_ );

static void
crude_health_create_observer
(
  _In_ ecs_iter_t                                         *it
);

void
crude_health_system_import
(
  _In_ crude_ecs                                          *world,
  _In_ crude_components_serialization_manager             *manager,
  _In_ crude_health_system_context                        *ctx
)
{
  CRUDE_ECS_MODULE( world, crude_health_system );
  
  CRUDE_ECS_COMPONENT_DEFINE( world, crude_health );
  CRUDE_PARSE_JSON_TO_COMPONENT_FUNC_DEFINE( manager, crude_health );
  CRUDE_PARSE_COMPONENT_TO_JSON_FUNC_DEFINE( manager, crude_health );
  CRUDE_PARSE_COMPONENT_TO_IMGUI_FUNC_DEFINE( manager, crude_health );
  
  CRUDE_ECS_SYSTEM_DEFINE( world, crude_health_game_update_system_, crude_ecs_on_game_update, ctx, {
    { .id = ecs_id( crude_health ) },
  } );

  CRUDE_ECS_OBSERVER_DEFINE( world, crude_health_create_observer, EcsOnSet, ctx, { 
    { .id = ecs_id( crude_health ), .oper = EcsAnd }
  } );

  crude_scene_components_import( world, manager );
}

void
crude_health_game_update_system_
(
  _In_ ecs_iter_t                                         *it
)
{
  CRUDE_PROFILER_ZONE_NAME( "crude_health_game_update_system_" );

  crude_game                                              *game;
  crude_health_system_context                             *ctx;
  crude_health                                            *health_per_entity;

  game = crude_game_instance( );
  ctx = CRUDE_CAST( crude_health_system_context*, it->ctx );
  health_per_entity = ecs_field( it, crude_health, 0 );
  
  for ( uint32 i = 0; i < it->count; ++i )
  {
  }
  CRUDE_PROFILER_ZONE_END;
}

void
crude_health_create_observer
(
  _In_ ecs_iter_t                                         *it
)
{
  CRUDE_PROFILER_ZONE_NAME( "crude_health_create_observer" );

  crude_game                                              *game;
  crude_health_system_context                             *ctx;
  crude_health                                            *health_per_entity;

  game = crude_game_instance( );
  ctx = CRUDE_CAST( crude_health_system_context*, it->ctx );
  health_per_entity = ecs_field( it, crude_health, 0 );
  
  for ( uint32 i = 0; i < it->count; ++i )
  {
    health_per_entity[ i ].total_damage = 0;
  }
  CRUDE_PROFILER_ZONE_END;
}