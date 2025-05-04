#include <graphics/graphics_components.h>

ECS_COMPONENT_DECLARE( crude_graphics_creation );
ECS_COMPONENT_DECLARE( crude_graphics_component );

void
crude_graphics_componentsImport
(
  ecs_world_t *world
)
{
  ECS_MODULE( world, crude_graphics_components );
  ECS_COMPONENT_DEFINE( world, crude_graphics_creation );
  ECS_COMPONENT_DEFINE( world, crude_graphics_component );
}