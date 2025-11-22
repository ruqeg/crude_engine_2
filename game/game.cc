#if CRUDE_DEVELOP
#include <imgui/imgui.h>
#include <imgui/backends/imgui_impl_sdl3.h>
#include <imgui/backends/imgui_impl_vulkan.h>
#endif

#include <core/hash_map.h>
#include <core/file.h>
#include <core/memory.h>
#include <core/process.h>
#include <platform/platform_system.h>
#include <platform/platform_components.h>
#include <scene/free_camera_system.h>
#include <scene/scene_components.h>
#include <scene/scripts_components.h>
#include <graphics/gpu_resources_loader.h>
#include <physics/physics.h>
#include <game_components.h>
#include <player_controller_system.h>
#include <enemy_system.h>
#include <level_01_system.h>

#include <game.h>

game_t                                                    *game_instance_;

CRUDE_ECS_SYSTEM_DECLARE( game_graphics_system_ );
CRUDE_ECS_SYSTEM_DECLARE( game_input_system_ );
CRUDE_ECS_SYSTEM_DECLARE( game_physics_system_ );

static void
game_initialize_allocators_
(
  _In_ game_t                                             *game
);

#if CRUDE_DEVELOP
static void
game_initialize_imgui_
(
  _In_ game_t                                             *game
);
#endif

static void
game_initialize_constant_strings_
(
  _In_ game_t                                             *game,
  _In_ char const                                         *scene_relative_filepath,
  _In_ char const                                         *render_graph_relative_directory,
  _In_ char const                                         *resources_relative_directory,
  _In_ char const                                         *shaders_relative_directory,
  _In_ char const                                         *techniques_relative_directory,
  _In_ char const                                         *compiled_shaders_relative_directory,
  _In_ char const                                         *working_absolute_directory
);

static void
game_deinitialize_constant_strings_
(
  _In_ game_t                                             *game
);

static void
game_initialize_platform_
(
  _In_ game_t                                             *game
);

static void
game_initialize_physics_
(
  _In_ game_t                                             *game
);

static void
game_initialize_scene_
(
  _In_ game_t                                             *game
);

static void
game_initialize_graphics_
(
  _In_ game_t                                             *game
);

static void
game_graphics_system_
(
  _In_ ecs_iter_t                                         *it
);

static void
game_input_system_
(
  _In_ ecs_iter_t                                         *it
);

static void
game_physics_system_
(
  _In_ ecs_iter_t                                         *it
);

static void
game_graphics_deinitialize_
(
  _In_ game_t                                             *game
);

static bool
game_parse_json_to_component_
( 
  _In_ crude_entity                                        node, 
  _In_ cJSON const                                        *component_json,
  _In_ char const                                         *component_name,
  _In_ crude_scene                                        *scene
);

static void
game_parse_all_components_to_json_
( 
  _In_ crude_entity                                        node, 
  _In_ cJSON                                               *node_components_json,
  _In_ crude_scene                                         *scene
);

#if CRUDE_DEVELOP
static void
game_input_callback_
(
  _In_ void                                               *ctx,
  _In_ void                                               *sdl_event
);
#endif

