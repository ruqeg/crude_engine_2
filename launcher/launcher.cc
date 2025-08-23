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
#include <core/process.h>
#include <core/file.h>
#include <core/profiler.h>
#include <core/time.h>

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

static void
load_texture_
(
  _In_ crude_launcher                                     *launcher,
  _In_ char const                                         *filename,
  _Out_ SDL_Texture                                      **texture
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

  /* Initialize engine */
  {
    crude_engine_creation creation = CRUDE_COMPOUNT_EMPTY( crude_engine_creation );
    creation.asynchronous_loader_manager = true;
    crude_engine_initialize( &launcher->engine, &creation );
  }
  
  ECS_IMPORT( launcher->engine.world, crude_platform_system );
  
  /* Initialize Paprika cr */
  launcher->paprika_cr.userdata = &launcher->paprika;
  launcher->engine_simulation_cr.userdata = &launcher->engine;

  cr_plugin_open( launcher->paprika_cr, get_plugin_dll_path_( temporary_buffer, sizeof( temporary_buffer ), CR_PLUGIN( "crude_paprika" ) ) );
  cr_plugin_open( launcher->engine_simulation_cr, get_plugin_dll_path_( temporary_buffer, sizeof( temporary_buffer ), CR_PLUGIN( "crude_engine_simulation" ) ) );

  launcher->paprika.working = false;
  
  /* Initialize Imgui Context */
  {
    IMGUI_CHECKVERSION();
    launcher->imgui_context = ImGui::CreateContext();
    ImGui::SetCurrentContext( launcher->imgui_context );
    imgui_io = &ImGui::GetIO();
    imgui_io->ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard | ImGuiConfigFlags_DockingEnable | ImGuiConfigFlags_NoMouseCursorChange;
    imgui_io->ConfigWindowsResizeFromEdges = true;
    imgui_io->ConfigWindowsMoveFromTitleBarOnly = true;
    ImGui::StyleColorsDark();
  }

  /* Creation platform entity */
  {
    crude_window                                           window_comp;
    crude_input                                            input_comp;

    display_content_scale = SDL_GetDisplayContentScale( SDL_GetPrimaryDisplay() );
    
    launcher->platform_node = crude_entity_create_empty( launcher->engine.world, "crude_launcher" );

    window_comp = CRUDE_COMPOUNT_EMPTY( crude_window );
    window_comp.width     = CRUDE_CAST( int32, 1200 * display_content_scale );
    window_comp.height    = CRUDE_CAST( int32, 600 * display_content_scale );
    window_comp.maximized = false;
    window_comp.flags     = SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIDDEN | SDL_WINDOW_HIGH_PIXEL_DENSITY;
    CRUDE_ENTITY_SET_COMPONENT( launcher->platform_node, crude_window, { window_comp } );

    input_comp = CRUDE_COMPOUNT_EMPTY( crude_input );
    input_comp.callback = launcher_input_callback_;
    input_comp.ctx = launcher->imgui_context;
    CRUDE_ENTITY_SET_COMPONENT( launcher->platform_node, crude_input, { input_comp } );

    sdl_window = ( SDL_Window* ) CRUDE_ENTITY_GET_MUTABLE_COMPONENT( launcher->platform_node, crude_window_handle )->value;
  }
 
  imgui_style = &ImGui::GetStyle();
  imgui_style->ScaleAllSizes( display_content_scale );

  /* Initialize SDL Renderer */
  {
    launcher->sdl_renderer = SDL_CreateRenderer( sdl_window, nullptr );
    SDL_SetRenderVSync( launcher->sdl_renderer, 1 );
    SDL_ShowWindow( sdl_window );

    ImGui_ImplSDL3_InitForSDLRenderer( sdl_window, launcher->sdl_renderer );
    ImGui_ImplSDLRenderer3_Init( launcher->sdl_renderer );
  }

  load_texture_( launcher, "textures\\paprika_button_texture.bmp", &launcher->paprika_texture );
  load_texture_( launcher, "textures\\vscode_button_texture.bmp", &launcher->shaders_button_texture );
  load_texture_( launcher, "textures\\dragoninn_button_texture.bmp", &launcher->dragoninn_texture );

  crude_stack_allocator_initialize( &launcher->temporary_allocator, CRUDE_RMEGA( 32 ), "temprorary_allocator" );
}

