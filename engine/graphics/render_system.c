#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>
#include <stb_ds.h>

#include <gui/gui.h>
#include <platform/sdl_system.h>
#include <graphics/render_core.h>
#include <core/assert.h>
#include <core/utils.h>

#include <graphics/render_system.h>

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

static void handle_vk_result( VkResult result, char const* msg )
{
  if ( result != VK_SUCCESS )
  {
    CRUDE_ABORT( CRUDE_CHANNEL_GRAPHICS, "vulkan result isn't success: %i %s", result, msg );
  }
}

static VKAPI_ATTR VkBool32 debug_callback(
  VkDebugUtilsMessageSeverityFlagBitsEXT      messageSeverity,
  VkDebugUtilsMessageTypeFlagsEXT             messageType,
  VkDebugUtilsMessengerCallbackDataEXT const *pCallbackData,
  void                                       *pUserData)
{
  if ( messageSeverity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT )
  {
    CRUDE_LOG_ERROR( CRUDE_CHANNEL_GRAPHICS, "validation layer: %s", pCallbackData->pMessage );
  }
  else if ( messageSeverity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT )
  {
    CRUDE_LOG_WARNING( CRUDE_CHANNEL_GRAPHICS, "validation layer: %s", pCallbackData->pMessage );
  }
  return VK_FALSE;
}

static VkInstance create_instance(
  char const            *application_name,
  uint32                 application_version,
  VkAllocationCallbacks *allocation_callbacks)
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
  handle_vk_result( vkCreateInstance( &instance_create_info, allocation_callbacks, &handle ), "failed to create instance" );

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
  CRUDE_ASSERT( vkCreateDebugUtilsMessengerEXT );
  handle_vk_result( vkCreateDebugUtilsMessengerEXT( instance, &create_info, allocation_callbacks, &handle ), "failed to create debug utils messenger" );
  return handle;
}

static VkSurfaceKHR create_surface( crude_window_handle *window_handle, VkInstance instance, VkAllocationCallbacks *allocation_callbacks )
{
  VkSurfaceKHR handle;
  if ( !SDL_Vulkan_CreateSurface( window_handle->value, instance, allocation_callbacks, &handle ) )
  {
    CRUDE_ABORT( CRUDE_CHANNEL_GRAPHICS, "failed to create vk_surface: %s", SDL_GetError() );
    return VK_NULL_HANDLE;
  }
  return handle;
}

static bool check_queues_support_graphics_and_present( VkPhysicalDevice physical_device, VkSurfaceKHR surface )
{
  uint32 queue_family_count = 0u;
  vkGetPhysicalDeviceQueueFamilyProperties( physical_device, &queue_family_count, NULL );
  if ( queue_family_count == 0u )
  {
    return false;
  }
  
  VkQueueFamilyProperties *queue_families_properties = NULL;
  arrsetlen( queue_families_properties, queue_family_count ); // tofree
  vkGetPhysicalDeviceQueueFamilyProperties( physical_device, &queue_family_count, queue_families_properties );
  
  bool support_graphics = false;
  bool support_present = false;
  for ( uint32 i = 0; i < queue_family_count; ++i )
  {
    if ( queue_families_properties[i].queueFlags & VK_QUEUE_GRAPHICS_BIT )
    {
      support_graphics = true;
    }
    VkBool32 vk_support_surface = false;
    vkGetPhysicalDeviceSurfaceSupportKHR( physical_device, i, surface, &vk_support_surface );
    if ( vk_support_surface )
    {
      support_present = true;
    }
  }
  arrfree( queue_families_properties );
  return support_graphics && support_present;
}

