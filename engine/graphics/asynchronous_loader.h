#pragma once

#include <graphics/renderer.h>

/**
 * - Process load from file request
 * - Process gpu upload transfers
 * - Manage a staging buffer to handle a copy of the data
 * - Enqueue the command buffers with copy commands
 * - Signal to the renderer that a texture has finished a transfer
 * - !TODO It's fucking borken, need huge rework (too lazy for now)
 */

/************************************************
 *
 * Asynchronous Loader Structs
 * 
 ***********************************************/
typedef struct crude_gfx_file_load_request
{
  char                                                     path[ 512 ];
  crude_gfx_texture_handle                                 texture;
  crude_gfx_buffer_handle                                  buffer;
} crude_gfx_file_load_request;


typedef struct crude_gfx_upload_request
{
  void                                                    *data;
  uint32                                                  *completed;
  crude_gfx_texture_handle                                 texture;
  crude_gfx_buffer_handle                                  cpu_buffer;
  crude_gfx_buffer_handle                                  gpu_buffer;
} crude_gfx_upload_request;


typedef struct crude_gfx_asynchronous_loader
{
  crude_gfx_renderer                                      *renderer;

  crude_gfx_file_load_request                             *file_load_requests;
  crude_gfx_upload_request                                *upload_requests;
  
  crude_gfx_buffer                                        *staging_buffer;
  uint32                                                   staging_buffer_offset;

  VkFence                                                  vk_transfer_completed_fence;

  crude_gfx_texture_handle                                 texture_ready;
  crude_gfx_buffer_handle                                  cpu_buffer_ready;
  crude_gfx_buffer_handle                                  gpu_buffer_ready;

  VkCommandPool                                            vk_cmd_pools[ CRUDE_GFX_MAX_SWAPCHAIN_IMAGES ];
  crude_gfx_cmd_buffer                                     cmd_buffers[ CRUDE_GFX_MAX_SWAPCHAIN_IMAGES ];
} crude_gfx_asynchronous_loader;

/************************************************
 *
 * Asynchronous Loader Manager Structs
 * 
 ***********************************************/
typedef struct crude_gfx_asynchronous_loader_manager_creation
{
  void                                                    *task_sheduler;
  crude_allocator_container                                allocator_container;
} crude_gfx_asynchronous_loader_manager_creation;

typedef struct crude_gfx_asynchronous_loader_manager
{
  crude_gfx_asynchronous_loader                          **async_loaders;
  bool                                                     async_loaders_valid;
  void                                                    *async_loader_task;
  void                                                    *task_sheduler;
  mtx_t                                                    task_mutex;
} crude_gfx_asynchronous_loader_manager;

/************************************************
 *
 * Asynchronous Functions Declaration
 * 
 ***********************************************/
CRUDE_API void
crude_gfx_asynchronous_loader_initialize
(
  _In_ crude_gfx_asynchronous_loader                      *asynloader,
  _In_ crude_gfx_renderer                                 *renderer
);

CRUDE_API void
crude_gfx_asynchronous_loader_deinitialize
(
  _In_ crude_gfx_asynchronous_loader                      *asynloader
);

CRUDE_API void
crude_gfx_asynchronous_loader_request_texture_data
(
  _In_ crude_gfx_asynchronous_loader                      *asynloader,
  _In_ char const                                         *filename,
  _In_ crude_gfx_texture_handle                            texture
);

CRUDE_API void
crude_gfx_asynchronous_loader_request_buffer_copy
(
  _In_ crude_gfx_asynchronous_loader                      *asynloader,
  _In_ crude_gfx_buffer_handle                             cpu_buffer,
  _In_ crude_gfx_buffer_handle                             gpu_buffer
);

CRUDE_API void
crude_gfx_asynchronous_loader_update
(
  _In_ crude_gfx_asynchronous_loader                      *asynloader
);

/************************************************
 *
 * Asynchronous Loader Manager Functions Declaration
 * 
 ***********************************************/
CRUDE_API void
crude_gfx_asynchronous_loader_manager_intiailize
(
  _In_ crude_gfx_asynchronous_loader_manager                *manager,
  _In_ crude_gfx_asynchronous_loader_manager_creation const *creation
);

CRUDE_API void
crude_gfx_asynchronous_loader_manager_add_loader
(
  _In_ crude_gfx_asynchronous_loader_manager              *manager,
  _In_ crude_gfx_asynchronous_loader                      *async_loader
);

CRUDE_API void
crude_gfx_asynchronous_loader_manager_remove_loader
(
  _In_ crude_gfx_asynchronous_loader_manager              *manager,
  _In_ crude_gfx_asynchronous_loader                      *async_loader
);

CRUDE_API void
crude_gfx_asynchronous_loader_manager_deintiailize
(
  _In_ crude_gfx_asynchronous_loader_manager              *manager
);