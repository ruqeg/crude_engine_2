#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>
#include <stb_ds.h>
#include <math.h>

#include <core/assert.h>
#include <graphics/gpu_resources.h>

#include <graphics/gpu_device.h>

#define CLAMP( v, lo, hi ) ( ( ( v ) < ( lo ) ) ? ( lo ) : ( ( hi ) < ( v ) ) ? ( hi ) : ( v ) )

static char const *const vk_device_required_extensions[] = 
{ 
  VK_KHR_SWAPCHAIN_EXTENSION_NAME, 
  VK_KHR_SPIRV_1_4_EXTENSION_NAME, 
  VK_EXT_MESH_SHADER_EXTENSION_NAME, 
  VK_KHR_SHADER_FLOAT_CONTROLS_EXTENSION_NAME, 
  VK_KHR_8BIT_STORAGE_EXTENSION_NAME, 
  VK_KHR_PUSH_DESCRIPTOR_EXTENSION_NAME, 
  VK_EXT_ROBUSTNESS_2_EXTENSION_NAME,
  VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME,
  VK_KHR_RAY_QUERY_EXTENSION_NAME,
  VK_KHR_RAY_TRACING_PIPELINE_EXTENSION_NAME,
  VK_KHR_DEFERRED_HOST_OPERATIONS_EXTENSION_NAME,
  VK_KHR_BUFFER_DEVICE_ADDRESS_EXTENSION_NAME,
  VK_EXT_DESCRIPTOR_INDEXING_EXTENSION_NAME
};

static char const *const *vk_instance_required_extensions[] =
{
  VK_EXT_DEBUG_UTILS_EXTENSION_NAME
};

static char const *const *vk_required_layers[] =
{
  "VK_LAYER_KHRONOS_validation"
};

#define HANDLE_VULKAN_RESULT( result, msg ) if ( result != VK_SUCCESS ) CRUDE_ABORT( CRUDE_CHANNEL_GRAPHICS, "vulkan result isn't success: %i %s", result, msg );

static VKAPI_ATTR VkBool32 debug_callback(
  _In_ VkDebugUtilsMessageSeverityFlagBitsEXT      messageSeverity,
  _In_ VkDebugUtilsMessageTypeFlagsEXT             messageType,
  _In_ VkDebugUtilsMessengerCallbackDataEXT const *pCallbackData,
  _In_ void                                       *pUserData)
{
  if ( messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT )
  {
    CRUDE_LOG_ERROR( CRUDE_CHANNEL_GRAPHICS, "validation layer: %s", pCallbackData->pMessage );
  }
  else if ( messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT )
  {
    CRUDE_LOG_WARNING( CRUDE_CHANNEL_GRAPHICS, "validation layer: %s", pCallbackData->pMessage );
  }
  return VK_FALSE;
}