void
game_initialize
(
  _In_ game_t                                             *game,
  _In_ crude_game_creation const                          *creation
)
{
  game->engine = creation->engine;
  game->framerate = creation->framerate;
  game->last_graphics_update_time = 0.f;

  ECS_IMPORT( game->engine->world, crude_platform_system );
  //ECS_IMPORT( game->engine->world, crude_physics_system );
  ECS_IMPORT( game->engine->world, crude_player_controller_system );
  ECS_IMPORT( game->engine->world, crude_enemy_system );
  ECS_IMPORT( game->engine->world, crude_level_01_system );

  game_initialize_allocators_( game );
#if CRUDE_DEVELOP
  game_initialize_imgui_( game );
#endif
  game_initialize_constant_strings_( game, creation->scene_relative_filepath, creation->render_graph_relative_directory, creation->resources_relative_directory, creation->shaders_relative_directory, creation->techniques_relative_directory, creation->compiled_shaders_relative_directory, creation->working_absolute_directory );
  game_initialize_platform_( game );
  game_initialize_physics_( game );
  crude_collisions_resources_manager_instance_allocate( crude_heap_allocator_pack( &game->allocator ) );
  crude_collisions_resources_manager_initialize( crude_collisions_resources_manager_instance( ), &game->allocator, &game->cgltf_temporary_allocator );
  game_initialize_scene_( game );
  game_initialize_graphics_( game );
  crude_devmenu_initialize( &game->devmenu );

  CRUDE_ARRAY_INITIALIZE_WITH_CAPACITY( game->commands_queue, 0, crude_heap_allocator_pack( &game->allocator ) );
  
  CRUDE_ECS_SYSTEM_DEFINE( game->engine->world, game_graphics_system_, EcsPreStore, game, { } );
  
  CRUDE_ECS_SYSTEM_DEFINE( game->engine->world, game_input_system_, EcsOnUpdate, game, {
    { .id = ecs_id( crude_input ) },
    { .id = ecs_id( crude_window_handle ) },
  } );
  
  CRUDE_ECS_SYSTEM_DEFINE( game->engine->world, game_physics_system_, EcsOnUpdate, game, { } );
}

void
game_postupdate
(
  _In_ game_t                                             *game
)
{
  crude_devmenu_update( &game->devmenu );

  for ( uint32 i = 0; i < CRUDE_ARRAY_LENGTH( game->commands_queue ); ++i )
  {
    switch ( game->commands_queue[ i ].type )
    {
    case CRUDE_GAME_QUEUE_COMMAND_TYPE_RELOAD_SCENE:
    {
      crude_scene_creation                                 scene_creation;
      bool                                                 buffer_recreated;

      vkDeviceWaitIdle( game->gpu.vk_device );
      crude_entity_destroy_hierarchy( game->scene.main_node );
      crude_scene_deinitialize( &game->scene );

      scene_creation = CRUDE_COMPOUNT_EMPTY( crude_scene_creation );
      scene_creation.world = game->engine->world;
      scene_creation.input_entity = game->platform_node;
      scene_creation.scene_absolute_filepath = game->commands_queue[ i ].reload_scene.filepath;
      scene_creation.temporary_allocator = &game->temporary_allocator;
      scene_creation.allocator_container = crude_heap_allocator_pack( &game->allocator );
      scene_creation.additional_parse_all_components_to_json_func = game_parse_all_components_to_json_;
      scene_creation.additional_parse_json_to_component_func = game_parse_json_to_component_;
      scene_creation.physics = &game->physics;
      crude_scene_initialize( &game->scene, &scene_creation );

      buffer_recreated = crude_gfx_scene_renderer_update_instances_from_node( &game->scene_renderer, game->scene.main_node );
      crude_gfx_model_renderer_resources_manager_wait_till_uploaded( &game->model_renderer_resources_manager );

      if ( buffer_recreated )
      {
        crude_gfx_scene_renderer_deinitialize_passes( &game->scene_renderer );
        crude_gfx_scene_renderer_initialize_pases( &game->scene_renderer );
        crude_gfx_scene_renderer_register_passes( &game->scene_renderer, &game->render_graph );
      }

      NFD_FreePathU8( game->commands_queue[ i ].reload_scene.filepath );
      break;
    }
    case CRUDE_GAME_QUEUE_COMMAND_TYPE_RELOAD_TECHNIQUES:
    {
      for ( uint32 i = 0; i < CRUDE_HASHMAP_CAPACITY( game->gpu.resource_cache.techniques ); ++i )
      {
        if ( !crude_hashmap_backet_key_valid( game->gpu.resource_cache.techniques[ i ].key ) )
        {
          continue;
        }
        
        crude_gfx_technique *technique = game->gpu.resource_cache.techniques[ i ].value; 
        crude_gfx_destroy_technique_instant( &game->gpu, technique );
        crude_gfx_technique_load_from_file( technique->technique_relative_filepath, &game->gpu, &game->render_graph, &game->temporary_allocator );
      }

      crude_gfx_render_graph_on_techniques_reloaded( &game->render_graph );
      break;
    }
    }
  }

  CRUDE_ARRAY_SET_LENGTH( game->commands_queue, 0u );
}

