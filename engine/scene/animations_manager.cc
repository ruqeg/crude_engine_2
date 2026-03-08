#include <thirdparty/cgltf/cgltf.h>

#include <engine/core/array.h>
#include <engine/core/assert.h>
#include <engine/core/string.h>
#include <engine/scene/scene_ecs.h>
#include <engine/graphics/model_renderer_resources_manager.h>

#include <engine/scene/animations_manager.h>

void
crude_animations_manager_initialize
(
  _In_ crude_animations_manager                           *manager
)
{
}

void
crude_animations_manager_deinitialize
(
  _In_ crude_animations_manager                           *manager
)
{
}

void
crude_animations_manager_update
(
  _In_ crude_animations_manager                           *manager,
  _In_ crude_ecs                                          *world,
  _In_ float32                                             delta_time
)
{
  for ( uint32 i = 0; i < CRUDE_ARRAY_LENGTH( manager->animations ); ++i )
  {
    crude_animation                                       *animation;

    animation = &manager->animations[ i ];

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
      crude_animation_channel                             *channel; 
      crude_animation_sampler                             *sampler;
      
      channel = &animation->channels[ channel_index ];
      sampler = &animation->samplers[ channel->sampler_index ];

      if ( sampler->interpolation != CRUDE_ANIMATION_SAMPLER_INTERPOLATION_TYPE_LINEAR )
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

          transform = &manager->model_renderer_resources_manager->nodes[ channel->node ].transform;;

          if ( channel->path == CRUDE_ANIMATION_CHANNEL_PATH_TRANSLATION )
          {
            x = XMLoadFloat4( &sampler->outputs[ i ] );
            y = XMLoadFloat4( &sampler->outputs[ i + 1 ] );
            XMStoreFloat3( &transform->translation, XMVectorLerp( x, y, t ) );
          }
          if ( channel->path == CRUDE_ANIMATION_CHANNEL_PATH_ROTATION )
          {
            x = XMLoadFloat4( &sampler->outputs[ i ] );
            y = XMLoadFloat4( &sampler->outputs[ i + 1 ] );
            XMStoreFloat4( &transform->rotation, XMQuaternionNormalize( XMQuaternionSlerp( x, y, t ) ) );
          }
          if ( channel->path == CRUDE_ANIMATION_CHANNEL_PATH_SCALE )
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

void
crude_animations_manager_add_animation_from_cgltf
(
  _In_ crude_animations_manager                           *manager,
  _In_ void                                               *gltf_raw,
  _In_ void                                               *gltf_animation_raw,
  _In_ uint64                                              nodes_offset
)
{
  cgltf_data                                            *gltf;
  cgltf_animation                                       *gltf_animation;
  crude_animation                                        new_animation;
    
  gltf = CRUDE_CAST( cgltf_data*, gltf_raw );
  gltf_animation = CRUDE_CAST( cgltf_animation*, gltf_animation_raw );
  crude_string_copy( new_animation.name, gltf_animation->name, sizeof( new_animation.name ) );

  CRUDE_ARRAY_INITIALIZE_WITH_LENGTH( new_animation.samplers, gltf_animation->samplers_count, crude_heap_allocator_pack( manager->allocator ) );

  for ( uint64 sampler_index = 0; sampler_index < gltf_animation->samplers_count; ++sampler_index )
  {
    cgltf_animation_sampler                               *gltf_sampler;
    crude_animation_sampler                               *sampler;
    uint8                                                 *inputs_data;
    uint8                                                 *outputs_data;

    gltf_sampler = &gltf_animation->samplers[ sampler_index ];
    sampler = &new_animation.samplers[ sampler_index ];
  
    if ( gltf_sampler->interpolation == cgltf_interpolation_type_linear )
    {
      sampler->interpolation == CRUDE_ANIMATION_SAMPLER_INTERPOLATION_TYPE_LINEAR; 
    }
    else
    {
      CRUDE_ASSERT( false );
    }
  
    inputs_data = CRUDE_CAST( uint8*, gltf_sampler->input->buffer_view->buffer->data ) + gltf_sampler->input->buffer_view->offset;
    CRUDE_ARRAY_INITIALIZE_WITH_LENGTH( sampler->inputs, gltf_sampler->input->count, crude_heap_allocator_pack( manager->allocator ) );

    for ( uint64 i = 0; i < gltf_sampler->input->count; ++i )
    {
      sampler->inputs[ i ] = *CRUDE_CAST( float32*, inputs_data );
      inputs_data += gltf_sampler->input->stride;
    }
  
    for ( uint64 i = 0; i < gltf_sampler->input->count; ++i )
    {
      if ( sampler->inputs[ i ] < new_animation.start )
      {
        new_animation.start = sampler->inputs[ i ];
      }
      if ( sampler->inputs[ i ] > new_animation.end )
      {
        new_animation.end = sampler->inputs[ i ];
      }
    }
  
    CRUDE_ARRAY_INITIALIZE_WITH_LENGTH( sampler->outputs, gltf_sampler->output->count, crude_heap_allocator_pack( manager->allocator ) );
    outputs_data = CRUDE_CAST( uint8*, gltf_sampler->output->buffer_view->buffer->data ) + gltf_sampler->output->buffer_view->offset;
    
    for ( uint32 i = 0; i < gltf_sampler->output->count; ++i )
    {
      if ( gltf_sampler->output->type == cgltf_type_vec3 )
      {
        XMFLOAT3 *data = CRUDE_CAST( XMFLOAT3*, gltf_sampler->output );
        sampler->outputs[ i ].x = data->x;
        sampler->outputs[ i ].y = data->y;
        sampler->outputs[ i ].z = data->z;
        sampler->outputs[ i ].w = 0;
      }
      else if ( gltf_sampler->output->type == cgltf_type_vec3 )
      {
        sampler->outputs[ i ] = *CRUDE_CAST( XMFLOAT4*, gltf_sampler->output );
      }
      else
      {
        CRUDE_ASSERT( false );
      }
      outputs_data += gltf_sampler->output->stride;
    }
  }

  CRUDE_ARRAY_INITIALIZE_WITH_LENGTH( new_animation.channels, gltf_animation->channels_count, crude_heap_allocator_pack( manager->allocator ) );

  for ( uint64 i = 0u; i < gltf_animation->channels_count; ++i )
  {
    switch ( gltf_animation->channels[ i ].target_path )
    {
    case cgltf_animation_path_type_rotation:
    {
      new_animation.channels[ i ].path = CRUDE_ANIMATION_CHANNEL_PATH_ROTATION;
      break;
    }
    case cgltf_animation_path_type_scale:
    {
      new_animation.channels[ i ].path = CRUDE_ANIMATION_CHANNEL_PATH_SCALE;
      break;
    }
    case cgltf_animation_path_type_translation:
    {
      new_animation.channels[ i ].path = CRUDE_ANIMATION_CHANNEL_PATH_TRANSLATION;
      break;
    }
    }
    
    new_animation.channels[ i ].sampler_index = gltf_animation->channels[ i ].sampler - gltf_animation->samplers;
    
    new_animation.channels[ i ].node = nodes_offset + cgltf_node_index( gltf, gltf_animation->channels[ i ].target_node );
  }
}