#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>
#include <stb_ds.h>

#include <platform/sdl_system.h>
#include <graphics/render_core.h>
#include <core/assert.h>
#include <core/utils.h>

#include <graphics/render_system.h>

void handle_vk_result( VkResult result, char const* msg )
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
    CRUDE_ABORT( CRUDE_CHANNEL_GRAPHICS, "validation layer: %s", pCallbackData->pMessage );
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
  VkAllocationCallbacks *vk_allocation_callbacks)
{
  // extensions
  uint32 surface_extensions_count;
  char const *const *surface_extensions_array = SDL_Vulkan_GetInstanceExtensions( &surface_extensions_count );
  char const *const *debug_extensions[] = { VK_EXT_DEBUG_UTILS_EXTENSION_NAME };
  uint32 const debug_extensions_count = ARRAY_SIZE( debug_extensions );

  char const **instance_enabled_extensions = NULL;
  uint32 const instance_enabled_extensions_count = surface_extensions_count + debug_extensions_count;
  arrput( instance_enabled_extensions, instance_enabled_extensions_count ); // tofree
  CRUDE_ASSERT( instance_enabled_extensions );

  for ( uint32 i = 0; i < surface_extensions_count; ++i )
  {
    instance_enabled_extensions[i] = surface_extensions_array[i];
  }
  for ( uint32 i = 0; i < debug_extensions_count; ++i )
  {
    instance_enabled_extensions[surface_extensions_count + i] = debug_extensions[i];
  }

  // layers
  char const *instance_enabled_layers[] = { "VK_LAYER_KHRONOS_validation" };

  // application
  VkApplicationInfo vk_application = ( VkApplicationInfo ) {
    .pApplicationName   = application_name,
    .applicationVersion = application_version,
    .pEngineName        = "crude_engine",
    .engineVersion      = VK_MAKE_VERSION( 1, 0, 0 ),
    .apiVersion         = VK_API_VERSION_1_0 
  };

  // initialize instance & debug_utils_messenger
  VkInstanceCreateInfo vk_instance_create_info;
  memset( &vk_instance_create_info, 0u, sizeof( vk_instance_create_info ) );
  vk_instance_create_info.sType                    = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
  vk_instance_create_info.pApplicationInfo         = &vk_application;
  vk_instance_create_info.flags                    = 0u;
  vk_instance_create_info.ppEnabledExtensionNames  = instance_enabled_extensions;
  vk_instance_create_info.enabledExtensionCount    = instance_enabled_extensions_count;
  vk_instance_create_info.ppEnabledLayerNames     = instance_enabled_layers;
  vk_instance_create_info.enabledLayerCount       = ARRAY_SIZE( instance_enabled_layers );
#ifdef VK_EXT_debug_utils
  VkDebugUtilsMessengerCreateInfoEXT vk_debug_create_info;
  memset( &vk_debug_create_info, 0u, sizeof( vk_debug_create_info ) );
  vk_debug_create_info.sType            = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
  vk_debug_create_info.pNext            = NULL;
  vk_debug_create_info.flags            = 0u;
  vk_debug_create_info.pfnUserCallback  = debug_callback;
  vk_debug_create_info.pUserData        = NULL;
  vk_debug_create_info.messageSeverity =
    VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
    VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT |
    VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
    VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
  vk_debug_create_info.messageType =
    VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
    VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
    VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
  vk_debug_create_info.pNext = &vk_debug_create_info;
#else // VK_EXT_debug_utils
  vk_debug_create_info.pNext = nullptr;
#endif // VK_EXT_debug_utils
  
  VkInstance vk_instance;
  handle_vk_result( vkCreateInstance( &vk_instance_create_info, vk_allocation_callbacks, &vk_instance ), "failed to create instance" );

  arrfree( instance_enabled_extensions );

  return vk_instance;
}


static VkDebugUtilsMessengerEXT create_debug_utils_messsenger( VkInstance vk_instance, VkAllocationCallbacks *vk_allocation_callbacks )
{
  VkDebugUtilsMessengerCreateInfoEXT vk_create_info = ( VkDebugUtilsMessengerCreateInfoEXT) {
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
  PFN_vkCreateDebugUtilsMessengerEXT vkCreateDebugUtilsMessengerEXT = vkGetInstanceProcAddr( vk_instance, "vkCreateDebugUtilsMessengerEXT" );
  if ( !vkCreateDebugUtilsMessengerEXT )
  {
    CRUDE_ABORT( CRUDE_CHANNEL_GRAPHICS, "failed to get vkCreateDebugUtilsMessengerEXT " );
    return VK_NULL_HANDLE;
  }

  handle_vk_result( vkCreateDebugUtilsMessengerEXT( vk_instance, &vk_create_info, vk_allocation_callbacks, &handle ), "failed to create debug utils messenger" );
  return handle;
}

static void initialize_render_core( ecs_iter_t *it  )
{
  crude_render_core_config *config = ecs_field( it, crude_render_core_config, 0 );

  for ( uint32 i = 0; i < it->count; ++i )
  {
    crude_render_core *core = ecs_ensure( it->world, it->entities[i], crude_render_core );
    core->vk_allocation_callbacks = config->vk_allocation_callbacks;
    core->vk_instance = create_instance( config->application_name, config->application_version, core->vk_allocation_callbacks );
    core->vk_debug_utils_messenger = create_debug_utils_messsenger( core->vk_instance, core->vk_allocation_callbacks );
  }
}

static void deinitialize_render_core( ecs_iter_t *it )
{
  crude_render_core *core = ecs_field( it, crude_render_core, 0 );

  for ( uint32 i = 0; i < it->count; ++i )
  {
    PFN_vkDestroyDebugUtilsMessengerEXT vkDestroyDebugUtilsMessengerEXT = vkGetInstanceProcAddr( core->vk_instance, "vkDestroyDebugUtilsMessengerEXT" );
    CRUDE_ASSERTM( CRUDE_CHANNEL_GRAPHICS, vkDestroyDebugUtilsMessengerEXT, "failed to get vkDestroyDebugUtilsMessengerEXT " );
    if ( vkDestroyDebugUtilsMessengerEXT )
    {
      vkDestroyDebugUtilsMessengerEXT( core->vk_instance, core->vk_debug_utils_messenger, core->vk_allocation_callbacks );
    }

    vkDestroyInstance( core->vk_instance, core->vk_allocation_callbacks );
  }
}

void crude_render_systemImport( ecs_world_t *world )
{
  ECS_MODULE( world, crude_render_system );
  ECS_IMPORT( world, crude_render_core_components );
 
  ecs_observer( world, {
    .query.terms = { 
      ( ecs_term_t ) { .id = ecs_id( crude_render_core_config ) },
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