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

#include <editor/editor.h>

crude_editor                                              *crude_editor_instance_;

static void
crude_editor_deinitialize_constant_strings_
(
  _In_ crude_editor                                       *editor
);

static void
crude_editor_update_input_
(
  _In_ crude_editor                                       *editor
);

static void
crude_editor_input_callback_
(
  _In_ void                                               *ctx,
  _In_ void                                               *sdl_event
);

static void
crude_editor_setup_custom_nodes_to_scene_
( 
  _In_ crude_editor                                       *editor,
  _In_ crude_ecs                                          *world
);

void
crude_editor_initialize
(
  _In_ crude_editor                                       *editor,
  _In_ crude_engine                                       *engine,
  _In_ char const                                         *working_directory
)
{
  crude_ecs                                               *world;
  char                                                     starting_node_absolute_filepath[ 4096 ];

  editor->engine = engine;
  
  world = crude_scene_thread_manager_lock_world( &editor->engine->___scene_thread_manager );
  
  editor->editor_camera_node = CRUDE_COMPOUNT_EMPTY( crude_entity );
  editor->added_node_data = CRUDE_COMPOUNT_EMPTY( crude_devgui_added_node_data );
  editor->node_to_add = CRUDE_COMPOUNT_EMPTY( crude_entity );
  editor->node_to_dublicate = CRUDE_COMPOUNT_EMPTY( crude_entity );
  editor->node_to_remove = CRUDE_COMPOUNT_EMPTY( crude_entity );
  
  editor->free_camera_system_context = CRUDE_COMPOUNT_EMPTY( crude_free_camera_system_context );
  editor->free_camera_system_context.input = crude_scene_thread_manager_get_input_copy_ptr( &editor->engine->___scene_thread_manager );

  crude_free_camera_system_import( world, &editor->free_camera_system_context );
  crude_physics_components_import( world );
  crude_physics_components_import( world );
  crude_scene_debug_components_import( world );
  crude_game_components_import( world );
  
  crude_string_buffer_initialize( &editor->debug_constant_strings_buffer, 4096, crude_heap_allocator_pack( &editor->engine->common_allocator ) );
  crude_string_buffer_initialize( &editor->debug_strings_buffer, 4096, crude_heap_allocator_pack( &editor->engine->common_allocator ) );
  editor->syringe_spawnpoint_debug_model_absolute_filepath = crude_string_buffer_append_use_f( &editor->debug_constant_strings_buffer, "%s%s", editor->engine->environment.directories.resources_absolute_directory, "debug\\models\\syringe_spawnpoint_model.gltf" );
  editor->enemy_spawnpoint_debug_model_absolute_filepath = crude_string_buffer_append_use_f( &editor->debug_constant_strings_buffer, "%s%s", editor->engine->environment.directories.resources_absolute_directory, "debug\\models\\enemy_spawnpoint_model.gltf" );

  editor->physics_debug_system_context = CRUDE_COMPOUNT_EMPTY( crude_physics_debug_system_context );
  editor->physics_debug_system_context.resources_absolute_directory = editor->engine->environment.directories.resources_absolute_directory;
  editor->physics_debug_system_context.string_bufffer = &editor->debug_strings_buffer;
  crude_physics_debug_system_import( world, &editor->physics_debug_system_context );
  
  editor->game_debug_system_context = CRUDE_COMPOUNT_EMPTY( crude_game_debug_system_context );
  editor->game_debug_system_context.enemy_spawnpoint_model_absolute_filepath = editor->enemy_spawnpoint_debug_model_absolute_filepath;
  editor->game_debug_system_context.syringe_spawnpoint_model_absolute_filepath = editor->syringe_spawnpoint_debug_model_absolute_filepath;
  crude_game_debug_system_import( world, &editor->game_debug_system_context );
  
  crude_snprintf( starting_node_absolute_filepath, sizeof( starting_node_absolute_filepath ), "%s\\%s", editor->engine->environment.directories.resources_absolute_directory, "game\\nodes\\level0.crude_node" );
  editor->main_node = crude_node_manager_get_node( &editor->engine->node_manager, starting_node_absolute_filepath, world );

  crude_scene_thread_manager_set_main_node_UNSAFE( &editor->engine->___scene_thread_manager, editor->main_node );
  
  editor->selected_node = editor->main_node;
  
  crude_devgui_initialize( &editor->devgui );

  crude_editor_setup_custom_nodes_to_scene_( editor, world );
  crude_scene_thread_manager_unlock_world( &editor->engine->___scene_thread_manager );
}

void
crude_editor_deinitialize
(
  _In_ crude_editor                                             *editor
)
{
  crude_devgui_deinitialize( &editor->devgui );
  crude_editor_deinitialize_constant_strings_( editor );
}

void
crude_editor_update
(
  _In_ crude_editor                                       *editor
)
{
  crude_editor_update_input_( editor );
}

