#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>
#include <nfd.h>

#include <core/file.h>
#include <core/assert.h>

#include <platform/platform.h>

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