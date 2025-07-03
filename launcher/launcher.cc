#include <flecs.h>

#define CR_HOST
#include <cr/cr.h>
#include <filesystem>

#include <imgui.h>
#include <imgui/backends/imgui_impl_sdl3.h>
#include <imgui/backends/imgui_impl_sdlrenderer3.h>
#include <SDL3/SDL.h>

#include <engine.h>
#include <core/profiler.h>
#include <sandbox.h>
#include <paprika.h>

std::filesystem::path
get_executable_path_
(
)
{
#ifdef _WIN32
  wchar_t path[ MAX_PATH ] = { 0 };
  GetModuleFileNameW( NULL, path, MAX_PATH );
  return std::filesystem::path( path ).remove_filename();
#else
  return std::filesystem::canonical("/proc/self/exe");
#endif
}

#define PLUGIN_PATH_STR( name ) get_executable_path_().concat( CR_PLUGIN( name ) ).string()

int
main
(
)
{
  ImVec4                                                   clear_color = ImVec4( 14 / 255.f, 6 / 255.f, 19 / 255.f, 1.00f );
  
  crude_engine                                             engine;
  crude_sandbox                                            sandbox;
  crude_paprika                                            paprika;
  cr_plugin                                                crude_sandbox_cr, crude_paprika_cr, crude_engine_simulation_cr;
  ImGuiContext                                            *imgui_context;
  ImGuiIO                                                 *imgui_io;
  ImGuiStyle                                              *imgui_style;
  SDL_Renderer                                            *sdl_renderer;
  SDL_Window                                              *sdl_window;
  float32                                                  display_content_scale;
  bool                                                     should_close_window;
  
  crude_engine_initialize( &engine, 1u );
  crude_sandbox_initialize( &sandbox, &engine );
  crude_paprika_initialize( &paprika, &engine );
  
  crude_sandbox_cr.userdata           = &sandbox;
  crude_paprika_cr.userdata           = &paprika;
  crude_engine_simulation_cr.userdata = &engine;

  cr_plugin_open( crude_engine_simulation_cr, PLUGIN_PATH_STR( "crude_engine_simulation" ).c_str() );
  cr_plugin_open( crude_sandbox_cr, PLUGIN_PATH_STR( "crude_sandbox" ).c_str() );
  cr_plugin_open( crude_paprika_cr, PLUGIN_PATH_STR( "crude_paprika" ).c_str() );

  if ( !SDL_Init( SDL_INIT_VIDEO) )
  {
    goto cleanup;
  }
  
  display_content_scale = SDL_GetDisplayContentScale( SDL_GetPrimaryDisplay() );
  sdl_window = SDL_CreateWindow( "crude_launcher", 1280 * display_content_scale, 720 * display_content_scale, SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIDDEN | SDL_WINDOW_HIGH_PIXEL_DENSITY );
  if ( !sdl_window )
  {
    goto cleanup;
  }

  sdl_renderer = SDL_CreateRenderer( sdl_window, nullptr);
  SDL_SetRenderVSync( sdl_renderer, 1 );
  if ( !sdl_renderer )
  {
    goto cleanup;
  }
  SDL_SetWindowPosition( sdl_window, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED );
  SDL_ShowWindow( sdl_window );

  IMGUI_CHECKVERSION();
  imgui_context = ImGui::CreateContext();
  ImGui::SetCurrentContext( imgui_context );
  imgui_io = &ImGui::GetIO();
  imgui_io->ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard | ImGuiConfigFlags_DockingEnable;
  imgui_io->ConfigWindowsResizeFromEdges = true;
  imgui_io->ConfigWindowsMoveFromTitleBarOnly = true;
  ImGui::StyleColorsDark();

  imgui_style = &ImGui::GetStyle();
  imgui_style->ScaleAllSizes( display_content_scale );

  // Setup Platform/Renderer backends
  ImGui_ImplSDL3_InitForSDLRenderer( sdl_window, sdl_renderer );
  ImGui_ImplSDLRenderer3_Init( sdl_renderer );

  // Main loop
  should_close_window = false;
  while ( !should_close_window )
  {
    SDL_Event                                              sdl_event;
    while ( SDL_PollEvent( &sdl_event ) )
    {
      ImGui_ImplSDL3_ProcessEvent( &sdl_event );
      if ( sdl_event.type == SDL_EVENT_QUIT )
      {
        should_close_window = true;
      }
      if ( sdl_event.type == SDL_EVENT_WINDOW_CLOSE_REQUESTED && sdl_event.window.windowID == SDL_GetWindowID( sdl_window ) )
      {
        should_close_window = true;
      }
    }
    
    if ( SDL_GetWindowFlags( sdl_window ) & SDL_WINDOW_MINIMIZED )
    {
      SDL_Delay( 10 );
      continue;
    }
    
    ImGui::DockSpaceOverViewport(0u, ImGui::GetMainViewport());
    ImGui_ImplSDLRenderer3_NewFrame();
    ImGui_ImplSDL3_NewFrame();
    ImGui::NewFrame();
    
    // 2. Show a simple window that we create ourselves. We use a Begin/End pair to create a named window.
    {
        static float f = 0.0f;
        static int counter = 0;
    
        ImGui::Begin("Hello, world!");                          // Create a window called "Hello, world!" and append into it.
    
        ImGui::Text("This is some useful text.");               // Display some text (you can use a format strings too)
    
        ImGui::SliderFloat("float", &f, 0.0f, 1.0f);            // Edit 1 float using a slider from 0.0f to 1.0f
        ImGui::ColorEdit3("clear color", (float*)&clear_color); // Edit 3 floats representing a color
    
        if (ImGui::Button("Button"))                            // Buttons return true when clicked (most widgets return true when edited/activated)
            counter++;
        ImGui::SameLine();
        ImGui::Text("counter = %d", counter);
    
        ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / imgui_io->Framerate, imgui_io->Framerate);
        ImGui::End();
    }
    
    ImGui::Render();
    SDL_SetRenderScale( sdl_renderer, imgui_io->DisplayFramebufferScale.x, imgui_io->DisplayFramebufferScale.y );
    SDL_SetRenderDrawColorFloat( sdl_renderer, clear_color.x, clear_color.y, clear_color.z, clear_color.w );
    SDL_RenderClear( sdl_renderer );
    ImGui_ImplSDLRenderer3_RenderDrawData( ImGui::GetDrawData(), sdl_renderer );
    SDL_RenderPresent( sdl_renderer );
  }

cleanup:
  ImGui_ImplSDLRenderer3_Shutdown();
  ImGui_ImplSDL3_Shutdown();
  ImGui::DestroyContext();

  if ( sdl_renderer )
  {
    SDL_DestroyRenderer( sdl_renderer );
  }
  if ( sdl_window )
  {
    SDL_DestroyWindow( sdl_window );
  }
  SDL_Quit();
  
  cr_plugin_close( crude_paprika_cr );
  cr_plugin_close( crude_sandbox_cr );
  cr_plugin_close( crude_engine_simulation_cr );
  
  crude_paprika_deinitialize( &paprika );
  crude_sandbox_deinitialize( &sandbox );
  crude_engine_deinitialize( &engine );

  //while ( engine.running )
  //{
  //  if ( sandbox.working )
  //  {
  //    cr_plugin_update( crude_sandbox_cr );
  //  }
  //  if ( paprika.working )
  //  {
  //    cr_plugin_update( crude_paprika_cr );
  //  }
  //  cr_plugin_update( crude_engine_simulation_cr );
  //  CRUDE_PROFILER_MARK_FRAME;
  //}

  return 0;
}
