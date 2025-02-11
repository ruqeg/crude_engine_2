#pragma once

#include <flecs.h>
#include <vulkan/vulkan.h>

#include <core/alias.h>

#define CRUDE_MAX_SWAPCHAIN_IMAGES 3

typedef struct crude_render_core_config
{
  char const                *application_name;
  uint32                     application_version;
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
  VkAllocationCallbacks     *vulkan_allocation_callbacks;
} crude_render_core;

CRUDE_API extern ECS_COMPONENT_DECLARE( crude_render_core );
CRUDE_API extern ECS_COMPONENT_DECLARE( crude_render_core_config );

CRUDE_API void crude_render_core_componentsImport( ecs_world_t *world );