#include <imgui/imgui.h>

#include <core/memory.h>
#include <scene/scene_components.h>
#include <scene/scripts_components.h>
#include <player_controller_components.h>
#include <game.h>

#include <level_01_components.h>

ECS_COMPONENT_DECLARE( crude_level_01 );

CRUDE_COMPONENT_STRING_DEFINE( crude_level_01, "crude_level_01" );

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
  crude_entity editor_camera_node = crude_ecs_lookup_entity_from_parent( node, "editor_camera" );
  crude_entity game_controller_node = crude_ecs_lookup_entity_from_parent( node, "player" );
  crude_entity game_camera_node = crude_ecs_lookup_entity_from_parent( node, "player.pivot.camera" );

  if ( ImGui::Checkbox( "Editor Camera Controller", &component->editor_camera_controller_enabled ) )
  {
    if ( component->editor_camera_controller_enabled )
    {
      crude_free_camera *free_camera = CRUDE_ENTITY_GET_MUTABLE_COMPONENT( editor_camera_node, crude_free_camera );
      crude_player_controller *player_controller = CRUDE_ENTITY_GET_MUTABLE_COMPONENT( game_controller_node, crude_player_controller );
      
      game_set_focused_camera_node( game_instance( ), editor_camera_node );
      player_controller->_input_enabled = false;
      free_camera->enabled = true;
    }
    else
    { 
      crude_free_camera *free_camera = CRUDE_ENTITY_GET_MUTABLE_COMPONENT( editor_camera_node, crude_free_camera );
      crude_player_controller *player_controller = CRUDE_ENTITY_GET_MUTABLE_COMPONENT( game_controller_node, crude_player_controller );
      
      game_set_focused_camera_node( game_instance( ), game_camera_node );
      player_controller->_input_enabled = true;
      free_camera->enabled = false;
    }
  }
}

CRUDE_ECS_MODULE_IMPORT_IMPL( crude_level_01_components )
{
  ECS_MODULE( world, crude_level_01_components );
  ECS_COMPONENT_DEFINE( world, crude_level_01 );
}