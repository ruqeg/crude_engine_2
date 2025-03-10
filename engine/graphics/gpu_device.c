#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>
#include <stb_ds.h>
#include <math.h>

#include <core/assert.h>
#include <graphics/gpu_resources.h>
#include <graphics/command_buffer.h>

#include <graphics/gpu_device.h>

///////////////////////////////////
//@ Constants
///////////////////////////////////

static char const *const vk_device_required_extensions[] = 
{ 
  VK_KHR_SWAPCHAIN_EXTENSION_NAME
};

static char const *const *vk_instance_required_extensions[] =
{
  VK_EXT_DEBUG_UTILS_EXTENSION_NAME
};

static char const *const *vk_required_layers[] =
{
  "VK_LAYER_KHRONOS_validation"
};

///////////////////////////////////
//@ Global
///////////////////////////////////
static crude_command_buffer_manager g_command_buffer_manager;

///////////////////////////////////
//@ Local vulkal utils
///////////////////////////////////
static VKAPI_ATTR VkBool32
_vk_debug_callback
(
  _In_ VkDebugUtilsMessageSeverityFlagBitsEXT      messageSeverity,
  _In_ VkDebugUtilsMessageTypeFlagsEXT             messageType,
  _In_ VkDebugUtilsMessengerCallbackDataEXT const *pCallbackData,
  _In_ void                                       *pUserData
)
{
  if ( messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT )
  {
    CRUDE_LOG_ERROR( CRUDE_CHANNEL_GRAPHICS, "validation layer: %s", pCallbackData->pMessage );
  }
  else if ( messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT )
  {
    CRUDE_LOG_WARNING( CRUDE_CHANNEL_GRAPHICS, "validation layer: %s", pCallbackData->pMessage );
  }
  //else if ( messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT )
  //{
  //  CRUDE_LOG_INFO( CRUDE_CHANNEL_GRAPHICS, "validation layer: %s", pCallbackData->pMessage );
  //}
  return VK_FALSE;
}

static void
_vk_transition_image_layout
(
  _In_ VkCommandBuffer  command_buffer,
  _In_ VkImage          image,
  _In_ VkFormat         format,
  _In_ VkImageLayout    old_layout,
  _In_ VkImageLayout    new_layout,
  _In_ bool             is_depth
)
{

  VkImageMemoryBarrier barrier = {
    .sType                           = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
    .oldLayout                       = old_layout,
    .newLayout                       = new_layout,
    .srcQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED,
    .dstQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED,
    .image                           = image,
    .subresourceRange.aspectMask     = is_depth ? VK_IMAGE_ASPECT_DEPTH_BIT : VK_IMAGE_ASPECT_COLOR_BIT,
    .subresourceRange.baseMipLevel   = 0,
    .subresourceRange.levelCount     = 1,
    .subresourceRange.baseArrayLayer = 0,
    .subresourceRange.layerCount     = 1,
  };

  VkPipelineStageFlags sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
  VkPipelineStageFlags destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
  
  if ( old_layout == VK_IMAGE_LAYOUT_UNDEFINED && new_layout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL )
  {
    barrier.srcAccessMask = 0;
    barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    
    sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
    destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
  }
  else if ( old_layout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && new_layout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL )
  {
    barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
    
    sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
    destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
  }
  else
  {
    //CRUDE_LOG_ERROR( CRUDE_CHANNEL_GRAPHICS, "Unsupported layout transition!" );
  }
  
  vkCmdPipelineBarrier( command_buffer, sourceStage, destinationStage, 0, 0, NULL, 0, NULL, 1, &barrier );
}

static VkInstance
_vk_create_instance
(
  _In_     char const            *vk_application_name,
  _In_     uint32                 vk_application_version,
  _In_opt_ VkAllocationCallbacks *vk_allocation_callbacks
)
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
  debug_create_info.pfnUserCallback  = _vk_debug_callback;
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
  CRUDE_GFX_HANDLE_VULKAN_RESULT( vkCreateInstance( &instance_create_info, vk_allocation_callbacks, &handle ), "failed to create instance" );

  arrfree( instance_enabled_extensions );

  return handle;
}

static VkDebugUtilsMessengerEXT
_vk_create_debug_utils_messsenger
(
  _In_     VkInstance              instance,
  _In_opt_ VkAllocationCallbacks  *allocation_callbacks
)
{
  VkDebugUtilsMessengerCreateInfoEXT create_info = {
    .sType            = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT,
    .pfnUserCallback  = _vk_debug_callback,
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
  CRUDE_GFX_HANDLE_VULKAN_RESULT( vkCreateDebugUtilsMessengerEXT( instance, &create_info, allocation_callbacks, &handle ), "failed to create debug utils messenger" );
  return handle;
}

static VkSurfaceKHR
_vk_create_surface
(
  _In_     SDL_Window             *sdl_window,
  _In_     VkInstance              vk_instance,
  _In_opt_ VkAllocationCallbacks  *vk_allocation_callbacks
)
{
  VkSurfaceKHR handle;
  if ( !SDL_Vulkan_CreateSurface( sdl_window, vk_instance, vk_allocation_callbacks, &handle ) )
  {
    CRUDE_ABORT( CRUDE_CHANNEL_GRAPHICS, "failed to create vk_surface: %s", SDL_GetError() );
    return VK_NULL_HANDLE;
  }
  return handle;
}

static int32
_vk_get_supported_queue_family_index
(
  _In_ VkPhysicalDevice  vk_physical_device,
  _In_ VkSurfaceKHR      vk_surface
)
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

static bool
_vk_check_support_required_extensions
(
  _In_ VkPhysicalDevice vk_physical_device
)
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

static bool
_vk_check_swap_chain_adequate
(
  _In_ VkPhysicalDevice  vk_physical_device,
  _In_ VkSurfaceKHR      vk_surface
)
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

static bool
_vk_check_support_required_features
(
  _In_ VkPhysicalDevice vk_physical_device
)
{
  VkPhysicalDeviceFeatures features;
  vkGetPhysicalDeviceFeatures( vk_physical_device, &features );
  return features.samplerAnisotropy;
}

static VkPhysicalDevice
_vk_pick_physical_device
(
  _In_  VkInstance     vk_instance,
  _In_  VkSurfaceKHR   vk_surface,
  _Out_ int32         *vulkan_selected_queue_family_index
)
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
    
    if ( !_vk_check_support_required_extensions( physical_device ) )
    {
      continue;
    }
    if ( !_vk_check_swap_chain_adequate( physical_device, vk_surface ) )
    {
      continue;
    }
    if ( !_vk_check_support_required_features( physical_device ) )
    {
      continue;
    }
    int32 queue_family_index = _vk_get_supported_queue_family_index( physical_device, vk_surface ); 
    if ( queue_family_index == -1 )
    {
      continue;
    }
    
    *vulkan_selected_queue_family_index = queue_family_index;
    selected_physical_devices = physical_device;
    break;
  }
  
  if ( selected_physical_devices == VK_NULL_HANDLE )
  {
    CRUDE_LOG_INFO( CRUDE_CHANNEL_GRAPHICS, "Can't find suitable physical device! physical_devices_count: %i", physical_devices_count );
    return VK_NULL_HANDLE;
  }
  VkPhysicalDeviceProperties properties;
  vkGetPhysicalDeviceProperties( selected_physical_devices, &properties );
  CRUDE_LOG_INFO( CRUDE_CHANNEL_GRAPHICS, "Selected physical device %s %i", properties.deviceName, properties.deviceType );

  return selected_physical_devices;
}

static VkDevice
_vk_create_device
(
  _In_     VkPhysicalDevice       vk_physical_device,
  _In_     int32                  vk_queue_family_index,
  _In_opt_ VkAllocationCallbacks *vk_allocation_callbacks
)
{
  float const queue_priority[] = { 1.0f };
  VkDeviceQueueCreateInfo queue_info[ 1 ];
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
  CRUDE_GFX_HANDLE_VULKAN_RESULT( vkCreateDevice( vk_physical_device, &device_create_info, vk_allocation_callbacks, &device ), "failed to create logic device!" );
  return device;
}

