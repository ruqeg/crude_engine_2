#include <core/ecs_utils.h>

#include <platform/platform_components.h>

CRUDE_ECS_COMPONENT_DECLARE( crude_input );
CRUDE_ECS_COMPONENT_DECLARE( crude_window );
CRUDE_ECS_COMPONENT_DECLARE( crude_window_handle );

CRUDE_ECS_MODULE_IMPORT_IMPL( crude_platform_components )
{
  ECS_MODULE( world, crude_platform_components );
  ECS_COMPONENT_DEFINE( world, crude_input );
  ECS_COMPONENT_DEFINE( world, crude_window );
  ECS_COMPONENT_DEFINE( world, crude_window_handle );
}