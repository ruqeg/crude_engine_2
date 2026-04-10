#include <engine/scene/scene_ecs.h>

#include <engine/scene/scene_resources.h>

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

crude_gltf
crude_gltf_empty
(
)
{
  crude_gltf gltf = CRUDE_COMPOUNT_EMPTY( crude_gltf );
  gltf.hidden = false;
  crude_gfx_model_renderer_resources_instance_initialize( &gltf.model_renderer_resources_instance, NULL, CRUDE_COMPOUNT( crude_gfx_model_renderer_resources_handle, { -1 } ) );
  return gltf;
}

crude_camera
crude_camera_empty
(
)
{
  crude_camera camera = CRUDE_COMPOUNT_EMPTY( crude_camera );
  camera.fov_radians = 1;
  camera.aspect_ratio = 1.8;
  camera.near_z = 1;
  camera.far_z = 300;
  return camera;
}

crude_node_external
crude_node_external_empty
(
)
{
  crude_node_external node_external = CRUDE_COMPOUNT_EMPTY( crude_node_external );
  return node_external;
}

XMMATRIX
crude_transform_node_to_world
(
  _In_ crude_ecs                                          *world,
  _In_ crude_entity                                        node,
  _In_opt_ crude_transform const                          *transform
)
{
  crude_transform const                                   *parent_transform;
  XMMATRIX                                                 node_to_world;
  crude_entity                                             parent;

  if ( transform == NULL )
  {
    transform = CRUDE_ENTITY_GET_IMMUTABLE_COMPONENT( world, node, crude_transform );
  }

  node_to_world = crude_transform_node_to_parent( transform );
  parent = crude_entity_get_parent( world, node );

  while ( crude_entity_valid( world, parent ) )
  {
    parent_transform = CRUDE_ENTITY_GET_IMMUTABLE_COMPONENT( world, parent, crude_transform );

    if ( parent_transform )
    {
      node_to_world = XMMatrixMultiply( node_to_world, crude_transform_node_to_parent( parent_transform ) );
    }
    
    node = parent;
    parent = crude_entity_get_parent( world, parent );
  }
  
  return node_to_world;
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

crude_transform
crude_transform_lerp
(
  _In_ crude_transform                                    *transform1,
  _In_ crude_transform                                    *transform2,
  _In_ float32                                             t
)
{
  crude_transform result;
  XMStoreFloat3( &result.translation, XMVectorLerp( XMLoadFloat3( &transform1->translation ), XMLoadFloat3( &transform2->translation ), t ) );
  XMStoreFloat4( &result.rotation, XMQuaternionSlerp( XMLoadFloat4( &transform1->rotation ), XMLoadFloat4( &transform2->rotation ), t ) );
  XMStoreFloat3( &result.scale, XMVectorLerp( XMLoadFloat3( &transform1->scale ), XMLoadFloat3( &transform2->scale ), t ) );
  return result;
}