#pragma once

#include <graphics/gpu_device.h>

typedef struct crude_buffer_resource_handle
{
  crude_resource_handle                index;
} crude_buffer_resource_handle;

typedef struct crude_texture_resource_handle
{
  crude_resource_handle                index;
} crude_texture_resource_handle;

typedef struct crude_sampler_resource_handle
{
  crude_resource_handle                index;
} crude_sampler_resource_handle;

typedef struct crude_buffer_resource
{
  uint32                               references;
  crude_buffer_handle                  handle;
  uint32                               pool_index;
  crude_buffer_description             desc;
  char const                          *name;
} crude_buffer_resource;

typedef struct crude_texture_resource
{
  uint32                               references;
  crude_texture_handle                 handle;
  uint32                               pool_index;
  char const                          *name;
} crude_texture_resource;

typedef struct crude_sampler_resource
{
  uint32                               references;
  crude_sampler_handle                 handle;
  uint32                               pool_index;
  char const                          *name;
} crude_sampler_resource;

typedef struct crude_renderer
{
  crude_gpu_device                    *gpu;
  crude_resource_pool                  buffers;
  crude_resource_pool                  textures;
  crude_resource_pool                  samplers;
  crude_allocator                      allocator;
} crude_renderer;

typedef struct crude_renderer_creation
{
  crude_gpu_device                    *gpu;
  crude_allocator                      allocator;
} crude_renderer_creation;

CRUDE_API void
crude_gfx_initialize_renderer
(
  _In_ crude_renderer                 *renderer,
  _In_ crude_renderer_creation const  *creation
);

CRUDE_API void
crude_gfx_deinitialize_renderer
(
  _In_ crude_renderer                 *renderer
);

CRUDE_API crude_buffer_resource*
crude_gfx_renderer_create_buffer
(
  _In_ crude_renderer                 *renderer,
  _In_ crude_buffer_creation const    *creation
);

CRUDE_API crude_texture_resource*
crude_gfx_renderer_create_texture
(
  _In_ crude_renderer                 *renderer,
  _In_ crude_texture_creation const   *creation
);

CRUDE_API crude_sampler_resource*
crude_gfx_renderer_create_sampler
(
  _In_ crude_renderer                 *renderer,
  _In_ crude_sampler_creation const   *creation
);

#define CRUDE_GFX_RENDERER_OBTAIN_BUFFER( renderer )                   CRUDE_OBTAIN_RESOURCE( renderer->buffers )
#define CRUDE_GFX_RENDERER_ACCESS_BUFFER( renderer, handle )           CRUDE_ACCESS_RESOURCE( renderer->buffers, crude_buffer_resource, handle  )
#define CRUDE_GFX_RENDERER_RELEASE_BUFFER( renderer, handle )          CRUDE_RELEASE_RESOURCE( renderer->buffers, handle )

#define CRUDE_GFX_RENDERER_OBTAIN_TEXTURE( renderer )                  CRUDE_OBTAIN_RESOURCE( renderer->textures )
#define CRUDE_GFX_RENDERER_ACCESS_TEXTURE( renderer, handle )          CRUDE_ACCESS_RESOURCE( renderer->textures, crude_texture_resource, handle  )
#define CRUDE_GFX_RENDERER_RELEASE_TEXTURE( renderer, handle )         CRUDE_RELEASE_RESOURCE( renderer->textures, handle )

#define CRUDE_GFX_RENDERER_OBTAIN_SAMPLER( renderer )                  CRUDE_OBTAIN_RESOURCE( renderer->samplers )
#define CRUDE_GFX_RENDERER_ACCESS_SAMPLER( renderer, handle )          CRUDE_ACCESS_RESOURCE( renderer->samplers, crude_texture_resource, handle  )
#define CRUDE_GFX_RENDERER_RELEASE_SAMPLER( renderer, handle )         CRUDE_RELEASE_RESOURCE( renderer->samplers, handle )