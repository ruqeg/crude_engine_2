#include <engine/core/hashmapstr.h>
#include <engine/core/file.h>
#include <engine/core/memory.h>
#include <engine/core/process.h>
#include <engine/scene/scripts/free_camera_ecs.h>
#include <engine/scene/scene_ecs.h>
#include <engine/scene/scene_debug_ecs.h>
#include <engine/graphics/gpu_resources_loader.h>
#include <engine/physics/physics_ecs.h>

#include <game/game.h>

crude_game                                              *crude_game_instance_;

void
crude_game_update_input_
(
  _In_ crude_game                                         *game
);

void
crude_game_imgui_custom_draw
(
  _In_ void                                               *ctx
);

void
crude_game_initialize
(
  _In_ crude_game                                         *game,
  _In_ crude_engine                                       *engine,
  _In_ char const                                         *working_directory
)
{
  game->engine = engine;
  
  CRUDE_ECS_GAME_STAGE_ENABLE( engine->world, false );

  game->player_controller_system_context = CRUDE_COMPOUNT_EMPTY( crude_player_controller_system_context );
  game->player_controller_system_context.input = &engine->platform.input;
  game->player_controller_system_context.physics_manager = &engine->physics;
  crude_player_controller_system_import( engine->world, &engine->components_serialization_manager, &game->player_controller_system_context );
  
  game->player_system_context = CRUDE_COMPOUNT_EMPTY( crude_player_system_context );
  crude_player_system_import( engine->world, &engine->components_serialization_manager, &game->player_system_context );

  game->weapon_system_context = CRUDE_COMPOUNT_EMPTY( crude_weapon_system_context );
  crude_weapon_system_import( engine->world, &engine->components_serialization_manager, &game->weapon_system_context );

  game->zombie_system_context = CRUDE_COMPOUNT_EMPTY( crude_zombie_system_context );
  crude_zombie_system_import( engine->world, &engine->components_serialization_manager, &game->zombie_system_context );

  game->health_system_context = CRUDE_COMPOUNT_EMPTY( crude_health_system_context );
  crude_health_system_import( engine->world, &engine->components_serialization_manager, &game->health_system_context );
  
  game->training_area_level_system_context = CRUDE_COMPOUNT_EMPTY( crude_training_area_level_system_context );
  game->training_area_level_system_context.input = &engine->platform.input;
  crude_training_area_level_system_import( engine->world, &engine->components_serialization_manager, &game->training_area_level_system_context );
  
  game->maze_level_system_context = CRUDE_COMPOUNT_EMPTY( crude_maze_level_system_context );
  game->maze_level_system_context.input = &engine->platform.input;
  crude_maze_level_system_import( engine->world, &engine->components_serialization_manager, &game->maze_level_system_context );

  crude_engine_commands_manager_push_load_node_command( &game->engine->commands_manager, "game\\rb9\\nodes\\player.crude_node" );
  crude_engine_commands_manager_update( &engine->commands_manager );
  
  game->engine->imgui_draw_custom_fn = crude_game_imgui_custom_draw;
  game->engine->imgui_draw_custom_ctx = game;
}

void
crude_game_imgui_custom_draw
(
  _In_ void                                               *ctx
)
{
}

void
crude_game_deinitialize
(
  _In_ crude_game                                         *game
)
{
}

void
crude_game_update
(
  _In_ crude_game                                         *game
)
{
  crude_game_update_input_( game );
}

void
crude_game_update_input_
(
  _In_ crude_game                                         *game
)
{
}

void
crude_game_instance_intialize
(
)
{
  crude_game_instance_ = CRUDE_CAST( crude_game*, malloc( sizeof( crude_game ) ) );
}

void
crude_game_instance_deintialize
(
)
{
  free( crude_game_instance_ );
}

crude_game*
crude_game_instance
(
)
{
  return crude_game_instance_;
}