#pragma once

#include <engine/graphics/gpu_resources.h>
#include <engine/graphics/gpu_memory.h>

/************************************************
 *
 * Command Buffer & Command Buffer Manager Structs
 * 
 ***********************************************/
typedef struct crude_gfx_device crude_gfx_device;

typedef struct crude_gfx_cmd_buffer_manager
{
  crude_gfx_device                                        *gpu;

  crude_gfx_cmd_buffer_handle                             *primary_cmd_buffers;

  uint32                                                   num_pools_per_frame;
  uint32                                                   num_primary_cmd_buffers_per_thread;

  uint8                                                   *num_used_primary_cmd_buffers_per_frame;
} crude_gfx_cmd_buffer_manager;

/************************************************
 *
 * Command Buffer Functions
 * 
 ***********************************************/
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
crude_gfx_cmd_copy_texture
(
  _In_ crude_gfx_cmd_buffer                               *cmd,
  _In_ crude_gfx_texture_handle                            src_handle,
  _In_ crude_gfx_texture_handle                            dst_handle
);

CRUDE_API void
crude_gfx_cmd_set_viewport
(
  _In_ crude_gfx_cmd_buffer                               *cmd,
  _In_opt_ crude_gfx_viewport const                       *dev_viewport
);

CRUDE_API void
crude_gfx_cmd_set_clear_color_f32
(
  _In_ crude_gfx_cmd_buffer                               *cmd,
  _In_ float32                                             r,
  _In_ float32                                             g,
  _In_ float32                                             b,
  _In_ float32                                             a,
  _In_ uint32                                              index
);

CRUDE_API void
crude_gfx_cmd_set_clear_depth_and_stencil
(
  _In_ crude_gfx_cmd_buffer                               *cmd,
  _In_ float32                                             depth,
  _In_ float32                                             stencil
);