static bool check_support_required_extensions( VkPhysicalDevice physical_device )
{
  uint32 available_extensions_count = 0u;
  vkEnumerateDeviceExtensionProperties( physical_device, NULL, &available_extensions_count, NULL );
  if ( available_extensions_count == 0u)
  {
    return false;
  }
    
  VkExtensionProperties *available_extensions = NULL;
  arrsetlen( available_extensions, available_extensions_count ); // tofree
  vkEnumerateDeviceExtensionProperties( physical_device, NULL, &available_extensions_count, available_extensions );
  
  bool support_required_extensions = true;
  for ( uint32 i = 0; i < ARRAY_SIZE( device_required_extensions ); ++i )
  {
    bool extension_found = false;
    for ( uint32 k = 0; k < available_extensions_count; ++k )
    {
      if ( strcmp( device_required_extensions[i], available_extensions[i].extensionName ) )
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

static bool check_swap_chain_adequate( VkPhysicalDevice physical_device, VkSurfaceKHR surface )
{
  uint32 formats_count;
  vkGetPhysicalDeviceSurfaceFormatsKHR( physical_device, surface, &formats_count, NULL );
  if ( formats_count == 0u )
  {
    return false;
  }

  uint32 presents_mode_count;
  vkGetPhysicalDeviceSurfacePresentModesKHR( physical_device, surface, &presents_mode_count, NULL );
  if ( presents_mode_count == 0u ) 
  {
    return false;
  }

  return true;
}

static bool check_support_required_features( VkPhysicalDevice physical_device )
{
  VkPhysicalDeviceFeatures features;
  vkGetPhysicalDeviceFeatures( physical_device, &features );
  return features.samplerAnisotropy;
}

static VkPhysicalDevice pick_physical_device( VkInstance instance, VkSurfaceKHR surface )
{
  uint32 physical_devices_count = 0u;
  vkEnumeratePhysicalDevices( instance, &physical_devices_count, NULL );

  if ( physical_devices_count == 0u ) 
  {
    return VK_NULL_HANDLE;
  }

  VkPhysicalDevice *physical_devices = NULL;
  arrput( physical_devices, physical_devices_count ); // tofree
  vkEnumeratePhysicalDevices( instance, &physical_devices_count, physical_devices );

  VkPhysicalDevice selected_physical_devices = VK_NULL_HANDLE;
  for ( uint32 physical_device_index = 0; physical_device_index < physical_devices_count; ++physical_device_index )
  {
    VkPhysicalDevice physical_device = physical_devices[physical_device_index];
    if ( !check_queues_support_graphics_and_present( physical_device, surface ) )
    {
      continue;
    }
    if ( !check_support_required_extensions( physical_device ) )
    {
      continue;
    }
    if ( !check_swap_chain_adequate( physical_device, surface ) )
    {
      continue;
    }
    if ( !check_support_required_features( physical_device ) )
    {
      continue;
    }
    
    selected_physical_devices = physical_device;
    break;
  }
  
  VkPhysicalDeviceProperties properties;
  vkGetPhysicalDeviceProperties( selected_physical_devices, &properties );
  CRUDE_LOG_INFO( CRUDE_CHANNEL_GRAPHICS, "Selected physical device %s %i", properties.deviceName, properties.deviceType );

  return selected_physical_devices;
}

static void initialize_render_core( ecs_iter_t *it  )
{
  crude_render_core_config *config = ecs_field( it, crude_render_core_config, 0 );
  crude_window_handle *window_handle = ecs_field( it, crude_window_handle, 1 );

  for ( uint32 i = 0; i < it->count; ++i )
  {
    crude_render_core *core = ecs_ensure( it->world, it->entities[i], crude_render_core );
    core->allocation_callbacks = config[i].allocation_callbacks;
    core->instance = create_instance( config[i].application_name, config[i].application_version, core->allocation_callbacks );
    core->debug_utils_messenger = create_debug_utils_messsenger( core->instance, core->allocation_callbacks );
    core->surface = create_surface( &window_handle[i], core->instance, core->allocation_callbacks );
    core->physical_device = pick_physical_device( core->instance, core->surface );
  }
}

static void deinitialize_render_core( ecs_iter_t *it )
{
  crude_render_core *core = ecs_field( it, crude_render_core, 0 );

  for ( uint32 i = 0; i < it->count; ++i )
  {
    vkDestroySurfaceKHR( core->instance, core->surface, core->allocation_callbacks );

    PFN_vkDestroyDebugUtilsMessengerEXT vkDestroyDebugUtilsMessengerEXT = vkGetInstanceProcAddr( core->instance, "vkDestroyDebugUtilsMessengerEXT" );
    CRUDE_ASSERT( vkDestroyDebugUtilsMessengerEXT );
    if ( vkDestroyDebugUtilsMessengerEXT )
    {
      vkDestroyDebugUtilsMessengerEXT( core->instance, core->debug_utils_messenger, core->allocation_callbacks );
    }

    vkDestroyInstance( core->instance, core->allocation_callbacks );
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