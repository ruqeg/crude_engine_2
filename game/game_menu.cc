#include <SDL3/SDL.h>
#include <imgui/imgui.h>

#include <engine/core/hash_map.h>
#include <engine/core/profiler.h>
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
  
  CRUDE_PROFILER_ZONE_NAME( "crude_game_menu_draw" );
  game = game_instance( );

  ImGui::PushFont( game->im_game_font );
  
  ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.10f, 0.10f, 0.10f, 1.00f));
  ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, ImVec4(0.15f, 0.15f, 0.15f, 1.00f));
  ImGui::PushStyleColor(ImGuiCol_FrameBgActive, ImVec4(0.08f, 0.08f, 0.08f, 1.00f));
  ImGui::PushStyleColor(ImGuiCol_SliderGrab, ImVec4(0.30f, 0.30f, 0.30f, 1.00f));
  ImGui::PushStyleColor(ImGuiCol_SliderGrabActive, ImVec4(0.40f, 0.40f, 0.40f, 1.00f));
  ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.12f, 0.12f, 0.12f, 1.00f));
  ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.18f, 0.18f, 0.18f, 1.00f));
  ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.25f, 0.25f, 0.25f, 1.00f));
  ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.95f, 0.95f, 0.95f, 1.00f));

  
  if ( CRUDE_ENTITY_HAS_COMPONENT( game->main_node, crude_level_boss_fight ) )
  {
    crude_level_boss_fight const *level =CRUDE_ENTITY_GET_IMMUTABLE_COMPONENT( game->main_node, crude_level_boss_fight );
    ImGui::SetNextWindowPos( ImVec2( 0, 0 ) );
    ImGui::SetNextWindowSize( ImVec2( game->gpu.vk_swapchain_width, 300 ) );
    ImGui::Begin( "Overlay Game", NULL, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoBackground );
    crude_boss *boss = CRUDE_ENTITY_GET_MUTABLE_COMPONENT( crude_ecs_lookup_entity_from_parent( game->main_node, "boss" ), crude_boss );
    ImDrawList *draw_list = ImGui::GetWindowDrawList( );
    
    if ( level->type == 0 )
    {
      uint32 bg_color = crude_color_set( 0.3, 0, 0.5, 1.0 );
      uint32 hp_color  = crude_color_set( 1, 0, 0, 1 );
      draw_list->AddRectFilled(
        { game->gpu.vk_swapchain_width / 2.f - 200.f, 20.f },
        { game->gpu.vk_swapchain_width / 2.f + 200.f, 40.f },
        bg_color
      );
      
      draw_list->AddRectFilled(
        { game->gpu.vk_swapchain_width / 2.f - 200.f, 20.f },
        { game->gpu.vk_swapchain_width / 2.f + 200.f - 400.f * ( 1.f - ( boss->health_eye_0 + boss->health_eye_1 + boss->health_eye_2 ) / 3.f ), 40.f },
        hp_color
      );
    }
    else
    {
      uint32 bg_color = crude_color_set( 1, 1, 1, 0.5 );
      uint32 hp_color  = crude_color_set( 0.3, 0, 0.5, 1.0 );
      draw_list->AddRectFilled(
        { game->gpu.vk_swapchain_width / 2.f - 200.f, 20.f },
        { game->gpu.vk_swapchain_width / 2.f + 200.f, 40.f },
        bg_color
      );
      
      draw_list->AddRectFilled(
        { game->gpu.vk_swapchain_width / 2.f - 200.f, 20.f },
        { game->gpu.vk_swapchain_width / 2.f + 200.f - 400.f * ( 1.f - boss->health ), 40.f },
        hp_color
      );
    }
    ImGui::End();
  }

  if ( game->death_screen )
  {
    ImGui::SetNextWindowPos( ImVec2( -50, -50 ) );
    ImGui::SetNextWindowSize( ImVec2( game->gpu.vk_swapchain_width + 100, game->gpu.vk_swapchain_height + 100 ) );
    ImGui::Begin( "Game Death Background", NULL, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoBackground );
    ImDrawList *draw_list = ImGui::GetWindowDrawList( );
    draw_list->AddRectFilled(
      { -50, -50 },
      { float32( game->gpu.vk_swapchain_width + 50 ), float32( game->gpu.vk_swapchain_height + 50 ) },
      crude_color_set( game->death_overlap_color.x, game->death_overlap_color.y, game->death_overlap_color.z, game->death_overlap_color.w )
    );
    ImGui::End();

    ImGui::SetNextWindowPos( ImVec2( ImGui::GetIO( ).DisplaySize.x * 0.5f, ImGui::GetIO( ).DisplaySize.y * 0.5f ), ImGuiCond_Always, ImVec2( 0.5f, 0.5f ) );
    ImGui::Begin( "Game Overlay", NULL, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoBackground );
    ImGui::Text( "YOU ARE DEAD" );
    ImGui::Text( "Press Space to restart..." );
    ImGui::End();
  }

  if ( menu->enabled )
  {
    ImGui::SetNextWindowPos( ImVec2( -50, -50 ) );
    ImGui::SetNextWindowSize( ImVec2( game->gpu.vk_swapchain_width + 100, game->gpu.vk_swapchain_height  + 100 ) );
    ImGui::Begin( "Game Menu Background", NULL, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoBackground );
    ImDrawList *draw_list = ImGui::GetWindowDrawList( );
    draw_list->AddRectFilled(
      { -50, -50 },
      { float32( game->gpu.vk_swapchain_width + 50 ), float32( game->gpu.vk_swapchain_height + 50 ) },
      crude_color_set( 0, 0, 0, 0.5 )
    );
    ImGui::End();

    ImGui::SetNextWindowPos( ImVec2( game->gpu.vk_swapchain_width * 0.5f - 200, game->gpu.vk_swapchain_height * 0.5f - 100 ) );
    ImGui::SetNextWindowSize( ImVec2( 400, 200 ) );

    ImGui::Begin( "Game Menu", NULL, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoBackground );
    
    ImGui::DragFloat( "Sensetivity (Default 1.0)", &game->sensetivity, 0.1 );
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
    if ( ImGui::DragInt( "FPS Limit", &framerate, 5, 1 ) )
    {
      game->framerate = framerate;
    }

    if ( game->gpu.vk_selected_present_mode == VK_PRESENT_MODE_IMMEDIATE_KHR )
    {
      ImGui::Text( "Immediate present mode" );
    }
    else if ( game->gpu.vk_selected_present_mode == VK_PRESENT_MODE_MAILBOX_KHR )
    {
      ImGui::Text( "Mailbox present mode" );
    }
    else if ( game->gpu.vk_selected_present_mode == VK_PRESENT_MODE_FIFO_KHR  )
    {
      ImGui::Text( "Fifo present mode" );
    }
    else if ( game->gpu.vk_selected_present_mode == VK_PRESENT_MODE_FIFO_RELAXED_KHR )
    {
      ImGui::Text( "Fifo relaxed present mode" );
    }

    ImGui::End( );
  }

  ImGui::PopStyleColor( 9 );
  ImGui::PopFont( );
  CRUDE_PROFILER_ZONE_END;
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