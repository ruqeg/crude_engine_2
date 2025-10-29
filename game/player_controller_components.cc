#include <player_controller_components.h>

ECS_COMPONENT_DECLARE( crude_player_controller );

CRUDE_ECS_MODULE_IMPORT_IMPL( crude_crude_player_controller_components )
{
  ECS_MODULE( world, crude_crude_player_controller_components );
  ECS_COMPONENT_DEFINE( world, crude_player_controller );
}