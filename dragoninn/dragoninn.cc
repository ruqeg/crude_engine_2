#include <imgui/backends/imgui_impl_sdl3.h>
#include <imgui/backends/imgui_impl_sdlrenderer3.h>
#include <SDL3/SDL.h>

#include <platform/platform_system.h>
#include <platform/platform_components.h>

#include <dragoninn.h>

CRUDE_ECS_SYSTEM_DECLARE( dragoninn_update_system_ );

static void
dragoninn_update_system_
(
  _In_ ecs_iter_t                                         *it
);

static void
dragoninn_input_callback_
(
  _In_ void                                               *ctx,
  _In_ void                                               *sdl_event
);

void
crude_dragoninn_initialize
(
  _In_ crude_dragoninn                                    *dragoninn,
  _In_ crude_engine                                       *engine
)
{
  dragoninn->working = true;
  dragoninn->engine = engine;
  
  ImGuiStyle                                              *imgui_style;
  ImGuiIO                                                 *imgui_io;
  SDL_Window                                              *sdl_window;
  char                                                     temporary_buffer[ 1024 ];
  float32                                                  display_content_scale;

  ECS_IMPORT( dragoninn->engine->world, crude_platform_system );

  if ( !SDL_Init( SDL_INIT_VIDEO) )
  {
    return;
  }
  
  display_content_scale = SDL_GetDisplayContentScale( SDL_GetPrimaryDisplay() );
  
  IMGUI_CHECKVERSION();
  dragoninn->imgui_context = ImGui::CreateContext();

  dragoninn->platform_node = crude_entity_create_empty( dragoninn->engine->world, "crude_dragoninn" );
  CRUDE_ENTITY_SET_COMPONENT( dragoninn->platform_node, crude_window, { 
    .width     = CRUDE_CAST( int32, 1200 * display_content_scale ),
    .height    = CRUDE_CAST( int32, 600 * display_content_scale ),
    .maximized = false,
    .flags     = SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIDDEN | SDL_WINDOW_HIGH_PIXEL_DENSITY
  });
  CRUDE_ENTITY_SET_COMPONENT( dragoninn->platform_node, crude_input, { 
    .callback = dragoninn_input_callback_,
    .ctx = dragoninn->imgui_context
  } );

  sdl_window = ( SDL_Window* ) CRUDE_ENTITY_GET_MUTABLE_COMPONENT( dragoninn->platform_node, crude_window_handle )->value;

  dragoninn->sdl_renderer = SDL_CreateRenderer( sdl_window, nullptr );
  SDL_SetRenderVSync( dragoninn->sdl_renderer, 1 );
  SDL_ShowWindow( sdl_window );

  ImGui::SetCurrentContext( dragoninn->imgui_context );
  imgui_io = &ImGui::GetIO();
  imgui_io->ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard | ImGuiConfigFlags_DockingEnable | ImGuiConfigFlags_NoMouseCursorChange;
  imgui_io->ConfigWindowsResizeFromEdges = true;
  imgui_io->ConfigWindowsMoveFromTitleBarOnly = true;
  ImGui::StyleColorsDark();

  imgui_style = &ImGui::GetStyle();
  imgui_style->ScaleAllSizes( display_content_scale );

  ImGui_ImplSDL3_InitForSDLRenderer( sdl_window, dragoninn->sdl_renderer );
  ImGui_ImplSDLRenderer3_Init( dragoninn->sdl_renderer );
  
  ax::NodeEditor::Config config;
  config.SettingsFile = "Simple.json";
  dragoninn->node_editor_context = ax::NodeEditor::CreateEditor( &config );

  CRUDE_ECS_SYSTEM_DEFINE( dragoninn->engine->world, dragoninn_update_system_, EcsOnUpdate, dragoninn, {
    { .id = ecs_id( crude_input ) },
    { .id = ecs_id( crude_window_handle ) },
  } );
}

void
crude_dragoninn_deinitialize
(
  _In_ crude_dragoninn                                    *dragoninn
)
{
  if ( !dragoninn->working )
  {
    return;
  }
  
  axn::DestroyEditor( dragoninn->node_editor_context );

  dragoninn->working = false;
}

void
crude_dragoninn_update
(
  _In_ crude_dragoninn                                    *dragoninn
)
{
  crude_input const *input = CRUDE_ENTITY_GET_IMMUTABLE_COMPONENT( dragoninn->platform_node, crude_input );
  if ( input && input->should_close_window )
  {
    crude_dragoninn_deinitialize( dragoninn );
  }
}

void
dragoninn_update_system_
(
  _In_ ecs_iter_t                                         *it
)
{
  crude_dragoninn *dragoninn = ( crude_dragoninn* )it->ctx;
  crude_input *input_per_entity = ecs_field( it, crude_input, 0 );
  crude_window_handle *window_handle_per_entity = ecs_field( it, crude_window_handle, 1 );
  
  ImVec4 clear_color = ImVec4( 14 / 255.f, 6 / 255.f, 19 / 255.f, 1.00f );

  ImGui::SetCurrentContext( dragoninn->imgui_context );

  ImGuiIO *imgui_io = &ImGui::GetIO();

  ImGui_ImplSDLRenderer3_NewFrame();
  ImGui_ImplSDL3_NewFrame();
  ImGui::NewFrame();

  ImGui::DockSpaceOverViewport(0u, ImGui::GetMainViewport());
  {
    ImGui::Begin( "main", NULL );
    ImGui::End();
  }

  
  auto& io = ImGui::GetIO();
  
  axn::SetCurrentEditor( dragoninn->node_editor_context );
  axn::Begin("My Editor", ImVec2(0.0, 0.0f));
  int uniqueId = 1;
  // Start drawing nodes.
  axn::BeginNode(uniqueId++);
      ImGui::Text("Node A");
      axn::BeginPin(uniqueId++, axn::PinKind::Input);
          ImGui::Text("-> In");
      axn::EndPin();
      ImGui::SameLine();
      axn::BeginPin(uniqueId++, axn::PinKind::Output);
          ImGui::Text("Out ->");
      axn::EndPin();
  axn::EndNode();
  axn::End();
  axn::SetCurrentEditor(nullptr);

  ImGui::Render();
  SDL_SetRenderScale( dragoninn->sdl_renderer, imgui_io->DisplayFramebufferScale.x, imgui_io->DisplayFramebufferScale.y );
  SDL_SetRenderDrawColorFloat( dragoninn->sdl_renderer, clear_color.x, clear_color.y, clear_color.z, clear_color.w );
  SDL_RenderClear( dragoninn->sdl_renderer );
  ImGui_ImplSDLRenderer3_RenderDrawData( ImGui::GetDrawData(), dragoninn->sdl_renderer );
  SDL_RenderPresent( dragoninn->sdl_renderer );

  for ( uint32 i = 0; i < it->count; ++i )
  {
    crude_input *input = &input_per_entity[ i ];
    crude_window_handle *window_handle = &window_handle_per_entity[ i ];
  }
}

void
dragoninn_input_callback_
(
  _In_ void                                               *ctx,
  _In_ void                                               *sdl_event
)
{
  ImGui::SetCurrentContext( CRUDE_CAST( ImGuiContext*, ctx ) );
  ImGui_ImplSDL3_ProcessEvent( CRUDE_CAST( SDL_Event*, sdl_event ) );
}