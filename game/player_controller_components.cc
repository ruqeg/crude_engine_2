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
  component->jump_velocity = cJSON_GetNumberValue( cJSON_GetObjectItemCaseSensitive( component_json, "jump_velocity" ) );
  component->walk_speed = cJSON_GetNumberValue( cJSON_GetObjectItemCaseSensitive( component_json, "walk_speed" ) );
  component->run_speed = cJSON_GetNumberValue( cJSON_GetObjectItemCaseSensitive( component_json, "run_speed" ) );
  component->stop_change_coeff = cJSON_GetNumberValue( cJSON_GetObjectItemCaseSensitive( component_json, "stop_change_coeff" ) );
  component->move_change_coeff = cJSON_GetNumberValue( cJSON_GetObjectItemCaseSensitive( component_json, "move_change_coeff" ) );
  component->weight = cJSON_GetNumberValue( cJSON_GetObjectItemCaseSensitive( component_json, "weight" ) );
  return true;
}

CRUDE_PARSE_COMPONENT_TO_JSON_FUNC_DECLARATION( crude_player_controller )
{
  cJSON *json = cJSON_CreateObject( );
  cJSON_AddItemToObject( json, "type", cJSON_CreateString( CRUDE_COMPONENT_STRING( crude_player_controller ) ) );
  cJSON_AddItemToObject( json, "rotation_speed", cJSON_CreateNumber( component->rotation_speed ) );
  cJSON_AddItemToObject( json, "jump_velocity", cJSON_CreateNumber( component->jump_velocity ) );
  cJSON_AddItemToObject( json, "walk_speed", cJSON_CreateNumber( component->walk_speed ) );
  cJSON_AddItemToObject( json, "run_speed", cJSON_CreateNumber( component->run_speed ) );
  cJSON_AddItemToObject( json, "stop_change_coeff", cJSON_CreateNumber( component->stop_change_coeff ) );
  cJSON_AddItemToObject( json, "move_change_coeff", cJSON_CreateNumber( component->move_change_coeff ) );
  cJSON_AddItemToObject( json, "weight", cJSON_CreateNumber( component->weight ) );
  return json;
}

CRUDE_PARSE_COMPONENT_TO_IMGUI_FUNC_DECLARATION( crude_player_controller )
{
  ImGui::DragFloat( "Rotation Speed", &component->rotation_speed, 0.01 );
  ImGui::DragFloat( "Jump Velocity", &component->jump_velocity, 0.01 );
  ImGui::DragFloat( "Walk Speed", &component->walk_speed, 0.01 );
  ImGui::DragFloat( "Run Speed", &component->run_speed, 0.01 );
  ImGui::DragFloat( "Stop Speed Coeff", &component->stop_change_coeff, 0.01 );
  ImGui::DragFloat( "Move Change Coeff", &component->move_change_coeff, 0.01 );
  ImGui::DragFloat( "Weight", &component->weight, 0.01 );
}