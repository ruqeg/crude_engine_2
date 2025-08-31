#include <imgui/imgui.h>
#include <imgui/backends/imgui_impl_sdl3.h>
#include <imgui/backends/imgui_impl_vulkan.h>

#include <core/hash_map.h>
#include <core/file.h>
#include <core/process.h>
#include <platform/platform_system.h>
#include <platform/platform_components.h>
#include <scene/free_camera_system.h>
#include <scene/scene_components.h>
#include <scene/scripts_components.h>
#include <graphics/renderer_resources_loader.h>

#include <game.h>

CRUDE_ECS_SYSTEM_DECLARE( game_graphics_system_ );
CRUDE_ECS_SYSTEM_DECLARE( game_update_system_ );

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
game_graphics_initialize_
(
  _In_ game_t                                             *game,
  _In_ crude_window_handle                                 window_handle
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

  crude_heap_allocator_initialize( &game->allocator, CRUDE_RMEGA( 64 ), "game_allocator" );
  crude_stack_allocator_initialize( &game->temporary_allocator, CRUDE_RMEGA( 64 ), "temprorary_allocator" );
  
  ECS_IMPORT( game->engine->world, crude_platform_system );
  ECS_IMPORT( game->engine->world, crude_free_camera_system );

  {
    IMGUI_CHECKVERSION();
    game->imgui_context = ImGui::CreateContext();
    ImGui::SetCurrentContext( CRUDE_CAST( ImGuiContext*, game->imgui_context ) );
    ImGui::StyleColorsDark();
    ImGuiIO *imgui_io = &ImGui::GetIO();
    imgui_io->ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard | ImGuiConfigFlags_DockingEnable | ImGuiConfigFlags_NoMouseCursorChange;
    imgui_io->ConfigWindowsResizeFromEdges = true;
    imgui_io->ConfigWindowsMoveFromTitleBarOnly = true;
  }

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
  
  /* Create scene */
  {
    crude_scene_creation creation = CRUDE_COMPOUNT_EMPTY( crude_scene_creation );
    creation.world = game->engine->world;
    creation.input_entity = game->platform_node;
    creation.resources_path = "\\..\\..\\resources\\";
    creation.temporary_allocator = &game->temporary_allocator;
    creation.allocator_container = crude_heap_allocator_pack( &game->allocator );
    crude_scene_initialize( &game->scene, &creation );
  }
  crude_scene_load( &game->scene, "scene.json" );

  /* Create Graphics */
  {
    crude_window_handle *window_handle = CRUDE_ENTITY_GET_MUTABLE_COMPONENT( game->platform_node, crude_window_handle );
    game_graphics_initialize_( game, *window_handle );
    CRUDE_ECS_SYSTEM_DEFINE( game->engine->world, game_graphics_system_, EcsPreStore, game, {
      { .id = ecs_id( crude_transform ) },
      { .id = ecs_id( crude_free_camera ) },
    } );
  }
  
  crude_devgui_initialize( &game->devgui, &game->render_graph, &game->renderer, &game->allocator );
  
  CRUDE_ECS_SYSTEM_DEFINE( game->engine->world, game_update_system_, EcsOnUpdate, game, {
    { .id = ecs_id( crude_input ) },
    { .id = ecs_id( crude_window_handle ) },
  } );

  crude_free_camera *free_camera = CRUDE_ENTITY_GET_MUTABLE_COMPONENT( game->scene.main_camera, crude_free_camera );
  free_camera->enabled = true;
}

void
game_update
(
  _In_ game_t                                             *game
)
{
}

void
game_deinitialize
(
  _In_ game_t                                             *game
)
{
  // crude_entity_destroy( game->platform_node );
  crude_scene_deinitialize( &game->scene, false /* should be destroyd with world */ );
  crude_devgui_deinitialize( &game->devgui );
  game_graphics_deinitialize_( game );
  crude_heap_allocator_deinitialize( &game->allocator );
  crude_stack_allocator_deinitialize( &game->temporary_allocator );

  ImGui::DestroyContext( ( ImGuiContext* )game->imgui_context );
}

