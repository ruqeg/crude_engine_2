#include <imgui.h>
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

#include <paprika.h>

CRUDE_ECS_SYSTEM_DECLARE( paprika_graphics_system_ );
CRUDE_ECS_SYSTEM_DECLARE( paprika_update_system_ );

static void
paprika_graphics_system_
(
  _In_ ecs_iter_t                                         *it
);

static void
paprika_update_system_
(
  _In_ ecs_iter_t                                         *it
);

static void
paprika_graphics_initialize_
(
  _In_ crude_paprika                                      *paprika,
  _In_ crude_window_handle                                 window_handle
);

static void
paprika_graphics_deinitialize_
(
  _In_ crude_paprika                                      *paprika
);

static void
paprika_input_callback_
(
  _In_ void                                               *ctx,
  _In_ void                                               *sdl_event
);

static void
paprika_draw_imgui_
(
  _In_ crude_paprika                                      *paprika
);

void
crude_paprika_initialize
(
  _In_ crude_paprika                                      *paprika,
  _In_ crude_engine                                       *engine
)
{
  paprika->working = true;
  paprika->engine = engine;
  paprika->framerate = 60;
  paprika->last_graphics_update_time = 0.f;

  crude_heap_allocator_initialize( &paprika->graphics_allocator, CRUDE_RMEGA( 64 ), "renderer_allocator" );
  crude_stack_allocator_initialize( &paprika->temporary_allocator, CRUDE_RMEGA( 64 ), "temprorary_allocator" );
  
  ECS_IMPORT( paprika->engine->world, crude_platform_system );
  ECS_IMPORT( paprika->engine->world, crude_free_camera_system );

  {
    IMGUI_CHECKVERSION();
    paprika->imgui_context = ImGui::CreateContext();
    ImGui::SetCurrentContext( CRUDE_CAST( ImGuiContext*, paprika->imgui_context ) );
    ImGui::StyleColorsDark();
    ImGuiIO *imgui_io = &ImGui::GetIO();
    imgui_io->ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard | ImGuiConfigFlags_DockingEnable | ImGuiConfigFlags_NoMouseCursorChange;
    imgui_io->ConfigWindowsResizeFromEdges = true;
    imgui_io->ConfigWindowsMoveFromTitleBarOnly = true;
  }

  paprika->platform_node = crude_entity_create_empty( paprika->engine->world, "paprika" );
  CRUDE_ENTITY_SET_COMPONENT( paprika->platform_node, crude_window, { 
    .width     = 800,
    .height    = 600,
    .maximized = false,
    .flags     = SDL_WINDOW_VULKAN | SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIGH_PIXEL_DENSITY
  });
  CRUDE_ENTITY_SET_COMPONENT( paprika->platform_node, crude_input, {
    .callback = paprika_input_callback_,
    .ctx = paprika->imgui_context
  } );
  
  /* Create scene */
  {
    crude_scene_creation creation = {
      .world = paprika->engine->world,
      .input_entity = paprika->platform_node,
      .resources_path = "\\..\\..\\resources\\",
      .temporary_allocator = &paprika->temporary_allocator,
      .allocator_container = crude_heap_allocator_pack( &paprika->graphics_allocator ), 
    };
    crude_scene_initialize( &paprika->scene, &creation );
  }
  crude_scene_load( &paprika->scene, "scene.json" );

  /* Create Graphics */
  {
    crude_window_handle *window_handle = CRUDE_ENTITY_GET_MUTABLE_COMPONENT( paprika->platform_node, crude_window_handle );
    paprika_graphics_initialize_( paprika, *window_handle );
    CRUDE_ECS_SYSTEM_DEFINE( paprika->engine->world, paprika_graphics_system_, EcsPreStore, paprika, {
      { .id = ecs_id( crude_transform ) },
      { .id = ecs_id( crude_free_camera ) },
    } );
  }
  
  crude_devgui_initialize( &paprika->devgui, &paprika->graphics.render_graph, &paprika->graphics.renderer );
  
  CRUDE_ECS_SYSTEM_DEFINE( paprika->engine->world, paprika_update_system_, EcsOnUpdate, paprika, {
    { .id = ecs_id( crude_input ) },
    { .id = ecs_id( crude_window_handle ) },
  } );

  crude_free_camera *free_camera = CRUDE_ENTITY_GET_MUTABLE_COMPONENT( paprika->scene.main_camera, crude_free_camera );
  free_camera->enabled = true;
}

