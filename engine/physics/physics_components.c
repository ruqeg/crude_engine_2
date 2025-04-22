#include <physics/physics_components.h>

ECS_COMPONENT_DECLARE( crude_physics_particle );
ECS_COMPONENT_DECLARE( crude_physics_particle_spring );

void
crude_physics_componentsImport
(
  ecs_world_t *world
)
{
  ECS_MODULE( world, crude_physics_components );
  ECS_COMPONENT_DEFINE( world, crude_physics_particle );
  ECS_COMPONENT_DEFINE( world, crude_physics_particle_spring );
}