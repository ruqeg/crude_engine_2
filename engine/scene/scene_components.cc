#include <scene/scene_components.h>

ECS_COMPONENT_DECLARE( crude_transform );
ECS_COMPONENT_DECLARE( crude_camera );
ECS_COMPONENT_DECLARE( crude_scene );
ECS_COMPONENT_DECLARE( crude_scene_creation );
ECS_COMPONENT_DECLARE( crude_scene_handle );
ECS_COMPONENT_DECLARE( crude_gltf );

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

CRUDE_API XMMATRIX
crude_transform_node_to_parent
(
  _In_ crude_transform const                              *transform
)
{ 
  return XMMatrixAffineTransformation( XMLoadFloat3( &transform->scale ), XMVectorZero( ), XMLoadFloat4( &transform->rotation ), XMLoadFloat3( &transform->translation ) );
}

CRUDE_ECS_MODULE_IMPORT_IMPL( crude_scene_components )
{
  ECS_MODULE( world, crude_scene_components );
  ECS_COMPONENT_DEFINE( world, crude_transform );
  ECS_COMPONENT_DEFINE( world, crude_camera );
  ECS_COMPONENT_DEFINE( world, crude_scene );
  ECS_COMPONENT_DEFINE( world, crude_gltf );
  ECS_COMPONENT_DEFINE( world, crude_scene_creation );
  ECS_COMPONENT_DEFINE( world, crude_scene_handle );
}