#include <engine/core/assert.h>

#include <engine/scene/scene_ecs.h>
#include <engine/graphics/imgui.h>
#include <engine/physics/physics_ecs.h>
#include <engine/scene/node_manager.h>

/**********************************************************
 *
 *                 Components
 *
 *********************************************************/

ECS_COMPONENT_DECLARE( crude_transform );
ECS_COMPONENT_DECLARE( crude_light );
ECS_COMPONENT_DECLARE( crude_camera );
ECS_COMPONENT_DECLARE( crude_gltf );
ECS_COMPONENT_DECLARE( crude_node_external );
ECS_COMPONENT_DECLARE( crude_node_runtime );

CRUDE_COMPONENT_STRING_DEFINE( crude_camera, "crude_camera" );
CRUDE_COMPONENT_STRING_DEFINE( crude_transform, "crude_transform" );
CRUDE_COMPONENT_STRING_DEFINE( crude_gltf, "crude_gltf" );
CRUDE_COMPONENT_STRING_DEFINE( crude_light, "crude_light" );
CRUDE_COMPONENT_STRING_DEFINE( crude_node_runtime, "crude_node_runtime" );
CRUDE_COMPONENT_STRING_DEFINE( crude_node_external, "crude_node_external" );

void
crude_scene_components_import
(
  _In_ crude_ecs                                          *world,
  _In_ crude_components_serialization_manager             *manager
)
{
  CRUDE_ECS_MODULE( world, crude_scene_components );
  CRUDE_ECS_COMPONENT_DEFINE( world, crude_transform );
  CRUDE_ECS_COMPONENT_DEFINE( world, crude_light );
  CRUDE_ECS_COMPONENT_DEFINE( world, crude_camera );
  CRUDE_ECS_COMPONENT_DEFINE( world, crude_gltf );
  CRUDE_ECS_COMPONENT_DEFINE( world, crude_node_external );
  CRUDE_ECS_COMPONENT_DEFINE( world, crude_node_runtime );
  CRUDE_PARSE_COMPONENT_TO_IMGUI_FUNC_DEFINE( manager, crude_transform );
  CRUDE_PARSE_COMPONENT_TO_IMGUI_FUNC_DEFINE( manager, crude_light );
  CRUDE_PARSE_COMPONENT_TO_IMGUI_FUNC_DEFINE( manager, crude_camera );
  CRUDE_PARSE_COMPONENT_TO_IMGUI_FUNC_DEFINE( manager, crude_gltf );
  CRUDE_PARSE_COMPONENT_TO_IMGUI_FUNC_DEFINE( manager, crude_node_external );
  CRUDE_PARSE_COMPONENT_TO_IMGUI_FUNC_DEFINE( manager, crude_node_runtime );
}

CRUDE_PARSE_JSON_TO_COMPONENT_FUNC_DECLARATION( crude_camera )
{
  crude_memory_set( component, 0, sizeof( crude_camera ) );
  component->fov_radians = CRUDE_STATIC_CAST( float32, cJSON_GetNumberValue( cJSON_GetObjectItemCaseSensitive( component_json, "fov_radians" ) ) );
  component->near_z = CRUDE_STATIC_CAST( float32, cJSON_GetNumberValue( cJSON_GetObjectItemCaseSensitive( component_json, "near_z" ) ) );
  component->far_z = CRUDE_STATIC_CAST( float32, cJSON_GetNumberValue( cJSON_GetObjectItemCaseSensitive( component_json, "far_z" ) ) );
  component->aspect_ratio = CRUDE_STATIC_CAST( float32, cJSON_GetNumberValue( cJSON_GetObjectItemCaseSensitive( component_json, "aspect_ratio" ) ) );
  return true;
}

CRUDE_PARSE_COMPONENT_TO_JSON_FUNC_DECLARATION( crude_camera )
{
  cJSON *camera_json = cJSON_CreateObject( );
  cJSON_AddItemToObject( camera_json, "type", cJSON_CreateString( CRUDE_COMPONENT_STRING( crude_camera ) ) );
  cJSON_AddItemToObject( camera_json, "fov_radians", cJSON_CreateNumber( component->fov_radians ) );
  cJSON_AddItemToObject( camera_json, "near_z", cJSON_CreateNumber( component->near_z ) );
  cJSON_AddItemToObject( camera_json, "far_z", cJSON_CreateNumber( component->far_z ) );
  cJSON_AddItemToObject( camera_json, "aspect_ratio", cJSON_CreateNumber( component->aspect_ratio ) );
  return camera_json;
}