void
crude_paprika_deinitialize
(
  _In_ crude_paprika                                      *paprika
)
{
  if ( !paprika->working )
  {
    return;
  }
  paprika->working = false;
  
  crude_entity_destroy( paprika->platform_node );
  crude_scene_deinitialize( &paprika->scene );
  paprika_graphics_deinitialize_( paprika );
  crude_heap_allocator_deinitialize( &paprika->graphics_allocator );
  crude_stack_allocator_deinitialize( &paprika->temporary_allocator );

  ImGui::DestroyContext( ( ImGuiContext* )paprika->imgui_context );
}

static void
paprika_graphics_initialize_
(
  _In_ crude_paprika                                      *paprika,
  _In_ crude_window_handle                                 window_handle
)
{
  uint32                                                   temporary_allocator_marker;

  temporary_allocator_marker = crude_stack_allocator_get_marker( &paprika->temporary_allocator );
  
  paprika->graphics.asynchronous_loader_manager = &paprika->engine->asynchronous_loader_manager;
  paprika->graphics.allocator_container = crude_heap_allocator_pack( &paprika->graphics_allocator );
  
  /* Create Device */
  {
    crude_gfx_device_creation device_creation = {
      .sdl_window             = CRUDE_REINTERPRET_CAST( SDL_Window*, window_handle.value ),
      .vk_application_name    = "CrudeEngine",
      .vk_application_version = VK_MAKE_VERSION( 1, 0, 0 ),
      .allocator_container    = paprika->graphics.allocator_container,
      .temporary_allocator    = &paprika->temporary_allocator,
      .queries_per_frame      = 1u,
      .num_threads            = CRUDE_STATIC_CAST( uint16, enkiGetNumTaskThreads( CRUDE_REINTERPRET_CAST( enkiTaskScheduler*, paprika->graphics.asynchronous_loader_manager->task_sheduler ) ) ),
    };
    crude_gfx_device_initialize( &paprika->graphics.gpu, &device_creation );
  }
  
  /* Create Renderer */
  {
    crude_gfx_renderer_creation renderer_creation = {
      .gpu                 = &paprika->graphics.gpu,
      .allocator_container = paprika->graphics.allocator_container,
    };
    crude_gfx_renderer_initialize( &paprika->graphics.renderer, &renderer_creation );
  }    
  
  /* Create Render Graph */
  crude_gfx_render_graph_builder_initialize( &paprika->graphics.render_graph_builder, &paprika->graphics.gpu );
  crude_gfx_render_graph_initialize( &paprika->graphics.render_graph, &paprika->graphics.render_graph_builder );
  
  /* Create Async Loader */
  {
    crude_gfx_asynchronous_loader_initialize( &paprika->graphics.async_loader, &paprika->graphics.renderer );
    crude_gfx_asynchronous_loader_manager_add_loader( paprika->graphics.asynchronous_loader_manager, &paprika->graphics.async_loader );
  }
  
  /* Parse render graph*/
  {
    crude_string_buffer                                    temporary_name_buffer;
    char                                                   working_directory[ 512 ];
    char const                                            *render_graph_file_path;

    crude_get_current_working_directory( working_directory, sizeof( working_directory ) );
    crude_string_buffer_initialize( &temporary_name_buffer, 1024, crude_stack_allocator_pack( &paprika->temporary_allocator ) );
  
    render_graph_file_path = crude_string_buffer_append_use_f( &temporary_name_buffer, "%s%s", working_directory, "\\..\\..\\resources\\render_graph.json", "\\..\\..\\resources\\render_graph.json" );
    crude_gfx_render_graph_parse_from_file( &paprika->graphics.render_graph, render_graph_file_path, &paprika->temporary_allocator );
    crude_gfx_render_graph_compile( &paprika->graphics.render_graph );
  }
  
  crude_gfx_render_graph_node *pointlight_shadow_pass_node = crude_gfx_render_graph_builder_access_node_by_name( &paprika->graphics.render_graph_builder, "point_shadows_pass" );
  if ( pointlight_shadow_pass_node )
  {
    CRUDE_ASSERT( CRUDE_RESOURCE_HANDLE_IS_INVALID( pointlight_shadow_pass_node->render_pass ) );
    crude_gfx_render_pass_creation creation = crude_gfx_render_pass_creation_empty( );
    creation.name = "point_shadows_pass";
    creation.depth_stencil_format = VK_FORMAT_D16_UNORM;
    creation.depth_stencil_final_layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    creation.depth_operation = CRUDE_GFX_RENDER_PASS_OPERATION_CLEAR;
    pointlight_shadow_pass_node->render_pass = crude_gfx_create_render_pass( &paprika->graphics.gpu, &creation );
  }

  /* Create Render Tecnhique & Renderer Passes*/
  crude_gfx_renderer_technique_load_from_file( "\\..\\..\\shaders\\meshlet_technique.json", &paprika->graphics.renderer, &paprika->graphics.render_graph, &paprika->temporary_allocator );
  crude_gfx_renderer_technique_load_from_file( "\\..\\..\\shaders\\culling_technique.json", &paprika->graphics.renderer, &paprika->graphics.render_graph, &paprika->temporary_allocator );
  crude_gfx_renderer_technique_load_from_file( "\\..\\..\\shaders\\debug_technique.json", &paprika->graphics.renderer, &paprika->graphics.render_graph, &paprika->temporary_allocator );
  crude_gfx_renderer_technique_load_from_file( "\\..\\..\\shaders\\fullscreen_technique.json", &paprika->graphics.renderer, &paprika->graphics.render_graph, &paprika->temporary_allocator );
  crude_gfx_renderer_technique_load_from_file( "\\..\\..\\shaders\\imgui_technique.json", &paprika->graphics.renderer, &paprika->graphics.render_graph, &paprika->temporary_allocator );

  /* Create Scene Renderer */
  {
    crude_gfx_scene_renderer_creation creation = CRUDE_COMPOUNT_EMPTY( crude_gfx_scene_renderer_creation );
    creation.renderer = &paprika->graphics.renderer;
    creation.async_loader = &paprika->graphics.async_loader;
    creation.allocator_container = paprika->graphics.allocator_container;
    creation.temporary_allocator = &paprika->temporary_allocator;
    creation.task_scheduler = paprika->graphics.asynchronous_loader_manager->task_sheduler;
    creation.imgui_context = paprika->imgui_context;
    creation.scene = &paprika->scene;
    crude_gfx_scene_renderer_initialize( &paprika->graphics.scene_renderer, &creation );
    crude_gfx_scene_renderer_register_passes( &paprika->graphics.scene_renderer, &paprika->graphics.render_graph );
  }
  
  crude_stack_allocator_free_marker( &paprika->temporary_allocator, temporary_allocator_marker );
}