static VkInstance create_instance(
  _In_     char const            *vk_application_name,
  _In_     uint32                 vk_application_version,
  _In_opt_ VkAllocationCallbacks *vk_allocation_callbacks)
{
  // extensions
  uint32 surface_extensions_count;
  char const *const *surface_extensions_array = SDL_Vulkan_GetInstanceExtensions( &surface_extensions_count );
  uint32 const debug_extensions_count = ARRAY_SIZE( vk_instance_required_extensions );

  char const **instance_enabled_extensions = NULL;
  uint32 const instance_enabled_extensions_count = surface_extensions_count + debug_extensions_count;
  arrsetlen( instance_enabled_extensions, instance_enabled_extensions_count ); // tofree
  CRUDE_ASSERT( instance_enabled_extensions );

  for ( uint32 i = 0; i < surface_extensions_count; ++i )
  {
    instance_enabled_extensions[i] = surface_extensions_array[i];
  }
  for ( uint32 i = 0; i < debug_extensions_count; ++i )
  {
    instance_enabled_extensions[surface_extensions_count + i] = vk_instance_required_extensions [i];
  }

  // layers
  char const *instance_enabled_layers[] = { "VK_LAYER_KHRONOS_validation" };

  // application
  VkApplicationInfo application = ( VkApplicationInfo ) {
    .pApplicationName   = vk_application_name,
    .applicationVersion = vk_application_version,
    .pEngineName        = "crude_engine",
    .engineVersion      = VK_MAKE_VERSION( 1, 0, 0 ),
    .apiVersion         = VK_API_VERSION_1_0 
  };

  // initialize instance & debug_utils_messenger
  VkInstanceCreateInfo instance_create_info;
  memset( &instance_create_info, 0u, sizeof( instance_create_info ) );
  instance_create_info.sType                    = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
  instance_create_info.pApplicationInfo         = &application;
  instance_create_info.flags                    = 0u;
  instance_create_info.ppEnabledExtensionNames  = instance_enabled_extensions;
  instance_create_info.enabledExtensionCount    = instance_enabled_extensions_count;
  instance_create_info.ppEnabledLayerNames     = instance_enabled_layers;
  instance_create_info.enabledLayerCount       = ARRAY_SIZE( instance_enabled_layers );
#ifdef VK_EXT_debug_utils
  VkDebugUtilsMessengerCreateInfoEXT debug_create_info;
  memset( &debug_create_info, 0u, sizeof( debug_create_info ) );
  debug_create_info.sType            = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
  debug_create_info.pNext            = NULL;
  debug_create_info.flags            = 0u;
  debug_create_info.pfnUserCallback  = debug_callback;
  debug_create_info.pUserData        = NULL;
  debug_create_info.messageSeverity =
    VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
    VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT |
    VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
    VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
  debug_create_info.messageType =
    VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
    VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
    VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
  instance_create_info.pNext = &debug_create_info;
#else // VK_EXT_debug_utils
  instance_create_info.pNext = nullptr;
#endif // VK_EXT_debug_utils
  
  VkInstance handle;
  HANDLE_VULKAN_RESULT( vkCreateInstance( &instance_create_info, vk_allocation_callbacks, &handle ), "failed to create instance" );

  arrfree( instance_enabled_extensions );

  return handle;
}


static VkDebugUtilsMessengerEXT create_debug_utils_messsenger(
  _In_     VkInstance              instance,
  _In_opt_ VkAllocationCallbacks  *allocation_callbacks )
{
  VkDebugUtilsMessengerCreateInfoEXT create_info = {
    .sType            = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT,
    .pfnUserCallback  = debug_callback,
    .messageSeverity  =
      VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
      VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT |
      VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
      VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT,
    .messageType      =
      VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
      VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
      VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT,
    .pUserData        = NULL,
    .pNext            = NULL,
    .flags            = 0u,
  };

  VkDebugUtilsMessengerEXT handle;
  PFN_vkCreateDebugUtilsMessengerEXT vkCreateDebugUtilsMessengerEXT = vkGetInstanceProcAddr( instance, "vkCreateDebugUtilsMessengerEXT" );  
  HANDLE_VULKAN_RESULT( vkCreateDebugUtilsMessengerEXT( instance, &create_info, allocation_callbacks, &handle ), "failed to create debug utils messenger" );
  return handle;
}

static VkSurfaceKHR create_surface(
  _In_     SDL_Window             *sdl_window,
  _In_     VkInstance              vk_instance,
  _In_opt_ VkAllocationCallbacks  *vk_allocation_callbacks )
{
  VkSurfaceKHR handle;
  if ( !SDL_Vulkan_CreateSurface( sdl_window, vk_instance, vk_allocation_callbacks, &handle ) )
  {
    CRUDE_ABORT( CRUDE_CHANNEL_GRAPHICS, "failed to create vk_surface: %s", SDL_GetError() );
    return VK_NULL_HANDLE;
  }
  return handle;
}

