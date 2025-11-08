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
#include <physics/physics_system.h>
#include <player_controller_components.h>
#include <player_controller_system.h>

#include <game.h>

CRUDE_ECS_SYSTEM_DECLARE( game_graphics_system_ );
CRUDE_ECS_SYSTEM_DECLARE( game_input_system_ );
CRUDE_ECS_SYSTEM_DECLARE( game_physics_system_ );

static void
game_initialize_allocators_
(
  _In_ game_t                                             *game
);

static void
game_initialize_imgui_
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

static void
game_input_callback_
(
  _In_ void                                               *ctx,
  _In_ void                                               *sdl_event
);

static bool
game_parse_json_to_component_
( 
  _In_ crude_entity                                        node, 
  _In_ cJSON const                                        *component_json,
  _In_ char const                                         *component_name
);

static void
game_parse_all_components_to_json_
( 
  _In_ crude_entity                                        node, 
  _In_ cJSON                                               *node_components_json 
);

static void
game_setup_custom_nodes_to_scene_
( 
  _In_ game_t                                             *game
);

void
game_initialize
(
  _In_ game_t                                             *game,
  _In_ crude_engine                                       *engine
)
{
  game->engine = engine;
  game->framerate = 60;
  game->last_graphics_update_time = 0.f;
  
  ECS_IMPORT( game->engine->world, crude_platform_system );
  ECS_IMPORT( game->engine->world, crude_free_camera_system );
  ECS_IMPORT( game->engine->world, crude_physics_system );
  ECS_IMPORT( game->engine->world, crude_player_controller_system );

  game_initialize_allocators_( game );
  game_initialize_imgui_( game );
  game_initialize_platform_( game );
  game_initialize_physics_( game );
  game_initialize_scene_( game );
  game_initialize_graphics_( game );
  
  crude_devgui_initialize( &game->devgui, game );
  
  CRUDE_ECS_SYSTEM_DEFINE( game->engine->world, game_graphics_system_, EcsPreStore, game, {
    { .id = ecs_id( crude_transform ) },
    { .id = ecs_id( crude_free_camera ) },
  } );

  CRUDE_ECS_SYSTEM_DEFINE( game->engine->world, game_input_system_, EcsOnUpdate, game, {
    { .id = ecs_id( crude_input ) },
    { .id = ecs_id( crude_window_handle ) },
  } );
  
  CRUDE_ECS_SYSTEM_DEFINE( game->engine->world, game_physics_system_, EcsOnUpdate, game, { } );
  
  game_setup_custom_nodes_to_scene_( game );
}

void
game_update
(
  _In_ game_t                                             *game
)
{
  crude_devgui_graphics_post_update( &game->devgui );
}

void
game_deinitialize
(
  _In_ game_t                                             *game
)
{
  crude_devgui_deinitialize( &game->devgui );
  game_graphics_deinitialize_( game );
  crude_physics_deinitialize( );
  crude_scene_deinitialize( &game->scene );
  crude_heap_allocator_deinitialize( &game->cgltf_temporary_allocator );
  crude_heap_allocator_deinitialize( &game->allocator );
  crude_heap_allocator_deinitialize( &game->resources_allocator );
  crude_stack_allocator_deinitialize( &game->model_renderer_resources_manager_temporary_allocator );
  crude_stack_allocator_deinitialize( &game->temporary_allocator );

  ImGui::DestroyContext( ( ImGuiContext* )game->imgui_context );
}

void
game_reload_scene
(
  _In_ game_t                                             *game,
  _In_ char const                                         *filename
)
{
  crude_scene_creation                                     scene_creation;
  crude_gfx_scene_renderer_creation                        scene_renderer_creation;

  vkDeviceWaitIdle( game->gpu.vk_device );
  crude_entity_destroy_hierarchy( game->scene.main_node );
  crude_scene_deinitialize( &game->scene );
  
  crude_gfx_scene_renderer_deinitialize_passes( &game->scene_renderer );

  scene_creation = CRUDE_COMPOUNT_EMPTY( crude_scene_creation );
  scene_creation.world = game->engine->world;
  scene_creation.input_entity = game->platform_node;
  scene_creation.filepath = filename;
  scene_creation.temporary_allocator = &game->temporary_allocator;
  scene_creation.allocator_container = crude_heap_allocator_pack( &game->allocator );
  scene_creation.additional_parse_all_components_to_json_func = game_parse_all_components_to_json_;
  scene_creation.additional_parse_json_to_component_func = game_parse_json_to_component_;
  crude_scene_initialize( &game->scene, &scene_creation );

  game->editor_camera_node = crude_ecs_lookup_entity_from_parent( game->scene.main_node, "editor_camera" );
  game->game_camera_node = crude_ecs_lookup_entity_from_parent( game->scene.main_node, "player" );
  game->focused_camera_node = game->editor_camera_node;

  crude_gfx_scene_renderer_rebuild_main_node( &game->scene_renderer, game->scene.main_node );
  crude_gfx_model_renderer_resources_manager_wait_till_uploaded( &game->model_renderer_resources_manager );

  crude_gfx_scene_renderer_rebuild_meshes_gpu_buffers(  &game->scene_renderer );
  crude_gfx_scene_renderer_initialize_pases( &game->scene_renderer );
  crude_gfx_scene_renderer_register_passes( &game->scene_renderer, &game->render_graph );

  game_setup_custom_nodes_to_scene_( game );
}

