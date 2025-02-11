#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>
#include <stb_ds.h>

#include <gui/gui.h>
#include <platform/sdl_system.h>
#include <graphics/render_core.h>
#include <core/assert.h>
#include <core/utils.h>
#include <math.h>

#include <graphics/render_system.h>

#define CLAMP( v, lo, hi ) ( ( ( v ) < ( lo ) ) ? ( lo ) : ( ( hi ) < ( v ) ) ? ( hi ) : ( v ) )

static char const *const device_required_extensions[] = 
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

static char const *const *instance_required_extensions[] =
{
  VK_EXT_DEBUG_UTILS_EXTENSION_NAME
};

static char const *const *required_layers[] =
{
  "VK_LAYER_KHRONOS_validation"
};

#define HANDLE_VULKAN_RESULT( result, msg ) if ( result != VK_SUCCESS ) CRUDE_ABORT( CRUDE_CHANNEL_GRAPHICS, "vulkan result isn't success: %i %s", result, msg );

static VKAPI_ATTR VkBool32 debug_callback(
  VkDebugUtilsMessageSeverityFlagBitsEXT      messageSeverity,
  VkDebugUtilsMessageTypeFlagsEXT             messageType,
  VkDebugUtilsMessengerCallbackDataEXT const *pCallbackData,
  void                                       *pUserData)
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
  char const            *application_name,
  uint32                 application_version,
  VkAllocationCallbacks *vulkan_allocation_callbacks)
{
  // extensions
  uint32 surface_extensions_count;
  char const *const *surface_extensions_array = SDL_Vulkan_GetInstanceExtensions( &surface_extensions_count );
  uint32 const debug_extensions_count = ARRAY_SIZE( instance_required_extensions );

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
    instance_enabled_extensions[surface_extensions_count + i] = instance_required_extensions[i];
  }

  // layers
  char const *instance_enabled_layers[] = { "VK_LAYER_KHRONOS_validation" };

  // application
  VkApplicationInfo application = ( VkApplicationInfo ) {
    .pApplicationName   = application_name,
    .applicationVersion = application_version,
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
  HANDLE_VULKAN_RESULT( vkCreateInstance( &instance_create_info, vulkan_allocation_callbacks, &handle ), "failed to create instance" );

  arrfree( instance_enabled_extensions );

  return handle;
}


