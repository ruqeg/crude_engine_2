#pragma once

#include <graphics/gpu_resources.h>

typedef struct crude_gpu_device crude_gpu_device;

typedef struct crude_command_buffer
{
  crude_gpu_device   *gpu;
  VkCommandBuffer     vk_command_buffer;
  bool                is_recording;
  crude_render_pass  *current_render_pass;
  crude_pipeline     *current_pipeline;
  VkClearValue        clears[2];
  uint32              handle;
} crude_command_buffer;

CRUDE_API void crude_reset_command_buffer( _In_ crude_command_buffer *command_buffer );
CRUDE_API void crude_bind_render_pass( _In_ crude_command_buffer *command_buffer, _In_ crude_render_pass_handle handle );
CRUDE_API void crude_bind_pipeline( _In_ crude_command_buffer *command_buffer, _In_ crude_pipeline_handle handle );

CRUDE_API void crude_set_viewport( _In_ crude_command_buffer *command_buffer, _In_opt_ crude_viewport const *viewport );
CRUDE_API void crude_set_scissor( _In_ crude_command_buffer *command_buffer, _In_opt_ crude_rect2d_int const *rect );

CRUDE_API void crude_command_buffer_draw( _In_ crude_command_buffer *command_buffer, _In_ uint32 first_vertex, _In_ uint32 vertex_count, _In_ uint32 first_instance, _In_ uint32 instance_count );