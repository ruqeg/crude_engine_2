#include <imgui/imgui.h>

#include <engine/core/memory.h>
#include <engine/scene/scripts_components.h>
#include <engine/external/game_components.h>

ECS_TAG_DECLARE( crude_serum_station_enabled );

ECS_COMPONENT_DECLARE( crude_serum_station );
ECS_COMPONENT_DECLARE( crude_enemy );
ECS_COMPONENT_DECLARE( crude_weapon );
ECS_COMPONENT_DECLARE( crude_level_01 );
ECS_COMPONENT_DECLARE( crude_player_controller );
ECS_COMPONENT_DECLARE( crude_player );
ECS_COMPONENT_DECLARE( crude_recycle_station );

CRUDE_COMPONENT_STRING_DEFINE( crude_serum_station, "crude_serum_station" );
CRUDE_COMPONENT_STRING_DEFINE( crude_enemy, "crude_enemy" );
CRUDE_COMPONENT_STRING_DEFINE( crude_weapon, "crude_weapon" );
CRUDE_COMPONENT_STRING_DEFINE( crude_level_01, "crude_level_01" );
CRUDE_COMPONENT_STRING_DEFINE( crude_player_controller, "crude_player_controller" );
CRUDE_COMPONENT_STRING_DEFINE( crude_player, "crude_player" );
CRUDE_COMPONENT_STRING_DEFINE( crude_recycle_station, "crude_recycle_station" );

CRUDE_ECS_MODULE_IMPORT_IMPL( crude_game_components )
{
  ECS_MODULE( world, crude_game_components );
  ECS_COMPONENT_DEFINE( world, crude_serum_station );
  ECS_COMPONENT_DEFINE( world, crude_enemy );
  ECS_COMPONENT_DEFINE( world, crude_weapon );
  ECS_COMPONENT_DEFINE( world, crude_level_01 );
  ECS_COMPONENT_DEFINE( world, crude_player_controller );
  ECS_COMPONENT_DEFINE( world, crude_player );
  ECS_COMPONENT_DEFINE( world, crude_recycle_station );
  ECS_TAG_DEFINE( world, crude_serum_station_enabled );
}

CRUDE_PARSE_JSON_TO_COMPONENT_FUNC_DECLARATION( crude_serum_station )
{
  crude_memory_set( component, 0, sizeof( crude_serum_station ) );
  return true;
}

CRUDE_PARSE_COMPONENT_TO_JSON_FUNC_DECLARATION( crude_serum_station )
{
  cJSON *json = cJSON_CreateObject( );
  cJSON_AddItemToObject( json, "type", cJSON_CreateString( CRUDE_COMPONENT_STRING( crude_serum_station ) ) );
  return json;
}

CRUDE_PARSE_COMPONENT_TO_IMGUI_FUNC_DECLARATION( crude_serum_station )
{
}

CRUDE_PARSE_JSON_TO_COMPONENT_FUNC_DECLARATION( crude_enemy )
{
  crude_memory_set( component, 0, sizeof( crude_enemy ) );
  return true;
}

CRUDE_PARSE_COMPONENT_TO_JSON_FUNC_DECLARATION( crude_enemy )
{
  cJSON *json = cJSON_CreateObject( );
  cJSON_AddItemToObject( json, "type", cJSON_CreateString( CRUDE_COMPONENT_STRING( crude_enemy ) ) );
  return json;
}

CRUDE_PARSE_COMPONENT_TO_IMGUI_FUNC_DECLARATION( crude_enemy )
{
  ImGui::DragFloat( "Moving Speed", &component->moving_speed, 0.01 );
}

CRUDE_PARSE_JSON_TO_COMPONENT_FUNC_DECLARATION( crude_weapon )
{
  crude_memory_set( component, 0, sizeof( crude_weapon ) );
  component->max_ammo = cJSON_GetNumberValue( cJSON_GetObjectItemCaseSensitive( component_json, "max_ammo" ) );
  return true;
}

CRUDE_PARSE_COMPONENT_TO_JSON_FUNC_DECLARATION( crude_weapon )
{
  cJSON *json = cJSON_CreateObject( );
  cJSON_AddItemToObject( json, "type", cJSON_CreateString( CRUDE_COMPONENT_STRING( crude_weapon ) ) );
  cJSON_AddItemToObject( json, "max_ammo", cJSON_CreateNumber( component->max_ammo ) );
  return json;
}

CRUDE_PARSE_COMPONENT_TO_IMGUI_FUNC_DECLARATION( crude_weapon )
{
  ImGui::DragInt( "Max Ammo", &component->max_ammo, 0.01 );
}

