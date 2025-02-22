#pragma once

#include <graphics/gpu_resources.h>

typedef struct crude_gpu_device crude_gpu_device;

typedef struct crude_command_buffer
{
  crude_gpu_device   *gpu;
  VkCommandBuffer     vk_command_buffer;
  bool                is_recording;
  crude_render_pass  *current_render_pass;
  uint32              handle;
} crude_command_buffer;

CRUDE_API void crude_reset_command_buffer( _In_ crude_command_buffer *command_buffer );