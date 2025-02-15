#pragma once

#include <graphics/gpu_resources.h>

typedef struct crude_gpu_device crude_gpu_device;

typedef struct crude_command_buffer
{
  crude_render_pass              *current_render_pass;
  crude_gpu_device               *gpu;
  uint32                          handle;
  VkCommandBuffer                 vk_command_buffer;
  bool                            is_recording;
} crude_command_buffer;

CRUDE_API crude_reset_command_buffer( _In_ crude_command_buffer *command_buffer );