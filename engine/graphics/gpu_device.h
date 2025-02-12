#pragma once

#include <vulkan/vulkan.h>
#include <SDL3/SDL.h>
#include <vma_usage.h>

#include <core/alias.h>
#include <core/resource_pool.h>

#define CRUDE_MAX_SWAPCHAIN_IMAGES 3

typedef struct crude_gpu_device_creation
{
  SDL_Window                *sdl_window;
  char const                *vk_application_name;
  uint32                     vk_application_version;
  crude_allocator            allocator;
  uint16                     queries_per_frame;
  uint16                     max_frames;
} crude_gpu_device_creation;

typedef struct crude_gpu_device
{
  VkInstance                 vk_instance;
  VkDebugUtilsMessengerEXT   vk_debug_utils_messenger;
  VkSurfaceKHR               vk_surface;
  VkPhysicalDevice           vk_physical_device;
  VkDevice                   vk_device;
  int32                      vk_queue_family_index;
  VkQueue                    vk_queue;
  VkSwapchainKHR             vk_swapchain;
  uint32                     vk_swapchain_images_count;
  VkImage                    vk_swapchain_images[ CRUDE_MAX_SWAPCHAIN_IMAGES ];
  VkImageView                vk_swapchain_images_views[ CRUDE_MAX_SWAPCHAIN_IMAGES ];
  VkDescriptorPool           vk_descriptor_pool;
  VkQueryPool                vk_timestamp_query_pool;
  crude_resource_pool        buffers;
  crude_resource_pool        textures;
  crude_resource_pool        pipelines;
  crude_resource_pool        samplers;
  crude_resource_pool        descriptor_set_layouts;
  crude_resource_pool        descriptor_sets;
  crude_resource_pool        render_passes;
  crude_resource_pool        command_buffers;
  crude_resource_pool        shaders;
  VmaAllocator               vma_allocator;
  VkAllocationCallbacks     *vk_allocation_callbacks;
  crude_allocator            allocator;
  uint16                     max_frames;
} crude_gpu_device;

CRUDE_API void crude_initialize_gpu_device( _In_ crude_gpu_device *gpu, _In_ crude_gpu_device_creation *creation );
CRUDE_API void crude_deinitialize_gpu_device( _In_ crude_gpu_device *gpu );