static void
game_graphics_initialize_
(
  _In_ game_t                                             *game,
  _In_ crude_window_handle                                 window_handle
)
{
  uint32                                                   temporary_allocator_marker;

  temporary_allocator_marker = crude_stack_allocator_get_marker( &game->temporary_allocator );
  
  /* Create Device */
  {
    crude_gfx_device_creation device_creation = CRUDE_COMPOUNT_EMPTY( crude_gfx_device_creation );
    device_creation.sdl_window             = CRUDE_REINTERPRET_CAST( SDL_Window*, window_handle.value );
    device_creation.vk_application_name    = "CrudeEngine";
    device_creation.vk_application_version = VK_MAKE_VERSION( 1, 0, 0 );
    device_creation.allocator_container    = crude_heap_allocator_pack( &game->allocator );
    device_creation.temporary_allocator    = &game->temporary_allocator;
    device_creation.queries_per_frame      = 1u;
    device_creation.num_threads            = CRUDE_STATIC_CAST( uint16, enkiGetNumTaskThreads( CRUDE_REINTERPRET_CAST( enkiTaskScheduler*, game->engine->asynchronous_loader_manager.task_sheduler ) ) );
    crude_gfx_device_initialize( &game->gpu, &device_creation );
  }
  
  /* Create Renderer */
  {
    crude_gfx_renderer_creation renderer_creation = CRUDE_COMPOUNT_EMPTY( crude_gfx_renderer_creation );
    renderer_creation.gpu                 = &game->gpu;
    renderer_creation.allocator_container = crude_heap_allocator_pack( &game->allocator );
    crude_gfx_renderer_initialize( &game->renderer, &renderer_creation );
  }    
  
  /* Create Render Graph */
  crude_gfx_render_graph_builder_initialize( &game->render_graph_builder, &game->gpu );
  crude_gfx_render_graph_initialize( &game->render_graph, &game->render_graph_builder );
  
  /* Create Async Loader */
  {
    crude_gfx_asynchronous_loader_initialize( &game->async_loader, &game->renderer );
    crude_gfx_asynchronous_loader_manager_add_loader( &game->engine->asynchronous_loader_manager, &game->async_loader );
  }
  
  /* Parse render graph*/
  {
    crude_string_buffer                                    temporary_name_buffer;
    char                                                   working_directory[ 512 ];
    char const                                            *render_graph_file_path;

    crude_get_current_working_directory( working_directory, sizeof( working_directory ) );
    crude_string_buffer_initialize( &temporary_name_buffer, 1024, crude_stack_allocator_pack( &game->temporary_allocator ) );
  
    render_graph_file_path = crude_string_buffer_append_use_f( &temporary_name_buffer, "%s%s", working_directory, "\\..\\..\\resources\\render_graph.json", "\\..\\..\\resources\\render_graph.json" );
    crude_gfx_render_graph_parse_from_file( &game->render_graph, render_graph_file_path, &game->temporary_allocator );
    crude_gfx_render_graph_compile( &game->render_graph, &game->temporary_allocator );
  }
  
  /* Create Render Tecnhique & Renderer Passes*/
  crude_gfx_renderer_technique_load_from_file( "\\..\\..\\shaders\\meshlet_technique.json", &game->renderer, &game->render_graph, &game->temporary_allocator );
  crude_gfx_renderer_technique_load_from_file( "\\..\\..\\shaders\\culling_technique.json", &game->renderer, &game->render_graph, &game->temporary_allocator );
  crude_gfx_renderer_technique_load_from_file( "\\..\\..\\shaders\\debug_technique.json", &game->renderer, &game->render_graph, &game->temporary_allocator );
  crude_gfx_renderer_technique_load_from_file( "\\..\\..\\shaders\\fullscreen_technique.json", &game->renderer, &game->render_graph, &game->temporary_allocator );
  crude_gfx_renderer_technique_load_from_file( "\\..\\..\\shaders\\imgui_technique.json", &game->renderer, &game->render_graph, &game->temporary_allocator );

  /* Create Scene Renderer */
  {
    crude_gfx_scene_renderer_creation creation = CRUDE_COMPOUNT_EMPTY( crude_gfx_scene_renderer_creation );
    creation.renderer = &game->renderer;
    creation.async_loader = &game->async_loader;
    creation.allocator = &game->allocator;
    creation.temporary_allocator = &game->temporary_allocator;
    creation.task_scheduler = game->engine->asynchronous_loader_manager.task_sheduler;
    creation.imgui_context = game->imgui_context;
    creation.scene = &game->scene;
    crude_gfx_scene_renderer_initialize( &game->scene_renderer, &creation );
    crude_gfx_scene_renderer_register_passes( &game->scene_renderer, &game->render_graph );
  }
  
  crude_stack_allocator_free_marker( &game->temporary_allocator, temporary_allocator_marker );
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

  crude_devgui_graphics_pre_update( &game->devgui );

  crude_gfx_new_frame( &game->gpu );
  
  ImGui::SetCurrentContext( ( ImGuiContext* ) game->imgui_context );
  ImGui_ImplSDL3_NewFrame();
  ImGui::NewFrame();
  ImGui::DockSpaceOverViewport( 0u, ImGui::GetMainViewport( ) );

  if ( game->gpu.swapchain_resized_last_frame )
  {
    crude_gfx_scene_renderer_on_resize( &game->scene_renderer );
    crude_gfx_render_graph_on_resize( &game->render_graph, game->gpu.vk_swapchain_width, game->gpu.vk_swapchain_height );
    crude_devgui_on_resize( &game->devgui );
  }
  
  crude_devgui_draw( &game->devgui, game->scene.main_node, game->scene.main_camera );
  crude_gfx_scene_renderer_submit_draw_task( &game->scene_renderer, false );
  
  crude_gfx_renderer_add_texture_update_commands( &game->renderer, 1u );

  {
    crude_gfx_texture *final_render_texture = crude_gfx_access_texture( &game->gpu, crude_gfx_render_graph_builder_access_resource_by_name( game->scene_renderer.render_graph->builder, "imgui" )->resource_info.texture.handle );
    crude_gfx_present( &game->gpu, final_render_texture );
  }

  crude_devgui_graphics_post_update( &game->devgui );
}

