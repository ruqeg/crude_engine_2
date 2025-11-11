#include <imgui/imgui.h>
#include <physics/physics_system.h>

#include <physics/physics_components.h>

ECS_COMPONENT_DECLARE( crude_physics_static_body );
ECS_COMPONENT_DECLARE( crude_physics_dynamic_body );
ECS_COMPONENT_DECLARE( crude_physics_body_handle );

CRUDE_COMPONENT_STRING_DEFINE( crude_physics_static_body, "crude_physics_static_body" );
CRUDE_COMPONENT_STRING_DEFINE( crude_physics_dynamic_body, "crude_physics_dynamic_body" );

CRUDE_ECS_MODULE_IMPORT_IMPL( crude_physics_components )
{
  ECS_MODULE( world, crude_physics_components );
  ECS_COMPONENT_DEFINE( world, crude_physics_static_body );
  ECS_COMPONENT_DEFINE( world, crude_physics_dynamic_body );
  ECS_COMPONENT_DEFINE( world, crude_physics_body_handle );
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
  cJSON_AddItemToObject( static_body_json, "layers", cJSON_CreateNumber( component->layers ) );
  return static_body_json;
}

CRUDE_PARSE_JSON_TO_COMPONENT_FUNC_DECLARATION( crude_physics_dynamic_body )
{
  crude_memory_set( component, 0, sizeof( crude_physics_dynamic_body ) );
  component->lock_rotation = cJSON_GetNumberValue( cJSON_GetObjectItemCaseSensitive( component_json, "lock_rotation" ) );
  return true;
}

CRUDE_PARSE_COMPONENT_TO_JSON_FUNC_DECLARATION( crude_physics_dynamic_body )
{
  cJSON *dynamic_body_json = cJSON_CreateObject( );
  cJSON_AddItemToObject( dynamic_body_json, "type", cJSON_CreateString( CRUDE_COMPONENT_STRING( crude_physics_dynamic_body ) ) );
  cJSON_AddItemToObject( dynamic_body_json, "lock_rotation", cJSON_CreateNumber( component->lock_rotation ) );
  cJSON_AddItemToObject( dynamic_body_json, "layers", cJSON_CreateNumber( component->layers ) );
  return dynamic_body_json;
}

CRUDE_PARSE_COMPONENT_TO_IMGUI_FUNC_DECLARATION( crude_physics_dynamic_body )
{
  bool                                                     layers_updated;
  
  layers_updated = false;

  ImGui::Text( "Type: Dynamic Body" );
  ImGui::Text( component->lock_rotation ? "Rotation Locked" : "Rotation Allowed" );
  
  ImGui::Text( "Layers" ); ImGui::SameLine( );
  layers_updated |= ImGui::CheckboxFlags( "C", &component->layers, CRUDE_PHYSICS_BODY_LAYERS_COLLIDING ); ImGui::SameLine( );
  layers_updated |= ImGui::CheckboxFlags( "1", &component->layers, CRUDE_PHYSICS_BODY_LAYERS_SENSOR_1 ); ImGui::SameLine( );
  layers_updated |= ImGui::CheckboxFlags( "2", &component->layers, CRUDE_PHYSICS_BODY_LAYERS_SENSOR_2 ); ImGui::SameLine( );
  layers_updated |= ImGui::CheckboxFlags( "3", &component->layers, CRUDE_PHYSICS_BODY_LAYERS_SENSOR_3 );
  if ( layers_updated )
  {
    crude_physics_body_set_body_layer( CRUDE_ENTITY_GET_MUTABLE_COMPONENT( node, crude_physics_body_handle ), component->layers );
  }
}

CRUDE_PARSE_COMPONENT_TO_IMGUI_FUNC_DECLARATION( crude_physics_static_body )
{
  bool                                                     layers_updated;
  
  layers_updated = false;

  ImGui::Text( "Type: Static Body" );
  ImGui::Text( "Layers" ); ImGui::SameLine( );
  layers_updated |= ImGui::CheckboxFlags( "C", &component->layers, CRUDE_PHYSICS_BODY_LAYERS_COLLIDING ); ImGui::SameLine( );
  layers_updated |= ImGui::CheckboxFlags( "1", &component->layers, CRUDE_PHYSICS_BODY_LAYERS_SENSOR_1 ); ImGui::SameLine( );
  layers_updated |= ImGui::CheckboxFlags( "2", &component->layers, CRUDE_PHYSICS_BODY_LAYERS_SENSOR_2 ); ImGui::SameLine( );
  layers_updated |= ImGui::CheckboxFlags( "3", &component->layers, CRUDE_PHYSICS_BODY_LAYERS_SENSOR_3 );
  if ( layers_updated )
  {
    crude_physics_body_set_body_layer( CRUDE_ENTITY_GET_MUTABLE_COMPONENT( node, crude_physics_body_handle ), component->layers );
  }
}