static int32 get_supported_queue_family_index(
  _In_ VkPhysicalDevice  vk_physical_device,
  _In_ VkSurfaceKHR      vk_surface )
{
  uint32 queue_family_count = 0u;
  vkGetPhysicalDeviceQueueFamilyProperties( vk_physical_device, &queue_family_count, NULL );
  if ( queue_family_count == 0u )
  {
    return -1;
  }
  
  VkQueueFamilyProperties *queue_families_properties = NULL;
  arrsetlen( queue_families_properties, queue_family_count ); // tofree
  vkGetPhysicalDeviceQueueFamilyProperties( vk_physical_device, &queue_family_count, queue_families_properties );
  
  int32 queue_index = -1;
  for ( uint32 i = 0; i < queue_family_count; ++i )
  {
    if ( queue_families_properties[i].queueCount > 0 && queue_families_properties[i].queueFlags & ( VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT ) )
    {
      VkBool32 surface_supported = false;
      vkGetPhysicalDeviceSurfaceSupportKHR( vk_physical_device, i, vk_surface, &surface_supported );
      if ( surface_supported )
      {
        queue_index = i;
        break;
      }
    }
  }
  arrfree( queue_families_properties );
  return queue_index;
}

static bool check_support_required_extensions(
  _In_ VkPhysicalDevice vk_physical_device )
{
  uint32 available_extensions_count = 0u;
  vkEnumerateDeviceExtensionProperties( vk_physical_device, NULL, &available_extensions_count, NULL );
  if ( available_extensions_count == 0u)
  {
    return false;
  }
    
  VkExtensionProperties *available_extensions = NULL;
  arrsetlen( available_extensions, available_extensions_count ); // tofree
  vkEnumerateDeviceExtensionProperties( vk_physical_device, NULL, &available_extensions_count, available_extensions );

  bool support_required_extensions = true;
  for ( uint32 i = 0; i < ARRAY_SIZE( vk_device_required_extensions ); ++i )
  {
    bool extension_found = false;
    for ( uint32 k = 0; k < available_extensions_count; ++k )
    {
      if ( strcmp( vk_device_required_extensions[i], available_extensions[k].extensionName ) == 0 )
      {
        extension_found = true;
        break;
      }
    }
    if ( !extension_found )
    {
      support_required_extensions = false;
      break;
    }
  }

  arrfree( available_extensions );
  return support_required_extensions;
}

static bool check_swap_chain_adequate(
  _In_ VkPhysicalDevice  vk_physical_device,
  _In_ VkSurfaceKHR      vk_surface )
{
  uint32 formats_count;
  vkGetPhysicalDeviceSurfaceFormatsKHR( vk_physical_device, vk_surface, &formats_count, NULL );
  if ( formats_count == 0u )
  {
    return false;
  }

  uint32 presents_mode_count;
  vkGetPhysicalDeviceSurfacePresentModesKHR( vk_physical_device, vk_surface, &presents_mode_count, NULL );
  if ( presents_mode_count == 0u ) 
  {
    return false;
  }

  return true;
}

static bool check_support_required_features(
  _In_ VkPhysicalDevice vk_physical_device )
{
  VkPhysicalDeviceFeatures features;
  vkGetPhysicalDeviceFeatures( vk_physical_device, &features );
  return features.samplerAnisotropy;
}

