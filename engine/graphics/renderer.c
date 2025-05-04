#include <graphics/renderer.h>

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
  renderer->allocator = creation->allocator;

  crude_resource_pool_initialize( &renderer->buffers, renderer->allocator, 1024, sizeof( crude_gfx_renderer_buffer ) );
  crude_resource_pool_initialize( &renderer->textures, renderer->allocator, 1024, sizeof( crude_gfx_renderer_texture ) );
  crude_resource_pool_initialize( &renderer->samplers, renderer->allocator, 1024, sizeof( crude_gfx_renderer_sampler ) );
  crude_resource_pool_initialize( &renderer->programs, renderer->allocator, 1024, sizeof( crude_gfx_renderer_program ) );
  crude_resource_pool_initialize( &renderer->materials, renderer->allocator, 1024, sizeof( crude_gfx_renderer_material ) );

  renderer->num_textures_to_update = 0;

  mtx_init( &renderer->texture_update_mutex, mtx_plain );
}

void
crude_gfx_renderer_deinitialize
(
  _In_ crude_gfx_renderer                                 *renderer
)
{
  crude_resource_pool_deinitialize( &renderer->buffers );
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
  _In_ uint32                                              thread_id
)
{
  mtx_lock( &renderer->texture_update_mutex );
  
  if ( renderer->num_textures_to_update == 0 )
  {
    mtx_unlock( &renderer->texture_update_mutex );
    return;
  }
  
  crude_gfx_cmd_buffer *cmd = crude_gfx_get_primary_cmd( renderer->gpu, thread_id, true );

  for ( uint32 i = 0; i < renderer->num_textures_to_update; ++i )
  {
    crude_gfx_texture *texture = crude_gfx_access_texture( renderer->gpu, renderer->textures_to_update[ i ] );
    texture->vk_image_layout = crude_gfx_cmd_add_image_barrier_ext( cmd, texture->vk_image, CRUDE_GFX_RESOURCE_STATE_COPY_DEST, CRUDE_GFX_RESOURCE_STATE_COPY_SOURCE, 0, 1, false, renderer->gpu->vk_transfer_queue_family, renderer->gpu->vk_main_queue_family, CRUDE_GFX_QUEUE_TYPE_GRAPHICS, CRUDE_GFX_QUEUE_TYPE_GRAPHICS );
    
    //generate_mipmaps( texture, cb, true );
  }

  crude_gfx_queue_cmd( cmd );
  
  renderer->num_textures_to_update = 0;

  mtx_unlock( &renderer->texture_update_mutex );
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
  crude_gfx_renderer_buffer_handle buffer_resource_handle = { CRUDE_GFX_RENDERER_OBTAIN_BUFFER( renderer ) };
  if ( CRUDE_RESOURCE_HANDLE_IS_INVALID( buffer_resource_handle ) )
  {
    return NULL;
  }
  
  crude_gfx_renderer_buffer *buffer_resource = CRUDE_GFX_RENDERER_ACCESS_BUFFER( renderer, buffer_resource_handle );
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
  _In_ crude_gfx_renderer                                 *renderer,
  _In_ crude_gfx_renderer_buffer                          *buffer
)
{
  crude_gfx_destroy_buffer( renderer->gpu, buffer->handle );
  CRUDE_GFX_RENDERER_RELEASE_BUFFER( renderer, ( crude_gfx_renderer_buffer_handle ){ buffer->pool_index } );
}

crude_gfx_renderer_texture*
crude_gfx_renderer_create_texture
(
  _In_ crude_gfx_renderer                                 *renderer,
  _In_ crude_gfx_texture_creation const                   *creation
)
{
  crude_gfx_renderer_texture_handle texture_resource_handle = { CRUDE_GFX_RENDERER_OBTAIN_TEXTURE( renderer ) };
  if ( CRUDE_RESOURCE_HANDLE_IS_INVALID( texture_resource_handle ) )
  {
    return NULL;
  }
  
  crude_gfx_renderer_texture *texture_resource = CRUDE_GFX_RENDERER_ACCESS_TEXTURE( renderer, texture_resource_handle );
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
  _In_ crude_gfx_renderer                                 *renderer,
  _In_ crude_gfx_renderer_texture                         *texture
)
{
  crude_gfx_destroy_texture( renderer->gpu, texture->handle );
  CRUDE_GFX_RENDERER_RELEASE_TEXTURE( renderer, ( crude_gfx_renderer_texture_handle ){ texture->pool_index } );
}

crude_gfx_renderer_sampler*
crude_gfx_renderer_create_sampler
(
  _In_ crude_gfx_renderer                                 *renderer,
  _In_ crude_gfx_sampler_creation const                   *creation
)
{
  crude_gfx_renderer_sampler_handle sampler_resource_handle = { CRUDE_GFX_RENDERER_OBTAIN_TEXTURE( renderer ) };
  if ( CRUDE_RESOURCE_HANDLE_IS_INVALID( sampler_resource_handle ) )
  {
    return NULL;
  }
  
  crude_gfx_renderer_sampler *sampler_resource = CRUDE_GFX_RENDERER_ACCESS_TEXTURE( renderer, sampler_resource_handle );
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
  _In_ crude_gfx_renderer                                 *renderer,
  _In_ crude_gfx_renderer_sampler                         *sampler
)
{
  crude_gfx_destroy_sampler( renderer->gpu, sampler->handle );
  CRUDE_GFX_RENDERER_RELEASE_TEXTURE( renderer, ( crude_gfx_renderer_sampler_handle ) { sampler->pool_index } );
}

crude_gfx_renderer_program*
crude_gfx_renderer_create_program
(
  _In_ crude_gfx_renderer                                 *renderer,
  _In_ crude_gfx_renderer_program_creation const          *creation
)
{
  crude_gfx_renderer_program_handle program_handle = { CRUDE_GFX_RENDERER_OBTAIN_PROGRAM( renderer ) };
  if ( CRUDE_RESOURCE_HANDLE_IS_INVALID( program_handle ) )
  {
    return NULL;
  }
  
  crude_gfx_renderer_program *program = CRUDE_GFX_RENDERER_ACCESS_PROGRAM( renderer, program_handle );
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
  _In_ crude_gfx_renderer                                 *renderer,
  _In_ crude_gfx_renderer_program                         *program
)
{
  crude_gfx_destroy_pipeline( renderer->gpu, program->passes[ 0 ].pipeline );
  CRUDE_GFX_RENDERER_RELEASE_PROGRAM( renderer, ( crude_gfx_renderer_program_handle ) { program->pool_index } );
  return program;
}

crude_gfx_renderer_material*
crude_gfx_renderer_create_material
(
  _In_ crude_gfx_renderer                                 *renderer,
  _In_ crude_gfx_renderer_material_creation const         *creation
)
{
  crude_gfx_renderer_material_handle material_handle = { CRUDE_GFX_RENDERER_OBTAIN_MATERIAL( renderer ) };
  if ( CRUDE_RESOURCE_HANDLE_IS_INVALID( material_handle ) )
  {
    return NULL;
  }
  
  crude_gfx_renderer_material *material = CRUDE_GFX_RENDERER_ACCESS_MATERIAL( renderer, material_handle );
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
  _In_ crude_gfx_renderer                                 *renderer,
  _In_ crude_gfx_renderer_material                        *material
)
{
  CRUDE_GFX_RENDERER_RELEASE_MATERIAL( renderer, ( crude_gfx_renderer_material_handle ){ material->pool_index } );
  return material;
}