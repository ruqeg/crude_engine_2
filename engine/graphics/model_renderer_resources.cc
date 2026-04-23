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
  mesh_draw_gpu->emissive = mesh->emmision;
  mesh_draw_gpu->albedo_color_factor = mesh->albedo_color_factor;
  mesh_draw_gpu->metallic_roughness_occlusion_factor.x = mesh->metallic_roughness_occlusion_factor.x;
  mesh_draw_gpu->metallic_roughness_occlusion_factor.y = mesh->metallic_roughness_occlusion_factor.y;
  mesh_draw_gpu->metallic_roughness_occlusion_factor.z = mesh->metallic_roughness_occlusion_factor.z;
  mesh_draw_gpu->flags = mesh->flags;
  mesh_draw_gpu->mesh_index = mesh->gpu_mesh_global_index;
  mesh_draw_gpu->meshletes_count = mesh->meshlets_count;
  mesh_draw_gpu->meshletes_offset = mesh->meshlets_offset;
  mesh_draw_gpu->index_buffer = mesh->index_hga.gpu_address;
}

XMMATRIX
crude_gfx_node_to_model
(
  _In_ crude_gfx_node const                               *nodes,
  _In_ crude_transform const                              *transforms,
  _In_ uint64                                              node_index
)
{
  uint64                                                 current_parent_index;
  XMMATRIX                                               node_to_model;
  
  node_to_model = crude_transform_node_to_parent( &transforms[ node_index ] );
  current_parent_index = nodes[ node_index ].parent;
  
  while ( current_parent_index != -1 )
  {
    node_to_model = XMMatrixMultiply( node_to_model, crude_transform_node_to_parent( &transforms[ current_parent_index ] ) );
    current_parent_index = nodes[ current_parent_index ].parent;
  }
  
  return node_to_model;
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
  _In_ crude_gfx_device                                   *gpu,
  _In_ crude_gfx_model_renderer_resources                 *model_renderer_resources
)
{
#if CRUDE_GFX_RAY_TRACING_ENABLED
  if ( model_renderer_resources->rtx_affected )
  {
    for ( uint32 i = 0; i < CRUDE_ARRAY_LENGTH( model_renderer_resources->meshes ); ++i )
    {
      crude_gfx_memory_deallocate( gpu, model_renderer_resources->blases_hga[ i ] );
    }
    CRUDE_ARRAY_DEINITIALIZE( model_renderer_resources->blases_hga );
    
    for ( uint32 i = 0; i < CRUDE_ARRAY_LENGTH( model_renderer_resources->meshes ); ++i )
    {
      gpu->vkDestroyAccelerationStructureKHR( gpu->vk_device, model_renderer_resources->vk_blases[ i ], gpu->vk_allocation_callbacks );
    }
    CRUDE_ARRAY_DEINITIALIZE( model_renderer_resources->vk_blases );
  }
#endif /* CRUDE_GFX_RAY_TRACING_ENABLED */

  CRUDE_ARRAY_DEINITIALIZE( model_renderer_resources->meshes );

  for ( uint32 k = 0; k < CRUDE_ARRAY_LENGTH( model_renderer_resources->nodes ); ++k )
  {
    if ( model_renderer_resources->nodes[ k ].affected_joints )
    {
      CRUDE_ARRAY_DEINITIALIZE( model_renderer_resources->nodes[ k ].affected_joints );
    }
    if ( model_renderer_resources->nodes[ k ].affected_joints_local_aabb )
    {
      CRUDE_ARRAY_DEINITIALIZE( model_renderer_resources->nodes[ k ].affected_joints_local_aabb );
    }
    if ( model_renderer_resources->nodes[ k ].meshes )
    {
      CRUDE_ARRAY_DEINITIALIZE( model_renderer_resources->nodes[ k ].meshes );
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

  CRUDE_HASHMAPSTR_DEINITIALIZE( model_renderer_resources->animation_name_to_index );
}

void
crude_gfx_model_renderer_resources_instance_initialize
(
  _In_ crude_gfx_model_renderer_resources_instance        *instance,
  _In_opt_ crude_gfx_model_renderer_resources_manager     *manager,
  _In_opt_ crude_gfx_model_renderer_resources_handle       handle
)
{
  *instance = CRUDE_COMPOUNT_EMPTY( crude_gfx_model_renderer_resources_instance );
  instance->model_renderer_resources_handle.index = manager ? handle.index : -1;
  for ( uint32 i = 0; i < CRUDE_COUNTOF( instance->animations_instances ); ++i )
  {
    instance->animations_instances[ i ].speed = 1.f;
    instance->animations_instances[ i ].loop = true;
    instance->animations_instances[ i ].disabled = true;
    instance->animations_instances[ i ].animation_index = 1;
    crude_memory_set( &instance->animations_instances[ i ].nodes_enabled_bits, 0xffffffffffffffff, sizeof( instance->animations_instances[ i ].nodes_enabled_bits ) );
  }
  instance->cast_shadow = true;
  XMStoreFloat4x4( &instance->model_to_world, XMMatrixIdentity( ) );

  if ( manager )
  {
    crude_gfx_model_renderer_resources                    *model_renderer_resources;

    model_renderer_resources = crude_gfx_model_renderer_resources_manager_access_model_renderer_resources( manager, handle );
    
    CRUDE_ARRAY_INITIALIZE_WITH_LENGTH( instance->nodes_transforms, CRUDE_ARRAY_LENGTH( model_renderer_resources->default_nodes_transforms ) , crude_heap_allocator_pack( manager->allocator ) );
    for ( uint32 i = 0; i < CRUDE_ARRAY_LENGTH( model_renderer_resources->default_nodes_transforms ); ++i )
    {
      instance->nodes_transforms[ i ] = model_renderer_resources->default_nodes_transforms[ i ];
    }
    
    for ( uint32 k = 0; k < CRUDE_COUNTOF( instance->animations_instances ); ++k )
    {
      CRUDE_ARRAY_INITIALIZE_WITH_LENGTH( instance->animations_instances[ k ].nodes_transforms, CRUDE_ARRAY_LENGTH( model_renderer_resources->default_nodes_transforms ), crude_heap_allocator_pack( manager->allocator ) );
      for ( uint32 i = 0; i < CRUDE_ARRAY_LENGTH( model_renderer_resources->default_nodes_transforms ); ++i )
      {
        instance->animations_instances[ k ].nodes_transforms[ i ] = model_renderer_resources->default_nodes_transforms[ i ];
      }
    }
  }
}

void
crude_gfx_model_renderer_resources_instance_deinitialize
(
  _In_ crude_gfx_model_renderer_resources_instance        *instance
)
{
  if ( instance->nodes_transforms )
  {
    CRUDE_ARRAY_DEINITIALIZE( instance->nodes_transforms );
  }
  
  for ( uint32 k = 0; k < CRUDE_COUNTOF( instance->animations_instances ); ++k )
  {
    if ( instance->animations_instances[ k ].nodes_transforms )
    {
      CRUDE_ARRAY_DEINITIALIZE( instance->animations_instances[ k ].nodes_transforms );
    }
  }
}

void
crude_gfx_model_renderer_resources_update_instance_animations
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

  for ( uint32 animation_instance_index = 0; animation_instance_index < CRUDE_COUNTOF( model_renderer_resources_instance->animations_instances ); ++animation_instance_index )
  {
    float32                                                animation_instance_delta_time;

    animation_instance_delta_time = delta_time;
    animation_instance = &model_renderer_resources_instance->animations_instances[ animation_instance_index];

    if ( animation_instance->animation_index == -1 )
    {
      continue;
    }
    
    if ( animation_instance->disabled )
    {
      continue;
    }
    
    animation = &model_renderer_resources->animations[ animation_instance->animation_index ];
    
    animation_instance_delta_time *= animation_instance->speed;
    if ( animation_instance->inverse )
    {
      animation_instance_delta_time *= -1.f;
    }

    if ( !animation_instance->paused )
    {
      animation_instance->current_time += animation_instance_delta_time;

      if ( animation_instance_delta_time < 0 ) 
      {
        if ( animation_instance->current_time < animation->start )
        {
          if ( animation_instance->loop )
          {
            animation_instance->current_time += animation->end;
          }
          else
          {
            // !TODO
            animation_instance->disabled = true;
          }
        }
      }
      else
      {
        if ( animation_instance->current_time > animation->end )
        {
          if ( animation_instance->loop )
          {
            animation_instance->current_time -= animation->end;
          }
          else
          {
            // !TODO
            animation_instance->disabled = true;
          }
        }
      }
    }
    
    for ( uint32 channel_index = 0u; channel_index < CRUDE_ARRAY_LENGTH( animation->channels ); ++channel_index )
    {
      crude_gfx_animation_channel                           *channel; 
      crude_gfx_animation_sampler                           *sampler;
      
      channel = &animation->channels[ channel_index ];
      sampler = &animation->samplers[ channel->sampler_index ];
    
      if ( !crude_gfx_model_renderer_resources_animation_instance_is_enabled_node( animation_instance, channel->node ) )
      {
        continue;
      }

      for ( uint32 i = 0; i < CRUDE_ARRAY_LENGTH( sampler->inputs ) - 1; ++i )
      {
        if ( ( animation_instance->current_time >= sampler->inputs[ i ] ) && ( animation_instance->current_time <= sampler->inputs[ i + 1 ] ) )
        {
          crude_transform                                 *transform;
          XMVECTOR                                         x, y;
          float32                                          t;
    
    
          if ( sampler->interpolation == CRUDE_GFX_ANIMATION_SAMPLER_INTERPOLATION_TYPE_LINEAR )
          {
            t = ( animation_instance->current_time - sampler->inputs[ i ] ) / ( sampler->inputs[ i + 1 ] - sampler->inputs[ i ] );
          }
          else if ( sampler->interpolation == CRUDE_GFX_ANIMATION_SAMPLER_INTERPOLATION_TYPE_STEP )
          {
            t = CRUDE_FLOOR( ( animation_instance->current_time - sampler->inputs[ i ] ) / ( sampler->inputs[ i + 1 ] - sampler->inputs[ i ] ) );
          }
    
          transform = &animation_instance->nodes_transforms[ channel->node ];
    
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
}

uint64
crude_gfx_model_renderer_resources_instance_find_animation_index_by_name
(
  _In_ crude_gfx_model_renderer_resources_instance        *instance,
  _In_ crude_gfx_model_renderer_resources_manager         *manager,
  _In_ char const                                         *name
)
{
  crude_gfx_model_renderer_resources *model_renderer_resources = crude_gfx_model_renderer_resources_manager_access_model_renderer_resources( manager, instance->model_renderer_resources_handle );
  int64 index = CRUDE_HASHMAPSTR_GET_INDEX( model_renderer_resources->animation_name_to_index, name );
  CRUDE_ASSERT( index != -1 );
  return model_renderer_resources->animation_name_to_index[ index ].value;
}

void
crude_gfx_model_renderer_resources_instance_blend_one_animation
(
  _Inout_ crude_gfx_model_renderer_resources_instance     *model_renderer_resources_instance,
  _In_ int64                                               animation_instance_index
)
{
  crude_gfx_animation                                     *animation;
  crude_gfx_model_renderer_resources                      *model_renderer_resources;
  crude_gfx_model_renderer_resources_animation_instance   *animation_instance;
  
  if ( model_renderer_resources_instance->model_renderer_resources_handle.index == -1 )
  {
    return;
  }

  animation_instance = &model_renderer_resources_instance->animations_instances[ animation_instance_index];

  for ( uint32 i = 0u; i < CRUDE_ARRAY_LENGTH( animation_instance->nodes_transforms ); ++i )
  {
    if ( crude_gfx_model_renderer_resources_animation_instance_is_enabled_node( animation_instance, i ) )
    {
      model_renderer_resources_instance->nodes_transforms[ i ] = animation_instance->nodes_transforms[ i ];
    }
  }
}

void
crude_gfx_model_renderer_resources_instance_blend_two_animations
(
  _Inout_ crude_gfx_model_renderer_resources_instance     *model_renderer_resources_instance,
  _In_ uint32                                              from_index,
  _In_ uint32                                              to_index,
  _In_ float32                                             blend_factor
)
{
  for ( uint32 node_index = 0; node_index < CRUDE_ARRAY_LENGTH( model_renderer_resources_instance->nodes_transforms ); ++node_index )
  {
    crude_gfx_model_renderer_resources_animation_instance *to_animation_instance;
    crude_gfx_model_renderer_resources_animation_instance *from_animation_instance;
    bool                                                   to_node_enabled, from_node_enabled;

    to_animation_instance = &model_renderer_resources_instance->animations_instances[ to_index ];
    from_animation_instance = &model_renderer_resources_instance->animations_instances[ from_index ];

    to_node_enabled = crude_gfx_model_renderer_resources_animation_instance_is_enabled_node( to_animation_instance, node_index );
    from_node_enabled = crude_gfx_model_renderer_resources_animation_instance_is_enabled_node( from_animation_instance, node_index );
    
    if ( to_node_enabled && from_node_enabled )
    {
      model_renderer_resources_instance->nodes_transforms[ node_index ] = crude_transform_lerp(
        &from_animation_instance->nodes_transforms[ node_index ],
        &to_animation_instance->nodes_transforms[ node_index ],
        blend_factor );
    }
    else if ( to_node_enabled )
    {
      model_renderer_resources_instance->nodes_transforms[ node_index ] = to_animation_instance->nodes_transforms[ node_index ];
    }
    else if ( from_node_enabled )
    {
      model_renderer_resources_instance->nodes_transforms[ node_index ] = from_animation_instance->nodes_transforms[ node_index ];
    }
  }
}

void
crude_gfx_model_renderer_resources_instance_blend_animations
(
  _Inout_ crude_gfx_model_renderer_resources_instance     *model_renderer_resources_instance,
  _In_ int64                                               animations_indices[ 8 ],
  _In_ float32                                             weights[ 8 ]
)
{
  for ( uint32 node_index = 0; node_index < CRUDE_ARRAY_LENGTH( model_renderer_resources_instance->nodes_transforms ); ++node_index )
  {
    XMVECTOR                                        blended_translation;
    XMVECTOR                                        blended_scale;
    XMVECTOR                                        blended_rotation;
    float32                                         total_weight;
    bool                                            rotation_was_affected;
    
    total_weight = 0.f;
    blended_translation = XMVectorZero( );
    blended_scale = XMVectorZero( );

    for ( uint32 i = 0; i < 8; ++i )
    {
      crude_gfx_model_renderer_resources_animation_instance *animation_instance;

      if ( animations_indices[ i ] == -1 )
      {
        continue;
      }

      animation_instance = &model_renderer_resources_instance->animations_instances[ animations_indices[ i ] ];
      if ( !crude_gfx_model_renderer_resources_animation_instance_is_enabled_node( animation_instance, node_index ) )
      {
        continue;
      }

      total_weight += weights[ i ];
    }

    for ( uint32 i = 0; i < 8; ++i )
    {
      crude_gfx_model_renderer_resources_animation_instance *animation_instance;
      float32                                              normalized_weight;
      if ( animations_indices[ i ] == -1 )
      {
        continue;
      }

      animation_instance = &model_renderer_resources_instance->animations_instances[ animations_indices[ i ] ];
      if ( !crude_gfx_model_renderer_resources_animation_instance_is_enabled_node( animation_instance, node_index ) )
      {
        continue;
      }

      normalized_weight = weights[ i ] / total_weight;
      blended_translation = XMVectorAdd( blended_translation, XMVectorScale( XMLoadFloat3( &animation_instance->nodes_transforms[ node_index ].translation ), normalized_weight ) );
      blended_scale = XMVectorAdd( blended_scale, XMVectorScale( XMLoadFloat3( &animation_instance->nodes_transforms[ node_index ].scale ), normalized_weight ) );
    }

    rotation_was_affected = false;
    for ( uint32 i = 0; i < 8; ++i )
    {
      crude_gfx_model_renderer_resources_animation_instance *animation_instance;
      XMVECTOR                                             animation_instance_rotation;
      float32                                              normalized_weight;

      if ( animations_indices[ i ] == -1 )
      {
        continue;
      }

      animation_instance = &model_renderer_resources_instance->animations_instances[ animations_indices[ i ] ];
      if ( !crude_gfx_model_renderer_resources_animation_instance_is_enabled_node( animation_instance, node_index ) )
      {
        continue;
      }

      normalized_weight = weights[ i ] / total_weight;
      animation_instance_rotation = XMLoadFloat4( &animation_instance->nodes_transforms[ node_index ].rotation );
        
      if ( !rotation_was_affected )
      {
        blended_rotation = XMVectorScale( animation_instance_rotation, normalized_weight );
        rotation_was_affected = true;
      }
      else
      {

        if ( XMVectorGetX( XMVector4Dot( blended_rotation, animation_instance_rotation ) ) < 0 )
        {
          blended_rotation = XMVectorSubtract( blended_rotation, XMVectorScale( animation_instance_rotation, normalized_weight ) );
        }
        else
        {
          blended_rotation = XMVectorAdd( blended_rotation, XMVectorScale( animation_instance_rotation, normalized_weight ) );
        }
      }
    }
      
    blended_rotation = XMQuaternionNormalize( blended_rotation );

    if ( total_weight > 0.00001f )
    {
      XMStoreFloat3( &model_renderer_resources_instance->nodes_transforms[ node_index ].scale, blended_scale );
      XMStoreFloat3( &model_renderer_resources_instance->nodes_transforms[ node_index ].translation, blended_translation );
      XMStoreFloat4( &model_renderer_resources_instance->nodes_transforms[ node_index ].rotation, blended_rotation );
    }
  }
}

bool
crude_gfx_model_renderer_resources_animation_instance_is_enabled_node
(
  _Inout_ crude_gfx_model_renderer_resources_animation_instance *animation_instance,
  _In_ int64                                               node_index
)
{
  return animation_instance->nodes_enabled_bits[ node_index / 32 ] & ( 1 << ( node_index % 32 ) );
}

void
crude_gfx_model_renderer_resources_animation_instance_enable_node
(
  _Inout_ crude_gfx_model_renderer_resources_animation_instance *animation_instance,
  _In_ int64                                               node_index,
  _In_ bool                                                enabled
)
{
  if ( enabled )
  {
    animation_instance->nodes_enabled_bits[ node_index / 32 ] |= 1 << ( node_index % 32 );
  }
  else
  {
    animation_instance->nodes_enabled_bits[ node_index / 32 ] &= ~( 1 << ( node_index % 32 ) );
  }
}

int64
crude_gfx_model_renderer_resources_instance_find_node_by_name
(
  _In_ crude_gfx_model_renderer_resources_manager         *manager,
  _Inout_ crude_gfx_model_renderer_resources_instance     *model_renderer_resources_instance,
  _In_ char const                                         *name
)
{
  crude_gfx_model_renderer_resources                      *model_renderer_resources;
  
  if ( model_renderer_resources_instance->model_renderer_resources_handle.index == -1 )
  {
    return -1;
  }

  model_renderer_resources = crude_gfx_model_renderer_resources_manager_access_model_renderer_resources( manager, model_renderer_resources_instance->model_renderer_resources_handle );
  for ( uint32 i = 0; i < CRUDE_ARRAY_LENGTH( model_renderer_resources->nodes ); ++i )
  {
    if ( crude_string_cmp( model_renderer_resources->nodes[ i ].name, name ) == 0 )
    {
      return i;
    }
  }
  return -1;
}