void
game_deinitialize
(
  _In_ game_t                                             *game
)
{
  CRUDE_ARRAY_DEINITIALIZE( game->commands_queue );
  
  crude_devmenu_deinitialize( &game->devmenu );
  crude_collisions_resources_manager_deinitialize( crude_collisions_resources_manager_instance( ) );
  crude_collisions_resources_manager_instance_deallocate( crude_heap_allocator_pack( &game->allocator ) );
  game_graphics_deinitialize_( game );
  crude_physics_deinitialize( &game->physics );
  crude_scene_deinitialize( &game->scene );
  game_deinitialize_constant_strings_( game );
  crude_heap_allocator_deinitialize( &game->cgltf_temporary_allocator );
  crude_heap_allocator_deinitialize( &game->allocator );
  crude_heap_allocator_deinitialize( &game->resources_allocator );
  crude_stack_allocator_deinitialize( &game->model_renderer_resources_manager_temporary_allocator );
  crude_stack_allocator_deinitialize( &game->temporary_allocator );

  ImGui::DestroyContext( ( ImGuiContext* )game->imgui_context );
}

void
game_push_reload_scene_command
(
  _In_ game_t                                             *game,
  _In_ nfdu8char_t                                        *filepath
)
{
  crude_game_queue_command command = CRUDE_COMPOUNT_EMPTY( crude_game_queue_command );
  command.type = CRUDE_GAME_QUEUE_COMMAND_TYPE_RELOAD_SCENE;
  command.reload_scene.filepath = filepath;
  CRUDE_ARRAY_PUSH( game->commands_queue, command );
}

void
game_push_reload_techniques_command
(
  _In_ game_t                                             *game
)
{
  crude_game_queue_command command = CRUDE_COMPOUNT_EMPTY( crude_game_queue_command );
  command.type = CRUDE_GAME_QUEUE_COMMAND_TYPE_RELOAD_TECHNIQUES;
  CRUDE_ARRAY_PUSH( game->commands_queue, command );
}

void
game_set_focused_camera_node
(
  _In_ game_t                                             *game,
  _In_ crude_entity                                        focused_camera_node
)
{
  game->focused_camera_node = focused_camera_node;
}

crude_scene*
game_get_scene
(
  _In_ game_t                                             *game
)
{
  return &game->scene;
}

