#pragma once

#include <graphics/gpu_resources.h>

typedef struct crude_gpu_device crude_gpu_device;

typedef struct crude_command_buffer
{
  crude_gpu_device  *gpu;
  VkCommandBuffer    vk_command_buffer;
} crude_command_buffer;