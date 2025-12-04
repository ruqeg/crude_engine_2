#if CRUDE_DEVELOP
#include <imgui/imgui.h>
#include <imgui/backends/imgui_impl_sdl3.h>
#include <imgui/backends/imgui_impl_vulkan.h>
#endif

#include <engine/core/profiler.h>
#include <engine/core/hash_map.h>
#include <engine/core/file.h>
#include <engine/core/memory.h>
#include <engine/core/process.h>
#include <engine/platform/platform_system.h>
#include <engine/platform/platform_components.h>
#include <engine/scene/scene_components.h>
#include <engine/scene/scripts_components.h>
#include <engine/graphics/gpu_resources_loader.h>
#include <engine/physics/physics.h>
#include <engine/physics/physics_debug_system.h>
#include <engine/physics/physics_system.h>
#include <engine/external/game_components.h>
#include <engine/scene/scripts_components.h>
#include <engine/scene/free_camera_system.h>
#include <game/player_controller_system.h>
#include <game/player_system.h>
#include <game/enemy_system.h>
#include <game/level_01_system.h>
#include <game/serum_station_system.h>

#include <game/game.h>

game_t                                                    *game_instance_;

CRUDE_ECS_SYSTEM_DECLARE( game_update_system_ );
CRUDE_ECS_SYSTEM_DECLARE( game_graphics_system_ );
CRUDE_ECS_SYSTEM_DECLARE( game_input_system_ );

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
game_update_system_
(
  _In_ ecs_iter_t                                         *it
);

static void
game_input_system_
(
  _In_ ecs_iter_t                                         *it
);

static void
game_graphics_deinitialize_
(
  _In_ game_t                                             *game
);

static void
game_setup_custom_preload_nodes_
(
  _In_ game_t                                             *game
);

static void
game_setup_custom_postload_nodes_
(
  _In_ game_t                                             *game
);

static void
game_setup_custom_postload_model_resources_
(
  _In_ game_t                                             *game
);

static bool
game_parse_json_to_component_
( 
  _In_ crude_entity                                        node, 
  _In_ cJSON const                                        *component_json,
  _In_ char const                                         *component_name,
  _In_ crude_node_manager                                 *manager
);

static void
game_parse_all_components_to_json_
( 
  _In_ crude_entity                                        node, 
  _In_ cJSON                                               *node_components_json,
  _In_ crude_node_manager                                 *manager
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
  game->time = 0.f;

  ECS_IMPORT( game->engine->world, crude_platform_system );
  ECS_IMPORT( game->engine->world, crude_player_controller_system );
  ECS_IMPORT( game->engine->world, crude_enemy_system );
  ECS_IMPORT( game->engine->world, crude_level_01_system );
  ECS_IMPORT( game->engine->world, crude_player_system );
  ECS_IMPORT( game->engine->world, crude_serum_station_system );
  
  game_initialize_allocators_( game );
#if CRUDE_DEVELOP
  game_initialize_imgui_( game );
#endif
  game_initialize_constant_strings_( game, creation->scene_relative_filepath, creation->render_graph_relative_directory, creation->resources_relative_directory, creation->shaders_relative_directory, creation->techniques_relative_directory, creation->compiled_shaders_relative_directory, creation->working_absolute_directory );
  
  CRUDE_ARRAY_INITIALIZE_WITH_CAPACITY( game->commands_queue, 0, crude_heap_allocator_pack( &game->allocator ) );

  game->physics_debug_system_context = CRUDE_COMPOUNT_EMPTY( crude_physics_debug_system_context );
  game->physics_debug_system_context.resources_absolute_directory = game->resources_absolute_directory;
  game->physics_debug_system_context.string_bufffer = &game->debug_strings_buffer;
  crude_physics_debug_system_import( game->engine->world, &game->physics_debug_system_context );
  
  game->game_debug_system_context = CRUDE_COMPOUNT_EMPTY( crude_game_debug_system_context );
  game->game_debug_system_context.enemy_spawnpoint_model_absolute_filepath = game->enemy_spawnpoint_debug_model_absolute_filepath;
  game->game_debug_system_context.syringe_serum_station_active_model_absolute_filepath = game->syringe_serum_station_active_debug_model_absolute_filepath;
  game->game_debug_system_context.syringe_spawnpoint_model_absolute_filepath = game->syringe_spawnpoint_debug_model_absolute_filepath;
  crude_game_debug_system_import( game->engine->world, &game->game_debug_system_context );

  game_initialize_platform_( game );
  game_initialize_physics_( game );
  crude_collisions_resources_manager_initialize( &game->collision_resources_manager, &game->allocator, &game->cgltf_temporary_allocator );
  game_initialize_scene_( game );
  game_initialize_graphics_( game );

  crude_devmenu_initialize( &game->devmenu );
  
  CRUDE_ECS_SYSTEM_DEFINE( game->engine->world, game_graphics_system_, EcsPreStore, game, { } );
  CRUDE_ECS_SYSTEM_DEFINE( game->engine->world, game_update_system_, EcsPreStore, game, { } );
  
  CRUDE_ECS_SYSTEM_DEFINE( game->engine->world, game_input_system_, EcsOnUpdate, game, {
    { .id = ecs_id( crude_input ) },
    { .id = ecs_id( crude_window_handle ) },
  } );
}

