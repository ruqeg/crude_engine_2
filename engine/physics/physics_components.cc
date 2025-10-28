#include <physics/physics_components.h>

ECS_COMPONENT_DECLARE( crude_physics_static_body );
ECS_COMPONENT_DECLARE( crude_physics_static_body_handle );
ECS_COMPONENT_DECLARE( crude_physics_dynamic_body );
ECS_COMPONENT_DECLARE( crude_physics_dynamic_body_handle );

CRUDE_ECS_MODULE_IMPORT_IMPL( crude_physics_components )
{
  ECS_MODULE( world, crude_physics_components );
  ECS_COMPONENT_DEFINE( world, crude_physics_static_body );
  ECS_COMPONENT_DEFINE( world, crude_physics_static_body_handle );
  ECS_COMPONENT_DEFINE( world, crude_physics_dynamic_body );
  ECS_COMPONENT_DEFINE( world, crude_physics_dynamic_body_handle );
}