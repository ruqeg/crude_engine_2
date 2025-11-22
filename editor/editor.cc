#include <imgui/imgui.h>
#include <imgui/backends/imgui_impl_sdl3.h>
#include <imgui/backends/imgui_impl_vulkan.h>
#include <ImGuizmo/ImGuizmo.h>

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
#include <physics/physics_system.h>

#include <game_components.h>

#include <editor.h>

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
  _In_ crude_scene                                        *scene
);

static void
crude_editor_parse_all_components_to_json_
( 
  _In_ crude_entity                                        node, 
  _In_ cJSON                                               *node_components_json 
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
  editor->framerate = 60;
  editor->last_graphics_update_time = 0.f;

  editor->editor_camera_node = CRUDE_COMPOUNT_EMPTY( crude_entity );

  ECS_IMPORT( editor->engine->world, crude_platform_system );
  ECS_IMPORT( editor->engine->world, crude_physics_system );
  ECS_IMPORT( editor->engine->world, crude_free_camera_system );

  crude_editor_initialize_allocators_( editor );
  crude_editor_initialize_imgui_( editor );
  crude_editor_initialize_platform_( editor );
  crude_collisions_resources_manager_instance_allocate( crude_heap_allocator_pack( &editor->allocator ) );
  crude_collisions_resources_manager_initialize( crude_collisions_resources_manager_instance( ), &editor->allocator, &editor->cgltf_temporary_allocator );
  crude_editor_initialize_scene_( editor );
  crude_editor_initialize_graphics_( editor );
  
  crude_devgui_initialize( &editor->devgui, editor );
  
  CRUDE_ARRAY_INITIALIZE_WITH_CAPACITY( editor->commands_queue, 0, crude_heap_allocator_pack( &editor->allocator ) );
  
  CRUDE_ECS_SYSTEM_DEFINE( editor->engine->world, crude_editor_graphics_system_, EcsPreStore, editor, {
    { .id = ecs_id( crude_transform ) },
    { .id = ecs_id( crude_free_camera ) },
  } );
  
  CRUDE_ECS_SYSTEM_DEFINE( editor->engine->world, crude_editor_input_system_, EcsOnUpdate, editor, {
    { .id = ecs_id( crude_input ) },
    { .id = ecs_id( crude_window_handle ) },
  } );
  
  CRUDE_ECS_SYSTEM_DEFINE( editor->engine->world, crude_editor_physics_system_, EcsOnUpdate, editor, { } );
  
  game_setup_custom_nodes_to_scene_( editor );
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
      crude_scene_creation                                 scene_creation;
      crude_gfx_scene_renderer_creation                    scene_renderer_creation;
      bool                                                 buffer_recreated;

      vkDeviceWaitIdle( editor->gpu.vk_device );
      crude_entity_destroy_hierarchy( editor->scene.main_node );
      crude_scene_deinitialize( &editor->scene );

      scene_creation = CRUDE_COMPOUNT_EMPTY( crude_scene_creation );
      scene_creation.world = editor->engine->world;
      scene_creation.input_entity = editor->platform_node;
      scene_creation.filepath = editor->commands_queue[ i ].reload_scene.filepath;
      scene_creation.temporary_allocator = &editor->temporary_allocator;
      scene_creation.allocator_container = crude_heap_allocator_pack( &editor->allocator );
      scene_creation.additional_parse_all_components_to_json_func = crude_editor_parse_all_components_to_json_;
      scene_creation.additional_parse_json_to_component_func = crude_editor_parse_json_to_component_;
      crude_scene_initialize( &editor->scene, &scene_creation );

      buffer_recreated = crude_gfx_scene_renderer_update_instances_from_node( &editor->scene_renderer, editor->scene.main_node );
      crude_gfx_model_renderer_resources_manager_wait_till_uploaded( &editor->model_renderer_resources_manager );

      if ( buffer_recreated )
      {
        crude_gfx_scene_renderer_deinitialize_passes( &editor->scene_renderer );
        crude_gfx_scene_renderer_initialize_pases( &editor->scene_renderer );
        crude_gfx_scene_renderer_register_passes( &editor->scene_renderer, &editor->render_graph );
      }

      game_setup_custom_nodes_to_scene_( editor );

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
        crude_gfx_technique_load_from_file( technique->json_name, &editor->gpu, &editor->render_graph, &editor->temporary_allocator );
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

  crude_collisions_resources_manager_deinitialize( crude_collisions_resources_manager_instance( ) );
  crude_collisions_resources_manager_instance_deallocate( crude_heap_allocator_pack( &editor->allocator ) );
  crude_editor_graphics_deinitialize_( editor );
  crude_scene_deinitialize( &editor->scene );
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
  crude_game_queue_command command = CRUDE_COMPOUNT_EMPTY( crude_game_queue_command );
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
  crude_game_queue_command command = CRUDE_COMPOUNT_EMPTY( crude_game_queue_command );
  command.type = CRUDE_EDITOR_QUEUE_COMMAND_TYPE_RELOAD_TECHNIQUES;
  CRUDE_ARRAY_PUSH( editor->commands_queue, command );
}

