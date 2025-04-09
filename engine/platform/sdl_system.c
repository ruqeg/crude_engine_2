#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>

#include <platform/gui_components.h>
#include <platform/input_components.h>
#include <core/assert.h>

#include <platform/sdl_system.h>

static uint32
key_sym
(
  _In_ uint32 sdl_sym
)
{
  if (sdl_sym < 128)
  {
    return sdl_sym;
  }
  
  switch (sdl_sym)
  {
  case SDLK_RIGHT:  return 'R';
  case SDLK_LEFT:   return 'L';
  case SDLK_DOWN:   return 'D';
  case SDLK_UP:     return 'U';
  case SDLK_LCTRL:  return 'C';
  case SDLK_LSHIFT: return 'S';
  case SDLK_LALT:   return 'A';
  case SDLK_RCTRL:  return 'C';
  case SDLK_RSHIFT: return 'S';
  case SDLK_RALT:   return 'A';
  }
  return 0;
}

static void
key_down
(
  _In_ crude_key_state *key
)
{
  if (key->state)
  {
    key->pressed = false;
  }
  else
  {
    key->pressed = true;
  }
  
  key->state   = true;
  key->current = true;
}

static void
key_up
(
  _In_ crude_key_state *key
)
{
  key->current = false;
}

static void
key_reset
(
  _In_ crude_key_state *key
)
{
  if ( !key->current )
  {
    key->state   = 0;
    key->pressed = 0;
  }
  else if ( key->state )
  {
    key->pressed = 0;
  }
}

static void
mouse_reset
(
  _In_ crude_mouse_state *state
)
{
  state->rel.x = 0;
  state->rel.y = 0;
  
  state->scroll.x = 0;
  state->scroll.y = 0;
  
  state->view.x = 0;
  state->view.y = 0;
  
  state->wnd.x = 0;
  state->wnd.y = 0;
}

static void
sdl_create_window
(
  ecs_iter_t *it
)
{
  crude_window *windows = ecs_field( it, crude_window, 0 );

  for ( uint32 i = 0; i < it->count; ++i )
  {
    ecs_world_t *world = it->world;
    crude_window *window = &windows[ i ];
    ecs_entity_t *entity = it->entities[i];

    crude_window_handle *window_handle = ecs_ensure( world, entity, crude_window_handle );

    const char *title = ecs_doc_get_name( world, entity );
    if ( !title )
    {
      title = "SDL2 window";
    }

    SDL_WindowFlags flags = SDL_WINDOW_VULKAN | SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIGH_PIXEL_DENSITY;
    if ( window->maximized )
    {
      flags |= SDL_WINDOW_MAXIMIZED;
    }

    SDL_Window *created_window = SDL_CreateWindow( title, window->width, window->height, flags );
    if ( !created_window )
    {
      CRUDE_ABORT( CRUDE_CHANNEL_PLATFORM, "SDL2 window creation failed: %s", SDL_GetError() );
      return;
    }

    if ( window->maximized )
    {
      SDL_SetWindowBordered( created_window, false );
      
      int display_index = SDL_GetDisplayForWindow( created_window );
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

      SDL_SetWindowPosition( created_window, usable_bounds.x, usable_bounds.y );
      SDL_SetWindowSize( created_window, usable_bounds.w, usable_bounds.h );
    }
    else
    {
      SDL_SetWindowSize( created_window, window->width, window->height );
    }
    
    int32 actual_width, actual_height;
    SDL_GetWindowSizeInPixels(created_window, &actual_width, &actual_height);
    
    window_handle->value = created_window;
    window->width      = actual_width;
    window->height     = actual_height;
  }
}

static void
sdl_destroy_window
(
  ecs_iter_t *it
)
{
  crude_window_handle *windows = ecs_field( it, crude_window_handle, 0 );

  for ( uint32 i = 0; i < it->count; ++i )
  {
    SDL_DestroyWindow( windows[ i ].value );
  }
}

static void
sdl_shutdown
(
  ecs_world_t *world,
  void        *ctx
)
{
  SDL_Vulkan_UnloadLibrary();
  SDL_Quit();
  CRUDE_LOG_INFO( CRUDE_CHANNEL_PLATFORM, "SDL successfully shutdown" );
}