void
game_graphics_system_
(
  _In_ ecs_iter_t                                         *it
)
{
  game_t *game = ( game_t* )it->ctx;

  game->last_graphics_update_time += it->delta_time;

  if ( game->last_graphics_update_time < 1.f / game->framerate )
  {
    return;
  }
  game->last_graphics_update_time = 0.f;

  game->scene_renderer.options.camera_node = game->focused_camera_node;

  crude_devgui_graphics_pre_update( &game->devgui );

  crude_gfx_new_frame( &game->gpu );
  
  ImGui::SetCurrentContext( ( ImGuiContext* ) game->imgui_context );
  ImGuizmo::SetImGuiContext( ( ImGuiContext* ) game->imgui_context );
  ImGui_ImplSDL3_NewFrame();
  ImGui::NewFrame();
  ImGuizmo::SetOrthographic( false );
  ImGuizmo::BeginFrame();
  ImGui::DockSpaceOverViewport( 0u, ImGui::GetMainViewport( ) );

  if ( game->gpu.swapchain_resized_last_frame )
  {
    crude_gfx_scene_renderer_on_resize( &game->scene_renderer );
    crude_gfx_render_graph_on_resize( &game->render_graph, game->gpu.vk_swapchain_width, game->gpu.vk_swapchain_height );
    crude_devgui_on_resize( &game->devgui );
  }

  crude_devgui_draw( &game->devgui, game->scene.main_node, game->focused_camera_node );
  crude_gfx_scene_renderer_submit_draw_task( &game->scene_renderer, false );

  {
    crude_gfx_texture *final_render_texture = crude_gfx_access_texture( &game->gpu, crude_gfx_render_graph_builder_access_resource_by_name( game->scene_renderer.render_graph->builder, "imgui" )->resource_info.texture.handle );
    crude_gfx_present( &game->gpu, final_render_texture );
  }
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

  ImGui::SetCurrentContext( CRUDE_CAST( ImGuiContext*, game->imgui_context ) );
  for ( uint32 i = 0; i < it->count; ++i )
  {
    crude_input *input = &input_per_entity[ i ];
    crude_window_handle *window_handle = &window_handle_per_entity[ i ];

    crude_devgui_handle_input( &game->devgui, input );

    if ( input->mouse.right.current && input->mouse.right.current != input->prev_mouse.right.current )
    {
      SDL_GetMouseState( &game->last_unrelative_mouse_position.x, &game->last_unrelative_mouse_position.y );
      SDL_SetWindowRelativeMouseMode( CRUDE_CAST( SDL_Window*, window_handle->value ), true );
    }
    
    if ( !input->mouse.right.current && input->mouse.right.current != input->prev_mouse.right.current )
    {
      SDL_WarpMouseInWindow( CRUDE_CAST( SDL_Window*, window_handle->value ), game->last_unrelative_mouse_position.x, game->last_unrelative_mouse_position.y );
      SDL_SetWindowRelativeMouseMode( CRUDE_CAST( SDL_Window*, window_handle->value ), false );
    }
  }
}

void
game_physics_system_
(
  _In_ ecs_iter_t                                         *it
)
{
  game_t *game = ( game_t* )it->ctx;

  crude_physics_update( it->delta_time );
}

void
game_input_callback_
(
  _In_ void                                               *ctx,
  _In_ void                                               *sdl_event
)
{
  ImGui::SetCurrentContext( CRUDE_CAST( ImGuiContext*, ctx ) );
  ImGui_ImplSDL3_ProcessEvent( CRUDE_CAST( SDL_Event*, sdl_event ) );
}

