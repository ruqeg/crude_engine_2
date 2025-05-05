#include <graphics/graphics_components.h>

ECS_COMPONENT_DECLARE( crude_gfx_graphics_handle );
ECS_COMPONENT_DECLARE( crude_gfx_graphics_creation );

void
crude_graphics_componentsImport
(
  ecs_world_t *world
)
{
  ECS_MODULE( world, crude_graphics_components );
  ECS_COMPONENT_DEFINE( world, crude_gfx_graphics_handle );
  ECS_COMPONENT_DEFINE( world, crude_gfx_graphics_creation );
}