static VkPhysicalDevice pick_physical_device(
  _In_  VkInstance     vk_instance,
  _In_  VkSurfaceKHR   vk_surface,
  _Out_ int32         *vulkan_selected_queue_family_index )
{
  CRUDE_ASSERT( vulkan_selected_queue_family_index );

  uint32 physical_devices_count = 0u;
  vkEnumeratePhysicalDevices( vk_instance, &physical_devices_count, NULL );

  if ( physical_devices_count == 0u ) 
  {
    return VK_NULL_HANDLE;
  }

  VkPhysicalDevice *physical_devices = NULL;
  arrput( physical_devices, physical_devices_count ); // tofree
  vkEnumeratePhysicalDevices( vk_instance, &physical_devices_count, physical_devices );

  VkPhysicalDevice selected_physical_devices = VK_NULL_HANDLE;
  for ( uint32 physical_device_index = 0; physical_device_index < physical_devices_count; ++physical_device_index )
  {
    VkPhysicalDevice physical_device = physical_devices[physical_device_index];
    
    if ( !check_support_required_extensions( physical_device ) )
    {
      continue;
    }
    if ( !check_swap_chain_adequate( physical_device, vk_surface ) )
    {
      continue;
    }
    if ( !check_support_required_features( physical_device ) )
    {
      continue;
    }
    int32 queue_family_index = get_supported_queue_family_index( physical_device, vk_surface ); 
    if ( queue_family_index == -1 )
    {
      continue;
    }
    
    *vulkan_selected_queue_family_index = queue_family_index;
    selected_physical_devices = physical_device;
    break;
  }
  
  VkPhysicalDeviceProperties properties;
  vkGetPhysicalDeviceProperties( selected_physical_devices, &properties );
  CRUDE_LOG_INFO( CRUDE_CHANNEL_GRAPHICS, "Selected physical device %s %i", properties.deviceName, properties.deviceType );

  return selected_physical_devices;
}

static VkDevice create_device(
  _In_     VkPhysicalDevice       vk_physical_device,
  _In_     int32                  vk_queue_family_index,
  _In_opt_ VkAllocationCallbacks *vk_allocation_callbacks )
{
  float const queue_priority[] = { 1.0f };
  VkDeviceQueueCreateInfo queue_info[1];
  memset( queue_info, 0, sizeof( queue_info) );
  queue_info[0].sType            = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
  queue_info[0].queueFamilyIndex = vk_queue_family_index;
  queue_info[0].queueCount       = 1;
  queue_info[0].pQueuePriorities = queue_priority;
 
  VkPhysicalDeviceFeatures2 physical_features2 = { VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2 };
  vkGetPhysicalDeviceFeatures2( vk_physical_device, &physical_features2 );

  VkDeviceCreateInfo device_create_info = {
    .sType                    = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
    .pNext                    = &physical_features2,
    .flags                    = 0u,
    .queueCreateInfoCount     = ARRAY_SIZE( queue_info ),
    .pQueueCreateInfos        = queue_info,
    .pEnabledFeatures         = NULL,
    .enabledExtensionCount    = ARRAY_SIZE( vk_device_required_extensions ),
    .ppEnabledExtensionNames  = vk_device_required_extensions,
    .enabledLayerCount        = ARRAY_SIZE( vk_required_layers ),
    .ppEnabledLayerNames      = vk_required_layers,
  };
  VkDevice device;
  HANDLE_VULKAN_RESULT( vkCreateDevice( vk_physical_device, &device_create_info, vk_allocation_callbacks, &device ), "failed to create logic device!" );
  return device;
}

