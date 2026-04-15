#include <engine/scene/scene_ecs.h>
#include <engine/physics/physics.h>

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

crude_ray
crude_ray_empty
(
)
{
  crude_ray ray = CRUDE_COMPOUNT_EMPTY( crude_ray );
  return ray;
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

void
crude_transform_decompose
(
  _Out_ crude_transform                                   *transform,
  _In_ XMMATRIX                                            node_to_parent
)
{
  XMVECTOR                                                 t, s, r;
  
  XMMatrixDecompose( &s, &r, &t, node_to_parent );
  XMStoreFloat3( &transform->translation, t );
  XMStoreFloat3( &transform->scale, s );
  XMStoreFloat4( &transform->rotation, r );
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

bool
crude_ray_cast
(
  _In_ crude_physics                                      *physics,
  _In_ ecs_world_t                                        *world,
  _In_ crude_entity                                        ray_entity,
  _Out_ crude_ray_cast_result                             *ray_cast_result
)
{
  crude_ray const                                         *ray;
  XMMATRIX                                                 ray_to_world;
  XMVECTOR                                                 ray_direction, ray_origin;
  
  ray_to_world = crude_transform_node_to_world( world, ray_entity, NULL );
  ray = CRUDE_ENTITY_GET_IMMUTABLE_COMPONENT( world, ray_entity, crude_ray );

  ray_direction = XMVectorScale( XMVector3TransformNormal( XMVectorSet( 0, 0, 1, 0 ), ray_to_world ), ray->distance );
  ray_origin = ray_to_world.r[ 3 ];

  return crude_physics_ray_cast( physics, world, ray_origin, ray_direction, ray->broad_phase_mask, ray->layer_mask, ray_cast_result );
}

crude_light
crude_light_empty
(
)
{
  crude_light                                              light;

  light = CRUDE_COMPOUNT_EMPTY( crude_light );
  light.color.x = 1;
  light.color.y = 1;
  light.color.z = 1;
  light.intensity = 1;
  light.radius = 1;
  return light;
}