void
game_postupdate
(
  _In_ game_t                                             *game
)
{
  CRUDE_PROFILER_ZONE_NAME( "game_postupdate" );
  crude_devmenu_update( &game->devmenu );

  for ( uint32 i = 0; i < CRUDE_ARRAY_LENGTH( game->commands_queue ); ++i )
  {
    switch ( game->commands_queue[ i ].type )
    {
    case CRUDE_GAME_QUEUE_COMMAND_TYPE_RELOAD_SCENE:
    {
      bool                                                 buffer_recreated;

      vkDeviceWaitIdle( game->gpu.vk_device );
      
      crude_node_manager_clear( &game->node_manager );
      crude_physics_resources_manager_clear( &game->physics_resources_manager );
      crude_string_buffer_clear( &game->debug_strings_buffer );

      game_setup_custom_preload_nodes_( game );
      game->main_node = crude_node_manager_get_node( &game->node_manager, game->current_scene_absolute_filepath );
      game_setup_custom_postload_nodes_( game );

      crude_gfx_scene_renderer_update_instances_from_node( &game->scene_renderer, game->main_node );
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
    case CRUDE_GAME_QUEUE_COMMAND_TYPE_ENABLE_RANDOM_SERUM_STATION:
    {
      crude_entity                                         serum_stations_spawn_points_parent_node;
      ecs_iter_t                                           serum_stations_spawn_points_it;
      uint32                                               serum_stations_count;
 
      srand( time( NULL ) );

      serum_stations_spawn_points_parent_node = crude_ecs_lookup_entity_from_parent( game->main_node, "serum_station_spawn_points" );

      serum_stations_count = 0;
      serum_stations_spawn_points_it = ecs_children( game->engine->world, serum_stations_spawn_points_parent_node.handle );
      while ( ecs_children_next( &serum_stations_spawn_points_it ) )
      {
        uint32                                             serum_stations_spawn_points_random_index;
        uint32                                             serum_stations_spawn_points_count;

        serum_stations_spawn_points_count = serum_stations_spawn_points_it.count;
        serum_stations_spawn_points_random_index = rand( ) % serum_stations_spawn_points_count;

        while ( serum_stations_spawn_points_random_index < serum_stations_spawn_points_count )
        {
          crude_serum_station                             *serum_station_component;
          crude_entity                                     serum_station_spawn_point_node;
          char                                             serum_station_node_name_buffer[ 512 ];
          
          crude_snprintf( serum_station_node_name_buffer, sizeof( serum_station_node_name_buffer ), "serum_station_%i", serum_stations_spawn_points_random_index );

          serum_station_spawn_point_node = crude_ecs_lookup_entity_from_parent( game->main_node, serum_station_node_name_buffer );

          if ( serum_station_spawn_point_node.handle != game->commands_queue[ i ].enable_random_serum_station.ignored_serum_station.handle )
          {
            if ( !CRUDE_ENTITY_HAS_TAG( serum_station_spawn_point_node, crude_serum_station_enabled ) )
            {
              CRUDE_ENTITY_ADD_TAG( serum_station_spawn_point_node, crude_serum_station_enabled );
              break;
            }
          }
          serum_stations_spawn_points_random_index = ( serum_stations_spawn_points_random_index + 1 ) % serum_stations_spawn_points_count;
        }
      }
      break;
    }
    }
  }

  CRUDE_ARRAY_SET_LENGTH( game->commands_queue, 0u );
  CRUDE_PROFILER_END( "game_postupdate" );
}

void
game_deinitialize
(
  _In_ game_t                                             *game
)
{
  CRUDE_ARRAY_DEINITIALIZE( game->commands_queue );
  
  crude_devmenu_deinitialize( &game->devmenu );
  crude_collisions_resources_manager_deinitialize( &game->collision_resources_manager );
  game_graphics_deinitialize_( game );
  crude_node_manager_deinitialize( &game->node_manager );
  crude_physics_resources_manager_deinitialize( &game->physics_resources_manager );
  crude_physics_deinitialize( &game->physics );
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
  _In_ game_t                                             *game
)
{
  crude_game_queue_command command = CRUDE_COMPOUNT_EMPTY( crude_game_queue_command );
  command.type = CRUDE_GAME_QUEUE_COMMAND_TYPE_RELOAD_SCENE;
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
game_push_enable_random_serum_station_command
(
  _In_ game_t                                             *game,
  _In_ crude_entity                                        ignored_serum_station
)
{
  crude_game_queue_command command = CRUDE_COMPOUNT_EMPTY( crude_game_queue_command );
  command.type = CRUDE_GAME_QUEUE_COMMAND_TYPE_ENABLE_RANDOM_SERUM_STATION;
  command.enable_random_serum_station.ignored_serum_station = ignored_serum_station;
  CRUDE_ARRAY_PUSH( game->commands_queue, command );
}

void
game_player_set_item
(
  _In_ game_t                                             *game,
  _In_ crude_player                                       *player,
  _In_ uint32                                              slot,
  _In_ crude_game_item                                     item
)
{
  crude_entity                                             player_items_node, player_item_node;
  char                                                     item_node_name_buffer[ 128 ];;

  player_items_node = crude_ecs_lookup_entity_from_parent( game->player_node, "pivot.items" );
  
  crude_snprintf( item_node_name_buffer, sizeof( item_node_name_buffer ), "item_%i", slot );
  player_item_node = crude_ecs_lookup_entity_from_parent( player_items_node, item_node_name_buffer );
  
  player->inventory_items[ slot ] = item;

  switch ( item )
  {
  case CRUDE_GAME_ITEM_NONE:
  {
    CRUDE_ENTITY_REMOVE_COMPONENT( player_item_node, crude_gltf );
    break;
  }
  case CRUDE_GAME_ITEM_SERUM:
  {
    CRUDE_ENTITY_SET_COMPONENT( player_item_node, crude_gltf, { game->serum_model_absolute_filepath } );
    break;
  }
  case CRUDE_GAME_ITEM_SYRINGE_DRUG:
  {
    CRUDE_ENTITY_SET_COMPONENT( player_item_node, crude_gltf, { game->syringe_drug_model_absolute_filepath } );
    break;
  }
  case CRUDE_GAME_ITEM_SYRINGE_HEALTH:
  {
    CRUDE_ENTITY_SET_COMPONENT( player_item_node, crude_gltf, { game->syringe_health_model_absolute_filepath } );
    break;
  }
  }
  CRUDE_ENTITY_ADD_COMPONENT( player_item_node, crude_node_runtime );
}

void
game_update_system_
(
  _In_ ecs_iter_t                                         *it
)
{
  CRUDE_PROFILER_ZONE_NAME( "game_update_system_" );
  game_t                                                  *game;

  game = ( game_t* )it->ctx;
  game->time += it->delta_time;
  CRUDE_PROFILER_END;
}

void
game_graphics_system_
(
  _In_ ecs_iter_t                                         *it
)
{
  CRUDE_PROFILER_ZONE_NAME( "game_graphics_system_" );
  game_t                                                  *game;
  crude_gfx_texture                                       *final_render_texture;

  game = ( game_t* )it->ctx;
  
  final_render_texture = crude_gfx_access_texture( &game->gpu, crude_gfx_render_graph_builder_access_resource_by_name( game->scene_renderer.render_graph->builder, "final" )->resource_info.texture.handle );

  game->last_graphics_update_time += it->delta_time;

  if ( game->last_graphics_update_time < 1.f / game->framerate )
  {
    CRUDE_PROFILER_END;
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
#endif

  if ( game->gpu.swapchain_resized_last_frame )
  {
    crude_gfx_scene_renderer_on_resize( &game->scene_renderer );
    crude_gfx_render_graph_on_resize( &game->render_graph, game->gpu.vk_swapchain_width, game->gpu.vk_swapchain_height );
  }

  crude_gfx_scene_renderer_submit_draw_task( &game->scene_renderer, false );
  
  crude_gfx_present( &game->gpu, final_render_texture );
  
  CRUDE_ASSERT( !crude_gfx_scene_renderer_update_instances_from_node( &game->scene_renderer, game->main_node ) );
  crude_gfx_model_renderer_resources_manager_wait_till_uploaded( &game->model_renderer_resources_manager );
  CRUDE_PROFILER_END;
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
  crude_gfx_game_postprocessing_pass_deinitialize( &game->game_postprocessing_pass );
  crude_gfx_scene_renderer_deinitialize( &game->scene_renderer );
  crude_gfx_asynchronous_loader_deinitialize( &game->async_loader );
  crude_gfx_model_renderer_resources_manager_deintialize( &game->model_renderer_resources_manager );
  crude_gfx_render_graph_builder_deinitialize( &game->render_graph_builder );
  crude_gfx_render_graph_deinitialize( &game->render_graph );
  crude_gfx_device_deinitialize( &game->gpu );
}

void
game_setup_custom_preload_nodes_
(
  _In_ game_t                                             *game
)
{
  char const *enemy_node_relative_filepath = "game\\nodes\\enemy.crude_node";
  char const *serum_station_node_relative_filepath = "game\\nodes\\serum_station.crude_node";

  game->enemy_node_absolute_filepath = crude_string_buffer_append_use_f( &game->debug_strings_buffer, "%s%s", game->resources_absolute_directory, enemy_node_relative_filepath );
  game->template_enemy_node = crude_node_manager_get_node( &game->node_manager, game->enemy_node_absolute_filepath );
  crude_entity_enable_hierarchy( game->template_enemy_node, false );

  game->serum_station_node_absolute_filepath = crude_string_buffer_append_use_f( &game->debug_strings_buffer, "%s%s", game->resources_absolute_directory, serum_station_node_relative_filepath );
  game->template_serum_station_node = crude_node_manager_get_node( &game->node_manager, game->serum_station_node_absolute_filepath );
  crude_entity_enable_hierarchy( game->template_serum_station_node, false );
}

void
game_setup_custom_postload_nodes_
(
  _In_ game_t                                             *game
)
{
  game->player_node = crude_ecs_lookup_entity_from_parent( game->main_node, "player" );
  
  /*
    crude_entity editor_camera_node;
  {
    crude_transform                                        editor_camera_node_transform;
    crude_camera                                           editor_camera_node_camera;
    crude_free_camera                                      editor_camera_node_crude_free_camera;


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
    editor_camera_node_crude_free_camera.input_node = game->platform_node;

    editor_camera_node = crude_entity_create_empty( game->engine->world, "editor_camera" );
    CRUDE_ENTITY_SET_COMPONENT( editor_camera_node, crude_transform, { editor_camera_node_transform } );
    CRUDE_ENTITY_SET_COMPONENT( editor_camera_node, crude_camera, { editor_camera_node_camera } );
    CRUDE_ENTITY_SET_COMPONENT( editor_camera_node, crude_free_camera, { editor_camera_node_crude_free_camera } );
  }

  game->focused_camera_node = editor_camera_node;*/
}

void
game_setup_custom_postload_model_resources_
(
  _In_ game_t                                             *game
)
{
  crude_gfx_model_renderer_resources_manager_get_gltf_model( &game->model_renderer_resources_manager, game->serum_model_absolute_filepath , NULL );
  crude_gfx_model_renderer_resources_manager_get_gltf_model( &game->model_renderer_resources_manager, game->syringe_drug_model_absolute_filepath , NULL );
  crude_gfx_model_renderer_resources_manager_get_gltf_model( &game->model_renderer_resources_manager, game->syringe_health_model_absolute_filepath , NULL );
  crude_gfx_model_renderer_resources_manager_get_gltf_model( &game->model_renderer_resources_manager, game->syringe_spawnpoint_debug_model_absolute_filepath, NULL );
  crude_gfx_model_renderer_resources_manager_get_gltf_model( &game->model_renderer_resources_manager, game->enemy_spawnpoint_debug_model_absolute_filepath, NULL );
  crude_gfx_model_renderer_resources_manager_get_gltf_model( &game->model_renderer_resources_manager, game->syringe_serum_station_active_debug_model_absolute_filepath, NULL );
  crude_gfx_model_renderer_resources_manager_get_gltf_model( &game->model_renderer_resources_manager, game->serum_station_enabled_model_absolute_filepath, NULL );
  crude_gfx_model_renderer_resources_manager_get_gltf_model( &game->model_renderer_resources_manager, game->serum_station_disabled_model_absolute_filepath, NULL );
}

void
game_input_system_
(
  _In_ ecs_iter_t                                         *it
)
{
  CRUDE_PROFILER_ZONE_NAME( "game_input_system_" );
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
  CRUDE_PROFILER_END( "game_input_system_" );
#endif /* CRUDE_DEVELOP */
}

bool
game_parse_json_to_component_
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
game_parse_all_components_to_json_
( 
  _In_ crude_entity                                        node, 
  _In_ cJSON                                               *node_components_json,
  _In_ crude_node_manager                                 *manager
)
{
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
  char const *serum_model_relative_filepath = "game\\models\\serum.gltf";
  char const *syringe_drug_model_relative_filepath = "game\\models\\syringe_drug.gltf";
  char const *syringe_health_model_relative_filepath = "game\\models\\syringe_health.gltf";
  char const *serum_station_enabled_model_relative_filepath = "game\\models\\serum_station_enabled.gltf";
  char const *serum_station_disabled_model_relative_filepath = "game\\models\\serum_station_disabled.gltf";

  uint64 constant_string_buffer_size = 0;
  uint64 working_directory_length = crude_string_length( working_absolute_directory ) + 1;
  constant_string_buffer_size += working_directory_length;
  constant_string_buffer_size += working_directory_length;
  constant_string_buffer_size += working_directory_length + crude_string_length( shaders_relative_directory );
  constant_string_buffer_size += working_directory_length + crude_string_length( render_graph_relative_directory );
  constant_string_buffer_size += working_directory_length + crude_string_length( scene_relative_filepath );
  constant_string_buffer_size += working_directory_length + crude_string_length( techniques_relative_directory );
  constant_string_buffer_size += working_directory_length + crude_string_length( compiled_shaders_relative_directory );
  
  uint64 resources_absolute_directory_length = working_directory_length + crude_string_length( resources_relative_directory );
  constant_string_buffer_size += resources_absolute_directory_length;
  constant_string_buffer_size += resources_absolute_directory_length + crude_string_length( serum_model_relative_filepath );
  constant_string_buffer_size += resources_absolute_directory_length + crude_string_length( serum_station_enabled_model_relative_filepath );
  constant_string_buffer_size += resources_absolute_directory_length + crude_string_length( serum_station_disabled_model_relative_filepath );
  constant_string_buffer_size += resources_absolute_directory_length + crude_string_length( syringe_drug_model_relative_filepath );
  constant_string_buffer_size += resources_absolute_directory_length + crude_string_length( syringe_health_model_relative_filepath );

  crude_string_buffer_initialize( &game->constant_strings_buffer, constant_string_buffer_size, crude_heap_allocator_pack( &game->allocator ) );
  
  game->working_absolute_directory = crude_string_buffer_append_use_f( &game->constant_strings_buffer, "%s", working_absolute_directory );
  game->shaders_absolute_directory = crude_string_buffer_append_use_f( &game->constant_strings_buffer, "%s%s", game->working_absolute_directory, shaders_relative_directory );
  game->scene_absolute_filepath = crude_string_buffer_append_use_f( &game->constant_strings_buffer, "%s%s", game->working_absolute_directory, scene_relative_filepath );
  game->render_graph_absolute_directory = crude_string_buffer_append_use_f( &game->constant_strings_buffer, "%s%s", game->working_absolute_directory, render_graph_relative_directory );
  game->techniques_absolute_directory = crude_string_buffer_append_use_f( &game->constant_strings_buffer, "%s%s", game->working_absolute_directory, techniques_relative_directory );
  game->compiled_shaders_absolute_directory = crude_string_buffer_append_use_f( &game->constant_strings_buffer, "%s%s", game->working_absolute_directory, compiled_shaders_relative_directory );
  
  game->resources_absolute_directory = crude_string_buffer_append_use_f( &game->constant_strings_buffer, "%s%s", game->working_absolute_directory, resources_relative_directory );
  game->serum_model_absolute_filepath = crude_string_buffer_append_use_f( &game->constant_strings_buffer, "%s%s", game->resources_absolute_directory, serum_model_relative_filepath );
  game->syringe_drug_model_absolute_filepath = crude_string_buffer_append_use_f( &game->constant_strings_buffer, "%s%s", game->resources_absolute_directory, syringe_drug_model_relative_filepath );
  game->syringe_health_model_absolute_filepath = crude_string_buffer_append_use_f( &game->constant_strings_buffer, "%s%s", game->resources_absolute_directory, syringe_health_model_relative_filepath );
  game->serum_station_enabled_model_absolute_filepath = crude_string_buffer_append_use_f( &game->constant_strings_buffer, "%s%s", game->resources_absolute_directory, serum_station_enabled_model_relative_filepath );
  game->serum_station_disabled_model_absolute_filepath = crude_string_buffer_append_use_f( &game->constant_strings_buffer, "%s%s", game->resources_absolute_directory, serum_station_disabled_model_relative_filepath );

  crude_string_buffer_initialize( &game->debug_strings_buffer, 4096, crude_heap_allocator_pack( &game->allocator ) );
  crude_string_buffer_initialize( &game->debug_constant_strings_buffer, 4096, crude_heap_allocator_pack( &game->allocator ) );
  crude_string_buffer_initialize( &game->game_strings_buffer, 4096, crude_heap_allocator_pack( &game->allocator ) );

  game->syringe_spawnpoint_debug_model_absolute_filepath = crude_string_buffer_append_use_f( &game->debug_constant_strings_buffer, "%s%s", game->resources_absolute_directory, "debug\\models\\syringe_spawnpoint_model.gltf" );
  game->enemy_spawnpoint_debug_model_absolute_filepath = crude_string_buffer_append_use_f( &game->debug_constant_strings_buffer, "%s%s", game->resources_absolute_directory, "debug\\models\\enemy_spawnpoint_model.gltf" );
  game->syringe_serum_station_active_debug_model_absolute_filepath = crude_string_buffer_append_use_f( &game->debug_constant_strings_buffer, "%s%s", game->resources_absolute_directory, "debug\\models\\syringe_serum_station_active_model.gltf" );
}

void
game_deinitialize_constant_strings_
(
  _In_ game_t                                             *game
)
{
  crude_string_buffer_deinitialize( &game->debug_constant_strings_buffer );
  crude_string_buffer_deinitialize( &game->constant_strings_buffer );
  crude_string_buffer_deinitialize( &game->debug_strings_buffer );
  crude_string_buffer_deinitialize( &game->game_strings_buffer );
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
  crude_physics_resources_manager_creation physics_resources_manager_creation = CRUDE_COMPOUNT_EMPTY( crude_physics_resources_manager_creation );
  physics_resources_manager_creation.allocator = &game->allocator;
  crude_physics_resources_manager_initialize( &game->physics_resources_manager, &physics_resources_manager_creation );

  crude_physics_creation physics_creation = CRUDE_COMPOUNT_EMPTY( crude_physics_creation );
  physics_creation.collision_manager = &game->collision_resources_manager;
  physics_creation.manager = &game->physics_resources_manager;
  physics_creation.world = game->engine->world;
  crude_physics_initialize( &game->physics, &physics_creation );

  game->physics_system_context = CRUDE_COMPOUNT_EMPTY( crude_physics_system_context );
  game->physics_system_context.physics = &game->physics;
  crude_physics_system_import( game->engine->world, &game->physics_system_context );
}

void
game_initialize_scene_
(
  _In_ game_t                                             *game
)
{
  crude_string_copy( game->current_scene_absolute_filepath, game->scene_absolute_filepath, sizeof( game->current_scene_absolute_filepath ) );

  crude_node_manager_creation                              node_manager_creation;
  node_manager_creation = CRUDE_COMPOUNT_EMPTY( crude_node_manager_creation );
  node_manager_creation.world = game->engine->world;
  node_manager_creation.resources_absolute_directory = game->resources_absolute_directory;
  node_manager_creation.temporary_allocator = &game->temporary_allocator;
  node_manager_creation.additional_parse_all_components_to_json_func = game_parse_all_components_to_json_;
  node_manager_creation.additional_parse_json_to_component_func = game_parse_json_to_component_;
  node_manager_creation.physics_resources_manager = &game->physics_resources_manager;
  node_manager_creation.collisions_resources_manager = &game->collision_resources_manager;
  node_manager_creation.allocator = &game->allocator;
  crude_node_manager_initialize( &game->node_manager, &node_manager_creation );
  
  game_setup_custom_preload_nodes_( game );
  game->main_node = crude_node_manager_get_node( &game->node_manager, game->scene_absolute_filepath );
  game_setup_custom_postload_nodes_( game );
}

void
game_initialize_graphics_
(
  _In_ game_t                                            *game
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
  crude_gfx_technique_load_from_file( "game/fullscreen.json", &game->gpu, &game->render_graph, &game->temporary_allocator );
  
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
  scene_renderer_creation.model_renderer_resources_manager = &game->model_renderer_resources_manager;
#if CRUDE_DEVELOP
  scene_renderer_creation.imgui_pass_enalbed = true;
  scene_renderer_creation.imgui_context = game->imgui_context;
#endif
  crude_gfx_scene_renderer_initialize( &game->scene_renderer, &scene_renderer_creation );

  game->scene_renderer.options.hide_collision = true;
  game->scene_renderer.options.hide_debug_gltf = true;
  game->scene_renderer.options.ambient_color = CRUDE_COMPOUNT( XMFLOAT3, { 1, 1, 1 } );
  game->scene_renderer.options.ambient_intensity = 1.5f;
  game->scene_renderer.options.background_intensity = 0.f;
  game->scene_renderer.options.hdr_pre_tonemapping_texture_name = "game_hdr_pre_tonemapping";

  game_setup_custom_postload_model_resources_( game );

  crude_gfx_scene_renderer_update_instances_from_node( &game->scene_renderer, game->main_node );
  crude_gfx_scene_renderer_rebuild_light_gpu_buffers( &game->scene_renderer );
  crude_gfx_model_renderer_resources_manager_wait_till_uploaded( &game->model_renderer_resources_manager );

  crude_gfx_scene_renderer_initialize_pases( &game->scene_renderer );
  crude_gfx_game_postprocessing_pass_initialize( &game->game_postprocessing_pass, &game->scene_renderer );
  crude_gfx_scene_renderer_register_passes( &game->scene_renderer, &game->render_graph );
  crude_gfx_render_graph_builder_register_render_pass( game->render_graph.builder, "game_postprocessing_pass", crude_gfx_game_postprocessing_pass_pack( &game->game_postprocessing_pass ) );

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