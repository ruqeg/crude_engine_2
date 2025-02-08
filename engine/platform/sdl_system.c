#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>

#include <gui/gui.h>
#include <platform/input.h>
#include <platform/sdl_window.h>
#include <core/assert.h>

#include <platform/sdl_system.h>

static void sdl_create_window( ecs_iter_t *it )
{
  crude_window *window = ecs_field( it, crude_window, 0 );

  for ( uint32 i = 0; i < it->count; ++i )
  {
    ecs_world_t *world = it->world;
    ecs_entity_t *entity = it->entities[i];

    crude_window_handle *window_handle = ecs_ensure( world, entity, crude_window_handle );
    crude_sdl_window *sdl_window = ecs_ensure( world, entity, crude_sdl_window );
    
    const char *title = ecs_doc_get_name( world, entity );
    if ( !title )
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
  crude_window_handle *window = ecs_field( it, crude_window_handle, 0 );

  for ( uint32 i = 0; i < it->count; ++i )
  {
    SDL_DestroyWindow( window[i].value );
  }
}

static void sdl_shutdown( ecs_world_t *world, void *ctx )
{
  SDL_Vulkan_UnloadLibrary();
  SDL_Quit();
  CRUDE_LOG_INFO( CRUDE_CHANNEL_PLATFORM, "SDL successfully shutdown" );
}

static void sdl_process_events( ecs_iter_t *it )
{
  crude_input *input = ecs_field( it, crude_input, 0 );
  crude_window *app_window = ecs_field( it, crude_window, 1 );
  crude_window_handle *app_window_handle = ecs_field( it, crude_window_handle, 2 );
  
  uint32 current_event_index = 0u;

  SDL_Event sdl_event;
  while (SDL_PollEvent( &sdl_event ))
  {
    if ( sdl_event.type == SDL_EVENT_QUIT )
    {
      ecs_quit( it->world );
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
      ecs_quit( it->world );
    }
  }
}

void crude_sdl_systemImport( ecs_world_t *world )
{
  ECS_MODULE( world, crude_sdl_system );
  ECS_IMPORT( world, crude_gui_components );
  ECS_IMPORT( world, crude_input_components );
  ECS_IMPORT( world, crude_sdl_components );
 
  ecs_observer_desc_t s = (ecs_observer_desc_t) { .query.terms = { (ecs_term_t) { .id = ecs_id( crude_window ), .oper = EcsNot } } };
  
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