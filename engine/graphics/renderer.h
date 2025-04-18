#pragma once

#include <graphics/gpu_device.h>

#define CRUDE_INVALID_TEXTURE_INDEX    ( ~0u )
#define CRUDE_INVALID_SAMPLER_INDEX    ( ~0u )

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

typedef struct crude_program_handle
{
  crude_resource_handle                index;
} crude_program_handle;

typedef struct crude_material_handle
{
  crude_resource_handle                index;
} crude_material_handle;

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

typedef struct crude_program_creation
{
  crude_pipeline_creation              pipeline_creation;
} crude_program_creation;

typedef struct crude_program_pass
{
  crude_pipeline_handle                pipeline;
  crude_descriptor_set_layout_handle   descriptor_set_layout;
} crude_program_pass;

typedef struct crude_program
{
  crude_program_pass                   passes[ 1 ];
  uint32                               pool_index;
  char const                          *name;
} crude_program;

typedef struct crude_material_creation
{
  crude_program                       *program;
  char const                          *name;
  uint32                               render_index;
} crude_material_creation;

typedef struct crude_material
{
  crude_program*                       program;
  uint32                               render_index;
  uint32                               pool_index;
  char const                          *name;
} crude_material;

typedef enum crude_draw_flags
{
  CRUDE_DRAW_FLAGS_ALPHA_MASK = 1 << 0,
} crude_draw_flags;

typedef struct crude_renderer
{
  crude_gpu_device                    *gpu;
  crude_resource_pool                  materials;
  crude_resource_pool                  buffers;
  crude_resource_pool                  textures;
  crude_resource_pool                  samplers;
  crude_resource_pool                  programs;
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

CRUDE_API crude_program*
crude_gfx_renderer_create_program
(
  _In_ crude_renderer                 *renderer,
  _In_ crude_program_creation const   *creation
);

CRUDE_API crude_material*
crude_gfx_renderer_create_material
(
  _In_ crude_renderer                 *renderer,
  _In_ crude_material_creation const  *creation
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

#define CRUDE_GFX_RENDERER_OBTAIN_PROGRAM( renderer )                  CRUDE_OBTAIN_RESOURCE( renderer->programs )
#define CRUDE_GFX_RENDERER_ACCESS_PROGRAM( renderer, handle )          CRUDE_ACCESS_RESOURCE( renderer->programs, crude_program, handle  )
#define CRUDE_GFX_RENDERER_RELEASE_PROGRAM( renderer, handle )         CRUDE_RELEASE_RESOURCE( renderer->programs, handle )

#define CRUDE_GFX_RENDERER_OBTAIN_MATERIAL( renderer )                 CRUDE_OBTAIN_RESOURCE( renderer->materials )
#define CRUDE_GFX_RENDERER_ACCESS_MATERIAL( renderer, handle )         CRUDE_ACCESS_RESOURCE( renderer->materials, crude_material, handle  )
#define CRUDE_GFX_RENDERER_RELEASE_MATERIAL( renderer, handle )        CRUDE_RELEASE_RESOURCE( renderer->materials, handle )