#include <imgui/imgui.h>

#include <core/memory.h>

#include <player_controller_components.h>

ECS_COMPONENT_DECLARE( crude_player_controller );

CRUDE_COMPONENT_STRING_DEFINE( crude_player_controller, "crude_player_controller" );

CRUDE_ECS_MODULE_IMPORT_IMPL( crude_player_controller_components )
{
  ECS_MODULE( world, crude_player_controller_components );
  ECS_COMPONENT_DEFINE( world, crude_player_controller );
}

CRUDE_PARSE_JSON_TO_COMPONENT_FUNC_DECLARATION( crude_player_controller )
{
  crude_memory_set( component, 0, sizeof( crude_player_controller ) );
  component->rotation_speed = cJSON_GetNumberValue( cJSON_GetObjectItemCaseSensitive( component_json, "rotation_speed" ) );
  component->weight = cJSON_GetNumberValue( cJSON_GetObjectItemCaseSensitive( component_json, "weight" ) );
  return true;
}

CRUDE_PARSE_COMPONENT_TO_JSON_FUNC_DECLARATION( crude_player_controller )
{
  cJSON *json = cJSON_CreateObject( );
  cJSON_AddItemToObject( json, "type", cJSON_CreateString( CRUDE_COMPONENT_STRING( crude_player_controller ) ) );
  cJSON_AddItemToObject( json, "rotation_speed", cJSON_CreateNumber( component->rotation_speed ) );
  cJSON_AddItemToObject( json, "weight", cJSON_CreateNumber( component->weight ) );
  return json;
}

CRUDE_PARSE_COMPONENT_TO_IMGUI_FUNC_DECLARATION( crude_player_controller )
{
  ImGui::DragFloat( "Rotation Speed", &component->rotation_speed, 0.01 );
  ImGui::DragFloat( "Weight", &component->weight, 0.01 );
}