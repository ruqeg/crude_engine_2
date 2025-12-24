#pragma once

#include <engine/core/task_sheduler.h>
#include <engine/graphics/gpu_device.h>
#include <engine/graphics/gpu_memory.h>

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
  char                                                     path[ 1024 ];
  crude_gfx_texture_handle                                 texture;
  crude_gfx_memory_allocation                              allocation;
} crude_gfx_file_load_request;

typedef struct crude_gfx_upload_request
{
  void                                                    *data;
  uint32                                                  *completed;
  crude_gfx_texture_handle                                 texture;
  crude_gfx_memory_allocation                              cpu_allocation;
  crude_gfx_memory_allocation                              gpu_allocation;
  crude_gfx_memory_allocation                              gpu_old_allocation;
} crude_gfx_upload_request;

typedef struct crude_gfx_asynchronous_loader
{
  crude_gfx_device                                        *gpu;

  crude_gfx_file_load_request                             *file_load_requests;
  crude_gfx_upload_request                                *upload_requests;
  
  int32                                                    file_load_requests_lpos;
  int32                                                    upload_requests_lpos;
  int32                                                    file_load_requests_rpos;
  int32                                                    upload_requests_rpos;

  VkFence                                                  vk_transfer_completed_fence;
  
  crude_gfx_memory_allocation                              staging_allocation;

  crude_gfx_texture_handle                                 texture_ready;
  crude_gfx_memory_allocation                              cpu_allocation_ready;
  crude_gfx_memory_allocation                              gpu_allocation_ready;
  crude_gfx_memory_allocation                              gpu_old_allocation_ready;
  uint32                                                   total_requests_count;

  VkCommandPool                                            vk_cmd_pools[ CRUDE_GRAPHICS_MAX_SWAPCHAIN_IMAGES ];
  crude_gfx_cmd_buffer                                     cmd_buffers[ CRUDE_GRAPHICS_MAX_SWAPCHAIN_IMAGES ];

  mtx_t                                                    request_mutex;

  crude_linear_allocator                                   linear_allocator;
} crude_gfx_asynchronous_loader;

/************************************************
 *
 * Asynchronous Functions Declaration
 * 
 ***********************************************/
CRUDE_API void
crude_gfx_asynchronous_loader_initialize
(
  _In_ crude_gfx_asynchronous_loader                      *asynloader,
  _In_ crude_gfx_device                                   *gpu
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
  _In_ crude_gfx_memory_allocation                         cpu_allocation,
  _In_ crude_gfx_memory_allocation                         gpu_allocation
);

CRUDE_API void
crude_gfx_asynchronous_loader_request_buffer_reallocate_and_copy
(
  _In_ crude_gfx_asynchronous_loader                      *asynloader,
  _In_ crude_gfx_memory_allocation                         cpu_allocation,
  _In_ crude_gfx_memory_allocation                         gpu_allocation,
  _In_ crude_gfx_memory_allocation                         gpu_old_allocation
);

CRUDE_API void
crude_gfx_asynchronous_loader_update
(
  _In_ crude_gfx_asynchronous_loader                      *asynloader
);

CRUDE_API bool
crude_gfx_asynchronous_loader_has_requests
(
  _In_ crude_gfx_asynchronous_loader                      *asynloader
);