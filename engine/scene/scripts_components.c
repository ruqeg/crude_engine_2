#include <scene/scripts_components.h>

ECS_COMPONENT_DECLARE( crude_free_camera );

void
crude_scripts_componentsImport
(
  ecs_world_t *world
)
{
  ECS_MODULE( world, crude_scripts_components );
  ECS_COMPONENT_DEFINE( world, crude_free_camera );
}