void
game_graphics_system_
(
  _In_ ecs_iter_t                                         *it
)
{
  game_t                                                  *game;
  crude_gfx_texture                                       *final_render_texture;
#if CRUDE_DEVELOP
  crude_gfx_texture                                       *imgui_render_texture;
#endif

  game = ( game_t* )it->ctx;
  
  final_render_texture = crude_gfx_access_texture( &game->gpu, crude_gfx_render_graph_builder_access_resource_by_name( game->scene_renderer.render_graph->builder, "final" )->resource_info.texture.handle );
#if CRUDE_DEVELOP
  imgui_render_texture = crude_gfx_access_texture( &game->gpu, crude_gfx_render_graph_builder_access_resource_by_name( game->scene_renderer.render_graph->builder, "imgui" )->resource_info.texture.handle );
#endif

  game->last_graphics_update_time += it->delta_time;

  if ( game->last_graphics_update_time < 1.f / game->framerate )
  {
    return;
  }

  game->last_graphics_update_time = 0.f;

  game->scene_renderer.options.camera_node = game->focused_camera_node;

  crude_gfx_new_frame( &game->gpu );
 
#if CRUDE_DEVELOP
  ImGui::SetCurrentContext( ( ImGuiContext* ) game->imgui_context );
  ImGui_ImplSDL3_NewFrame( );
  ImGui::NewFrame( );

  crude_devmenu_draw( &game->devmenu );
  ImGui::SetNextWindowSize( ImVec2( game->gpu.vk_swapchain_width, game->gpu.vk_swapchain_height ) );
  ImGui::Begin( "Background", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoInputs | ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoFocusOnAppearing );
  ImGui::Image( CRUDE_CAST( ImTextureRef, &final_render_texture->handle.index ), ImGui::GetContentRegionAvail( ) );
  ImGui::End( );
#endif

  if ( game->gpu.swapchain_resized_last_frame )
  {
    crude_gfx_scene_renderer_on_resize( &game->scene_renderer );
    crude_gfx_render_graph_on_resize( &game->render_graph, game->gpu.vk_swapchain_width, game->gpu.vk_swapchain_height );
  }

  crude_gfx_scene_renderer_submit_draw_task( &game->scene_renderer, false );
  
#if CRUDE_DEVELOP
  crude_gfx_present( &game->gpu, imgui_render_texture );
#else
  crude_gfx_present( &game->gpu, final_render_texture );
#endif
  
  CRUDE_ASSERT( !crude_gfx_scene_renderer_update_instances_from_node( &game->scene_renderer, game->scene.main_node ) );
  crude_gfx_model_renderer_resources_manager_wait_till_uploaded( &game->model_renderer_resources_manager );
}

void
game_graphics_deinitialize_
(
  _In_ game_t                                             *game
)
{
  crude_gfx_asynchronous_loader_manager_remove_loader( &game->engine->asynchronous_loader_manager, &game->async_loader );
  vkDeviceWaitIdle( game->gpu.vk_device );
  crude_gfx_scene_renderer_deinitialize_passes( &game->scene_renderer );
  crude_gfx_scene_renderer_deinitialize( &game->scene_renderer );
  crude_gfx_asynchronous_loader_deinitialize( &game->async_loader );
  crude_gfx_model_renderer_resources_manager_deintialize( &game->model_renderer_resources_manager );
  crude_gfx_render_graph_builder_deinitialize( &game->render_graph_builder );
  crude_gfx_render_graph_deinitialize( &game->render_graph );
  crude_gfx_device_deinitialize( &game->gpu );
}

void
game_input_system_
(
  _In_ ecs_iter_t                                         *it
)
{
  game_t *game = ( game_t* )it->ctx;
  crude_input *input_per_entity = ecs_field( it, crude_input, 0 );
  crude_window_handle *window_handle_per_entity = ecs_field( it, crude_window_handle, 1 );
  
#if CRUDE_DEVELOP
  ImGui::SetCurrentContext( CRUDE_CAST( ImGuiContext*, game->imgui_context ) );
  for ( uint32 i = 0; i < it->count; ++i )
  {
    crude_input *input = &input_per_entity[ i ];
    crude_window_handle *window_handle = &window_handle_per_entity[ i ];

    crude_devmenu_handle_input( &game->devmenu, input );
    //crude_devgui_handle_input( &editor->devgui, input );

    //if ( input->mouse.right.current && input->mouse.right.current != input->prev_mouse.right.current )
    //{
    //  SDL_GetMouseState( &game->last_unrelative_mouse_position.x, &game->last_unrelative_mouse_position.y );
    //  SDL_SetWindowRelativeMouseMode( CRUDE_CAST( SDL_Window*, window_handle->value ), true );
    //}
    //
    //if ( !input->mouse.right.current && input->mouse.right.current != input->prev_mouse.right.current )
    //{
    //  SDL_WarpMouseInWindow( CRUDE_CAST( SDL_Window*, window_handle->value ), editor->last_unrelative_mouse_position.x, editor->last_unrelative_mouse_position.y );
    //  SDL_SetWindowRelativeMouseMode( CRUDE_CAST( SDL_Window*, window_handle->value ), false );
    //}
  }
#endif /* CRUDE_DEVELOP */
}

