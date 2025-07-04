#include <flecs.h>

#define CR_HOST
#include <cr/cr.h>

#include <imgui.h>
#include <imgui/backends/imgui_impl_sdl3.h>
#include <imgui/backends/imgui_impl_sdlrenderer3.h>
#include <SDL3/SDL.h>

#include <platform/platform_system.h>
#include <platform/platform_components.h>
#include <core/string.h>
#include <core/file.h>
#include <core/profiler.h>

#include "launcher.h"

static char*
get_plugin_dll_path_
(
  _In_ char                                               *buffer,
  _In_ uint32                                              buffer_size,
  _In_ char const                                         *plugin_dll
);

static void
launcher_input_callback_
(
  _In_ void                                               *ctx,
  _In_ void                                               *sdl_event
);

void
crude_launcher_initialize
(
  _In_ crude_launcher                                     *launcher
)
{
  ImGuiStyle                                              *imgui_style;
  ImGuiIO                                                 *imgui_io;
  SDL_Window                                              *sdl_window;
  char                                                     temporary_buffer[ 1024 ];
  float32                                                  display_content_scale;

  launcher->paprika.working = false;
  launcher->sandbox.working = false;

  crude_engine_initialize( &launcher->engine, 1u );
  
  ECS_IMPORT( launcher->engine.world, crude_platform_system );

  launcher->crude_sandbox_cr.userdata           = &launcher->sandbox;
  launcher->crude_paprika_cr.userdata           = &launcher->paprika;
  launcher->crude_engine_simulation_cr.userdata = &launcher->engine;
  
  cr_plugin_open( launcher->crude_engine_simulation_cr, get_plugin_dll_path_( temporary_buffer, sizeof( temporary_buffer ), CR_PLUGIN( "crude_engine_simulation" ) ) );
  cr_plugin_open( launcher->crude_sandbox_cr, get_plugin_dll_path_( temporary_buffer, sizeof( temporary_buffer ), CR_PLUGIN( "crude_sandbox" ) ) );
  cr_plugin_open( launcher->crude_paprika_cr, get_plugin_dll_path_( temporary_buffer, sizeof( temporary_buffer ), CR_PLUGIN( "crude_paprika" ) ) );

  if ( !SDL_Init( SDL_INIT_VIDEO) )
  {
    return;
  }
  
  display_content_scale = SDL_GetDisplayContentScale( SDL_GetPrimaryDisplay() );
  
  IMGUI_CHECKVERSION();
  launcher->imgui_context = ImGui::CreateContext();

  launcher->platform_node = crude_entity_create_empty( launcher->engine.world, "crude_launcher" );
  CRUDE_ENTITY_SET_COMPONENT( launcher->platform_node, crude_window, { 
    .width     = CRUDE_CAST( int32, 1200 * display_content_scale ),
    .height    = CRUDE_CAST( int32, 600 * display_content_scale ),
    .maximized = false,
    .flags     = SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIDDEN | SDL_WINDOW_HIGH_PIXEL_DENSITY
  });
  CRUDE_ENTITY_SET_COMPONENT( launcher->platform_node, crude_input, { 
    .callback = launcher_input_callback_,
    .ctx = launcher->imgui_context
  } );

  sdl_window = ( SDL_Window* ) CRUDE_ENTITY_GET_MUTABLE_COMPONENT( launcher->platform_node, crude_window_handle )->value;

  launcher->sdl_renderer = SDL_CreateRenderer( sdl_window, nullptr );
  SDL_SetRenderVSync( launcher->sdl_renderer, 1 );
  SDL_ShowWindow( sdl_window );

  ImGui::SetCurrentContext( launcher->imgui_context );
  imgui_io = &ImGui::GetIO();
  imgui_io->ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard | ImGuiConfigFlags_DockingEnable | ImGuiConfigFlags_NoMouseCursorChange;
  imgui_io->ConfigWindowsResizeFromEdges = true;
  imgui_io->ConfigWindowsMoveFromTitleBarOnly = true;
  ImGui::StyleColorsDark();

  imgui_style = &ImGui::GetStyle();
  imgui_style->ScaleAllSizes( display_content_scale );

  ImGui_ImplSDL3_InitForSDLRenderer( sdl_window, launcher->sdl_renderer );
  ImGui_ImplSDLRenderer3_Init( launcher->sdl_renderer );
 
  temporary_buffer[ 0 ] = 0u;
  crude_get_current_working_directory( temporary_buffer, sizeof( temporary_buffer ) );
  strcat( temporary_buffer, launcher->engine.resources_path );
  strcat( temporary_buffer, "textures\\paprika_button_texture.bmp" );
  SDL_Surface* loaded_surface = SDL_LoadBMP( temporary_buffer );
  if( !loaded_surface )
  {
    return;
  }
  launcher->paprika_texture = SDL_CreateTextureFromSurface( launcher->sdl_renderer, loaded_surface );
  SDL_DestroySurface( loaded_surface );
}

