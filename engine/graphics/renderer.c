#include <graphics/renderer.h>

/************************************************
 *
 * Renderer Initialize/Deinitialize Functions
 * 
 ***********************************************/
void
crude_gfx_initialize_renderer
(
  _In_ crude_renderer                *renderer,
  _In_ crude_renderer_creation const *creation
)
{
  renderer->gpu = creation->gpu;
  renderer->allocator = creation->allocator;
  crude_initialize_resource_pool( &renderer->buffers, renderer->allocator, 1024, sizeof( crude_buffer_resource ) );
  crude_initialize_resource_pool( &renderer->textures, renderer->allocator, 1024, sizeof( crude_texture_resource ) );
  crude_initialize_resource_pool( &renderer->samplers, renderer->allocator, 1024, sizeof( crude_sampler_resource ) );
  crude_initialize_resource_pool( &renderer->programs, renderer->allocator, 1024, sizeof( crude_program ) );
  crude_initialize_resource_pool( &renderer->materials, renderer->allocator, 1024, sizeof( crude_material ) );
}

void
crude_gfx_deinitialize_renderer
(
  _In_ crude_renderer                 *renderer
)
{
  crude_deinitialize_resource_pool( &renderer->buffers );
}

/************************************************
 *
 * Renderer Common Functions
 * 
 ***********************************************/
CRUDE_API void
crude_gfx_renderer_add_texture_to_update
(
  _In_ crude_renderer                 *renderer,
  _In_ crude_texture_handle            texture
)
{
  mtx_lock( &renderer->texture_update_mutex );
  renderer->textures_to_update[ renderer->num_textures_to_update++ ] = texture;
  mtx_unlock( &renderer->texture_update_mutex );
}

void
crude_gfx_renderer_add_texture_update_commands
(
  _In_ crude_renderer                 *renderer,
  _In_ uint32                          thread_id
)
{
  mtx_lock( &renderer->texture_update_mutex );
  
  if ( renderer->num_textures_to_update == 0 )
  {
    mtx_unlock( &renderer->texture_update_mutex );
    return;
  }
  
  //crude_command_buffer *cmd = crude_gfx_get_cmd_buffer( renderer->gpu, thread_id, false );
  //crude_gfx_cmd_begin( cmd );
  //
  //for ( u32 i = 0; i < num_textures_to_update; ++i ) {
  //
  //    Texture* texture = gpu->access_texture( textures_to_update[i] );
  //
  //    texture->vk_image_layout = add_image_barrier2( cb->vk_command_buffer, texture->vk_image, RESOURCE_STATE_COPY_DEST, RESOURCE_STATE_COPY_SOURCE,
  //                        0, 1, false, gpu->vulkan_transfer_queue_family, gpu->vulkan_main_queue_family );
  //
  //    generate_mipmaps( texture, cb, true );
  //}
  //
  //// TODO: this is done before submitting to the queue in the device.
  ////cb->end();
  //gpu->queue_command_buffer( cb );
  //
  //num_textures_to_update = 0;

  mtx_unlock( &renderer->texture_update_mutex );
}

/************************************************
 *
 * Renderer Resoruces Functions
 * 
 ***********************************************/
crude_buffer_resource*
crude_gfx_renderer_create_buffer
(
  _In_ crude_renderer                *renderer,
  _In_ crude_buffer_creation const   *creation
)
{
  crude_buffer_resource_handle buffer_resource_handle = { CRUDE_GFX_RENDERER_OBTAIN_BUFFER( renderer ) };
  if ( buffer_resource_handle.index == CRUDE_RESOURCE_INVALID_INDEX )
  {
    return NULL;
  }
  
  crude_buffer_resource *buffer_resource = CRUDE_GFX_RENDERER_ACCESS_BUFFER( renderer, buffer_resource_handle );
  buffer_resource->handle = crude_gfx_create_buffer( renderer->gpu, creation );
  buffer_resource->name = creation->name;
  buffer_resource->pool_index = buffer_resource_handle.index;
  crude_gfx_query_buffer( renderer->gpu, buffer_resource->handle, &buffer_resource->desc );
  
  if ( creation->name )
  {
    // !TODO resource_cache.buffers.insert( hash_calculate( creation.name ), buffer );
  }
  
  buffer_resource->references = 1;
  return buffer_resource;
}

void
crude_gfx_renderer_destroy_buffer
(
  _In_ crude_renderer                 *renderer,
  _In_ crude_buffer_resource          *buffer
)
{
  crude_gfx_destroy_buffer( renderer->gpu, buffer->handle );
  CRUDE_GFX_RENDERER_RELEASE_BUFFER( renderer, ( crude_buffer_resource_handle ){ buffer->pool_index } );
}

