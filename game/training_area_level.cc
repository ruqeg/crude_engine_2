#include <engine/core/log.h>
#include <engine/core/profiler.h>
#include <engine/scene/scene_ecs.h>
#include <engine/platform/platform.h>
#include <engine/graphics/imgui.h>
#include <game/game.h>

#include <game/training_area_level.h>

/**********************************************************
 *
 *                 Component
 *
 *********************************************************/
ECS_COMPONENT_DECLARE( crude_training_area_level );
CRUDE_COMPONENT_STRING_DEFINE( crude_training_area_level, "crude_training_area_level" );

CRUDE_PARSE_JSON_TO_COMPONENT_FUNC_IMPLEMENTATION( crude_training_area_level )
{
  crude_memory_set( component, 0, sizeof( crude_training_area_level ) );
  return true;
}

CRUDE_PARSE_COMPONENT_TO_JSON_FUNC_IMPLEMENTATION( crude_training_area_level )
{
  cJSON *free_camera_json = cJSON_CreateObject( );
  cJSON_AddItemToObject( free_camera_json, "type", cJSON_CreateString( CRUDE_COMPONENT_STRING( crude_training_area_level ) ) );
  return free_camera_json;
}

CRUDE_PARSE_COMPONENT_TO_IMGUI_FUNC_IMPLEMENTATION( crude_training_area_level )
{
  CRUDE_IMGUI_START_OPTIONS;
}

/**********************************************************
 *
 *                 System
 *
 *********************************************************/
CRUDE_ECS_SYSTEM_DECLARE( crude_training_area_level_update_system_ );

void
crude_training_area_level_update_system_
(
  _In_ ecs_iter_t                                         *it
)
{
  CRUDE_PROFILER_ZONE_NAME( "crude_training_area_level_update_system" );

  crude_game                                              *game;
  crude_training_area_level_system_context                *ctx;
  crude_training_area_level                               *training_area_level_per_entity;

  game = crude_game_instance( );
  ctx = CRUDE_CAST( crude_training_area_level_system_context*, it->ctx );
  training_area_level_per_entity = ecs_field( it, crude_training_area_level, 0 );
  
  for ( uint32 i = 0; i < it->count; ++i )
  {
    crude_input const                                     *input;
    crude_training_area_level                             *training_area_level;
    crude_player_controller                               *player_controller;
    crude_gltf                                            *player_model;
    crude_entity                                           entity;
    crude_entity                                           player_entity;
    crude_entity                                           player_model_entity;

    input = ctx->input;

    entity = crude_entity_from_iterator( it, i );

    training_area_level = &training_area_level_per_entity[ i ];

    player_entity = crude_ecs_lookup_entity_from_parent( it->world, entity, "player" );
    player_controller = CRUDE_ENTITY_GET_MUTABLE_COMPONENT( it->world, player_entity, crude_player_controller );
    player_controller->input_enabled = true;
    player_controller->camera_enabled = true;

    player_model_entity = crude_ecs_lookup_entity_from_parent( it->world, player_entity, "model" );
    player_model = CRUDE_ENTITY_GET_MUTABLE_COMPONENT( it->world, player_model_entity, crude_gltf );
    crude_gfx_model_renderer_resources_instance_set_animation_by_name( &player_model->model_renderer_resources_instance, &game->engine->model_renderer_resources_manager, "Armature|mixamo.com|Layer0" ); 
    player_model->model_renderer_resources_instance.animation_instance.loop = true;
  }
  CRUDE_PROFILER_ZONE_END;
}

void
crude_training_area_level_system_import
(
  _In_ crude_ecs                                          *world,
  _In_ crude_components_serialization_manager             *manager,
  _In_ crude_training_area_level_system_context           *ctx
)
{
  CRUDE_ECS_MODULE( world, crude_training_area_level_system );
  
  CRUDE_ECS_COMPONENT_DEFINE( world, crude_training_area_level );
  CRUDE_PARSE_JSON_TO_COMPONENT_FUNC_DEFINE( manager, crude_training_area_level );
  CRUDE_PARSE_COMPONENT_TO_JSON_FUNC_DEFINE( manager, crude_training_area_level );
  CRUDE_PARSE_COMPONENT_TO_IMGUI_FUNC_DEFINE( manager, crude_training_area_level );

  crude_scene_components_import( world, manager );

  CRUDE_ECS_SYSTEM_DEFINE( world, crude_training_area_level_update_system_, crude_ecs_on_game_update, ctx, {
    { .id = ecs_id( crude_training_area_level ) },
  } );
}