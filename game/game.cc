#include <engine/core/hash_map.h>
#include <engine/core/file.h>
#include <engine/core/memory.h>
#include <engine/core/process.h>
#include <engine/scene/scripts/free_camera_ecs.h>
#include <engine/scene/scene_ecs.h>
#include <engine/scene/scene_debug_ecs.h>
#include <engine/graphics/gpu_resources_loader.h>
#include <engine/physics/physics_ecs.h>
#include <engine/physics/physics_debug_ecs.h>
#include <engine/external/game_ecs.h>
#include <game/level_mars_ecs.h>

#include <game/game.h>

crude_game                                                *crude_game_instance_;

static void
crude_game_deinitialize_constant_strings_
(
  _In_ crude_game                                         *editor
);

static void
crude_game_input_callback_
(
  _In_ void                                               *ctx,
  _In_ void                                               *sdl_event
);

static void
crude_game_setup_custom_nodes_to_scene_
( 
  _In_ crude_game                                         *game,
  _In_ crude_ecs                                          *world
);

void
crude_game_update_input_
(
  _In_ crude_game                                         *game
);

void
crude_game_imgui_custom_draw
(
  _In_ void                                               *ctx,
  _In_ crude_ecs                                          *world
);

void
crude_game_initialize
(
  _In_ crude_game                                         *game,
  _In_ crude_engine                                       *engine,
  _In_ char const                                         *working_directory
)
{
  crude_ecs                                               *world;
  char                                                     starting_node_absolute_filepath[ 4096 ];

  game->engine = engine;
  
  world = crude_scene_thread_manager_lock_world( &game->engine->___scene_thread_manager );
  
  
  game->free_camera_system_context = CRUDE_COMPOUNT_EMPTY( crude_free_camera_system_context );
  game->free_camera_system_context.input = crude_scene_thread_manager_get_input_copy_ptr( &game->engine->___scene_thread_manager );
  crude_free_camera_system_import( world, &game->free_camera_system_context );
  crude_physics_components_import( world );
  crude_physics_components_import( world );
  crude_scene_debug_components_import( world );
  crude_game_components_import( world );
  game->level_mars_system_context = CRUDE_COMPOUNT_EMPTY( crude_level_mars_system_context );
  crude_level_mars_system_import( world, &game->level_mars_system_context );
  
  crude_string_buffer_initialize( &game->debug_constant_strings_buffer, 4096, crude_heap_allocator_pack( &game->engine->common_allocator ) );
  crude_string_buffer_initialize( &game->debug_strings_buffer, 4096, crude_heap_allocator_pack( &game->engine->common_allocator ) );

  game->physics_debug_system_context = CRUDE_COMPOUNT_EMPTY( crude_physics_debug_system_context );
  game->physics_debug_system_context.resources_absolute_directory = game->engine->environment.directories.resources_absolute_directory;
  game->physics_debug_system_context.string_bufffer = &game->debug_strings_buffer;
  crude_physics_debug_system_import( world, &game->physics_debug_system_context );
  
  crude_snprintf( starting_node_absolute_filepath, sizeof( starting_node_absolute_filepath ), "%s\\%s", game->engine->environment.directories.resources_absolute_directory, "game\\nodes\\level_mars.crude_node" );
  game->main_node = crude_node_manager_get_node( &game->engine->node_manager, starting_node_absolute_filepath, world );

  crude_scene_thread_manager_set_main_node_UNSAFE( &game->engine->___scene_thread_manager, game->main_node );
  
  crude_scene_thread_manager_unlock_world( &game->engine->___scene_thread_manager );

  crude_graphics_thread_manager_set_imgui_custom_draw( &game->engine->___graphics_thread_manager, crude_game_imgui_custom_draw, game );
}

void
crude_game_imgui_custom_draw
(
  _In_ void                                               *ctx,
  _In_ crude_ecs                                          *world
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

void
crude_game_input_callback_
(
  _In_ void                                               *ctx,
  _In_ void                                               *sdl_event
)
{
  crude_game *game = CRUDE_CAST( crude_game*, ctx );
  
  mtx_lock( &game->engine->imgui_mutex );
  ImGui::SetCurrentContext( game->engine->imgui_context );
  ImGui_ImplSDL3_ProcessEvent( CRUDE_CAST( SDL_Event*, sdl_event ) );
  mtx_unlock( &game->engine->imgui_mutex );
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
  _In_ crude_game                                         *game
)
{
  crude_string_buffer_deinitialize( &game->debug_strings_buffer );
  crude_string_buffer_deinitialize( &game->debug_constant_strings_buffer );
}

void
crude_game_setup_custom_nodes_to_scene_
( 
  _In_ crude_game                                         *game,
  _In_ crude_ecs                                          *world
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