void
crude_launcher_deinitialize
(
  _In_ crude_launcher                                     *launcher
)
{
  ImGui::SetCurrentContext( launcher->imgui_context );
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
  
  cr_plugin_close( launcher->paprika_cr );
  cr_plugin_close( launcher->engine_simulation_cr );
  
  crude_paprika_deinitialize( &launcher->paprika );
  crude_engine_deinitialize( &launcher->engine );
}

void
crude_launcher_update
(
  _In_ crude_launcher                                     *launcher
)
{
  ImGuiIO                                                 *imgui_io;
  crude_input                                             *input;
  ImVec4                                                   clear_color;
  crude_string_buffer                                      temporary_string_buffer;
  uint32                                                   temporary_allocator_mark;

  temporary_allocator_mark = crude_stack_allocator_get_marker( &launcher->temporary_allocator );
  crude_string_buffer_initialize( &temporary_string_buffer, 1024, crude_stack_allocator_pack( &launcher->temporary_allocator ) );

  clear_color = CRUDE_COMPOUNT( ImVec4, { 14 / 255.f, 6 / 255.f, 19 / 255.f, 1.00f } );

  input = CRUDE_ENTITY_GET_MUTABLE_COMPONENT( launcher->platform_node, crude_input );

  ImGui::SetCurrentContext( launcher->imgui_context );

  imgui_io = &ImGui::GetIO();

  ImGui_ImplSDLRenderer3_NewFrame();
  ImGui_ImplSDL3_NewFrame();
  ImGui::NewFrame();

  ImGui::DockSpaceOverViewport(0u, ImGui::GetMainViewport());
  {
    ImVec2 button_size;
    button_size.x = ( ImGui::GetMainViewport()->WorkSize.x - 55 ) / 3;
    button_size.y = button_size.x * ( ( float32 )launcher->paprika_texture->h / launcher->paprika_texture->w );
    ImGui::Begin( "main", NULL );
    if ( ImGui::ImageButton( "crude_paprika", (ImTextureRef)launcher->paprika_texture, button_size ) )
    {
      if ( !launcher->paprika.working )
      {
        crude_paprika_deinitialize( &launcher->paprika );
        crude_paprika_initialize( &launcher->paprika, &launcher->engine );
        ImGui::SetCurrentContext( launcher->imgui_context );
      }
    }
    ImGui::SameLine();
    if ( ImGui::ImageButton( "crude_shaders", (ImTextureRef)launcher->shaders_button_texture, button_size ) )
    {
      char working_directory[ 1024 ];
      crude_get_current_working_directory( working_directory, sizeof( working_directory ) );
      const char *open_vs_shall = crude_string_buffer_append_use_f( &temporary_string_buffer, "code %s%s", working_directory, launcher->engine.shaders_path );
      crude_system( open_vs_shall );
    }

    ImGui::End();
  }

  ImGui::Render();
  SDL_SetRenderScale( launcher->sdl_renderer, imgui_io->DisplayFramebufferScale.x, imgui_io->DisplayFramebufferScale.y );
  SDL_SetRenderDrawColorFloat( launcher->sdl_renderer, clear_color.x, clear_color.y, clear_color.z, clear_color.w );
  SDL_RenderClear( launcher->sdl_renderer );
  ImGui_ImplSDLRenderer3_RenderDrawData( ImGui::GetDrawData(), launcher->sdl_renderer );
  SDL_RenderPresent( launcher->sdl_renderer );

  crude_stack_allocator_free_marker( &launcher->temporary_allocator, temporary_allocator_mark );

  if ( launcher->paprika.working )
  {
    cr_plugin_update( launcher->paprika_cr );
  }

  cr_plugin_update( launcher->engine_simulation_cr );
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

void
load_texture_
(
  _In_ crude_launcher                                     *launcher,
  _In_ char const                                         *filename,
  _Out_ SDL_Texture                                      **texture
)
{
  char                                                     temporary_buffer[ 1024 ];
  SDL_Surface                                             *surface;

  temporary_buffer[ 0 ] = 0u;
  crude_get_current_working_directory( temporary_buffer, sizeof( temporary_buffer ) );
  strcat( temporary_buffer, launcher->engine.resources_path );
  strcat( temporary_buffer, filename );
  surface = SDL_LoadBMP( temporary_buffer );
  if( !surface )
  {
    return;
  }
  *texture = SDL_CreateTextureFromSurface( launcher->sdl_renderer, surface );
  SDL_DestroySurface( surface );
}