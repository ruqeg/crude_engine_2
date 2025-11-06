#include <core/assert.h>
#include <imgui/imgui.h>

#include <scene/scene_components.h>

ECS_COMPONENT_DECLARE( crude_transform );
ECS_COMPONENT_DECLARE( crude_light );
ECS_COMPONENT_DECLARE( crude_camera );
ECS_COMPONENT_DECLARE( crude_scene );
ECS_COMPONENT_DECLARE( crude_scene_creation );
ECS_COMPONENT_DECLARE( crude_scene_handle );
ECS_COMPONENT_DECLARE( crude_gltf );
ECS_COMPONENT_DECLARE( crude_collision_shape );

CRUDE_COMPONENT_STRING_DEFINE( crude_camera, "crude_camera" );
CRUDE_COMPONENT_STRING_DEFINE( crude_transform, "crude_transform" );
CRUDE_COMPONENT_STRING_DEFINE( crude_gltf, "crude_gltf" );
CRUDE_COMPONENT_STRING_DEFINE( crude_light, "crude_light" );
CRUDE_COMPONENT_STRING_DEFINE( crude_collision_shape, "crude_collision_shape" );

CRUDE_ECS_MODULE_IMPORT_IMPL( crude_scene_components )
{
  ECS_MODULE( world, crude_scene_components );
  ECS_COMPONENT_DEFINE( world, crude_transform );
  ECS_COMPONENT_DEFINE( world, crude_light );
  ECS_COMPONENT_DEFINE( world, crude_camera );
  ECS_COMPONENT_DEFINE( world, crude_scene );
  ECS_COMPONENT_DEFINE( world, crude_gltf );
  ECS_COMPONENT_DEFINE( world, crude_scene_creation );
  ECS_COMPONENT_DEFINE( world, crude_scene_handle );
  ECS_COMPONENT_DEFINE( world, crude_collision_shape );
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
  CRUDE_PARSE_JSON_TO_COMPONENT( XMFLOAT3 )( &component->translation, cJSON_GetObjectItemCaseSensitive( component_json, "translation" ) );
  CRUDE_PARSE_JSON_TO_COMPONENT( XMFLOAT4 )( &component->rotation, cJSON_GetObjectItemCaseSensitive( component_json, "rotation" ) ),
  CRUDE_PARSE_JSON_TO_COMPONENT( XMFLOAT3 )( &component->scale, cJSON_GetObjectItemCaseSensitive( component_json, "scale" ) );
  return true;
}
CRUDE_PARSE_COMPONENT_TO_JSON_FUNC_DECLARATION( crude_transform )
{
  cJSON *transform_json = cJSON_CreateObject( );
  cJSON_AddItemToObject( transform_json, "translation", CRUDE_PARSE_COMPONENT_TO_JSON( XMFLOAT3 )( &component->translation ) );
  cJSON_AddItemToObject( transform_json, "rotation", CRUDE_PARSE_COMPONENT_TO_JSON( XMFLOAT4 )( &component->rotation ) );
  cJSON_AddItemToObject( transform_json, "scale", CRUDE_PARSE_COMPONENT_TO_JSON( XMFLOAT3 )( &component->scale ) );
  return transform_json;
}

CRUDE_PARSE_JSON_TO_COMPONENT_FUNC_DECLARATION( crude_gltf )
{
  crude_memory_set( component, 0, sizeof( crude_gltf ) );
  component->original_path = cJSON_GetStringValue( cJSON_GetObjectItemCaseSensitive( component_json, "path" ) );
  return true;
}

CRUDE_PARSE_COMPONENT_TO_JSON_FUNC_DECLARATION( crude_gltf )
{
  cJSON *gltf_json = cJSON_CreateObject( );     
  cJSON_AddItemToObject( gltf_json, "type", cJSON_CreateString( CRUDE_COMPONENT_STRING( crude_gltf ) ) );
  cJSON_AddItemToObject( gltf_json, "path", cJSON_CreateString( component->original_path ) );
  return gltf_json;
}

CRUDE_PARSE_JSON_TO_COMPONENT_FUNC_DECLARATION( crude_light )
{
  crude_memory_set( component, 0, sizeof( crude_light ) );
  component->radius = cJSON_GetNumberValue( cJSON_GetObjectItemCaseSensitive( component_json, "radius" ) );
  CRUDE_PARSE_JSON_TO_COMPONENT( XMFLOAT3 )( &component->color, cJSON_GetObjectItemCaseSensitive( component_json, "color" ) ),
  component->intensity = cJSON_GetNumberValue( cJSON_GetObjectItemCaseSensitive( component_json, "intensity" ) );
  return true;
}

