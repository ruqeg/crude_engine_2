#pragma once

#include <threads.h>

#include <graphics/gpu_device.h>

/************************************************
 *
 * Invalid Renderer Resoruces Indices
 * 
 ***********************************************/
#define CRUDE_GFX_RENDERER_INVALID_TEXTURE_INDEX ( ~0u )

/************************************************
 *
 * Renderer Resoruces Handles
 * 
 ***********************************************/
typedef struct crude_gfx_renderer_buffer_handle
{
  crude_gfx_resource_index                                 index;
} crude_gfx_renderer_buffer_handle;

typedef struct crude_gfx_renderer_texture_handle
{
  crude_gfx_resource_index                                 index;
} crude_gfx_renderer_texture_handle;

typedef struct crude_gfx_renderer_sampler_handle
{
  crude_gfx_resource_index                                 index;
} crude_gfx_renderer_sampler_handle;

typedef struct crude_gfx_renderer_program_handle
{
  crude_gfx_resource_index                                 index;
} crude_gfx_renderer_program_handle;

typedef struct crude_gfx_renderer_material_handle
{
  crude_gfx_resource_index                                 index;
} crude_gfx_renderer_material_handle;

/************************************************
 *
 * Renderer Resoruces Structs
 * 
 ***********************************************/
typedef struct crude_gfx_renderer_buffer
{
  uint32                                                   references;
  crude_gfx_buffer_handle                                  handle;
  uint32                                                   pool_index;
  crude_gfx_buffer_description                             desc;
  char const                                              *name;
} crude_gfx_renderer_buffer;

typedef struct crude_gfx_renderer_texture
{
  uint32                                                   references;
  crude_gfx_texture_handle                                 handle;
  uint32                                                   pool_index;
  char const                                              *name;
} crude_gfx_renderer_texture;

typedef struct crude_gfx_renderer_sampler
{
  uint32                                                   references;
  crude_gfx_sampler_handle                                 handle;
  uint32                                                   pool_index;
  char const                                              *name;
} crude_gfx_renderer_sampler;

typedef struct crude_gfx_renderer_program_creation
{
  crude_gfx_pipeline_creation                             pipeline_creation;
} crude_gfx_renderer_program_creation;

typedef struct crude_gfx_renderer_program_pass
{
  crude_gfx_pipeline_handle                               pipeline;
  crude_gfx_descriptor_set_layout_handle                  descriptor_set_layout;
} crude_gfx_renderer_program_pass;

typedef struct crude_gfx_renderer_program
{
  crude_gfx_renderer_program_pass                          passes[ 1 ];
  uint32                                                   pool_index;
  char const                                              *name;
} crude_gfx_renderer_program;

typedef struct crude_gfx_renderer_material_creation
{
  crude_gfx_renderer_program                              *program;
  char const                                              *name;
  uint32                                                   render_index;
} crude_gfx_renderer_material_creation;

typedef struct crude_gfx_renderer_material
{
  crude_gfx_renderer_program                              *program;
  uint32                                                   render_index;
  uint32                                                   pool_index;
  char const                                              *name;
} crude_gfx_renderer_material;

/************************************************
 *
 * Renderer Structs
 * 
 ***********************************************/
typedef struct crude_gfx_renderer_creation
{
  crude_gfx_device                                        *gpu;
  crude_allocator                                          allocator;
} crude_gfx_renderer_creation;

typedef struct crude_gfx_renderer
{
  crude_gfx_device                                        *gpu;
  crude_resource_pool                                      materials;
  crude_resource_pool                                      buffers;
  crude_resource_pool                                      textures;
  crude_resource_pool                                      samplers;
  crude_resource_pool                                      programs;
  crude_allocator                                          allocator;
  crude_gfx_texture_handle                                 textures_to_update[ 128 ];
  uint32                                                   num_textures_to_update;
  mtx_t                                                    texture_update_mutex;
} crude_gfx_renderer;

/************************************************
 *
 * Renderer Initialize/Deinitialize Functions
 * 
 ***********************************************/
CRUDE_API void
crude_gfx_initialize_renderer
(
  _In_ crude_gfx_renderer                                 *renderer,
  _In_ crude_gfx_renderer_creation const                  *creation
);

CRUDE_API void
crude_gfx_deinitialize_renderer
(
  _In_ crude_gfx_renderer                                 *renderer
);

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
);

CRUDE_API void
crude_gfx_renderer_add_texture_update_commands
(
  _In_ crude_gfx_renderer                                 *renderer,
  _In_ uint32                                              thread_id
);

/************************************************
 *
 * Renderer Resoruces Functions
 * 
 ***********************************************/
CRUDE_API crude_gfx_renderer_buffer*
crude_gfx_renderer_create_buffer
(
  _In_ crude_gfx_renderer                                 *renderer,
  _In_ crude_gfx_buffer_creation const                    *creation
);

