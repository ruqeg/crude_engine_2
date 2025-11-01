#include <core/memory.h>

#include <player_controller_components.h>

ECS_COMPONENT_DECLARE( crude_player_controller );

CRUDE_ECS_MODULE_IMPORT_IMPL( crude_crude_player_controller_components )
{
  ECS_MODULE( world, crude_crude_player_controller_components );
  ECS_COMPONENT_DEFINE( world, crude_player_controller );
}

CRUDE_PARSE_JSON_TO_COMPONENT_FUNC_DECLARATION( crude_player_controller )
{
  crude_memory_set( component, 0, sizeof( crude_player_controller ) );
  return true;
}
CRUDE_PARSE_COMPONENT_TO_JSON_FUNC_DECLARATION( crude_player_controller )
{
  cJSON *light_json = cJSON_CreateObject( );
  return light_json;
}