static VkSwapchainKHR create_swapchain( 
  _In_      VkDevice                 vk_device, 
  _In_      VkPhysicalDevice         vk_physical_device, 
  _In_      VkSurfaceKHR             vk_surface, 
  _In_      int32                    vk_queue_family_index,
  _In_opt_  VkAllocationCallbacks   *vk_allocation_callbacks,
  _Out_     uint32                  *vk_swapchain_images_count,
  _Out_     VkImage                 *vk_swapchain_images,
  _Out_     VkImageView             *vk_swapchain_images_views )
{
  VkSurfaceCapabilitiesKHR surface_capabilities;
  vkGetPhysicalDeviceSurfaceCapabilitiesKHR( vk_physical_device, vk_surface, &surface_capabilities);
  
  VkExtent2D swapchain_extent = surface_capabilities.currentExtent;
  if ( swapchain_extent.width == UINT32_MAX )
  {
    swapchain_extent.width = CLAMP( swapchain_extent.width, surface_capabilities.minImageExtent.width, surface_capabilities.maxImageExtent.width );
    swapchain_extent.height = CLAMP( swapchain_extent.height, surface_capabilities.minImageExtent.height, surface_capabilities.maxImageExtent.height );
  }
  
  uint32 available_formats_count;
  vkGetPhysicalDeviceSurfaceFormatsKHR( vk_physical_device, vk_surface, &available_formats_count, NULL );

  if ( available_formats_count == 0u )
  {
    CRUDE_LOG_ERROR( CRUDE_CHANNEL_GRAPHICS, "Can't find available surface format" );
    return VK_NULL_HANDLE;
  }

  VkSurfaceFormatKHR *available_formats = NULL;
  arrsetlen( available_formats, available_formats_count ); // tofree
  vkGetPhysicalDeviceSurfaceFormatsKHR( vk_physical_device, vk_surface, &available_formats_count, available_formats );

  bool surface_format_found = false;
  VkSurfaceFormatKHR vulkan_surface_format;
  for ( uint32 i = 0; i < available_formats_count; ++i )
  {
    if ( available_formats[i].format == VK_FORMAT_B8G8R8A8_SRGB && available_formats[i].colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR )
    {
      vulkan_surface_format = available_formats[i];
      surface_format_found = true;
    }
  }

  if ( !surface_format_found )
  {
    CRUDE_LOG_ERROR( CRUDE_CHANNEL_GRAPHICS, "Can't find available surface format" );
    arrfree( available_formats );
    return VK_NULL_HANDLE;
  }
  
  uint32 available_present_modes_count;
  vkGetPhysicalDeviceSurfacePresentModesKHR( vk_physical_device, vk_surface, &available_present_modes_count, NULL );
  if ( available_present_modes_count == 0u ) 
  {
    CRUDE_LOG_ERROR( CRUDE_CHANNEL_GRAPHICS, "Can't find available surface present_mode" );
    arrfree( available_formats );
    return VK_NULL_HANDLE;
  }
  
  VkPresentModeKHR *available_present_modes = NULL;
  arrsetlen( available_present_modes, available_present_modes_count ); // tofree
  vkGetPhysicalDeviceSurfacePresentModesKHR( vk_physical_device, vk_surface, &available_present_modes_count, available_present_modes );
  
  VkPresentModeKHR surface_present_mode = VK_PRESENT_MODE_FIFO_KHR;
  for ( uint32 i = 0; i < available_present_modes_count; ++i )
  {
    if ( available_present_modes[i] == VK_PRESENT_MODE_MAILBOX_KHR )
    {
      surface_present_mode = VK_PRESENT_MODE_MAILBOX_KHR;
      break;
    }
  }

  uint32 const image_count = ( surface_present_mode == VK_PRESENT_MODE_IMMEDIATE_KHR ? 2 : 3 );
  uint32 const queue_family_indices[] = { vk_queue_family_index };
  
  VkSwapchainCreateInfoKHR swapchain_create_info = ( VkSwapchainCreateInfoKHR ) {
    .sType                  = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
    .pNext                  = NULL,
    .surface                = vk_surface,
    .minImageCount          = image_count,
    .imageFormat            = vulkan_surface_format.format,
    .imageColorSpace        = vulkan_surface_format.colorSpace,
    .imageExtent            = swapchain_extent,
    .imageArrayLayers       = 1,
    .imageUsage             = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
    .imageSharingMode       = ARRAY_SIZE( queue_family_indices ) > 1 ? VK_SHARING_MODE_CONCURRENT : VK_SHARING_MODE_EXCLUSIVE,
    .queueFamilyIndexCount  = ARRAY_SIZE( queue_family_indices ),
    .pQueueFamilyIndices    = queue_family_indices,
    .preTransform           = surface_capabilities.currentTransform,
    .compositeAlpha         = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
    .presentMode            = surface_present_mode,
    .clipped                = true,
    .oldSwapchain           = VK_NULL_HANDLE,
  };
  
  VkSwapchainKHR vulkan_swapchain = VK_NULL_HANDLE;
  HANDLE_VULKAN_RESULT( vkCreateSwapchainKHR( vk_device, &swapchain_create_info, vk_allocation_callbacks, &vulkan_swapchain ), "failed to create swapchain!" );

  vkGetSwapchainImagesKHR( vk_device, vulkan_swapchain, vk_swapchain_images_count, NULL );
  vkGetSwapchainImagesKHR( vk_device, vulkan_swapchain, vk_swapchain_images_count, vk_swapchain_images );
  
  for ( uint32 i = 0; i < *vk_swapchain_images_count; ++i )
  {
    VkImageViewCreateInfo image_view_info = { 
      .sType                       = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
      .viewType                    = VK_IMAGE_VIEW_TYPE_2D,
      .format                      = vulkan_surface_format.format,
      .image                       = vk_swapchain_images[ i ],
      .subresourceRange.levelCount = 1,
      .subresourceRange.layerCount = 1,
      .subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
      .components.r                = VK_COMPONENT_SWIZZLE_R,
      .components.g                = VK_COMPONENT_SWIZZLE_G,
      .components.b                = VK_COMPONENT_SWIZZLE_B,
      .components.a                = VK_COMPONENT_SWIZZLE_A,
    };
    
    HANDLE_VULKAN_RESULT( vkCreateImageView( vk_device, &image_view_info, vk_allocation_callbacks, &vk_swapchain_images_views[ i ] ), "Failed to create image view for swapchain image" );
  }

  arrfree( available_formats );
  arrfree( available_present_modes );

  return vulkan_swapchain;
}

