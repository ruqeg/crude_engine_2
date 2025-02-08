#include <platform/input.h>

ECS_COMPONENT_DECLARE( crude_input );

void crude_input_componentsImport( ecs_world_t *world )
{
  ECS_MODULE( world, crude_input_components );
  ECS_COMPONENT_DEFINE( world, crude_input );
}