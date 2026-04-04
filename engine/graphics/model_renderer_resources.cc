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
  mesh_draw_gpu->mesh_indices_count = mesh->indices_count;
  mesh_draw_gpu->position_buffer = mesh->position_hga.gpu_address + mesh->position_offset;
  mesh_draw_gpu->texcoord_buffer = mesh->texcoord_hga.gpu_address + mesh->texcoord_offset;
  mesh_draw_gpu->index_buffer = mesh->index_hga.gpu_address + mesh->index_offset;
  mesh_draw_gpu->normal_buffer = mesh->normal_hga.gpu_address + mesh->normal_offset;
  mesh_draw_gpu->tangent_buffer = mesh->tangent_hga.gpu_address + mesh->tangent_offset;
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
  _In_ crude_gfx_model_renderer_resources                 *model_renderer_resources
)
{
  for ( uint32 i = 0; i < CRUDE_ARRAY_LENGTH( model_renderer_resources->meshes ); ++i )
  {
    if ( model_renderer_resources->meshes[ i ].affected_joints )
    {
      CRUDE_ARRAY_DEINITIALIZE( model_renderer_resources->meshes[ i ].affected_joints );
    }
    if ( model_renderer_resources->meshes[ i ].affected_joints_local_aabb )
    {
      CRUDE_ARRAY_DEINITIALIZE( model_renderer_resources->meshes[ i ].affected_joints_local_aabb );
    }
  }
  CRUDE_ARRAY_DEINITIALIZE( model_renderer_resources->meshes );

  for ( uint32 k = 0; k < CRUDE_ARRAY_LENGTH( model_renderer_resources->nodes ); ++k )
  {
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
    instance->animations_instances[ i ].stop = true;
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
crude_gfx_model_renderer_resources_instance_update_animation
(
  _In_ crude_gfx_model_renderer_resources_manager         *manager,
  _Inout_ crude_gfx_model_renderer_resources_instance     *model_renderer_resources_instance,
  _In_ int64                                               animation_instance_index,
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
  animation_instance = &model_renderer_resources_instance->animations_instances[ animation_instance_index];

  if ( animation_instance->animation_index == -1 )
  {
    return;
  }
  
  if ( animation_instance->stoped )
  {
    return;
  }
  
  animation = &model_renderer_resources->animations[ animation_instance->animation_index ];
  
  delta_time *= animation_instance->speed;
  if ( animation_instance->inverse )
  {
    delta_time *= -1.f;
  }

  if ( !animation_instance->paused )
  {
    animation_instance->current_time += delta_time;

    if ( delta_time < 0 ) 
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
          animation_instance->animation_index = -1;
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
          animation_instance->animation_index = -1;
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
  
    if ( !crude_gfx_model_renderer_resources_animation_instance_is_enabled_node( animation_instance, channel->node % 64 ) )
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

void
crude_gfx_model_renderer_resources_instance_blend_animations
(
  _In_ crude_gfx_model_renderer_resources_manager         *manager,
  _Inout_ crude_gfx_model_renderer_resources_instance     *model_renderer_resources_instance,
  _In_ uint32                                              from_index,
  _In_ uint32                                              to_index,
  _In_ uint32                                              blend_factor,
  _In_ crude_stack_allocator                              *temporary_allocator
)
{
  crude_transform                                         *original_transforms;
  crude_transform                                         *from_transforms;
  uint64                                                   nodes_count, temporary_allocator_marker;

  temporary_allocator_marker = crude_stack_allocator_get_marker( temporary_allocator );

  nodes_count = CRUDE_ARRAY_LENGTH( model_renderer_resources_instance->nodes_transforms );
  CRUDE_ARRAY_INITIALIZE_WITH_CAPACITY( original_transforms, nodes_count, crude_stack_allocator_pack( temporary_allocator ) );

  for ( uint32 i = 0; i < nodes_count; ++i )
  {
    CRUDE_ARRAY_PUSH( original_transforms, model_renderer_resources_instance->nodes_transforms[ i ] );
  }

  crude_gfx_model_renderer_resources_instance_update_animation( manager, model_renderer_resources_instance, from_index, 0.0f );
  
  CRUDE_ARRAY_INITIALIZE_WITH_CAPACITY( from_transforms, nodes_count, crude_stack_allocator_pack( temporary_allocator ) );
  
  for ( uint32 i = 0; i < nodes_count; ++i )
  {
    CRUDE_ARRAY_PUSH( from_transforms, model_renderer_resources_instance->nodes_transforms[ i ] );
  }
  
  for ( uint32 i = 0; i < nodes_count; ++i )
  {
    model_renderer_resources_instance->nodes_transforms[ i ] = original_transforms[ i ];
  }

  crude_gfx_model_renderer_resources_instance_update_animation( manager, model_renderer_resources_instance, to_index, 0.0f );

  for ( uint32 i = 0; i < nodes_count; ++i )
  {
    model_renderer_resources_instance->nodes_transforms[ i ] = crude_transform_lerp(
      &from_transforms[ i ],
      &model_renderer_resources_instance->nodes_transforms[ i ],
      blend_factor );
  }

  crude_stack_allocator_free_marker( temporary_allocator, temporary_allocator_marker );
}