#pragma once

#include <vulkan/vulkan.h>

typedef struct crude_gpu_device crude_gpu_device;

typedef struct crude_command_buffer
{
  crude_gpu_device               *gpu;
  uint32                          handle;
  VkCommandBuffer                 vk_command_buffer;
} crude_command_buffer;

CRUDE_API crude_reset_command_buffer( _In_ crude_command_buffer *command_buffer );