CRUDE_PARSE_COMPONENT_TO_JSON_FUNC_DECLARATION( crude_light )
{
  cJSON *light_json = cJSON_CreateObject( );
  cJSON_AddItemToObject( light_json, "type", cJSON_CreateString( CRUDE_COMPONENT_STRING( crude_light ) ) );
  cJSON_AddItemToObject( light_json, "radius", cJSON_CreateNumber( component->radius ) );
  cJSON_AddItemToObject( light_json, "color", CRUDE_PARSE_COMPONENT_TO_JSON( XMFLOAT3 )( &component->color ) );
  cJSON_AddItemToObject( light_json, "intensity", cJSON_CreateNumber( component->intensity ) );
  return light_json;
}

CRUDE_PARSE_JSON_TO_COMPONENT_FUNC_DECLARATION( crude_collision_shape )
{
  crude_memory_set( component, 0, sizeof( crude_collision_shape ) );

  component->type = crude_collision_shape_str_to_type( cJSON_GetStringValue( cJSON_GetObjectItemCaseSensitive( component_json, "shape_type" ) ) );
  if ( component->type == CRUDE_COLLISION_SHAPE_TYPE_BOX )
  {
    CRUDE_PARSE_JSON_TO_COMPONENT( XMFLOAT3 )( &component->box.extent, cJSON_GetObjectItemCaseSensitive( component_json, "extent" ) );
  }
  else if ( component->type == CRUDE_COLLISION_SHAPE_TYPE_SPHERE )
  {
    component->sphere.radius = cJSON_GetNumberValue( cJSON_GetObjectItemCaseSensitive( component_json, "radius" ) );
  }
  else
  {
    CRUDE_ASSERT( false );
  }
  return true;
}

CRUDE_PARSE_COMPONENT_TO_JSON_FUNC_DECLARATION( crude_collision_shape )
{
  cJSON *collision_shape_json = cJSON_CreateObject( );
  cJSON_AddItemToObject( collision_shape_json, "type", cJSON_CreateString( CRUDE_COMPONENT_STRING( crude_collision_shape ) ) );
  cJSON_AddItemToObject( collision_shape_json, "shape_type", cJSON_CreateString( crude_collision_shape_type_to_str( component->type ) ) );
  if ( component->type == CRUDE_COLLISION_SHAPE_TYPE_BOX )
  {
    cJSON_AddItemToObject( collision_shape_json, "extent", CRUDE_PARSE_COMPONENT_TO_JSON( XMFLOAT3 )( &component->box.extent ) );
  }
  else if ( component->type == CRUDE_COLLISION_SHAPE_TYPE_SPHERE )
  {
    cJSON_AddItemToObject( collision_shape_json, "radius", cJSON_CreateNumber( component->sphere.radius ) );
  }
  else
  {
    CRUDE_ASSERT( false );
  }
  return collision_shape_json;
}

CRUDE_PARSE_COMPONENT_TO_IMGUI_FUNC_DECLARATION( crude_collision_shape )
{
  if ( component->type == CRUDE_COLLISION_SHAPE_TYPE_BOX )
  {
    ImGui::Text( "Type: Box" );
    if ( ImGui::DragFloat3( "Extent", &component->box.extent.x, 0.01 ) )
    {
      CRUDE_ENTITY_COMPONENT_MODIFIED( node, crude_collision_shape );
    }
  }
}

XMMATRIX
crude_camera_view_to_clip
(
  _In_ crude_camera const                                 *camera
)
{
  return XMMatrixPerspectiveFovLH( camera->fov_radians, camera->aspect_ratio, camera->near_z, camera->far_z );
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

char const*
crude_collision_shape_type_to_str
(
  _In_ crude_collision_shape_type                          type
)
{
  if ( type == CRUDE_COLLISION_SHAPE_TYPE_BOX )
  {
    return "box";
  }
  else if ( type == CRUDE_COLLISION_SHAPE_TYPE_SPHERE )
  {
    return "sphere";
  }
  else
  {
    CRUDE_ASSERT( false );
  }
}

crude_collision_shape_type
crude_collision_shape_str_to_type
(
  _In_ char const*                                         type_str
)
{
  if ( crude_string_cmp( type_str, "box" ) == 0 )
  {
    return CRUDE_COLLISION_SHAPE_TYPE_BOX;
  }
  else if ( crude_string_cmp( type_str, "sphere" ) == 0 )
  {
    return CRUDE_COLLISION_SHAPE_TYPE_SPHERE;
  }
  else
  {
    CRUDE_ASSERT( false );
  }
}

char const*
crude_collision_shape_to_model_filename
(
  _In_ crude_collision_shape_type                          type
)
{
  if ( type == CRUDE_COLLISION_SHAPE_TYPE_BOX )
  {
    return "editor\\models\\crude_physics_box_collision_shape.gltf";
  }
  else if ( type == CRUDE_COLLISION_SHAPE_TYPE_SPHERE )
  {
    return "editor\\models\\crude_physics_box_collision_shape.gltf";
  }
  else
  {
    CRUDE_ASSERT( false );
  }
}