static VkSwapchainKHR
_vk_create_swapchain
( 
  _In_      VkDevice                 vk_device, 
  _In_      VkPhysicalDevice         vk_physical_device, 
  _In_      VkSurfaceKHR             vk_surface, 
  _In_      int32                    vk_queue_family_index,
  _In_opt_  VkAllocationCallbacks   *vk_allocation_callbacks,
  _Out_     uint32                  *vk_swapchain_images_count,
  _Out_     VkImage                 *vk_swapchain_images,
  _Out_     VkImageView             *vk_swapchain_images_views,
  _Out_     VkSurfaceFormatKHR      *vk_surface_format,
  _Out_     uint16                  *vk_swapchain_width,
  _Out_     uint16                  *vk_swapchain_height
)
{
  VkSurfaceCapabilitiesKHR surface_capabilities;
  vkGetPhysicalDeviceSurfaceCapabilitiesKHR( vk_physical_device, vk_surface, &surface_capabilities);
  
  VkExtent2D swapchain_extent = surface_capabilities.currentExtent;
  if ( swapchain_extent.width == UINT32_MAX )
  {
    swapchain_extent.width = CLAMP( swapchain_extent.width, surface_capabilities.minImageExtent.width, surface_capabilities.maxImageExtent.width );
    swapchain_extent.height = CLAMP( swapchain_extent.height, surface_capabilities.minImageExtent.height, surface_capabilities.maxImageExtent.height );
  }

  *vk_swapchain_width  = swapchain_extent.width;
  *vk_swapchain_height = swapchain_extent.height;
  
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
  for ( uint32 i = 0; i < available_formats_count; ++i )
  {
    if ( available_formats[i].format == VK_FORMAT_B8G8R8A8_SRGB && available_formats[i].colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR )
    {
      *vk_surface_format = available_formats[i];
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
    .imageFormat            = vk_surface_format->format,
    .imageColorSpace        = vk_surface_format->colorSpace,
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
  CRUDE_GFX_HANDLE_VULKAN_RESULT( vkCreateSwapchainKHR( vk_device, &swapchain_create_info, vk_allocation_callbacks, &vulkan_swapchain ), "failed to create swapchain!" );

  vkGetSwapchainImagesKHR( vk_device, vulkan_swapchain, vk_swapchain_images_count, NULL );
  vkGetSwapchainImagesKHR( vk_device, vulkan_swapchain, vk_swapchain_images_count, vk_swapchain_images );
  
  for ( uint32 i = 0; i < *vk_swapchain_images_count; ++i )
  {
    VkImageViewCreateInfo image_view_info = { 
      .sType                       = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
      .viewType                    = VK_IMAGE_VIEW_TYPE_2D,
      .format                      = vk_surface_format->format,
      .image                       = vk_swapchain_images[ i ],
      .subresourceRange.levelCount = 1,
      .subresourceRange.layerCount = 1,
      .subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
      .components.r                = VK_COMPONENT_SWIZZLE_R,
      .components.g                = VK_COMPONENT_SWIZZLE_G,
      .components.b                = VK_COMPONENT_SWIZZLE_B,
      .components.a                = VK_COMPONENT_SWIZZLE_A,
    };
    
    CRUDE_GFX_HANDLE_VULKAN_RESULT( vkCreateImageView( vk_device, &image_view_info, vk_allocation_callbacks, &vk_swapchain_images_views[ i ] ), "Failed to create image view for swapchain image" );
  }

  arrfree( available_formats );
  arrfree( available_present_modes );

  return vulkan_swapchain;
}

static VmaAllocation
_vk_create_vma_allocator
(
  _In_ VkDevice          vk_device,
  _In_ VkPhysicalDevice  vk_physical_device,
  _In_ VkInstance        vk_instance
)
{
  VmaAllocatorCreateInfo allocator_info = {
    .physicalDevice   = vk_physical_device,
    .device           = vk_device,
    .instance         = vk_instance,
  };

  VmaAllocation vma_allocator;
  CRUDE_GFX_HANDLE_VULKAN_RESULT( vmaCreateAllocator( &allocator_info, &vma_allocator ), "Failed to create vma allocator" );
  return vma_allocator;
}

static VkDescriptorPool
_vk_create_descriptor_pool
(
  _In_     VkDevice               vk_device,
  _In_opt_ VkAllocationCallbacks *vk_allocation_callbacks
)
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
  CRUDE_GFX_HANDLE_VULKAN_RESULT( vkCreateDescriptorPool( vk_device, &pool_info, vk_allocation_callbacks, &vulkan_descriptor_pool ), "Failed create descriptor pool" );
  return vulkan_descriptor_pool;
}

static VkQueryPool
_vk_create_timestamp_query_pool
(
  _In_     VkDevice               vk_device, 
  _In_     int32                  max_frames,
  _In_opt_ VkAllocationCallbacks *vk_allocation_callbacks
)
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
  CRUDE_GFX_HANDLE_VULKAN_RESULT( vkCreateQueryPool( vk_device, &query_pool_create_info, vk_allocation_callbacks, &vulkan_timestamp_query_pool ), "Failed to create query pool" );
  return vulkan_timestamp_query_pool;
}

static void
_vk_create_swapchain_pass
(
  _In_ crude_gpu_device             *gpu,
  _In_ crude_render_pass_creation   *creation,
  _Out_ crude_render_pass           *render_pass
)
{
  VkAttachmentDescription color_attachment = {
    .format         = gpu->vk_surface_format.format,
    .samples        = VK_SAMPLE_COUNT_1_BIT,
    .loadOp         = VK_ATTACHMENT_LOAD_OP_CLEAR,
    .storeOp        = VK_ATTACHMENT_STORE_OP_STORE,
    .stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
    .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
    .initialLayout  = VK_IMAGE_LAYOUT_UNDEFINED,
    .finalLayout    = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
  };
  
  VkAttachmentReference color_attachment_ref = {
    .attachment     = 0,
    .layout         = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, 
  };
  
  crude_texture* depth_texture = crude_resource_pool_access_resource( &gpu->textures, gpu->depth_texture.index );
  VkAttachmentDescription depth_attachment = {
    .format         = depth_texture->vk_format,
    .samples        = VK_SAMPLE_COUNT_1_BIT,
    .loadOp         = VK_ATTACHMENT_LOAD_OP_CLEAR,
    .storeOp        = VK_ATTACHMENT_STORE_OP_DONT_CARE,
    .stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
    .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
    .initialLayout  = VK_IMAGE_LAYOUT_UNDEFINED,
    .finalLayout    = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
  };
  
  VkAttachmentReference depth_attachment_ref = {
    .attachment     = 1,
    .layout         = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
  };
  
  VkSubpassDescription subpass = {
    .pipelineBindPoint        = VK_PIPELINE_BIND_POINT_GRAPHICS,
    .colorAttachmentCount     = 1,
    .pColorAttachments        = &color_attachment_ref,
    .pDepthStencilAttachment  = &depth_attachment_ref,
  };
  
  VkAttachmentDescription attachments[] = { color_attachment, depth_attachment };
  VkRenderPassCreateInfo render_pass_info = { 
    .sType            = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
    .attachmentCount  = ARRAY_SIZE( attachments ),
    .pAttachments     = attachments,
    .subpassCount     = 1,
    .pSubpasses       = &subpass,
  };

  CRUDE_GFX_HANDLE_VULKAN_RESULT( vkCreateRenderPass( gpu->vk_device, &render_pass_info, NULL, &render_pass->vk_render_pass ), "Failed to create swapchain render pass" );

  crude_gfx_set_resource_name( gpu, VK_OBJECT_TYPE_RENDER_PASS, CAST( uint64, render_pass->vk_render_pass ), creation->name );
  
  VkImageView framebuffer_attachments[2];

  VkFramebufferCreateInfo framebuffer_info = {
    .sType            = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
    .renderPass       = render_pass->vk_render_pass,
    .attachmentCount  = ARRAY_SIZE( framebuffer_attachments ),
    .width            = gpu->vk_swapchain_width,
    .height           = gpu->vk_swapchain_height,
    .layers           = 1,
  };

  framebuffer_attachments[1] = depth_texture->vk_image_view;
  for ( uint32 i = 0; i < gpu->vk_swapchain_images_count; ++i )
  {
    framebuffer_attachments[0] = gpu->vk_swapchain_images_views[i];
    framebuffer_info.pAttachments = framebuffer_attachments;
    CRUDE_GFX_HANDLE_VULKAN_RESULT( vkCreateFramebuffer( gpu->vk_device, &framebuffer_info, NULL, &gpu->vk_swapchain_framebuffers[i] ), "Failed to create framebuffer" );
    crude_gfx_set_resource_name( gpu, VK_OBJECT_TYPE_FRAMEBUFFER, gpu->vk_swapchain_framebuffers[ i ], creation->name );
  }

  render_pass->width = gpu->vk_swapchain_width;
  render_pass->height = gpu->vk_swapchain_height;

  VkCommandBufferBeginInfo beginInfo = {
    .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
    .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
  };

  crude_command_buffer *command_buffer = crude_gfx_cmd_manager_get_cmd_buffer_instant( &g_command_buffer_manager, gpu->current_frame, false );
  vkBeginCommandBuffer( command_buffer->vk_handle, &beginInfo );
  for ( uint64 i = 0; i < gpu->vk_swapchain_images_count; ++i )
  {
    _vk_transition_image_layout( command_buffer->vk_handle, gpu->vk_swapchain_images[ i ], gpu->vk_surface_format.format, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR, false );
  }
  vkEndCommandBuffer( command_buffer->vk_handle );

  VkSubmitInfo submitInfo = { 
    .sType              = VK_STRUCTURE_TYPE_SUBMIT_INFO,
    .commandBufferCount = 1,
    .pCommandBuffers    = &command_buffer->vk_handle,
  };
  
  vkQueueSubmit( gpu->vk_queue, 1, &submitInfo, VK_NULL_HANDLE );
  vkQueueWaitIdle( gpu->vk_queue );
}

static void
_vk_destroy_swapchain
(
  _In_     VkDevice                 vk_device,
  _In_     VkSwapchainKHR           vulkan_swapchain,
  _In_     uint32                   vk_swapchain_images_count,
  _In_     VkImageView             *vk_swapchain_images_views,
  _In_     VkFramebuffer           *vk_swapchain_framebuffers,
  _In_opt_ VkAllocationCallbacks   *vk_allocation_callbacks
)
{
  for ( uint32 i = 0; i < vk_swapchain_images_count; ++i )
  {
    vkDestroyImageView( vk_device, vk_swapchain_images_views[ i ], vk_allocation_callbacks );
    vkDestroyFramebuffer( vk_device, vk_swapchain_framebuffers[ i ], vk_allocation_callbacks );
  }
  vkDestroySwapchainKHR( vk_device, vulkan_swapchain, vk_allocation_callbacks );
}

