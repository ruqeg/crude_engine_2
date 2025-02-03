#include <platform/sdl_system.h>
#include <platform/sdl_window.h>
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
      
      int display_index = SDL_GetWindowDisplayIndex( created_window );
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

void crude_sdl_system_import( crude_world *world )
{
  ECS_TAG( world, OnInput );

  ECS_MODULE( world, sdl_system );
  ECS_OBSERVER( world, sdl_create_window, EcsOnSet, [in] gui.components.crude_window, !gui.components.crude_window );
  ECS_SYSTEM( world, SdlProcessEvents, OnInput, input.components.Input($), gui.components.crude_window, gui.components.crude_window_handle );
  ECS_OBSERVER( world, sdl_destroy_window, EcsOnDelete, [in] gui.components.crude_window_handle );

  ecs_set_ptr( world, ecs_id(crude_input), crude_input, NULL );

  if (SDL_Init( SDL_INIT_VIDEO ) != 0)
  {
    CRUDE_ABORT( CRUDE_CHANNEL_PLATFORM, "Unable to initialize SDL: %s", SDL_GetError() );
    return;
  }

  ecs_atfini( world, sdl_shutdown, NULL );
}