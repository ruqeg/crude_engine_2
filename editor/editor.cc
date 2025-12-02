#include <imgui/imgui.h>
#include <imgui/backends/imgui_impl_sdl3.h>
#include <imgui/backends/imgui_impl_vulkan.h>
#include <ImGuizmo/ImGuizmo.h>

#include <engine/core/hash_map.h>
#include <engine/core/file.h>
#include <engine/core/memory.h>
#include <engine/core/process.h>
#include <engine/platform/platform_system.h>
#include <engine/platform/platform_components.h>
#include <engine/scene/free_camera_system.h>
#include <engine/scene/scene_components.h>
#include <engine/scene/scripts_components.h>
#include <engine/graphics/gpu_resources_loader.h>
#include <engine/physics/physics_components.h>
#include <engine/physics/physics_debug_system.h>
#include <engine/external/game_components.h>

#include <editor/editor.h>

crude_editor                                              *crude_editor_instance_;

CRUDE_ECS_SYSTEM_DECLARE( crude_editor_graphics_system_ );
CRUDE_ECS_SYSTEM_DECLARE( crude_editor_input_system_ );
CRUDE_ECS_SYSTEM_DECLARE( crude_editor_physics_system_ );

static void
crude_editor_initialize_allocators_
(
  _In_ crude_editor                                             *editor
);

static void
crude_editor_initialize_imgui_
(
  _In_ crude_editor                                       *editor
);

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
crude_editor_initialize_platform_
(
  _In_ crude_editor                                       *editor
);

static void
crude_editor_initialize_scene_
(
  _In_ crude_editor                                       *editor
);

static void
crude_editor_initialize_graphics_
(
  _In_ crude_editor                                       *editor
);

static void
crude_editor_initialize_physics_
(
  _In_ crude_editor                                       *editor
);

static void
crude_editor_graphics_system_
(
  _In_ ecs_iter_t                                         *it
);

static void
crude_editor_input_system_
(
  _In_ ecs_iter_t                                         *it
);

static void
crude_editor_physics_system_
(
  _In_ ecs_iter_t                                         *it
);

static void
crude_editor_graphics_deinitialize_
(
  _In_ crude_editor                                       *editor
);

static void
crude_editor_input_callback_
(
  _In_ void                                               *ctx,
  _In_ void                                               *sdl_event
);

static bool
crude_editor_parse_json_to_component_
( 
  _In_ crude_entity                                        node, 
  _In_ cJSON const                                        *component_json,
  _In_ char const                                         *component_name,
  _In_ crude_node_manager                                 *manager
);

static void
crude_editor_parse_all_components_to_json_
( 
  _In_ crude_entity                                        node, 
  _In_ cJSON                                              *node_components_json,
  _In_ crude_node_manager                                 *manager
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
  _In_ crude_editor_creation const                        *creation
)
{
  editor->engine = creation->engine;
  editor->framerate = creation->framerate;
  editor->last_graphics_update_time = 0.f;

  editor->editor_camera_node = CRUDE_COMPOUNT_EMPTY( crude_entity );
  editor->added_node_data = CRUDE_COMPOUNT_EMPTY( crude_devgui_added_node_data );
  editor->node_to_add = CRUDE_COMPOUNT_EMPTY( crude_entity );
  editor->node_to_dublicate = CRUDE_COMPOUNT_EMPTY( crude_entity );
  editor->node_to_remove = CRUDE_COMPOUNT_EMPTY( crude_entity );

  ECS_IMPORT( editor->engine->world, crude_platform_system );
  ECS_IMPORT( editor->engine->world, crude_free_camera_system );
  ECS_IMPORT( editor->engine->world, crude_game_components );
  ECS_IMPORT( editor->engine->world, crude_physics_components );

  crude_editor_initialize_allocators_( editor );
  crude_editor_initialize_imgui_( editor );
  crude_editor_initialize_constant_strings_( editor, creation->scene_relative_filepath, creation->render_graph_relative_directory, creation->resources_relative_directory, creation->shaders_relative_directory, creation->techniques_relative_directory, creation->compiled_shaders_relative_directory, creation->working_absolute_directory );

  editor->physics_debug_system_context = CRUDE_COMPOUNT_EMPTY( crude_physics_debug_system_context );
  editor->physics_debug_system_context.resources_absolute_directory = editor->resources_absolute_directory;
  editor->physics_debug_system_context.string_bufffer = &editor->debug_strings_buffer;
  crude_physics_debug_system_import( editor->engine->world, &editor->physics_debug_system_context );
  
  editor->game_debug_system_context = CRUDE_COMPOUNT_EMPTY( crude_game_debug_system_context );
  editor->game_debug_system_context.enemy_spawnpoint_model_absolute_filepath = editor->enemy_spawnpoint_debug_model_absolute_filepath;
  editor->game_debug_system_context.syringe_spawnpoint_model_absolute_filepath = editor->syringe_spawnpoint_debug_model_absolute_filepath;
  crude_game_debug_system_import( editor->engine->world, &editor->game_debug_system_context );

  crude_editor_initialize_platform_( editor );
  crude_collisions_resources_manager_initialize( &editor->collision_resouces_manager, &editor->allocator, &editor->cgltf_temporary_allocator );
  crude_editor_initialize_physics_( editor );
  crude_editor_initialize_scene_( editor );
  crude_editor_initialize_graphics_( editor );
  
  crude_devgui_initialize( &editor->devgui );
  
  CRUDE_ARRAY_INITIALIZE_WITH_CAPACITY( editor->commands_queue, 0, crude_heap_allocator_pack( &editor->allocator ) );
  
  CRUDE_ECS_SYSTEM_DEFINE( editor->engine->world, crude_editor_graphics_system_, EcsPreStore, editor, { } );
  
  CRUDE_ECS_SYSTEM_DEFINE( editor->engine->world, crude_editor_input_system_, EcsOnUpdate, editor, {
    { .id = ecs_id( crude_input ) },
    { .id = ecs_id( crude_window_handle ) },
  } );
  
  CRUDE_ECS_SYSTEM_DEFINE( editor->engine->world, crude_editor_physics_system_, EcsOnUpdate, editor, { } );

  crude_editor_setup_custom_nodes_to_scene_( editor );
}

