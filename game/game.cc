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

static void
crude_game_deinitialize_constant_strings_
(
  _In_ crude_game                                         *game
);

static void
crude_game_setup_custom_nodes_to_scene_
( 
  _In_ crude_game                                         *game
);

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
  
  crude_string_buffer_initialize( &game->debug_constant_strings_buffer, 4096, crude_heap_allocator_pack( &game->engine->common_allocator ) );
  crude_string_buffer_initialize( &game->debug_strings_buffer, 4096, crude_heap_allocator_pack( &game->engine->common_allocator ) );

  game->player_controller_system_context = CRUDE_COMPOUNT_EMPTY( crude_player_controller_system_context );
  game->player_controller_system_context.input = &engine->platform.input;
  game->player_controller_system_context.physics_manager = &engine->physics;
  crude_player_controller_system_import( engine->world, &engine->components_serialization_manager, &game->player_controller_system_context );
  
  game->training_area_level_system_context = CRUDE_COMPOUNT_EMPTY( crude_training_area_level_system_context );
  game->training_area_level_system_context.input = &engine->platform.input;
  crude_training_area_level_system_import( engine->world, &engine->components_serialization_manager, &game->training_area_level_system_context );

  crude_engine_commands_manager_push_load_node_command( &game->engine->commands_manager, "game\\rb9\\nodes\\player.crude_node" );
  crude_engine_commands_manager_update( &engine->commands_manager );

  crude_game_setup_custom_nodes_to_scene_( game );

  game->engine->imgui_draw_custom_fn = crude_game_imgui_custom_draw;
  game->engine->imgui_draw_custom_ctx = game;

  CRUDE_ECS_GAME_STAGE_ENABLE( engine->world, false );
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
  crude_game_deinitialize_constant_strings_( game );
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

bool
crude_game_parse_json_to_component_
( 
  _In_ crude_entity                                        node, 
  _In_ cJSON const                                        *component_json,
  _In_ char const                                         *component_name,
  _In_ crude_node_manager                                 *manager
)
{
  return true;
}

void
crude_game_parse_all_components_to_json_
( 
  _In_ crude_entity                                        node, 
  _In_ cJSON                                              *node_components_json,
  _In_ crude_node_manager                                 *manager
)
{
}

void
crude_game_deinitialize_constant_strings_
(
  _In_ crude_game                                       *editor
)
{
  crude_string_buffer_deinitialize( &editor->debug_strings_buffer );
  crude_string_buffer_deinitialize( &editor->debug_constant_strings_buffer );
}

void
crude_game_setup_custom_nodes_to_scene_
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