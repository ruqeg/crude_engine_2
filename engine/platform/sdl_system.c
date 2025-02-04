#include <platform/sdl_system.h>
#include <platform/sdl_window.h>
#include <platform/input.h>
#include <gui/gui.h>
#include <scene/entity.h>
#include <core/assert.h>
#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>

static void sdl_create_window( ecs_iter_t *it )
{
  crude_window *window = ecs_field( it, crude_window, 1 );
  ecs_entity_t ecs_id( crude_window_handle ) = ecs_field_id( it, 2 );

  for ( uint32 i = 0; i < it->count; ++i )
  {
    crude_entity entity = ( crude_entity ){ it->entities[i], it->world };
    crude_window_handle *window_handle = CRUDE_ENTITY_GET_OR_ADD_COMPONENT( entity, crude_window_handle );
    
    ecs_add( it->world, it->entities[i], crude_sdl_window );
    
    const char *title = CRUDE_ENTITY_GET_NAME( entity );
    if (!title)
    {
      title = "SDL2 window";
    }
    
    int x = SDL_WINDOWPOS_UNDEFINED;
    int y = SDL_WINDOWPOS_UNDEFINED;
    uint32 flags = SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIGH_PIXEL_DENSITY;
    if ( window[i].maximized )
      flags |= SDL_WINDOW_MAXIMIZED;

    SDL_Window *created_window = SDL_CreateWindow( title, x, y, window[i].width, window[i].height, flags );
    if ( !created_window )
    {
      CRUDE_ABORT( CRUDE_CHANNEL_PLATFORM, "SDL2 window creation failed: %s", SDL_GetError() );
      return;
    }
    
    if ( window[i].maximized )
    {
      SDL_SetWindowBordered( created_window, false );
      
      int display_index = SDL_GetDisplayForWindow( created_window );
      if ( display_index < 0 )
      {
        CRUDE_ABORT( CRUDE_CHANNEL_PLATFORM, "Error getting window display" );
        return;
      }
      
      SDL_Rect usable_bounds;
      if ( 0 != SDL_GetDisplayUsableBounds( display_index, &usable_bounds ) )
      {
        CRUDE_ABORT( CRUDE_CHANNEL_PLATFORM, "Error getting usable bounds" );
        return;
      }
      
      SDL_SetWindowPosition( created_window, usable_bounds.x, usable_bounds.y );
      SDL_SetWindowSize( created_window, usable_bounds.w, usable_bounds.h );
    }
    else
    {
      SDL_SetWindowSize( created_window, window[i].width, window[i].height );
    }
    
    int32 actual_width, actual_height;
    SDL_GetWindowSizeInPixels(created_window, &actual_width, &actual_height);
    
    window_handle->value = created_window;
    window[i].width      = actual_width;
    window[i].height     = actual_height;
  }
}

static void sdl_destroy_window( ecs_iter_t *it )
{
  crude_window_handle *window = ecs_field( it, crude_window_handle, 1 );

  for ( uint32 i = 0; i < it->count; ++i )
  {
    SDL_DestroyWindow( window[i].value );
  }
}

static void sdl_shutdown( crude_world *world, void *ctx )
{
  SDL_Quit();
  CRUDE_LOG_INFO( CRUDE_CHANNEL_PLATFORM, "SDL successfully shutdown" );
}

static void sdl_process_events( ecs_iter_t *it )
{
  crude_input *input = ecs_field( it, crude_input, 1 );
  crude_window *app_window = ecs_field( it, crude_window, 2 );
  crude_window_handle *app_window_handle = ecs_field( it, crude_window_handle, 3 );
  
  SDL_Event sdl_event;
  while (SDL_PollEvent( &sdl_event ))
  {
    if (input->callback)
    {
      input->callback( &sdl_event );
    }

    if ( sdl_event.type == SDL_EVENT_QUIT )
    {
      crude_world_destroy( it->world );
    }
    else if ( sdl_event.window.type == SDL_EVENT_WINDOW_RESIZED || sdl_event.window.type == SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED )
    {
      for ( int32 i = 0; i < it->count; ++i )
      {
        if ( SDL_GetWindowID( app_window_handle[i]. value) == sdl_event.window.windowID )
        {
          int actual_width, actual_height;
          SDL_GetWindowSizeInPixels( app_window_handle[i].value, &actual_width, &actual_height );
          app_window[i].width  = actual_width;
          app_window[i].height = actual_height;
          ecs_modified( it->world, it->entities[i], crude_window );
          break;
        }
      }
    }
    else if ( sdl_event.window.type == SDL_EVENT_WINDOW_CLOSE_REQUESTED )
    {
      crude_world_destroy( it->world );
    }
  }
}

void crude_sdl_systemImport( crude_world *world )
{
  ECS_TAG( world, OnInput );

  ECS_MODULE( world, sdl_system );
  ECS_OBSERVER( world, sdl_create_window, EcsOnSet, [in] crude_gui.components.crude_window, !crude_gui.components.crude_window );
  ECS_SYSTEM( world, sdl_process_events, OnInput, crude_input.components.crude_input($), crude_gui.components.crude_window, crude_gui.components.crude_window_handle );
  ECS_OBSERVER( world, sdl_destroy_window, EcsOnDelete, [in] crude_gui.components.crude_window_handle );

  ecs_set_ptr( world, ecs_id(crude_input), crude_input, NULL );

  if (SDL_Init( SDL_INIT_VIDEO ) != 0)
  {
    CRUDE_ABORT( CRUDE_CHANNEL_PLATFORM, "Unable to initialize SDL: %s", SDL_GetError() );
    return;
  }

  ecs_atfini( world, sdl_shutdown, NULL );
}