crude_texture_resource*
crude_gfx_renderer_create_texture
(
  _In_ crude_renderer                 *renderer,
  _In_ crude_texture_creation const   *creation
)
{
  crude_texture_resource_handle texture_resource_handle = { CRUDE_GFX_RENDERER_OBTAIN_TEXTURE( renderer ) };
  if ( texture_resource_handle.index == CRUDE_RESOURCE_INVALID_INDEX )
  {
    return NULL;
  }
  
  crude_texture_resource *texture_resource = CRUDE_GFX_RENDERER_ACCESS_TEXTURE( renderer, texture_resource_handle );
  texture_resource->handle = crude_gfx_create_texture( renderer->gpu, creation );
  texture_resource->name = creation->name;
  texture_resource->pool_index = texture_resource_handle.index;
  
  if ( creation->name )
  {
    // !TODO resource_cache.buffers.insert( hash_calculate( creation.name ), buffer );
  }
  
  texture_resource->references = 1;
  return texture_resource;
}

void
crude_gfx_renderer_destroy_texture
(
  _In_ crude_renderer                 *renderer,
  _In_ crude_texture_resource         *texture
)
{
  crude_gfx_destroy_texture( renderer->gpu, texture->handle );
  CRUDE_GFX_RENDERER_RELEASE_TEXTURE( renderer, ( crude_texture_resource_handle ){ texture->pool_index } );
}

crude_sampler_resource*
crude_gfx_renderer_create_sampler
(
  _In_ crude_renderer                 *renderer,
  _In_ crude_sampler_creation const   *creation
)
{
  crude_sampler_resource_handle sampler_resource_handle = { CRUDE_GFX_RENDERER_OBTAIN_TEXTURE( renderer ) };
  if ( sampler_resource_handle.index == CRUDE_RESOURCE_INVALID_INDEX )
  {
    return NULL;
  }
  
  crude_sampler_resource *sampler_resource = CRUDE_GFX_RENDERER_ACCESS_TEXTURE( renderer, sampler_resource_handle );
  sampler_resource->handle = crude_gfx_create_sampler( renderer->gpu, creation );
  sampler_resource->name = creation->name;
  sampler_resource->pool_index = sampler_resource_handle.index;
  
  if ( creation->name )
  {
    // !TODO resource_cache.buffers.insert( hash_calculate( creation.name ), buffer );
  }
  
  sampler_resource->references = 1;
  return sampler_resource;
}

void
crude_gfx_renderer_destroy_sampler
(
  _In_ crude_renderer                 *renderer,
  _In_ crude_sampler_resource         *sampler
)
{
  crude_gfx_destroy_sampler( renderer->gpu, sampler->handle );
  CRUDE_GFX_RENDERER_RELEASE_TEXTURE( renderer, ( crude_sampler_resource_handle ) { sampler->pool_index } );
}

crude_program*
crude_gfx_renderer_create_program
(
  _In_ crude_renderer                 *renderer,
  _In_ crude_program_creation const   *creation
)
{
  crude_program_handle program_handle = { CRUDE_GFX_RENDERER_OBTAIN_PROGRAM( renderer ) };
  if ( program_handle.index == CRUDE_RESOURCE_INVALID_INDEX )
  {
    return NULL;
  }
  
  crude_program *program = CRUDE_GFX_RENDERER_ACCESS_PROGRAM( renderer, program_handle );
  program->name = creation->pipeline_creation.name;
  program->pool_index = program_handle.index;
  program->passes[ 0 ].pipeline = crude_gfx_create_pipeline( renderer->gpu, &creation->pipeline_creation );
  program->passes[ 0 ].descriptor_set_layout = crude_gfx_get_descriptor_set_layout( renderer->gpu, program->passes[ 0 ].pipeline, 0 );

  if ( program->name )
  {
    // !TODO resource_cache.buffers.insert( hash_calculate( creation.name ), buffer );
  }

  return program;
}

void
crude_gfx_renderer_destroy_program
(
  _In_ crude_renderer                 *renderer,
  _In_ crude_program                  *program
)
{
  crude_gfx_destroy_pipeline( renderer->gpu, program->passes[ 0 ].pipeline );
  CRUDE_GFX_RENDERER_RELEASE_PROGRAM( renderer, ( crude_program_handle ) { program->pool_index } );
  return program;
}

crude_material*
crude_gfx_renderer_create_material
(
  _In_ crude_renderer                 *renderer,
  _In_ crude_material_creation const  *creation
)
{
  crude_material_handle material_handle = { CRUDE_GFX_RENDERER_OBTAIN_MATERIAL( renderer ) };
  if ( material_handle.index == CRUDE_RESOURCE_INVALID_INDEX )
  {
    return NULL;
  }
  
  crude_material *material = CRUDE_GFX_RENDERER_ACCESS_MATERIAL( renderer, material_handle );
  material->name = creation->name;
  material->pool_index = material_handle.index;
  material->program = creation->program;
  material->render_index = creation->render_index;
  if ( material->name )
  {
    // !TODO resource_cache.buffers.insert( hash_calculate( creation.name ), buffer );
  }

  return material;
}

void
crude_gfx_renderer_destroy_material
(
  _In_ crude_renderer                 *renderer,
  _In_ crude_material                 *material
)
{
  CRUDE_GFX_RENDERER_RELEASE_MATERIAL( renderer, ( crude_material_handle ){ material->pool_index } );
  return material;
}