static VmaAllocation create_vma_allocator(
  _In_ VkDevice          vk_device,
  _In_ VkPhysicalDevice  vk_physical_device,
  _In_ VkInstance        vk_instance )
{
  VmaAllocatorCreateInfo allocator_info = {
    .physicalDevice   = vk_physical_device,
    .device           = vk_device,
    .instance         = vk_instance,
  };

  VmaAllocation vma_allocator;
  HANDLE_VULKAN_RESULT( vmaCreateAllocator( &allocator_info, &vma_allocator ), "Failed to create vma allocator" );
  return vma_allocator;
}

static VkDescriptorPool create_descriptor_pool(
  _In_     VkDevice               vk_device,
  _In_opt_ VkAllocationCallbacks *vk_allocation_callbacks )
{
  uint32 const global_pool_elements = 128;
  VkDescriptorPoolSize pool_sizes[] =
  {
    { VK_DESCRIPTOR_TYPE_SAMPLER, global_pool_elements },
    { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, global_pool_elements },
    { VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, global_pool_elements },
    { VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, global_pool_elements },
    { VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, global_pool_elements },
    { VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, global_pool_elements },
    { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, global_pool_elements },
    { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, global_pool_elements },
    { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, global_pool_elements },
    { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, global_pool_elements },
    { VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, global_pool_elements }
  };
  
  VkDescriptorPoolCreateInfo pool_info = {
    .sType         = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
    .flags         = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT,
    .maxSets       = global_pool_elements * ARRAY_SIZE( pool_sizes ),
    .poolSizeCount = ARRAY_SIZE( pool_sizes ),
    .pPoolSizes    = pool_sizes,
  };

  VkDescriptorPool vulkan_descriptor_pool;
  HANDLE_VULKAN_RESULT( vkCreateDescriptorPool( vk_device, &pool_info, vk_allocation_callbacks, &vulkan_descriptor_pool ), "Failed create descriptor pool" );
  return vulkan_descriptor_pool;
}