static void
_vk_create_texture
(
  _In_ crude_gpu_device             *gpu,
  _In_ crude_texture_creation const *creation,
  _In_ crude_texture_handle          handle,
  _In_ crude_texture                *texture
)
{
  texture->width          = creation->width;
  texture->height         = creation->height;
  texture->depth          = creation->depth;
  texture->mipmaps        = creation->mipmaps;
  texture->type           = creation->type;
  texture->name           = creation->name;
  texture->vk_format      = creation->format;
  texture->sampler        = NULL;
  texture->flags          = creation->flags;
  texture->handle         = handle;
  
  VkImageCreateInfo image_info = { 
    .sType          = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
    .format         = texture->vk_format,
    .flags          = 0,
    .imageType      = crude_to_vk_image_type( creation->type ),
    .extent.width   = creation->width,
    .extent.height  = creation->height,
    .extent.depth   = creation->depth,
    .mipLevels      = creation->mipmaps,
    .arrayLayers    = 1,
    .samples        = VK_SAMPLE_COUNT_1_BIT,
    .tiling         = VK_IMAGE_TILING_OPTIMAL,
  };
  
  bool const is_render_target = ( creation->flags & CRUDE_TEXTURE_MASK_RENDER_TARGET ) == CRUDE_TEXTURE_MASK_RENDER_TARGET;
  bool const is_compute_used = ( creation->flags & CRUDE_TEXTURE_MASK_COMPUTE ) == CRUDE_TEXTURE_MASK_COMPUTE;
  
  image_info.usage = VK_IMAGE_USAGE_SAMPLED_BIT;
  image_info.usage |= is_compute_used ? VK_IMAGE_USAGE_STORAGE_BIT : 0;
  
  if ( crude_has_depth_or_stencil( creation->format ) )
  {
    image_info.usage |= VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
  }
  else
  {
    image_info.usage |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    image_info.usage |= is_render_target ? VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT : 0;
  }
  
  image_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
  image_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  
  VmaAllocationCreateInfo memory_info = {
    .usage = VMA_MEMORY_USAGE_GPU_ONLY
  };
  
  CRUDE_GFX_HANDLE_VULKAN_RESULT( vmaCreateImage( gpu->vma_allocator, &image_info, &memory_info, &texture->vk_image, &texture->vma_allocation, NULL ), "Failed to create image!" );
  
  crude_gfx_set_resource_name( gpu, VK_OBJECT_TYPE_IMAGE, texture->vk_image, creation->name );
  
  VkImageViewCreateInfo info = {
    .sType    = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
    .image    = texture->vk_image,
    .viewType = crude_to_vk_image_view_type( creation->type ),
    .format   = image_info.format,
  };
  
  if ( crude_has_depth_or_stencil( creation->format ) )
  {
    info.subresourceRange.aspectMask = crude_has_depth( creation->format ) ? VK_IMAGE_ASPECT_DEPTH_BIT : 0;
  }
  else
  {
    info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  }
  info.subresourceRange.levelCount = 1;
  info.subresourceRange.layerCount = 1;
  CRUDE_GFX_HANDLE_VULKAN_RESULT( vkCreateImageView( gpu->vk_device, &info, gpu->vk_allocation_callbacks, &texture->vk_image_view ), "Failed to create image view" );
  
  crude_gfx_set_resource_name( gpu, VK_OBJECT_TYPE_IMAGE_VIEW, texture->vk_image_view, creation->name );
  
  texture->vk_image_layout = VK_IMAGE_LAYOUT_UNDEFINED;
}

static void
_vk_resize_texture
(
  _In_ crude_gpu_device *gpu,
  _In_ crude_texture    *texture,
  _In_ crude_texture    *texture_to_delete,
  _In_ uint16            width,
  _In_ uint16            height,
  _In_ uint16            depth
)
{
  texture_to_delete->vk_image_view = texture->vk_image_view;
  texture_to_delete->vk_image = texture->vk_image;
  texture_to_delete->vma_allocation = texture->vma_allocation;
  
  crude_texture_creation texture_creation = {
    .width    = width,
    .height   = height,
    .depth    = depth,
    .mipmaps  = texture->mipmaps,
    .flags    = texture->flags,
    .format   = texture->vk_format,
    .type     = texture->type,
    .name     = texture->name,
  };
  _vk_create_texture( gpu, &texture_creation, texture->handle, texture );
}

static void
_vk_resize_swapchain
(
  _In_ crude_gpu_device *gpu
)
{
  vkDeviceWaitIdle( gpu->vk_device );

  VkSurfaceCapabilitiesKHR surface_capabilities;
  vkGetPhysicalDeviceSurfaceCapabilitiesKHR( gpu->vk_physical_device, gpu->vk_surface, &surface_capabilities );
  VkExtent2D swapchain_extent = surface_capabilities.currentExtent;
  
  if ( swapchain_extent.width == 0 || swapchain_extent.height == 0 )
  {
    return;
  }

  crude_render_pass* swapchain_pass = CRUDE_GFX_GPU_ACCESS_RENDER_PASS( gpu, gpu->swapchain_pass );
  vkDestroyRenderPass( gpu->vk_device, swapchain_pass->vk_render_pass, gpu->vk_allocation_callbacks );
  
  _vk_destroy_swapchain( gpu->vk_device, gpu->vk_swapchain, gpu->vk_swapchain_images_count, gpu->vk_swapchain_images_views, gpu->vk_swapchain_framebuffers, gpu->vk_allocation_callbacks );
  vkDestroySurfaceKHR( gpu->vk_instance, gpu->vk_surface, gpu->vk_allocation_callbacks );

  if ( !SDL_Vulkan_CreateSurface( gpu->sdl_window, gpu->vk_instance, gpu->vk_allocation_callbacks, &gpu->vk_surface ) )
  {
    CRUDE_ABORT( CRUDE_CHANNEL_GRAPHICS, "Failed to create vk_surface: %s!", SDL_GetError() );
  }
  
  gpu->vk_swapchain = _vk_create_swapchain( gpu->vk_device, gpu->vk_physical_device, gpu->vk_surface, gpu->vk_queue_family_index, gpu->vk_allocation_callbacks, &gpu->vk_swapchain_images_count, gpu->vk_swapchain_images, gpu->vk_swapchain_images_views, &gpu->vk_surface_format, &gpu->vk_swapchain_width, &gpu->vk_swapchain_height);
  
  crude_texture_handle texture_to_delete_handle = { CRUDE_GFX_GPU_OBTAIN_TEXTURE( gpu ) };
  crude_texture *texture_to_delete = CRUDE_GFX_GPU_ACCESS_TEXTURE( gpu, texture_to_delete_handle );
  texture_to_delete->handle = texture_to_delete_handle;

  crude_texture *depth_texture = CRUDE_GFX_GPU_ACCESS_TEXTURE( gpu, gpu->depth_texture );
  _vk_resize_texture( gpu, depth_texture, texture_to_delete, gpu->vk_swapchain_width, gpu->vk_swapchain_height, 1 );
  crude_gfx_destroy_texture( gpu, texture_to_delete_handle );
  
  crude_render_pass_creation swapchain_pass_creation = {
    .type                  = CRUDE_RENDER_PASS_TYPE_SWAPCHAIN,
    .name                  = "swapchain",
    .color_operation       = CRUDE_RENDER_PASS_OPERATION_CLEAR,
    .depth_operation       = CRUDE_RENDER_PASS_OPERATION_CLEAR,
    .depth_stencil_texture = CRUDE_RENDER_PASS_OPERATION_CLEAR,
    .scale_x               = 1.f,
    .scale_y               = 1.f,
    .resize                = 1,
  };
  _vk_create_swapchain_pass( gpu, &swapchain_pass_creation, swapchain_pass );
  
  vkDeviceWaitIdle( gpu->vk_device );
}

///////////////////
//@ Gpu device
///////////////////