void
crude_editor_graphics_system_
(
  _In_ ecs_iter_t                                         *it
)
{
  crude_editor *editor = ( crude_editor* )it->ctx;

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

  crude_devgui_draw( &editor->devgui, editor->scene.main_node, editor->focused_camera_node );
  crude_gfx_scene_renderer_submit_draw_task( &editor->scene_renderer, false );

  {
    crude_gfx_texture *final_render_texture = crude_gfx_access_texture( &editor->gpu, crude_gfx_render_graph_builder_access_resource_by_name( editor->scene_renderer.render_graph->builder, "imgui" )->resource_info.texture.handle );
    crude_gfx_present( &editor->gpu, final_render_texture );
  }
  
  CRUDE_ASSERT( !crude_gfx_scene_renderer_update_instances_from_node( &editor->scene_renderer, editor->scene.main_node ) );
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
  ImGui::SetCurrentContext( CRUDE_CAST( ImGuiContext*, ctx ) );
  ImGui_ImplSDL3_ProcessEvent( CRUDE_CAST( SDL_Event*, sdl_event ) );
}

bool
crude_editor_parse_json_to_component_
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
crude_editor_parse_all_components_to_json_
( 
  _In_ crude_entity                                        node, 
  _In_ cJSON                                               *node_components_json
)
{
  crude_player_controller const                           *player_component;
  crude_enemy const                                       *enemy;
  crude_level_01 const                                    *level01;
  
  player_component = CRUDE_ENTITY_GET_IMMUTABLE_COMPONENT( node, crude_player_controller );
  if ( player_component )
  {
    cJSON_AddItemToArray( node_components_json, CRUDE_PARSE_COMPONENT_TO_JSON( crude_player_controller )( player_component ) );
  }
  
  enemy = CRUDE_ENTITY_GET_IMMUTABLE_COMPONENT( node, crude_enemy );
  if ( enemy )
  {
    cJSON_AddItemToArray( node_components_json, CRUDE_PARSE_COMPONENT_TO_JSON( crude_enemy )( enemy ) );
  }
  
  level01 = CRUDE_ENTITY_GET_IMMUTABLE_COMPONENT( node, crude_level_01 );
  if ( level01 )
  {
    cJSON_AddItemToArray( node_components_json, CRUDE_PARSE_COMPONENT_TO_JSON( crude_level_01 )( level01 ) );
  }
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
    .ctx = editor->imgui_context
  } );
}