CRUDE_PARSE_JSON_TO_COMPONENT_FUNC_DECLARATION( crude_level_01 )
{
  crude_memory_set( component, 0, sizeof( crude_level_01 ) );
  return true;
}

CRUDE_PARSE_COMPONENT_TO_JSON_FUNC_DECLARATION( crude_level_01 )
{
  cJSON *json = cJSON_CreateObject( );
  cJSON_AddItemToObject( json, "type", cJSON_CreateString( CRUDE_COMPONENT_STRING( crude_level_01 ) ) );
  return json;
}

CRUDE_PARSE_COMPONENT_TO_IMGUI_FUNC_DECLARATION( crude_level_01 )
{
  //crude_entity editor_camera_node = crude_ecs_lookup_entity_from_parent( node, "editor_camera" );
  //crude_entity game_controller_node = crude_ecs_lookup_entity_from_parent( node, "player" );
  //crude_entity game_camera_node = crude_ecs_lookup_entity_from_parent( node, "player.pivot.camera" );
  //
  //if ( ImGui::Checkbox( "Editor Camera Controller", &component->editor_camera_controller_enabled ) )
  //{
  //  if ( component->editor_camera_controller_enabled )
  //  {
  //    crude_free_camera *free_camera = CRUDE_ENTITY_GET_MUTABLE_COMPONENT( editor_camera_node, crude_free_camera );
  //    crude_player_controller *player_controller = CRUDE_ENTITY_GET_MUTABLE_COMPONENT( game_controller_node, crude_player_controller );
  //    
  //    game_set_focused_camera_node( game_instance( ), editor_camera_node );
  //    player_controller->input_enabled = false;
  //    free_camera->enabled = true;
  //  }
  //  else
  //  { 
  //    crude_free_camera *free_camera = CRUDE_ENTITY_GET_MUTABLE_COMPONENT( editor_camera_node, crude_free_camera );
  //    crude_player_controller *player_controller = CRUDE_ENTITY_GET_MUTABLE_COMPONENT( game_controller_node, crude_player_controller );
  //    
  //    game_set_focused_camera_node( game_instance( ), game_camera_node );
  //    player_controller->input_enabled = true;
  //    free_camera->enabled = false;
  //  }
  //}
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

CRUDE_PARSE_JSON_TO_COMPONENT_FUNC_DECLARATION( crude_player )
{
  crude_memory_set( component, 0, sizeof( crude_player ) );
  return true;
}

CRUDE_PARSE_COMPONENT_TO_JSON_FUNC_DECLARATION( crude_player )
{
  cJSON *json = cJSON_CreateObject( );
  cJSON_AddItemToObject( json, "type", cJSON_CreateString( CRUDE_COMPONENT_STRING( crude_player ) ) );
  return json;
}

CRUDE_PARSE_COMPONENT_TO_IMGUI_FUNC_DECLARATION( crude_player )
{
}

CRUDE_PARSE_JSON_TO_COMPONENT_FUNC_DECLARATION( crude_recycle_station )
{
  crude_memory_set( component, 0, sizeof( crude_recycle_station ) );
  component->game_item = CRUDE_CAST( crude_game_item, cJSON_GetNumberValue( cJSON_GetObjectItemCaseSensitive( component_json, "game_item" ) ) );
  return true;
}

CRUDE_PARSE_COMPONENT_TO_JSON_FUNC_DECLARATION( crude_recycle_station )
{
  cJSON *json = cJSON_CreateObject( );
  cJSON_AddItemToObject( json, "type", cJSON_CreateString( CRUDE_COMPONENT_STRING( crude_recycle_station ) ) );
  cJSON_AddItemToObject( json, "game_item", cJSON_CreateNumber( component->game_item  ) );
  return json;
}

CRUDE_PARSE_COMPONENT_TO_IMGUI_FUNC_DECLARATION( crude_recycle_station )
{
  const char* items[] = { crude_game_item_to_string( CRUDE_GAME_ITEM_NONE ), crude_game_item_to_string( CRUDE_GAME_ITEM_SERUM ), crude_game_item_to_string( CRUDE_GAME_ITEM_SYRINGE_HEALTH ), crude_game_item_to_string( CRUDE_GAME_ITEM_SYRINGE_DRUG ), crude_game_item_to_string( CRUDE_GAME_ITEM_AMMUNITION ) };
  int32 item_current = component->game_item;
  ImGui::Combo( "item", &item_current, items, IM_ARRAYSIZE( items ) );
  component->game_item = CRUDE_CAST( crude_game_item, item_current );
}