void
game_graphics_deinitialize_
(
  _In_ game_t                                             *game
)
{
  crude_gfx_asynchronous_loader_manager_remove_loader( &game->engine->asynchronous_loader_manager, &game->async_loader );
  vkDeviceWaitIdle( game->gpu.vk_device );
  crude_gfx_scene_renderer_deinitialize( &game->scene_renderer );
  crude_gfx_asynchronous_loader_deinitialize( &game->async_loader );
  crude_gfx_render_graph_builder_deinitialize( &game->render_graph_builder );
  crude_gfx_render_graph_deinitialize( &game->render_graph );
  crude_gfx_renderer_deinitialize( &game->renderer );
  crude_gfx_device_deinitialize( &game->gpu );
}

void
game_update_system_
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
      ImGuiIO *imgui_io = &ImGui::GetIO();
      imgui_io->ConfigFlags |= ImGuiConfigFlags_NoMouseCursorChange;
      game->wrapwnd.x = input->mouse.wnd.x;
      game->wrapwnd.y = input->mouse.wnd.y;
      SDL_HideCursor();
    }
    
    if ( !input->mouse.right.current && input->mouse.right.current != input->prev_mouse.right.current )
    {
      ImGuiIO *imgui_io = &ImGui::GetIO();
      imgui_io->ConfigFlags ^= ImGuiConfigFlags_NoMouseCursorChange;
      SDL_WarpMouseInWindow( CRUDE_CAST( SDL_Window*, window_handle->value ), game->wrapwnd.x, game->wrapwnd.y );
      SDL_ShowCursor();
    }
    
    if ( input->mouse.rel.x || input->mouse.rel.y )
    {
      if ( !SDL_CursorVisible() )
      {
        SDL_WarpMouseInWindow( CRUDE_CAST( SDL_Window*, window_handle->value ), game->wrapwnd.x, game->wrapwnd.y );
      }
    }
  }
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