void
paprika_graphics_system_
(
  _In_ ecs_iter_t                                         *it
)
{
  crude_paprika *paprika = ( crude_paprika* )it->ctx;

  paprika->last_graphics_update_time += it->delta_time;

  if ( paprika->last_graphics_update_time < 1.f / paprika->framerate )
  {
    return;
  }
  paprika->last_graphics_update_time = 0.f;

  crude_gfx_new_frame( &paprika->graphics.gpu );
  
  ImGui::SetCurrentContext( ( ImGuiContext* ) paprika->imgui_context );
  ImGuizmo::SetImGuiContext( ( ImGuiContext* ) paprika->imgui_context );
  ImGui_ImplSDL3_NewFrame();
  ImGui::NewFrame();
  ImGuizmo::BeginFrame();
  ImGuizmo::SetOrthographic( false );
  ImGui::DockSpaceOverViewport( 0u, ImGui::GetMainViewport( ) );

  if ( paprika->graphics.gpu.swapchain_resized_last_frame )
  {
    crude_gfx_render_graph_on_resize( &paprika->graphics.render_graph, paprika->graphics.gpu.vk_swapchain_width, paprika->graphics.gpu.vk_swapchain_height );
  }

  paprika_draw_imgui_( paprika );
  
  crude_gfx_scene_renderer_submit_draw_task( &paprika->graphics.scene_renderer, false );
  
  crude_gfx_renderer_add_texture_update_commands( &paprika->graphics.renderer, 1u );

  {
    crude_gfx_texture *final_render_texture = crude_gfx_access_texture( &paprika->graphics.gpu, crude_gfx_render_graph_builder_access_resource_by_name( paprika->graphics.scene_renderer.render_graph->builder, "imgui" )->resource_info.texture.handle );
    crude_gfx_present( &paprika->graphics.gpu, final_render_texture );
  }

  crude_devgui_post_graphics_update( &paprika->devgui );
}