void
game_physics_system_
(
  _In_ ecs_iter_t                                         *it
)
{
  game_t *game = ( game_t* )it->ctx;
  crude_physics_update( &game->physics, CRUDE_MIN( it->delta_time, 0.016f ) );
}

bool
game_parse_json_to_component_
( 
  _In_ crude_entity                                        node, 
  _In_ cJSON const                                        *component_json,
  _In_ char const                                         *component_name,
  _In_ crude_scene                                        *scene
)
{
  if ( crude_string_cmp( component_name, CRUDE_COMPONENT_STRING( crude_player_controller ) ) == 0 )
  {
    crude_player_controller                                player_controller;
    CRUDE_PARSE_JSON_TO_COMPONENT( crude_player_controller )( &player_controller, component_json, node, scene );
    CRUDE_ENTITY_SET_COMPONENT( node, crude_player_controller, { player_controller } );
  }
  else if ( crude_string_cmp( component_name, CRUDE_COMPONENT_STRING( crude_enemy ) ) == 0 )
  {
    crude_enemy                                enemy;
    CRUDE_PARSE_JSON_TO_COMPONENT( crude_enemy )( &enemy, component_json, node, scene );
    CRUDE_ENTITY_SET_COMPONENT( node, crude_enemy, { enemy } );
  }
  else if ( crude_string_cmp( component_name, CRUDE_COMPONENT_STRING( crude_level_01 ) ) == 0 )
  {
    crude_level_01                                         level01;
    CRUDE_PARSE_JSON_TO_COMPONENT( crude_level_01 )( &level01, component_json, node, scene );
    CRUDE_ENTITY_SET_COMPONENT( node, crude_level_01, { level01 } );
  }
  return true;
}

void
game_parse_all_components_to_json_
( 
  _In_ crude_entity                                        node, 
  _In_ cJSON                                               *node_components_json,
  _In_ crude_scene                                         *scene
)
{
  crude_player_controller const                           *player_component;
  crude_enemy const                                       *enemy;
  crude_level_01 const                                    *level01;
  
  player_component = CRUDE_ENTITY_GET_IMMUTABLE_COMPONENT( node, crude_player_controller );
  if ( player_component )
  {
    cJSON_AddItemToArray( node_components_json, CRUDE_PARSE_COMPONENT_TO_JSON( crude_player_controller )( player_component, scene ) );
  }
  
  enemy = CRUDE_ENTITY_GET_IMMUTABLE_COMPONENT( node, crude_enemy );
  if ( enemy )
  {
    cJSON_AddItemToArray( node_components_json, CRUDE_PARSE_COMPONENT_TO_JSON( crude_enemy )( enemy, scene ) );
  }
  
  level01 = CRUDE_ENTITY_GET_IMMUTABLE_COMPONENT( node, crude_level_01 );
  if ( level01 )
  {
    cJSON_AddItemToArray( node_components_json, CRUDE_PARSE_COMPONENT_TO_JSON( crude_level_01 )( level01, scene ) );
  }
}

#if CRUDE_DEVELOP
void
game_input_callback_
(
  _In_ void                                               *ctx,
  _In_ void                                               *sdl_event
)
{
  game_t *game = CRUDE_CAST( game_t*, ctx );
  ImGui::SetCurrentContext( CRUDE_CAST( ImGuiContext*, game->imgui_context ) );

  if ( game->devmenu.enabled )
  {
    ImGui_ImplSDL3_ProcessEvent( CRUDE_CAST( SDL_Event*, sdl_event ) );
  }
}
#endif

