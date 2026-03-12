#include <engine/graphics/model_renderer_resources_manager.h>

#include <engine/graphics/model_renderer_resources.h>

void
crude_gfx_mesh_cpu_to_mesh_draw_gpu
(
  _In_ crude_gfx_device                                   *gpu,
  _In_ crude_gfx_mesh_cpu const                           *mesh,
  _Out_ crude_gfx_mesh_draw                               *mesh_draw_gpu
)
{
  mesh_draw_gpu->textures.x = mesh->albedo_texture_handle.index;
  mesh_draw_gpu->textures.y = mesh->metallic_roughness_texture_handle.index;
  mesh_draw_gpu->textures.z = mesh->normal_texture_handle.index;
  mesh_draw_gpu->textures.w = mesh->occlusion_texture_handle.index;
  mesh_draw_gpu->albedo_color_factor = mesh->albedo_color_factor;
  mesh_draw_gpu->metallic_roughness_occlusion_factor.x = mesh->metallic_roughness_occlusion_factor.x;
  mesh_draw_gpu->metallic_roughness_occlusion_factor.y = mesh->metallic_roughness_occlusion_factor.y;
  mesh_draw_gpu->metallic_roughness_occlusion_factor.z = mesh->metallic_roughness_occlusion_factor.z;
  mesh_draw_gpu->flags = mesh->flags;
  mesh_draw_gpu->mesh_index = mesh->gpu_mesh_index;
  mesh_draw_gpu->meshletes_count = mesh->meshlets_count;
  mesh_draw_gpu->meshletes_offset = mesh->meshlets_offset;
  mesh_draw_gpu->mesh_indices_count = mesh->indices_count;
  mesh_draw_gpu->position_buffer = mesh->position_hga.gpu_address + mesh->position_offset;
  mesh_draw_gpu->texcoord_buffer = mesh->texcoord_hga.gpu_address + mesh->texcoord_offset;
  mesh_draw_gpu->index_buffer = mesh->index_hga.gpu_address + mesh->index_offset;
  mesh_draw_gpu->normal_buffer = mesh->normal_hga.gpu_address + mesh->normal_offset;
  mesh_draw_gpu->tangent_buffer = mesh->tangent_hga.gpu_address + mesh->tangent_offset;
}

XMMATRIX
crude_gfx_node_to_world
(
  _In_ crude_gfx_node const                               *nodes,
  _In_ crude_transform const                              *transforms,
  _In_ uint64                                              node_index
)
{
  uint64                                                 current_parent_index;
  XMMATRIX                                               node_to_world;
  
  node_to_world = crude_transform_node_to_parent( &transforms[ node_index ] );
  current_parent_index = nodes[ node_index ].parent;
  
  while ( current_parent_index != -1 )
  {
    node_to_world = XMMatrixMultiply( node_to_world, crude_transform_node_to_parent( &transforms[ current_parent_index ] ) );
    current_parent_index = nodes[ current_parent_index ].parent;
  }
  
  return node_to_world;
}

void
crude_gfx_model_renderer_resources_initialize
(
  _In_ crude_gfx_model_renderer_resources                 *model_renderer_resources
)
{
  // !TODO
}

void
crude_gfx_model_renderer_resources_deinitialize
(
  _In_ crude_gfx_model_renderer_resources                 *model_renderer_resources
)
{
  for ( uint32 k = 0; k < CRUDE_ARRAY_LENGTH( model_renderer_resources->nodes ); ++k )
  {
    if ( model_renderer_resources->nodes[ k ].meshes_gpu )
    {
      CRUDE_ARRAY_DEINITIALIZE( model_renderer_resources->nodes[ k ].meshes_gpu );
    }
    if ( model_renderer_resources->nodes[ k ].childrens )
    {
      CRUDE_ARRAY_DEINITIALIZE( model_renderer_resources->nodes[ k ].childrens );
    }
  }
  
  CRUDE_ARRAY_DEINITIALIZE( model_renderer_resources->nodes );
  CRUDE_ARRAY_DEINITIALIZE( model_renderer_resources->default_nodes_transforms );

  for ( uint32 k = 0; k < CRUDE_ARRAY_LENGTH( model_renderer_resources->skins ); ++k )
  {
    CRUDE_ARRAY_DEINITIALIZE( model_renderer_resources->skins[ k ].joints );
    CRUDE_ARRAY_DEINITIALIZE( model_renderer_resources->skins[ k ].inverse_bind_matrices );
  }
  CRUDE_ARRAY_DEINITIALIZE( model_renderer_resources->skins );
  for ( uint32 k = 0; k < CRUDE_ARRAY_LENGTH( model_renderer_resources->animations ); ++k )
  {
    CRUDE_ARRAY_DEINITIALIZE( model_renderer_resources->animations[ k ].channels );
    for ( uint32 j = 0; j < CRUDE_ARRAY_LENGTH( model_renderer_resources->animations[ k ].samplers ); ++j )
    {
      CRUDE_ARRAY_DEINITIALIZE( model_renderer_resources->animations[ k ].samplers[ j ].inputs );
      CRUDE_ARRAY_DEINITIALIZE( model_renderer_resources->animations[ k ].samplers[ j ].outputs );
    }
    CRUDE_ARRAY_DEINITIALIZE( model_renderer_resources->animations[ k ].samplers );
  }
  CRUDE_ARRAY_DEINITIALIZE( model_renderer_resources->animations );
}

