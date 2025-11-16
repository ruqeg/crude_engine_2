#include <imgui/imgui.h>

#include <core/string.h>
#include <core/memory.h>
#include <core/assert.h>
#include <physics/physics.h>
#include <scene/scene.h>

#include <physics/physics_components.h>

ECS_COMPONENT_DECLARE( crude_physics_static_body_handle );
ECS_COMPONENT_DECLARE( crude_physics_character_body_handle );
ECS_COMPONENT_DECLARE( crude_physics_collision_shape );

CRUDE_COMPONENT_STRING_DEFINE( crude_physics_static_body_handle, "crude_physics_static_body_handle" );
CRUDE_COMPONENT_STRING_DEFINE( crude_physics_character_body_handle, "crude_physics_character_body_handle" );
CRUDE_COMPONENT_STRING_DEFINE( crude_physics_collision_shape, "crude_physics_collision_shape" );

CRUDE_ECS_MODULE_IMPORT_IMPL( crude_physics_components )
{
  ECS_MODULE( world, crude_physics_components );
  ECS_COMPONENT_DEFINE( world, crude_physics_static_body_handle );
  ECS_COMPONENT_DEFINE( world, crude_physics_character_body_handle );
  ECS_COMPONENT_DEFINE( world, crude_physics_collision_shape );
}

CRUDE_PARSE_JSON_TO_COMPONENT_FUNC_DECLARATION( crude_physics_static_body_handle )
{
  crude_physics_static_body_handle *previous_handle = CRUDE_ENTITY_GET_MUTABLE_COMPONENT( node, crude_physics_static_body_handle );
  if ( previous_handle )
  {
    crude_physics_destroy_static_body( crude_physics_instance( ), *previous_handle );
  }

  *component = crude_physics_create_static_body( crude_physics_instance( ), node );
  crude_physics_static_body *static_body = crude_physics_access_static_body( crude_physics_instance( ), *component );
  static_body->layer = cJSON_GetNumberValue( cJSON_GetObjectItem( component_json, "layer" ) );
  return true;
}

CRUDE_PARSE_COMPONENT_TO_JSON_FUNC_DECLARATION( crude_physics_static_body_handle )
{
  cJSON *static_body_json = cJSON_CreateObject( );

  crude_physics_static_body *static_body = crude_physics_access_static_body( crude_physics_instance( ), *component );

  cJSON_AddItemToObject( static_body_json, "type", cJSON_CreateString( CRUDE_COMPONENT_STRING( crude_physics_static_body_handle ) ) );
  cJSON_AddItemToObject( static_body_json, "layer", cJSON_CreateNumber( static_body->layer ) );
  return static_body_json;
}

CRUDE_PARSE_JSON_TO_COMPONENT_FUNC_DECLARATION( crude_physics_character_body_handle )
{
  crude_physics_character_body_handle *previous_handle = CRUDE_ENTITY_GET_MUTABLE_COMPONENT( node, crude_physics_character_body_handle );
  if ( previous_handle )
  {
    crude_physics_destroy_character_body( crude_physics_instance( ), *previous_handle );
  }
  *component = crude_physics_create_character_body( crude_physics_instance( ), node );
  crude_physics_character_body *character_body = crude_physics_access_character_body( crude_physics_instance( ), *component );
  character_body->mask = cJSON_GetNumberValue( cJSON_GetObjectItem( component_json, "mask" ) );
  return true;
}

CRUDE_PARSE_COMPONENT_TO_JSON_FUNC_DECLARATION( crude_physics_character_body_handle )
{
  cJSON *dynamic_body_json = cJSON_CreateObject( );
  crude_physics_character_body *character_body = crude_physics_access_character_body( crude_physics_instance( ), *component );
  cJSON_AddItemToObject( dynamic_body_json, "type", cJSON_CreateString( CRUDE_COMPONENT_STRING( crude_physics_character_body_handle ) ) );
  cJSON_AddItemToObject( dynamic_body_json, "mask", cJSON_CreateNumber( character_body->mask ) );
  return dynamic_body_json;
}

CRUDE_PARSE_COMPONENT_TO_IMGUI_FUNC_DECLARATION( crude_physics_character_body_handle )
{
  crude_physics_character_body *character_body = crude_physics_access_character_body( crude_physics_instance( ), *component );
  ImGui::Text( "Mask" );
  ImGui::SameLine( );
  ImGui::CheckboxFlags( "0", &character_body->mask, 1 );
  ImGui::SameLine( );
  ImGui::CheckboxFlags( "1", &character_body->mask, 1 << 2 );
  ImGui::SameLine( );
  ImGui::CheckboxFlags( "2", &character_body->mask, 1 << 3 );
  ImGui::SameLine( );
  ImGui::CheckboxFlags( "3", &character_body->mask, 1 << 4 );
  ImGui::SameLine( );
  ImGui::CheckboxFlags( "4", &character_body->mask, 1 << 5 );
}