void
game_initialize_allocators_
(
  _In_ game_t                                             *game
)
{
  crude_heap_allocator_initialize( &game->cgltf_temporary_allocator, CRUDE_RMEGA( 16 ), "cgltf_temporary_allocator" );
  crude_heap_allocator_initialize( &game->allocator, CRUDE_RMEGA( 16 ), "common_allocator" );
  crude_heap_allocator_initialize( &game->resources_allocator, CRUDE_RMEGA( 16 ), "resources_allocator" );
  crude_stack_allocator_initialize( &game->temporary_allocator, CRUDE_RMEGA( 16 ), "temprorary_allocator" );
  crude_stack_allocator_initialize( &game->model_renderer_resources_manager_temporary_allocator, CRUDE_RMEGA( 64 ), "model_renderer_resources_manager_temporary_allocator" );
}

#if CRUDE_DEVELOP
void
game_initialize_imgui_
(
  _In_ game_t                                             *game
)
{
  ImGuiIO                                                 *imgui_io;

  IMGUI_CHECKVERSION();
  game->imgui_context = ImGui::CreateContext();
  ImGui::SetCurrentContext( CRUDE_CAST( ImGuiContext*, game->imgui_context ) );
  ImGui::StyleColorsDark();
  imgui_io = &ImGui::GetIO();
  imgui_io->ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
  imgui_io->ConfigWindowsResizeFromEdges = true;
  imgui_io->ConfigWindowsMoveFromTitleBarOnly = true;
}
#endif

void
game_initialize_constant_strings_
(
  _In_ game_t                                             *game,
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

  crude_string_buffer_initialize( &game->constant_strings_buffer, constant_string_buffer_size, crude_heap_allocator_pack( &game->allocator ) );
  game->working_absolute_directory = crude_string_buffer_append_use_f( &game->constant_strings_buffer, "%s", working_absolute_directory );
  game->resources_absolute_directory = crude_string_buffer_append_use_f( &game->constant_strings_buffer, "%s%s", game->working_absolute_directory, resources_relative_directory );
  game->shaders_absolute_directory = crude_string_buffer_append_use_f( &game->constant_strings_buffer, "%s%s", game->working_absolute_directory, shaders_relative_directory );
  game->scene_absolute_filepath = crude_string_buffer_append_use_f( &game->constant_strings_buffer, "%s%s", game->working_absolute_directory, scene_relative_filepath );
  game->render_graph_absolute_directory = crude_string_buffer_append_use_f( &game->constant_strings_buffer, "%s%s", game->working_absolute_directory, render_graph_relative_directory );
  game->techniques_absolute_directory = crude_string_buffer_append_use_f( &game->constant_strings_buffer, "%s%s", game->working_absolute_directory, techniques_relative_directory );
  game->compiled_shaders_absolute_directory = crude_string_buffer_append_use_f( &game->constant_strings_buffer, "%s%s", game->working_absolute_directory, compiled_shaders_relative_directory );
}

void
game_deinitialize_constant_strings_
(
  _In_ game_t                                             *game
)
{
  crude_string_buffer_deinitialize( &game->constant_strings_buffer );
}

void
game_initialize_platform_
(
  _In_ game_t                                             *game
)
{
  game->platform_node = crude_entity_create_empty( game->engine->world, "game" );

  CRUDE_ENTITY_SET_COMPONENT( game->platform_node, crude_window, { 
    .width     = 800,
    .height    = 600,
    .maximized = false,
    .flags     = SDL_WINDOW_VULKAN | SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIGH_PIXEL_DENSITY
  });

  CRUDE_ENTITY_SET_COMPONENT( game->platform_node, crude_input, {
    .callback = game_input_callback_,
    .ctx = game
  } );
}

void
game_initialize_physics_
(
  _In_ game_t                                             *game
)
{
  crude_physics_creation physics_creation = CRUDE_COMPOUNT_EMPTY( crude_physics_creation );
  physics_creation.allocator = &game->allocator;
  crude_physics_initialize( &game->physics, &physics_creation );
}

