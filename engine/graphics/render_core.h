#pragma once

#include <flecs.h>
#include <vulkan/vulkan.h>
#include <vma/vk_mem_alloc.h>

#include <core/alias.h>
#include <core/resource_pool.h>

#define CRUDE_MAX_SWAPCHAIN_IMAGES 3
#define CRUDE_MAX_FRAMES           3

typedef struct crude_render_core_config
{
  char const                *application_name;
  uint32                     application_version;
  crude_allocator            allocator;
} crude_render_core_config;

typedef struct crude_render_core
{
  VkInstance                 vulkan_instance;
  VkDebugUtilsMessengerEXT   vulkan_debug_utils_messenger;
  VkSurfaceKHR               vulkan_surface;
  VkPhysicalDevice           vulkan_physical_device;
  VkDevice                   vulkan_device;
  int32                      vulkan_queue_family_index;
  VkQueue                    vulkan_queue;
  VkSwapchainKHR             vulkan_swapchain;
  uint32                     vulkan_swapchain_images_count;
  VkImage                    vulkan_swapchain_images[ CRUDE_MAX_SWAPCHAIN_IMAGES ];
  VkImageView                vulkan_swapchain_images_views[ CRUDE_MAX_SWAPCHAIN_IMAGES ];
  VkDescriptorPool           vulkan_descriptor_pool;
  VkQueryPool                vulkan_timestamp_query_pool;
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
  VkAllocationCallbacks     *vulkan_allocation_callbacks;

} crude_render_core;

CRUDE_API extern ECS_COMPONENT_DECLARE( crude_render_core );
CRUDE_API extern ECS_COMPONENT_DECLARE( crude_render_core_config );

CRUDE_API void crude_render_core_componentsImport( ecs_world_t *world );