bool
game_parse_json_to_component_
( 
  _In_ crude_entity                                        node, 
  _In_ cJSON const                                        *component_json,
  _In_ char const                                         *component_name
)
{
  if ( crude_string_cmp( component_name, CRUDE_COMPONENT_STRING( crude_player_controller ) ) == 0 )
  {
    crude_player_controller                                player_controller;
    CRUDE_PARSE_JSON_TO_COMPONENT( crude_player_controller )( &player_controller, component_json );
    CRUDE_ENTITY_SET_COMPONENT( node, crude_player_controller, { player_controller } );
  }
  return true;
}

void
game_parse_all_components_to_json_
( 
  _In_ crude_entity                                        node, 
  _In_ cJSON                                               *node_components_json 
)
{
  crude_player_controller const                           *player_component;
  
  player_component = CRUDE_ENTITY_GET_IMMUTABLE_COMPONENT( node, crude_player_controller );
  if ( player_component )
  {
    cJSON_AddItemToArray( node_components_json, CRUDE_PARSE_COMPONENT_TO_JSON( crude_player_controller )( player_component ) );
  }
}

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
  imgui_io->ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard | ImGuiConfigFlags_DockingEnable;
  imgui_io->ConfigWindowsResizeFromEdges = true;
  imgui_io->ConfigWindowsMoveFromTitleBarOnly = true;
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
    .ctx = game->imgui_context
  } );
}

void
game_initialize_physics_
(
  _In_ game_t                                             *game
)
{
  crude_physics_creation physics_creation = crude_physics_creation_empty( );
  crude_physics_initialize( &physics_creation );
}

