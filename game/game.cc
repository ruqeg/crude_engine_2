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

#include <game/game.h>

crude_editor                                              *crude_editor_instance_;

static void
crude_editor_deinitialize_constant_strings_
(
  _In_ crude_editor                                       *editor
);

static void
crude_editor_setup_custom_nodes_to_scene_
( 
  _In_ crude_editor                                       *editor
);

void
crude_editor_update_input_
(
  _In_ crude_editor                                       *editor
);

void
crude_editor_imgui_custom_draw
(
  _In_ void                                               *ctx
);

void
crude_editor_initialize
(
  _In_ crude_editor                                       *editor,
  _In_ crude_engine                                       *engine,
  _In_ char const                                         *working_directory
)
{
  char                                                     starting_node_absolute_filepath[ 4096 ];

  editor->engine = engine;
  
  editor->editor_camera_node = CRUDE_COMPOUNT_EMPTY( crude_entity );
  
  editor->free_camera_system_context = CRUDE_COMPOUNT_EMPTY( crude_free_camera_system_context );
  editor->free_camera_system_context.input = &editor->engine->platform.input;

  crude_free_camera_system_import( engine->world, &engine->components_serialization_manager, &editor->free_camera_system_context );
  crude_physics_components_import( engine->world, &engine->components_serialization_manager );
  crude_physics_components_import( engine->world, &engine->components_serialization_manager );
  crude_scene_debug_components_import( engine->world, &engine->components_serialization_manager );
  crude_game_components_import( engine->world, &engine->components_serialization_manager );
  
  crude_string_buffer_initialize( &editor->debug_constant_strings_buffer, 4096, crude_heap_allocator_pack( &editor->engine->common_allocator ) );
  crude_string_buffer_initialize( &editor->debug_strings_buffer, 4096, crude_heap_allocator_pack( &editor->engine->common_allocator ) );

  editor->physics_debug_system_context = CRUDE_COMPOUNT_EMPTY( crude_physics_debug_system_context );
  editor->physics_debug_system_context.resources_absolute_directory = editor->engine->environment.directories.resources_absolute_directory;
  editor->physics_debug_system_context.string_bufffer = &editor->debug_strings_buffer;
  crude_physics_debug_system_import( engine->world, &engine->components_serialization_manager, &editor->physics_debug_system_context );
  
  editor->game_debug_system_context = CRUDE_COMPOUNT_EMPTY( crude_game_debug_system_context );
  crude_game_debug_system_import( engine->world, &editor->game_debug_system_context );
  
  crude_snprintf( starting_node_absolute_filepath, sizeof( starting_node_absolute_filepath ), "%s\\%s", editor->engine->environment.directories.resources_absolute_directory, "game\\nodes\\level_mars.crude_node" );
  editor->main_node = crude_node_manager_get_node( &editor->engine->node_manager, starting_node_absolute_filepath, engine->world );

  editor->engine->main_node = editor->main_node;
  crude_editor_setup_custom_nodes_to_scene_( editor );

  editor->engine->imgui_draw_custom_fn = crude_editor_imgui_custom_draw;
  editor->engine->imgui_draw_custom_ctx = editor;
}

void
crude_editor_imgui_custom_draw
(
  _In_ void                                               *ctx
)
{
}

void
crude_editor_deinitialize
(
  _In_ crude_editor                                             *editor
)
{
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

    if ( crude_entity_valid( editor->engine->world, editor->editor_camera_node ) )
    {
      crude_entity_destroy( editor->engine->world, editor->editor_camera_node );
    }

    editor_camera_node_transform = CRUDE_COMPOUNT_EMPTY( crude_transform );
    XMStoreFloat4( &editor_camera_node_transform.rotation, XMQuaternionIdentity( ) );
    XMStoreFloat3( &editor_camera_node_transform.scale, XMVectorSplatOne( ) );
    XMStoreFloat3( &editor_camera_node_transform.translation, XMVectorZero( ) );

    editor_camera_node_camera = CRUDE_COMPOUNT_EMPTY( crude_camera );
    editor_camera_node_camera.fov_radians = 1;
    editor_camera_node_camera.aspect_ratio = 1.8;
    editor_camera_node_camera.near_z = 1;
    editor_camera_node_camera.far_z = 300;

    editor_camera_node_crude_free_camera = CRUDE_COMPOUNT_EMPTY( crude_free_camera );
    editor_camera_node_crude_free_camera.moving_speed_multiplier = 10.f;
    editor_camera_node_crude_free_camera.rotating_speed_multiplier = 0.004f;
    editor_camera_node_crude_free_camera.input_enabled = true;
    editor_camera_node_crude_free_camera.input = &editor->engine->platform.input;

    editor->editor_camera_node = crude_entity_create_empty( editor->engine->world, "editor_camera" );
    CRUDE_ENTITY_SET_COMPONENT( editor->engine->world, editor->editor_camera_node, crude_transform, { editor_camera_node_transform } );
    CRUDE_ENTITY_SET_COMPONENT( editor->engine->world, editor->editor_camera_node, crude_camera, { editor_camera_node_camera } );
    CRUDE_ENTITY_SET_COMPONENT( editor->engine->world, editor->editor_camera_node, crude_free_camera, { editor_camera_node_crude_free_camera } );
  }

  editor->editor_camera_node = crude_ecs_lookup_entity_from_parent( editor->engine->world, editor->main_node, "editor_camera" );

  editor->engine->camera_node = editor->editor_camera_node;
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