#pragma once

#include <threads.h>

#include <graphics/renderer_resources.h>
#include <graphics/gpu_device.h>

typedef struct crude_gfx_renderer_resource_cache
{
  struct{ uint64 key; crude_gfx_renderer_technique *value; }         *techniques;
  struct{ uint64 key; crude_gfx_renderer_material *value; }          *materials;
} crude_gfx_renderer_resource_cache;

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
  crude_resource_pool                                      techniques;
  crude_allocator_container                                allocator_container;
  crude_gfx_texture_handle                                 textures_to_update[ 128 ];
  uint32                                                   num_textures_to_update;
  mtx_t                                                    texture_update_mutex;
  crude_gfx_renderer_resource_cache                        resource_cache;
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

CRUDE_API crude_gfx_renderer_technique*
crude_gfx_renderer_access_technique_by_name
(
  _In_ crude_gfx_renderer                                 *renderer,
  _In_ char const                                         *technique_name
);

CRUDE_API crude_gfx_renderer_technique_pass*
crude_gfx_renderer_access_technique_pass_by_name
(
  _In_ crude_gfx_renderer                                 *renderer,
  _In_ char const                                         *technique_name,
  _In_ char const                                         *pass_name
);

CRUDE_API void                                     
crude_gfx_renderer_generate_mipmaps
(                                                  
  _In_ crude_gfx_renderer                                 *renderer,
  _In_ crude_gfx_texture                                  *texture
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

CRUDE_API crude_gfx_renderer_technique*
crude_gfx_renderer_create_technique
(
  _In_ crude_gfx_renderer                                 *renderer,
  _In_ crude_gfx_renderer_technique_creation const        *creation
);

CRUDE_API void
crude_gfx_renderer_destroy_technique
(
  _In_ crude_gfx_renderer                                 *renderer,
  _In_ crude_gfx_renderer_technique                       *technique
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

CRUDE_API crude_gfx_renderer_technique_handle
crude_gfx_renderer_obtain_technique
(
  _In_ crude_gfx_renderer                                 *renderer
);

CRUDE_API crude_gfx_renderer_technique*
crude_gfx_renderer_access_technique
(
  _In_ crude_gfx_renderer                                 *renderer,
  _In_ crude_gfx_renderer_technique_handle                 handle
);

CRUDE_API void
crude_gfx_renderer_release_technique
(
  _In_ crude_gfx_renderer                                 *renderer,
  _In_ crude_gfx_renderer_technique_handle                 handle
);