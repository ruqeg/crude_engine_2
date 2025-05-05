#pragma once

#include <threads.h>

#include <graphics/renderer_resources.h>
#include <graphics/gpu_device.h>

/************************************************
 *
 * Renderer Structs
 * 
 ***********************************************/
typedef struct crude_gfx_renderer_creation
{
  crude_gfx_device                                        *gpu;
  crude_allocator_container                                allocator_container;
} crude_gfx_renderer_creation;

typedef struct crude_gfx_renderer
{
  crude_gfx_device                                        *gpu;
  crude_resource_pool                                      materials;
  crude_resource_pool                                      buffers;
  crude_resource_pool                                      textures;
  crude_resource_pool                                      samplers;
  crude_resource_pool                                      programs;
  crude_allocator_container                                allocator_container;
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
crude_gfx_renderer_initialize
(
  _In_ crude_gfx_renderer                                 *renderer,
  _In_ crude_gfx_renderer_creation const                  *creation
);

CRUDE_API void
crude_gfx_renderer_deinitialize
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
 * Renderer Resoruces Pools Functions
 * 
 ***********************************************/
CRUDE_API crude_gfx_renderer_buffer_handle
crude_gfx_renderer_obtain_buffer
(
  _In_ crude_gfx_renderer                                 *renderer
);

CRUDE_API crude_gfx_renderer_buffer*
crude_gfx_renderer_access_buffer
(
  _In_ crude_gfx_renderer                                 *renderer,
  _In_ crude_gfx_renderer_buffer_handle                    handle
);

CRUDE_API void
crude_gfx_renderer_release_buffer
(
  _In_ crude_gfx_renderer                                 *renderer,
  _In_ crude_gfx_renderer_buffer_handle                    handle
);

CRUDE_API crude_gfx_renderer_texture_handle
crude_gfx_renderer_obtain_texture
(
  _In_ crude_gfx_renderer                                 *renderer
);

CRUDE_API crude_gfx_renderer_texture*
crude_gfx_renderer_access_texture
(
  _In_ crude_gfx_renderer                                 *renderer,
  _In_ crude_gfx_renderer_texture_handle                   handle
);

CRUDE_API void
crude_gfx_renderer_release_texture
(
  _In_ crude_gfx_renderer                                 *renderer,
  _In_ crude_gfx_renderer_texture_handle                   handle
);

CRUDE_API crude_gfx_renderer_sampler_handle
crude_gfx_renderer_obtain_sampler
(
  _In_ crude_gfx_renderer                                 *renderer
);

CRUDE_API crude_gfx_renderer_sampler*
crude_gfx_renderer_access_sampler
(
  _In_ crude_gfx_renderer                                 *renderer,
  _In_ crude_gfx_renderer_sampler_handle                   handle
);

CRUDE_API void
crude_gfx_renderer_release_sampler
(
  _In_ crude_gfx_renderer                                 *renderer,
  _In_ crude_gfx_renderer_sampler_handle                   handle
);

CRUDE_API crude_gfx_renderer_program_handle
crude_gfx_renderer_obtain_program
(
  _In_ crude_gfx_renderer                                 *renderer
);

CRUDE_API crude_gfx_renderer_program*
crude_gfx_renderer_access_program
(
  _In_ crude_gfx_renderer                                 *renderer,
  _In_ crude_gfx_renderer_program_handle                   handle
);

CRUDE_API void
crude_gfx_renderer_release_program
(
  _In_ crude_gfx_renderer                                 *renderer,
  _In_ crude_gfx_renderer_program_handle                   handle
);

CRUDE_API crude_gfx_renderer_material_handle
crude_gfx_renderer_obtain_material
(
  _In_ crude_gfx_renderer                                 *renderer
);

CRUDE_API crude_gfx_renderer_material*
crude_gfx_renderer_access_material
(
  _In_ crude_gfx_renderer                                 *renderer,
  _In_ crude_gfx_renderer_material_handle                  handle
);

CRUDE_API void
crude_gfx_renderer_release_material
(
  _In_ crude_gfx_renderer                                 *renderer,
  _In_ crude_gfx_renderer_material_handle                  handle
);