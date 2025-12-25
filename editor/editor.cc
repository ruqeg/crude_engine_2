#include <engine/core/hash_map.h>
#include <engine/core/file.h>
#include <engine/core/memory.h>
#include <engine/core/process.h>
#include <engine/scene/free_camera_system.h>
#include <engine/scene/scene_components.h>
#include <engine/scene/scripts_components.h>
#include <engine/graphics/gpu_resources_loader.h>
#include <engine/physics/physics_components.h>
#include <engine/physics/physics_debug_system.h>
#include <engine/external/game_components.h>

#include <editor/editor.h>

crude_editor                                              *crude_editor_instance_;

CRUDE_ECS_SYSTEM_DECLARE( crude_editor_input_system_ );


static void
crude_editor_initialize_constant_strings_
(
  _In_ crude_editor                                       *editor,
  _In_ char const                                         *scene_relative_filepath,
  _In_ char const                                         *render_graph_relative_directory,
  _In_ char const                                         *resources_relative_directory,
  _In_ char const                                         *shaders_relative_directory,
  _In_ char const                                         *techniques_relative_directory,
  _In_ char const                                         *compiled_shaders_relative_directory,
  _In_ char const                                         *working_absolute_directory
);

static void
crude_editor_deinitialize_constant_strings_
(
  _In_ crude_editor                                       *editor
);

static void
crude_editor_input_system_
(
  _In_ ecs_iter_t                                         *it
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
  _In_ crude_editor                                       *editor
);

