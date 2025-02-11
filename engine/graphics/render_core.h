#pragma once

#include <flecs.h>
#include <vulkan/vulkan.h>

#include <core/alias.h>

typedef struct crude_render_core_config
{
  char const                *application_name;
  uint32                     application_version;
  VkAllocationCallbacks     *allocation_callbacks;
} crude_render_core_config;

typedef struct crude_render_core
{
  VkInstance                 instance;
  VkDebugUtilsMessengerEXT   debug_utils_messenger;
  VkSurfaceKHR               surface;
  VkPhysicalDevice           physical_device;
  VkDevice                   device;
  int32                      queue_family_index;
  VkQueue                    queue;
  VkAllocationCallbacks     *allocation_callbacks;
} crude_render_core;

CRUDE_API extern ECS_COMPONENT_DECLARE( crude_render_core );
CRUDE_API extern ECS_COMPONENT_DECLARE( crude_render_core_config );

CRUDE_API void crude_render_core_componentsImport( ecs_world_t *world );