void
crude_editor_initialize_scene_
(
  _In_ crude_editor                                             *editor
)
{
  uint32                                                   temporary_allocator_marker;
  crude_scene_creation                                     scene_creation;
  crude_string_buffer                                      temporary_string_buffer;
  char                                                     working_directory[ 512 ];

  temporary_allocator_marker = crude_stack_allocator_get_marker( &editor->temporary_allocator );

  crude_string_buffer_initialize( &temporary_string_buffer, 1024, crude_stack_allocator_pack( &editor->temporary_allocator ) );

  crude_get_current_working_directory( working_directory, sizeof( working_directory ) );

  scene_creation = CRUDE_COMPOUNT_EMPTY( crude_scene_creation );
  scene_creation.world = editor->engine->world;
  scene_creation.input_entity = editor->platform_node;
  scene_creation.filepath = crude_string_buffer_append_use_f( &temporary_string_buffer, "%s%s%s", working_directory, CRUDE_RESOURCES_DIR, "nodes\\test_level.crude_node" );
  scene_creation.temporary_allocator = &editor->temporary_allocator;
  scene_creation.allocator_container = crude_heap_allocator_pack( &editor->allocator );
  scene_creation.additional_parse_all_components_to_json_func = crude_editor_parse_all_components_to_json_;
  scene_creation.additional_parse_json_to_component_func = crude_editor_parse_json_to_component_;
  crude_scene_initialize( &editor->scene, &scene_creation );

  crude_stack_allocator_free_marker( &editor->temporary_allocator, temporary_allocator_marker );
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
  char                                                     working_directory[ 512 ];
  crude_gfx_model_renderer_resources_manager_creation      model_renderer_resources_manager_creation;
  crude_gfx_device_creation                                device_creation;
  crude_gfx_scene_renderer_creation                        scene_renderer_creation;
  uint32                                                   temporary_allocator_marker;

  temporary_allocator_marker = crude_stack_allocator_get_marker( &editor->temporary_allocator );
  
  crude_get_current_working_directory( working_directory, sizeof( working_directory ) );
  crude_string_buffer_initialize( &temporary_name_buffer, 1024, crude_stack_allocator_pack( &editor->temporary_allocator ) );

  window_handle = CRUDE_ENTITY_GET_MUTABLE_COMPONENT( editor->platform_node, crude_window_handle );

  device_creation = CRUDE_COMPOUNT_EMPTY( crude_gfx_device_creation );
  device_creation.sdl_window             = CRUDE_REINTERPRET_CAST( SDL_Window*, window_handle->value );
  device_creation.vk_application_name    = "CrudeEngine";
  device_creation.vk_application_version = VK_MAKE_VERSION( 1, 0, 0 );
  device_creation.allocator_container    = crude_heap_allocator_pack( &editor->allocator );
  device_creation.temporary_allocator    = &editor->temporary_allocator;
  device_creation.queries_per_frame      = 1u;
  device_creation.num_threads            = CRUDE_STATIC_CAST( uint16, enkiGetNumTaskThreads( CRUDE_REINTERPRET_CAST( enkiTaskScheduler*, editor->engine->asynchronous_loader_manager.task_sheduler ) ) );
  crude_gfx_device_initialize( &editor->gpu, &device_creation );
  
  crude_gfx_render_graph_builder_initialize( &editor->render_graph_builder, &editor->gpu );
  crude_gfx_render_graph_initialize( &editor->render_graph, &editor->render_graph_builder );
  
  crude_gfx_asynchronous_loader_initialize( &editor->async_loader, &editor->gpu );
  crude_gfx_asynchronous_loader_manager_add_loader( &editor->engine->asynchronous_loader_manager, &editor->async_loader );
  
  render_graph_file_path = crude_string_buffer_append_use_f( &temporary_name_buffer, "%s%s", working_directory, "\\..\\..\\resources\\render_graph.json" );
  crude_gfx_render_graph_parse_from_file( &editor->render_graph, render_graph_file_path, &editor->temporary_allocator );
  crude_gfx_render_graph_compile( &editor->render_graph, &editor->temporary_allocator );

  crude_gfx_technique_load_from_file( "\\..\\..\\shaders\\deferred_meshlet.json", &editor->gpu, &editor->render_graph, &editor->temporary_allocator );
  crude_gfx_technique_load_from_file( "\\..\\..\\shaders\\pointshadow_meshlet.json", &editor->gpu, &editor->render_graph, &editor->temporary_allocator );
  crude_gfx_technique_load_from_file( "\\..\\..\\shaders\\compute.json", &editor->gpu, &editor->render_graph, &editor->temporary_allocator );
  crude_gfx_technique_load_from_file( "\\..\\..\\shaders\\debug.json", &editor->gpu, &editor->render_graph, &editor->temporary_allocator );
  crude_gfx_technique_load_from_file( "\\..\\..\\shaders\\fullscreen.json", &editor->gpu, &editor->render_graph, &editor->temporary_allocator );
  crude_gfx_technique_load_from_file( "\\..\\..\\shaders\\imgui.json", &editor->gpu, &editor->render_graph, &editor->temporary_allocator );
  
#ifdef CRUDE_GRAPHICS_RAY_TRACING_ENABLED
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
  scene_renderer_creation.imgui_context = editor->imgui_context;
  scene_renderer_creation.scene = &editor->scene;
  scene_renderer_creation.model_renderer_resources_manager = &editor->model_renderer_resources_manager;
  crude_gfx_scene_renderer_initialize( &editor->scene_renderer, &scene_renderer_creation );

  editor->scene_renderer.options.ambient_color = CRUDE_COMPOUNT( XMFLOAT3, { 1, 1, 1 } );
  editor->scene_renderer.options.ambient_intensity = 1.5f;

  crude_gfx_scene_renderer_update_instances_from_node( &editor->scene_renderer, editor->scene.main_node );
  crude_gfx_scene_renderer_rebuild_light_gpu_buffers( &editor->scene_renderer );
  crude_gfx_model_renderer_resources_manager_wait_till_uploaded( &editor->model_renderer_resources_manager );

  crude_gfx_scene_renderer_initialize_pases( &editor->scene_renderer );
  crude_gfx_scene_renderer_register_passes( &editor->scene_renderer, &editor->render_graph );

  crude_stack_allocator_free_marker( &editor->temporary_allocator, temporary_allocator_marker );
}

void
game_setup_custom_nodes_to_scene_
( 
  _In_ crude_editor                                             *editor
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
    XMStoreFloat3( &editor_camera_node_crude_free_camera.moving_speed_multiplier, XMVectorReplicate( 7 ) );
    XMStoreFloat2( &editor_camera_node_crude_free_camera.rotating_speed_multiplier, XMVectorReplicate( -0.001 ) );

    editor->editor_camera_node = crude_entity_create_empty( editor->scene.world, "editor_camera" );
    CRUDE_ENTITY_SET_COMPONENT( editor->editor_camera_node, crude_transform, { editor_camera_node_transform } );
    CRUDE_ENTITY_SET_COMPONENT( editor->editor_camera_node, crude_camera, { editor_camera_node_camera } );
    CRUDE_ENTITY_SET_COMPONENT( editor->editor_camera_node, crude_free_camera, { editor_camera_node_crude_free_camera } );
  }

  editor->editor_camera_node = crude_ecs_lookup_entity_from_parent( editor->scene.main_node, "editor_camera" );
  editor->focused_camera_node = editor->editor_camera_node;
  
  free_camera = CRUDE_ENTITY_GET_MUTABLE_COMPONENT( editor->editor_camera_node, crude_free_camera );
  free_camera->entity_input = editor->scene.input_entity;
  free_camera->enabled = true;
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