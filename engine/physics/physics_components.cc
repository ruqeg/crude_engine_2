#include <physics/physics_components.h>

ECS_COMPONENT_DECLARE( crude_physics_particle );
ECS_COMPONENT_DECLARE( crude_physics_particle_spring );

CRUDE_ECS_MODULE_IMPORT_IMPL( crude_physics_components )
{
  ECS_MODULE( world, crude_physics_components );
  ECS_COMPONENT_DEFINE( world, crude_physics_particle );
  ECS_COMPONENT_DEFINE( world, crude_physics_particle_spring );
}