CRUDE_API void
crude_gfx_cmd_set_scissor
(
  _In_ crude_gfx_cmd_buffer                               *cmd,
  _In_opt_ crude_gfx_rect2d_int const                     *rect
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
crude_gfx_cmd_draw_inderect
(
  _In_ crude_gfx_cmd_buffer                               *cmd,
  _In_ crude_gfx_buffer_handle                             buffer_handle,
  _In_ uint32                                              offset,
  _In_ uint32                                              draw_count,
  _In_ uint32                                              stride
);

CRUDE_API void
crude_gfx_cmd_draw_indirect_count
(
  _In_ crude_gfx_cmd_buffer                               *cmd,
  _In_ crude_gfx_buffer_handle                             argument_buffer_handle,
  _In_ uint32                                              argument_offset,
  _In_ crude_gfx_buffer_handle                             count_buffer_handle,
  _In_ uint32                                              count_offset,
  _In_ uint32                                              max_draws,
  _In_ uint32                                              stride
);

CRUDE_API void
crude_gfx_cmd_draw_mesh_task
(
  _In_ crude_gfx_cmd_buffer                               *cmd,
  _In_ uint32                                              group_count_x,
  _In_ uint32                                              group_count_y,
  _In_ uint32                                              group_count_z
);

CRUDE_API void
crude_gfx_cmd_draw_mesh_task_indirect_count
(
  _In_ crude_gfx_cmd_buffer                               *cmd,
  _In_ crude_gfx_buffer_handle                             argument_buffer_handle,
  _In_ uint32                                              argument_offset,
  _In_ crude_gfx_buffer_handle                             count_buffer_handle,
  _In_ uint32                                              count_offset,
  _In_ uint32                                              max_draws,
  _In_ uint32                                              stride
);

CRUDE_API void
crude_gfx_cmd_dispatch
(
  _In_ crude_gfx_cmd_buffer                               *cmd,
  _In_ uint32                                              group_count_x,
  _In_ uint32                                              group_count_y,
  _In_ uint32                                              group_count_z
);

CRUDE_API void
crude_gfx_cmd_bind_bindless_descriptor_set
(
  _In_ crude_gfx_cmd_buffer                               *cmd
);

CRUDE_API void
crude_gfx_cmd_bind_acceleration_structure_descriptor_set
(
  _In_ crude_gfx_cmd_buffer                               *cmd,
  _In_ crude_gfx_descriptor_set_handle                     handle
);

CRUDE_API void
crude_gfx_cmd_add_buffer_barrier
(
  _In_ crude_gfx_cmd_buffer                               *cmd,
  _In_ crude_gfx_buffer_handle                             buffer_handle,
  _In_ crude_gfx_resource_state                            old_state,
  _In_ crude_gfx_resource_state                            new_state
);

CRUDE_API void
crude_gfx_cmd_add_image_barrier
(
  _In_ crude_gfx_cmd_buffer                               *cmd,
  _In_ crude_gfx_texture                                  *texture,
  _In_ crude_gfx_resource_state                            new_state,
  _In_ uint32                                              base_mip_level,
  _In_ uint32                                              mip_count,
  _In_ bool                                                is_depth
);

CRUDE_API void
crude_gfx_cmd_add_image_barrier_ext
(
  _In_ crude_gfx_cmd_buffer                               *cmd,
  _In_ crude_gfx_texture                                  *texture,
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
crude_gfx_cmd_add_image_barrier_ext2
(
  _In_ crude_gfx_cmd_buffer                               *cmd,
  _In_ VkImage                                             vk_image,
  _In_ crude_gfx_resource_state                            old_state,
  _In_ crude_gfx_resource_state                            new_state,
  _In_ uint32                                              base_mip_level,
  _In_ uint32                                              mip_count,
  _In_ bool                                                is_depth
);

CRUDE_API void
crude_gfx_cmd_add_image_barrier_ext3
(
  _In_ crude_gfx_cmd_buffer                               *cmd,
  _In_ VkImage                                             vk_image,
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
crude_gfx_cmd_add_image_barrier_ext4
(
  _In_ crude_gfx_cmd_buffer                               *cmd,
  _In_ crude_gfx_texture                                  *texture,
  _In_ crude_gfx_resource_state                            new_state,
  _In_ uint32                                              base_mip_level,
  _In_ uint32                                              mip_count,
  _In_ uint32                                              base_array_layer,
  _In_ uint32                                              array_layer_count,
  _In_ bool                                                is_depth
);

CRUDE_API void
crude_gfx_cmd_add_image_barrier_ext5
(
  _In_ crude_gfx_cmd_buffer                               *cmd,
  _In_ VkImage                                             vk_image,
  _In_ crude_gfx_resource_state                            old_state,
  _In_ crude_gfx_resource_state                            new_state,
  _In_ uint32                                              base_mip_level,
  _In_ uint32                                              mip_count,
  _In_ uint32                                              base_array_layer,
  _In_ uint32                                              array_layer_count,
  _In_ bool                                                is_depth,
  _In_ uint32                                              source_queue_family,
  _In_ uint32                                              destination_family,
  _In_ crude_gfx_queue_type                                source_queue_type,
  _In_ crude_gfx_queue_type                                destination_queue_type
);

CRUDE_API void
crude_gfx_cmd_global_debug_barrier
(
  _In_ crude_gfx_cmd_buffer                               *cmd
);

CRUDE_API void
crude_gfx_cmd_memory_copy_to_texture
(
  _In_ crude_gfx_cmd_buffer                               *cmd,
  _In_ crude_gfx_texture_handle                            texture_handle,
  _In_ crude_gfx_memory_allocation                         memory_allocation
);

CRUDE_API void
crude_gfx_cmd_memory_copy
(
  _In_ crude_gfx_cmd_buffer                               *cmd,
  _In_ crude_gfx_memory_allocation                         src_memory_allocation,
  _In_ crude_gfx_memory_allocation                         dst_memory_allocation,
  _In_ uint64                                              src_offset,
  _In_ uint64                                              dst_offset
);

CRUDE_API void
crude_gfx_cmd_push_marker
(
  _In_ crude_gfx_cmd_buffer                               *cmd,
  _In_ char const                                         *name
);

CRUDE_API void
crude_gfx_cmd_pop_marker
(
  _In_ crude_gfx_cmd_buffer                               *cmd
);

CRUDE_API void
crude_gfx_cmd_push_constant
(
  _In_ crude_gfx_cmd_buffer                               *cmd,
  _In_ void const                                         *data,
  _In_ uint64                                              size
);

CRUDE_API void
crude_gfx_cmd_fill_buffer
(
  _In_ crude_gfx_cmd_buffer                               *cmd,
  _In_ crude_gfx_buffer_handle                             handle,
  _In_ uint32                                              value
);

CRUDE_API void
crude_gfx_cmd_trace_rays
(
  _In_ crude_gfx_cmd_buffer                               *cmd,
  _In_ crude_gfx_pipeline_handle                           pipeline_handle,
  _In_ uint32                                              width,
  _In_ uint32                                              height,
  _In_ uint32                                              depth
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
  _In_ uint32                                              num_pools_per_frame,
  _In_ uint32                                              num_primary_cmd_buffers_per_pool
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