void
crude_gfx_initialize_gpu_device
(
  _Out_ crude_gpu_device          *gpu,
  _In_ crude_gpu_device_creation  *creation
)
{
  gpu->sdl_window = creation->sdl_window;
  gpu->allocator  = creation->allocator;
  gpu->vk_allocation_callbacks = NULL;
  gpu->max_frames = creation->max_frames;
  gpu->vk_instance = _vk_create_instance( creation->vk_application_name, creation->vk_application_version, gpu->vk_allocation_callbacks );
  gpu->vk_debug_utils_messenger = _vk_create_debug_utils_messsenger( gpu->vk_instance, gpu->vk_allocation_callbacks );
  gpu->vk_surface = _vk_create_surface( creation->sdl_window, gpu->vk_instance, gpu->vk_allocation_callbacks );
  gpu->vk_physical_device = _vk_pick_physical_device( gpu->vk_instance, gpu->vk_surface, &gpu->vk_queue_family_index );
  gpu->vk_device = _vk_create_device( gpu->vk_physical_device, gpu->vk_queue_family_index, gpu->vk_allocation_callbacks );
  vkGetDeviceQueue( gpu->vk_device, gpu->vk_queue_family_index, 0u, &gpu->vk_queue );
  gpu->vk_swapchain = _vk_create_swapchain( gpu->vk_device, gpu->vk_physical_device, gpu->vk_surface, gpu->vk_queue_family_index, gpu->vk_allocation_callbacks, &gpu->vk_swapchain_images_count, gpu->vk_swapchain_images, gpu->vk_swapchain_images_views, &gpu->vk_surface_format, &gpu->vk_swapchain_width, &gpu->vk_swapchain_height );
  gpu->vma_allocator = _vk_create_vma_allocator( gpu->vk_device, gpu->vk_physical_device, gpu->vk_instance );
  gpu->vk_descriptor_pool = _vk_create_descriptor_pool( gpu->vk_device, gpu->vk_allocation_callbacks );
  gpu->vk_timestamp_query_pool = _vk_create_timestamp_query_pool( gpu->vk_device, gpu->max_frames, gpu->vk_allocation_callbacks );
  crude_initialize_resource_pool( &gpu->buffers, gpu->allocator, 4096, sizeof( crude_buffer ) );
  crude_initialize_resource_pool( &gpu->textures, gpu->allocator, 512, sizeof( crude_texture ) );
  crude_initialize_resource_pool( &gpu->render_passes, gpu->allocator, 256, sizeof( crude_render_pass ) );
  crude_initialize_resource_pool( &gpu->descriptor_set_layouts, gpu->allocator, 128, sizeof( crude_descriptor_set_layout ) );
  crude_initialize_resource_pool( &gpu->pipelines, gpu->allocator, 128, sizeof( crude_pipeline ) );
  crude_initialize_resource_pool( &gpu->shaders, gpu->allocator, 128, sizeof( crude_shader_state ) );
  crude_initialize_resource_pool( &gpu->descriptor_sets, gpu->allocator, 256, sizeof( crude_descriptor_set ) );
  crude_initialize_resource_pool( &gpu->samplers, gpu->allocator, 32, sizeof( crude_sampler ) );
  
  VkSemaphoreCreateInfo semaphore_info = { .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO };
  VkFenceCreateInfo fence_info = { .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO, .flags = VK_FENCE_CREATE_SIGNALED_BIT };
  for ( uint32 i = 0; i < CRUDE_MAX_SWAPCHAIN_IMAGES; ++i )
  {
    vkCreateSemaphore( gpu->vk_device, &semaphore_info, gpu->vk_allocation_callbacks, &gpu->vk_render_finished_semaphores[ i ] );
    vkCreateSemaphore( gpu->vk_device, &semaphore_info, gpu->vk_allocation_callbacks, &gpu->vk_image_avalivable_semaphores[ i ] );
    vkCreateFence( gpu->vk_device, &fence_info, gpu->vk_allocation_callbacks, &gpu->vk_command_buffer_executed_fences[ i ] );
  }
  
  crude_gfx_initialize_cmd_manager( &g_command_buffer_manager, gpu );
  gpu->queued_command_buffers = gpu->allocator.allocate( sizeof( crude_command_buffer* ) * 128, 1 );

  gpu->previous_frame = 0;
  gpu->current_frame = 1;
  gpu->vk_swapchain_image_index = 0;
  gpu->queued_command_buffers_count = 0;

  gpu->resource_deletion_queue = NULL;
  arrsetcap( gpu->resource_deletion_queue, 16 );

  crude_sampler_creation sampler_creation = {
    .address_mode_u = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
    .address_mode_v = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
    .address_mode_w = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
    .min_filter     = VK_FILTER_LINEAR,
    .mag_filter     = VK_FILTER_LINEAR,
    .mip_filter     = VK_SAMPLER_MIPMAP_MODE_LINEAR,
    .name           = "sampler default"
  };
  gpu->default_sampler = crude_gfx_create_sampler( gpu, &sampler_creation );
  
  crude_texture_creation depth_texture_creation = { 
    .width    = gpu->vk_swapchain_width,
    .height   = gpu->vk_swapchain_height, 
    .depth    = 1,
    .mipmaps  = 1, 
    .format   = VK_FORMAT_D32_SFLOAT, 
    .type     = CRUDE_TEXTURE_TYPE_TEXTURE_2D, 
    .name     = "depth_image_texture"
  };
  gpu->depth_texture = crude_gfx_create_texture( gpu, &depth_texture_creation );

  crude_reset_render_pass_output( &gpu->swapchain_output );
  gpu->swapchain_output.color_formats[ gpu->swapchain_output.num_color_formats++ ] = gpu->vk_surface_format.format;
  gpu->swapchain_output.depth_operation = VK_FORMAT_D32_SFLOAT;

  crude_render_pass_creation swapchain_pass_creation = {
    .name                  = "swapchain",
    .type                  = CRUDE_RENDER_PASS_TYPE_SWAPCHAIN,
    .color_operation       = CRUDE_RENDER_PASS_OPERATION_CLEAR,
    .depth_operation       = CRUDE_RENDER_PASS_OPERATION_CLEAR,
    .depth_stencil_texture = CRUDE_RENDER_PASS_OPERATION_CLEAR,
    .scale_x             = 1.f,
    .scale_y             = 1.f,
    .resize              = 1,
  };
  gpu->swapchain_pass = crude_gfx_create_render_pass( gpu, &swapchain_pass_creation );
  
  gpu->dynamic_allocated_size = 0;
  gpu->dynamic_max_per_frame_size = 0;
  gpu->dynamic_per_frame_size = 1024 * 1024 * 10;
  crude_buffer_creation buffer_creation = {
    .name       = "dynamic_persistent_buffer",
    .type_flags = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
    .usage      = CRUDE_RESOURCE_USAGE_TYPE_IMMUTABLE,
    .size       = gpu->dynamic_per_frame_size * gpu->max_frames,
  };
  gpu->dynamic_buffer = crude_gfx_create_buffer( gpu, &buffer_creation );
  
  crude_map_buffer_parameters buffer_map = {
    .buffer = gpu->dynamic_buffer
  };
  gpu->dynamic_mapped_memory = ( uint8* )crude_gfx_map_buffer( gpu, &buffer_map );
 }

void
crude_gfx_deinitialize_gpu_device
(
  _In_ crude_gpu_device *gpu
)
{
  vkDeviceWaitIdle( gpu->vk_device );
  
  crude_gfx_deinitialize_cmd_manager( &g_command_buffer_manager );

  for ( uint32 i = 0; i < CRUDE_MAX_SWAPCHAIN_IMAGES; ++i )
  {
    vkDestroySemaphore( gpu->vk_device, gpu->vk_render_finished_semaphores[ i ], gpu->vk_allocation_callbacks );
    vkDestroySemaphore( gpu->vk_device, gpu->vk_image_avalivable_semaphores[ i ], gpu->vk_allocation_callbacks );
    vkDestroyFence( gpu->vk_device, gpu->vk_command_buffer_executed_fences[ i ], gpu->vk_allocation_callbacks );
  }
  
  crude_map_buffer_parameters buffer_map = {
    .buffer = gpu->dynamic_buffer
  };
  crude_gfx_unmap_buffer( gpu, &buffer_map );
  crude_gfx_destroy_buffer( gpu, gpu->dynamic_buffer );
  crude_gfx_destroy_texture( gpu, gpu->depth_texture );
  crude_gfx_destroy_render_pass( gpu, gpu->swapchain_pass );
  crude_gfx_destroy_sampler( gpu, gpu->default_sampler );

  for ( uint32 i = 0; i < arrlen( gpu->resource_deletion_queue ); ++i )
  {
    crude_resource_update* resource_deletion = &gpu->resource_deletion_queue[ i ];

    if ( resource_deletion->current_frame == -1 )
      continue;

    switch ( resource_deletion->type )
    {
      case CRUDE_RESOURCE_DELETION_TYPE_SAMPLER:
      {
        crude_gfx_destroy_sampler_instant( gpu, ( crude_sampler_handle ){ resource_deletion->handle } );
        break;
      }
      case CRUDE_RESOURCE_DELETION_TYPE_TEXTURE:
      {
        crude_gfx_destroy_texture_instant( gpu, ( crude_texture_handle ){ resource_deletion->handle } );
        break;
      }
      case CRUDE_RESOURCE_DELETION_TYPE_RENDER_PASS:
      {
        crude_gfx_destroy_render_pass_instant( gpu, ( crude_render_pass_handle ){ resource_deletion->handle } );
        break;
      }
      case CRUDE_RESOURCE_DELETION_TYPE_SHADER_STATE:
      {
        crude_gfx_destroy_shader_state_instant( gpu, ( crude_shader_state_handle ){ resource_deletion->handle } );
        break;
      }
      case CRUDE_RESOURCE_DELETION_TYPE_PIPELINE:
      {
        crude_gfx_destroy_pipeline_instant( gpu, ( crude_pipeline_handle ){ resource_deletion->handle } );
        break;
      }
      case CRUDE_RESOURCE_DELETION_TYPE_BUFFER:
      {
        crude_gfx_destroy_buffer_instant( gpu, ( crude_buffer_handle ){ resource_deletion->handle } );
        break;
      }
    }
  }

  arrfree( gpu->resource_deletion_queue );

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
  _vk_destroy_swapchain( gpu->vk_device, gpu->vk_swapchain, gpu->vk_swapchain_images_count, gpu->vk_swapchain_images_views, gpu->vk_swapchain_framebuffers, gpu->vk_allocation_callbacks );
  vmaDestroyAllocator( gpu->vma_allocator );
  vkDestroyDevice( gpu->vk_device, gpu->vk_allocation_callbacks );
  vkDestroySurfaceKHR( gpu->vk_instance, gpu->vk_surface, gpu->vk_allocation_callbacks );
  PFN_vkDestroyDebugUtilsMessengerEXT vkDestroyDebugUtilsMessengerEXT = vkGetInstanceProcAddr( gpu->vk_instance, "vkDestroyDebugUtilsMessengerEXT" );
  vkDestroyDebugUtilsMessengerEXT( gpu->vk_instance, gpu->vk_debug_utils_messenger, gpu->vk_allocation_callbacks );
  vkDestroyInstance( gpu->vk_instance, gpu->vk_allocation_callbacks );
}

