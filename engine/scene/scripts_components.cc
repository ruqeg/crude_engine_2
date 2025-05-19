#include <scene/scripts_components.h>

ECS_COMPONENT_DECLARE( crude_free_camera );

CRUDE_ECS_MODULE_IMPORT_IMPL( crude_scripts_components )
{
  ECS_MODULE( world, crude_scripts_components );
  ECS_COMPONENT_DEFINE( world, crude_free_camera );
}