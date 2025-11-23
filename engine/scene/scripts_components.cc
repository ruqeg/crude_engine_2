#include <cJSON.h>

#include <engine/core/string.h>
#include <engine/scene/scripts_components.h>
#include <engine/scene/node_manager.h>

ECS_COMPONENT_DECLARE( crude_free_camera );

CRUDE_COMPONENT_STRING_DEFINE( crude_free_camera, "crude_free_camera" );

CRUDE_ECS_MODULE_IMPORT_IMPL( crude_scripts_components )
{
  ECS_MODULE( world, crude_scripts_components );
  ECS_COMPONENT_DEFINE( world, crude_free_camera );
}

CRUDE_PARSE_JSON_TO_COMPONENT_FUNC_DECLARATION( crude_free_camera )
{
  crude_memory_set( component, 0, sizeof( crude_free_camera ) );
  crude_parse_json_to_float3( &component->moving_speed_multiplier, cJSON_GetObjectItemCaseSensitive( component_json, "moving_speed_multiplier" ) );
  crude_parse_json_to_float2( &component->rotating_speed_multiplier, cJSON_GetObjectItemCaseSensitive( component_json, "rotating_speed_multiplier" ) );
  return true;
}

CRUDE_PARSE_COMPONENT_TO_JSON_FUNC_DECLARATION( crude_free_camera )
{
  cJSON *free_camera_json = cJSON_CreateObject( );
  cJSON_AddItemToObject( free_camera_json, "type", cJSON_CreateString( "crude_free_camera" ) );
  cJSON_AddItemToObject( free_camera_json, "moving_speed_multiplier", cJSON_CreateFloatArray( &component->moving_speed_multiplier.x, 3 ) );
  cJSON_AddItemToObject( free_camera_json, "rotating_speed_multiplier", cJSON_CreateFloatArray( &component->rotating_speed_multiplier.x, 2 ) );
  return free_camera_json;
}