/////////////////////
////@ Common
/////////////////////
void
crude_gfx_new_frame
(
  _In_ crude_gpu_device *gpu
)
{
  VkFence *render_complete_fence = &gpu->vk_command_buffer_executed_fences[ gpu->current_frame ];
  if ( vkGetFenceStatus( gpu->vk_device, *render_complete_fence ) != VK_SUCCESS )
  {
    vkWaitForFences( gpu->vk_device, 1, render_complete_fence, VK_TRUE, UINT64_MAX );
  }
  
  vkResetFences( gpu->vk_device, 1, render_complete_fence );
  
  VkResult result = vkAcquireNextImageKHR( gpu->vk_device, gpu->vk_swapchain, UINT64_MAX, gpu->vk_image_avalivable_semaphores[ gpu->current_frame ], VK_NULL_HANDLE, &gpu->vk_swapchain_image_index );
  if ( result == VK_ERROR_OUT_OF_DATE_KHR  )
  {
    _vk_resize_swapchain( gpu );
  }

  crude_gfx_reset_cmd_manager( &g_command_buffer_manager, gpu->current_frame );

  uint32 used_size = gpu->dynamic_allocated_size - ( gpu->dynamic_per_frame_size * gpu->previous_frame );
  gpu->dynamic_max_per_frame_size = MAX( used_size, gpu->dynamic_max_per_frame_size );
  gpu->dynamic_allocated_size = gpu->dynamic_per_frame_size * gpu->current_frame;
}

void
crude_gfx_present
(
  _In_ crude_gpu_device *gpu
)
{
  VkFence     *render_complete_fence = &gpu->vk_command_buffer_executed_fences[ gpu->current_frame ];
  VkSemaphore *render_complete_semaphore = &gpu->vk_render_finished_semaphores[ gpu->current_frame ];

  VkCommandBuffer enqueued_command_buffers[ 4 ];
  for ( uint32 i = 0; i < gpu->queued_command_buffers_count; ++i )
  {
    crude_command_buffer* command_buffer = gpu->queued_command_buffers[i];
    enqueued_command_buffers[ i ] = command_buffer->vk_handle;

    if ( command_buffer->is_recording && command_buffer->current_render_pass && ( command_buffer->current_render_pass->type != CRUDE_RENDER_PASS_TYPE_COMPUTE ) )
    {
      vkCmdEndRenderPass( command_buffer->vk_handle );
    }

    vkEndCommandBuffer( command_buffer->vk_handle );
  }

  VkSemaphore wait_semaphores[] = { gpu->vk_image_avalivable_semaphores[ gpu->current_frame ]};
  VkPipelineStageFlags wait_stages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
  VkSubmitInfo submit_info = { 
    .sType                = VK_STRUCTURE_TYPE_SUBMIT_INFO,
    .waitSemaphoreCount   = 1,
    .pWaitSemaphores      = wait_semaphores,
    .pWaitDstStageMask    = wait_stages,
    .commandBufferCount   = gpu->queued_command_buffers_count,
    .pCommandBuffers      = enqueued_command_buffers,
    .signalSemaphoreCount = 1,
    .pSignalSemaphores    = render_complete_semaphore,
  };
  
  CRUDE_GFX_HANDLE_VULKAN_RESULT( vkQueueSubmit( gpu->vk_queue, 1, &submit_info, *render_complete_fence ), "Failed to sumbit queue" );

  VkSwapchainKHR swap_chains[] = { gpu->vk_swapchain };
  VkPresentInfoKHR present_info = {
    .sType              = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
    .waitSemaphoreCount = 1,
    .pWaitSemaphores    = render_complete_semaphore,
    .swapchainCount     = ARRAY_SIZE( swap_chains ),
    .pSwapchains        = swap_chains,
    .pImageIndices      = &gpu->vk_swapchain_image_index,
  };
  VkResult result = vkQueuePresentKHR( gpu->vk_queue, &present_info );
  
  gpu->queued_command_buffers_count = 0u;

  if ( result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR )
  {
    _vk_resize_swapchain( gpu );
    return;
  }

  gpu->previous_frame = gpu->current_frame;
  gpu->current_frame = ( gpu->current_frame + 1u ) % gpu->vk_swapchain_images_count;
  
  for ( uint32 i = 0; i < arrlen( gpu->resource_deletion_queue ); ++i )
  {
    crude_resource_update* resource_deletion = &gpu->resource_deletion_queue[ i ];
    
    if ( resource_deletion->current_frame != gpu->current_frame )
    {
      continue;
    }

    switch ( resource_deletion->type )
    {
      case CRUDE_RESOURCE_DELETION_TYPE_SAMPLER:
      {
        crude_gfx_destroy_sampler_instant( gpu, ( crude_sampler_handle ){ resource_deletion->handle } );
        break;
      }
      case CRUDE_RESOURCE_DELETION_TYPE_TEXTURE:
      {
        crude_gfx_destroy_texture_instant( gpu, ( crude_texture_handle ){ resource_deletion->handle } );
        break;
      }
      case CRUDE_RESOURCE_DELETION_TYPE_RENDER_PASS:
      {
        crude_gfx_destroy_render_pass_instant( gpu, ( crude_render_pass_handle ){ resource_deletion->handle } );
        break;
      }
      case CRUDE_RESOURCE_DELETION_TYPE_SHADER_STATE:
      {
        crude_gfx_destroy_shader_state_instant( gpu, ( crude_shader_state_handle ){ resource_deletion->handle } );
        break;
      }
      case CRUDE_RESOURCE_DELETION_TYPE_PIPELINE:
      {
        crude_gfx_destroy_pipeline_instant( gpu, ( crude_pipeline_handle ){ resource_deletion->handle } );
        break;
      }
      case CRUDE_RESOURCE_DELETION_TYPE_BUFFER:
      {
        crude_gfx_destroy_buffer_instant( gpu, ( crude_buffer_handle ){ resource_deletion->handle } );
        break;
      }
    }

    resource_deletion->current_frame = UINT32_MAX;
    arrdelswap( gpu->resource_deletion_queue, i );
    --i;
  }
}

crude_command_buffer*
crude_gfx_get_cmd_buffer
(
  _In_ crude_gpu_device  *gpu,
  _In_ crude_queue_type   type,
  _In_ bool               begin
)
{
  crude_command_buffer *cmd = crude_gfx_cmd_manager_get_cmd_buffer( &g_command_buffer_manager, gpu->current_frame, begin );
 
  //if ( begin )
  //{
  //  vkCmdResetQueryPool( command_buffer->vk_command_buffer, gpu->vk_timestamp_query_pool, gpu->current_frame * 3 * 2, 3 );
  //}
  
  return cmd;
}

void
crude_gfx_queue_cmd_buffer
(
  _In_ crude_command_buffer *cmd
)
{
  cmd->gpu->queued_command_buffers[ cmd->gpu->queued_command_buffers_count++ ] = cmd;
}

void*
crude_gfx_map_buffer
(
  _In_ crude_gpu_device                     *gpu,
  _In_ crude_map_buffer_parameters const    *parameters
)
{
  if ( parameters->buffer.index == CRUDE_RESOURCE_INVALID_INDEX )
    return NULL;

  crude_buffer *buffer = CRUDE_GFX_GPU_ACCESS_BUFFER( gpu, parameters->buffer );
  
  if ( buffer->parent_buffer.index == gpu->dynamic_buffer.index )
  {
    buffer->global_offset = gpu->dynamic_allocated_size;
    return crude_gfx_dynamic_allocate( gpu, parameters->size == 0 ? buffer->size : parameters->size );
  }

  void* data;
  vmaMapMemory( gpu->vma_allocator, buffer->vma_allocation, &data );
  return data;
}

void
crude_gfx_unmap_buffer
(
  _In_ crude_gpu_device                     *gpu,
  _In_ crude_map_buffer_parameters const    *parameters
)
{
  if ( parameters->buffer.index == CRUDE_RESOURCE_INVALID_INDEX )
    return;
  
  crude_buffer *buffer = CRUDE_GFX_GPU_ACCESS_BUFFER( gpu, parameters->buffer );
  if ( buffer->parent_buffer.index == gpu->dynamic_buffer.index )
    return;
  
  vmaUnmapMemory( gpu->vma_allocator, buffer->vma_allocation );
}

void*
crude_gfx_dynamic_allocate
(
  _In_ crude_gpu_device                     *gpu,
  _In_ uint32                                size
)
{
  void *mapped_memory = gpu->dynamic_mapped_memory + gpu->dynamic_allocated_size;
  gpu->dynamic_allocated_size += crude_memory_align( size, CRUDE_UBO_ALIGNMENT );
  return mapped_memory;
}

/////////////////////
//// @Query
/////////////////////

void
crude_gfx_query_buffer
(
  _In_ crude_gpu_device                     *gpu,
  _In_ crude_buffer_handle                   buffer,
  _Out_ crude_buffer_description            *description
)
{
  if ( buffer.index == CRUDE_RESOURCE_INVALID_INDEX )
  {
    return;
  }

  const crude_buffer* buffer_data = CRUDE_GFX_GPU_ACCESS_BUFFER( gpu, buffer );
  description->name = buffer_data->name;
  description->size = buffer_data->size;
  description->type_flags = buffer_data->type_flags;
  description->usage = buffer_data->usage;
  description->parent_handle = buffer_data->parent_buffer;
  description->native_handle = &buffer_data->vk_buffer;
}

void
crude_gfx_set_resource_name
(
  _In_ crude_gpu_device  *gpu,
  _In_ VkObjectType       type,
  _In_ uint64             handle,
  _In_ char const        *name
)
{
  VkDebugUtilsObjectNameInfoEXT name_info = {
    .sType        = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT,
    .objectType   = type,
    .objectHandle = handle,
    .pObjectName  = name,
  };
  PFN_vkSetDebugUtilsObjectNameEXT vkSetDebugUtilsObjectNameEXT = vkGetDeviceProcAddr( gpu->vk_device, "vkSetDebugUtilsObjectNameEXT" );
  vkSetDebugUtilsObjectNameEXT( gpu->vk_device, &name_info );
}

///////////////////
//@ Resources
///////////////////

