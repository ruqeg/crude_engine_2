#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>
#include <imgui/backends/imgui_impl_sdl3.h>

#include <core/array.h>
#include <core/profiler.h>
#include <core/memory.h>
#include <core/assert.h>
#include <platform/platform_components.h>

#include <platform/platform_system.h>

CRUDE_ECS_SYSTEM_DECLARE( crude_process_events_system_ );
CRUDE_ECS_OBSERVER_DECLARE( crude_window_creation_observer_ );
CRUDE_ECS_OBSERVER_DECLARE( crude_window_destrotion_observer_ );

static uint32
key_sym_
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
key_down_
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
key_up_
(
  _In_ crude_key_state *key
)
{
  key->current = false;
}

static void
key_reset_
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
mouse_reset_
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
}

static void
crude_window_creation_observer_
(
  ecs_iter_t *it
)
{
  crude_window *windows_per_entity = ecs_field( it, crude_window, 0 );

  for ( uint32 i = 0; i < it->count; ++i )
  {
    ecs_world_t *world = it->world;
    crude_window *window = &windows_per_entity[ i ];
    ecs_entity_t entity = it->entities[i];

    crude_window_handle *window_handle = ecs_ensure( world, entity, crude_window_handle );

    const char *title = ecs_doc_get_name( world, entity );
    if ( !title )
    {
      title = "SDL2 window";
    }

    if ( window->maximized )
    {
      window->flags |= SDL_WINDOW_MAXIMIZED;
    }

    SDL_Window *created_window = SDL_CreateWindow( title, window->width, window->height, window->flags );
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
crude_window_destrotion_observer_
(
  ecs_iter_t *it
)
{
  crude_window_handle *windows_per_entity = ecs_field( it, crude_window_handle, 0 );

  for ( uint32 i = 0; i < it->count; ++i )
  {
    SDL_DestroyWindow( CRUDE_REINTERPRET_CAST( SDL_Window*, windows_per_entity[ i ].value ) );
  }
}

static void
crude_process_events_system_
(
  ecs_iter_t *it
)
{
  crude_input *inputs_per_entity = ecs_field( it, crude_input, 0 );
  crude_window *windows_per_entity = ecs_field( it, crude_window, 1 );
  crude_window_handle *windows_handles_per_entity = ecs_field( it, crude_window_handle, 2 );
  
  CRUDE_PROFILER_ZONE_NAME( "SDLProcessEvents" );

  /* Reset input */
  for ( uint32 i = 0; i < it->count; ++i )
  {
    crude_input *input = &inputs_per_entity[ i ];
    
    input->prev_mouse = input->mouse;
    for ( uint32 k = 0; k < 128; k++ )
    {
      input->prev_keys[ k ] = input->keys[ k ];
    }
  }
  
  for ( uint32 i = 0; i < it->count; ++i )
  {
    crude_input *input = &inputs_per_entity[ i ];

    for ( uint32 k = 0; k < 128; k++ )
    {
      key_reset_( &input->keys[ k ] );
    }

    key_reset_( &input->mouse.left );
    key_reset_( &input->mouse.right );

    mouse_reset_( &input->mouse );
  }

  /* Handle new input for each window*/
  SDL_Event sdl_event;
  while ( SDL_PollEvent( &sdl_event ) )
  {
    /* Handle Global event */
    if ( sdl_event.type == SDL_EVENT_QUIT )
    {
      ecs_quit( it->world );
    }
    
    /* Handle Window event */
    {
      crude_entity                                         focused_entity;
      crude_input                                         *focused_input;
      crude_window                                        *focused_window;
      crude_window_handle                                 *focused_window_handle;
      bool                                                 window_event;
          
      focused_input = NULL;
      focused_window = NULL;
      focused_window_handle = NULL;
      window_event = ( sdl_event.window.type == SDL_EVENT_WINDOW_RESIZED ) || ( sdl_event.window.type == SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED ) || ( sdl_event.window.type == SDL_EVENT_WINDOW_CLOSE_REQUESTED );

      if ( window_event )
      {
        for ( uint32 i = 0; i < it->count; ++i )
        {
          if ( sdl_event.window.windowID == SDL_GetWindowID( CRUDE_REINTERPRET_CAST( SDL_Window*, windows_handles_per_entity[ i ].value ) ) )
          {
            focused_input = &inputs_per_entity[ i ];
            focused_window = &windows_per_entity[ i ];
            focused_window_handle = &windows_handles_per_entity[ i ];
            focused_entity = CRUDE_COMPOUNT( crude_entity, { .handle = it->entities[ i ], .world = it->world } );
          }
        }
        
        if ( focused_input && focused_window && focused_window_handle )
        {
          if ( sdl_event.window.type == SDL_EVENT_WINDOW_RESIZED || sdl_event.window.type == SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED )
          {
            int actual_width, actual_height;
            SDL_GetWindowSizeInPixels( CRUDE_REINTERPRET_CAST( SDL_Window*, focused_window_handle->value ), &actual_width, &actual_height );
            focused_window->width  = actual_width;
            focused_window->height = actual_height;
            ecs_modified( it->world, focused_entity.handle, crude_window );
          }
          else if ( sdl_event.window.type == SDL_EVENT_WINDOW_CLOSE_REQUESTED )
          {
            focused_input->should_close_window = true;
          }
          focused_input->callback( focused_input->ctx, &sdl_event );
        }
      }
    }

    /* Handle Mouse event */
    {
      crude_input                                         *focused_input;
      crude_window                                        *focused_window;
      crude_window_handle                                 *focused_window_handle;
      bool                                                 mouse_event;
    
      focused_input = NULL;
      focused_window = NULL;
      focused_window_handle = NULL;
      mouse_event = ( sdl_event.type == SDL_EVENT_MOUSE_BUTTON_DOWN ) || ( sdl_event.type == SDL_EVENT_MOUSE_BUTTON_UP ) || ( sdl_event.type == SDL_EVENT_MOUSE_MOTION )  || ( sdl_event.type == SDL_EVENT_MOUSE_WHEEL );
    
      if ( mouse_event )
      {
        for ( uint32 i = 0; i < it->count; ++i )
        {
          if ( SDL_GetMouseFocus() == windows_handles_per_entity[ i ].value )
          {
            focused_input = &inputs_per_entity[ i ];
            focused_window = &windows_per_entity[ i ];
            focused_window_handle = &windows_handles_per_entity[ i ];
          }
        }

        if ( focused_input && focused_window && focused_window_handle )
        {
          if ( sdl_event.type == SDL_EVENT_MOUSE_BUTTON_DOWN )
          {
            if ( sdl_event.button.button == SDL_BUTTON_LEFT )
            {
              key_down_( &focused_input->mouse.left );
            }
            else if ( sdl_event.button.button == SDL_BUTTON_RIGHT )
            {
              key_down_( &focused_input->mouse.right );
            }
          }
          else if ( sdl_event.type == SDL_EVENT_MOUSE_BUTTON_UP )
          {
            if ( sdl_event.button.button == SDL_BUTTON_LEFT )
            {
              key_up_( &focused_input->mouse.left );
            }
            else if ( sdl_event.button.button == SDL_BUTTON_RIGHT )
            {
              key_up_( &focused_input->mouse.right );
            }
          }
          else if ( sdl_event.type == SDL_EVENT_MOUSE_MOTION )
          {
            focused_input->mouse.wnd.x = sdl_event.motion.x;
            focused_input->mouse.wnd.y = sdl_event.motion.y;
            focused_input->mouse.rel.x = sdl_event.motion.xrel;
            focused_input->mouse.rel.y = sdl_event.motion.yrel;
          }
          else if ( sdl_event.type == SDL_EVENT_MOUSE_WHEEL )
          {
            focused_input->mouse.scroll.x = sdl_event.wheel.x;
            focused_input->mouse.scroll.y = sdl_event.wheel.y;
          }
          focused_input->callback( focused_input->ctx, &sdl_event );
        }
      }
    }
    /* Handle Keyboard event */
    {
      crude_input                                         *focused_input;
      crude_window                                        *focused_window;
      crude_window_handle                                 *focused_window_handle;
      bool                                                 keyboard_event;
      
      focused_input = NULL;
      focused_window = NULL;
      focused_window_handle = NULL;
      keyboard_event = true;

      if ( keyboard_event )
      {
        for ( uint32 i = 0; i < it->count; ++i )
        {
          if ( SDL_GetKeyboardFocus() == windows_handles_per_entity[ i ].value )
          {
            focused_input = &inputs_per_entity[ i ];
            focused_window = &windows_per_entity[ i ];
            focused_window_handle = &windows_handles_per_entity[ i ];
          }
        }
        
        if ( focused_input && focused_window && focused_window_handle )
        {
          if ( sdl_event.type == SDL_EVENT_KEY_DOWN )
          {
            uint32 sym = key_sym_( sdl_event.key.key );
            key_down_( &focused_input->keys[ sym ] );
          }
          else if ( sdl_event.type == SDL_EVENT_KEY_UP )
          {
            uint32 sym = key_sym_( sdl_event.key.key );
            key_up_( &focused_input->keys[ sym ] );
          }
          focused_input->callback( focused_input->ctx, &sdl_event );
        }
      }
    }
  }
  CRUDE_PROFILER_END;
}

CRUDE_ECS_MODULE_IMPORT_IMPL( crude_platform_system )
{
  ECS_MODULE( world, crude_platform_system );
  ECS_IMPORT( world, crude_platform_components );

  CRUDE_ECS_OBSERVER_DEFINE( world, crude_window_creation_observer_, EcsOnSet, { 
    { .id = ecs_id( crude_window ) },
    { .id = ecs_id( crude_window_handle ), .oper = EcsNot }
  } );

  CRUDE_ECS_OBSERVER_DEFINE( world, crude_window_destrotion_observer_, EcsOnRemove, { 
    { .id = ecs_id( crude_window_handle ) }
  } );

  CRUDE_ECS_SYSTEM_DEFINE( world, crude_process_events_system_, EcsPreUpdate, NULL, {
    { .id = ecs_id( crude_input ) },
    { .id = ecs_id( crude_window ) },
    { .id = ecs_id( crude_window_handle ) },
  } );
}