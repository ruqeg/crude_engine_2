#include <imgui/imgui.h>

#include <core/memory.h>

#include <enemy_components.h>

ECS_COMPONENT_DECLARE( crude_enemy );

CRUDE_COMPONENT_STRING_DEFINE( crude_enemy, "crude_enemy" );

CRUDE_ECS_MODULE_IMPORT_IMPL( crude_enemy_components )
{
  ECS_MODULE( world, crude_enemy_components );
  ECS_COMPONENT_DEFINE( world, crude_enemy );
}

CRUDE_PARSE_JSON_TO_COMPONENT_FUNC_DECLARATION( crude_enemy )
{
  crude_memory_set( component, 0, sizeof( crude_enemy ) );
  component->moving_speed = cJSON_GetNumberValue( cJSON_GetObjectItemCaseSensitive( component_json, "moving_speed" ) );
  return true;
}

CRUDE_PARSE_COMPONENT_TO_JSON_FUNC_DECLARATION( crude_enemy )
{
  cJSON *json = cJSON_CreateObject( );
  cJSON_AddItemToObject( json, "type", cJSON_CreateString( CRUDE_COMPONENT_STRING( crude_enemy ) ) );
  cJSON_AddItemToObject( json, "moving_speed", cJSON_CreateNumber( component->moving_speed ) );
  return json;
}

CRUDE_PARSE_COMPONENT_TO_IMGUI_FUNC_DECLARATION( crude_enemy )
{
  ImGui::DragFloat( "Moving Speed", &component->moving_speed, 0.01 );
}