crude_sampler_handle
crude_gfx_create_sampler
(
  _In_ crude_gpu_device             *gpu,
  _In_ crude_sampler_creation const *creation
)
{
  crude_sampler_handle handle = { CRUDE_GFX_GPU_OBTAIN_SAMPLER( gpu ) };
  if ( handle.index == CRUDE_RESOURCE_INVALID_INDEX )
  {
    return handle;
  }
  
  crude_sampler *sampler = CRUDE_GFX_GPU_ACCESS_SAMPLER( gpu, handle );
  sampler->address_mode_u = creation->address_mode_u;
  sampler->address_mode_v = creation->address_mode_v;
  sampler->address_mode_w = creation->address_mode_w;
  sampler->min_filter     = creation->min_filter;
  sampler->mag_filter     = creation->mag_filter;
  sampler->mip_filter     = creation->mip_filter;
  sampler->name           = creation->name;
  
  VkSamplerCreateInfo create_info = { 
    .sType                    = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
    .addressModeU             = creation->address_mode_u,
    .addressModeV             = creation->address_mode_v,
    .addressModeW             = creation->address_mode_w,
    .minFilter                = creation->min_filter,
    .magFilter                = creation->mag_filter,
    .mipmapMode               = creation->mip_filter,
    .anisotropyEnable         = 0,
    .compareEnable            = 0,
    .unnormalizedCoordinates  = 0,
    .borderColor              = VK_BORDER_COLOR_INT_OPAQUE_WHITE,
  };
  
  CRUDE_GFX_HANDLE_VULKAN_RESULT( vkCreateSampler( gpu->vk_device, &create_info, gpu->vk_allocation_callbacks, &sampler->vk_sampler ), "Failed to create sampler" );
  crude_gfx_set_resource_name( gpu, VK_OBJECT_TYPE_SAMPLER, CAST( uint64, sampler->vk_sampler ), creation->name );
  return handle;
}

void
crude_gfx_destroy_sampler
(
  _In_ crude_gpu_device     *gpu,
  _In_ crude_sampler_handle  handle
)
{
  if ( handle.index >= gpu->samplers.pool_size )
  {
    CRUDE_LOG_ERROR( CRUDE_CHANNEL_GRAPHICS, "Trying to free invalid sampler %u", handle.index );
    return;
  }
  crude_resource_update sampler_update_event = { 
    .type          = CRUDE_RESOURCE_DELETION_TYPE_SAMPLER,
    .handle        = handle.index,
    .current_frame = gpu->current_frame };
  arrput( gpu->resource_deletion_queue, sampler_update_event );
}

void
crude_gfx_destroy_sampler_instant
(
  _In_ crude_gpu_device       *gpu,
  _In_ crude_sampler_handle    handle
)
{
  crude_sampler *sampler = CRUDE_GFX_GPU_ACCESS_SAMPLER( gpu, handle );
  if ( sampler )
  {
    vkDestroySampler( gpu->vk_device, sampler->vk_sampler, gpu->vk_allocation_callbacks );
  }
  CRUDE_GFX_GPU_RELEASE_SAMPLER( gpu, handle );
}

crude_texture_handle crude_gfx_create_texture
(
  _In_ crude_gpu_device             *gpu,
  _In_ crude_texture_creation const *creation
)
{
  crude_texture_handle handle = { CRUDE_GFX_GPU_OBTAIN_TEXTURE( gpu ) };
  if ( handle.index == CRUDE_RESOURCE_INVALID_INDEX )
  {
    return handle;
  }
  
  crude_texture *texture = CRUDE_GFX_GPU_ACCESS_TEXTURE( gpu, handle );
  _vk_create_texture( gpu, creation, handle, texture );
  return handle;
}

void
crude_gfx_destroy_texture
(
  _In_ crude_gpu_device     *gpu,
  _In_ crude_texture_handle  handle
)
{
  if ( handle.index >= gpu->textures.pool_size )
  {
    CRUDE_LOG_ERROR( CRUDE_CHANNEL_GRAPHICS, "Trying to free invalid texture %u", handle.index );
    return;
  }
  crude_resource_update texture_update_event = { 
    .type          = CRUDE_RESOURCE_DELETION_TYPE_TEXTURE,
    .handle        = handle.index,
    .current_frame = gpu->current_frame };
  arrput( gpu->resource_deletion_queue, texture_update_event );
}

void
crude_gfx_destroy_texture_instant
(
  _In_ crude_gpu_device      *gpu,
  _In_ crude_texture_handle  handle
)
{
  crude_texture *texture = CRUDE_GFX_GPU_ACCESS_TEXTURE( gpu, handle );
  
  if ( texture )
  {
    vkDestroyImageView( gpu->vk_device, texture->vk_image_view, gpu->vk_allocation_callbacks );
    vmaDestroyImage( gpu->vma_allocator, texture->vk_image, texture->vma_allocation );
  }
  CRUDE_GFX_GPU_RELEASE_TEXTURE( gpu, handle );
}

crude_render_pass_handle
crude_gfx_create_render_pass
(
  _In_ crude_gpu_device                 *gpu,
  _In_ crude_render_pass_creation const *creation
)
{
  crude_render_pass_handle handle = { CRUDE_GFX_GPU_OBTAIN_RENDER_PASS( gpu) };
  if ( handle.index == CRUDE_RESOURCE_INVALID_INDEX )
  {
    return handle;
  }
  
  crude_render_pass *render_pass = CRUDE_GFX_GPU_ACCESS_RENDER_PASS( gpu, handle );
  render_pass->type               = creation->type;
  render_pass->num_render_targets = creation->num_render_targets;
  render_pass->dispatch_x         = 0;
  render_pass->dispatch_y         = 0;
  render_pass->dispatch_z         = 0;
  render_pass->name               = creation->name;
  render_pass->vk_frame_buffer    = NULL;
  render_pass->vk_render_pass     = NULL;
  render_pass->scale_x            = creation->scale_x;
  render_pass->scale_y            = creation->scale_y;
  render_pass->resize             = creation->resize;
  
  for ( uint32 i = 0 ; i < creation->num_render_targets; ++i )
  {
    crude_texture *texture = CRUDE_GFX_GPU_ACCESS_TEXTURE( gpu, creation->output_textures[ i ] );
    
    render_pass->width = texture->width;
    render_pass->height = texture->height;
    render_pass->output_textures[ i ] = creation->output_textures[ i ];
  }
  
  render_pass->output_depth = creation->depth_stencil_texture;
  
  switch ( creation->type )
  {
    case CRUDE_RENDER_PASS_TYPE_SWAPCHAIN:
    {
      _vk_create_swapchain_pass( gpu, creation, render_pass );
      break;
    }
  }
  
  return handle;
}

void
crude_gfx_destroy_render_pass
(
  _In_ crude_gpu_device         *gpu,
  _In_ crude_render_pass_handle  handle
)
{
  if ( handle.index >= gpu->render_passes.pool_size )
  {
    CRUDE_LOG_ERROR( CRUDE_CHANNEL_GRAPHICS, "Trying to free invalid texture %u", handle.index );
    return;
  }
  crude_resource_update render_pass_update_event = { 
    .type          = CRUDE_RESOURCE_DELETION_TYPE_RENDER_PASS,
    .handle        = handle.index,
    .current_frame = gpu->current_frame };
  arrput( gpu->resource_deletion_queue, render_pass_update_event );
}

void
crude_gfx_destroy_render_pass_instant
(
  _In_ crude_gpu_device         *gpu,
  _In_ crude_render_pass_handle  handle
)
{
  crude_render_pass *render_pass = CRUDE_GFX_GPU_ACCESS_RENDER_PASS( gpu, handle );
  if ( render_pass )
  {
    if ( render_pass->num_render_targets )
    {
      vkDestroyFramebuffer( gpu->vk_device, render_pass->vk_frame_buffer, gpu->vk_allocation_callbacks );
    }
    vkDestroyRenderPass( gpu->vk_device, render_pass->vk_render_pass, gpu->vk_allocation_callbacks );
  }
  CRUDE_GFX_GPU_RELEASE_RENDER_PASS( gpu, handle );
}