CRUDE_API void
crude_gfx_renderer_destroy_buffer
(
  _In_ crude_gfx_renderer                                 *renderer,
  _In_ crude_gfx_renderer_buffer                          *buffer
);

CRUDE_API crude_gfx_renderer_texture*
crude_gfx_renderer_create_texture
(
  _In_ crude_gfx_renderer                                 *renderer,
  _In_ crude_gfx_texture_creation const                   *creation
);

CRUDE_API void
crude_gfx_renderer_destroy_texture
(
  _In_ crude_gfx_renderer                                 *renderer,
  _In_ crude_gfx_renderer_texture                         *texture
);

CRUDE_API crude_gfx_renderer_sampler*
crude_gfx_renderer_create_sampler
(
  _In_ crude_gfx_renderer                                 *renderer,
  _In_ crude_gfx_sampler_creation const                   *creation
);

CRUDE_API void
crude_gfx_renderer_destroy_sampler
(
  _In_ crude_gfx_renderer                                 *renderer,
  _In_ crude_gfx_renderer_sampler                         *sampler
);

CRUDE_API crude_gfx_renderer_program*
crude_gfx_renderer_create_program
(
  _In_ crude_gfx_renderer                                 *renderer,
  _In_ crude_gfx_renderer_program_creation const          *creation
);

CRUDE_API void
crude_gfx_renderer_destroy_program
(
  _In_ crude_gfx_renderer                                 *renderer,
  _In_ crude_gfx_renderer_program                         *program
);

CRUDE_API crude_gfx_renderer_material*
crude_gfx_renderer_create_material
(
  _In_ crude_gfx_renderer                                 *renderer,
  _In_ crude_gfx_renderer_material_creation const         *creation
);

CRUDE_API void
crude_gfx_renderer_destroy_material
(
  _In_ crude_gfx_renderer                                 *renderer,
  _In_ crude_gfx_renderer_material                        *material
);

/************************************************
 *
 * Renderer Resoruces Pools Macros
 * 
 ***********************************************/
#define CRUDE_GFX_RENDERER_OBTAIN_BUFFER( renderer )                   CRUDE_OBTAIN_RESOURCE( renderer->buffers )
#define CRUDE_GFX_RENDERER_ACCESS_BUFFER( renderer, handle )           CRUDE_ACCESS_RESOURCE( renderer->buffers, crude_gfx_renderer_buffer, handle  )
#define CRUDE_GFX_RENDERER_RELEASE_BUFFER( renderer, handle )          CRUDE_RELEASE_RESOURCE( renderer->buffers, handle )

#define CRUDE_GFX_RENDERER_OBTAIN_TEXTURE( renderer )                  CRUDE_OBTAIN_RESOURCE( renderer->textures )
#define CRUDE_GFX_RENDERER_ACCESS_TEXTURE( renderer, handle )          CRUDE_ACCESS_RESOURCE( renderer->textures, crude_gfx_renderer_texture, handle  )
#define CRUDE_GFX_RENDERER_RELEASE_TEXTURE( renderer, handle )         CRUDE_RELEASE_RESOURCE( renderer->textures, handle )

#define CRUDE_GFX_RENDERER_OBTAIN_SAMPLER( renderer )                  CRUDE_OBTAIN_RESOURCE( renderer->samplers )
#define CRUDE_GFX_RENDERER_ACCESS_SAMPLER( renderer, handle )          CRUDE_ACCESS_RESOURCE( renderer->samplers, crude_gfx_renderer_texture, handle  )
#define CRUDE_GFX_RENDERER_RELEASE_SAMPLER( renderer, handle )         CRUDE_RELEASE_RESOURCE( renderer->samplers, handle )

#define CRUDE_GFX_RENDERER_OBTAIN_PROGRAM( renderer )                  CRUDE_OBTAIN_RESOURCE( renderer->programs )
#define CRUDE_GFX_RENDERER_ACCESS_PROGRAM( renderer, handle )          CRUDE_ACCESS_RESOURCE( renderer->programs, crude_gfx_renderer_program, handle  )
#define CRUDE_GFX_RENDERER_RELEASE_PROGRAM( renderer, handle )         CRUDE_RELEASE_RESOURCE( renderer->programs, handle )

#define CRUDE_GFX_RENDERER_OBTAIN_MATERIAL( renderer )                 CRUDE_OBTAIN_RESOURCE( renderer->materials )
#define CRUDE_GFX_RENDERER_ACCESS_MATERIAL( renderer, handle )         CRUDE_ACCESS_RESOURCE( renderer->materials, crude_gfx_renderer_material, handle  )
#define CRUDE_GFX_RENDERER_RELEASE_MATERIAL( renderer, handle )        CRUDE_RELEASE_RESOURCE( renderer->materials, handle )