void
crude_editor_postupdate
(
  _In_ crude_editor                                             *editor
)
{
  for ( uint32 i = 0; i < CRUDE_ARRAY_LENGTH( editor->commands_queue ); ++i )
  {
    switch ( editor->commands_queue[ i ].type )
    {
    case CRUDE_EDITOR_QUEUE_COMMAND_TYPE_RELOAD_SCENE:
    {
      bool                                                 buffer_recreated, model_initialized;

      vkDeviceWaitIdle( editor->gpu.vk_device );

      crude_node_manager_clear( &editor->node_manager );
      crude_gfx_model_renderer_resources_manager_clear( &editor->model_renderer_resources_manager );
      editor->main_node = crude_node_manager_get_node( &editor->node_manager, editor->commands_queue[ i ].reload_scene.filepath );
      editor->selected_node = editor->main_node;

      buffer_recreated = crude_gfx_scene_renderer_update_instances_from_node( &editor->scene_renderer, editor->main_node );
      crude_gfx_model_renderer_resources_manager_wait_till_uploaded( &editor->model_renderer_resources_manager );

      if ( buffer_recreated )
      {
        crude_gfx_render_graph_on_techniques_reloaded( &editor->render_graph );
      }

      NFD_FreePathU8( editor->commands_queue[ i ].reload_scene.filepath );
      break;
    }
    case CRUDE_EDITOR_QUEUE_COMMAND_TYPE_RELOAD_TECHNIQUES:
    {
      for ( uint32 i = 0; i < CRUDE_HASHMAP_CAPACITY( editor->gpu.resource_cache.techniques ); ++i )
      {
        if ( !crude_hashmap_backet_key_valid( editor->gpu.resource_cache.techniques[ i ].key ) )
        {
          continue;
        }
        
        crude_gfx_technique *technique = editor->gpu.resource_cache.techniques[ i ].value; 
        crude_gfx_destroy_technique_instant( &editor->gpu, technique );
        crude_gfx_technique_load_from_file( technique->technique_relative_filepath, &editor->gpu, &editor->render_graph, &editor->temporary_allocator );
      }

      crude_gfx_render_graph_on_techniques_reloaded( &editor->render_graph );
      break;
    }
    }
  }

  CRUDE_ARRAY_SET_LENGTH( editor->commands_queue, 0u );
}