crude_shader_state_handle
crude_gfx_create_shader_state
(
  _In_ crude_gpu_device                  *gpu,
  _In_ crude_shader_state_creation const *creation
)
{ 
  if ( creation->stages_count == 0 || creation->stages == NULL )
  {
    CRUDE_LOG_ERROR( CRUDE_CHANNEL_GRAPHICS, "Shader %s does not contain shader stages.", creation->name );
    return ( crude_shader_state_handle ){ CRUDE_RESOURCE_INVALID_INDEX };
  }
  
  crude_shader_state_handle handle = { CRUDE_GFX_GPU_OBTAIN_SHADER_STATE( gpu ) };
  if ( handle.index == CRUDE_RESOURCE_INVALID_INDEX )
  {
    return handle;
  }

  uint32 compiled_shaders = 0u;

  crude_shader_state *shader_state = CRUDE_GFX_GPU_ACCESS_SHADER_STATE( gpu, handle );
  shader_state->graphics_pipeline = true;
  shader_state->active_shaders = 0;

  for ( compiled_shaders = 0; compiled_shaders < creation->stages_count; ++compiled_shaders )
  {
    crude_shader_stage const *stage = &creation->stages[ compiled_shaders ];
  
    if ( stage->type == VK_SHADER_STAGE_COMPUTE_BIT )
    {
      shader_state->graphics_pipeline = false;
    }
  
    VkShaderModuleCreateInfo shader_create_info = { .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO };
    if ( creation->spv_input )
    {
      shader_create_info.codeSize = stage->code_size;
      shader_create_info.pCode = CAST( uint32 const *, stage->code );
    }
    else
    {
      shader_create_info = crude_gfx_compile_shader( stage->code, stage->code_size, stage->type, creation->name );
    }
  
    VkPipelineShaderStageCreateInfo *shader_stage_info = &shader_state->shader_stage_info[ compiled_shaders ];
    memset( shader_stage_info, 0, sizeof( VkPipelineShaderStageCreateInfo ) );
    shader_stage_info->sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    shader_stage_info->pName = "main";
    shader_stage_info->stage = stage->type;
    
    if ( vkCreateShaderModule( gpu->vk_device, &shader_create_info, NULL, &shader_state->shader_stage_info[ compiled_shaders ].module ) != VK_SUCCESS )
    {
      break;
    }
    
    crude_gfx_set_resource_name( gpu, VK_OBJECT_TYPE_SHADER_MODULE, ( uint64 ) shader_state->shader_stage_info[ compiled_shaders ].module, creation->name );
  }
  
  bool creation_failed = compiled_shaders != creation->stages_count;
  if ( creation_failed )
  {
    crude_gfx_destroy_shader_state( gpu, handle );
    
    CRUDE_LOG_ERROR( CRUDE_CHANNEL_GRAPHICS, "Error in creation of shader %s. Dumping all shader informations.", creation->name );
    for ( compiled_shaders = 0; compiled_shaders < creation->stages_count; ++compiled_shaders )
    {
      crude_shader_stage const *stage = &creation->stages[ compiled_shaders ];
      CRUDE_LOG_ERROR( CRUDE_CHANNEL_GRAPHICS, "%u:\n%s", stage->type, stage->code );
    }
    return ( crude_shader_state_handle ) { CRUDE_RESOURCE_INVALID_INDEX };
  }

  shader_state->active_shaders = compiled_shaders;
  shader_state->name = creation->name;
  return handle;
}

void
crude_gfx_destroy_shader_state
(
  _In_ crude_gpu_device          *gpu,
  _In_ crude_shader_state_handle  handle
)
{
  if ( handle.index >= gpu->shaders.pool_size )
  {
    CRUDE_LOG_ERROR( CRUDE_CHANNEL_GRAPHICS, "Trying to free invalid shader state %u", handle.index );
    return;
  }
  crude_resource_update shader_state_update_event = { 
    .type          = CRUDE_RESOURCE_DELETION_TYPE_SHADER_STATE,
    .handle        = handle.index,
    .current_frame = gpu->current_frame };
  arrput( gpu->resource_deletion_queue, shader_state_update_event );
}

void
crude_gfx_destroy_shader_state_instant
(
  _In_ crude_gpu_device          *gpu,
  _In_ crude_shader_state_handle  handle
)
{
  crude_shader_state *shader_state = CRUDE_GFX_GPU_ACCESS_SHADER_STATE( gpu, handle );
  if ( shader_state )
  {
    for ( uint32 i = 0; i < shader_state->active_shaders; ++i )
    {
      vkDestroyShaderModule( gpu->vk_device, shader_state->shader_stage_info[ i ].module, gpu->vk_allocation_callbacks );
    }
  }
  CRUDE_GFX_GPU_RELEASE_SHADER_STATE( gpu, handle );
}

crude_pipeline_handle
crude_gfx_create_pipeline
(
  _In_ crude_gpu_device              *gpu,
  _In_ crude_pipeline_creation const *creation
)
{
  crude_pipeline_handle handle = { CRUDE_GFX_GPU_OBTAIN_PIPELINE( gpu ) };
  if ( handle.index == CRUDE_RESOURCE_INVALID_INDEX )
  {
    return handle;
  }

  crude_shader_state_handle shader_state = crude_gfx_create_shader_state( gpu, &creation->shaders );
  if ( shader_state.index == CRUDE_RESOURCE_INVALID_INDEX )
  {
    CRUDE_GFX_GPU_RELEASE_PIPELINE( gpu, handle );
    return ( crude_pipeline_handle ) { CRUDE_RESOURCE_INVALID_INDEX };
  }

  crude_pipeline *pipeline = CRUDE_GFX_GPU_ACCESS_PIPELINE( gpu, handle );
  crude_shader_state *shader_state_data = CRUDE_GFX_GPU_ACCESS_SHADER_STATE( gpu, shader_state );
  
  pipeline->shader_state = shader_state;

  VkDescriptorSetLayout vk_layouts[ CRUDE_MAX_DESCRIPTOR_SET_LAYOUTS ];
  for ( uint32 i = 0; i < creation->num_active_layouts; ++i )
  {
    pipeline->descriptor_set_layout[i] = CRUDE_GFX_GPU_ACCESS_DESCRIPTOR_SET( gpu, creation->descriptor_set_layout[i] );
    pipeline->descriptor_set_layout_handle[i] = creation->descriptor_set_layout[i];

    vk_layouts[i] = pipeline->descriptor_set_layout[i]->vk_descriptor_set_layout;
  }

  VkPipelineLayoutCreateInfo pipeline_layout_info = {
    .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
    .pSetLayouts = vk_layouts,
    .setLayoutCount = creation->num_active_layouts,
  };
  
  VkPipelineLayout pipeline_layout;
  CRUDE_GFX_HANDLE_VULKAN_RESULT( vkCreatePipelineLayout( gpu->vk_device, &pipeline_layout_info, gpu->vk_allocation_callbacks, &pipeline_layout ), "Failed to create pipeline layout" );

  pipeline->vk_pipeline_layout = pipeline_layout;
  pipeline->num_active_layouts = creation->num_active_layouts;

  VkPipelineVertexInputStateCreateInfo vertex_input_info = { .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO };
  
  VkVertexInputAttributeDescription vertex_attributes[8];
  if ( creation->vertex_input.num_vertex_attributes )
  {
    for ( uint32 i = 0; i < creation->vertex_input.num_vertex_attributes; ++i )
    {
      crude_vertex_attribute const *vertex_attribute = &creation->vertex_input.vertex_attributes[i];
      vertex_attributes[i] = ( VkVertexInputAttributeDescription ){
        .location = vertex_attribute->location,
        .binding = vertex_attribute->binding,
        .format = crude_to_vk_vertex_format( vertex_attribute->format ),
        .offset = vertex_attribute->offset
      };
    }
    vertex_input_info.vertexAttributeDescriptionCount = creation->vertex_input.num_vertex_attributes;
    vertex_input_info.pVertexAttributeDescriptions = vertex_attributes;
  }
  else
  {
    vertex_input_info.vertexAttributeDescriptionCount = 0;
    vertex_input_info.pVertexAttributeDescriptions = NULL;
  }

  VkVertexInputBindingDescription vertex_bindings[8];
  if ( creation->vertex_input.num_vertex_streams )
  {
    vertex_input_info.vertexBindingDescriptionCount = creation->vertex_input.num_vertex_streams;
  
    for ( uint32 i = 0; i < creation->vertex_input.num_vertex_streams; ++i )
    {
      crude_vertex_stream const *vertex_stream = &creation->vertex_input.vertex_streams[i];
      VkVertexInputRate vertex_rate = vertex_stream->input_rate == CRUDE_VERTEX_INPUT_RATE_PER_VERTEX ? VK_VERTEX_INPUT_RATE_VERTEX : VK_VERTEX_INPUT_RATE_INSTANCE;
      vertex_bindings[i] = ( VkVertexInputBindingDescription ){
        .binding = vertex_stream->binding,
        .stride = vertex_stream->stride,
        .inputRate = vertex_rate
      };
    }
    vertex_input_info.pVertexBindingDescriptions = vertex_bindings;
  }
  else
  {
    vertex_input_info.vertexBindingDescriptionCount = 0;
    vertex_input_info.pVertexBindingDescriptions = NULL;
  }
  
  VkPipelineInputAssemblyStateCreateInfo input_assembly = {
    .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
    .topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
    .primitiveRestartEnable = VK_FALSE
  };

  VkPipelineColorBlendAttachmentState color_blend_attachment[8];
  
  if ( creation->blend_state.active_states )
  {
    for ( uint32 i = 0; i < creation->blend_state.active_states; ++i )
    {
      crude_blend_state const *blend_state = &creation->blend_state.blend_states[i];
  
      color_blend_attachment[i].colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
      color_blend_attachment[i].blendEnable = blend_state->blend_enabled ? VK_TRUE : VK_FALSE;
      color_blend_attachment[i].srcColorBlendFactor = blend_state->source_color;
      color_blend_attachment[i].dstColorBlendFactor = blend_state->destination_color;
      color_blend_attachment[i].colorBlendOp = blend_state->color_operation;
      
      if ( blend_state->separate_blend )
      {
        color_blend_attachment[i].srcAlphaBlendFactor = blend_state->source_alpha;
        color_blend_attachment[i].dstAlphaBlendFactor = blend_state->destination_alpha;
        color_blend_attachment[i].alphaBlendOp = blend_state->alpha_operation;
      }
      else
      {
        color_blend_attachment[i].srcAlphaBlendFactor = blend_state->source_color;
        color_blend_attachment[i].dstAlphaBlendFactor = blend_state->destination_color;
        color_blend_attachment[i].alphaBlendOp = blend_state->color_operation;
      }
    }
  }
  else
  {
    memset( &color_blend_attachment[0], 0u, sizeof( color_blend_attachment[0] ) );
    color_blend_attachment[0].blendEnable = VK_FALSE;
    color_blend_attachment[0].colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
  }
  
  VkPipelineColorBlendStateCreateInfo color_blending = {
    .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
    .logicOpEnable = VK_FALSE,
    .logicOp = VK_LOGIC_OP_COPY,
    .attachmentCount = creation->blend_state.active_states ? creation->blend_state.active_states : 1,
    .pAttachments = color_blend_attachment,
    .blendConstants[0] = 0.0f,
    .blendConstants[1] = 0.0f,
    .blendConstants[2] = 0.0f,
    .blendConstants[3] = 0.0f,
  };
  
  VkPipelineDepthStencilStateCreateInfo depth_stencil = { 
    .sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
    .depthWriteEnable = creation->depth_stencil.depth_write_enable ? VK_TRUE : VK_FALSE,
    .stencilTestEnable = creation->depth_stencil.stencil_enable ? VK_TRUE : VK_FALSE,
    .depthTestEnable = creation->depth_stencil.depth_enable ? VK_TRUE : VK_FALSE,
    .depthCompareOp = creation->depth_stencil.depth_comparison,
  };
  
  if ( creation->depth_stencil.stencil_enable )
  {
    CRUDE_ABORT( CRUDE_CHANNEL_GRAPHICS, "TODO" );
  }
  
  VkPipelineMultisampleStateCreateInfo multisampling = {
    .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
    .sampleShadingEnable = VK_FALSE,
    .rasterizationSamples = VK_SAMPLE_COUNT_1_BIT,
    .minSampleShading = 1.0f,
    .pSampleMask = NULL,
    .alphaToCoverageEnable = VK_FALSE,
    .alphaToOneEnable = VK_FALSE,
  };
  
  VkPipelineRasterizationStateCreateInfo rasterizer = {
    .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
    .depthClampEnable = VK_FALSE,
    .rasterizerDiscardEnable = VK_FALSE,
    .polygonMode = VK_POLYGON_MODE_FILL,
    .lineWidth = 1.0f,
    .cullMode = creation->rasterization.cull_mode,
    .frontFace = creation->rasterization.front,
    .depthBiasEnable = VK_FALSE,
    .depthBiasConstantFactor = 0.0f,
    .depthBiasClamp = 0.0f,
    .depthBiasSlopeFactor = 0.0f,
  };
  
  VkViewport viewport = {
    .x = 0.0f,
    .y = 0.0f,
    .width = gpu->vk_swapchain_width,
    .height = gpu->vk_swapchain_height,
    .minDepth = 0.0f,
    .maxDepth = 1.0f,
  };
  
  VkRect2D scissor = {
    .offset = { 0, 0 },
    .extent = { gpu->vk_swapchain_width, gpu->vk_swapchain_height },
  };
  
  VkPipelineViewportStateCreateInfo viewport_state = { 
    .sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
    .viewportCount = 1,
    .pViewports = &viewport,
    .scissorCount = 1,
    .pScissors = &scissor,
  };
  
  VkDynamicState dynamic_states[] = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
  VkPipelineDynamicStateCreateInfo dynamic_state = { 
    .sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
    .dynamicStateCount = ARRAY_SIZE( dynamic_states ),
    .pDynamicStates = dynamic_states,
  };
  
  crude_render_pass *render_pass = crude_resource_pool_access_resource( &gpu->render_passes, gpu->swapchain_pass.index );

  VkGraphicsPipelineCreateInfo pipeline_info = {
    .pStages = shader_state_data->shader_stage_info,
    .stageCount = shader_state_data->active_shaders,
    .layout = pipeline_layout,
    .pVertexInputState = &vertex_input_info,
    .pInputAssemblyState = &input_assembly,
    .pColorBlendState = &color_blending,
    .pDepthStencilState = &depth_stencil,
    .pMultisampleState = &multisampling,
    .pRasterizationState = &rasterizer,
    .pViewportState = &viewport_state,
    .renderPass = render_pass->vk_render_pass,
    .pDynamicState = &dynamic_state,
    .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
    .pStages = shader_state_data->shader_stage_info,
    .stageCount = shader_state_data->active_shaders,
    .layout = pipeline_layout,
  };
  
  CRUDE_GFX_HANDLE_VULKAN_RESULT( vkCreateGraphicsPipelines( gpu->vk_device, VK_NULL_HANDLE, 1, &pipeline_info, gpu->vk_allocation_callbacks, &pipeline->vk_pipeline ), "Failed to create pipeline" );
  pipeline->vk_bind_point = VK_PIPELINE_BIND_POINT_GRAPHICS;

  return handle;
}