CRUDE_PARSE_COMPONENT_TO_IMGUI_FUNC_IMPLEMENTATION( crude_camera )
{
}

CRUDE_PARSE_JSON_TO_COMPONENT_FUNC_DECLARATION( crude_transform )
{
  crude_memory_set( component, 0, sizeof( crude_camera ) );
  crude_parse_json_to_float3( &component->translation, cJSON_GetObjectItemCaseSensitive( component_json, "translation" ) );
  crude_parse_json_to_float4( &component->rotation, cJSON_GetObjectItemCaseSensitive( component_json, "rotation" ) );
  crude_parse_json_to_float3( &component->scale, cJSON_GetObjectItemCaseSensitive( component_json, "scale" ) );
  return true;
}
CRUDE_PARSE_COMPONENT_TO_JSON_FUNC_DECLARATION( crude_transform )
{
  cJSON *transform_json = cJSON_CreateObject( );
  cJSON_AddItemToObject( transform_json, "translation", cJSON_CreateFloatArray( &component->translation.x, 3 ) );
  cJSON_AddItemToObject( transform_json, "rotation", cJSON_CreateFloatArray( &component->rotation.x, 4 ) );
  cJSON_AddItemToObject( transform_json, "scale", cJSON_CreateFloatArray( &component->scale.x, 3 ) );
  return transform_json;
}

CRUDE_PARSE_COMPONENT_TO_IMGUI_FUNC_IMPLEMENTATION( crude_transform )
{
  XMFLOAT3                                               pitch_yaw_roll;

  CRUDE_IMGUI_START_OPTIONS;
  CRUDE_IMGUI_OPTION( "Translation", {
    ImGui::DragFloat3( "##Translation", &component->translation.x, 1.f, 0.f, 0.f, "%.3f", ImGuiSliderFlags_ColorMarkers );
  } );
  
  CRUDE_IMGUI_OPTION( "Quaternion Rotation", {
    XMStoreFloat3( &pitch_yaw_roll, crude_quaternion_to_pitch_yaw_roll( XMLoadFloat4( &component->rotation ) ) );
    ImGui::DragFloat4( "##Quaternion Rotation", &component->rotation.x, 0.05f, 0.f, 0.f, "%.3f", ImGuiSliderFlags_ColorMarkers );
  } );

  CRUDE_IMGUI_OPTION( "Scale", {
    ImGui::DragFloat3( "##Scale", &component->scale.x, 1.f, 0.f, 0.f, "%.3f", ImGuiSliderFlags_ColorMarkers );
  } );
}

CRUDE_PARSE_JSON_TO_COMPONENT_FUNC_DECLARATION( crude_gltf )
{
  char const                                              *gltf_relative_filepath;

  gltf_relative_filepath = cJSON_GetStringValue( cJSON_GetObjectItemCaseSensitive( component_json, "path" ) );

  crude_memory_set( component, 0, sizeof( crude_gltf ) );
  crude_string_copy( component->relative_filepath, gltf_relative_filepath, sizeof( component->relative_filepath ) );
  component->hidden = cJSON_HasObjectItem( component_json, "hidden" ) ? cJSON_GetNumberValue( cJSON_GetObjectItemCaseSensitive( component_json, "hidden" ) ) : false;
  return true;
}

CRUDE_PARSE_COMPONENT_TO_JSON_FUNC_DECLARATION( crude_gltf )
{
  cJSON *gltf_json = cJSON_CreateObject( );     
  cJSON_AddItemToObject( gltf_json, "type", cJSON_CreateString( CRUDE_COMPONENT_STRING( crude_gltf ) ) );
  cJSON_AddItemToObject( gltf_json, "path", cJSON_CreateString( component->relative_filepath ) );
  if ( component->hidden )
  {
    cJSON_AddItemToObject( gltf_json, "hidden", cJSON_CreateBool( component->hidden ) );
  }
  return gltf_json;
}

CRUDE_PARSE_COMPONENT_TO_IMGUI_FUNC_IMPLEMENTATION( crude_gltf )
{
  CRUDE_IMGUI_START_OPTIONS;
  CRUDE_IMGUI_OPTION( "Hidden", {
    ImGui::Checkbox( "##Hidden", &component->hidden );
  } );

  CRUDE_IMGUI_OPTION( "Relative Filepath", {
    ImGui::Text( "\"%s\"", component->relative_filepath );
    if ( ImGui::BeginDragDropTarget( ) )
    {
      ImGuiPayload const                                  *im_payload;
      char                                                *replace_relative_filepath;

      im_payload = ImGui::AcceptDragDropPayload( "crude_content_browser_file" );
      if ( im_payload )
      {
        replace_relative_filepath = CRUDE_CAST( char*, im_payload->Data );
        if ( strstr( replace_relative_filepath, ".gltf" ) )
        {
          crude_string_copy( component->relative_filepath, replace_relative_filepath, sizeof( component->relative_filepath ) );
        }
      }
      ImGui::EndDragDropTarget();
    }
  } );
}