void
crude_editor_update_input_
(
  _In_ crude_editor                                       *editor
)
{
  ImGui::SetCurrentContext( editor->engine->imgui_context );
    
  crude_devgui_handle_input( &editor->devgui, &editor->engine->platform.input );

  if ( editor->engine->platform.input.mouse.right.current && editor->engine->platform.input.mouse.right.current != editor->engine->platform.input.prev_mouse.right.current )
  {
    SDL_GetMouseState( &editor->engine->last_unrelative_mouse_position.x, &editor->engine->last_unrelative_mouse_position.y );
    crude_platform_hide_cursor( &editor->engine->platform );
  }
  
  if ( !editor->engine->platform.input.mouse.right.current && editor->engine->platform.input.mouse.right.current != editor->engine->platform.input.prev_mouse.right.current )
  {
    SDL_WarpMouseInWindow( editor->engine->platform.sdl_window, editor->engine->last_unrelative_mouse_position.x, editor->engine->last_unrelative_mouse_position.y );
    crude_platform_show_cursor( &editor->engine->platform );
  }
}

void
crude_editor_input_callback_
(
  _In_ void                                               *ctx,
  _In_ void                                               *sdl_event
)
{
  crude_editor *editor = CRUDE_CAST( crude_editor*, ctx );

  ImGui::SetCurrentContext( editor->engine->imgui_context );
  ImGui_ImplSDL3_ProcessEvent( CRUDE_CAST( SDL_Event*, sdl_event ) );
}

bool
crude_editor_parse_json_to_component_
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
crude_editor_parse_all_components_to_json_
( 
  _In_ crude_entity                                        node, 
  _In_ cJSON                                              *node_components_json,
  _In_ crude_node_manager                                 *manager
)
{
}

void
crude_editor_deinitialize_constant_strings_
(
  _In_ crude_editor                                       *editor
)
{
  crude_string_buffer_deinitialize( &editor->constant_strings_buffer );
  crude_string_buffer_deinitialize( &editor->dynamic_strings_buffer );
  crude_string_buffer_deinitialize( &editor->debug_strings_buffer );
  crude_string_buffer_deinitialize( &editor->debug_constant_strings_buffer );
}

void
crude_editor_setup_custom_nodes_to_scene_
( 
  _In_ crude_editor                                       *editor,
  _In_ crude_ecs                                          *world
)
{
  {
    crude_transform                                        editor_camera_node_transform;
    crude_camera                                           editor_camera_node_camera;
    crude_free_camera                                      editor_camera_node_crude_free_camera;

    if ( crude_entity_valid( world, editor->editor_camera_node ) )
    {
      crude_entity_destroy( world, editor->editor_camera_node );
    }

    editor_camera_node_transform = CRUDE_COMPOUNT_EMPTY( crude_transform );
    XMStoreFloat4( &editor_camera_node_transform.rotation, XMQuaternionIdentity( ) );
    XMStoreFloat3( &editor_camera_node_transform.scale, XMVectorSplatOne( ) );
    XMStoreFloat3( &editor_camera_node_transform.translation, XMVectorZero( ) );

    editor_camera_node_camera = CRUDE_COMPOUNT_EMPTY( crude_camera );
    editor_camera_node_camera.fov_radians = 1;
    editor_camera_node_camera.aspect_ratio = 1.8;
    editor_camera_node_camera.near_z = 0.001;
    editor_camera_node_camera.far_z = 1000;

    editor_camera_node_crude_free_camera = CRUDE_COMPOUNT_EMPTY( crude_free_camera );
    editor_camera_node_crude_free_camera.moving_speed_multiplier = 10.f;
    editor_camera_node_crude_free_camera.rotating_speed_multiplier = -0.004f;
    editor_camera_node_crude_free_camera.input_enabled = true;
    editor_camera_node_crude_free_camera.input = crude_scene_thread_manager_get_input_copy_ptr( &editor->engine->___scene_thread_manager );

    editor->editor_camera_node = crude_entity_create_empty( world, "editor_camera" );
    CRUDE_ENTITY_SET_COMPONENT( world, editor->editor_camera_node, crude_transform, { editor_camera_node_transform } );
    CRUDE_ENTITY_SET_COMPONENT( world, editor->editor_camera_node, crude_camera, { editor_camera_node_camera } );
    CRUDE_ENTITY_SET_COMPONENT( world, editor->editor_camera_node, crude_free_camera, { editor_camera_node_crude_free_camera } );
  }

  editor->editor_camera_node = crude_ecs_lookup_entity_from_parent( world, editor->main_node, "editor_camera" );

  crude_scene_thread_manager_set_camera_node_UNSAFE( &editor->engine->___scene_thread_manager, editor->editor_camera_node );
}

void
crude_editor_instance_intialize
(
)
{
  crude_editor_instance_ = CRUDE_CAST( crude_editor*, malloc( sizeof( crude_editor ) ) );
}

void
crude_editor_instance_deintialize
(
)
{
  free( crude_editor_instance_ );
}

crude_editor*
crude_editor_instance
(
)
{
  return crude_editor_instance_;
}