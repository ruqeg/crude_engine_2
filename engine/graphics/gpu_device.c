#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>

#include <core/profiler.h>
#include <core/algorithms.h>
#include <core/math.h>
#include <core/assert.h>

#include <graphics/gpu_device.h>


/************************************************
 *
 * Constants
 * 
 ***********************************************/

#define MAX_BINDLESS_RESOURCES 1024
#define BINDLESS_TEXTURE_BINDING 10

static char const *const vk_device_required_extensions[] = 
{ 
  VK_KHR_SWAPCHAIN_EXTENSION_NAME,
  VK_KHR_MAINTENANCE_1_EXTENSION_NAME,
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


/************************************************
 *
 * Global variables
 * 
 ***********************************************/
static crude_gfx_cmd_buffer_manager g_command_buffer_manager;


/************************************************
 *
 * Local Vulkan Helper Functions Declaration.
 * 
 ***********************************************/
static VKAPI_ATTR VkBool32
_vk_debug_callback
(
  _In_ VkDebugUtilsMessageSeverityFlagBitsEXT        messageSeverity,
  _In_ VkDebugUtilsMessageTypeFlagsEXT               messageType,
  _In_ VkDebugUtilsMessengerCallbackDataEXT const   *pCallbackData,
  _In_ void                                         *pUserData
);
static VkInstance
_vk_create_instance
(
  _In_     char const                               *vk_application_name,
  _In_     uint32                                    vk_application_version,
  _In_opt_ VkAllocationCallbacks                    *vk_allocation_callbacks
);
static VkDebugUtilsMessengerEXT
_vk_create_debug_utils_messsenger
(
  _In_     VkInstance                                instance,
  _In_opt_ VkAllocationCallbacks                    *allocation_callbacks
);
static VkSurfaceKHR
_vk_create_surface
(
  _In_     SDL_Window                               *sdl_window,
  _In_     VkInstance                                vk_instance,
  _In_opt_ VkAllocationCallbacks                    *vk_allocation_callbacks
);
static int32
_vk_get_supported_queue_family_index
(
  _In_ VkPhysicalDevice                              vk_physical_device,
  _In_ VkSurfaceKHR                                  vk_surface
);
static bool
_vk_check_support_required_extensions
(
  _In_ VkPhysicalDevice                              vk_physical_device
);
static bool
_vk_check_swap_chain_adequate
(
  _In_ VkPhysicalDevice                              vk_physical_device,
  _In_ VkSurfaceKHR                                  vk_surface
);
static bool
_vk_check_support_required_features
(
  _In_ VkPhysicalDevice                              vk_physical_device
);
static VkPhysicalDevice
_vk_pick_physical_device
(
  _In_  VkInstance                                   vk_instance,
  _In_  VkSurfaceKHR                                 vk_surface
);
static VkDevice
_vk_create_device
(
  _In_ VkPhysicalDevice                              vk_physical_device,
  _Out_ VkQueue                                     *vk_main_queue,
  _Out_ VkQueue                                     *vk_transfer_queue,
  _Out_ uint32                                      *vk_main_queue_family,
  _Out_ uint32                                      *vk_transfer_queue_family,
  _In_opt_ VkAllocationCallbacks                    *vk_allocation_callbacks
);
static VkSwapchainKHR
_vk_create_swapchain
( 
  _In_      VkDevice                                 vk_device, 
  _In_      VkPhysicalDevice                         vk_physical_device, 
  _In_      VkSurfaceKHR                             vk_surface, 
  _In_      int32                                    vk_queue_family_index,
  _In_opt_  VkAllocationCallbacks                   *vk_allocation_callbacks,
  _Out_     uint32                                  *vk_swapchain_images_count,
  _Out_     VkImage                                 *vk_swapchain_images,
  _Out_     VkImageView                             *vk_swapchain_images_views,
  _Out_     VkSurfaceFormatKHR                      *vk_surface_format,
  _Out_     uint16                                  *vk_swapchain_width,
  _Out_     uint16                                  *vk_swapchain_height
);
static VmaAllocation
_vk_create_vma_allocator
(
  _In_ VkDevice                                      vk_device,
  _In_ VkPhysicalDevice                              vk_physical_device,
  _In_ VkInstance                                    vk_instance
);
static void
_vk_create_descriptor_pool
(
  _In_     VkDevice                                  vk_device,
  _In_opt_ VkAllocationCallbacks                    *vk_allocation_callbacks,
  _Out_    VkDescriptorPool                         *vk_bindless_descriptor_pool,
  _Out_    VkDescriptorSetLayout                    *vk_bindless_descriptor_set_layout,
  _Out_    VkDescriptorSet                          *vk_bindless_descriptor_set
);
static VkQueryPool
_vk_create_timestamp_query_pool
(
  _In_     VkDevice                                  vk_device, 
  _In_     int32                                     max_frames,
  _In_opt_ VkAllocationCallbacks                    *vk_allocation_callbacks
);
static void
_vk_create_swapchain_pass
(
  _In_ crude_gfx_device                             *gpu,
  _In_ crude_gfx_render_pass_creation                   *creation,
  _Out_ crude_gfx_render_pass                           *render_pass
);
static void
_vk_destroy_swapchain
(
  _In_     VkDevice                                  vk_device,
  _In_     VkSwapchainKHR                            vulkan_swapchain,
  _In_     uint32                                    vk_swapchain_images_count,
  _In_     VkImageView                              *vk_swapchain_images_views,
  _In_     VkFramebuffer                            *vk_swapchain_framebuffers,
  _In_opt_ VkAllocationCallbacks                    *vk_allocation_callbacks
);
static void
_vk_create_texture
(
  _In_ crude_gfx_device                             *gpu,
  _In_ crude_gfx_texture_creation const                 *creation,
  _In_ crude_gfx_texture_handle                          handle,
  _In_ crude_gfx_texture                                *texture
);
static void
_vk_resize_texture
(
  _In_ crude_gfx_device                             *gpu,
  _In_ crude_gfx_texture                                *texture,
  _In_ crude_gfx_texture                                *texture_to_delete,
  _In_ uint16                                        width,
  _In_ uint16                                        height,
  _In_ uint16                                        depth
);
static void
_vk_resize_swapchain
(
  _In_ crude_gfx_device                             *gpu
);
static inline crude_gfx_vertex_component_format
_reflect_format_to_vk_format
(
  _In_ SpvReflectFormat                                    format
);
static void
_vk_reflect_shader
(
  _In_ crude_gfx_device                                   *gpu,
  _In_ void                                               *code,
  _In_ uint32                                              code_size,
  _In_ crude_gfx_shader_reflect                           *reflect
);
static void
_vk_destroy_resources_instant
(
  _In_ crude_gfx_device                             *gpu,
  _In_ crude_gfx_resource_deletion_type                  type,
  _In_ crude_gfx_resource_index                         handle
);

/************************************************
 *
 * GPU Device Initialize/Deinitialize
 * 
 ***********************************************/
void
crude_gfx_initialize_device
(
  _Out_ crude_gfx_device                                  *gpu,
  _In_ crude_gfx_device_creation                          *creation
)
{
  gpu->sdl_window = creation->sdl_window;
  gpu->allocator  = creation->allocator;
  gpu->vk_allocation_callbacks = NULL;
  gpu->max_frames = creation->max_frames;
  gpu->vk_instance = _vk_create_instance( creation->vk_application_name, creation->vk_application_version, gpu->vk_allocation_callbacks );
  gpu->vk_debug_utils_messenger = _vk_create_debug_utils_messsenger( gpu->vk_instance, gpu->vk_allocation_callbacks );
  gpu->vk_surface = _vk_create_surface( creation->sdl_window, gpu->vk_instance, gpu->vk_allocation_callbacks );
  gpu->vk_physical_device = _vk_pick_physical_device( gpu->vk_instance, gpu->vk_surface );
  gpu->vk_device = _vk_create_device( gpu->vk_physical_device, &gpu->vk_main_queue, &gpu->vk_transfer_queue, &gpu->vk_main_queue_family, &gpu->vk_transfer_queue_family, gpu->vk_allocation_callbacks );
  gpu->vk_swapchain = _vk_create_swapchain( gpu->vk_device, gpu->vk_physical_device, gpu->vk_surface, gpu->vk_main_queue_family, gpu->vk_allocation_callbacks, &gpu->vk_swapchain_images_count, gpu->vk_swapchain_images, gpu->vk_swapchain_images_views, &gpu->vk_surface_format, &gpu->vk_swapchain_width, &gpu->vk_swapchain_height );
  gpu->vma_allocator = _vk_create_vma_allocator( gpu->vk_device, gpu->vk_physical_device, gpu->vk_instance );
  _vk_create_descriptor_pool( gpu->vk_device, gpu->vk_allocation_callbacks, &gpu->vk_bindless_descriptor_pool, &gpu->vk_bindless_descriptor_set_layout, &gpu->vk_bindless_descriptor_set );
  gpu->vk_timestamp_query_pool = _vk_create_timestamp_query_pool( gpu->vk_device, gpu->max_frames, gpu->vk_allocation_callbacks );
  crude_initialize_resource_pool( &gpu->buffers, gpu->allocator, 4096, sizeof( crude_gfx_buffer ) );
  crude_initialize_resource_pool( &gpu->textures, gpu->allocator, 512, sizeof( crude_gfx_texture ) );
  crude_initialize_resource_pool( &gpu->render_passes, gpu->allocator, 256, sizeof( crude_gfx_render_pass ) );
  crude_initialize_resource_pool( &gpu->descriptor_set_layouts, gpu->allocator, 128, sizeof( crude_gfx_descriptor_set_layout ) );
  crude_initialize_resource_pool( &gpu->pipelines, gpu->allocator, 128, sizeof( crude_gfx_pipeline ) );
  crude_initialize_resource_pool( &gpu->shaders, gpu->allocator, 128, sizeof( crude_gfx_shader_state ) );
  crude_initialize_resource_pool( &gpu->samplers, gpu->allocator, 32, sizeof( crude_gfx_sampler ) );
  
  VkSemaphoreCreateInfo semaphore_info = { .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO };
  VkFenceCreateInfo fence_info = { .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO, .flags = VK_FENCE_CREATE_SIGNALED_BIT };
  for ( uint32 i = 0; i < CRUDE_GFX_MAX_SWAPCHAIN_IMAGES; ++i )
  {
    vkCreateSemaphore( gpu->vk_device, &semaphore_info, gpu->vk_allocation_callbacks, &gpu->vk_render_finished_semaphores[ i ] );
    vkCreateSemaphore( gpu->vk_device, &semaphore_info, gpu->vk_allocation_callbacks, &gpu->vk_image_avalivable_semaphores[ i ] );
    vkCreateFence( gpu->vk_device, &fence_info, gpu->vk_allocation_callbacks, &gpu->vk_command_buffer_executed_fences[ i ] );
  }
  
  crude_gfx_initialize_cmd_manager( &g_command_buffer_manager, gpu, creation->num_threads );
  gpu->queued_command_buffers = gpu->allocator.allocate( sizeof( crude_gfx_cmd_buffer* ) * 128, 1 );

  gpu->previous_frame = 0;
  gpu->current_frame = 1;
  gpu->vk_swapchain_image_index = 0;
  gpu->queued_command_buffers_count = 0;

  gpu->resource_deletion_queue = NULL;
  CRUDE_ARR_SETCAP( gpu->resource_deletion_queue, 16 );
  gpu->texture_to_update_bindless = NULL;
  CRUDE_ARR_SETCAP( gpu->texture_to_update_bindless, 16 );

  crude_gfx_sampler_creation sampler_creation = {
    .address_mode_u = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
    .address_mode_v = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
    .address_mode_w = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
    .min_filter     = VK_FILTER_LINEAR,
    .mag_filter     = VK_FILTER_LINEAR,
    .mip_filter     = VK_SAMPLER_MIPMAP_MODE_LINEAR,
    .name           = "sampler default"
  };
  gpu->default_sampler = crude_gfx_create_sampler( gpu, &sampler_creation );
  
  crude_gfx_texture_creation depth_texture_creation = { 
    .width    = gpu->vk_swapchain_width,
    .height   = gpu->vk_swapchain_height, 
    .depth    = 1,
    .mipmaps  = 1, 
    .format   = VK_FORMAT_D32_SFLOAT, 
    .type     = CRUDE_GFX_TEXTURE_TYPE_TEXTURE_2D, 
    .name     = "depth_image_texture"
  };
  gpu->depth_texture = crude_gfx_create_texture( gpu, &depth_texture_creation );

  crude_gfx_reset_render_pass_output( &gpu->swapchain_output );
  gpu->swapchain_output.color_formats[ gpu->swapchain_output.num_color_formats++ ] = gpu->vk_surface_format.format;
  gpu->swapchain_output.depth_operation = VK_FORMAT_D32_SFLOAT;

  crude_gfx_render_pass_creation swapchain_pass_creation = {
    .name                  = "swapchain",
    .type                  = CRUDE_GFX_RENDER_PASS_TYPE_SWAPCHAIN,
    .color_operation       = CRUDE_GFX_RENDER_PASS_OPERATION_CLEAR,
    .depth_operation       = CRUDE_GFX_RENDER_PASS_OPERATION_CLEAR,
    .depth_stencil_texture = CRUDE_GFX_RENDER_PASS_OPERATION_CLEAR,
    .scale_x             = 1.f,
    .scale_y             = 1.f,
    .resize              = 1,
  };
  gpu->swapchain_pass = crude_gfx_create_render_pass( gpu, &swapchain_pass_creation );
  
  gpu->dynamic_allocated_size = 0;
  gpu->dynamic_max_per_frame_size = 0;
  gpu->dynamic_per_frame_size = 1024 * 1024 * 10;
  crude_gfx_buffer_creation buffer_creation = {
    .name       = "dynamic_persistent_buffer",
    .type_flags = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
    .usage      = CRUDE_GFX_RESOURCE_USAGE_TYPE_IMMUTABLE,
    .size       = gpu->dynamic_per_frame_size * gpu->max_frames
  };
  gpu->dynamic_buffer = crude_gfx_create_buffer( gpu, &buffer_creation );
  
  crude_gfx_map_buffer_parameters buffer_map = {
    .buffer = gpu->dynamic_buffer
  };
  gpu->dynamic_mapped_memory = ( uint8* )crude_gfx_map_buffer( gpu, &buffer_map );
 }

void
crude_gfx_deinitialize_device
(
  _In_ crude_gfx_device                                   *gpu
)
{
  vkDeviceWaitIdle( gpu->vk_device );
  
  crude_gfx_deinitialize_cmd_manager( &g_command_buffer_manager );

  for ( uint32 i = 0; i < CRUDE_GFX_MAX_SWAPCHAIN_IMAGES; ++i )
  {
    vkDestroySemaphore( gpu->vk_device, gpu->vk_render_finished_semaphores[ i ], gpu->vk_allocation_callbacks );
    vkDestroySemaphore( gpu->vk_device, gpu->vk_image_avalivable_semaphores[ i ], gpu->vk_allocation_callbacks );
    vkDestroyFence( gpu->vk_device, gpu->vk_command_buffer_executed_fences[ i ], gpu->vk_allocation_callbacks );
  }
  
  crude_gfx_unmap_buffer( gpu, gpu->dynamic_buffer );
  crude_gfx_destroy_buffer( gpu, gpu->dynamic_buffer );
  crude_gfx_destroy_texture( gpu, gpu->depth_texture );
  crude_gfx_destroy_render_pass( gpu, gpu->swapchain_pass );
  crude_gfx_destroy_sampler( gpu, gpu->default_sampler );

  for ( uint32 i = 0; i < CRUDE_ARR_LEN( gpu->resource_deletion_queue ); ++i )
  {
    crude_gfx_resource_update* resource_deletion = &gpu->resource_deletion_queue[ i ];

    if ( resource_deletion->current_frame == -1 )
      continue;

    _vk_destroy_resources_instant( gpu, resource_deletion->type, resource_deletion->handle );
  }

  CRUDE_ARR_FREE( gpu->resource_deletion_queue );
  CRUDE_ARR_FREE( gpu->texture_to_update_bindless );

  crude_deinitialize_resource_pool( &gpu->buffers );
  crude_deinitialize_resource_pool( &gpu->textures );
  crude_deinitialize_resource_pool( &gpu->render_passes );
  crude_deinitialize_resource_pool( &gpu->descriptor_set_layouts );
  crude_deinitialize_resource_pool( &gpu->pipelines );
  crude_deinitialize_resource_pool( &gpu->shaders );
  crude_deinitialize_resource_pool( &gpu->samplers );

  vkDestroyDescriptorSetLayout( gpu->vk_device, gpu->vk_bindless_descriptor_set_layout, gpu->vk_allocation_callbacks );
  vkDestroyDescriptorPool( gpu->vk_device, gpu->vk_bindless_descriptor_pool, gpu->vk_allocation_callbacks );

  vkDestroyQueryPool( gpu->vk_device, gpu->vk_timestamp_query_pool, gpu->vk_allocation_callbacks );
  _vk_destroy_swapchain( gpu->vk_device, gpu->vk_swapchain, gpu->vk_swapchain_images_count, gpu->vk_swapchain_images_views, gpu->vk_swapchain_framebuffers, gpu->vk_allocation_callbacks );
  vmaDestroyAllocator( gpu->vma_allocator );
  vkDestroyDevice( gpu->vk_device, gpu->vk_allocation_callbacks );
  vkDestroySurfaceKHR( gpu->vk_instance, gpu->vk_surface, gpu->vk_allocation_callbacks );
  PFN_vkDestroyDebugUtilsMessengerEXT vkDestroyDebugUtilsMessengerEXT = vkGetInstanceProcAddr( gpu->vk_instance, "vkDestroyDebugUtilsMessengerEXT" );
  vkDestroyDebugUtilsMessengerEXT( gpu->vk_instance, gpu->vk_debug_utils_messenger, gpu->vk_allocation_callbacks );
  vkDestroyInstance( gpu->vk_instance, gpu->vk_allocation_callbacks );
}

/************************************************
 *
 * GPU Device Common Functions
 * 
 ***********************************************/  
void
crude_gfx_new_frame
(
  _In_ crude_gfx_device                                   *gpu
)
{
  CRUDE_PROFILER_ZONE_NAME( "GPUNewFrame" );
  VkFence *render_complete_fence = &gpu->vk_command_buffer_executed_fences[ gpu->current_frame ];
  if ( vkGetFenceStatus( gpu->vk_device, *render_complete_fence ) != VK_SUCCESS )
  {
    CRUDE_PROFILER_ZONE_NAME( "WaitForRenderComplete" );
    vkWaitForFences( gpu->vk_device, 1, render_complete_fence, VK_TRUE, UINT64_MAX );
    CRUDE_PROFILER_END;
  }
  
  vkResetFences( gpu->vk_device, 1, render_complete_fence );
  
  {
  CRUDE_PROFILER_ZONE_NAME( "AcquireImage" );
  VkResult result = vkAcquireNextImageKHR( gpu->vk_device, gpu->vk_swapchain, UINT64_MAX, gpu->vk_image_avalivable_semaphores[ gpu->current_frame ], VK_NULL_HANDLE, &gpu->vk_swapchain_image_index );
  if ( result == VK_ERROR_OUT_OF_DATE_KHR  )
  {
    _vk_resize_swapchain( gpu );
  }
  CRUDE_PROFILER_END;
  }

  crude_gfx_cmd_manager_reset( &g_command_buffer_manager, gpu->current_frame );

  uint32 used_size = gpu->dynamic_allocated_size - ( gpu->dynamic_per_frame_size * gpu->previous_frame );
  gpu->dynamic_max_per_frame_size = crude_max( used_size, gpu->dynamic_max_per_frame_size );
  gpu->dynamic_allocated_size = gpu->dynamic_per_frame_size * gpu->current_frame;
  CRUDE_PROFILER_END;
}

void
crude_gfx_present
(
  _In_ crude_gfx_device                                   *gpu
)
{
  CRUDE_PROFILER_ZONE_NAME( "GPUPresent" );
  VkFence     *render_complete_fence = &gpu->vk_command_buffer_executed_fences[ gpu->current_frame ];
  VkSemaphore *render_complete_semaphore = &gpu->vk_render_finished_semaphores[ gpu->current_frame ];
  
  VkCommandBuffer enqueued_command_buffers[ 4 ];
  for ( uint32 i = 0; i < gpu->queued_command_buffers_count; ++i )
  {
    CRUDE_PROFILER_ZONE_NAME( "EndRenderPassAndCommandBuffers" );
    crude_gfx_cmd_buffer* command_buffer = gpu->queued_command_buffers[i];
    enqueued_command_buffers[ i ] = command_buffer->vk_cmd_buffer;

    if ( command_buffer->is_recording && command_buffer->current_render_pass && ( command_buffer->current_render_pass->type != CRUDE_GFX_RENDER_PASS_TYPE_COMPUTE ) )
    {
      vkCmdEndRenderPass( command_buffer->vk_cmd_buffer );
    }

    vkEndCommandBuffer( command_buffer->vk_cmd_buffer );
    command_buffer->is_recording = false;
    command_buffer->current_render_pass = NULL;
    CRUDE_PROFILER_END;
  }

  VkWriteDescriptorSet bindless_descriptor_writes[ MAX_BINDLESS_RESOURCES ];
  VkDescriptorImageInfo bindless_image_info[ MAX_BINDLESS_RESOURCES ];
  uint32 current_write_index = 0;
  for ( int32 i = CRUDE_ARR_LEN( gpu->texture_to_update_bindless ) - 1; i >= 0; --i )
  {
    crude_gfx_resource_update* texture_to_update = &gpu->texture_to_update_bindless[ i ];
    
    crude_gfx_texture *texture = CRUDE_GFX_ACCESS_TEXTURE( gpu, ( crude_gfx_texture_handle ) { texture_to_update->handle } );

    VkWriteDescriptorSet *descriptor_write = &bindless_descriptor_writes[ current_write_index ];
    memset( descriptor_write, 0, sizeof( VkWriteDescriptorSet ) );
    descriptor_write->sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptor_write->descriptorCount = 1;
    descriptor_write->dstArrayElement = texture_to_update->handle;
    descriptor_write->descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    descriptor_write->dstSet = gpu->vk_bindless_descriptor_set;
    descriptor_write->dstBinding = BINDLESS_TEXTURE_BINDING;

    VkDescriptorImageInfo *descriptor_image_info = &bindless_image_info[ current_write_index ];    
    if ( texture->sampler )
    {
      descriptor_image_info->sampler = texture->sampler->vk_sampler;
    }
    else
    {
      crude_gfx_sampler *default_sampler = CRUDE_GFX_ACCESS_SAMPLER( gpu, gpu->default_sampler );
      descriptor_image_info->sampler = default_sampler->vk_sampler;
    }
    
    CRUDE_ASSERT( texture->vk_format != VK_FORMAT_UNDEFINED );
    descriptor_image_info->imageView = texture->vk_image_view;
    descriptor_image_info->imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    descriptor_write->pImageInfo = descriptor_image_info;
    
    CRUDE_ARR_DELSWAP( gpu->texture_to_update_bindless, i );

    ++current_write_index;
  }

  if ( current_write_index )
  {
    CRUDE_PROFILER_ZONE_NAME( "UpdateDescriptorSets" );
    vkUpdateDescriptorSets( gpu->vk_device, current_write_index, bindless_descriptor_writes, 0, NULL );
    CRUDE_PROFILER_END;
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
  
  {
  CRUDE_PROFILER_ZONE_NAME( "QueueSubmit" );
  CRUDE_GFX_HANDLE_VULKAN_RESULT( vkQueueSubmit( gpu->vk_main_queue, 1, &submit_info, *render_complete_fence ), "Failed to sumbit queue" );
  CRUDE_PROFILER_END;
  }

  VkSwapchainKHR swap_chains[] = { gpu->vk_swapchain };
  VkPresentInfoKHR present_info = {
    .sType              = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
    .waitSemaphoreCount = 1,
    .pWaitSemaphores    = render_complete_semaphore,
    .swapchainCount     = CRUDE_STACK_ARRAY_SIZE( swap_chains ),
    .pSwapchains        = swap_chains,
    .pImageIndices      = &gpu->vk_swapchain_image_index,
  };
  
  {
  CRUDE_PROFILER_ZONE_NAME( "QueuePresent" );
  VkResult result = vkQueuePresentKHR( gpu->vk_main_queue, &present_info );

  gpu->queued_command_buffers_count = 0u;

  if ( result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR )
  {
    _vk_resize_swapchain( gpu );
    CRUDE_PROFILER_END;
    return;
  }
  CRUDE_PROFILER_END;
  }

  gpu->previous_frame = gpu->current_frame;
  gpu->current_frame = ( gpu->current_frame + 1u ) % gpu->vk_swapchain_images_count;
  
  {
  CRUDE_PROFILER_ZONE_NAME( "DestroyResoucesInstants" );
  for ( uint32 i = 0; i < CRUDE_ARR_LEN( gpu->resource_deletion_queue ); ++i )
  {
    crude_gfx_resource_update* resource_deletion = &gpu->resource_deletion_queue[ i ];
    
    if ( resource_deletion->current_frame != gpu->current_frame )
    {
      continue;
    }
    
    _vk_destroy_resources_instant( gpu, resource_deletion->type, resource_deletion->handle );

    resource_deletion->current_frame = UINT32_MAX;
    CRUDE_ARR_DELSWAP( gpu->resource_deletion_queue, i );
    --i;
  }
  CRUDE_PROFILER_END;
  }

  CRUDE_PROFILER_END;
}

crude_gfx_cmd_buffer*
crude_gfx_get_primary_cmd
(
  _In_ crude_gfx_device                                   *gpu,
  _In_ uint32                                              thread_index,
  _In_ bool                                                begin
)
{
  crude_gfx_cmd_buffer *cmd = crude_gfx_cmd_manager_get_primary_cmd( &g_command_buffer_manager, gpu->current_frame, thread_index, begin );

  if ( /*gpu_timestamp_reset &&*/ begin )
  {
    //vkCmdResetQueryPool( cmd->vk_cmd_buffer, vulkan_timestamp_query_pool, current_frame * gpu_timestamp_manager->queries_per_frame * 2, gpu_timestamp_manager->queries_per_frame );
    /*gpu_timestamp_reset = false;*/
  }

  return cmd;
}

crude_gfx_cmd_buffer*
crude_gfx_get_secondary_cmd
(
  _In_ crude_gfx_device                                   *gpu,
  _In_ uint32                                              thread_index
)
{
  crude_gfx_cmd_buffer *cmd = crude_gfx_cmd_manager_get_secondary_cmd( &g_command_buffer_manager, gpu->current_frame, thread_index );
  return cmd;
}

void
crude_gfx_queue_cmd
(
  _In_ crude_gfx_cmd_buffer                               *cmd
)
{
  cmd->gpu->queued_command_buffers[ cmd->gpu->queued_command_buffers_count++ ] = cmd;
}

void*
crude_gfx_map_buffer
(
  _In_ crude_gfx_device                                   *gpu,
  _In_ crude_gfx_map_buffer_parameters const              *parameters
)
{
  if ( CRUDE_GFX_IS_HANDLE_INVALID( parameters->buffer ) )
  {
    return NULL;
  }

  crude_gfx_buffer *buffer = CRUDE_GFX_ACCESS_BUFFER( gpu, parameters->buffer );
  
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
  _In_ crude_gfx_device                                   *gpu,
  _In_ crude_gfx_buffer_handle                             handle
)
{
  if ( CRUDE_GFX_IS_HANDLE_INVALID( handle ) )
  {
    return;
  }

  crude_gfx_buffer *buffer = CRUDE_GFX_ACCESS_BUFFER( gpu, handle );
  if ( buffer->parent_buffer.index == gpu->dynamic_buffer.index )
  {
    return;
  }
  
  vmaUnmapMemory( gpu->vma_allocator, buffer->vma_allocation );
}

void*
crude_gfx_dynamic_allocate
(
  _In_ crude_gfx_device                                   *gpu,
  _In_ uint32                                              size
)
{
  void *mapped_memory = gpu->dynamic_mapped_memory + gpu->dynamic_allocated_size;
  gpu->dynamic_allocated_size += crude_memory_align( size, CRUDE_GFX_UBO_ALIGNMENT );
  return mapped_memory;
}

void
crude_gfx_link_texture_sampler
(
  _In_ crude_gfx_device                                   *gpu,
  _In_ crude_gfx_texture_handle                            texture,
  _In_ crude_gfx_sampler_handle                            sampler
)
{
  crude_gfx_texture* texture_vk = CRUDE_GFX_ACCESS_TEXTURE( gpu, texture );
  crude_gfx_sampler* sampler_vk = CRUDE_GFX_ACCESS_SAMPLER( gpu, sampler );
  
  texture_vk->sampler = sampler_vk;
}

crude_gfx_descriptor_set_layout_handle
crude_gfx_get_descriptor_set_layout
(
  _In_ crude_gfx_device                                   *gpu,
  _In_ crude_gfx_pipeline_handle                           pipeline_handle,
  _In_ uint32                                              layout_index
)
{
  crude_gfx_pipeline *pipeline = CRUDE_GFX_ACCESS_PIPELINE( gpu, pipeline_handle );
  return pipeline->descriptor_set_layout_handle[ layout_index ];
}

void
crude_gfx_query_buffer
(
  _In_ crude_gfx_device                                   *gpu,
  _In_ crude_gfx_buffer_handle                             buffer,
  _Out_ crude_gfx_buffer_description                      *description
)
{
  if ( CRUDE_GFX_IS_HANDLE_INVALID( buffer ) )
  {
    return;
  }

  const crude_gfx_buffer* buffer_data = CRUDE_GFX_ACCESS_BUFFER( gpu, buffer );
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
  _In_ crude_gfx_device                                   *gpu,
  _In_ VkObjectType                                        type,
  _In_ uint64                                              handle,
  _In_ char const                                         *name
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

bool
crude_gfx_buffer_ready
(
  _In_ crude_gfx_device                                   *gpu,
  _In_ crude_gfx_buffer_handle                             buffer_handle
)
{
  crude_gfx_buffer *buffer = CRUDE_GFX_ACCESS_BUFFER( gpu, buffer_handle );
  return buffer->ready;
}

/************************************************
 *
 * GPU Device Resources Functions
 * 
 ***********************************************/   
crude_gfx_sampler_handle
crude_gfx_create_sampler
(
  _In_ crude_gfx_device                                   *gpu,
  _In_ crude_gfx_sampler_creation const                   *creation
)
{
  crude_gfx_sampler_handle handle = { CRUDE_GFX_OBTAIN_SAMPLER( gpu ) };
  if ( CRUDE_GFX_IS_HANDLE_INVALID( handle ) )
  {
    return handle;
  }
  
  crude_gfx_sampler *sampler = CRUDE_GFX_ACCESS_SAMPLER( gpu, handle );
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
  _In_ crude_gfx_device                                   *gpu,
  _In_ crude_gfx_sampler_handle                            handle
)
{
  if ( handle.index >= gpu->samplers.pool_size )
  {
    CRUDE_LOG_ERROR( CRUDE_CHANNEL_GRAPHICS, "Trying to free invalid sampler %u", handle.index );
    return;
  }
  crude_gfx_resource_update sampler_update_event = { 
    .type          = CRUDE_GFX_RESOURCE_DELETION_TYPE_SAMPLER,
    .handle        = handle.index,
    .current_frame = gpu->current_frame };
  CRUDE_ARR_PUT( gpu->resource_deletion_queue, sampler_update_event );
}

void
crude_gfx_destroy_sampler_instant
(
  _In_ crude_gfx_device                                   *gpu,
  _In_ crude_gfx_sampler_handle                            handle
)
{
  crude_gfx_sampler *sampler = CRUDE_GFX_ACCESS_SAMPLER( gpu, handle );
  if ( sampler )
  {
    vkDestroySampler( gpu->vk_device, sampler->vk_sampler, gpu->vk_allocation_callbacks );
  }
  CRUDE_GFX_RELEASE_SAMPLER( gpu, handle );
}

crude_gfx_texture_handle
crude_gfx_create_texture
(
  _In_ crude_gfx_device                                   *gpu,
  _In_ crude_gfx_texture_creation const                   *creation
)
{
  crude_gfx_texture_handle handle = { CRUDE_GFX_OBTAIN_TEXTURE( gpu ) };
  if ( CRUDE_GFX_IS_HANDLE_INVALID( handle ) )
  {
    return handle;
  }
  
  crude_gfx_texture *texture = CRUDE_GFX_ACCESS_TEXTURE( gpu, handle );
  _vk_create_texture( gpu, creation, handle, texture );

  if ( creation->initial_data )
  {
    VkBufferCreateInfo buffer_info =
    {
      .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
      .usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT
    };
    
    uint64 image_size = creation->width * creation->height * 4u;
    buffer_info.size = image_size;
    
    VmaAllocationCreateInfo memory_info =
    {
      .flags = VMA_ALLOCATION_CREATE_STRATEGY_BEST_FIT_BIT,
      .usage = VMA_MEMORY_USAGE_CPU_TO_GPU,
    };
    
    VmaAllocationInfo allocation_info;
    VkBuffer staging_buffer;
    VmaAllocation staging_allocation;
    CRUDE_GFX_HANDLE_VULKAN_RESULT( vmaCreateBuffer( gpu->vma_allocator, &buffer_info, &memory_info, &staging_buffer, &staging_allocation, &allocation_info ), "Failed to create staging buffer for texture data" );
    
    void* destination_data;
    vmaMapMemory( gpu->vma_allocator, staging_allocation, &destination_data );
    memcpy( destination_data, creation->initial_data, image_size );
    vmaUnmapMemory( gpu->vma_allocator, staging_allocation );
    
    crude_gfx_cmd_buffer *cmd = crude_gfx_cmd_manager_get_primary_cmd( &g_command_buffer_manager, gpu->current_frame, 0u, true );
    
    VkBufferImageCopy region =
    {
      .bufferOffset = 0,
      .bufferRowLength = 0,
      .bufferImageHeight = 0,
      .imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
      .imageSubresource.mipLevel = 0,
      .imageSubresource.baseArrayLayer = 0,
      .imageSubresource.layerCount = 1,
      .imageOffset = { 0, 0, 0 },
      .imageExtent = { creation->width, creation->height, creation->depth }
    };
    
    crude_gfx_cmd_add_image_barrier( cmd, texture->vk_image, CRUDE_GFX_RESOURCE_STATE_UNDEFINED, CRUDE_GFX_RESOURCE_STATE_COPY_DEST, 0, 1, false );
    
    vkCmdCopyBufferToImage( cmd->vk_cmd_buffer, staging_buffer, texture->vk_image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region );
    
    if ( creation->mipmaps > 1 )
    {
      crude_gfx_cmd_add_image_barrier( cmd, texture->vk_image, CRUDE_GFX_RESOURCE_STATE_COPY_DEST, CRUDE_GFX_RESOURCE_STATE_COPY_SOURCE, 0, 1, false );
    }
    
    int32 w = creation->width;
    int32 h = creation->height;
    
    for ( int32 mip_index = 1; mip_index < creation->mipmaps; ++mip_index )
    {
      crude_gfx_cmd_add_image_barrier( cmd, texture->vk_image, CRUDE_GFX_RESOURCE_STATE_UNDEFINED, CRUDE_GFX_RESOURCE_STATE_COPY_DEST, mip_index, 1, false );
    
      VkImageBlit blit_region =
      { 
        .srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
        .srcSubresource.mipLevel = mip_index - 1,
        .srcSubresource.baseArrayLayer = 0,
        .srcSubresource.layerCount = 1,
    
        .srcOffsets[0] = { 0, 0, 0 },
        .srcOffsets[1] = { w, h, 1 },
    
        .dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
        .dstSubresource.mipLevel = mip_index,
        .dstSubresource.baseArrayLayer = 0,
        .dstSubresource.layerCount = 1,
    
        .dstOffsets[0] = { 0, 0, 0 },
        .dstOffsets[1] = { w / 2, h / 2, 1 },
      };
    
      w /= 2;
      h /= 2;
    
      vkCmdBlitImage( cmd->vk_cmd_buffer, texture->vk_image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, texture->vk_image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &blit_region, VK_FILTER_LINEAR );
    
      crude_gfx_cmd_add_image_barrier( cmd, texture->vk_image, CRUDE_GFX_RESOURCE_STATE_COPY_DEST, CRUDE_GFX_RESOURCE_STATE_COPY_SOURCE, mip_index, 1, false );
    }
    
    crude_gfx_cmd_add_image_barrier( cmd, texture->vk_image, ( creation->mipmaps > 1 ) ? CRUDE_GFX_RESOURCE_STATE_COPY_SOURCE : CRUDE_GFX_RESOURCE_STATE_COPY_DEST, CRUDE_GFX_RESOURCE_STATE_SHADER_RESOURCE, 0, creation->mipmaps, false );
    
    crude_gfx_cmd_end( cmd );
    
    VkSubmitInfo submit_info = {
      .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
      .commandBufferCount = 1,
      .pCommandBuffers = &cmd->vk_cmd_buffer,
    };
    
    vkQueueSubmit( gpu->vk_main_queue, 1, &submit_info, VK_NULL_HANDLE );
    vkQueueWaitIdle( gpu->vk_main_queue);
    vmaDestroyBuffer( gpu->vma_allocator, staging_buffer, staging_allocation );

    vkResetCommandBuffer( cmd->vk_cmd_buffer, VK_COMMAND_BUFFER_RESET_RELEASE_RESOURCES_BIT );
    
    texture->vk_image_layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
  }

  return handle;
}

void
crude_gfx_destroy_texture
(
  _In_ crude_gfx_device                                   *gpu,
  _In_ crude_gfx_texture_handle                            handle
)
{
  if ( handle.index >= gpu->textures.pool_size )
  {
    CRUDE_LOG_ERROR( CRUDE_CHANNEL_GRAPHICS, "Trying to free invalid texture %u", handle.index );
    return;
  }
  crude_gfx_resource_update texture_update_event = { 
    .type          = CRUDE_GFX_RESOURCE_DELETION_TYPE_TEXTURE,
    .handle        = handle.index,
    .current_frame = gpu->current_frame };
  CRUDE_ARR_PUT( gpu->resource_deletion_queue, texture_update_event );

  crude_gfx_resource_update texture_update_bindless_event = { 
    .type          = CRUDE_GFX_RESOURCE_DELETION_TYPE_TEXTURE,
    .handle        = handle.index,
    .current_frame = gpu->current_frame };
  CRUDE_ARR_PUT( gpu->texture_to_update_bindless, texture_update_event );
}

void
crude_gfx_destroy_texture_instant
(
  _In_ crude_gfx_device                                   *gpu,
  _In_ crude_gfx_texture_handle                            handle
)
{
  crude_gfx_texture *texture = CRUDE_GFX_ACCESS_TEXTURE( gpu, handle );
  
  if ( texture )
  {
    vkDestroyImageView( gpu->vk_device, texture->vk_image_view, gpu->vk_allocation_callbacks );
    vmaDestroyImage( gpu->vma_allocator, texture->vk_image, texture->vma_allocation );
  }
  CRUDE_GFX_RELEASE_TEXTURE( gpu, handle );
}

crude_gfx_shader_state_handle
crude_gfx_create_shader_state
(
  _In_ crude_gfx_device                                   *gpu,
  _In_ crude_gfx_shader_state_creation const              *creation
)
{ 
  if ( creation->stages_count == 0 || creation->stages == NULL )
  {
    CRUDE_LOG_ERROR( CRUDE_CHANNEL_GRAPHICS, "Shader %s does not contain shader stages.", creation->name );
    return CRUDE_GFX_INVALID_SHADER_STATE_HANDLE;
  }
  
  crude_gfx_shader_state_handle handle = { CRUDE_GFX_OBTAIN_SHADER_STATE( gpu ) };
  if ( CRUDE_GFX_IS_HANDLE_INVALID( handle ) )
  {
    return handle;
  }

  uint32 compiled_shaders = 0u;

  crude_gfx_shader_state *shader_state = CRUDE_GFX_ACCESS_SHADER_STATE( gpu, handle );
  shader_state->graphics_pipeline = true;
  shader_state->active_shaders = 0;

  for ( compiled_shaders = 0; compiled_shaders < creation->stages_count; ++compiled_shaders )
  {
    crude_gfx_shader_stage const *stage = &creation->stages[ compiled_shaders ];
  
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
  
    CRUDE_ASSERTM( CRUDE_CHANNEL_GRAPHICS, shader_create_info.pCode, "Shader code is empty!" );

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

    if ( shader_create_info.pCode )
    {
      _vk_reflect_shader( gpu, shader_create_info.pCode, shader_create_info.codeSize, &shader_state->reflect );
    }
  }
  
  shader_state->active_shaders = compiled_shaders;
  shader_state->name = creation->name;

  bool creation_failed = compiled_shaders != creation->stages_count;
  if ( creation_failed )
  {
    crude_gfx_destroy_shader_state( gpu, handle );
    
    CRUDE_LOG_ERROR( CRUDE_CHANNEL_GRAPHICS, "Error in creation of shader %s. Dumping all shader informations.", creation->name );
    for ( compiled_shaders = 0; compiled_shaders < creation->stages_count; ++compiled_shaders )
    {
      crude_gfx_shader_stage const *stage = &creation->stages[ compiled_shaders ];
      CRUDE_LOG_ERROR( CRUDE_CHANNEL_GRAPHICS, "%u:\n%s", stage->type, stage->code );
    }
    return CRUDE_GFX_INVALID_SHADER_STATE_HANDLE;
  }

  return handle;
}

void
crude_gfx_destroy_shader_state
(
  _In_ crude_gfx_device                                   *gpu,
  _In_ crude_gfx_shader_state_handle                       handle
)
{
  if ( handle.index >= gpu->shaders.pool_size )
  {
    CRUDE_LOG_ERROR( CRUDE_CHANNEL_GRAPHICS, "Trying to free invalid shader state %u", handle.index );
    return;
  }
  crude_gfx_resource_update shader_state_update_event = { 
    .type          = CRUDE_GFX_RESOURCE_DELETION_TYPE_SHADER_STATE,
    .handle        = handle.index,
    .current_frame = gpu->current_frame };
  CRUDE_ARR_PUT( gpu->resource_deletion_queue, shader_state_update_event );
}

void
crude_gfx_destroy_shader_state_instant
(
  _In_ crude_gfx_device                                   *gpu,
  _In_ crude_gfx_shader_state_handle                       handle
)
{
  crude_gfx_shader_state *shader_state = CRUDE_GFX_ACCESS_SHADER_STATE( gpu, handle );
  if ( shader_state )
  {
    for ( uint32 i = 0; i < shader_state->active_shaders; ++i )
    {
      vkDestroyShaderModule( gpu->vk_device, shader_state->shader_stage_info[ i ].module, gpu->vk_allocation_callbacks );
    }
  }
  CRUDE_GFX_RELEASE_SHADER_STATE( gpu, handle );
}

crude_gfx_render_pass_handle
crude_gfx_create_render_pass
(
  _In_ crude_gfx_device                                   *gpu,
  _In_ crude_gfx_render_pass_creation const               *creation
)
{
  crude_gfx_render_pass_handle handle = { CRUDE_GFX_OBTAIN_RENDER_PASS( gpu) };
  if ( CRUDE_GFX_IS_HANDLE_INVALID( handle ) )
  {
    return handle;
  }
  
  crude_gfx_render_pass *render_pass = CRUDE_GFX_ACCESS_RENDER_PASS( gpu, handle );
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
    crude_gfx_texture *texture = CRUDE_GFX_ACCESS_TEXTURE( gpu, creation->output_textures[ i ] );
    
    render_pass->width = texture->width;
    render_pass->height = texture->height;
    render_pass->output_textures[ i ] = creation->output_textures[ i ];
  }
  
  render_pass->output_depth = creation->depth_stencil_texture;
  
  switch ( creation->type )
  {
    case CRUDE_GFX_RENDER_PASS_TYPE_SWAPCHAIN:
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
  _In_ crude_gfx_device                                   *gpu,
  _In_ crude_gfx_render_pass_handle                        handle
)
{
  if ( handle.index >= gpu->render_passes.pool_size )
  {
    CRUDE_LOG_ERROR( CRUDE_CHANNEL_GRAPHICS, "Trying to free invalid texture %u", handle.index );
    return;
  }
  crude_gfx_resource_update render_pass_update_event = { 
    .type          = CRUDE_GFX_RESOURCE_DELETION_TYPE_RENDER_PASS,
    .handle        = handle.index,
    .current_frame = gpu->current_frame };
  CRUDE_ARR_PUT( gpu->resource_deletion_queue, render_pass_update_event );
}

void
crude_gfx_destroy_render_pass_instant
(
  _In_ crude_gfx_device                                   *gpu,
  _In_ crude_gfx_render_pass_handle                        handle
)
{
  crude_gfx_render_pass *render_pass = CRUDE_GFX_ACCESS_RENDER_PASS( gpu, handle );
  if ( render_pass )
  {
    if ( render_pass->num_render_targets )
    {
      vkDestroyFramebuffer( gpu->vk_device, render_pass->vk_frame_buffer, gpu->vk_allocation_callbacks );
    }
    vkDestroyRenderPass( gpu->vk_device, render_pass->vk_render_pass, gpu->vk_allocation_callbacks );
  }
  CRUDE_GFX_RELEASE_RENDER_PASS( gpu, handle );
}

crude_gfx_pipeline_handle
crude_gfx_create_pipeline
(
  _In_ crude_gfx_device                                   *gpu,
  _In_ crude_gfx_pipeline_creation const                  *creation
)
{
  crude_gfx_pipeline_handle handle = { CRUDE_GFX_OBTAIN_PIPELINE( gpu ) };
  if ( CRUDE_GFX_IS_HANDLE_INVALID( handle ) )
  {
    return handle;
  }

  crude_gfx_shader_state_handle shader_state = crude_gfx_create_shader_state( gpu, &creation->shaders );
  if ( CRUDE_GFX_IS_HANDLE_INVALID( shader_state ) )
  {
    CRUDE_GFX_RELEASE_PIPELINE( gpu, handle );
    return CRUDE_GFX_INVALID_PIPELINE_HANDLE;
  }

  crude_gfx_pipeline *pipeline = CRUDE_GFX_ACCESS_PIPELINE( gpu, handle );
  crude_gfx_shader_state *shader_state_data = CRUDE_GFX_ACCESS_SHADER_STATE( gpu, shader_state );
  
  pipeline->shader_state = shader_state;
  
  // Create VkPipelineLayout
  VkDescriptorSetLayout vk_layouts[ CRUDE_GFX_MAX_DESCRIPTOR_SET_LAYOUTS ];
  for ( uint32 i = 0; i < shader_state_data->reflect.descriptor.sets_count; ++i )
  {
    pipeline->descriptor_set_layout_handle[ i ] = crude_gfx_create_descriptor_set_layout( gpu, &shader_state_data->reflect.descriptor.sets[ i ] );
    crude_gfx_descriptor_set_layout *descriptor_set_layout = CRUDE_GFX_ACCESS_DESCRIPTOR_SET_LAYOUT( gpu, pipeline->descriptor_set_layout_handle[ i ] );
    vk_layouts[ i ] = descriptor_set_layout->vk_descriptor_set_layout;
  }

  vk_layouts[ shader_state_data->reflect.descriptor.sets_count ] = gpu->vk_bindless_descriptor_set_layout;

  VkPipelineLayoutCreateInfo pipeline_layout_info = {
    .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
    .pSetLayouts = vk_layouts,
    .setLayoutCount = shader_state_data->reflect.descriptor.sets_count + 1,
  };
  
  VkPipelineLayout pipeline_layout;
  CRUDE_GFX_HANDLE_VULKAN_RESULT( vkCreatePipelineLayout( gpu->vk_device, &pipeline_layout_info, gpu->vk_allocation_callbacks, &pipeline_layout ), "Failed to create pipeline layout" );

  pipeline->vk_pipeline_layout = pipeline_layout;
  pipeline->num_active_layouts = shader_state_data->reflect.descriptor.sets_count;

  VkPipelineVertexInputStateCreateInfo vertex_input_info = { .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO };
  
  VkVertexInputAttributeDescription vk_vertex_attributes[ 8 ];
  CRUDE_ARR( crude_gfx_vertex_attribute ) vertex_attributes = shader_state_data->reflect.input.vertex_attributes;
  CRUDE_ASSERT( CRUDE_STACK_ARRAY_SIZE( vk_vertex_attributes ) >= CRUDE_ARR_LEN( vertex_attributes ) );

  if ( CRUDE_ARR_LEN( vertex_attributes ) )
  {
    for ( uint32 i = 0; i < CRUDE_ARR_LEN( vertex_attributes ); ++i )
    {
      vk_vertex_attributes[ i ] = ( VkVertexInputAttributeDescription ){
        .location = vertex_attributes[ i ].location,
        .binding = vertex_attributes[ i ].binding,
        .format = crude_gfx_to_vk_vertex_format( vertex_attributes[ i ].format ),
        .offset = vertex_attributes[ i ].offset
      };
    }
    vertex_input_info.vertexAttributeDescriptionCount = CRUDE_ARR_LEN( vertex_attributes );
    vertex_input_info.pVertexAttributeDescriptions = vk_vertex_attributes;
  }
  else
  {
    vertex_input_info.vertexAttributeDescriptionCount = 0;
    vertex_input_info.pVertexAttributeDescriptions = NULL;
  }

  VkVertexInputBindingDescription vk_vertex_bindings[ 8 ];
  CRUDE_ARR( crude_gfx_vertex_stream ) vertex_streams = shader_state_data->reflect.input.vertex_streams;
  CRUDE_ASSERT( CRUDE_STACK_ARRAY_SIZE( vk_vertex_bindings ) >= CRUDE_ARR_LEN( vertex_streams ) );
  if ( CRUDE_ARR_LEN( vertex_streams ) )
  {
    for ( uint32 i = 0; i < CRUDE_ARR_LEN( vertex_streams ); ++i )
    {
      VkVertexInputRate vertex_rate = vertex_streams[ i ].input_rate == CRUDE_GFX_VERTEX_INPUT_RATE_PER_VERTEX ? VK_VERTEX_INPUT_RATE_VERTEX : VK_VERTEX_INPUT_RATE_INSTANCE;
      vk_vertex_bindings[ i ] = ( VkVertexInputBindingDescription ){
        .binding = vertex_streams[ i ].binding,
        .stride = vertex_streams[ i ].stride,
        .inputRate = vertex_rate
      };
    }
    vertex_input_info.vertexBindingDescriptionCount = CRUDE_ARR_LEN( vertex_streams );
    vertex_input_info.pVertexBindingDescriptions = vk_vertex_bindings;
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
      crude_gfx_blend_state const *blend_state = &creation->blend_state.blend_states[i];
  
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
    .dynamicStateCount = CRUDE_STACK_ARRAY_SIZE( dynamic_states ),
    .pDynamicStates = dynamic_states,
  };
  
  crude_gfx_render_pass *render_pass = crude_resource_pool_access_resource( &gpu->render_passes, gpu->swapchain_pass.index );

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
  _In_ crude_gfx_device                                   *gpu,
  _In_ crude_gfx_pipeline_handle                           handle
)
{
  if ( handle.index >= gpu->pipelines.pool_size )
  {
    CRUDE_LOG_ERROR( CRUDE_CHANNEL_GRAPHICS, "Trying to free invalid pipeline %u", handle.index );
    return;
  }
  crude_gfx_resource_update pipeline_update_event = { 
    .type          = CRUDE_GFX_RESOURCE_DELETION_TYPE_PIPELINE,
    .handle        = handle.index,
    .current_frame = gpu->current_frame };
  CRUDE_ARR_PUT( gpu->resource_deletion_queue, pipeline_update_event );

  crude_gfx_pipeline *pipeline = CRUDE_GFX_ACCESS_PIPELINE( gpu, handle );
  crude_gfx_destroy_shader_state( gpu, pipeline->shader_state );
}

void
crude_gfx_destroy_pipeline_instant
(
  _In_ crude_gfx_device                                   *gpu,
  _In_ crude_gfx_pipeline_handle                           handle
)
{
  crude_gfx_pipeline *pipeline = CRUDE_GFX_ACCESS_PIPELINE( gpu, handle );

  if ( pipeline )
  {
    for ( uint32 i = 0; i < pipeline->num_active_layouts; ++i )
    {
      crude_gfx_destroy_descriptor_set_layout( gpu, pipeline->descriptor_set_layout_handle[ i ] );
    }
    vkDestroyPipeline( gpu->vk_device, pipeline->vk_pipeline, gpu->vk_allocation_callbacks );
    vkDestroyPipelineLayout( gpu->vk_device, pipeline->vk_pipeline_layout, gpu->vk_allocation_callbacks );
  }

  CRUDE_GFX_RELEASE_PIPELINE( gpu, handle );
}

crude_gfx_buffer_handle
crude_gfx_create_buffer
(
  _In_ crude_gfx_device                                   *gpu,
  _In_ crude_gfx_buffer_creation const                    *creation
)
{
  crude_gfx_buffer_handle handle = { CRUDE_GFX_OBTAIN_BUFFER( gpu ) };
  if ( CRUDE_GFX_IS_HANDLE_INVALID( handle ) )
  {
      return handle;
  }

  crude_gfx_buffer *buffer = CRUDE_GFX_ACCESS_BUFFER( gpu, handle );
  buffer->name = creation->name;
  buffer->size = creation->size;
  buffer->type_flags = creation->type_flags;
  buffer->usage = creation->usage;
  buffer->handle = handle;
  buffer->global_offset = 0;
  buffer->parent_buffer = CRUDE_GFX_INVALID_BUFFER_HANDLE;
  buffer->ready = true;

  bool use_global_buffer = ( creation->type_flags & ( VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT ) ) != 0;
  if ( creation->usage == CRUDE_GFX_RESOURCE_USAGE_TYPE_DYNAMIC && use_global_buffer )
  {
    buffer->parent_buffer = gpu->dynamic_buffer;
    return handle;
  }

  VkBufferCreateInfo buffer_info = {
    .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
    .usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT | creation->type_flags,
    .size = creation->size > 0 ? creation->size : 1,
  };
  
  VmaAllocationCreateInfo allocation_create_info = {
    .flags = VMA_ALLOCATION_CREATE_STRATEGY_BEST_FIT_BIT
  };

  if ( creation->persistent )
  {
    allocation_create_info.flags = allocation_create_info.flags | VMA_ALLOCATION_CREATE_MAPPED_BIT;
  }

  if ( creation->device_only )
  {
    allocation_create_info.usage = VMA_MEMORY_USAGE_GPU_ONLY;
  }
  else
  {
    allocation_create_info.usage = VMA_MEMORY_USAGE_GPU_TO_CPU;
  }
  
  VmaAllocationInfo allocation_info;
  CRUDE_GFX_HANDLE_VULKAN_RESULT( vmaCreateBuffer( gpu->vma_allocator, &buffer_info, &allocation_create_info, &buffer->vk_buffer, &buffer->vma_allocation, &allocation_info ),
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
  
  if ( creation->persistent )
  {
    buffer->mapped_data = ( uint8* )allocation_info.pMappedData;
  }

  return handle;
}

void
crude_gfx_destroy_buffer
(
  _In_ crude_gfx_device                                   *gpu,
  _In_ crude_gfx_buffer_handle                             handle
)
{
  if ( handle.index >= gpu->buffers.pool_size )
  {
    CRUDE_LOG_ERROR( CRUDE_CHANNEL_GRAPHICS, "Trying to free invalid buffer %u", handle.index );
    return;
  }
  crude_gfx_resource_update buffer_update_event = { 
    .type          = CRUDE_GFX_RESOURCE_DELETION_TYPE_BUFFER,
    .handle        = handle.index,
    .current_frame = gpu->current_frame };
  CRUDE_ARR_PUT( gpu->resource_deletion_queue, buffer_update_event );
}

void
crude_gfx_destroy_buffer_instant
(
  _In_ crude_gfx_device                                   *gpu,
  _In_ crude_gfx_buffer_handle                             handle
)
{
  crude_gfx_buffer *buffer = CRUDE_GFX_ACCESS_BUFFER( gpu, handle );

  if ( buffer && CRUDE_GFX_IS_HANDLE_INVALID( buffer->parent_buffer ) )
  {
    vmaDestroyBuffer( gpu->vma_allocator, buffer->vk_buffer, buffer->vma_allocation );
  }

  CRUDE_GFX_RELEASE_BUFFER( gpu, handle );
}

crude_gfx_descriptor_set_layout_handle
crude_gfx_create_descriptor_set_layout
(
  _In_ crude_gfx_device                                   *gpu,
  _In_ crude_gfx_descriptor_set_layout_creation const     *creation
)
{
  crude_gfx_descriptor_set_layout_handle handle = { CRUDE_GFX_OBTAIN_DESCRIPTOR_SET_LAYOUT( gpu ) };
  if ( CRUDE_GFX_IS_HANDLE_INVALID( handle ) )
  {
    return handle;
  }
  
  crude_gfx_descriptor_set_layout *descriptor_set_layout = CRUDE_GFX_ACCESS_DESCRIPTOR_SET_LAYOUT( gpu, handle );
  
  uint8 *memory = gpu->allocator.allocate( ( sizeof( VkDescriptorSetLayoutBinding ) + sizeof( crude_gfx_descriptor_binding ) ) * creation->num_bindings, 1 );
  descriptor_set_layout->num_bindings = creation->num_bindings;
  descriptor_set_layout->bindings     = CAST( crude_gfx_descriptor_binding*, memory );
  descriptor_set_layout->vk_binding   = CAST( VkDescriptorSetLayoutBinding*, memory + sizeof( crude_gfx_descriptor_binding ) * creation->num_bindings );
  descriptor_set_layout->handle       = handle;
  descriptor_set_layout->set_index    = creation->set_index;
  
  uint32 used_bindings = 0;
  for ( uint32 i = 0; i < creation->num_bindings; ++i )
  {
    crude_gfx_descriptor_binding *binding = &descriptor_set_layout->bindings[ i ];
    memset( binding, 0, sizeof( *binding ) );

    crude_gfx_descriptor_set_layout_binding const *input_binding = &creation->bindings[ i ];
    binding->start = ( input_binding->start == UINT16_MAX ) ? i : input_binding->start;
    binding->count = 1;
    binding->type = input_binding->type;
    binding->name = input_binding->name;
    binding->set = descriptor_set_layout->set_index;
    
    VkDescriptorSetLayoutBinding *const vk_binding = &descriptor_set_layout->vk_binding[ used_bindings++ ];
    vk_binding->binding = binding->start;
    vk_binding->descriptorType = input_binding->type;
    vk_binding->descriptorType = vk_binding->descriptorType == VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER ? VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC : vk_binding->descriptorType;
    vk_binding->descriptorCount = 1;
    vk_binding->stageFlags = VK_SHADER_STAGE_ALL;
    vk_binding->pImmutableSamplers = NULL;
  }

  VkDescriptorSetLayoutCreateInfo layout_info = {
    .sType        = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
    .bindingCount = used_bindings,
    .pBindings    = descriptor_set_layout->vk_binding,
  };

  CRUDE_GFX_HANDLE_VULKAN_RESULT( vkCreateDescriptorSetLayout( gpu->vk_device, &layout_info, gpu->vk_allocation_callbacks, &descriptor_set_layout->vk_descriptor_set_layout ), "Failed to create descriptor set layout" );
  return handle;
}

void                                      
crude_gfx_destroy_descriptor_set_layout
(                                                   
  _In_ crude_gfx_device                                   *gpu,
  _In_ crude_gfx_descriptor_set_layout_handle              handle
)
{
  if ( handle.index >= gpu->descriptor_set_layouts.pool_size )
  {
    CRUDE_LOG_ERROR( CRUDE_CHANNEL_GRAPHICS, "Trying to free invalid descriptor set layout %u", handle.index );
    return;
  }
  crude_gfx_resource_update descriptor_set_layout_update_event = { 
    .type          = CRUDE_GFX_RESOURCE_DELETION_TYPE_DESCRIPTOR_SET_LAYOUT,
    .handle        = handle.index,
    .current_frame = gpu->current_frame };
  CRUDE_ARR_PUT( gpu->resource_deletion_queue, descriptor_set_layout_update_event );
}

void
crude_gfx_destroy_descriptor_set_layout_instant
(                                                   
  _In_ crude_gfx_device                                   *gpu,
  _In_ crude_gfx_descriptor_set_layout_handle              handle
)
{
  crude_gfx_descriptor_set_layout *descriptor_set_layout = CRUDE_GFX_ACCESS_DESCRIPTOR_SET_LAYOUT( gpu, handle );
  gpu->allocator.deallocate( descriptor_set_layout->bindings );
  vkDestroyDescriptorSetLayout( gpu->vk_device, descriptor_set_layout->vk_descriptor_set_layout, gpu->vk_allocation_callbacks );
  CRUDE_GFX_RELEASE_DESCRIPTOR_SET_LAYOUT( gpu, handle );
}

VkShaderModuleCreateInfo
crude_gfx_compile_shader
(
  _In_ const                                              *code,
  _In_ uint32                                              code_size,
  _In_ VkShaderStageFlagBits                               stage,
  _In_ char const                                         *name
)
{
  CRUDE_ABORT( CRUDE_CHANNEL_GRAPHICS, "TODO" );
}

/************************************************
 *
 * Local Vulkan Helper Functions Implementation.
 * 
 ***********************************************/
VKAPI_ATTR VkBool32
_vk_debug_callback
(
  _In_ VkDebugUtilsMessageSeverityFlagBitsEXT        messageSeverity,
  _In_ VkDebugUtilsMessageTypeFlagsEXT               messageType,
  _In_ VkDebugUtilsMessengerCallbackDataEXT const   *pCallbackData,
  _In_ void                                         *pUserData
)
{
  if ( messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT )
  {
    CRUDE_LOG_ERROR( CRUDE_CHANNEL_GRAPHICS, "%s", pCallbackData->pMessage );
  }
  else if ( messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT )
  {
    CRUDE_LOG_WARNING( CRUDE_CHANNEL_GRAPHICS, "%s", pCallbackData->pMessage );
  }
  //else if ( messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT )
  //{
  //  CRUDE_LOG_INFO( CRUDE_CHANNEL_GRAPHICS, "%s", pCallbackData->pMessage );
  //}
  return VK_FALSE;
}

VkInstance
_vk_create_instance
(
  _In_     char const                               *vk_application_name,
  _In_     uint32                                    vk_application_version,
  _In_opt_ VkAllocationCallbacks                    *vk_allocation_callbacks
)
{
  // extensions
  uint32 surface_extensions_count;
  char const *const *surface_extensions_array = SDL_Vulkan_GetInstanceExtensions( &surface_extensions_count );
  uint32 const debug_extensions_count = CRUDE_STACK_ARRAY_SIZE( vk_instance_required_extensions );

  char const **instance_enabled_extensions = NULL;
  uint32 const instance_enabled_extensions_count = surface_extensions_count + debug_extensions_count;
  CRUDE_ARR_SETLEN( instance_enabled_extensions, instance_enabled_extensions_count ); // tofree
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
    .apiVersion         = VK_API_VERSION_1_3 
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
  instance_create_info.enabledLayerCount       = CRUDE_STACK_ARRAY_SIZE( instance_enabled_layers );
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

  CRUDE_ARR_FREE( instance_enabled_extensions );

  return handle;
}

VkDebugUtilsMessengerEXT
_vk_create_debug_utils_messsenger
(
  _In_     VkInstance                                instance,
  _In_opt_ VkAllocationCallbacks                    *allocation_callbacks
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

VkSurfaceKHR
_vk_create_surface
(
  _In_     SDL_Window                               *sdl_window,
  _In_     VkInstance                                vk_instance,
  _In_opt_ VkAllocationCallbacks                    *vk_allocation_callbacks
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

int32
_vk_get_supported_queue_family_index
(
  _In_ VkPhysicalDevice                              vk_physical_device,
  _In_ VkSurfaceKHR                                  vk_surface
)
{
  uint32 queue_family_count = 0u;
  vkGetPhysicalDeviceQueueFamilyProperties( vk_physical_device, &queue_family_count, NULL );
  if ( queue_family_count == 0u )
  {
    return -1;
  }
  
  VkQueueFamilyProperties *queue_families_properties = NULL;
  CRUDE_ARR_SETLEN( queue_families_properties, queue_family_count ); // tofree
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
  CRUDE_ARR_FREE( queue_families_properties );
  return queue_index;
}

bool
_vk_check_support_required_extensions
(
  _In_ VkPhysicalDevice                              vk_physical_device
)
{
  uint32 available_extensions_count = 0u;
  vkEnumerateDeviceExtensionProperties( vk_physical_device, NULL, &available_extensions_count, NULL );
  if ( available_extensions_count == 0u)
  {
    return false;
  }
    
  VkExtensionProperties *available_extensions = NULL;
  CRUDE_ARR_SETLEN( available_extensions, available_extensions_count ); // tofree
  vkEnumerateDeviceExtensionProperties( vk_physical_device, NULL, &available_extensions_count, available_extensions );

  bool support_required_extensions = true;
  for ( uint32 i = 0; i < CRUDE_STACK_ARRAY_SIZE( vk_device_required_extensions ); ++i )
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

  CRUDE_ARR_FREE( available_extensions );
  return support_required_extensions;
}

bool
_vk_check_swap_chain_adequate
(
  _In_ VkPhysicalDevice                              vk_physical_device,
  _In_ VkSurfaceKHR                                  vk_surface
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

bool
_vk_check_support_required_features
(
  _In_ VkPhysicalDevice                              vk_physical_device
)
{
  VkPhysicalDeviceFeatures features;
  vkGetPhysicalDeviceFeatures( vk_physical_device, &features );
  return features.samplerAnisotropy;
}

VkPhysicalDevice
_vk_pick_physical_device
(
  _In_  VkInstance                                   vk_instance,
  _In_  VkSurfaceKHR                                 vk_surface
)
{
  uint32 physical_devices_count = 0u;
  vkEnumeratePhysicalDevices( vk_instance, &physical_devices_count, NULL );
  
  if ( physical_devices_count == 0u ) 
  {
    return VK_NULL_HANDLE;
  }
  
  VkPhysicalDevice *physical_devices = NULL;
  CRUDE_ARR_PUT( physical_devices, physical_devices_count ); // tofree
  vkEnumeratePhysicalDevices( vk_instance, &physical_devices_count, physical_devices );
  
  VkPhysicalDevice selected_physical_devices = VK_NULL_HANDLE;
  for ( uint32 physical_device_index = 0; physical_device_index < physical_devices_count; ++physical_device_index )
  {
    VkPhysicalDeviceProperties vk_physical_properties;
    VkPhysicalDevice physical_device = physical_devices[physical_device_index];
    vkGetPhysicalDeviceProperties( physical_device, &vk_physical_properties );
    
    if ( vk_physical_properties.deviceType != VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU )
    {
      continue;
    }
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

VkDevice
_vk_create_device
(
  _In_ VkPhysicalDevice                              vk_physical_device,
  _Out_ VkQueue                                     *vk_main_queue,
  _Out_ VkQueue                                     *vk_transfer_queue,
  _Out_ uint32                                      *vk_main_queue_family,
  _Out_ uint32                                      *vk_transfer_queue_family,
  _In_opt_ VkAllocationCallbacks                    *vk_allocation_callbacks
)
{
  uint32 queue_family_count = 0;
  vkGetPhysicalDeviceQueueFamilyProperties( vk_physical_device, &queue_family_count, NULL );
  
  VkQueueFamilyProperties *queue_families = NULL;
  CRUDE_ARR_SETLEN( queue_families, queue_family_count );
  vkGetPhysicalDeviceQueueFamilyProperties( vk_physical_device, &queue_family_count, queue_families );
  
  uint32 main_queue_index = UINT32_MAX;
  uint32 transfer_queue_index = UINT32_MAX;
  uint32 compute_queue_index = UINT32_MAX;
  uint32 present_queue_index = UINT32_MAX;
  for ( uint32 family_index = 0; family_index < queue_family_count; ++family_index )
  {
    VkQueueFamilyProperties queue_family = queue_families[ family_index ];
    
    if ( queue_family.queueCount == 0 )
    {
      continue;
    }
    
    if ( ( queue_family.queueFlags & ( VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT ) ) == ( VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT  ) )
    {
      main_queue_index = family_index;
    }

    if ( ( queue_family.queueFlags & VK_QUEUE_COMPUTE_BIT ) == 0 && ( queue_family.queueFlags & VK_QUEUE_TRANSFER_BIT ) )
    {
      transfer_queue_index = family_index;
    }
  }
  
  *vk_main_queue_family = main_queue_index;
  *vk_transfer_queue_family = transfer_queue_index;

  CRUDE_ARR_FREE( queue_families );

  float const queue_priority[] = { 1.0f };

  VkDeviceQueueCreateInfo queue_info[ 2 ];
  memset( queue_info, 0, sizeof( queue_info ) );
  VkDeviceQueueCreateInfo *main_queue = &queue_info[ 0 ];
  main_queue->sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
  main_queue->queueFamilyIndex = main_queue_index;
  main_queue->queueCount = 1;
  main_queue->pQueuePriorities = queue_priority;
  
  if ( transfer_queue_index < queue_family_count )
  {
    VkDeviceQueueCreateInfo *transfer_queue_info = &queue_info[ 1 ];
    transfer_queue_info->sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    transfer_queue_info->queueFamilyIndex = transfer_queue_index;
    transfer_queue_info->queueCount = 1;
    transfer_queue_info->pQueuePriorities = queue_priority;
  }

  VkPhysicalDeviceDescriptorIndexingFeatures indexing_features = {
    .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_INDEXING_FEATURES
  };

  VkPhysicalDeviceFeatures2 physical_features2 = { 
    .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2,
    .pNext = &indexing_features,
  };
  vkGetPhysicalDeviceFeatures2( vk_physical_device, &physical_features2 );

  VkDeviceCreateInfo device_create_info = {
    .sType                    = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
    .pNext                    = &physical_features2,
    .flags                    = 0u,
    .queueCreateInfoCount     = transfer_queue_index < queue_family_count ? 2 : 1,
    .pQueueCreateInfos        = queue_info,
    .pEnabledFeatures         = NULL,
    .enabledExtensionCount    = CRUDE_STACK_ARRAY_SIZE( vk_device_required_extensions ),
    .ppEnabledExtensionNames  = vk_device_required_extensions,
    .enabledLayerCount        = CRUDE_STACK_ARRAY_SIZE( vk_required_layers ),
    .ppEnabledLayerNames      = vk_required_layers,
  };
  VkDevice device;
  CRUDE_GFX_HANDLE_VULKAN_RESULT( vkCreateDevice( vk_physical_device, &device_create_info, vk_allocation_callbacks, &device ), "failed to create logic device!" );
  vkGetDeviceQueue( device, main_queue_index, 0u, vk_main_queue );
  if ( transfer_queue_index < queue_family_count )
  {
    vkGetDeviceQueue( device, transfer_queue_index, 0, vk_transfer_queue );
  }

  return device;
}

VkSwapchainKHR
_vk_create_swapchain
( 
  _In_      VkDevice                                 vk_device, 
  _In_      VkPhysicalDevice                         vk_physical_device, 
  _In_      VkSurfaceKHR                             vk_surface, 
  _In_      int32                                    vk_queue_family_index,
  _In_opt_  VkAllocationCallbacks                   *vk_allocation_callbacks,
  _Out_     uint32                                  *vk_swapchain_images_count,
  _Out_     VkImage                                 *vk_swapchain_images,
  _Out_     VkImageView                             *vk_swapchain_images_views,
  _Out_     VkSurfaceFormatKHR                      *vk_surface_format,
  _Out_     uint16                                  *vk_swapchain_width,
  _Out_     uint16                                  *vk_swapchain_height
)
{
  VkSurfaceCapabilitiesKHR surface_capabilities;
  vkGetPhysicalDeviceSurfaceCapabilitiesKHR( vk_physical_device, vk_surface, &surface_capabilities);
  
  VkExtent2D swapchain_extent = surface_capabilities.currentExtent;
  if ( swapchain_extent.width == UINT32_MAX )
  {
    swapchain_extent.width = crude_clamp( swapchain_extent.width, surface_capabilities.minImageExtent.width, surface_capabilities.maxImageExtent.width );
    swapchain_extent.height = crude_clamp( swapchain_extent.height, surface_capabilities.minImageExtent.height, surface_capabilities.maxImageExtent.height );
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
  CRUDE_ARR_SETLEN( available_formats, available_formats_count ); // tofree
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
    CRUDE_ARR_FREE( available_formats );
    return VK_NULL_HANDLE;
  }
  
  uint32 available_present_modes_count;
  vkGetPhysicalDeviceSurfacePresentModesKHR( vk_physical_device, vk_surface, &available_present_modes_count, NULL );
  if ( available_present_modes_count == 0u ) 
  {
    CRUDE_LOG_ERROR( CRUDE_CHANNEL_GRAPHICS, "Can't find available surface present_mode" );
    CRUDE_ARR_FREE( available_formats );
    return VK_NULL_HANDLE;
  }
  
  CRUDE_ARR( VkPresentModeKHR ) available_present_modes = NULL;
  CRUDE_ARR_SETLEN( available_present_modes, available_present_modes_count ); // tofree
  vkGetPhysicalDeviceSurfacePresentModesKHR( vk_physical_device, vk_surface, &available_present_modes_count, available_present_modes );
  
  VkPresentModeKHR surface_present_mode = VK_PRESENT_MODE_FIFO_KHR;
  for ( uint32 i = 0; i < available_present_modes_count; ++i )
  {
    if ( available_present_modes[ i ] == VK_PRESENT_MODE_MAILBOX_KHR )
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
    .imageSharingMode       = CRUDE_STACK_ARRAY_SIZE( queue_family_indices ) > 1 ? VK_SHARING_MODE_CONCURRENT : VK_SHARING_MODE_EXCLUSIVE,
    .queueFamilyIndexCount  = CRUDE_STACK_ARRAY_SIZE( queue_family_indices ),
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

  CRUDE_ARR_FREE( available_formats );
  CRUDE_ARR_FREE( available_present_modes );

  return vulkan_swapchain;
}

VmaAllocation
_vk_create_vma_allocator
(
  _In_ VkDevice                                      vk_device,
  _In_ VkPhysicalDevice                              vk_physical_device,
  _In_ VkInstance                                    vk_instance
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

void
_vk_create_descriptor_pool
(
  _In_     VkDevice                                  vk_device,
  _In_opt_ VkAllocationCallbacks                    *vk_allocation_callbacks,
  _Out_    VkDescriptorPool                         *vk_bindless_descriptor_pool,
  _Out_    VkDescriptorSetLayout                    *vk_bindless_descriptor_set_layout,
  _Out_    VkDescriptorSet                          *vk_bindless_descriptor_set
)
{
  VkDescriptorPoolSize pool_sizes_bindless[] =
  {
    { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, MAX_BINDLESS_RESOURCES },
    { VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, MAX_BINDLESS_RESOURCES },
  };
  
  VkDescriptorPoolCreateInfo pool_info = {
    .sType         = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
    .flags         = VK_DESCRIPTOR_POOL_CREATE_UPDATE_AFTER_BIND_BIT_EXT,
    .maxSets       = MAX_BINDLESS_RESOURCES * CRUDE_STACK_ARRAY_SIZE( pool_sizes_bindless ),
    .poolSizeCount = CRUDE_STACK_ARRAY_SIZE( pool_sizes_bindless ),
    .pPoolSizes    = pool_sizes_bindless,
  };
  CRUDE_GFX_HANDLE_VULKAN_RESULT( vkCreateDescriptorPool( vk_device, &pool_info, vk_allocation_callbacks, vk_bindless_descriptor_pool ), "Failed create descriptor pool" );

  uint32 pool_count = CRUDE_STACK_ARRAY_SIZE( pool_sizes_bindless );
  VkDescriptorSetLayoutBinding vk_binding[ 2 ];
  VkDescriptorSetLayoutBinding  *image_sampler_binding = &vk_binding[ 0 ];
  image_sampler_binding->descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
  image_sampler_binding->descriptorCount = MAX_BINDLESS_RESOURCES;
  image_sampler_binding->binding = BINDLESS_TEXTURE_BINDING;
  image_sampler_binding->stageFlags = VK_SHADER_STAGE_ALL;
  image_sampler_binding->pImmutableSamplers = NULL;

  VkDescriptorSetLayoutBinding  *storage_image_binding = &vk_binding[ 1 ];
  storage_image_binding->descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
  storage_image_binding->descriptorCount = MAX_BINDLESS_RESOURCES;
  storage_image_binding->binding = BINDLESS_TEXTURE_BINDING + 1;
  storage_image_binding->stageFlags = VK_SHADER_STAGE_ALL;
  storage_image_binding->pImmutableSamplers = NULL;
        
  VkDescriptorBindingFlags bindless_flags = /*VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT_EXT |*/ VK_DESCRIPTOR_BINDING_UPDATE_AFTER_BIND_BIT_EXT;
  VkDescriptorBindingFlags binding_flags[ 2 ] = { bindless_flags, bindless_flags };
  
  VkDescriptorSetLayoutBindingFlagsCreateInfoEXT extended_info = {
    .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO_EXT,
    .bindingCount = pool_count,
    .pBindingFlags = binding_flags
  };

  VkDescriptorSetLayoutCreateInfo layout_info = {
    .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
    .bindingCount = pool_count,
    .pBindings = vk_binding,
    .flags = VK_DESCRIPTOR_SET_LAYOUT_CREATE_UPDATE_AFTER_BIND_POOL_BIT_EXT,
    .pNext = &extended_info
  };
  
  CRUDE_GFX_HANDLE_VULKAN_RESULT( vkCreateDescriptorSetLayout( vk_device, &layout_info, vk_allocation_callbacks, vk_bindless_descriptor_set_layout ), "Failed create descriptor set layout" );
  
  VkDescriptorSetAllocateInfo alloc_info = {
    .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
    .descriptorPool = *vk_bindless_descriptor_pool,
    .descriptorSetCount = 1,
    .pSetLayouts = vk_bindless_descriptor_set_layout
  };
  
  CRUDE_GFX_HANDLE_VULKAN_RESULT( vkAllocateDescriptorSets( vk_device, &alloc_info, vk_bindless_descriptor_set ), "Failed allocate descriptor set" );
}

VkQueryPool
_vk_create_timestamp_query_pool
(
  _In_     VkDevice                                  vk_device, 
  _In_     int32                                     max_frames,
  _In_opt_ VkAllocationCallbacks                    *vk_allocation_callbacks
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

void
_vk_create_swapchain_pass
(
  _In_ crude_gfx_device                             *gpu,
  _In_ crude_gfx_render_pass_creation                   *creation,
  _Out_ crude_gfx_render_pass                           *render_pass
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
  
  crude_gfx_texture* depth_texture = crude_resource_pool_access_resource( &gpu->textures, gpu->depth_texture.index );
  VkAttachmentDescription depth_attachment = {
    .format         = depth_texture->vk_format,
    .samples        = VK_SAMPLE_COUNT_1_BIT,
    .loadOp         = VK_ATTACHMENT_LOAD_OP_CLEAR,
    .storeOp        = VK_ATTACHMENT_STORE_OP_STORE,
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
    .attachmentCount  = CRUDE_STACK_ARRAY_SIZE( attachments ),
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
    .attachmentCount  = CRUDE_STACK_ARRAY_SIZE( framebuffer_attachments ),
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

  crude_gfx_cmd_buffer *command_buffer = crude_gfx_cmd_manager_get_primary_cmd( &g_command_buffer_manager, gpu->current_frame, 0u, true );
  for ( uint64 i = 0; i < gpu->vk_swapchain_images_count; ++i )
  {
    crude_gfx_cmd_add_image_barrier( command_buffer, gpu->vk_swapchain_images[ i ], CRUDE_GFX_RESOURCE_STATE_UNDEFINED, CRUDE_GFX_RESOURCE_STATE_PRESENT, 0u, 1u, false );
  }
  crude_gfx_cmd_end( command_buffer );

  VkSubmitInfo submitInfo = { 
    .sType              = VK_STRUCTURE_TYPE_SUBMIT_INFO,
    .commandBufferCount = 1,
    .pCommandBuffers    = &command_buffer->vk_cmd_buffer,
  };
  
  vkQueueSubmit( gpu->vk_main_queue, 1, &submitInfo, VK_NULL_HANDLE );
  vkQueueWaitIdle( gpu->vk_main_queue );
}

void
_vk_destroy_swapchain
(
  _In_     VkDevice                                  vk_device,
  _In_     VkSwapchainKHR                            vulkan_swapchain,
  _In_     uint32                                    vk_swapchain_images_count,
  _In_     VkImageView                              *vk_swapchain_images_views,
  _In_     VkFramebuffer                            *vk_swapchain_framebuffers,
  _In_opt_ VkAllocationCallbacks                    *vk_allocation_callbacks
)
{
  for ( uint32 i = 0; i < vk_swapchain_images_count; ++i )
  {
    vkDestroyImageView( vk_device, vk_swapchain_images_views[ i ], vk_allocation_callbacks );
    vkDestroyFramebuffer( vk_device, vk_swapchain_framebuffers[ i ], vk_allocation_callbacks );
  }
  vkDestroySwapchainKHR( vk_device, vulkan_swapchain, vk_allocation_callbacks );
}

void
_vk_create_texture
(
  _In_ crude_gfx_device                             *gpu,
  _In_ crude_gfx_texture_creation const                 *creation,
  _In_ crude_gfx_texture_handle                          handle,
  _In_ crude_gfx_texture                                *texture
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
    .imageType      = crude_gfx_to_vk_image_type( creation->type ),
    .extent.width   = creation->width,
    .extent.height  = creation->height,
    .extent.depth   = creation->depth,
    .mipLevels      = creation->mipmaps,
    .arrayLayers    = 1,
    .samples        = VK_SAMPLE_COUNT_1_BIT,
    .tiling         = VK_IMAGE_TILING_OPTIMAL,
  };
  
  bool const is_render_target = ( creation->flags & CRUDE_GFX_TEXTURE_MASK_RENDER_TARGET ) == CRUDE_GFX_TEXTURE_MASK_RENDER_TARGET;
  bool const is_compute_used = ( creation->flags & CRUDE_GFX_TEXTURE_MASK_COMPUTE ) == CRUDE_GFX_TEXTURE_MASK_COMPUTE;
  
  image_info.usage = VK_IMAGE_USAGE_SAMPLED_BIT;
  image_info.usage |= is_compute_used ? VK_IMAGE_USAGE_STORAGE_BIT : 0;
  
  if ( crude_gfx_has_depth_or_stencil( creation->format ) )
  {
    image_info.usage |= VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
  }
  else
  {
    image_info.usage |= VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
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
    .viewType = crude_gfx_to_vk_image_view_type( creation->type ),
    .format   = image_info.format,
  };
  
  if ( crude_gfx_has_depth_or_stencil( creation->format ) )
  {
    info.subresourceRange.aspectMask = crude_gfx_has_depth( creation->format ) ? VK_IMAGE_ASPECT_DEPTH_BIT : 0;
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
  
  crude_gfx_resource_update texture_update_event = { 
    .type          = CRUDE_GFX_RESOURCE_DELETION_TYPE_TEXTURE,
    .handle        = handle.index,
    .current_frame = gpu->current_frame };
  CRUDE_ARR_PUT( gpu->texture_to_update_bindless, texture_update_event );
}

void
_vk_resize_texture
(
  _In_ crude_gfx_device                             *gpu,
  _In_ crude_gfx_texture                                *texture,
  _In_ crude_gfx_texture                                *texture_to_delete,
  _In_ uint16                                        width,
  _In_ uint16                                        height,
  _In_ uint16                                        depth
)
{
  texture_to_delete->vk_image_view = texture->vk_image_view;
  texture_to_delete->vk_image = texture->vk_image;
  texture_to_delete->vma_allocation = texture->vma_allocation;
  
  crude_gfx_texture_creation texture_creation = {
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

void
_vk_resize_swapchain
(
  _In_ crude_gfx_device *gpu
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

  crude_gfx_render_pass* swapchain_pass = CRUDE_GFX_ACCESS_RENDER_PASS( gpu, gpu->swapchain_pass );
  vkDestroyRenderPass( gpu->vk_device, swapchain_pass->vk_render_pass, gpu->vk_allocation_callbacks );
  
  _vk_destroy_swapchain( gpu->vk_device, gpu->vk_swapchain, gpu->vk_swapchain_images_count, gpu->vk_swapchain_images_views, gpu->vk_swapchain_framebuffers, gpu->vk_allocation_callbacks );
  vkDestroySurfaceKHR( gpu->vk_instance, gpu->vk_surface, gpu->vk_allocation_callbacks );

  if ( !SDL_Vulkan_CreateSurface( gpu->sdl_window, gpu->vk_instance, gpu->vk_allocation_callbacks, &gpu->vk_surface ) )
  {
    CRUDE_ABORT( CRUDE_CHANNEL_GRAPHICS, "Failed to create vk_surface: %s!", SDL_GetError() );
  }
  
  gpu->vk_swapchain = _vk_create_swapchain( gpu->vk_device, gpu->vk_physical_device, gpu->vk_surface, gpu->vk_main_queue_family, gpu->vk_allocation_callbacks, &gpu->vk_swapchain_images_count, gpu->vk_swapchain_images, gpu->vk_swapchain_images_views, &gpu->vk_surface_format, &gpu->vk_swapchain_width, &gpu->vk_swapchain_height);
  
  crude_gfx_texture_handle texture_to_delete_handle = { CRUDE_GFX_OBTAIN_TEXTURE( gpu ) };
  crude_gfx_texture *texture_to_delete = CRUDE_GFX_ACCESS_TEXTURE( gpu, texture_to_delete_handle );
  texture_to_delete->handle = texture_to_delete_handle;

  crude_gfx_texture *depth_texture = CRUDE_GFX_ACCESS_TEXTURE( gpu, gpu->depth_texture );
  _vk_resize_texture( gpu, depth_texture, texture_to_delete, gpu->vk_swapchain_width, gpu->vk_swapchain_height, 1 );
  crude_gfx_destroy_texture( gpu, texture_to_delete_handle );
  
  crude_gfx_render_pass_creation swapchain_pass_creation = {
    .type                  = CRUDE_GFX_RENDER_PASS_TYPE_SWAPCHAIN,
    .name                  = "swapchain",
    .color_operation       = CRUDE_GFX_RENDER_PASS_OPERATION_CLEAR,
    .depth_operation       = CRUDE_GFX_RENDER_PASS_OPERATION_CLEAR,
    .depth_stencil_texture = CRUDE_GFX_RENDER_PASS_OPERATION_CLEAR,
    .scale_x               = 1.f,
    .scale_y               = 1.f,
    .resize                = 1,
  };
  _vk_create_swapchain_pass( gpu, &swapchain_pass_creation, swapchain_pass );
  
  vkDeviceWaitIdle( gpu->vk_device );
}

inline crude_gfx_vertex_component_format
_reflect_format_to_vk_format
(
  _In_ SpvReflectFormat                                    format
)
{
  switch ( format )
  {
    case SPV_REFLECT_FORMAT_R16G16_SINT         : return CRUDE_GFX_VERTEX_COMPONENT_FORMAT_SHORT2;
    case SPV_REFLECT_FORMAT_R16G16B16A16_SINT   : return CRUDE_GFX_VERTEX_COMPONENT_FORMAT_SHORT4;
    case SPV_REFLECT_FORMAT_R32_UINT            : return CRUDE_GFX_VERTEX_COMPONENT_FORMAT_UINT;
    case SPV_REFLECT_FORMAT_R32_SFLOAT          : return CRUDE_GFX_VERTEX_COMPONENT_FORMAT_FLOAT;
    case SPV_REFLECT_FORMAT_R32G32_UINT         : return CRUDE_GFX_VERTEX_COMPONENT_FORMAT_UINT2;
    case SPV_REFLECT_FORMAT_R32G32_SFLOAT       : return CRUDE_GFX_VERTEX_COMPONENT_FORMAT_FLOAT2;
    case SPV_REFLECT_FORMAT_R32G32B32_SFLOAT    : return CRUDE_GFX_VERTEX_COMPONENT_FORMAT_FLOAT3;
    case SPV_REFLECT_FORMAT_R32G32B32A32_UINT   : return CRUDE_GFX_VERTEX_COMPONENT_FORMAT_UINT4;
    case SPV_REFLECT_FORMAT_R32G32B32A32_SFLOAT : return CRUDE_GFX_VERTEX_COMPONENT_FORMAT_FLOAT4;
  };
  CRUDE_LOG_ERROR( CRUDE_CHANNEL_GRAPHICS, "Don't support reflect format %i", format );
}

void
_vk_reflect_shader
(
  _In_ crude_gfx_device                                   *gpu,
  _In_ void                                               *code,
  _In_ uint32                                              code_size,
  _In_ crude_gfx_shader_reflect                           *reflect
)
{
  SpvReflectShaderModule spv_reflect;
  SpvReflectResult result = spvReflectCreateShaderModule( code_size, code, &spv_reflect );
  CRUDE_ASSERT( result == SPV_REFLECT_RESULT_SUCCESS );
  
  if ( spv_reflect.shader_stage == SPV_REFLECT_SHADER_STAGE_VERTEX_BIT )
  {
    CRUDE_ARR_SETLEN( reflect->input.vertex_attributes, spv_reflect.input_variable_count );
    CRUDE_ARR_SETLEN( reflect->input.vertex_streams, spv_reflect.input_variable_count );
    for ( uint32 input_index = 0; input_index < spv_reflect.input_variable_count; ++input_index )
    {
      SpvReflectInterfaceVariable const *spv_input = spv_reflect.input_variables[ input_index ];
      
      reflect->input.vertex_attributes[ input_index ] = ( crude_gfx_vertex_attribute ) {
        .location = spv_input->location,
        .binding = spv_input->location,
        .offset = 0,
        .format = _reflect_format_to_vk_format( spv_input->format )
      };
      
      uint32 const stride = ( spv_input->numeric.vector.component_count * spv_input->numeric.scalar.width ) / 8;
      reflect->input.vertex_streams[ input_index ] = ( crude_gfx_vertex_stream ){
        .binding = spv_input->location,
        .stride = stride,
        .input_rate = CRUDE_GFX_VERTEX_INPUT_RATE_PER_VERTEX
      };
    }
  }

  for ( uint32 set_index = 0; set_index < spv_reflect.descriptor_set_count; ++set_index )
  {
    SpvReflectDescriptorSet const *spv_descriptor_set = &spv_reflect.descriptor_sets[ set_index ];

    crude_gfx_descriptor_set_layout_creation *set_layout = &reflect->descriptor.sets[ spv_descriptor_set->set ];
    set_layout->set_index = spv_descriptor_set->set;
    set_layout->num_bindings = 0;

    for ( uint32 binding_index = 0; binding_index < spv_descriptor_set->binding_count; ++binding_index )
    {
      SpvReflectDescriptorBinding const *spv_binding = spv_descriptor_set->bindings[ binding_index ];
      crude_gfx_descriptor_set_layout_binding *binding = &set_layout->bindings[ binding_index ];
      memset( binding, 0, sizeof( crude_gfx_descriptor_set_layout_binding ) );
      binding->start = spv_binding->binding;
      binding->name  = spv_binding->name; //!TODO UNSAFE
      binding->count = spv_binding->count;
      
      switch ( spv_binding->descriptor_type )
      {
        case SPV_REFLECT_DESCRIPTOR_TYPE_UNIFORM_BUFFER:
        {
          reflect->descriptor.sets_count = crude_max( reflect->descriptor.sets_count, ( spv_descriptor_set->set + 1 ) );
          binding->type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
          ++set_layout->num_bindings;
          break;
        }
      }
    }
  }

  spvReflectDestroyShaderModule( &spv_reflect );
}

void
_vk_destroy_resources_instant
(
  _In_ crude_gfx_device                             *gpu,
  _In_ crude_gfx_resource_deletion_type                  type,
  _In_ crude_gfx_resource_index                         handle
)
{
  switch ( type )
  {
    case CRUDE_GFX_RESOURCE_DELETION_TYPE_SAMPLER:
    {
      crude_gfx_destroy_sampler_instant( gpu, ( crude_gfx_sampler_handle ){ handle } );
      break;
    }
    case CRUDE_GFX_RESOURCE_DELETION_TYPE_TEXTURE:
    {
      crude_gfx_destroy_texture_instant( gpu, ( crude_gfx_texture_handle ){ handle } );
      break;
    }
    case CRUDE_GFX_RESOURCE_DELETION_TYPE_RENDER_PASS:
    {
      crude_gfx_destroy_render_pass_instant( gpu, ( crude_gfx_render_pass_handle ){ handle } );
      break;
    }
    case CRUDE_GFX_RESOURCE_DELETION_TYPE_SHADER_STATE:
    {
      crude_gfx_destroy_shader_state_instant( gpu, ( crude_gfx_shader_state_handle ){ handle } );
      break;
    }
    case CRUDE_GFX_RESOURCE_DELETION_TYPE_PIPELINE:
    {
      crude_gfx_destroy_pipeline_instant( gpu, ( crude_gfx_pipeline_handle ){ handle } );
      break;
    }
    case CRUDE_GFX_RESOURCE_DELETION_TYPE_BUFFER:
    {
      crude_gfx_destroy_buffer_instant( gpu, ( crude_gfx_buffer_handle ){ handle } );
      break;
    }
    case CRUDE_GFX_RESOURCE_DELETION_TYPE_DESCRIPTOR_SET_LAYOUT:
    {
      crude_gfx_destroy_descriptor_set_layout_instant( gpu, ( crude_gfx_descriptor_set_layout_handle ){ handle } );
      break;
    }
  }
}