static VkDebugUtilsMessengerEXT create_debug_utils_messsenger( VkInstance instance, VkAllocationCallbacks *allocation_callbacks )
{
  VkDebugUtilsMessengerCreateInfoEXT create_info = ( VkDebugUtilsMessengerCreateInfoEXT) {
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

static VkSurfaceKHR create_surface( crude_window_handle *window_handle, VkInstance vulkan_instance, VkAllocationCallbacks *vulkan_allocation_callbacks )
{
  VkSurfaceKHR handle;
  if ( !SDL_Vulkan_CreateSurface( window_handle->value, vulkan_instance, vulkan_allocation_callbacks, &handle ) )
  {
    CRUDE_ABORT( CRUDE_CHANNEL_GRAPHICS, "failed to create vk_surface: %s", SDL_GetError() );
    return VK_NULL_HANDLE;
  }
  return handle;
}

static int32 get_supported_queue_family_index( VkPhysicalDevice vulkan_physical_device, VkSurfaceKHR vulkan_surface )
{
  uint32 queue_family_count = 0u;
  vkGetPhysicalDeviceQueueFamilyProperties( vulkan_physical_device, &queue_family_count, NULL );
  if ( queue_family_count == 0u )
  {
    return -1;
  }
  
  VkQueueFamilyProperties *queue_families_properties = NULL;
  arrsetlen( queue_families_properties, queue_family_count ); // tofree
  vkGetPhysicalDeviceQueueFamilyProperties( vulkan_physical_device, &queue_family_count, queue_families_properties );
  
  int32 queue_index = -1;
  for ( uint32 i = 0; i < queue_family_count; ++i )
  {
    if ( queue_families_properties[i].queueCount > 0 && queue_families_properties[i].queueFlags & ( VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT ) )
    {
      VkBool32 surface_supported = false;
      vkGetPhysicalDeviceSurfaceSupportKHR( vulkan_physical_device, i, vulkan_surface, &surface_supported );
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

static bool check_support_required_extensions( VkPhysicalDevice vulkan_physical_device )
{
  uint32 available_extensions_count = 0u;
  vkEnumerateDeviceExtensionProperties( vulkan_physical_device, NULL, &available_extensions_count, NULL );
  if ( available_extensions_count == 0u)
  {
    return false;
  }
    
  VkExtensionProperties *available_extensions = NULL;
  arrsetlen( available_extensions, available_extensions_count ); // tofree
  vkEnumerateDeviceExtensionProperties( vulkan_physical_device, NULL, &available_extensions_count, available_extensions );

  bool support_required_extensions = true;
  for ( uint32 i = 0; i < ARRAY_SIZE( device_required_extensions ); ++i )
  {
    bool extension_found = false;
    for ( uint32 k = 0; k < available_extensions_count; ++k )
    {
      if ( strcmp( device_required_extensions[i], available_extensions[k].extensionName ) == 0 )
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

static bool check_swap_chain_adequate( VkPhysicalDevice vulkan_physical_device, VkSurfaceKHR vulkan_surface )
{
  uint32 formats_count;
  vkGetPhysicalDeviceSurfaceFormatsKHR( vulkan_physical_device, vulkan_surface, &formats_count, NULL );
  if ( formats_count == 0u )
  {
    return false;
  }

  uint32 presents_mode_count;
  vkGetPhysicalDeviceSurfacePresentModesKHR( vulkan_physical_device, vulkan_surface, &presents_mode_count, NULL );
  if ( presents_mode_count == 0u ) 
  {
    return false;
  }

  return true;
}

static bool check_support_required_features( VkPhysicalDevice vulkan_physical_device )
{
  VkPhysicalDeviceFeatures features;
  vkGetPhysicalDeviceFeatures( vulkan_physical_device, &features );
  return features.samplerAnisotropy;
}

static VkPhysicalDevice pick_physical_device( VkInstance vulkan_instance, VkSurfaceKHR vulkan_surface, int32 *vulkan_selected_queue_family_index )
{
  CRUDE_ASSERT( vulkan_selected_queue_family_index );

  uint32 physical_devices_count = 0u;
  vkEnumeratePhysicalDevices( vulkan_instance, &physical_devices_count, NULL );

  if ( physical_devices_count == 0u ) 
  {
    return VK_NULL_HANDLE;
  }

  VkPhysicalDevice *physical_devices = NULL;
  arrput( physical_devices, physical_devices_count ); // tofree
  vkEnumeratePhysicalDevices( vulkan_instance, &physical_devices_count, physical_devices );

  VkPhysicalDevice selected_physical_devices = VK_NULL_HANDLE;
  for ( uint32 physical_device_index = 0; physical_device_index < physical_devices_count; ++physical_device_index )
  {
    VkPhysicalDevice physical_device = physical_devices[physical_device_index];
    
    if ( !check_support_required_extensions( physical_device ) )
    {
      continue;
    }
    if ( !check_swap_chain_adequate( physical_device, vulkan_surface ) )
    {
      continue;
    }
    if ( !check_support_required_features( physical_device ) )
    {
      continue;
    }
    int32 queue_family_index = get_supported_queue_family_index( physical_device, vulkan_surface ); 
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

static VkDevice create_device( VkPhysicalDevice vulkan_physical_device, int32 vulkan_queue_family_index, VkAllocationCallbacks *vulkan_allocation_callbacks )
{
  float const queue_priority[] = { 1.0f };
  VkDeviceQueueCreateInfo queue_info[1];
  memset( queue_info, 0, sizeof( queue_info) );
  queue_info[0].sType            = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
  queue_info[0].queueFamilyIndex = vulkan_queue_family_index;
  queue_info[0].queueCount       = 1;
  queue_info[0].pQueuePriorities = queue_priority;
 
  VkPhysicalDeviceFeatures2 physical_features2 = { VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2 };
  vkGetPhysicalDeviceFeatures2( vulkan_physical_device, &physical_features2 );

  VkDeviceCreateInfo device_create_info = ( VkDeviceCreateInfo ) {
    .sType                    = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
    .pNext                    = &physical_features2,
    .flags                    = 0u,
    .queueCreateInfoCount     = ARRAY_SIZE( queue_info ),
    .pQueueCreateInfos        = queue_info,
    .pEnabledFeatures         = NULL,
    .enabledExtensionCount    = ARRAY_SIZE( device_required_extensions ),
    .ppEnabledExtensionNames  = device_required_extensions,
    .enabledLayerCount        = ARRAY_SIZE( required_layers ),
    .ppEnabledLayerNames      = required_layers,
  };
  VkDevice device;
  HANDLE_VULKAN_RESULT( vkCreateDevice( vulkan_physical_device, &device_create_info, vulkan_allocation_callbacks, &device ), "failed to create logic device!" );
  return device;
}

static VkSwapchainKHR create_swapchain( 
  VkDevice                 vulkan_device, 
  VkPhysicalDevice         vulkan_physical_device, 
  VkSurfaceKHR             vulkan_surface, 
  int32                    vulkan_queue_family_index,
  VkAllocationCallbacks   *vulkan_allocation_callbacks,
  uint32                  *vulkan_swapchain_images_count,
  VkImage                 *vulkan_swapchain_images,
  VkImageView             *vulkan_swapchain_images_views )
{
  VkSurfaceCapabilitiesKHR surface_capabilities;
  vkGetPhysicalDeviceSurfaceCapabilitiesKHR( vulkan_physical_device, vulkan_surface, &surface_capabilities);
  
  VkExtent2D swapchain_extent = surface_capabilities.currentExtent;
  if ( swapchain_extent.width == UINT32_MAX )
  {
    swapchain_extent.width = CLAMP( swapchain_extent.width, surface_capabilities.minImageExtent.width, surface_capabilities.maxImageExtent.width );
    swapchain_extent.height = CLAMP( swapchain_extent.height, surface_capabilities.minImageExtent.height, surface_capabilities.maxImageExtent.height );
  }
  
  uint32 available_formats_count;
  vkGetPhysicalDeviceSurfaceFormatsKHR( vulkan_physical_device, vulkan_surface, &available_formats_count, NULL );

  if ( available_formats_count == 0u )
  {
    CRUDE_LOG_ERROR( CRUDE_CHANNEL_GRAPHICS, "Can't find available surface format" );
    return VK_NULL_HANDLE;
  }

  VkSurfaceFormatKHR *available_formats = NULL;
  arrsetlen( available_formats, available_formats_count ); // tofree
  vkGetPhysicalDeviceSurfaceFormatsKHR( vulkan_physical_device, vulkan_surface, &available_formats_count, available_formats );

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
  vkGetPhysicalDeviceSurfacePresentModesKHR( vulkan_physical_device, vulkan_surface, &available_present_modes_count, NULL );
  if ( available_present_modes_count == 0u ) 
  {
    CRUDE_LOG_ERROR( CRUDE_CHANNEL_GRAPHICS, "Can't find available surface present_mode" );
    arrfree( available_formats );
    return VK_NULL_HANDLE;
  }
  
  VkPresentModeKHR *available_present_modes = NULL;
  arrsetlen( available_present_modes, available_present_modes_count ); // tofree
  vkGetPhysicalDeviceSurfacePresentModesKHR( vulkan_physical_device, vulkan_surface, &available_present_modes_count, available_present_modes );
  
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
  uint32 const queue_family_indices[] = { vulkan_queue_family_index };
  
  VkSwapchainCreateInfoKHR swapchain_create_info = ( VkSwapchainCreateInfoKHR ) {
    .sType                  = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
    .pNext                  = NULL,
    .surface                = vulkan_surface,
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
  HANDLE_VULKAN_RESULT( vkCreateSwapchainKHR( vulkan_device, &swapchain_create_info, vulkan_allocation_callbacks, &vulkan_swapchain ), "failed to create swapchain!" );

  vkGetSwapchainImagesKHR( vulkan_device, vulkan_swapchain, vulkan_swapchain_images_count, NULL );
  vkGetSwapchainImagesKHR( vulkan_device, vulkan_swapchain, vulkan_swapchain_images_count, vulkan_swapchain_images );
  
  for ( uint32 i = 0; i < *vulkan_swapchain_images_count; ++i )
  {
    VkImageViewCreateInfo image_view_info = ( VkImageViewCreateInfo ) { 
      .sType                       = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
      .viewType                    = VK_IMAGE_VIEW_TYPE_2D,
      .format                      = vulkan_surface_format.format,
      .image                       = vulkan_swapchain_images[ i ],
      .subresourceRange.levelCount = 1,
      .subresourceRange.layerCount = 1,
      .subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
      .components.r                = VK_COMPONENT_SWIZZLE_R,
      .components.g                = VK_COMPONENT_SWIZZLE_G,
      .components.b                = VK_COMPONENT_SWIZZLE_B,
      .components.a                = VK_COMPONENT_SWIZZLE_A,
    };
    
    HANDLE_VULKAN_RESULT( vkCreateImageView( vulkan_device, &image_view_info, vulkan_allocation_callbacks, &vulkan_swapchain_images_views[ i ] ), "Failed to create image view for swapchain image" );
  }

  arrfree( available_formats );
  arrfree( available_present_modes );

  return vulkan_swapchain;
}

static void destroy_swapchain(
  VkDevice                 vulkan_device,
  VkSwapchainKHR           vulkan_swapchain,
  uint32                   vulkan_swapchain_images_count,
  VkImageView             *vulkan_swapchain_images_views,
  VkAllocationCallbacks   *vulkan_allocation_callbacks )
{
  for ( uint32 i = 0; i < vulkan_swapchain_images_count; ++i )
  {
    vkDestroyImageView( vulkan_device, vulkan_swapchain_images_views[ i ], vulkan_allocation_callbacks );
  }
 
  vkDestroySwapchainKHR( vulkan_device, vulkan_swapchain, vulkan_allocation_callbacks );
}

static void initialize_render_core( ecs_iter_t *it  )
{
  crude_render_core_config *config = ecs_field( it, crude_render_core_config, 0 );
  crude_window_handle *window_handle = ecs_field( it, crude_window_handle, 1 );

  for ( uint32 i = 0; i < it->count; ++i )
  {
    crude_render_core *core = ecs_ensure( it->world, it->entities[i], crude_render_core );
    core->vulkan_allocation_callbacks = NULL;
    core->vulkan_instance = create_instance( config[i].application_name, config[i].application_version, core->vulkan_allocation_callbacks );
    core->vulkan_debug_utils_messenger = create_debug_utils_messsenger( core->vulkan_instance, core->vulkan_allocation_callbacks );
    core->vulkan_surface = create_surface( &window_handle[i], core->vulkan_instance, core->vulkan_allocation_callbacks );
    core->vulkan_physical_device = pick_physical_device( core->vulkan_instance, core->vulkan_surface, &core->vulkan_queue_family_index );
    core->vulkan_device = create_device( core->vulkan_physical_device, core->vulkan_queue_family_index, core->vulkan_allocation_callbacks );
    vkGetDeviceQueue( core->vulkan_device, core->vulkan_queue_family_index, 0u, &core->vulkan_queue );
    core->vulkan_swapchain = create_swapchain( core->vulkan_device, core->vulkan_physical_device, core->vulkan_surface, core->vulkan_queue_family_index, core->vulkan_allocation_callbacks, &core->vulkan_swapchain_images_count, core->vulkan_swapchain_images, core->vulkan_swapchain_images_views );
  }
}

static void deinitialize_render_core( ecs_iter_t *it )
{
  crude_render_core *core = ecs_field( it, crude_render_core, 0 );

  for ( uint32 i = 0; i < it->count; ++i )
  {
    destroy_swapchain( core[i].vulkan_device, core[i].vulkan_swapchain, core[i].vulkan_swapchain_images_count, core[i].vulkan_swapchain_images_views, core[i].vulkan_allocation_callbacks );
    vkDestroySwapchainKHR( core[i].vulkan_device, core[i].vulkan_swapchain, core[i].vulkan_allocation_callbacks );
    vkDestroyDevice( core[i].vulkan_device, core[i].vulkan_allocation_callbacks );
    vkDestroySurfaceKHR( core[i].vulkan_instance, core[i].vulkan_surface, core[i].vulkan_allocation_callbacks );
    PFN_vkDestroyDebugUtilsMessengerEXT vkDestroyDebugUtilsMessengerEXT = vkGetInstanceProcAddr( core[i].vulkan_instance, "vkDestroyDebugUtilsMessengerEXT" );
    vkDestroyDebugUtilsMessengerEXT( core[i].vulkan_instance, core[i].vulkan_debug_utils_messenger, core[i].vulkan_allocation_callbacks );
    vkDestroyInstance( core[i].vulkan_instance, core[i].vulkan_allocation_callbacks );
  }
}

void crude_render_systemImport( ecs_world_t *world )
{
  ECS_MODULE( world, crude_render_system );
  ECS_IMPORT( world, crude_render_core_components );
 
  ecs_observer( world, {
    .query.terms = { 
      ( ecs_term_t ) { .id = ecs_id( crude_render_core_config ), .oper = EcsAnd },
      ( ecs_term_t ) { .id = ecs_id( crude_window_handle ), .oper = EcsAnd },
      ( ecs_term_t ) { .id = ecs_id( crude_render_core ), .oper = EcsNot }
    },
    .events = { EcsOnSet },
    .callback = initialize_render_core
    });
  ecs_observer( world, {
    .query.terms = { { ecs_id( crude_render_core ) } },
    .events = { EcsOnRemove },
    .callback = deinitialize_render_core
    } );
}