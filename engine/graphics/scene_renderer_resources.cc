#include <graphics/scene_renderer_resources.h>

bool
crude_gfx_mesh_is_transparent
(
  _In_ crude_gfx_mesh_cpu                                 *mesh
)
{
  return ( mesh->flags & ( CRUDE_GFX_DRAW_FLAGS_ALPHA_MASK | CRUDE_GFX_DRAW_FLAGS_TRANSPARENT_MASK ) ) != 0;
}

void
crude_gfx_camera_to_camera_gpu
(
  _In_ crude_entity                                        camera_node,
  _Out_ crude_gfx_camera_gpu                              *camera_gpu
)
{
  crude_camera const *camera = CRUDE_ENTITY_GET_IMMUTABLE_COMPONENT( camera_node, crude_camera );
  crude_transform const *transform = CRUDE_ENTITY_GET_IMMUTABLE_COMPONENT( camera_node, crude_transform );
  
  XMMATRIX view_to_world = crude_transform_node_to_world( camera_node, transform );
  XMMATRIX world_to_view = XMMatrixInverse( NULL, view_to_world );
  XMMATRIX view_to_clip = crude_camera_view_to_clip( camera );
  XMMATRIX clip_to_view = XMMatrixInverse( NULL, view_to_clip );
  XMMATRIX world_to_clip = XMMatrixMultiply( world_to_view, view_to_clip );
  
  XMStoreFloat4x4A( &camera_gpu->clip_to_view, clip_to_view ); 
  XMStoreFloat4x4A( &camera_gpu->view_to_clip, view_to_clip ); 
  XMStoreFloat4x4A( &camera_gpu->view_to_world, view_to_world );
  XMStoreFloat4x4A( &camera_gpu->world_to_view, world_to_view );
  XMStoreFloat4x4A( &camera_gpu->world_to_clip, world_to_clip );
  
  camera_gpu->position.x = transform->translation.x;
  camera_gpu->position.y = transform->translation.y;
  camera_gpu->position.z = transform->translation.z;

  camera_gpu->znear = camera->near_z;
  camera_gpu->zfar = camera->far_z;
  
  XMMATRIX view_to_clip_transposed = XMMatrixTranspose( view_to_clip );
  XMStoreFloat4A( &camera_gpu->frustum_planes_culling[ 0 ], XMPlaneNormalize( XMVectorAdd( view_to_clip_transposed.r[ 3 ], view_to_clip_transposed.r[ 0 ] ) ) );
  XMStoreFloat4A( &camera_gpu->frustum_planes_culling[ 1 ], XMPlaneNormalize( XMVectorSubtract( view_to_clip_transposed.r[ 3 ], view_to_clip_transposed.r[ 0 ] ) ) );
  XMStoreFloat4A( &camera_gpu->frustum_planes_culling[ 2 ], XMPlaneNormalize( XMVectorAdd( view_to_clip_transposed.r[ 3 ], view_to_clip_transposed.r[ 1 ] ) ) );
  XMStoreFloat4A( &camera_gpu->frustum_planes_culling[ 3 ], XMPlaneNormalize( XMVectorSubtract( view_to_clip_transposed.r[ 3 ], view_to_clip_transposed.r[ 1 ] ) ) );
  XMStoreFloat4A( &camera_gpu->frustum_planes_culling[ 4 ], XMPlaneNormalize( XMVectorAdd( view_to_clip_transposed.r[ 3 ], view_to_clip_transposed.r[ 2 ] ) ) );
  XMStoreFloat4A( &camera_gpu->frustum_planes_culling[ 5 ], XMPlaneNormalize( XMVectorSubtract( view_to_clip_transposed.r[ 3 ], view_to_clip_transposed.r[ 2 ] ) ) );
}

void
crude_gfx_mesh_cpu_to_mesh_material_gpu
(
  _In_ crude_gfx_mesh_cpu const                           *mesh,
  _Out_ crude_gfx_mesh_material_gpu                       *mesh_material_gpu
)
{
  crude_transform const *transform = CRUDE_ENTITY_GET_IMMUTABLE_COMPONENT( mesh->node, crude_transform );
  XMMATRIX model_to_world = crude_transform_node_to_world( mesh->node, transform );
  XMStoreFloat4x4( &mesh_material_gpu->model_to_world, model_to_world ); 
  mesh_material_gpu->textures.x = mesh->albedo_texture_handle.index; /* in case i will be confused in the future, bindless textures bineded by their handles, look at gpu_present... at least at the moment I write this comment */
  mesh_material_gpu->textures.y = mesh->roughness_texture_handle.index;
  mesh_material_gpu->textures.z = mesh->normal_texture_handle.index;
  mesh_material_gpu->textures.w = mesh->occlusion_texture_handle.index;
  mesh_material_gpu->albedo_color_factor = mesh->albedo_color_factor;
  mesh_material_gpu->flags = mesh->flags;
  mesh_material_gpu->mesh_index = mesh->gpu_mesh_index;
  mesh_material_gpu->meshletes_count = mesh->meshlets_count;
  mesh_material_gpu->meshletes_offset = mesh->meshlets_offset;
}