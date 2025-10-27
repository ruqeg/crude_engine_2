#include <graphics/renderer.h>

#include <core/hash_map.h>
#include <core/string.h>

/************************************************
 *
 * Renderer Initialize/Deinitialize Functions
 * 
 ***********************************************/
void
crude_gfx_renderer_initialize
(
  _In_ crude_gfx_renderer                                 *renderer,
  _In_ crude_gfx_renderer_creation const                  *creation
)
{
  renderer->gpu = creation->gpu;
  renderer->allocator_container = creation->allocator_container;

  crude_resource_pool_initialize( &renderer->buffers, renderer->allocator_container, 1024, sizeof( crude_gfx_renderer_buffer ) );
  crude_resource_pool_initialize( &renderer->textures, renderer->allocator_container, 1024, sizeof( crude_gfx_renderer_texture ) );
  crude_resource_pool_initialize( &renderer->samplers, renderer->allocator_container, 1024, sizeof( crude_gfx_renderer_sampler ) );
  crude_resource_pool_initialize( &renderer->techniques, renderer->allocator_container, 1024, sizeof( crude_gfx_renderer_technique ) );
  crude_resource_pool_initialize( &renderer->materials, renderer->allocator_container, 1024, sizeof( crude_gfx_renderer_material ) );

  renderer->num_textures_to_update = 0;

  mtx_init( &renderer->texture_update_mutex, mtx_plain );

  CRUDE_HASHMAP_INITIALIZE( renderer->resource_cache.techniques, renderer->allocator_container );
  CRUDE_HASHMAP_INITIALIZE( renderer->resource_cache.materials, renderer->allocator_container );
}

void
crude_gfx_renderer_deinitialize
(
  _In_ crude_gfx_renderer                                 *renderer
)
{
  for ( uint32 i = 0; i < CRUDE_HASHMAP_CAPACITY( renderer->resource_cache.techniques ); ++i )
  {
    if ( renderer->resource_cache.techniques[ i ].key )
    {
      crude_gfx_renderer_destroy_technique( renderer, renderer->resource_cache.techniques[ i ].value );
    }
  }
  CRUDE_HASHMAP_DEINITIALIZE( renderer->resource_cache.techniques );

  for ( uint32 i = 0; i < CRUDE_HASHMAP_CAPACITY( renderer->resource_cache.materials ); ++i )
  {
    if ( renderer->resource_cache.materials[ i ].key )
    {
      crude_gfx_renderer_destroy_material( renderer, renderer->resource_cache.materials[ i ].value );
    }
  }
  CRUDE_HASHMAP_DEINITIALIZE( renderer->resource_cache.materials );
  
  crude_resource_pool_deinitialize( &renderer->buffers );
  crude_resource_pool_deinitialize( &renderer->textures );
  crude_resource_pool_deinitialize( &renderer->samplers );
  crude_resource_pool_deinitialize( &renderer->techniques );
  crude_resource_pool_deinitialize( &renderer->materials );
  mtx_destroy( &renderer->texture_update_mutex );
}

/************************************************
 *
 * Renderer Common Functions
 * 
 ***********************************************/
CRUDE_API void
crude_gfx_renderer_add_texture_to_update
(
  _In_ crude_gfx_renderer                                 *renderer,
  _In_ crude_gfx_texture_handle                            texture
)
{
  mtx_lock( &renderer->texture_update_mutex );
  renderer->textures_to_update[ renderer->num_textures_to_update++ ] = texture;
  mtx_unlock( &renderer->texture_update_mutex );
}

