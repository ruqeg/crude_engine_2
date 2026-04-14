#include <engine/core/log.h>
#include <engine/core/profiler.h>
#include <engine/scene/scene_ecs.h>
#include <engine/platform/platform.h>
#include <engine/graphics/imgui.h>
#include <game/game.h>

#include <game/maze_level_ecs.h>

/**********************************************************
 *
 *                 Component
 *
 *********************************************************/
ECS_COMPONENT_DECLARE( crude_maze_level );
CRUDE_COMPONENT_STRING_DEFINE( crude_maze_level, "crude_maze_level" );

CRUDE_PARSE_JSON_TO_COMPONENT_FUNC_IMPLEMENTATION( crude_maze_level )
{
  crude_memory_set( component, 0, sizeof( crude_maze_level ) );
  return true;
}

CRUDE_PARSE_COMPONENT_TO_JSON_FUNC_IMPLEMENTATION( crude_maze_level )
{
  cJSON *free_camera_json = cJSON_CreateObject( );
  cJSON_AddItemToObject( free_camera_json, "type", cJSON_CreateString( CRUDE_COMPONENT_STRING( crude_maze_level ) ) );
  return free_camera_json;
}

CRUDE_PARSE_COMPONENT_TO_IMGUI_FUNC_IMPLEMENTATION( crude_maze_level )
{
  CRUDE_IMGUI_START_OPTIONS;
}

/**********************************************************
 *
 *                 System
 *
 *********************************************************/
CRUDE_ECS_OBSERVER_DECLARE( crude_maze_level_create_observer_ );
CRUDE_ECS_SYSTEM_DECLARE( crude_maze_level_update_system_ );

void
crude_maze_level_system_import
(
  _In_ crude_ecs                                          *world,
  _In_ crude_components_serialization_manager             *manager,
  _In_ crude_maze_level_system_context                    *ctx
)
{
  CRUDE_ECS_MODULE( world, crude_maze_level_system );
  
  CRUDE_ECS_COMPONENT_DEFINE( world, crude_maze_level );
  CRUDE_PARSE_JSON_TO_COMPONENT_FUNC_DEFINE( manager, crude_maze_level );
  CRUDE_PARSE_COMPONENT_TO_JSON_FUNC_DEFINE( manager, crude_maze_level );
  CRUDE_PARSE_COMPONENT_TO_IMGUI_FUNC_DEFINE( manager, crude_maze_level );

  crude_scene_components_import( world, manager );

  CRUDE_ECS_SYSTEM_DEFINE( world, crude_maze_level_update_system_, crude_ecs_on_game_update, ctx, {
    { .id = ecs_id( crude_maze_level ) },
  } );

  CRUDE_ECS_OBSERVER_DEFINE( world, crude_maze_level_create_observer_, EcsOnSet, ctx, { 
    { .id = ecs_id( crude_maze_level ), .oper = EcsAnd }
  } );
}

