#pragma once

#include <graphics/gpu_device.h>

typedef struct crude_buffer_resource
{
  crude_buffer_handle         handle;
  uint32                      pool_index;
  crude_buffer_description    desc;
  char const                 *name;
} crude_buffer_resource;

typedef struct crude_renderer
{
  crude_gpu_device    *gpu;
  crude_resource_pool  buffers;
} crude_renderer;

CRUDE_API crude_buffer_resource*
crude_gfx_renderer_create_buffer
(
  _In_ crude_renderer              *renderer,
  _In_ crude_buffer_creation const *creation
);

#define CRUDE_GFX_RENDERER_OBTAIN_BUFFER( renderer )                   CRUDE_OBTAIN_RESOURCE( renderer->buffers )
#define CRUDE_GFX_RENDERER_ACCESS_BUFFER( renderer, handle )           CRUDE_ACCESS_RESOURCE( renderer->buffers, crude_buffer_resource, handle  )
#define CRUDE_GFX_RENDERER_RELEASE_BUFFER( renderer, handle )          CRUDE_RELEASE_RESOURCE( renderer->buffers, handle )