VkQueryPool create_timestamp_query_pool(
  _In_     VkDevice               vk_device, 
  _In_     int32                  max_frames,
  _In_opt_ VkAllocationCallbacks *vk_allocation_callbacks )
{    
  uint32 const gpu_time_queries_per_frame = 32;
  VkQueryPoolCreateInfo query_pool_create_info = { 
    .sType = VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO,
    .pNext = NULL,
    .flags = 0u,
    .queryType = VK_QUERY_TYPE_TIMESTAMP,
    .queryCount = gpu_time_queries_per_frame * 2u * max_frames,
    .pipelineStatistics = 0u,
  };
  VkQueryPool vulkan_timestamp_query_pool;
  HANDLE_VULKAN_RESULT( vkCreateQueryPool( vk_device, &query_pool_create_info, vk_allocation_callbacks, &vulkan_timestamp_query_pool ), "Failed to create query pool" );
  return vulkan_timestamp_query_pool;
}

static void destroy_swapchain(
  _In_     VkDevice                 vk_device,
  _In_     VkSwapchainKHR           vulkan_swapchain,
  _In_     uint32                   vk_swapchain_images_count,
  _In_     VkImageView             *vk_swapchain_images_views,
  _In_opt_ VkAllocationCallbacks   *vk_allocation_callbacks )
{
  for ( uint32 i = 0; i < vk_swapchain_images_count; ++i )
  {
    vkDestroyImageView( vk_device, vk_swapchain_images_views[ i ], vk_allocation_callbacks );
  }
 
  vkDestroySwapchainKHR( vk_device, vulkan_swapchain, vk_allocation_callbacks );
}

void crude_initialize_gpu_device(
  _In_ crude_gpu_device           *gpu,
  _In_ crude_gpu_device_creation  *creation )
{
  gpu->allocator  = creation->allocator;
  gpu->vk_allocation_callbacks = NULL;
  gpu->max_frames = creation->max_frames;
  gpu->vk_instance = create_instance( creation->vk_application_name, creation->vk_application_version, gpu->vk_allocation_callbacks );
  gpu->vk_debug_utils_messenger = create_debug_utils_messsenger( gpu->vk_instance, gpu->vk_allocation_callbacks );
  gpu->vk_surface = create_surface( creation->sdl_window, gpu->vk_instance, gpu->vk_allocation_callbacks );
  gpu->vk_physical_device = pick_physical_device( gpu->vk_instance, gpu->vk_surface, &gpu->vk_queue_family_index );
  gpu->vk_device = create_device( gpu->vk_physical_device, gpu->vk_queue_family_index, gpu->vk_allocation_callbacks );
  vkGetDeviceQueue( gpu->vk_device, gpu->vk_queue_family_index, 0u, &gpu->vk_queue );
  gpu->vk_swapchain = create_swapchain( gpu->vk_device, gpu->vk_physical_device, gpu->vk_surface, gpu->vk_queue_family_index, gpu->vk_allocation_callbacks, &gpu->vk_swapchain_images_count, gpu->vk_swapchain_images, gpu->vk_swapchain_images_views );
  gpu->vma_allocator = create_vma_allocator( gpu->vk_device, gpu->vk_physical_device, gpu->vk_instance );
  gpu->vk_descriptor_pool = create_descriptor_pool( gpu->vk_device, gpu->vk_allocation_callbacks );
  gpu->vk_timestamp_query_pool = create_timestamp_query_pool( gpu->vk_device, gpu->max_frames, gpu->vk_allocation_callbacks );
  crude_initialize_resource_pool( &gpu->buffers, gpu->allocator, 4096, sizeof( crude_buffer ) );
  crude_initialize_resource_pool( &gpu->textures, gpu->allocator, 512, sizeof( crude_texture ) );
  crude_initialize_resource_pool( &gpu->render_passes, gpu->allocator, 256, sizeof( crude_render_pass ) );
  crude_initialize_resource_pool( &gpu->descriptor_set_layouts, gpu->allocator, 128, sizeof( crude_descriptor_set_layout ) );
  crude_initialize_resource_pool( &gpu->pipelines, gpu->allocator, 128, sizeof( crude_pipeline ) );
  crude_initialize_resource_pool( &gpu->shaders, gpu->allocator, 128, sizeof( crude_shader_state ) );
  crude_initialize_resource_pool( &gpu->descriptor_sets, gpu->allocator, 256, sizeof( crude_descriptor_set ) );
  crude_initialize_resource_pool( &gpu->samplers, gpu->allocator, 32, sizeof( crude_sampler ) );

    //uint8 *memory = NULL;
    //arrsetlen( sizeof( GPUTimestampManager ) + sizeof( CommandBuffer* ) * 128 ); // tofree

    //VkSemaphoreCreateInfo semaphore_info{ VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO };

    //for ( size_t i = 0; i < k_max_swapchain_images; i++ ) {

    //    vkCreateSemaphore( vk_device, &semaphore_info, vk_allocation_callbacks, &vulkan_render_complete_semaphore[ i ] );
    //    vkCreateSemaphore( vk_device, &semaphore_info, vk_allocation_callbacks, &vulkan_image_acquired_semaphore[ i ] );

    //    VkFenceCreateInfo fenceInfo{ VK_STRUCTURE_TYPE_FENCE_CREATE_INFO };
    //    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;
    //    vkCreateFence( vk_device, &fenceInfo, vk_allocation_callbacks, &vulkan_command_buffer_executed_fence[ i ] );
    //}

    //gpu_timestamp_manager = ( GPUTimestampManager* )( memory );
    //gpu_timestamp_manager->init( allocator, creation.gpu_time_queries_per_frame, k_max_frames );

    //command_buffer_ring.init( this );

    //// Allocate queued command buffers array
    //queued_command_buffers = ( CommandBuffer** )( gpu_timestamp_manager + 1 );
    //CommandBuffer** correctly_allocated_buffer = ( CommandBuffer** )( memory + sizeof( GPUTimestampManager ) );
    //RASSERTM( queued_command_buffers == correctly_allocated_buffer, "Wrong calculations for queued command buffers arrays. Should be %p, but it is %p.", correctly_allocated_buffer, queued_command_buffers );
 }