void
crude_editor_deinitialize
(
  _In_ crude_editor                                             *editor
)
{
  crude_devgui_deinitialize( &editor->devgui );
  
  CRUDE_ARRAY_DEINITIALIZE( editor->commands_queue );

  crude_collisions_resources_manager_deinitialize( &editor->collision_resouces_manager );
  crude_editor_graphics_deinitialize_( editor );
  crude_node_manager_deinitialize( &editor->node_manager );
  crude_editor_deinitialize_constant_strings_( editor );
  crude_physics_resources_manager_deinitialize( &editor->physics_resouces_manager );
  crude_heap_allocator_deinitialize( &editor->cgltf_temporary_allocator );
  crude_heap_allocator_deinitialize( &editor->allocator );
  crude_heap_allocator_deinitialize( &editor->resources_allocator );
  crude_stack_allocator_deinitialize( &editor->model_renderer_resources_manager_temporary_allocator );
  crude_stack_allocator_deinitialize( &editor->temporary_allocator );

  ImGui::DestroyContext( ( ImGuiContext* )editor->imgui_context );
}

void
crude_editor_push_reload_scene_command
(
  _In_ crude_editor                                             *editor,
  _In_ nfdu8char_t                                        *filepath
)
{
  crude_editor_queue_command command = CRUDE_COMPOUNT_EMPTY( crude_editor_queue_command );
  command.type = CRUDE_EDITOR_QUEUE_COMMAND_TYPE_RELOAD_SCENE;
  command.reload_scene.filepath = filepath;
  CRUDE_ARRAY_PUSH( editor->commands_queue, command );
}

void
crude_editor_push_reload_techniques_command
(
  _In_ crude_editor                                             *editor
)
{
  crude_editor_queue_command command = CRUDE_COMPOUNT_EMPTY( crude_editor_queue_command );
  command.type = CRUDE_EDITOR_QUEUE_COMMAND_TYPE_RELOAD_TECHNIQUES;
  CRUDE_ARRAY_PUSH( editor->commands_queue, command );
}

void
crude_editor_graphics_system_
(
  _In_ ecs_iter_t                                         *it
)
{
  crude_editor                                            *editor;
  crude_gfx_texture                                       *final_render_texture;
  
  editor = CRUDE_CAST( crude_editor*, it->ctx );
  editor->last_graphics_update_time += it->delta_time;

  if ( editor->last_graphics_update_time < 1.f / editor->framerate )
  {
    return;
  }
  editor->last_graphics_update_time = 0.f;

  editor->scene_renderer.options.camera_node = editor->focused_camera_node;
  
  crude_devgui_graphics_pre_update( &editor->devgui );

  crude_gfx_new_frame( &editor->gpu );
  
  ImGui::SetCurrentContext( ( ImGuiContext* ) editor->imgui_context );
  ImGuizmo::SetImGuiContext( ( ImGuiContext* ) editor->imgui_context );
  ImGui_ImplSDL3_NewFrame();
  ImGui::NewFrame();
  ImGuizmo::SetOrthographic( false );
  ImGuizmo::BeginFrame();
  ImGui::DockSpaceOverViewport( 0u, ImGui::GetMainViewport( ) );

  if ( editor->gpu.swapchain_resized_last_frame )
  {
    crude_gfx_scene_renderer_on_resize( &editor->scene_renderer );
    crude_gfx_render_graph_on_resize( &editor->render_graph, editor->gpu.vk_swapchain_width, editor->gpu.vk_swapchain_height );
    crude_devgui_on_resize( &editor->devgui );
  }

  crude_devgui_draw( &editor->devgui );
  crude_gfx_scene_renderer_submit_draw_task( &editor->scene_renderer, false );

  final_render_texture = crude_gfx_access_texture( &editor->gpu, crude_gfx_render_graph_builder_access_resource_by_name( editor->scene_renderer.render_graph->builder, "imgui" )->resource_info.texture.handle );
  crude_gfx_present( &editor->gpu, final_render_texture );
  
  CRUDE_ASSERT( !crude_gfx_scene_renderer_update_instances_from_node( &editor->scene_renderer, editor->main_node ) );
  crude_gfx_model_renderer_resources_manager_wait_till_uploaded( &editor->model_renderer_resources_manager );
}

void
crude_editor_graphics_deinitialize_
(
  _In_ crude_editor                                       *editor
)
{
  crude_gfx_asynchronous_loader_manager_remove_loader( &editor->engine->asynchronous_loader_manager, &editor->async_loader );
  vkDeviceWaitIdle( editor->gpu.vk_device );
  crude_gfx_scene_renderer_deinitialize_passes( &editor->scene_renderer );
  crude_gfx_scene_renderer_deinitialize( &editor->scene_renderer );
  crude_gfx_asynchronous_loader_deinitialize( &editor->async_loader );
  crude_gfx_model_renderer_resources_manager_deintialize( &editor->model_renderer_resources_manager );
  crude_gfx_render_graph_builder_deinitialize( &editor->render_graph_builder );
  crude_gfx_render_graph_deinitialize( &editor->render_graph );
  crude_gfx_device_deinitialize( &editor->gpu );
}