void
crude_gfx_renderer_add_texture_update_commands
(
  _In_ crude_gfx_renderer                                 *renderer,
  _In_ crude_gfx_cmd_buffer                               *cmd
)
{
  VkCommandBufferBeginInfo                                 begin_info;
  VkSubmitInfo2                                            submit_info;

  mtx_lock( &renderer->texture_update_mutex );
  
  if ( renderer->num_textures_to_update == 0 )
  {
    mtx_unlock( &renderer->texture_update_mutex );
    return;
  }
  
  // !TODO this is a temporary fix, should just queue_cmd
  for ( uint32 i = 0; i < renderer->num_textures_to_update; ++i )
  {
    crude_gfx_texture *texture = crude_gfx_access_texture( renderer->gpu, renderer->textures_to_update[ i ] );
    crude_gfx_cmd_add_image_barrier_ext3( cmd, texture->vk_image, CRUDE_GFX_RESOURCE_STATE_COPY_DEST, CRUDE_GFX_RESOURCE_STATE_SHADER_RESOURCE, 0, 1, false, renderer->gpu->vk_transfer_queue_family, renderer->gpu->vk_main_queue_family, CRUDE_GFX_QUEUE_TYPE_COPY_TRANSFER, CRUDE_GFX_QUEUE_TYPE_GRAPHICS );
    texture->ready = true;
    texture->state = CRUDE_GFX_RESOURCE_STATE_SHADER_RESOURCE;
    crude_gfx_generate_mipmaps( cmd, texture );
  }
  //crude_gfx_submit_immediate( cmd );

  //crude_gfx_cmd_buffer *cmd = crude_gfx_get_primary_cmd( renderer->gpu, thread_id, true );

  //for ( uint32 i = 0; i < renderer->num_textures_to_update; ++i )
  //{
  //  crude_gfx_texture *texture = crude_gfx_access_texture( renderer->gpu, renderer->textures_to_update[ i ] );
  //  crude_gfx_cmd_add_image_barrier_ext3( cmd, texture->vk_image, CRUDE_GFX_RESOURCE_STATE_COPY_DEST, CRUDE_GFX_RESOURCE_STATE_SHADER_RESOURCE, 0, 1, false, renderer->gpu->vk_transfer_queue_family, renderer->gpu->vk_main_queue_family, CRUDE_GFX_QUEUE_TYPE_COPY_TRANSFER, CRUDE_GFX_QUEUE_TYPE_GRAPHICS );
  //  texture->load_state = CRUDE_GFX_ASYNC_RESOURCE_ASYNS_LOAD_STATE_QUEUED;
  //  texture->state = CRUDE_GFX_RESOURCE_STATE_SHADER_RESOURCE;
  //  //generate_mipmaps( texture, cb, true );
  //}

  //crude_gfx_queue_cmd( cmd );
  
  renderer->num_textures_to_update = 0;

  mtx_unlock( &renderer->texture_update_mutex );
}

crude_gfx_renderer_technique*
crude_gfx_renderer_access_technique_by_name
(
  _In_ crude_gfx_renderer                                 *renderer,
  _In_ char const                                         *technique_name
)
{
  uint64 technique_name_hashed = crude_hash_string( technique_name, 0 );
  crude_gfx_renderer_technique *technique = CRUDE_HASHMAP_GET( renderer->resource_cache.techniques, technique_name_hashed )->value;
  return technique;
}

crude_gfx_renderer_technique_pass*
crude_gfx_renderer_access_technique_pass_by_name
(
  _In_ crude_gfx_renderer                                 *renderer,
  _In_ char const                                         *technique_name,
  _In_ char const                                         *pass_name
)
{
  crude_gfx_renderer_technique *technique = crude_gfx_renderer_access_technique_by_name( renderer, technique_name );
  uint64 pass_index = crude_gfx_renderer_technique_get_pass_index( technique, pass_name );
  return &technique->passes[ pass_index ];
}

/************************************************
 *
 * Renderer Resoruces Functions
 * 
 ***********************************************/
crude_gfx_renderer_buffer*
crude_gfx_renderer_create_buffer
(
  _In_ crude_gfx_renderer                                 *renderer,
  _In_ crude_gfx_buffer_creation const                    *creation
)
{
  crude_gfx_renderer_buffer_handle buffer_resource_handle = crude_gfx_renderer_obtain_buffer( renderer );
  if ( CRUDE_RESOURCE_HANDLE_IS_INVALID( buffer_resource_handle ) )
  {
    return NULL;
  }
  
  crude_gfx_renderer_buffer *buffer_resource = crude_gfx_renderer_access_buffer( renderer, buffer_resource_handle );
  buffer_resource->handle = crude_gfx_create_buffer( renderer->gpu, creation );
  buffer_resource->name = creation->name;
  buffer_resource->pool_index = buffer_resource_handle.index;
  crude_gfx_query_buffer( renderer->gpu, buffer_resource->handle, &buffer_resource->desc );
  
  if ( creation->name )
  {
    // !OPTTODO
    // Hash name and add to buffer hashmap
  }
  
  buffer_resource->references = 1;
  return buffer_resource;
}

void
crude_gfx_renderer_destroy_buffer
(
  _In_ crude_gfx_renderer                                 *renderer,
  _In_ crude_gfx_renderer_buffer                          *buffer
)
{
  crude_gfx_destroy_buffer( renderer->gpu, buffer->handle );
  crude_gfx_renderer_release_buffer( renderer, CRUDE_COMPOUNT( crude_gfx_renderer_buffer_handle, { buffer->pool_index } ) );
}