void crude_deinitialize_gpu_device( _In_ crude_gpu_device *gpu )
{
  crude_deinitialize_resource_pool( &gpu->buffers );
  crude_deinitialize_resource_pool( &gpu->textures );
  crude_deinitialize_resource_pool( &gpu->render_passes );
  crude_deinitialize_resource_pool( &gpu->descriptor_set_layouts );
  crude_deinitialize_resource_pool( &gpu->pipelines );
  crude_deinitialize_resource_pool( &gpu->shaders );
  crude_deinitialize_resource_pool( &gpu->descriptor_sets );
  crude_deinitialize_resource_pool( &gpu->samplers );

  vkDestroyQueryPool( gpu->vk_device, gpu->vk_timestamp_query_pool, gpu->vk_allocation_callbacks );
  vkDestroyDescriptorPool( gpu->vk_device, gpu->vk_descriptor_pool, gpu->vk_allocation_callbacks );
  vmaDestroyAllocator( gpu->vma_allocator );
  destroy_swapchain( gpu->vk_device, gpu->vk_swapchain, gpu->vk_swapchain_images_count, gpu->vk_swapchain_images_views, gpu->vk_allocation_callbacks );
  vkDestroySwapchainKHR( gpu->vk_device, gpu->vk_swapchain, gpu->vk_allocation_callbacks );
  vkDestroyDevice( gpu->vk_device, gpu->vk_allocation_callbacks );
  vkDestroySurfaceKHR( gpu->vk_instance, gpu->vk_surface, gpu->vk_allocation_callbacks );
  PFN_vkDestroyDebugUtilsMessengerEXT vkDestroyDebugUtilsMessengerEXT = vkGetInstanceProcAddr( gpu->vk_instance, "vkDestroyDebugUtilsMessengerEXT" );
  vkDestroyDebugUtilsMessengerEXT( gpu->vk_instance, gpu->vk_debug_utils_messenger, gpu->vk_allocation_callbacks );
  vkDestroyInstance( gpu->vk_instance, gpu->vk_allocation_callbacks );
}