crude_gfx_model_renderer_resources_instance
crude_gfx_model_renderer_resources_instance_empty
(
)
{
  crude_gfx_model_renderer_resources_instance              model_renderer_resources_instance;
  model_renderer_resources_instance = CRUDE_COMPOUNT_EMPTY( crude_gfx_model_renderer_resources_instance );
  model_renderer_resources_instance.model_renderer_resources_handle.index = -1;
  model_renderer_resources_instance.animation_instance.animation_index = -1;
  XMStoreFloat4x4( &model_renderer_resources_instance.model_to_world, XMMatrixIdentity( ) );
  return model_renderer_resources_instance;
}

void
crude_gfx_model_renderer_resources_instance_deinitialize
(
  _In_ crude_gfx_model_renderer_resources_instance        *model_renderer_resources_instance
)
{
}

void
crude_gfx_model_renderer_resources_instance_update_animation
(
  _In_ crude_gfx_model_renderer_resources_manager         *manager,
  _Inout_ crude_gfx_model_renderer_resources_instance     *model_renderer_resources_instance,
  _In_ float32                                             delta_time
)
{
  crude_gfx_animation                                     *animation;
  crude_gfx_model_renderer_resources                      *model_renderer_resources;
  crude_gfx_model_renderer_resources_animation_instance   *animation_instance;

  if ( model_renderer_resources_instance->model_renderer_resources_handle.index == -1 )
  {
    return;
  }

  model_renderer_resources = crude_gfx_model_renderer_resources_manager_access_model_renderer_resources( manager, model_renderer_resources_instance->model_renderer_resources_handle );
  animation_instance = &model_renderer_resources_instance->animation_instance;

  if ( animation_instance->animation_index < 0 )
  {
    return;
  }
  
  animation = &model_renderer_resources->animations[ animation_instance->animation_index ];

  animation_instance->current_time += delta_time;
  if ( animation_instance->current_time > animation->end )
  {
    if ( animation_instance->loop )
    {
      animation_instance->current_time -= animation->end;
    }
    else
    {
      // !TODO
      animation_instance->animation_index = -1;
    }
  }
  
  for ( uint32 channel_index = 0u; channel_index < CRUDE_ARRAY_LENGTH( animation->channels ); ++channel_index )
  {
    crude_gfx_animation_channel                           *channel; 
    crude_gfx_animation_sampler                           *sampler;
    
    channel = &animation->channels[ channel_index ];
    sampler = &animation->samplers[ channel->sampler_index ];
  
    if ( sampler->interpolation != CRUDE_GFX_ANIMATION_SAMPLER_INTERPOLATION_TYPE_LINEAR )
    {
      CRUDE_ASSERT( false );
      continue;
    }
  
    for ( uint32 i = 0; i < CRUDE_ARRAY_LENGTH( sampler->inputs ) - 1; ++i )
    {
      if ( ( animation_instance->current_time >= sampler->inputs[ i ] ) && ( animation_instance->current_time <= sampler->inputs[ i + 1 ] ) )
      {
        crude_transform                                 *transform;
        XMVECTOR                                         x, y;
        float32                                          t;
  
        t = ( animation_instance->current_time - sampler->inputs[ i ] ) / ( sampler->inputs[ i + 1 ] - sampler->inputs[ i ] );
  
        transform = &model_renderer_resources_instance->nodes_transforms[ channel->node ];
  
        if ( channel->path == CRUDE_GFX_ANIMATION_CHANNEL_PATH_TRANSLATION )
        {
          x = XMLoadFloat4( &sampler->outputs[ i ] );
          y = XMLoadFloat4( &sampler->outputs[ i + 1 ] );
          XMStoreFloat3( &transform->translation, XMVectorLerp( x, y, t ) );
        }
        else if ( channel->path == CRUDE_GFX_ANIMATION_CHANNEL_PATH_ROTATION )
        {
          x = XMLoadFloat4( &sampler->outputs[ i ] );
          y = XMLoadFloat4( &sampler->outputs[ i + 1 ] );
          XMStoreFloat4( &transform->rotation, XMQuaternionNormalize( XMQuaternionSlerp( x, y, t ) ) );
        }
        else if ( channel->path == CRUDE_GFX_ANIMATION_CHANNEL_PATH_SCALE )
        {
          x = XMLoadFloat4( &sampler->outputs[ i ] );
          y = XMLoadFloat4( &sampler->outputs[ i + 1 ] );
          XMStoreFloat3( &transform->scale, XMVectorLerp( x, y, t ) );
        }
      }
    }
  }
}