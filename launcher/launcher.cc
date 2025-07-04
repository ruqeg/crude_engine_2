#include <flecs.h>

#define CR_HOST
#include <cr/cr.h>
#include <filesystem>

#include <imgui.h>
#include <imgui/backends/imgui_impl_sdl3.h>
#include <imgui/backends/imgui_impl_sdlrenderer3.h>
#include <SDL3/SDL.h>

#include <engine.h>
#include <core/string.h>
#include <core/file.h>
#include <core/profiler.h>
#include <sandbox.h>
#include <paprika.h>

static char*
get_plugin_dll_path
(
  _In_ char                                               *buffer,
  _In_ uint32                                              buffer_size,
  _In_ char const                                         *plugin_dll
)
{
  buffer[ 0 ] = 0u;
  crude_get_executable_directory( buffer, buffer_size );
  crude_string_cat( buffer, plugin_dll, buffer_size );
  return buffer;
}
#define PLUGIN_PATH_STR( name )\ get_executable_path_().concat( CR_PLUGIN( name ) ).string()

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
  SDL_Renderer                                            *sdl_renderer;
  SDL_Window                                              *sdl_window;
  SDL_Texture                                             *paprika_texture;
  ImGuiContext                                            *imgui_context;
  ImGuiIO                                                 *imgui_io;
  ImGuiStyle                                              *imgui_style;
  float32                                                  display_content_scale;
  bool                                                     should_close_window;
  
  paprika.working = false;
  sandbox.working = false;

  crude_engine_initialize( &engine, 1u );

  crude_sandbox_cr.userdata           = &sandbox;
  crude_paprika_cr.userdata           = &paprika;
  crude_engine_simulation_cr.userdata = &engine;
  
  {
    char buffer[ 1024 ];

    cr_plugin_open( crude_engine_simulation_cr, get_plugin_dll_path( buffer, sizeof( buffer ), CR_PLUGIN( "crude_engine_simulation" ) ) );
    cr_plugin_open( crude_sandbox_cr, get_plugin_dll_path( buffer, sizeof( buffer ), CR_PLUGIN( "crude_sandbox" ) ) );
    cr_plugin_open( crude_paprika_cr, get_plugin_dll_path( buffer, sizeof( buffer ), CR_PLUGIN( "crude_paprika" ) ) );
  }

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
 
  {
    char paprika_texture_path[ 1024 ] = {};
    crude_get_current_working_directory( paprika_texture_path, sizeof( paprika_texture_path ) );
    strcat( paprika_texture_path, engine.resources_path );
    strcat( paprika_texture_path, "textures\\paprika_button_texture.bmp" );
    SDL_Surface* loaded_surface = SDL_LoadBMP( paprika_texture_path );
    if( !loaded_surface )
    {
      goto cleanup;
    }
    paprika_texture = SDL_CreateTextureFromSurface( sdl_renderer, loaded_surface );
    SDL_DestroySurface( loaded_surface );
  }

  // Main loop
  should_close_window = false;
  while ( !should_close_window && engine.running )
  {
    SDL_Event                                              sdl_event;
    while ( SDL_PollEvent( &sdl_event ) )
    {
      if ( SDL_GetMouseFocus() == sdl_window || SDL_GetKeyboardFocus() == sdl_window || sdl_event.window.windowID == SDL_GetWindowID( sdl_window ) )
      {
        ImGui_ImplSDL3_ProcessEvent( &sdl_event );
      }

      if ( sdl_event.window.windowID == SDL_GetWindowID( sdl_window ) )
      {
        if ( sdl_event.type == SDL_EVENT_WINDOW_CLOSE_REQUESTED )
        {
          should_close_window = true;
        }
      }
    }
    
    if ( SDL_GetWindowFlags( sdl_window ) & SDL_WINDOW_MINIMIZED )
    {
      SDL_Delay( 10 );
      continue;
    }
    
    ImGui::SetCurrentContext( imgui_context );
    ImGui_ImplSDLRenderer3_NewFrame();
    ImGui_ImplSDL3_NewFrame();
    ImGui::NewFrame();
    
    ImGui::DockSpaceOverViewport(0u, ImGui::GetMainViewport());
    {
      ImVec2 paprika_texture_size;
      paprika_texture_size.x = ImGui::GetMainViewport()->WorkSize.x / 3.0;
      paprika_texture_size.y = paprika_texture_size.x * ( ( float32 )paprika_texture->h / paprika_texture->w );
      ImGui::Begin( "main", NULL );
      if ( ImGui::ImageButton( "crude_paprika", (ImTextureRef)paprika_texture, paprika_texture_size ) )
      {
        if ( !paprika.working )
        {
          crude_paprika_deinitialize( &paprika );
          crude_paprika_initialize( &paprika, &engine );
          ImGui::SetCurrentContext( imgui_context );
        }
      }
      ImGui::End();
    }
    
    ImGui::Render();
    SDL_SetRenderScale( sdl_renderer, imgui_io->DisplayFramebufferScale.x, imgui_io->DisplayFramebufferScale.y );
    SDL_SetRenderDrawColorFloat( sdl_renderer, clear_color.x, clear_color.y, clear_color.z, clear_color.w );
    SDL_RenderClear( sdl_renderer );
    ImGui_ImplSDLRenderer3_RenderDrawData( ImGui::GetDrawData(), sdl_renderer );
    SDL_RenderPresent( sdl_renderer );
    
    if ( paprika.working )
    {
      cr_plugin_update( crude_paprika_cr );
    }
    cr_plugin_update( crude_engine_simulation_cr );

    CRUDE_PROFILER_MARK_FRAME;
  }

cleanup:
  ImGui_ImplSDLRenderer3_Shutdown();
  ImGui_ImplSDL3_Shutdown();
  if ( imgui_context )
  {
    ImGui::DestroyContext( imgui_context );
  }

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

  return 0;
}
