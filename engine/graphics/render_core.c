#include <stb_ds.h>
#include <SDL3/SDL_vulkan.h>

#include <graphics/render_core.h>

ECS_COMPONENT_DECLARE( crude_render_core );
ECS_COMPONENT_DECLARE( crude_render_core_config );

void crude_render_core_componentsImport( ecs_world_t *world )
{
  ECS_MODULE( world, crude_render_core_components );
  ECS_COMPONENT_DEFINE( world, crude_render_core );
  ECS_COMPONENT_DEFINE( world, crude_render_core_config );
}