crude_gfx_renderer_texture*
crude_gfx_renderer_create_texture
(
  _In_ crude_gfx_renderer                                 *renderer,
  _In_ crude_gfx_texture_creation const                   *creation
)
{
  crude_gfx_renderer_texture_handle texture_resource_handle = crude_gfx_renderer_obtain_texture( renderer );
  if ( CRUDE_RESOURCE_HANDLE_IS_INVALID( texture_resource_handle ) )
  {
    return NULL;
  }
  
  crude_gfx_renderer_texture *texture_resource = crude_gfx_renderer_access_texture( renderer, texture_resource_handle );
  texture_resource->handle = crude_gfx_create_texture( renderer->gpu, creation );
  texture_resource->name = creation->name;
  texture_resource->pool_index = texture_resource_handle.index;
  
  if ( creation->name )
  {
    // !OPTTODO
    // Hash name and add to texture hashmap
  }
  
  texture_resource->references = 1;
  return texture_resource;
}

void
crude_gfx_renderer_destroy_texture
(
  _In_ crude_gfx_renderer                                 *renderer,
  _In_ crude_gfx_renderer_texture                         *texture
)
{
  crude_gfx_destroy_texture( renderer->gpu, texture->handle );
  crude_gfx_renderer_release_texture( renderer, CRUDE_COMPOUNT( crude_gfx_renderer_texture_handle, { texture->pool_index } ) );
}

crude_gfx_renderer_sampler*
crude_gfx_renderer_create_sampler
(
  _In_ crude_gfx_renderer                                 *renderer,
  _In_ crude_gfx_sampler_creation const                   *creation
)
{
  crude_gfx_renderer_sampler_handle sampler_resource_handle = crude_gfx_renderer_obtain_sampler( renderer );
  if ( CRUDE_RESOURCE_HANDLE_IS_INVALID( sampler_resource_handle ) )
  {
    return NULL;
  }
  
  crude_gfx_renderer_sampler *sampler_resource = crude_gfx_renderer_access_sampler( renderer, sampler_resource_handle );
  sampler_resource->handle = crude_gfx_create_sampler( renderer->gpu, creation );
  sampler_resource->name = creation->name;
  sampler_resource->pool_index = sampler_resource_handle.index;
  
  if ( creation->name )
  {
    // !OPTTODO
    // Hash name and add to sampler hashmap
  }
  
  sampler_resource->references = 1;
  return sampler_resource;
}

void
crude_gfx_renderer_destroy_sampler
(
  _In_ crude_gfx_renderer                                 *renderer,
  _In_ crude_gfx_renderer_sampler                         *sampler
)
{
  crude_gfx_destroy_sampler( renderer->gpu, sampler->handle );
  crude_gfx_renderer_release_sampler( renderer, CRUDE_COMPOUNT( crude_gfx_renderer_sampler_handle, { sampler->pool_index } ) );
}


crude_gfx_renderer_material*
crude_gfx_renderer_create_material
(
  _In_ crude_gfx_renderer                                 *renderer,
  _In_ crude_gfx_renderer_material_creation const         *creation
)
{
  crude_gfx_renderer_material_handle material_handle = crude_gfx_renderer_obtain_material( renderer );
  if ( CRUDE_RESOURCE_HANDLE_IS_INVALID( material_handle ) )
  {
    return NULL;
  }
  
  crude_gfx_renderer_material *material = crude_gfx_renderer_access_material( renderer, material_handle );
  material->name = creation->name;
  material->pool_index = material_handle.index;
  material->technique = creation->technique;
  material->render_index = creation->render_index;

  if ( material->name )
  {
    uint64 key = crude_hash_string( material->name, 0 );
    CRUDE_HASHMAP_SET( renderer->resource_cache.materials, key, material );
  }

  return material;
}

void
crude_gfx_renderer_destroy_material
(
  _In_ crude_gfx_renderer                                 *renderer,
  _In_ crude_gfx_renderer_material                        *material
)
{
  crude_gfx_renderer_release_material( renderer, CRUDE_COMPOUNT( crude_gfx_renderer_material_handle, { material->pool_index } ) );
}

