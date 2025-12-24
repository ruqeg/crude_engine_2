#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>
#include <nfd.h>

#include <engine/core/profiler.h>
#include <engine/core/file.h>
#include <engine/core/assert.h>

#include <engine/platform/platform.h>

void
crude_platform_service_initialize
(
)
{
  NFD_Init( );

  if ( !SDL_Init( SDL_INIT_VIDEO ) )
  {
    CRUDE_ABORT( CRUDE_CHANNEL_PLATFORM, "Unable to initialize SDL: %s", SDL_GetError() );
    return;
  }

  if ( !SDL_Vulkan_LoadLibrary( NULL ) )
  {
    CRUDE_ABORT( CRUDE_CHANNEL_PLATFORM, "Unable to load SDL Vulkan: %s", SDL_GetError() );
    return;
  }
}

void
crude_platform_service_deinitialize
(
)
{
  SDL_Vulkan_UnloadLibrary();
  SDL_Quit();
  
  NFD_Quit( );
  
  CRUDE_LOG_INFO( CRUDE_CHANNEL_PLATFORM, "SDL successfully shutdown" );
}

void
crude_platform_intialize
(
  _In_ crude_platform                                     *platform,
  _In_ crude_platform_creation const                      *creation
)
{
  char const                                              *title;
  uint64                                                   flags;
  int32                                                    actual_width, actual_height;

  if ( !creation->window.title )
  {
    title = "CE2";
  }
  
  flags = creation->window.flags;
  if ( creation->window.maximized )
  {
    flags |= SDL_WINDOW_MAXIMIZED;
  }
  
  platform->sdl_window = SDL_CreateWindow( title, creation->window.width, creation->window.height, flags );
  if ( !platform->sdl_window )
  {
    CRUDE_ABORT( CRUDE_CHANNEL_PLATFORM, "SDL2 window creation failed: %s", SDL_GetError() );
    return;
  }
  
  if ( creation->window.maximized )
  {
    SDL_SetWindowBordered( platform->sdl_window, false );
    
    int64 display_index = SDL_GetDisplayForWindow( platform->sdl_window );
    if ( display_index < 0 )
    {
      CRUDE_ABORT( CRUDE_CHANNEL_PLATFORM, "Error getting window display" );
      return;
    }
    
    SDL_Rect usable_bounds;
    if ( SDL_GetDisplayUsableBounds( display_index, &usable_bounds ) )
    {
      CRUDE_ABORT( CRUDE_CHANNEL_PLATFORM, "Error getting usable bounds" );
      return;
    }
  
    SDL_SetWindowPosition( platform->sdl_window, usable_bounds.x, usable_bounds.y );
    SDL_SetWindowSize( platform->sdl_window, usable_bounds.w, usable_bounds.h );
  }
  else
  {
    SDL_SetWindowSize( platform->sdl_window, creation->window.width, creation->window.height );
  }
  
  SDL_GetWindowSizeInPixels( platform->sdl_window, &actual_width, &actual_height);

  platform->width = actual_width;
  platform->height = actual_height;
  platform->flags = creation->window.flags;
  platform->maximized = creation->window.maximized;

  platform->quit_callback = creation->quit_callback;
  platform->quit_callback_ctx = creation->quit_callback_ctx;
  platform->input.callback = creation->input_callback;
  platform->input.ctx = creation->input_callback_ctx;
}

void
crude_platform_deintialize
(
  _In_ crude_platform                                     *platform
)
{
  SDL_DestroyWindow( platform->sdl_window );
}

