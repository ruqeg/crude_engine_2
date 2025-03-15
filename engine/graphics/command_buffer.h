#pragma once

#include <graphics/gpu_resources.h>

#define CRUDE_COMMAND_BUFFER_MANAGER_MAX_THREADS     1
#define CRUDE_COMMAND_BUFFER_MANAGER_MAX_POOLS       CRUDE_MAX_SWAPCHAIN_IMAGES * CRUDE_COMMAND_BUFFER_MANAGER_MAX_THREADS
#define CRUDE_COMMAND_BUFFER_MANAGER_BUFFER_PER_POOL 3
#define CRUDE_COMMAND_BUFFER_MANAGER_MAX_BUFFERS     CRUDE_COMMAND_BUFFER_MANAGER_BUFFER_PER_POOL * CRUDE_COMMAND_BUFFER_MANAGER_MAX_POOLS

typedef struct crude_gpu_device crude_gpu_device;

typedef struct crude_command_buffer
{
  crude_gpu_device                  *gpu;
  VkCommandBuffer                    vk_handle;
  bool                               is_recording;
  crude_render_pass                 *current_render_pass;
  crude_pipeline                    *current_pipeline;
  VkClearValue                       clears[ 2 ];
} crude_command_buffer;

typedef struct crude_command_buffer_manager
{
  VkCommandPool                      vk_command_pools[ CRUDE_COMMAND_BUFFER_MANAGER_MAX_POOLS ];
  crude_command_buffer               command_buffers[ CRUDE_COMMAND_BUFFER_MANAGER_MAX_BUFFERS ];
  crude_gpu_device                  *gpu;
} crude_command_buffer_manager;

CRUDE_API void
crude_gfx_cmd_reset
(
  _In_ crude_command_buffer         *cmd
);

CRUDE_API void
crude_gfx_cmd_bind_render_pass
(
  _In_ crude_command_buffer         *cmd,
  _In_ crude_render_pass_handle      handle
);

CRUDE_API void
crude_gfx_cmd_bind_pipeline
(
  _In_ crude_command_buffer         *cmd,
  _In_ crude_pipeline_handle         handle
);

CRUDE_API void
crude_gfx_cmd_set_viewport
(
  _In_ crude_command_buffer         *cmd,
  _In_opt_ crude_viewport const     *viewport
);

CRUDE_API void
crude_gfx_cmd_set_scissor
(
  _In_ crude_command_buffer         *cmd,
  _In_opt_ crude_rect2d_int const   *rect
);

CRUDE_API void
crude_gfx_cmd_draw
(
  _In_ crude_command_buffer         *cmd,
  _In_ uint32                        first_vertex,
  _In_ uint32                        vertex_count,
  _In_ uint32                        first_instance,
  _In_ uint32                        instance_count
);

CRUDE_API void
crude_gfx_cmd_draw_indexed
(
  _In_ crude_command_buffer         *cmd,
  _In_ uint32                        index_count,
  _In_ uint32                        instance_count,
  _In_ uint32                        first_index,
  _In_ int32                         vertex_offset,
  _In_ uint32                        first_instance
);

CRUDE_API void
crude_gfx_cmd_bind_vertex_buffer
(
  _In_ crude_command_buffer         *cmd,
  _In_ crude_buffer_handle           handle,
  _In_ uint32                        binding,
  _In_ uint32                        offset
);

CRUDE_API void
crude_gfx_cmd_bind_index_buffer
(
  _In_ crude_command_buffer         *cmd,
  _In_ crude_buffer_handle           handle,
  _In_ uint32                        offset
);

CRUDE_API void
crude_gfx_initialize_cmd_manager
(
  _In_ crude_command_buffer_manager *cmd_manager,
  _In_ crude_gpu_device             *gpu
);

CRUDE_API void
crude_gfx_deinitialize_cmd_manager
(
  _In_ crude_command_buffer_manager *cmd_manager
);

CRUDE_API void
crude_gfx_reset_cmd_manager
(
  _In_ crude_command_buffer_manager *cmd_manager,
  _In_ uint32                        frame
);

CRUDE_API crude_command_buffer*
crude_gfx_cmd_manager_get_cmd_buffer
(
  _In_ crude_command_buffer_manager *cmd_manager,
  _In_ uint32                        frame,
  _In_ bool                          begin
);

CRUDE_API crude_command_buffer*
crude_gfx_cmd_manager_get_cmd_buffer_instant
(
  _In_ crude_command_buffer_manager *cmd_manager,
  _In_ uint32                        frame
);