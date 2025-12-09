#include <SDL3/SDL.h>
#include <imgui/imgui.h>

#include <engine/core/hash_map.h>
#include <engine/platform/platform.h>
#include <game/game.h>
#include <engine/scene/scripts_components.h>
#include <engine/external/game_components.h>

#include <game/game_menu.h>

void
crude_game_menu_initialize
(
  _In_ crude_game_menu                                    *menu
)
{
  menu->enabled = false;
}

void
crude_game_menu_deinitialize
(
  _In_ crude_game_menu                                    *menu
)
{
}

void
crude_game_menu_draw
(
  _In_ crude_game_menu                                    *menu
)
{
  game_t                                                  *game;

  game = game_instance( );

  ImGui::SetNextWindowPos( ImVec2( 0, 0 ) );
  if ( menu->enabled )
  {
    crude_player_controller *player_controller = CRUDE_ENTITY_GET_MUTABLE_COMPONENT( game->player_node, crude_player_controller );
    ImGui::SetNextWindowSize( ImVec2( game->gpu.vk_swapchain_width, game->gpu.vk_swapchain_height ) );
    
    ImGui::Begin( "Game Menu", NULL, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoBackground );
    
    ImGui::GetIO().FontGlobalScale = 0.5f;
    ImDrawList *draw_list = ImGui::GetWindowDrawList( );
    draw_list->AddRectFilled(
      { 0, 0 },
      { float32( game->gpu.vk_swapchain_width ), float32( game->gpu.vk_swapchain_height ) },
      crude_color_set( 0, 0, 0, 0.5 )
    );

    float32 sensetivity = 10 * player_controller->rotation_speed;
    if ( ImGui::DragFloat( "Sensetivity (Default -0.1)", &sensetivity, 0.1 ) )
    {
      player_controller->rotation_speed = 0.1f * sensetivity;
    }
    if ( ImGui::DragFloat( "Volume (Default 1.0)", &game->volume, 0.1 ) )
    {
      crude_audio_device_set_global_volume( &game->audio_device, game->volume );
    }
    ImGui::DragFloat( "Gamma (Default 2.2)", &game->scene_renderer.options.gamma, 0.1, 0.1, 5 );
    if ( ImGui::Button( "Window Fullscreen" ) )
    {
      SDL_Window *sdl_window = CRUDE_CAST( SDL_Window*, CRUDE_ENTITY_GET_MUTABLE_COMPONENT( game->platform_node, crude_window_handle )->value );
      SDL_SetWindowFullscreen( sdl_window, true );
      SDL_SetWindowBordered( sdl_window, false );
    }
    ImGui::SameLine( );
    if ( ImGui::Button( "Window Bordered" ) )
    {
      SDL_Window *sdl_window = CRUDE_CAST( SDL_Window*, CRUDE_ENTITY_GET_MUTABLE_COMPONENT( game->platform_node, crude_window_handle )->value );
      SDL_SetWindowBordered( sdl_window, true );
      SDL_SetWindowFullscreen( sdl_window, false );
    }
    ImGui::SameLine( );
    if ( ImGui::Button( "Exclusive fullscreen (TODO)" ) )
    {
    }

    int32 framerate = game->framerate;
    if ( ImGui::DragInt( "FPS Limit", &framerate ) )
    {
      game->framerate = framerate;
    }

    ImGui::GetIO().FontGlobalScale = 1.f;

    ImGui::End( );
  }
}

void
crude_game_menu_update
(
  _In_ crude_game_menu                                    *menu
)
{
  game_t *game = game_instance( );
}

void
crude_game_menu_handle_input
(
  _In_ crude_game_menu                                    *menu,
  _In_ crude_input                                        *input
)
{
  game_t                                                  *game;

  game = game_instance( );
  
  if ( input->keys[ SDL_SCANCODE_ESCAPE ].pressed )
  {
    menu->enabled = !menu->enabled;

    crude_window_handle *window_handle = CRUDE_ENTITY_GET_MUTABLE_COMPONENT( game->platform_node, crude_window_handle );
    if ( menu->enabled )
    {
      crude_platform_show_cursor( *window_handle );
    }
    else
    {
      crude_platform_hide_cursor( *window_handle );
    }
    
    crude_player_controller *player_controller = NULL;
    if ( crude_entity_valid( game->player_node ) )
    {
      player_controller = CRUDE_ENTITY_GET_MUTABLE_COMPONENT( game->player_node, crude_player_controller );
    }
    if ( player_controller )
    {
      player_controller->input_enabled = !menu->enabled;
    }
  }
}