void
game_initialize_scene_
(
  _In_ game_t                                             *game
)
{
  uint32                                                   temporary_allocator_marker;
  crude_scene_creation                                     scene_creation;
  crude_string_buffer                                      temporary_string_buffer;
  char                                                     working_directory[ 512 ];

  temporary_allocator_marker = crude_stack_allocator_get_marker( &game->temporary_allocator );

  crude_string_buffer_initialize( &temporary_string_buffer, 1024, crude_stack_allocator_pack( &game->temporary_allocator ) );

  crude_get_current_working_directory( working_directory, sizeof( working_directory ) );

  scene_creation = CRUDE_COMPOUNT_EMPTY( crude_scene_creation );
  scene_creation.world = game->engine->world;
  scene_creation.input_entity = game->platform_node;
  scene_creation.filepath = crude_string_buffer_append_use_f( &temporary_string_buffer, "%s%s%s", working_directory, CRUDE_RESOURCES_DIR, "scene.crude_scene" );
  scene_creation.temporary_allocator = &game->temporary_allocator;
  scene_creation.allocator_container = crude_heap_allocator_pack( &game->allocator );
  scene_creation.additional_parse_all_components_to_json_func = game_parse_all_components_to_json_;
  scene_creation.additional_parse_json_to_component_func = game_parse_json_to_component_;
  crude_scene_initialize( &game->scene, &scene_creation );

  crude_stack_allocator_free_marker( &game->temporary_allocator, temporary_allocator_marker );
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
  char                                                     working_directory[ 512 ];
  crude_gfx_model_renderer_resources_manager_creation      model_renderer_resources_manager_creation;
  crude_gfx_device_creation                                device_creation;
  crude_gfx_scene_renderer_creation                        scene_renderer_creation;
  uint32                                                   temporary_allocator_marker;

  temporary_allocator_marker = crude_stack_allocator_get_marker( &game->temporary_allocator );
  
  crude_get_current_working_directory( working_directory, sizeof( working_directory ) );
  crude_string_buffer_initialize( &temporary_name_buffer, 1024, crude_stack_allocator_pack( &game->temporary_allocator ) );

  window_handle = CRUDE_ENTITY_GET_MUTABLE_COMPONENT( game->platform_node, crude_window_handle );

  device_creation = CRUDE_COMPOUNT_EMPTY( crude_gfx_device_creation );
  device_creation.sdl_window             = CRUDE_REINTERPRET_CAST( SDL_Window*, window_handle->value );
  device_creation.vk_application_name    = "CrudeEngine";
  device_creation.vk_application_version = VK_MAKE_VERSION( 1, 0, 0 );
  device_creation.allocator_container    = crude_heap_allocator_pack( &game->allocator );
  device_creation.temporary_allocator    = &game->temporary_allocator;
  device_creation.queries_per_frame      = 1u;
  device_creation.num_threads            = CRUDE_STATIC_CAST( uint16, enkiGetNumTaskThreads( CRUDE_REINTERPRET_CAST( enkiTaskScheduler*, game->engine->asynchronous_loader_manager.task_sheduler ) ) );
  crude_gfx_device_initialize( &game->gpu, &device_creation );
  
  crude_gfx_render_graph_builder_initialize( &game->render_graph_builder, &game->gpu );
  crude_gfx_render_graph_initialize( &game->render_graph, &game->render_graph_builder );
  
  crude_gfx_asynchronous_loader_initialize( &game->async_loader, &game->gpu );
  crude_gfx_asynchronous_loader_manager_add_loader( &game->engine->asynchronous_loader_manager, &game->async_loader );
  
  render_graph_file_path = crude_string_buffer_append_use_f( &temporary_name_buffer, "%s%s", working_directory, "\\..\\..\\resources\\render_graph.json" );
  crude_gfx_render_graph_parse_from_file( &game->render_graph, render_graph_file_path, &game->temporary_allocator );
  crude_gfx_render_graph_compile( &game->render_graph, &game->temporary_allocator );
  
  crude_gfx_technique_load_from_file( "\\..\\..\\shaders\\deferred_meshlet.json", &game->gpu, &game->render_graph, &game->temporary_allocator );
  crude_gfx_technique_load_from_file( "\\..\\..\\shaders\\pointshadow_meshlet.json", &game->gpu, &game->render_graph, &game->temporary_allocator );
  crude_gfx_technique_load_from_file( "\\..\\..\\shaders\\compute.json", &game->gpu, &game->render_graph, &game->temporary_allocator );
  crude_gfx_technique_load_from_file( "\\..\\..\\shaders\\debug.json", &game->gpu, &game->render_graph, &game->temporary_allocator );
  crude_gfx_technique_load_from_file( "\\..\\..\\shaders\\fullscreen.json", &game->gpu, &game->render_graph, &game->temporary_allocator );
  crude_gfx_technique_load_from_file( "\\..\\..\\shaders\\imgui.json", &game->gpu, &game->render_graph, &game->temporary_allocator );
  
#ifdef CRUDE_GRAPHICS_RAY_TRACING_ENABLED
  crude_gfx_renderer_technique_load_from_file( "\\..\\..\\shaders\\ray_tracing_solid.json", &game->gpu, &game->render_graph, &game->temporary_allocator );
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
  scene_renderer_creation.imgui_context = game->imgui_context;
  scene_renderer_creation.scene = &game->scene;
	scene_renderer_creation.model_renderer_resources_manager = &game->model_renderer_resources_manager;
  crude_gfx_scene_renderer_initialize( &game->scene_renderer, &scene_renderer_creation );

  game->scene_renderer.options.ambient_color = CRUDE_COMPOUNT( XMFLOAT3, { 1, 1, 1 } );
  game->scene_renderer.options.ambient_intensity = 1.5f;

  crude_gfx_scene_renderer_rebuild_main_node( &game->scene_renderer, game->scene.main_node );
  crude_gfx_model_renderer_resources_manager_wait_till_uploaded( &game->model_renderer_resources_manager );

  crude_gfx_scene_renderer_rebuild_meshes_gpu_buffers(  &game->scene_renderer );
  crude_gfx_scene_renderer_initialize_pases( &game->scene_renderer );
  crude_gfx_scene_renderer_register_passes( &game->scene_renderer, &game->render_graph );

  crude_stack_allocator_free_marker( &game->temporary_allocator, temporary_allocator_marker );
}

void
game_setup_custom_nodes_to_scene_
( 
  _In_ game_t                                             *game
)
{
  crude_player_controller                                  *player_controller;
  crude_free_camera                                        *free_camera;

  game->editor_camera_node = crude_ecs_lookup_entity_from_parent( game->scene.main_node, "editor_camera" );
  game->game_controller_node = crude_ecs_lookup_entity_from_parent( game->scene.main_node, "player" );
  game->game_camera_node = crude_ecs_lookup_entity_from_parent( game->scene.main_node, "player.pivot.camera" );
  game->focused_camera_node = game->editor_camera_node;

  player_controller = CRUDE_ENTITY_GET_MUTABLE_COMPONENT( game->game_controller_node, crude_player_controller );
  player_controller->entity_input = game->scene.input_entity;
  free_camera = CRUDE_ENTITY_GET_MUTABLE_COMPONENT( game->editor_camera_node, crude_free_camera );
  free_camera->enabled = true;
}