CRUDE_PARSE_JSON_TO_COMPONENT_FUNC_DECLARATION( crude_light )
{
  crude_memory_set( component, 0, sizeof( crude_light ) );
  component->radius = cJSON_GetNumberValue( cJSON_GetObjectItemCaseSensitive( component_json, "radius" ) );
  crude_parse_json_to_float3( &component->color, cJSON_GetObjectItemCaseSensitive( component_json, "color" ) );
  component->intensity = cJSON_GetNumberValue( cJSON_GetObjectItemCaseSensitive( component_json, "intensity" ) );
  return true;
}

CRUDE_PARSE_COMPONENT_TO_JSON_FUNC_DECLARATION( crude_light )
{
  cJSON *light_json = cJSON_CreateObject( );
  cJSON_AddItemToObject( light_json, "type", cJSON_CreateString( CRUDE_COMPONENT_STRING( crude_light ) ) );
  cJSON_AddItemToObject( light_json, "radius", cJSON_CreateNumber( component->radius ) );
  cJSON_AddItemToObject( light_json, "color", cJSON_CreateFloatArray( &component->color.x, 3 ) );
  cJSON_AddItemToObject( light_json, "intensity", cJSON_CreateNumber( component->intensity ) );
  return light_json;
}

CRUDE_PARSE_COMPONENT_TO_IMGUI_FUNC_IMPLEMENTATION( crude_light )
{
}

CRUDE_PARSE_COMPONENT_TO_IMGUI_FUNC_IMPLEMENTATION( crude_node_runtime )
{
}

CRUDE_PARSE_COMPONENT_TO_IMGUI_FUNC_IMPLEMENTATION( crude_node_external )
{
}

XMMATRIX
crude_camera_view_to_clip
(
  _In_ crude_camera const                                 *camera
)
{
  return XMMatrixPerspectiveFovRH( camera->fov_radians, camera->aspect_ratio, camera->near_z, camera->far_z );
}

crude_transform
crude_transform_empty
(
)
{
  crude_transform transform = CRUDE_COMPOUNT_EMPTY( crude_transform );
  XMStoreFloat3( &transform.translation, XMVectorZero( ) );
  XMStoreFloat4( &transform.rotation, XMQuaternionIdentity( ) );
  XMStoreFloat3( &transform.scale, XMVectorSplatOne( ) );
  return transform;
}

XMMATRIX
crude_transform_node_to_world
(
  _In_ crude_ecs                                          *world,
  _In_ crude_entity                                        node,
  _In_opt_ crude_transform const                          *transform
)
{
  if ( transform == NULL )
  {
    transform = CRUDE_ENTITY_GET_IMMUTABLE_COMPONENT( world, node, crude_transform );
  }

  crude_entity parent = crude_entity_get_parent( world, node );
  
  if ( crude_entity_valid( world, parent ) && CRUDE_ENTITY_HAS_COMPONENT( world, parent, crude_transform ) )
  {
    crude_transform *parent_transform = CRUDE_ENTITY_GET_MUTABLE_COMPONENT( world, parent, crude_transform );
    return XMMatrixMultiply( crude_transform_node_to_parent( transform ), crude_transform_node_to_world( world, parent, parent_transform ) );
  }
  return crude_transform_node_to_parent( transform );
}

XMMATRIX
crude_transform_node_to_parent
(
  _In_ crude_transform const                              *transform
)
{ 
  return XMMatrixAffineTransformation( XMLoadFloat3( &transform->scale ), XMVectorZero( ), XMLoadFloat4( &transform->rotation ), XMLoadFloat3( &transform->translation ) );
}

XMMATRIX
crude_transform_parent_to_world
(
  _In_ crude_ecs                                          *world,
  _In_ crude_entity                                        node
)
{
  crude_entity parent = crude_entity_get_parent( world, node );
  
  if ( crude_entity_valid( world, parent ) && CRUDE_ENTITY_HAS_COMPONENT( world, parent, crude_transform ) )
  {
    crude_transform *parent_transform = CRUDE_ENTITY_GET_MUTABLE_COMPONENT( world, parent, crude_transform );
    return crude_transform_node_to_world( world, parent, parent_transform );
  }

  return XMMatrixIdentity( ) ;
}