crude_gfx_renderer_technique*
crude_gfx_renderer_create_technique
(
  _In_ crude_gfx_renderer                                 *renderer,
  _In_ crude_gfx_renderer_technique_creation const        *creation
)
{
  crude_gfx_renderer_technique_handle technique_handle = crude_gfx_renderer_obtain_technique( renderer );
  if ( CRUDE_RESOURCE_HANDLE_IS_INVALID( technique_handle ) )
  {
    return NULL;
  }

  crude_gfx_renderer_technique *technique = crude_gfx_renderer_access_technique( renderer, technique_handle );
  
  technique->json_name = creation->json_name;
  technique->name = creation->name;

  CRUDE_ARRAY_INITIALIZE_WITH_CAPACITY( technique->passes, creation->passes_count, renderer->gpu->allocator_container );
  CRUDE_HASHMAP_INITIALIZE( technique->name_hashed_to_pass_index, renderer->gpu->allocator_container );

  for ( size_t i = 0; i < creation->passes_count; ++i )
  {
    crude_gfx_renderer_technique_pass                     *pass;
    crude_gfx_renderer_technique_pass_creation const      *pass_creation;
    crude_gfx_pipeline                                    *pipeline;
    crude_gfx_renderer_technique_pass                      technique_pass;
    uint64                                                 pass_name_hashed;

    pass = &technique->passes[ i ];
    
    pass_creation = &creation->passes[ i ];
    technique_pass = CRUDE_COMPOUNT( crude_gfx_renderer_technique_pass, { pass_creation->pipeline } );
    if ( CRUDE_RESOURCE_HANDLE_IS_INVALID( technique_pass.pipeline ) )
    {
      return NULL;
    }

    pipeline = crude_gfx_access_pipeline( renderer->gpu, technique_pass.pipeline );

    CRUDE_ARRAY_PUSH( technique->passes, technique_pass );
    
    pass_name_hashed = crude_hash_string( pipeline->name, 0 );
    CRUDE_HASHMAP_SET( technique->name_hashed_to_pass_index, pass_name_hashed, i );
    
    for ( uint32 i = 0; i < pipeline->num_active_layouts; ++i )
    {
      crude_gfx_descriptor_set_layout const *descriptor_set_layout = pipeline->descriptor_set_layout[ i ];
      if ( descriptor_set_layout == NULL )
      {
        continue;
      }
      
      for ( uint32 b = 0; b < descriptor_set_layout->num_bindings; ++b )
      {
        // !OPTTODO
        // we can get binding->name from reflect of shader, but how to manager names, don't want to allocate sep memory for each of them
        // crude_gfx_descriptor_binding const *binding = &descriptor_set_layout->bindings[ b ];
        // uint64 binding_name_hashed = crude_hash_string( binding->name, 0 );
        // CRUDE_HASHMAP_SET( pass->name_hashed_to_descriptor_index, binding_name_hashed, binding->start );
      }
    }

  }
  
  if ( creation->name )
  {
    uint64 key = crude_hash_string( creation->name, 0 );
    CRUDE_HASHMAP_SET( renderer->resource_cache.techniques, key, technique );
  }

  technique->pool_index = technique_handle.index;
  return technique;
}

void
crude_gfx_renderer_destroy_technique
(
  _In_ crude_gfx_renderer                                 *renderer,
  _In_ crude_gfx_renderer_technique                       *technique
)
{
  if ( !technique )
  {
    return;
  }

  uint64 technique_name_hashed = crude_hash_string( technique->name, 0u );
  
  for ( size_t i = 0; i < CRUDE_ARRAY_LENGTH( technique->passes ); ++i )
  {
    crude_gfx_destroy_pipeline( renderer->gpu, technique->passes[ i ].pipeline );
    if ( technique->passes[ i ].name_hashed_to_descriptor_index )
    {
      CRUDE_HASHMAP_DEINITIALIZE( technique->passes[ i ].name_hashed_to_descriptor_index );
    }
  }
  
  CRUDE_ARRAY_DEINITIALIZE( technique->passes );
  CRUDE_HASHMAP_DEINITIALIZE( technique->name_hashed_to_pass_index );
  CRUDE_HASHMAP_REMOVE( renderer->resource_cache.techniques, technique_name_hashed );
  crude_gfx_renderer_release_technique( renderer, CRUDE_COMPOUNT( crude_gfx_renderer_technique_handle, { technique->pool_index } ) );
}

/************************************************
 *
 * Renderer Resoruces Pools Functions
 * 
 ***********************************************/
crude_gfx_renderer_buffer_handle
crude_gfx_renderer_obtain_buffer
(
  _In_ crude_gfx_renderer                                 *renderer
)
{
  return CRUDE_COMPOUNT( crude_gfx_renderer_buffer_handle, { crude_resource_pool_obtain_resource( &renderer->buffers ) } );
}