static void
sdl_process_events
(
  ecs_iter_t *it
)
{
  crude_input *inputs = ecs_field( it, crude_input, 0 );
  crude_window *app_windows = ecs_field( it, crude_window, 1 );
  crude_window_handle *app_windows_handles = ecs_field( it, crude_window_handle, 2 );
  
  for ( uint32 i = 0; i < it->count; ++i )
  {
    for ( uint32 k = 0; k < 128; k++ )
    {
      key_reset( &inputs[ i ].keys[ k ] );
    }

    key_reset( &inputs[ i ].mouse.left );
    key_reset( &inputs[ i ].mouse.right );

    mouse_reset( &inputs[ i ].mouse );

    SDL_Event sdl_event;
    while (SDL_PollEvent( &sdl_event ))
    {
      inputs[ i ].callback(&sdl_event);

      if ( sdl_event.type == SDL_EVENT_QUIT )
      {
        ecs_quit( it->world );
      }
      else if ( sdl_event.window.type == SDL_EVENT_WINDOW_RESIZED || sdl_event.window.type == SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED )
      {
        if ( SDL_GetWindowID( app_windows_handles[ i ].value) == sdl_event.window.windowID )
        {
          int actual_width, actual_height;
          SDL_GetWindowSizeInPixels( app_windows_handles[ i ].value, &actual_width, &actual_height );
          app_windows[ i ].width  = actual_width;
          app_windows[ i ].height = actual_height;
          ecs_modified( it->world, it->entities[ i ], crude_window );
          break;
        }
      }
      else if ( sdl_event.window.type == SDL_EVENT_WINDOW_CLOSE_REQUESTED )
      {
        ecs_quit( it->world );
      }
      else if ( sdl_event.type == SDL_EVENT_KEY_DOWN )
      {
        uint32 sym = key_sym( sdl_event.key.key );
        key_down( &inputs[ i ].keys[ sym ] );
      }
      else if ( sdl_event.type == SDL_EVENT_KEY_UP )
      {
        uint32 sym = key_sym( sdl_event.key.key );
        key_up( &inputs[ i ].keys[ sym ] );
      }
      else if ( sdl_event.type == SDL_EVENT_MOUSE_BUTTON_DOWN )
      {
        if ( sdl_event.button.button == SDL_BUTTON_LEFT )
        {
          key_down( &inputs[ i ].mouse.left );
        }
        else if ( sdl_event.button.button == SDL_BUTTON_RIGHT )
        {
          inputs[ i ].wrapwnd.x = sdl_event.motion.x;
          inputs[ i ].wrapwnd.y = sdl_event.motion.y;
          SDL_HideCursor();
          key_down( &inputs[ i ].mouse.right );
        }
      }
      else if ( sdl_event.type == SDL_EVENT_MOUSE_BUTTON_UP )
      {
        if ( sdl_event.button.button == SDL_BUTTON_LEFT )
        {
          key_up( &inputs[ i ].mouse.left );
        }
        else if ( sdl_event.button.button == SDL_BUTTON_RIGHT )
        {
          SDL_WarpMouseInWindow( app_windows_handles[ i ].value, inputs[ i ].wrapwnd.x, inputs[ i ].wrapwnd.y );
          SDL_ShowCursor();
          key_up( &inputs[ i ].mouse.right );
        }
      }
      else if ( sdl_event.type == SDL_EVENT_MOUSE_MOTION )
      {
        inputs[ i ].mouse.wnd.x = sdl_event.motion.x;
        inputs[ i ].mouse.wnd.y = sdl_event.motion.y;
        inputs[ i ].mouse.rel.x = sdl_event.motion.xrel;
        inputs[ i ].mouse.rel.y = sdl_event.motion.yrel;
  
        if ( !SDL_CursorVisible() )
        {
          SDL_WarpMouseInWindow( app_windows_handles[ i ].value, inputs[ i ].wrapwnd.x, inputs[ i ].wrapwnd.y );
        }
      }
      else if ( sdl_event.type == SDL_EVENT_MOUSE_WHEEL )
      {
        inputs[ i ].mouse.scroll.x = sdl_event.wheel.x;
        inputs[ i ].mouse.scroll.y = sdl_event.wheel.y;
      }
    }
  }
}

void
crude_sdl_systemImport
(
  ecs_world_t *world
)
{
  ECS_MODULE( world, crude_sdl_system );
  ECS_IMPORT( world, crude_gui_components );
  ECS_IMPORT( world, crude_input_components );
 
  ecs_observer( world, {
    .query.terms = { 
      ( ecs_term_t ) { .id = ecs_id( crude_window ) },
      ( ecs_term_t ) { .id = ecs_id( crude_window_handle ), .oper = EcsNot }
    },
    .events = { EcsOnSet },
    .callback = sdl_create_window
    });
  
  ecs_system( world, {
    .entity = ecs_entity( world, { .name = "sdl_process_events", .add = ecs_ids( ecs_dependson( EcsPreUpdate ) ) } ),
    .callback = sdl_process_events,
    .query.terms = { 
      {.id = ecs_id( crude_input ) },
      {.id = ecs_id( crude_window ) },
      {.id = ecs_id( crude_window_handle ) },
    } } );

  ecs_observer( world, {
    .query.terms = { { ecs_id( crude_window_handle ) } },
    .events = { EcsOnRemove },
    .callback = sdl_destroy_window
    } );

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

  ecs_atfini( world, sdl_shutdown, NULL );
}