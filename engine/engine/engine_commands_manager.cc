#include <engine/engine.h>

#include <engine/core/array.h>

#include <engine/engine/engine_commands_manager.h>

void
crude_engine_commands_manager_initialize
(
  _In_ crude_engine_commands_manager                      *manager,
  _In_ crude_engine                                       *engine,
  _In_ crude_heap_allocator                               *allocator
)
{
  CRUDE_ARRAY_INITIALIZE_WITH_LENGTH( manager->commands_queue, 0, crude_heap_allocator_pack( allocator ) );
}

void
crude_engine_commands_manager_deinitialize
(
  _In_ crude_engine_commands_manager                      *manager
)
{
  CRUDE_ARRAY_DEINITIALIZE( manager->commands_queue );
}

void
crude_engine_commands_manager_push_reload_scene_command
(
  _In_ crude_engine_commands_manager                      *manager
)
{
  crude_engine_commands_manager_queue_command command = CRUDE_COMPOUNT_EMPTY( crude_engine_commands_manager_queue_command );
  command.type = CRUDE_ENGINE_COMMANDS_MANAGER_QUEUE_COMMAND_TYPE_RELOAD_SCENE;
  CRUDE_ARRAY_PUSH( manager->commands_queue, command ); 
}

void
crude_engine_commands_manager_push_load_scene_command
(
  _In_ crude_engine_commands_manager                      *manager,
  _In_ char const                                         *absolute_filepath
)
{
  crude_engine_commands_manager_queue_command command = CRUDE_COMPOUNT_EMPTY( crude_engine_commands_manager_queue_command );
  command.type = CRUDE_ENGINE_COMMANDS_MANAGER_QUEUE_COMMAND_TYPE_LOAD_SCENE;
  command.load_scene.absolute_filepath = absolute_filepath;
  CRUDE_ARRAY_PUSH( manager->commands_queue, command );
}

void
crude_engine_commands_manager_push_reload_techniques_command
(
  _In_ crude_engine_commands_manager                      *manager
)
{
}

void
crude_engine_commands_manager_update
(
  _In_ crude_engine_commands_manager                      *manager
)
{
//  for ( uint32 i = 0; i < CRUDE_ARRAY_LENGTH( manager->commands_queue ); ++i )
//  {
//    switch ( manager->commands_queue[ i ].type )
//    {
//    case CRUDE_ENGINE_COMMANDS_MANAGER_QUEUE_COMMAND_TYPE_RELOAD_SCENE:
//    {
//      bool                                                 buffer_recreated;
//
//      crude_graphics_thread_manager_lock( &manager->engine->___graphics_thread_manager );
//      crude_ecs *world = crude_scene_thread_manager_lock_world( &manager->engine->___scene_thread_manager );
//      vkDeviceWaitIdle( manager->engine->___graphics_thread_manager.gpu.vk_device );
//      
//      crude_node_manager_clear( &manager->engine->node_manager, world );
//      crude_physics_resources_manager_clear( &manager->engine->physics_resources_manager );
//#if CRUDE_DEVELOP
//      crude_string_buffer_clear( &manager->engine->debug_strings_buffer );
//#endif
//      crude_string_buffer_clear( &manager->engine->game_strings_buffer );
//      game_setup_custom_preload_nodes_( game );
//      game->main_node = crude_node_manager_get_node( &manager->engine->node_manager, game->current_scene_absolute_filepath );
//      game_setup_custom_postload_nodes_( game );
//
//      crude_gfx_scene_renderer_update_instances_from_node( &game->scene_renderer, game->main_node );
//      
//      crude_audio_device_sound_stop( &game->audio_device, game->death_sound_handle );
//      game->death_screen = false;
//      game->death_overlap_color.w = 0;
//
//      crude_graphics_thread_manager_unlock( &manager->engine->___graphics_thread_manager );
//      crude_scene_thread_manager_unlock_world( &manager->engine->___scene_thread_manager );
//      break;
//    }
//    case CRUDE_ENGINE_COMMANDS_MANAGER_QUEUE_COMMAND_TYPE_LOAD_SCENE:
//    {
//      vkDeviceWaitIdle( game->gpu.vk_device );
//
//      crude_node_manager_clear( &game->node_manager );
//      crude_physics_resources_manager_clear( &game->physics_resources_manager );
//      crude_gfx_model_renderer_resources_manager_clear( &game->model_renderer_resources_manager );
//      
//#if CRUDE_DEVELOP
//      crude_string_buffer_clear( &game->debug_strings_buffer );
//#endif
//      crude_string_buffer_clear( &game->game_strings_buffer );
//      game_setup_custom_preload_nodes_( game );
//      game->main_node = crude_node_manager_get_node( &game->node_manager, game->commands_queue[ i ].load_scene.absolute_filepath );
//      game->current_scene_absolute_filepath = game->commands_queue[ i ].load_scene.absolute_filepath;
//      game_setup_custom_postload_nodes_( game );
//
//      bool buffer_recreated = crude_gfx_scene_renderer_update_instances_from_node( &game->scene_renderer, game->main_node );
//      crude_gfx_model_renderer_resources_manager_wait_till_uploaded( &game->model_renderer_resources_manager );
//
//      if ( buffer_recreated )
//      {
//        crude_gfx_render_graph_on_techniques_reloaded( &game->render_graph );
//      }
//
//      crude_audio_device_sound_stop( &game->audio_device, game->death_sound_handle );
//      game->death_screen = false;
//      game->death_overlap_color.w = 0;
//      break;
//    }
//    case CRUDE_ENGINE_COMMANDS_MANAGER_QUEUE_COMMAND_TYPE_RELOAD_TECHNIQUES:
//    {
//      for ( uint32 i = 0; i < CRUDE_HASHMAP_CAPACITY( game->gpu.resource_cache.techniques ); ++i )
//      {
//        if ( !crude_hashmap_backet_key_valid( game->gpu.resource_cache.techniques[ i ].key ) )
//        {
//          continue;
//        }
//        
//        crude_gfx_technique *technique = game->gpu.resource_cache.techniques[ i ].value; 
//        crude_gfx_destroy_technique_instant( &game->gpu, technique );
//        crude_gfx_technique_load_from_file( technique->technique_relative_filepath, &game->gpu, &game->render_graph, &game->temporary_allocator );
//      }
//
//      crude_gfx_render_graph_on_techniques_reloaded( &game->render_graph );
//      break;
//    }
//    }
//  }
//
//  CRUDE_ARRAY_SET_LENGTH( manager->commands_queue, 0u );
}