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
  _In_ crude_gfx_model_renderer_resources const           *model,
  _In_ uint64                                              node_index
)
{
  _In_ crude_gfx_node const                               *node;
  uint64                                                   current_parent_index;
  XMMATRIX                                                 node_to_world;
  XMMATRIX                                                 node_to_parent;
  
  node = &model->nodes[ node_index ];
  node_to_world = node_to_parent = crude_transform_node_to_parent( &node->transform );
  current_parent_index = node->parent;

  while ( current_parent_index != -1 )
  {
    crude_gfx_node                                        *current_parent;

    current_parent = &model->nodes[ current_parent_index ];

    node_to_world = XMMatrixMultiply( crude_transform_node_to_parent( &current_parent->transform ), node_to_parent );
    current_parent_index = current_parent->parent;
  }

  return node_to_world;
}

void
crude_gfx_model_renderer_resources_animations_update
(
  _In_ crude_gfx_model_renderer_resources const           *model,
  _In_ crude_ecs                                          *world,
  _In_ float32                                             delta_time
)
{
  for ( uint32 i = 0; i < CRUDE_ARRAY_LENGTH( model->animations ); ++i )
  {
    crude_gfx_animation                                   *animation;

    animation = &model->animations[ i ];

    if ( !animation->active )
    {
      continue;
    }

    animation->current_time += delta_time;
    if ( animation->current_time > animation->end )
    {
      animation->current_time -= animation->end;
    }

    for ( uint32 channel_index = 0u; channel_index < CRUDE_ARRAY_LENGTH( animation->channels ); ++channel_index )
    {
      crude_gfx_animation_channel                         *channel; 
      crude_gfx_animation_sampler                         *sampler;
      
      channel = &animation->channels[ channel_index ];
      sampler = &animation->samplers[ channel->sampler_index ];

      if ( sampler->interpolation != CRUDE_GFX_ANIMATION_SAMPLER_INTERPOLATION_TYPE_LINEAR )
      {
        CRUDE_ASSERT( false );
        continue;
      }

      for ( uint32 i = 0; i < CRUDE_ARRAY_LENGTH( sampler->inputs ) - 1; ++i )
      {
        if ( ( animation->current_time >= sampler->inputs[ i ] ) && ( animation->current_time <= sampler->inputs[ i + 1 ] ) )
        {
          crude_transform                                 *transform;
          XMVECTOR                                         x, y;
          float32                                          t;

          t = ( animation->current_time - sampler->inputs[ i ] ) / ( sampler->inputs[ i + 1 ] - sampler->inputs[ i ] );

          transform = &model->nodes[ channel->node ].transform;;

          if ( channel->path == CRUDE_GFX_ANIMATION_CHANNEL_PATH_TRANSLATION )
          {
            x = XMLoadFloat4( &sampler->outputs[ i ] );
            y = XMLoadFloat4( &sampler->outputs[ i + 1 ] );
            XMStoreFloat3( &transform->translation, XMVectorLerp( x, y, t ) );
          }
          if ( channel->path == CRUDE_GFX_ANIMATION_CHANNEL_PATH_ROTATION )
          {
            x = XMLoadFloat4( &sampler->outputs[ i ] );
            y = XMLoadFloat4( &sampler->outputs[ i + 1 ] );
            XMStoreFloat4( &transform->rotation, XMQuaternionNormalize( XMQuaternionSlerp( x, y, t ) ) );
          }
          if ( channel->path == CRUDE_GFX_ANIMATION_CHANNEL_PATH_SCALE )
          {
            x = XMLoadFloat4( &sampler->outputs[ i ] );
            y = XMLoadFloat4( &sampler->outputs[ i + 1 ] );
            XMStoreFloat3( &transform->scale, XMVectorLerp( x, y, t ) );
          }
        }
      }
    }
  }
}