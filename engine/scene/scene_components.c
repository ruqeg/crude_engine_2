#include <scene/scene_components.h>

ECS_COMPONENT_DECLARE( crude_transform );
ECS_COMPONENT_DECLARE( crude_camera );

static crude_matrix
get_node_to_parent_matrix
(
  _In_ crude_transform        *transform
)
{
  return crude_mat_affine_transformation( crude_load_float3( &transform->scale ), crude_vec_zero(), crude_load_float4( &transform->rotation ), crude_load_float3( &transform->translation ) );
}

void
crude_scene_componentsImport
(
  ecs_world_t *world
)
{
  ECS_MODULE( world, crude_scene_components );
  ECS_COMPONENT_DEFINE( world, crude_transform );
  ECS_COMPONENT_DEFINE( world, crude_camera );
}

crude_matrix
crude_camera_view_to_clip
(
  _In_ crude_camera           *camera
)
{
  return crude_mat_perspective_fov_lh( camera->fov_radians, camera->aspect_ratio, camera->near_z, camera->far_z );
}

crude_matrix
crude_transform_node_to_world
(
  _In_ crude_entity            node,
  _In_ crude_transform        *transform
)
{
  crude_entity parent = CRUDE_ENTITY_GET_PARENT( node );
  
  if ( CRUDE_ENTITY_VALID( parent ) && CRUDE_ENTITY_HAS_COMPONENT( parent, crude_transform ) )
  {
    crude_transform *parent_transform = CRUDE_ENTITY_GET_MUTABLE_COMPONENT( parent, crude_transform );
    return crude_mat_multiply( get_node_to_parent_matrix( transform ), crude_transform_node_to_world( parent, parent_transform ) );
  }
  return get_node_to_parent_matrix( transform );
}