void
crude_editor_input_system_
(
  _In_ ecs_iter_t                                         *it
)
{
  crude_editor *editor = ( crude_editor* )it->ctx;
  crude_input *input_per_entity = ecs_field( it, crude_input, 0 );
  crude_window_handle *window_handle_per_entity = ecs_field( it, crude_window_handle, 1 );

  ImGui::SetCurrentContext( CRUDE_CAST( ImGuiContext*, editor->imgui_context ) );
  for ( uint32 i = 0; i < it->count; ++i )
  {
    crude_input *input = &input_per_entity[ i ];
    crude_window_handle *window_handle = &window_handle_per_entity[ i ];

    crude_devgui_handle_input( &editor->devgui, input );

    if ( input->mouse.right.current && input->mouse.right.current != input->prev_mouse.right.current )
    {
      SDL_GetMouseState( &editor->last_unrelative_mouse_position.x, &editor->last_unrelative_mouse_position.y );
      SDL_SetWindowRelativeMouseMode( CRUDE_CAST( SDL_Window*, window_handle->value ), true );
    }
    
    if ( !input->mouse.right.current && input->mouse.right.current != input->prev_mouse.right.current )
    {
      SDL_WarpMouseInWindow( CRUDE_CAST( SDL_Window*, window_handle->value ), editor->last_unrelative_mouse_position.x, editor->last_unrelative_mouse_position.y );
      SDL_SetWindowRelativeMouseMode( CRUDE_CAST( SDL_Window*, window_handle->value ), false );
    }
  }
}

void
crude_editor_physics_system_
(
  _In_ ecs_iter_t                                         *it
)
{
  crude_editor *editor = ( crude_editor* )it->ctx;
}

