#include <engine/core/assert.h>
#include <imgui/imgui.h>

#include <engine/scene/scene_components.h>
#include <engine/physics/physics_components.h>
#include <engine/scene/node_manager.h>

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

CRUDE_ECS_MODULE_IMPORT_IMPL( crude_scene_components )
{
  ECS_MODULE( world, crude_scene_components );
  ECS_COMPONENT_DEFINE( world, crude_transform );
  ECS_COMPONENT_DEFINE( world, crude_light );
  ECS_COMPONENT_DEFINE( world, crude_camera );
  ECS_COMPONENT_DEFINE( world, crude_gltf );
  ECS_COMPONENT_DEFINE( world, crude_node_external );
  ECS_COMPONENT_DEFINE( world, crude_node_runtime );
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

CRUDE_PARSE_JSON_TO_COMPONENT_FUNC_DECLARATION( crude_gltf )
{
  char const                                              *gltf_path;

  gltf_path = cJSON_GetStringValue( cJSON_GetObjectItemCaseSensitive( component_json, "path" ) );

  crude_memory_set( component, 0, sizeof( crude_gltf ) );
  component->original_path = crude_string_buffer_append_use_f( &manager->string_bufffer, "%s", gltf_path );
  component->path = crude_string_buffer_append_use_f( &manager->string_bufffer, "%s%s", manager->resources_absolute_directory, gltf_path );
  component->hidden = cJSON_HasObjectItem( component_json, "hidden" ) ? cJSON_GetNumberValue( cJSON_GetObjectItemCaseSensitive( component_json, "hidden" ) ) : false;
  return true;
}

CRUDE_PARSE_COMPONENT_TO_JSON_FUNC_DECLARATION( crude_gltf )
{
  cJSON *gltf_json = cJSON_CreateObject( );     
  cJSON_AddItemToObject( gltf_json, "type", cJSON_CreateString( CRUDE_COMPONENT_STRING( crude_gltf ) ) );
  cJSON_AddItemToObject( gltf_json, "path", cJSON_CreateString( component->original_path ) );
  if ( component->hidden )
  {
    cJSON_AddItemToObject( gltf_json, "hidden", cJSON_CreateBool( component->hidden ) );
  }
  return gltf_json;
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

CRUDE_PARSE_COMPONENT_TO_IMGUI_FUNC_DECLARATION( crude_node_runtime )
{
}

XMMATRIX
crude_camera_view_to_clip
(
  _In_ crude_camera const                                 *camera
)
{
  return XMMatrixPerspectiveFovLH( camera->fov_radians, camera->aspect_ratio, camera->near_z, camera->far_z );
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
  _In_ crude_entity                                        node,
  _In_opt_ crude_transform const                          *transform
)
{
  if ( transform == NULL )
  {
    transform = CRUDE_ENTITY_GET_IMMUTABLE_COMPONENT( node, crude_transform );
  }

  crude_entity parent = crude_entity_get_parent( node );
  
  if ( crude_entity_valid( parent ) && CRUDE_ENTITY_HAS_COMPONENT( parent, crude_transform ) )
  {
    crude_transform *parent_transform = CRUDE_ENTITY_GET_MUTABLE_COMPONENT( parent, crude_transform );
    return XMMatrixMultiply( crude_transform_node_to_parent( transform ), crude_transform_node_to_world( parent, parent_transform ) );
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
  _In_ crude_entity                                        node
)
{
  crude_entity parent = crude_entity_get_parent( node );
  
  if ( crude_entity_valid( parent ) && CRUDE_ENTITY_HAS_COMPONENT( parent, crude_transform ) )
  {
    crude_transform *parent_transform = CRUDE_ENTITY_GET_MUTABLE_COMPONENT( parent, crude_transform );
    return crude_transform_node_to_world( parent, parent_transform );
  }

  return XMMatrixIdentity( ) ;
}