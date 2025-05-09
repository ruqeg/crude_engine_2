#pragma once

#include <graphics/gpu_resources.h>

/************************************************
 *
 * Command Buffer & Command Buffer Manager Structs
 * 
 ***********************************************/
typedef struct crude_gfx_device crude_gfx_device;

typedef struct crude_gfx_cmd_buffer
{
  crude_gfx_device                                        *gpu;
  bool                                                     is_recording;
  crude_gfx_render_pass                                   *current_render_pass;
  crude_gfx_framebuffer                                   *current_framebuffer;
  crude_gfx_pipeline                                      *current_pipeline;
  VkClearValue                                             clears[ 2 ];
  /**
   * frame_descriptor_sets holds the resource pool for descriptor sets for the current frame.
   * They will be automatically deallocated on the end of each frame.
   * Can be allocated by crude_gfx_cmd_create_local_descriptor_set()
   */
  crude_resource_pool                                      frame_descriptor_sets;
  VkDescriptorPool                                         vk_descriptor_pool;
  VkCommandBuffer                                          vk_cmd_buffer;
} crude_gfx_cmd_buffer;

typedef struct crude_gfx_cmd_buffer_manager
{
  crude_gfx_device                                        *gpu;

  VkCommandPool                                           *vk_cmd_pools;
  crude_gfx_cmd_buffer                                    *primary_cmd_buffers;
  crude_gfx_cmd_buffer                                    *secondary_cmd_buffers;

  uint32                                                   num_pools_per_frame;
  uint32                                                   num_primary_cmd_buffers_per_thread;
  uint32                                                   num_secondary_cmd_buffer_per_pool;

  uint8                                                   *num_used_primary_cmd_buffers_per_frame;
  uint8                                                   *num_used_secondary_cmd_buffers_per_frame;
} crude_gfx_cmd_buffer_manager;

/************************************************
 *
 * Command Buffer Functions
 * 
 ***********************************************/
CRUDE_API void
crude_gfx_cmd_initialize
(
  _In_ crude_gfx_cmd_buffer                               *cmd,
  _In_ crude_gfx_device                                   *gpu
);

CRUDE_API void
crude_gfx_cmd_deinitialize
(
  _In_ crude_gfx_cmd_buffer                               *cmd
);

CRUDE_API void
crude_gfx_cmd_reset
(
  _In_ crude_gfx_cmd_buffer                               *cmd
);

CRUDE_API void
crude_gfx_cmd_begin_primary
(
  _In_ crude_gfx_cmd_buffer                               *cmd
);

CRUDE_API void
crude_gfx_cmd_begin_secondary
(
  _In_ crude_gfx_cmd_buffer                               *cmd,
  _In_ crude_gfx_render_pass                              *render_pass,
  _In_ crude_gfx_framebuffer                              *framebuffer
);

CRUDE_API void
crude_gfx_cmd_end
(
  _In_ crude_gfx_cmd_buffer                               *cmd
);

CRUDE_API void
crude_gfx_cmd_end_render_pass
(
  _In_ crude_gfx_cmd_buffer                               *cmd
);

CRUDE_API void
crude_gfx_cmd_bind_render_pass
(
  _In_ crude_gfx_cmd_buffer                               *cmd,
  _In_ crude_gfx_render_pass_handle                        render_pass_handle,
  _In_ crude_gfx_framebuffer_handle                        framebuffer_handle,
  _In_ bool                                                use_secondary
);

CRUDE_API void
crude_gfx_cmd_bind_pipeline
(
  _In_ crude_gfx_cmd_buffer                               *cmd,
  _In_ crude_gfx_pipeline_handle                           handle
);

CRUDE_API void
crude_gfx_cmd_set_viewport
(
  _In_ crude_gfx_cmd_buffer                               *cmd,
  _In_opt_ crude_gfx_viewport const                       *viewport
);

CRUDE_API void
crude_gfx_cmd_set_clear_color
(
  _In_ crude_gfx_cmd_buffer                               *cmd,
  _In_ uint32                                              index,
  _In_ VkClearValue                                        clear
);

CRUDE_API void
crude_gfx_cmd_set_scissor
(
  _In_ crude_gfx_cmd_buffer                               *cmd,
  _In_opt_ crude_gfx_rect2d_int const                     *rect
);

CRUDE_API void
crude_gfx_cmd_bind_local_descriptor_set
(
  _In_ crude_gfx_cmd_buffer                               *cmd,
  _In_ crude_gfx_descriptor_set_handle                     handle
);