void
crude_platform_update
(
  _In_ crude_platform                                     *platform
)
{
  SDL_Event                                                sdl_event;

  CRUDE_PROFILER_ZONE_NAME( "crude_process_events_system_" );

  platform->input.prev_mouse = platform->input.mouse;
  for ( uint64 k = 0; k < SDL_SCANCODE_COUNT; k++ )
  {
    platform->input.prev_keys[ k ] = platform->input.keys[ k ];
  }
  
  for ( uint32 k = 0; k < SDL_SCANCODE_COUNT; k++ )
  {
    crude_platform_key_reset( &platform->input.keys[ k ] );
  }
  
  crude_platform_key_reset( &platform->input.mouse.left );
  crude_platform_key_reset( &platform->input.mouse.right );
  
  crude_platform_mouse_reset( &platform->input.mouse );

  /* Handle new input for each window*/
  while ( SDL_PollEvent( &sdl_event ) )
  {
    bool                                                   is_window_event, is_mouse_event, is_keyboard_event;

    /* Handle Global event */
    if ( sdl_event.type == SDL_EVENT_QUIT )
    {
      platform->quit_callback( platform->quit_callback_ctx );
    }
    
    is_window_event = ( sdl_event.window.type == SDL_EVENT_WINDOW_RESIZED ) || ( sdl_event.window.type == SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED ) || ( sdl_event.window.type == SDL_EVENT_WINDOW_CLOSE_REQUESTED );
    is_window_event = is_window_event && ( sdl_event.window.windowID == SDL_GetWindowID( platform->sdl_window ) );
    if ( is_window_event )
    {
      if ( sdl_event.window.type == SDL_EVENT_WINDOW_RESIZED || sdl_event.window.type == SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED )
      {
        int actual_width, actual_height;
        SDL_GetWindowSizeInPixels( platform->sdl_window, &actual_width, &actual_height );
        platform->width  = actual_width;
        platform->height = actual_height;
      }
      else if ( sdl_event.window.type == SDL_EVENT_WINDOW_CLOSE_REQUESTED )
      {
        platform->quit_callback( platform->quit_callback_ctx );
      }
      
      if ( platform->input.callback )
      {
        platform->input.callback( platform->input.ctx, &sdl_event );
      }
    }

    is_mouse_event = ( sdl_event.type == SDL_EVENT_MOUSE_BUTTON_DOWN ) || ( sdl_event.type == SDL_EVENT_MOUSE_BUTTON_UP ) || ( sdl_event.type == SDL_EVENT_MOUSE_MOTION )  || ( sdl_event.type == SDL_EVENT_MOUSE_WHEEL );
    is_mouse_event = is_mouse_event && ( SDL_GetMouseFocus() == platform->sdl_window );
    if ( is_mouse_event )
    {
      if ( sdl_event.type == SDL_EVENT_MOUSE_BUTTON_DOWN )
      {
        if ( sdl_event.button.button == SDL_BUTTON_LEFT )
        {
          crude_platform_key_down( &platform->input.mouse.left );
        }
        else if ( sdl_event.button.button == SDL_BUTTON_RIGHT )
        {
          crude_platform_key_down( &platform->input.mouse.right );
        }
      }
      else if ( sdl_event.type == SDL_EVENT_MOUSE_BUTTON_UP )
      {
        if ( sdl_event.button.button == SDL_BUTTON_LEFT )
        {
          crude_platform_key_up( &platform->input.mouse.left );
        }
        else if ( sdl_event.button.button == SDL_BUTTON_RIGHT )
        {
          crude_platform_key_up( &platform->input.mouse.right );
        }
      }
      else if ( sdl_event.type == SDL_EVENT_MOUSE_MOTION )
      {
        platform->input.mouse.wnd.x = sdl_event.motion.x;
        platform->input.mouse.wnd.y = sdl_event.motion.y;
        platform->input.mouse.rel.x = sdl_event.motion.xrel;
        platform->input.mouse.rel.y = sdl_event.motion.yrel;
      }
      else if ( sdl_event.type == SDL_EVENT_MOUSE_WHEEL )
      {
        platform->input.mouse.scroll.x = sdl_event.wheel.x;
        platform->input.mouse.scroll.y = sdl_event.wheel.y;
      }

      if ( platform->input.callback )
      {
        platform->input.callback( platform->input.ctx, &sdl_event );
      }
    }

    is_keyboard_event = ( SDL_GetKeyboardFocus( ) == platform->sdl_window );
    if ( is_keyboard_event )
    {
      if ( sdl_event.type == SDL_EVENT_KEY_DOWN )
      {
        crude_platform_key_down( &platform->input.keys[ sdl_event.key.scancode ] );
      }
      else if ( sdl_event.type == SDL_EVENT_KEY_UP )
      {
        crude_platform_key_up( &platform->input.keys[ sdl_event.key.scancode ] );
      }
      
      if ( platform->input.callback )
      {
        platform->input.callback( platform->input.ctx, &sdl_event );
      }
    }
  }
  CRUDE_PROFILER_ZONE_END;
}

void
crude_platform_hide_cursor
(
  _In_ crude_platform                                     *platform
)
{
  SDL_SetWindowRelativeMouseMode( platform->sdl_window, true );
}

void
crude_platform_show_cursor
(
  _In_ crude_platform                                     *platform
)
{
  SDL_SetWindowRelativeMouseMode( platform->sdl_window, false );
}