void
crude_editor_initialize
(
  _In_ crude_editor                                       *editor,
  _In_ crude_engine                                       *engine
)
{
  editor->engine = engine;

  editor->editor_camera_node = CRUDE_COMPOUNT_EMPTY( crude_entity );
  editor->added_node_data = CRUDE_COMPOUNT_EMPTY( crude_devgui_added_node_data );
  editor->node_to_add = CRUDE_COMPOUNT_EMPTY( crude_entity );
  editor->node_to_dublicate = CRUDE_COMPOUNT_EMPTY( crude_entity );
  editor->node_to_remove = CRUDE_COMPOUNT_EMPTY( crude_entity );

  ECS_IMPORT( editor->engine->world, crude_free_camera_system );
  ECS_IMPORT( editor->engine->world, crude_game_components );
  ECS_IMPORT( editor->engine->world, crude_physics_components );

  crude_editor_initialize_constant_strings_( editor, creation->scene_relative_filepath, creation->render_graph_relative_directory, creation->resources_relative_directory, creation->shaders_relative_directory, creation->techniques_relative_directory, creation->compiled_shaders_relative_directory, creation->working_absolute_directory );

  editor->physics_debug_system_context = CRUDE_COMPOUNT_EMPTY( crude_physics_debug_system_context );
  editor->physics_debug_system_context.resources_absolute_directory = editor->resources_absolute_directory;
  editor->physics_debug_system_context.string_bufffer = &editor->debug_strings_buffer;
  crude_physics_debug_system_import( editor->engine->world, &editor->physics_debug_system_context );
  
  editor->game_debug_system_context = CRUDE_COMPOUNT_EMPTY( crude_game_debug_system_context );
  editor->game_debug_system_context.enemy_spawnpoint_model_absolute_filepath = editor->enemy_spawnpoint_debug_model_absolute_filepath;
  editor->game_debug_system_context.syringe_spawnpoint_model_absolute_filepath = editor->syringe_spawnpoint_debug_model_absolute_filepath;
  crude_game_debug_system_import( editor->engine->world, &editor->game_debug_system_context );
  
  editor->main_node = crude_node_manager_get_node( &editor->engine->node_manager, editor->scene_absolute_filepath );
  editor->selected_node = editor->main_node;
  
  crude_devgui_initialize( &editor->devgui );
  
  CRUDE_ECS_SYSTEM_DEFINE( editor->engine->world, crude_editor_input_system_, EcsOnUpdate, editor, {} );

  crude_editor_setup_custom_nodes_to_scene_( editor );
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
crude_editor_input_system_
(
  _In_ ecs_iter_t                                         *it
)
{
  crude_editor *editor = ( crude_editor* )it->ctx;

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
crude_editor_initialize_constant_strings_
(
  _In_ crude_editor                                       *editor,
  _In_ char const                                         *scene_relative_filepath,
  _In_ char const                                         *render_graph_relative_directory,
  _In_ char const                                         *resources_relative_directory,
  _In_ char const                                         *shaders_relative_directory,
  _In_ char const                                         *techniques_relative_directory,
  _In_ char const                                         *compiled_shaders_relative_directory,
  _In_ char const                                         *working_absolute_directory
)
{
  uint64 working_directory_length = crude_string_length( working_absolute_directory ) + 1;
  uint64 resources_directory_length = working_directory_length + crude_string_length( resources_relative_directory );
  uint64 shaders_directory_length = working_directory_length + crude_string_length( shaders_relative_directory );
  uint64 render_graph_directory_length = working_directory_length + crude_string_length( render_graph_relative_directory );
  uint64 scene_filepath_length = working_directory_length + crude_string_length( scene_relative_filepath );
  uint64 techniques_relative_directory_length = working_directory_length + crude_string_length( techniques_relative_directory );
  uint64 compiled_shaders_relative_directory_length = working_directory_length + crude_string_length( compiled_shaders_relative_directory );

  uint64 constant_string_buffer_size = working_directory_length + resources_directory_length + shaders_directory_length + render_graph_directory_length + scene_filepath_length + techniques_relative_directory_length + compiled_shaders_relative_directory_length;
  
  crude_string_buffer_initialize( &editor->dynamic_strings_buffer, 4096, crude_heap_allocator_pack( &editor->engine->common_allocator ) );
  crude_string_buffer_initialize( &editor->debug_strings_buffer, 4096, crude_heap_allocator_pack( &editor->engine->common_allocator ) );
  crude_string_buffer_initialize( &editor->debug_constant_strings_buffer, 4096, crude_heap_allocator_pack( &editor->engine->common_allocator ) );

  crude_string_buffer_initialize( &editor->constant_strings_buffer, constant_string_buffer_size, crude_heap_allocator_pack( &editor->engine->common_allocator ) );
  editor->working_absolute_directory = crude_string_buffer_append_use_f( &editor->constant_strings_buffer, "%s", working_absolute_directory );
  editor->resources_absolute_directory = crude_string_buffer_append_use_f( &editor->constant_strings_buffer, "%s%s", editor->working_absolute_directory, resources_relative_directory );
  editor->shaders_absolute_directory = crude_string_buffer_append_use_f( &editor->constant_strings_buffer, "%s%s", editor->working_absolute_directory, shaders_relative_directory );
  editor->scene_absolute_filepath = crude_string_buffer_append_use_f( &editor->constant_strings_buffer, "%s%s", editor->working_absolute_directory, scene_relative_filepath );
  editor->render_graph_absolute_directory = crude_string_buffer_append_use_f( &editor->constant_strings_buffer, "%s%s", editor->working_absolute_directory, render_graph_relative_directory );
  editor->techniques_absolute_directory = crude_string_buffer_append_use_f( &editor->constant_strings_buffer, "%s%s", editor->working_absolute_directory, techniques_relative_directory );
  editor->compiled_shaders_absolute_directory = crude_string_buffer_append_use_f( &editor->constant_strings_buffer, "%s%s", editor->working_absolute_directory, compiled_shaders_relative_directory );

  editor->syringe_spawnpoint_debug_model_absolute_filepath = crude_string_buffer_append_use_f( &editor->debug_constant_strings_buffer, "%s%s", editor->resources_absolute_directory, "debug\\models\\syringe_spawnpoint_model.gltf" );
  editor->enemy_spawnpoint_debug_model_absolute_filepath = crude_string_buffer_append_use_f( &editor->debug_constant_strings_buffer, "%s%s", editor->resources_absolute_directory, "debug\\models\\enemy_spawnpoint_model.gltf" );
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
  _In_ crude_editor                                       *editor
)
{
  {
    crude_transform                                        editor_camera_node_transform;
    crude_camera                                           editor_camera_node_camera;
    crude_free_camera                                      editor_camera_node_crude_free_camera;

    if ( crude_entity_valid( editor->editor_camera_node ) )
    {
      crude_entity_destroy( editor->editor_camera_node );
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
    editor_camera_node_crude_free_camera.platformn = &editor->engine->platform;

    editor->editor_camera_node = crude_entity_create_empty( editor->engine->world, "editor_camera" );
    CRUDE_ENTITY_SET_COMPONENT( editor->editor_camera_node, crude_transform, { editor_camera_node_transform } );
    CRUDE_ENTITY_SET_COMPONENT( editor->editor_camera_node, crude_camera, { editor_camera_node_camera } );
    CRUDE_ENTITY_SET_COMPONENT( editor->editor_camera_node, crude_free_camera, { editor_camera_node_crude_free_camera } );
  }

  editor->editor_camera_node = crude_ecs_lookup_entity_from_parent( editor->main_node, "editor_camera" );
  editor->focused_camera_node = editor->editor_camera_node;
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