crude_gfx_renderer_buffer*
crude_gfx_renderer_access_buffer
(
  _In_ crude_gfx_renderer                                 *renderer,
  _In_ crude_gfx_renderer_buffer_handle                    handle
)
{
  return CRUDE_REINTERPRET_CAST( crude_gfx_renderer_buffer*, crude_resource_pool_access_resource( &renderer->buffers, handle.index ) );
}

void
crude_gfx_renderer_release_buffer
(
  _In_ crude_gfx_renderer                                 *renderer,
  _In_ crude_gfx_renderer_buffer_handle                    handle
)
{
  crude_resource_pool_release_resource( &renderer->buffers, handle.index );
}

crude_gfx_renderer_texture_handle
crude_gfx_renderer_obtain_texture
(
  _In_ crude_gfx_renderer                                 *renderer
)
{
  return CRUDE_COMPOUNT( crude_gfx_renderer_texture_handle, { crude_resource_pool_obtain_resource( &renderer->textures ) } );
}

crude_gfx_renderer_texture*
crude_gfx_renderer_access_texture
(
  _In_ crude_gfx_renderer                                 *renderer,
  _In_ crude_gfx_renderer_texture_handle                   handle
)
{
  return CRUDE_REINTERPRET_CAST( crude_gfx_renderer_texture*, crude_resource_pool_access_resource( &renderer->textures, handle.index ) );
}

void
crude_gfx_renderer_release_texture
(
  _In_ crude_gfx_renderer                                 *renderer,
  _In_ crude_gfx_renderer_texture_handle                   handle
)
{
  crude_resource_pool_release_resource( &renderer->textures, handle.index );
}

crude_gfx_renderer_sampler_handle
crude_gfx_renderer_obtain_sampler
(
  _In_ crude_gfx_renderer                                 *renderer
)
{
  return CRUDE_COMPOUNT( crude_gfx_renderer_sampler_handle, { crude_resource_pool_obtain_resource( &renderer->samplers ) } );
}

crude_gfx_renderer_sampler*
crude_gfx_renderer_access_sampler
(
  _In_ crude_gfx_renderer                                 *renderer,
  _In_ crude_gfx_renderer_sampler_handle                   handle
)
{
  return CRUDE_REINTERPRET_CAST( crude_gfx_renderer_sampler*, crude_resource_pool_access_resource( &renderer->samplers, handle.index ) );
}

void
crude_gfx_renderer_release_sampler
(
  _In_ crude_gfx_renderer                                 *renderer,
  _In_ crude_gfx_renderer_sampler_handle                   handle
)
{
  crude_resource_pool_release_resource( &renderer->samplers, handle.index );
}

crude_gfx_renderer_material_handle
crude_gfx_renderer_obtain_material
(
  _In_ crude_gfx_renderer                                 *renderer
)
{
  return CRUDE_COMPOUNT( crude_gfx_renderer_material_handle, { crude_resource_pool_obtain_resource( &renderer->materials ) } );
}

crude_gfx_renderer_material*
crude_gfx_renderer_access_material
(
  _In_ crude_gfx_renderer                                 *renderer,
  _In_ crude_gfx_renderer_material_handle                  handle
)
{
  return CRUDE_REINTERPRET_CAST( crude_gfx_renderer_material*, crude_resource_pool_access_resource( &renderer->materials, handle.index ) );
}

void
crude_gfx_renderer_release_material
(
  _In_ crude_gfx_renderer                                 *renderer,
  _In_ crude_gfx_renderer_material_handle                  handle
)
{
   crude_resource_pool_release_resource( &renderer->materials, handle.index );
}

crude_gfx_renderer_technique_handle
crude_gfx_renderer_obtain_technique
(
  _In_ crude_gfx_renderer                                 *renderer
)
{
  return CRUDE_COMPOUNT( crude_gfx_renderer_technique_handle, { crude_resource_pool_obtain_resource( &renderer->techniques ) } );
}

crude_gfx_renderer_technique*
crude_gfx_renderer_access_technique
(
  _In_ crude_gfx_renderer                                 *renderer,
  _In_ crude_gfx_renderer_technique_handle                 handle
)
{
  return CRUDE_REINTERPRET_CAST( crude_gfx_renderer_technique*, crude_resource_pool_access_resource( &renderer->techniques, handle.index ) );
}

void
crude_gfx_renderer_release_technique
(
  _In_ crude_gfx_renderer                                 *renderer,
  _In_ crude_gfx_renderer_technique_handle                 handle
)
{
   crude_resource_pool_release_resource( &renderer->techniques, handle.index );
}