void
game_initialize_scene_
(
  _In_ game_t                                             *game
)
{
  crude_scene_creation                                     scene_creation;
  scene_creation = CRUDE_COMPOUNT_EMPTY( crude_scene_creation );
  scene_creation.world = game->engine->world;
  scene_creation.input_entity = game->platform_node;
  scene_creation.scene_absolute_filepath = game->scene_absolute_filepath;
  scene_creation.resources_absolute_directory = game->resources_absolute_directory;
  scene_creation.temporary_allocator = &game->temporary_allocator;
  scene_creation.allocator_container = crude_heap_allocator_pack( &game->allocator );
  scene_creation.additional_parse_all_components_to_json_func = game_parse_all_components_to_json_;
  scene_creation.additional_parse_json_to_component_func = game_parse_json_to_component_;
  scene_creation.physics = &game->physics;
  crude_scene_initialize( &game->scene, &scene_creation );
}

void
game_initialize_graphics_
(
  _In_ game_t                                             *game
)
{
  crude_window_handle                                     *window_handle;
  char const                                              *render_graph_file_path;
  crude_string_buffer                                      temporary_name_buffer;
  crude_gfx_model_renderer_resources_manager_creation      model_renderer_resources_manager_creation;
  crude_gfx_device_creation                                device_creation;
  crude_gfx_scene_renderer_creation                        scene_renderer_creation;
  uint32                                                   temporary_allocator_marker;

  temporary_allocator_marker = crude_stack_allocator_get_marker( &game->temporary_allocator );
  
  crude_string_buffer_initialize( &temporary_name_buffer, 1024, crude_stack_allocator_pack( &game->temporary_allocator ) );

  window_handle = CRUDE_ENTITY_GET_MUTABLE_COMPONENT( game->platform_node, crude_window_handle );

  device_creation = CRUDE_COMPOUNT_EMPTY( crude_gfx_device_creation );
  device_creation.sdl_window = CRUDE_REINTERPRET_CAST( SDL_Window*, window_handle->value );
  device_creation.vk_application_name = "CrudeEngine";
  device_creation.vk_application_version = VK_MAKE_VERSION( 1, 0, 0 );
  device_creation.allocator_container = crude_heap_allocator_pack( &game->allocator );
  device_creation.temporary_allocator = &game->temporary_allocator;
  device_creation.queries_per_frame = 1u;
  device_creation.num_threads = CRUDE_STATIC_CAST( uint16, enkiGetNumTaskThreads( CRUDE_REINTERPRET_CAST( enkiTaskScheduler*, game->engine->asynchronous_loader_manager.task_sheduler ) ) );
  device_creation.shaders_absolute_directory = game->shaders_absolute_directory;
  device_creation.techniques_absolute_directory = game->techniques_absolute_directory;
  device_creation.compiled_shaders_absolute_directory = game->compiled_shaders_absolute_directory;
  crude_gfx_device_initialize( &game->gpu, &device_creation );
  
  crude_gfx_render_graph_builder_initialize( &game->render_graph_builder, &game->gpu );
  crude_gfx_render_graph_initialize( &game->render_graph, &game->render_graph_builder );
  
  crude_gfx_asynchronous_loader_initialize( &game->async_loader, &game->gpu );
  crude_gfx_asynchronous_loader_manager_add_loader( &game->engine->asynchronous_loader_manager, &game->async_loader );
#if CRUDE_DEVELOP
  render_graph_file_path = crude_string_buffer_append_use_f( &temporary_name_buffer, "%s%s", game->render_graph_absolute_directory, "game\\render_graph_develop.json" );
#else
  render_graph_file_path = crude_string_buffer_append_use_f( &temporary_name_buffer, "%s%s", game->render_graph_absolute_directory, "game\\render_graph.json" );
#endif
  crude_gfx_render_graph_parse_from_file( &game->render_graph, render_graph_file_path, &game->temporary_allocator );
  crude_gfx_render_graph_compile( &game->render_graph, &game->temporary_allocator );

  crude_gfx_technique_load_from_file( "deferred_meshlet.json", &game->gpu, &game->render_graph, &game->temporary_allocator );
  crude_gfx_technique_load_from_file( "pointshadow_meshlet.json", &game->gpu, &game->render_graph, &game->temporary_allocator );
  crude_gfx_technique_load_from_file( "compute.json", &game->gpu, &game->render_graph, &game->temporary_allocator );
  crude_gfx_technique_load_from_file( "debug.json", &game->gpu, &game->render_graph, &game->temporary_allocator );
  crude_gfx_technique_load_from_file( "fullscreen.json", &game->gpu, &game->render_graph, &game->temporary_allocator );
#if CRUDE_DEVELOP
  crude_gfx_technique_load_from_file( "imgui.json", &game->gpu, &game->render_graph, &game->temporary_allocator );
#endif
  
#if CRUDE_GRAPHICS_RAY_TRACING_ENABLED
  crude_gfx_renderer_technique_load_from_file( "ray_tracing_solid.json", &game->gpu, &game->render_graph, &game->temporary_allocator );
#endif /* CRUDE_GRAPHICS_RAY_TRACING_ENABLED */

  model_renderer_resources_manager_creation = CRUDE_COMPOUNT_EMPTY( crude_gfx_model_renderer_resources_manager_creation );
  model_renderer_resources_manager_creation.allocator = &game->allocator;
  model_renderer_resources_manager_creation.async_loader = &game->async_loader;
  model_renderer_resources_manager_creation.cgltf_temporary_allocator = &game->cgltf_temporary_allocator;
  model_renderer_resources_manager_creation.temporary_allocator = &game->model_renderer_resources_manager_temporary_allocator;
  model_renderer_resources_manager_creation.world = game->engine->world;
  crude_gfx_model_renderer_resources_manager_intialize( &game->model_renderer_resources_manager, &model_renderer_resources_manager_creation );

  scene_renderer_creation = CRUDE_COMPOUNT_EMPTY( crude_gfx_scene_renderer_creation );
  scene_renderer_creation.async_loader = &game->async_loader;
  scene_renderer_creation.allocator = &game->allocator;
  scene_renderer_creation.temporary_allocator = &game->temporary_allocator;
  scene_renderer_creation.task_scheduler = game->engine->asynchronous_loader_manager.task_sheduler;
  scene_renderer_creation.scene = &game->scene;
  scene_renderer_creation.model_renderer_resources_manager = &game->model_renderer_resources_manager;
#if CRUDE_DEVELOP
  scene_renderer_creation.imgui_pass_enalbed = true;
  scene_renderer_creation.imgui_context = game->imgui_context;
#endif
  crude_gfx_scene_renderer_initialize( &game->scene_renderer, &scene_renderer_creation );

  game->scene_renderer.options.ambient_color = CRUDE_COMPOUNT( XMFLOAT3, { 1, 1, 1 } );
  game->scene_renderer.options.ambient_intensity = 1.5f;

  crude_gfx_scene_renderer_update_instances_from_node( &game->scene_renderer, game->scene.main_node );
  crude_gfx_scene_renderer_rebuild_light_gpu_buffers( &game->scene_renderer );
  crude_gfx_model_renderer_resources_manager_wait_till_uploaded( &game->model_renderer_resources_manager );

  crude_gfx_scene_renderer_initialize_pases( &game->scene_renderer );
  crude_gfx_scene_renderer_register_passes( &game->scene_renderer, &game->render_graph );

  crude_stack_allocator_free_marker( &game->temporary_allocator, temporary_allocator_marker );
}

void
game_instance_intialize
(
)
{
  game_instance_ = CRUDE_CAST( game_t*, malloc( sizeof( game_t ) ) );
}

void
game_instance_deintialize
(
)
{
  free( game_instance_ );
}

game_t*
game_instance
(
)
{
  return game_instance_;
}