void
crude_editor_input_callback_
(
  _In_ void                                               *ctx,
  _In_ void                                               *sdl_event
)
{
  crude_editor *editor = CRUDE_CAST( crude_editor*, ctx );

  ImGui::SetCurrentContext( CRUDE_CAST( ImGuiContext*, editor->imgui_context ) );
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
crude_editor_initialize_allocators_
(
  _In_ crude_editor                                             *editor
)
{
  crude_heap_allocator_initialize( &editor->cgltf_temporary_allocator, CRUDE_RMEGA( 16 ), "cgltf_temporary_allocator" );
  crude_heap_allocator_initialize( &editor->allocator, CRUDE_RMEGA( 16 ), "common_allocator" );
  crude_heap_allocator_initialize( &editor->resources_allocator, CRUDE_RMEGA( 16 ), "resources_allocator" );
  crude_stack_allocator_initialize( &editor->temporary_allocator, CRUDE_RMEGA( 16 ), "temprorary_allocator" );
  crude_stack_allocator_initialize( &editor->model_renderer_resources_manager_temporary_allocator, CRUDE_RMEGA( 64 ), "model_renderer_resources_manager_temporary_allocator" );
}

void
crude_editor_initialize_imgui_
(
  _In_ crude_editor                                             *editor
)
{
  ImGuiIO                                                 *imgui_io;

  IMGUI_CHECKVERSION();
  editor->imgui_context = ImGui::CreateContext();
  ImGui::SetCurrentContext( CRUDE_CAST( ImGuiContext*, editor->imgui_context ) );
  ImGui::StyleColorsDark();
  imgui_io = &ImGui::GetIO();
  imgui_io->ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard | ImGuiConfigFlags_DockingEnable;
  imgui_io->ConfigWindowsResizeFromEdges = true;
  imgui_io->ConfigWindowsMoveFromTitleBarOnly = true;
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
  
  crude_string_buffer_initialize( &editor->dynamic_strings_buffer, 4096, crude_heap_allocator_pack( &editor->allocator ) );
  crude_string_buffer_initialize( &editor->debug_strings_buffer, 4096, crude_heap_allocator_pack( &editor->allocator ) );
  crude_string_buffer_initialize( &editor->debug_constant_strings_buffer, 4096, crude_heap_allocator_pack( &editor->allocator ) );

  crude_string_buffer_initialize( &editor->constant_strings_buffer, constant_string_buffer_size, crude_heap_allocator_pack( &editor->allocator ) );
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
crude_editor_initialize_platform_
(
  _In_ crude_editor                                             *editor
)
{
  editor->platform_node = crude_entity_create_empty( editor->engine->world, "editor" );
  CRUDE_ENTITY_SET_COMPONENT( editor->platform_node, crude_window, { 
    .width     = 800,
    .height    = 600,
    .maximized = false,
    .flags     = SDL_WINDOW_VULKAN | SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIGH_PIXEL_DENSITY
  });
  CRUDE_ENTITY_SET_COMPONENT( editor->platform_node, crude_input, {
    .callback = crude_editor_input_callback_,
    .ctx = editor
  } );
}

void
crude_editor_initialize_scene_
(
  _In_ crude_editor                                             *editor
)
{
  crude_node_manager_creation                              node_manager_creation;
  node_manager_creation = CRUDE_COMPOUNT_EMPTY( crude_node_manager_creation );
  node_manager_creation.world = editor->engine->world;
  node_manager_creation.resources_absolute_directory = editor->resources_absolute_directory;
  node_manager_creation.temporary_allocator = &editor->temporary_allocator;
  node_manager_creation.additional_parse_all_components_to_json_func = crude_editor_parse_all_components_to_json_;
  node_manager_creation.additional_parse_json_to_component_func = crude_editor_parse_json_to_component_;
  node_manager_creation.physics_resources_manager = &editor->physics_resouces_manager;
  node_manager_creation.collisions_resources_manager = &editor->collision_resouces_manager;
  node_manager_creation.allocator = &editor->allocator;
  crude_node_manager_initialize( &editor->node_manager, &node_manager_creation );

  editor->main_node = crude_node_manager_get_node( &editor->node_manager, editor->scene_absolute_filepath );
  editor->selected_node = editor->main_node;
}

void
crude_editor_initialize_graphics_
(
  _In_ crude_editor                                             *editor
)
{
  crude_window_handle                                     *window_handle;
  char const                                              *render_graph_file_path;
  crude_string_buffer                                      temporary_name_buffer;
  crude_gfx_model_renderer_resources_manager_creation      model_renderer_resources_manager_creation;
  crude_gfx_device_creation                                device_creation;
  crude_gfx_scene_renderer_creation                        scene_renderer_creation;
  uint32                                                   temporary_allocator_marker;

  temporary_allocator_marker = crude_stack_allocator_get_marker( &editor->temporary_allocator );
  
  crude_string_buffer_initialize( &temporary_name_buffer, crude_string_length( editor->scene_absolute_filepath ) + 512, crude_stack_allocator_pack( &editor->temporary_allocator ) );

  window_handle = CRUDE_ENTITY_GET_MUTABLE_COMPONENT( editor->platform_node, crude_window_handle );

  device_creation = CRUDE_COMPOUNT_EMPTY( crude_gfx_device_creation );
  device_creation.sdl_window = CRUDE_REINTERPRET_CAST( SDL_Window*, window_handle->value );
  device_creation.vk_application_name = "CrudeEngine";
  device_creation.vk_application_version = VK_MAKE_VERSION( 1, 0, 0 );
  device_creation.allocator_container = crude_heap_allocator_pack( &editor->allocator );
  device_creation.temporary_allocator = &editor->temporary_allocator;
  device_creation.queries_per_frame = 1u;
  device_creation.num_threads = CRUDE_STATIC_CAST( uint16, enkiGetNumTaskThreads( CRUDE_REINTERPRET_CAST( enkiTaskScheduler*, editor->engine->asynchronous_loader_manager.task_sheduler ) ) );
  device_creation.shaders_absolute_directory = editor->shaders_absolute_directory;
  device_creation.techniques_absolute_directory = editor->techniques_absolute_directory;
  device_creation.compiled_shaders_absolute_directory = editor->compiled_shaders_absolute_directory;
  crude_gfx_device_initialize( &editor->gpu, &device_creation );
  
  crude_gfx_render_graph_builder_initialize( &editor->render_graph_builder, &editor->gpu );
  crude_gfx_render_graph_initialize( &editor->render_graph, &editor->render_graph_builder );
  
  crude_gfx_asynchronous_loader_initialize( &editor->async_loader, &editor->gpu );
  crude_gfx_asynchronous_loader_manager_add_loader( &editor->engine->asynchronous_loader_manager, &editor->async_loader );
  
  render_graph_file_path = crude_string_buffer_append_use_f( &temporary_name_buffer, "%s%s", editor->resources_absolute_directory, "editor\\render_graph.json" );
  crude_gfx_render_graph_parse_from_file( &editor->render_graph, render_graph_file_path, &editor->temporary_allocator );
  crude_gfx_render_graph_compile( &editor->render_graph, &editor->temporary_allocator );

  crude_gfx_technique_load_from_file( "deferred_meshlet.json", &editor->gpu, &editor->render_graph, &editor->temporary_allocator );
  crude_gfx_technique_load_from_file( "pointshadow_meshlet.json", &editor->gpu, &editor->render_graph, &editor->temporary_allocator );
  crude_gfx_technique_load_from_file( "compute.json", &editor->gpu, &editor->render_graph, &editor->temporary_allocator );
  crude_gfx_technique_load_from_file( "debug.json", &editor->gpu, &editor->render_graph, &editor->temporary_allocator );
  crude_gfx_technique_load_from_file( "fullscreen.json", &editor->gpu, &editor->render_graph, &editor->temporary_allocator );
  crude_gfx_technique_load_from_file( "imgui.json", &editor->gpu, &editor->render_graph, &editor->temporary_allocator );
  
#if CRUDE_GRAPHICS_RAY_TRACING_ENABLED
  crude_gfx_renderer_technique_load_from_file( "\\..\\..\\shaders\\ray_tracing_solid.json", &editor->gpu, &editor->render_graph, &editor->temporary_allocator );
#endif /* CRUDE_GRAPHICS_RAY_TRACING_ENABLED */

  model_renderer_resources_manager_creation = CRUDE_COMPOUNT_EMPTY( crude_gfx_model_renderer_resources_manager_creation );
  model_renderer_resources_manager_creation.allocator = &editor->allocator;
  model_renderer_resources_manager_creation.async_loader = &editor->async_loader;
  model_renderer_resources_manager_creation.cgltf_temporary_allocator = &editor->cgltf_temporary_allocator;
  model_renderer_resources_manager_creation.temporary_allocator = &editor->model_renderer_resources_manager_temporary_allocator;
  model_renderer_resources_manager_creation.world = editor->engine->world;
  crude_gfx_model_renderer_resources_manager_intialize( &editor->model_renderer_resources_manager, &model_renderer_resources_manager_creation );

  scene_renderer_creation = CRUDE_COMPOUNT_EMPTY( crude_gfx_scene_renderer_creation );
  scene_renderer_creation.async_loader = &editor->async_loader;
  scene_renderer_creation.allocator = &editor->allocator;
  scene_renderer_creation.temporary_allocator = &editor->temporary_allocator;
  scene_renderer_creation.task_scheduler = editor->engine->asynchronous_loader_manager.task_sheduler;
  scene_renderer_creation.model_renderer_resources_manager = &editor->model_renderer_resources_manager;
  scene_renderer_creation.imgui_pass_enalbed = true;
  scene_renderer_creation.imgui_context = editor->imgui_context;
  crude_gfx_scene_renderer_initialize( &editor->scene_renderer, &scene_renderer_creation );

  editor->scene_renderer.options.ambient_color = CRUDE_COMPOUNT( XMFLOAT3, { 1, 1, 1 } );
  editor->scene_renderer.options.ambient_intensity = 1.5f;

  crude_gfx_scene_renderer_update_instances_from_node( &editor->scene_renderer, editor->main_node );
  crude_gfx_scene_renderer_rebuild_light_gpu_buffers( &editor->scene_renderer );
  crude_gfx_model_renderer_resources_manager_wait_till_uploaded( &editor->model_renderer_resources_manager );

  crude_gfx_scene_renderer_initialize_pases( &editor->scene_renderer );
  crude_gfx_scene_renderer_register_passes( &editor->scene_renderer, &editor->render_graph );

  crude_stack_allocator_free_marker( &editor->temporary_allocator, temporary_allocator_marker );
}

void
crude_editor_initialize_physics_
(
  _In_ crude_editor                                       *editor
)
{
  crude_physics_resources_manager_creation creation = CRUDE_COMPOUNT_EMPTY( crude_physics_resources_manager_creation );
  creation.allocator = &editor->allocator;
  crude_physics_resources_manager_initialize( &editor->physics_resouces_manager, &creation );
}

void
crude_editor_setup_custom_nodes_to_scene_
( 
  _In_ crude_editor                                       *editor
)
{
  crude_player_controller                                  *player_controller;
  crude_free_camera                                        *free_camera;

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
    editor_camera_node_crude_free_camera.input_node = editor->platform_node;

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