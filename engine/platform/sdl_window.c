#include <platform/sdl_window.h>

ECS_COMPONENT_DECLARE( crude_sdl_window );

void crude_sdl_componentsImport( ecs_world_t *world )
{
  ECS_MODULE( world, crude_sdl_components );
  ECS_COMPONENT_DEFINE( world, crude_sdl_window );
}