CRUDE_API void
crude_gfx_cmd_draw
(
  _In_ crude_gfx_cmd_buffer                               *cmd,
  _In_ uint32                                              first_vertex,
  _In_ uint32                                              vertex_count,
  _In_ uint32                                              first_instance,
  _In_ uint32                                              instance_count
);

CRUDE_API void
crude_gfx_cmd_draw_indexed
(
  _In_ crude_gfx_cmd_buffer                               *cmd,
  _In_ uint32                                              index_count,
  _In_ uint32                                              instance_count,
  _In_ uint32                                              first_index,
  _In_ int32                                               vertex_offset,
  _In_ uint32                                              first_instance
);

CRUDE_API void
crude_gfx_cmd_bind_vertex_buffer
(
  _In_ crude_gfx_cmd_buffer                               *cmd,
  _In_ crude_gfx_buffer_handle                             handle,
  _In_ uint32                                              binding,
  _In_ uint32                                              offset
);

CRUDE_API void
crude_gfx_cmd_bind_index_buffer
(
  _In_ crude_gfx_cmd_buffer                               *cmd,
  _In_ crude_gfx_buffer_handle                             handle,
  _In_ uint32                                              offset
);

CRUDE_API crude_gfx_descriptor_set_handle
crude_gfx_cmd_create_local_descriptor_set
(
  _In_ crude_gfx_cmd_buffer                               *cmd,
  _In_ crude_gfx_descriptor_set_creation const            *creation
);

CRUDE_API void
crude_gfx_cmd_add_image_barrier
(
  _In_ crude_gfx_cmd_buffer                               *cmd,
  _In_ VkImage                                             image,
  _In_ crude_gfx_resource_state                            old_state,
  _In_ crude_gfx_resource_state                            new_state,
  _In_ uint32                                              base_mip_level,
  _In_ uint32                                              mip_count,
  _In_ bool                                                is_depth
);

CRUDE_API VkImageLayout
crude_gfx_cmd_add_image_barrier_ext
(
  _In_ crude_gfx_cmd_buffer                               *cmd,
  _In_ VkImage                                             image,
  _In_ crude_gfx_resource_state                            old_state,
  _In_ crude_gfx_resource_state                            new_state,
  _In_ uint32                                              base_mip_level,
  _In_ uint32                                              mip_count,
  _In_ bool                                                is_depth,
  _In_ uint32                                              source_queue_family,
  _In_ uint32                                              destination_family,
  _In_ crude_gfx_queue_type                                source_queue_type,
  _In_ crude_gfx_queue_type                                destination_queue_type
);

CRUDE_API void
crude_gfx_cmd_upload_texture_data
(
  _In_ crude_gfx_cmd_buffer                               *cmd,
  _In_ crude_gfx_texture_handle                            texture_handle,
  _In_ void                                               *texture_data,
  _In_ crude_gfx_buffer_handle                             staging_buffer_handle,
  _In_ uint64                                              staging_buffer_offset
);

CRUDE_API void
crude_gfx_cmd_upload_buffer_data
(
  _In_ crude_gfx_cmd_buffer                               *cmd,
  _In_ crude_gfx_buffer_handle                             src_buffer,
  _In_ crude_gfx_buffer_handle                             dst_buffer
);

/************************************************
 *
 * Command Buffer Manager Functions
 * 
 ***********************************************/
CRUDE_API void
crude_gfx_cmd_manager_initialize
(
  _In_ crude_gfx_cmd_buffer_manager                       *cmd_manager,
  _In_ crude_gfx_device                                   *gpu,
  _In_ uint32                                              num_threads
);

CRUDE_API void
crude_gfx_cmd_manager_deinitialize
(
  _In_ crude_gfx_cmd_buffer_manager                       *cmd_manager
);

CRUDE_API void
crude_gfx_cmd_manager_reset
(
  _In_ crude_gfx_cmd_buffer_manager                       *cmd_manager,
  _In_ uint32                                              frame
);

CRUDE_API crude_gfx_cmd_buffer*
crude_gfx_cmd_manager_get_primary_cmd
(
  _In_ crude_gfx_cmd_buffer_manager                       *cmd_manager,
  _In_ uint32                                              frame,
  _In_ uint32                                              thread_index,
  _In_ bool                                                begin
);

CRUDE_API crude_gfx_cmd_buffer*
crude_gfx_cmd_manager_get_secondary_cmd
(
  _In_ crude_gfx_cmd_buffer_manager                       *cmd_manager,
  _In_ uint32                                              frame,
  _In_ uint32                                              thread_index
);