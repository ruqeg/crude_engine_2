#include <graphics/renderer.h>

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
}

void
crude_gfx_deinitialize_renderer
(
  _In_ crude_renderer                 *renderer
)
{
  crude_deinitialize_resource_pool( &renderer->buffers );
}

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