void
crude_gfx_destroy_pipeline
(
  _In_ crude_gpu_device     *gpu,
  _In_ crude_pipeline_handle handle
)
{
  if ( handle.index >= gpu->pipelines.pool_size )
  {
    CRUDE_LOG_ERROR( CRUDE_CHANNEL_GRAPHICS, "Trying to free invalid pipeline %u", handle.index );
    return;
  }
  crude_resource_update pipeline_update_event = { 
    .type          = CRUDE_RESOURCE_DELETION_TYPE_PIPELINE,
    .handle        = handle.index,
    .current_frame = gpu->current_frame };
  arrput( gpu->resource_deletion_queue, pipeline_update_event );

  crude_pipeline *pipeline = CRUDE_GFX_GPU_ACCESS_PIPELINE( gpu, handle );
  crude_gfx_destroy_shader_state( gpu, pipeline->shader_state );
}

void
crude_gfx_destroy_pipeline_instant
(
  _In_ crude_gpu_device      *gpu,
  _In_ crude_pipeline_handle  handle
)
{
  crude_pipeline *pipeline = CRUDE_GFX_GPU_ACCESS_PIPELINE( gpu, handle );

  if ( pipeline )
  {
    vkDestroyPipeline( gpu->vk_device, pipeline->vk_pipeline, gpu->vk_allocation_callbacks );
    vkDestroyPipelineLayout( gpu->vk_device, pipeline->vk_pipeline_layout, gpu->vk_allocation_callbacks );
  }

  CRUDE_GFX_GPU_RELEASE_PIPELINE( gpu, handle );
}

crude_buffer_handle
crude_gfx_create_buffer
(
  _In_ crude_gpu_device                     *gpu,
  _In_ crude_buffer_creation const          *creation
)
{
  crude_buffer_handle handle = { CRUDE_GFX_GPU_OBTAIN_BUFFER( gpu ) };
  if ( handle.index == CRUDE_RESOURCE_INVALID_INDEX )
  {
      return handle;
  }

  crude_buffer *buffer = CRUDE_GFX_GPU_ACCESS_BUFFER( gpu, handle );
  buffer->name = creation->name;
  buffer->size = creation->size;
  buffer->type_flags = creation->type_flags;
  buffer->usage = creation->usage;
  buffer->handle = handle;
  buffer->global_offset = 0;
  buffer->parent_buffer = ( crude_buffer_handle ) { CRUDE_RESOURCE_INVALID_INDEX };

  bool use_global_buffer = ( creation->type_flags & ( VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT ) );
  if ( creation->usage == CRUDE_RESOURCE_USAGE_TYPE_DYNAMIC && use_global_buffer )
  {
    buffer->parent_buffer = gpu->dynamic_buffer;
    return handle;
  }

  VkBufferCreateInfo buffer_info = {
    .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
    .usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT | creation->type_flags,
    .size = creation->size > 0 ? creation->size : 1,
  };
  
  VmaAllocationCreateInfo memory_info = {
    .flags = VMA_ALLOCATION_CREATE_STRATEGY_BEST_FIT_BIT,
    .usage = VMA_MEMORY_USAGE_CPU_TO_GPU,
  };
  
  VmaAllocationInfo allocation_info;
  CRUDE_GFX_HANDLE_VULKAN_RESULT( vmaCreateBuffer( gpu->vma_allocator, &buffer_info, &memory_info, &buffer->vk_buffer, &buffer->vma_allocation, &allocation_info ),
    "Failed to create buffer %s %u", buffer->name, handle.index );
  
  crude_gfx_set_resource_name( gpu, VK_OBJECT_TYPE_BUFFER, ( uint64 )buffer->vk_buffer, creation->name );

  buffer->vk_device_memory = allocation_info.deviceMemory;

  if ( creation->initial_data )
  {
    void* data;
    vmaMapMemory( gpu->vma_allocator, buffer->vma_allocation, &data );
    memcpy( data, creation->initial_data, creation->size );
    vmaUnmapMemory( gpu->vma_allocator, buffer->vma_allocation );
  }
  return handle;
}

void
crude_gfx_destroy_buffer
(
  _In_ crude_gpu_device                     *gpu,
  _In_ crude_buffer_handle                   handle
)
{
  if ( handle.index >= gpu->buffers.pool_size )
  {
    CRUDE_LOG_ERROR( CRUDE_CHANNEL_GRAPHICS, "Trying to free invalid buffer %u", handle.index );
    return;
  }
  crude_resource_update buffer_update_event = { 
    .type          = CRUDE_RESOURCE_DELETION_TYPE_BUFFER,
    .handle        = handle.index,
    .current_frame = gpu->current_frame };
  arrput( gpu->resource_deletion_queue, buffer_update_event );
}

void
crude_gfx_destroy_buffer_instant
(
  _In_ crude_gpu_device                     *gpu,
  _In_ crude_buffer_handle                   handle
)
{
  crude_buffer *buffer = CRUDE_GFX_GPU_ACCESS_BUFFER( gpu, handle );

  if ( buffer && buffer->parent_buffer.index == CRUDE_RESOURCE_INVALID_INDEX )
  {
    vmaDestroyBuffer( gpu->vma_allocator, buffer->vk_buffer, buffer->vma_allocation );
  }

  CRUDE_GFX_GPU_RELEASE_BUFFER( gpu, handle );
}

VkShaderModuleCreateInfo
crude_gfx_compile_shader
(
  _In_ char const            *code,
  _In_ uint32                 code_size,
  _In_ VkShaderStageFlagBits  stage,
  _In_ char const            *name
)
{
  CRUDE_ABORT( CRUDE_CHANNEL_GRAPHICS, "TODO" );
}