void
crude_maze_level_create_observer_
(
  _In_ ecs_iter_t                                         *it
)
{
  CRUDE_PROFILER_ZONE_NAME( "crude_maze_level_create_observer_" );

  crude_game                                     *game;
  crude_maze_level_system_context                *ctx;
  crude_maze_level                               *maze_area_level_per_entity;

  game = crude_game_instance( );
  ctx = CRUDE_CAST( crude_maze_level_system_context*, it->ctx );
  maze_area_level_per_entity = ecs_field( it, crude_maze_level, 0 );
  
  for ( uint32 i = 0; i < it->count; ++i )
  {
    crude_input const                                     *input;
    crude_maze_level                                      *maze_area_level;
    crude_player_controller                               *player_controller;
    crude_audio_player_handle                             *ambient_sound_player_handle;
    crude_entity                                           maze_area_level_entity;
    crude_entity                                           player_entity;
    crude_entity                                           player_spawnpoint_entity;
    crude_entity                                           ambient_sound_player_entity;
    
    input = ctx->input;
    
    maze_area_level_entity = crude_entity_from_iterator( it, i );
    
    maze_area_level = &maze_area_level_per_entity[ i ];
    
    player_spawnpoint_entity = crude_ecs_lookup_entity_from_parent( it->world, maze_area_level_entity, "player_spawnpoint" );
    player_entity = crude_ecs_lookup_entity_from_parent( it->world, player_spawnpoint_entity, "player" );
    player_controller = CRUDE_ENTITY_GET_MUTABLE_COMPONENT( it->world, player_entity, crude_player_controller );
    player_controller->input_enabled = true;
    player_controller->camera_enabled = true;

    if ( CRUDE_ECS_GAME_STAGE_IS_ENABLED( it->world ) )
    {
      ambient_sound_player_entity = crude_ecs_lookup_entity_from_parent( it->world, maze_area_level_entity, "ambient_sound" );
      ambient_sound_player_handle = CRUDE_ENTITY_GET_MUTABLE_COMPONENT( it->world, ambient_sound_player_entity, crude_audio_player_handle );
      crude_audio_device_sound_reset( &game->engine->audio_device, ambient_sound_player_handle->sound_handle );
      crude_audio_device_sound_start( &game->engine->audio_device, ambient_sound_player_handle->sound_handle );
    }

    {
      crude_physics_kinematic_body_container              *key_sensor_kinematic_body_container;
      crude_entity                                         key_sensor_entity;
      crude_physics_kinematic_body_handle                  key_sensor_kinematic_body_handle;

      key_sensor_entity = crude_ecs_lookup_entity_from_parent( it->world, maze_area_level_entity, "key.sensor" );
      key_sensor_kinematic_body_handle = *CRUDE_ENTITY_GET_IMMUTABLE_COMPONENT( it->world, key_sensor_entity, crude_physics_kinematic_body_handle );
      key_sensor_kinematic_body_container = crude_physics_access_kinematic_body( &game->engine->physics, key_sensor_kinematic_body_handle );
      key_sensor_kinematic_body_container->contact_added_callback = crude_key_sensor_callback_;
    }
  }
  CRUDE_PROFILER_ZONE_END;
}

void
crude_maze_level_update_system_
(
  _In_ ecs_iter_t                                         *it
)
{
  CRUDE_PROFILER_ZONE_NAME( "crude_maze_level_update_system" );

  crude_game                                              *game;
  crude_maze_level_system_context                *ctx;
  crude_maze_level                               *training_area_level_per_entity;

  game = crude_game_instance( );
  ctx = CRUDE_CAST( crude_maze_level_system_context*, it->ctx );
  training_area_level_per_entity = ecs_field( it, crude_maze_level, 0 );
  
  for ( uint32 i = 0; i < it->count; ++i )
  {
    crude_input const                                     *input;
    crude_maze_level                                      *training_area_level;
    crude_player_controller                               *player_controller;
    crude_gltf                                            *player_model;
    crude_entity                                           entity;
    crude_entity                                           player_spawnpoint_entity;
    crude_entity                                           player_entity;

    input = ctx->input;

    entity = crude_entity_from_iterator( it, i );

    training_area_level = &training_area_level_per_entity[ i ];

    player_spawnpoint_entity = crude_ecs_lookup_entity_from_parent( it->world, entity, "player_spawnpoint" );
    player_entity = crude_ecs_lookup_entity_from_parent( it->world, player_spawnpoint_entity, "player" );
    player_controller = CRUDE_ENTITY_GET_MUTABLE_COMPONENT( it->world, player_entity, crude_player_controller );
    player_controller->input_enabled = true;
    player_controller->camera_enabled = true;
  }
  CRUDE_PROFILER_ZONE_END;
}


void
crude_key_sensor_callback_
(
  _In_ crude_entity                                        signal_entity,
  _In_ crude_entity                                        hitted_entity
)
{
  crude_game                                              *game;
  crude_audio_player_handle                               *end_audio_player_handle;
  crude_entity                                             end_audio_player_entity;

  game = crude_game_instance( );

  end_audio_player_entity = crude_ecs_lookup_entity_from_parent( game->engine->world, crude_entity_get_parent( game->engine->world, end_audio_player_entity ), "key_sound" ); 
  end_audio_player_handle = CRUDE_ENTITY_GET_MUTABLE_COMPONENT( game->engine->world, end_audio_player_entity, crude_audio_player_handle );
  crude_audio_device_sound_reset( &game->engine->audio_device, end_audio_player_handle->sound_handle );
  crude_audio_device_sound_start( &game->engine->audio_device, end_audio_player_handle->sound_handle );
}