void
paprika_graphics_deinitialize_
(
  _In_ crude_paprika                                      *paprika
)
{
  crude_gfx_asynchronous_loader_manager_remove_loader( paprika->graphics.asynchronous_loader_manager, &paprika->graphics.async_loader );
  vkDeviceWaitIdle( paprika->graphics.gpu.vk_device );
  crude_gfx_scene_renderer_deinitialize( &paprika->graphics.scene_renderer );
  crude_gfx_asynchronous_loader_deinitialize( &paprika->graphics.async_loader );
  crude_gfx_render_graph_builder_deinitialize( &paprika->graphics.render_graph_builder );
  crude_gfx_render_graph_deinitialize( &paprika->graphics.render_graph );
  crude_gfx_renderer_deinitialize( &paprika->graphics.renderer );
  crude_gfx_device_deinitialize( &paprika->graphics.gpu );
}

void
paprika_update_system_
(
  _In_ ecs_iter_t                                         *it
)
{
  crude_paprika *paprika = ( crude_paprika* )it->ctx;
  crude_input *input_per_entity = ecs_field( it, crude_input, 0 );
  crude_window_handle *window_handle_per_entity = ecs_field( it, crude_window_handle, 1 );

  ImGui::SetCurrentContext( CRUDE_CAST( ImGuiContext*, paprika->imgui_context ) );
  for ( uint32 i = 0; i < it->count; ++i )
  {
    crude_input *input = &input_per_entity[ i ];
    crude_window_handle *window_handle = &window_handle_per_entity[ i ];

    crude_devgui_handle_input( &paprika->devgui, input );

    if ( input->mouse.right.current && input->mouse.right.current != input->prev_mouse.right.current )
    {
      ImGuiIO *imgui_io = &ImGui::GetIO();
      imgui_io->ConfigFlags |= ImGuiConfigFlags_NoMouseCursorChange;
      paprika->wrapwnd.x = input->mouse.wnd.x;
      paprika->wrapwnd.y = input->mouse.wnd.y;
      SDL_HideCursor();
    }
    
    if ( !input->mouse.right.current && input->mouse.right.current != input->prev_mouse.right.current )
    {
      ImGuiIO *imgui_io = &ImGui::GetIO();
      imgui_io->ConfigFlags ^= ImGuiConfigFlags_NoMouseCursorChange;
      SDL_WarpMouseInWindow( CRUDE_CAST( SDL_Window*, window_handle->value ), paprika->wrapwnd.x, paprika->wrapwnd.y );
      SDL_ShowCursor();
    }
    
    if ( input->mouse.rel.x || input->mouse.rel.y )
    {
      if ( !SDL_CursorVisible() )
      {
        SDL_WarpMouseInWindow( CRUDE_CAST( SDL_Window*, window_handle->value ), paprika->wrapwnd.x, paprika->wrapwnd.y );
      }
    }
  }
}

void
paprika_input_callback_
(
  _In_ void                                               *ctx,
  _In_ void                                               *sdl_event
)
{
  ImGui::SetCurrentContext( CRUDE_CAST( ImGuiContext*, ctx ) );
  ImGui_ImplSDL3_ProcessEvent( CRUDE_CAST( SDL_Event*, sdl_event ) );
}

void
paprika_draw_imgui_
(
  _In_ crude_paprika                                      *paprika
)
{
  crude_string_buffer                                      temporary_string_buffer;
  uint32                                                   temporary_allocator_mark;
  
  temporary_allocator_mark = crude_stack_allocator_get_marker( &paprika->temporary_allocator );
  crude_string_buffer_initialize( &temporary_string_buffer, 4096, crude_stack_allocator_pack( &paprika->temporary_allocator ) );
  
  crude_devgui_draw( &paprika->devgui, paprika->scene.main_node, paprika->scene.main_camera );

  crude_stack_allocator_free_marker( &paprika->temporary_allocator, temporary_allocator_mark );
}