CRUDE_PARSE_COMPONENT_TO_IMGUI_FUNC_DECLARATION( crude_physics_static_body_handle )
{
  crude_physics_static_body *static_body = crude_physics_access_static_body( crude_physics_instance( ), *component );
  ImGui::Text( "Layer" );
  ImGui::SameLine( );
  ImGui::CheckboxFlags( "0", &static_body->layer, 1 );
  ImGui::SameLine( );
  ImGui::CheckboxFlags( "1", &static_body->layer, 1 << 2 );
  ImGui::SameLine( );
  ImGui::CheckboxFlags( "2", &static_body->layer, 1 << 3 );
  ImGui::SameLine( );
  ImGui::CheckboxFlags( "3", &static_body->layer, 1 << 4 );
  ImGui::SameLine( );
  ImGui::CheckboxFlags( "4", &static_body->layer, 1 << 5 );
  
}

CRUDE_PARSE_JSON_TO_COMPONENT_FUNC_DECLARATION( crude_physics_collision_shape )
{
  crude_memory_set( component, 0, sizeof( crude_physics_collision_shape ) );

  component->type = crude_physics_collision_shape_string_to_type( cJSON_GetStringValue( cJSON_GetObjectItemCaseSensitive( component_json, "shape_type" ) ) );
  if ( component->type == CRUDE_PHYSICS_COLLISION_SHAPE_TYPE_BOX )
  {
    crude_parse_json_to_float3( &component->box.half_extent, cJSON_GetObjectItemCaseSensitive( component_json, "half_extent" ) );
  }
  else if ( component->type == CRUDE_PHYSICS_COLLISION_SHAPE_TYPE_SPHERE )
  {
    component->sphere.radius = cJSON_GetNumberValue( cJSON_GetObjectItemCaseSensitive( component_json, "radius" ) );
  }
  else if ( component->type == CRUDE_PHYSICS_COLLISION_SHAPE_TYPE_MESH )
  {
    component->mesh.model_filename = crude_string_buffer_append_use_f( &scene->string_bufffer, "%s", cJSON_GetStringValue( cJSON_GetObjectItemCaseSensitive( component_json, "path" ) ) );
    component->mesh.octree_handle = crude_collisions_resources_manager_get_octree_handle( crude_collisions_resources_manager_instance( ), crude_string_buffer_append_use_f( &scene->string_bufffer, "%s%s", scene->resources_path, component->mesh.model_filename ) );
  }
  else
  {
    CRUDE_ASSERT( false );
  }
  return true;
}

CRUDE_PARSE_COMPONENT_TO_JSON_FUNC_DECLARATION( crude_physics_collision_shape )
{
  cJSON *collision_shape_json = cJSON_CreateObject( );
  cJSON_AddItemToObject( collision_shape_json, "type", cJSON_CreateString( CRUDE_COMPONENT_STRING( crude_physics_collision_shape ) ) );
  cJSON_AddItemToObject( collision_shape_json, "shape_type", cJSON_CreateString( crude_physics_collision_shape_type_to_string( component->type ) ) );
  if ( component->type == CRUDE_PHYSICS_COLLISION_SHAPE_TYPE_BOX )
  {
    cJSON_AddItemToObject( collision_shape_json, "half_extent", cJSON_CreateFloatArray( &component->box.half_extent.x, 3 ) );
  }
  else if ( component->type == CRUDE_PHYSICS_COLLISION_SHAPE_TYPE_SPHERE )
  {
    cJSON_AddItemToObject( collision_shape_json, "radius", cJSON_CreateNumber( component->sphere.radius ) );
  }
  else if ( component->type == CRUDE_PHYSICS_COLLISION_SHAPE_TYPE_MESH )
  {
    cJSON_AddItemToObject( collision_shape_json, "path", cJSON_CreateString( component->mesh.model_filename ) );
  }
  else
  {
    CRUDE_ASSERT( false );
  }

  return collision_shape_json;
}

CRUDE_PARSE_COMPONENT_TO_IMGUI_FUNC_DECLARATION( crude_physics_collision_shape )
{
  if ( component->type == CRUDE_PHYSICS_COLLISION_SHAPE_TYPE_BOX )
  {
    ImGui::Text( "Type: Box" );
    if ( ImGui::DragFloat3( "Half Extent", &component->box.half_extent.x, 0.01 ) )
    {
      CRUDE_ENTITY_COMPONENT_MODIFIED( node, crude_physics_collision_shape );
    }
  }
  else if ( component->type == CRUDE_PHYSICS_COLLISION_SHAPE_TYPE_SPHERE )
  {
    ImGui::Text( "Type: Sphere" );
    if ( ImGui::DragFloat( "Radius", &component->sphere.radius, 0.01 ) )
    {
      CRUDE_ENTITY_COMPONENT_MODIFIED( node, crude_physics_collision_shape );
    }
  }
}