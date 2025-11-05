#include <physics/physics_components.h>

ECS_COMPONENT_DECLARE( crude_physics_static_body );
ECS_COMPONENT_DECLARE( crude_physics_dynamic_body );

CRUDE_COMPONENT_STRING_DEFINE( crude_physics_static_body, "crude_physics_static_body" );
CRUDE_COMPONENT_STRING_DEFINE( crude_physics_dynamic_body, "crude_physics_dynamic_body" );

CRUDE_ECS_MODULE_IMPORT_IMPL( crude_physics_components )
{
  ECS_MODULE( world, crude_physics_components );
  ECS_COMPONENT_DEFINE( world, crude_physics_static_body );
  ECS_COMPONENT_DEFINE( world, crude_physics_dynamic_body );
}

CRUDE_PARSE_JSON_TO_COMPONENT_FUNC_DECLARATION( crude_physics_static_body )
{
  crude_memory_set( component, 0, sizeof( crude_physics_static_body ) );
  return true;
}

CRUDE_PARSE_COMPONENT_TO_JSON_FUNC_DECLARATION( crude_physics_static_body )
{
  cJSON *static_body_json = cJSON_CreateObject( );
  cJSON_AddItemToObject( static_body_json, "type", cJSON_CreateString( CRUDE_COMPONENT_STRING( crude_physics_static_body ) ) );
  return static_body_json;
}

CRUDE_PARSE_JSON_TO_COMPONENT_FUNC_DECLARATION( crude_physics_dynamic_body )
{
  crude_memory_set( component, 0, sizeof( crude_physics_dynamic_body ) );
  return true;
}

CRUDE_PARSE_COMPONENT_TO_JSON_FUNC_DECLARATION( crude_physics_dynamic_body )
{
  cJSON *dynamic_body_json = cJSON_CreateObject( );
  cJSON_AddItemToObject( dynamic_body_json, "type", cJSON_CreateString( CRUDE_COMPONENT_STRING( crude_physics_dynamic_body ) ) );
  return dynamic_body_json;
}