void
crude_launcher_deinitialize
(
  _In_ crude_launcher                                     *launcher
)
{
  ImGui_ImplSDLRenderer3_Shutdown();
  ImGui_ImplSDL3_Shutdown();
  if ( launcher->imgui_context )
  {
    ImGui::DestroyContext( launcher->imgui_context );
  }

  if ( launcher->sdl_renderer )
  {
    SDL_DestroyRenderer( launcher->sdl_renderer );
  }
  SDL_Quit();
  
  cr_plugin_close( launcher->crude_paprika_cr );
  cr_plugin_close( launcher->crude_sandbox_cr );
  cr_plugin_close( launcher->crude_engine_simulation_cr );
  
  crude_paprika_deinitialize( &launcher->paprika );
  crude_sandbox_deinitialize( &launcher->sandbox );
  crude_engine_deinitialize( &launcher->engine );
}

void
crude_launcher_update
(
  _In_ crude_launcher                                     *launcher
)
{
  ImVec4 clear_color = ImVec4( 14 / 255.f, 6 / 255.f, 19 / 255.f, 1.00f );

  crude_input *input = CRUDE_ENTITY_GET_MUTABLE_COMPONENT( launcher->platform_node, crude_input );

  ImGui::SetCurrentContext( launcher->imgui_context );

  ImGuiIO *imgui_io = &ImGui::GetIO();

  ImGui_ImplSDLRenderer3_NewFrame();
  ImGui_ImplSDL3_NewFrame();
  ImGui::NewFrame();

  ImGui::DockSpaceOverViewport(0u, ImGui::GetMainViewport());
  {
    ImVec2 paprika_texture_size;
    paprika_texture_size.x = ImGui::GetMainViewport()->WorkSize.x / 3.0;
    paprika_texture_size.y = paprika_texture_size.x * ( ( float32 )launcher->paprika_texture->h / launcher->paprika_texture->w );
    ImGui::Begin( "main", NULL );
    if ( ImGui::ImageButton( "crude_paprika", (ImTextureRef)launcher->paprika_texture, paprika_texture_size ) )
    {
      if ( !launcher->paprika.working )
      {
        crude_paprika_deinitialize( &launcher->paprika );
        crude_paprika_initialize( &launcher->paprika, &launcher->engine );
        ImGui::SetCurrentContext( launcher->imgui_context );
      }
    }
    ImGui::End();
  }

  ImGui::Render();
  SDL_SetRenderScale( launcher->sdl_renderer, imgui_io->DisplayFramebufferScale.x, imgui_io->DisplayFramebufferScale.y );
  SDL_SetRenderDrawColorFloat( launcher->sdl_renderer, clear_color.x, clear_color.y, clear_color.z, clear_color.w );
  SDL_RenderClear( launcher->sdl_renderer );
  ImGui_ImplSDLRenderer3_RenderDrawData( ImGui::GetDrawData(), launcher->sdl_renderer );
  SDL_RenderPresent( launcher->sdl_renderer );

  if ( launcher->paprika.working )
  {
    cr_plugin_update( launcher->crude_paprika_cr );
  }

  cr_plugin_update( launcher->crude_engine_simulation_cr );

  CRUDE_PROFILER_MARK_FRAME;
}

char*
get_plugin_dll_path_
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

void
launcher_input_callback_
(
  _In_ void                                               *ctx,
  _In_ void                                               *sdl_event
)
{
  ImGui::SetCurrentContext( CRUDE_CAST( ImGuiContext*, ctx ) );
  ImGui_ImplSDL3_ProcessEvent( CRUDE_CAST( SDL_Event*, sdl_event ) );
}