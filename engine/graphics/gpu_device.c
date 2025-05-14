#include <core/profiler.h>
#include <core/string.h>
#include <core/file.h>
#include <core/process.h>

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>

#include <graphics/gpu_device.h>

/************************************************
 *
 * Constants
 * 
 ***********************************************/
static char const *const vk_device_required_extensions[] = 
{ 
  VK_KHR_SWAPCHAIN_EXTENSION_NAME,
  VK_KHR_MAINTENANCE_1_EXTENSION_NAME,
  VK_EXT_DESCRIPTOR_INDEXING_EXTENSION_NAME,
  VK_KHR_DYNAMIC_RENDERING_EXTENSION_NAME
};

static char const *const *vk_instance_required_extensions[] =
{
  VK_EXT_DEBUG_UTILS_EXTENSION_NAME
};

static char const *const *vk_required_layers[] =
{
  "VK_LAYER_KHRONOS_validation"
};

static char const *instance_enabled_layers[] =
{
  "VK_LAYER_KHRONOS_validation"
};

/************************************************
 *
 * Local Vulkan Helper Functions Declaration.
 * 
 ***********************************************/
static VKAPI_ATTR VkBool32
vk_debug_callback_
(
  _In_ VkDebugUtilsMessageSeverityFlagBitsEXT              messageSeverity,
  _In_ VkDebugUtilsMessageTypeFlagsEXT                     messageType,
  _In_ VkDebugUtilsMessengerCallbackDataEXT const         *pCallbackData,
  _In_ void                                               *pUserData
);
static void
vk_create_instance_
(
  _In_ crude_gfx_device                                   *gpu,
  _In_ char const                                         *vk_application_name,
  _In_ uint32                                              vk_application_version,
  _In_ crude_allocator_container                           temporary_allocator
);
static void
vk_create_debug_utils_messsenger_
(
  _In_ crude_gfx_device                                   *gpu
);
static void
vk_create_surface_
(
  _In_ crude_gfx_device                                   *gpu
);
static void
vk_pick_physical_device_
(
  _In_ crude_gfx_device                                   *gpu,
  _In_  crude_allocator_container                          temporary_allocator
);
static void
vk_create_device_
(
  _In_ crude_gfx_device                                   *gpu,
  _In_ crude_allocator_container                           temporary_allocator
);
static void
vk_create_swapchain_
( 
  _In_ crude_gfx_device                                   *gpu,
  _In_ crude_allocator_container                           temporary_allocator
);
static void
vk_create_vma_allocator_
(
  _In_ crude_gfx_device                                   *gpu
);
static void
vk_create_descriptor_pool_
(
  _In_ crude_gfx_device                                   *gpu
);
static void
vk_create_timestamp_query_pool_
(
  _In_ crude_gfx_device                                   *gpu
);
static int32
vk_get_supported_queue_family_index_
(
  _In_ VkPhysicalDevice                                    vk_physical_device,
  _In_ VkSurfaceKHR                                        vk_surface,
  _In_ crude_allocator_container                           temporary_allocator
);
static bool
vk_check_support_required_extensions_
(
  _In_ VkPhysicalDevice                                    vk_physical_device,
  _In_ crude_allocator_container                           temporary_allocator
);
static bool
vk_check_swap_chain_adequate_
(
  _In_ VkPhysicalDevice                                    vk_physical_device,
  _In_ VkSurfaceKHR                                        vk_surface
);
static bool
vk_check_support_required_features_
(
  _In_ VkPhysicalDevice                                    vk_physical_device
);
static void
vk_destroy_swapchain_
(
  _In_ crude_gfx_device                                   *gpu
);
static void
vk_create_texture_
(
  _In_ crude_gfx_device                                   *gpu,
  _In_ crude_gfx_texture_creation const                   *creation,
  _In_ crude_gfx_texture_handle                            handle,
  _In_ crude_gfx_texture                                  *texture
);
static void
vk_resize_swapchain_
(
  _In_ crude_gfx_device                                   *gpu
);
static crude_gfx_vertex_component_format
reflect_format_to_vk_format_
(
  _In_ SpvReflectFormat                                    format
);
static void
vk_reflect_shader_
(
  _In_ crude_gfx_device                                   *gpu,
  _In_ void                                               *code,
  _In_ uint32                                              code_size,
  _In_ crude_gfx_shader_reflect                           *reflect
);
static void
vk_destroy_resources_instant_
(
  _In_ crude_gfx_device                                   *gpu,
  _In_ crude_gfx_resource_deletion_type                    type,
  _In_ crude_gfx_resource_index                            handle
);

void dump_shader_code_
(
  _In_ char const                                         *code,
  _In_ VkShaderStageFlagBits                               stage,
  _In_ char const                                         *name,
  _In_ crude_string_buffer                                *temporary_string_buffer
);

VkShaderModuleCreateInfo
crude_gfx_compile_shader
(
  _In_ char const                                         *code,
  _In_ uint32                                              code_size,
  _In_ VkShaderStageFlagBits                               stage,
  _In_ char const                                         *name,
  _In_ crude_stack_allocator                              *temporary_allocator
);

/************************************************
 *
 * GPU Device Initialize/Deinitialize
 * 
 ***********************************************/
void
crude_gfx_device_initialize
(
  _Out_ crude_gfx_device                                  *gpu,
  _In_ crude_gfx_device_creation                          *creation
)
{
  crude_allocator_container                                temporary_allocator;
  uint32                                                   temporary_allocator_mark;
  
  temporary_allocator = crude_stack_allocator_pack( creation->temporary_allocator );
  temporary_allocator_mark = crude_stack_allocator_get_marker( creation->temporary_allocator );

  gpu->sdl_window = creation->sdl_window;
  gpu->allocator_container  = creation->allocator_container;
  gpu->vk_allocation_callbacks = NULL;
  gpu->temporary_allocator = creation->temporary_allocator;
  gpu->previous_frame = 0;
  gpu->current_frame = 1;
  gpu->vk_swapchain_image_index = 0;
  gpu->swapchain_resized_last_frame = false;

  crude_string_buffer_initialize( &gpu->objects_names_string_buffer, CRUDE_RMEGA( 1 ), gpu->allocator_container );

  vk_create_instance_( gpu, creation->vk_application_name, creation->vk_application_version, temporary_allocator );
  vk_create_debug_utils_messsenger_( gpu );
  vk_create_surface_( gpu );
  vk_pick_physical_device_( gpu, temporary_allocator );
  vk_create_device_( gpu, temporary_allocator );
  vk_create_vma_allocator_( gpu );
  
  crude_resource_pool_initialize( &gpu->buffers, gpu->allocator_container, 4096, sizeof( crude_gfx_buffer ) );
  crude_resource_pool_initialize( &gpu->textures, gpu->allocator_container, 512, sizeof( crude_gfx_texture ) );
  crude_resource_pool_initialize( &gpu->render_passes, gpu->allocator_container, 256, sizeof( crude_gfx_render_pass ) );
  crude_resource_pool_initialize( &gpu->descriptor_set_layouts, gpu->allocator_container, 128, sizeof( crude_gfx_descriptor_set_layout ) );
  crude_resource_pool_initialize( &gpu->pipelines, gpu->allocator_container, 128, sizeof( crude_gfx_pipeline ) );
  crude_resource_pool_initialize( &gpu->shaders, gpu->allocator_container, 128, sizeof( crude_gfx_shader_state ) );
  crude_resource_pool_initialize( &gpu->samplers, gpu->allocator_container, 32, sizeof( crude_gfx_sampler ) );
  crude_resource_pool_initialize( &gpu->framebuffers, gpu->allocator_container, 128, sizeof( crude_gfx_framebuffer ) );
  {
    
    VkSemaphoreCreateInfo semaphore_info = { .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO };
    VkFenceCreateInfo fence_info = { .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO, .flags = VK_FENCE_CREATE_SIGNALED_BIT };
    for ( uint32 i = 0; i < CRUDE_GFX_MAX_SWAPCHAIN_IMAGES; ++i )
    {
      vkCreateSemaphore( gpu->vk_device, &semaphore_info, gpu->vk_allocation_callbacks, &gpu->vk_image_avalivable_semaphores[ i ] );
      vkCreateSemaphore( gpu->vk_device, &semaphore_info, gpu->vk_allocation_callbacks, &gpu->vk_swapchain_updated_semaphore[ i ] );
      vkCreateSemaphore( gpu->vk_device, &semaphore_info, gpu->vk_allocation_callbacks, &gpu->vk_rendering_finished_semaphore[ i ] );
      vkCreateFence( gpu->vk_device, &fence_info, gpu->vk_allocation_callbacks, &gpu->vk_command_buffer_executed_fences[ i ] );
    }
  }
  
  crude_gfx_cmd_manager_initialize( &gpu->cmd_buffer_manager, gpu, creation->num_threads );
  CRUDE_ARRAY_INITIALIZE_WITH_CAPACITY( gpu->queued_command_buffers, 128, gpu->allocator_container );
  CRUDE_ARRAY_INITIALIZE_WITH_CAPACITY( gpu->resource_deletion_queue, 16, gpu->allocator_container );
  CRUDE_ARRAY_INITIALIZE_WITH_CAPACITY( gpu->texture_to_update_bindless, 16, gpu->allocator_container );
  
  vk_create_swapchain_( gpu, temporary_allocator );
  vk_create_descriptor_pool_( gpu );
  vk_create_timestamp_query_pool_( gpu );
  crude_stack_allocator_free_marker( creation->temporary_allocator, temporary_allocator_mark );
  
  {
    crude_gfx_texture_creation depth_texture_creation = crude_gfx_texture_creation_empty();
    depth_texture_creation.width    = gpu->vk_swapchain_width;
    depth_texture_creation.height   = gpu->vk_swapchain_height; 
    depth_texture_creation.depth    = 1;
    depth_texture_creation.mipmaps  = 1; 
    depth_texture_creation.format   = VK_FORMAT_D32_SFLOAT;
    depth_texture_creation.type     = CRUDE_GFX_TEXTURE_TYPE_TEXTURE_2D;
    depth_texture_creation.name     = "depth_image_texture";
    gpu->depth_texture = crude_gfx_create_texture( gpu, &depth_texture_creation );
  }
  
  {
    crude_gfx_sampler_creation default_sampler_creation = crude_gfx_sampler_creation_empty();
    default_sampler_creation .address_mode_u = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    default_sampler_creation .address_mode_v = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    default_sampler_creation .address_mode_w = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    default_sampler_creation .min_filter     = VK_FILTER_LINEAR;
    default_sampler_creation .mag_filter     = VK_FILTER_LINEAR;
    default_sampler_creation .mip_filter     = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    default_sampler_creation .name           = "sampler default";
    gpu->default_sampler = crude_gfx_create_sampler( gpu, &default_sampler_creation );
  }
  
  {
    crude_gfx_map_buffer_parameters                        buffer_map;
    crude_gfx_buffer_creation                              dynamic_buffer_creation;
    
    gpu->dynamic_allocated_size = 0;
    gpu->dynamic_max_per_frame_size = 0;
    gpu->dynamic_per_frame_size = 1024 * 1024 * 10;

    dynamic_buffer_creation = crude_gfx_buffer_creation_empty();
    dynamic_buffer_creation.name = "dynamic_persistent_buffer";
    dynamic_buffer_creation.type_flags = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
    dynamic_buffer_creation.usage = CRUDE_GFX_RESOURCE_USAGE_TYPE_IMMUTABLE;
    dynamic_buffer_creation.size = gpu->dynamic_per_frame_size * CRUDE_GFX_MAX_SWAPCHAIN_IMAGES;
    gpu->dynamic_buffer = crude_gfx_create_buffer( gpu, &dynamic_buffer_creation );
    
    buffer_map = ( crude_gfx_map_buffer_parameters ){
      .buffer = gpu->dynamic_buffer
    };
    gpu->dynamic_mapped_memory = ( uint8* )crude_gfx_map_buffer( gpu, &buffer_map );
  }

  
  gpu->swapchain_output.depth_stencil_format = VK_FORMAT_D32_SFLOAT;
  gpu->swapchain_output.depth_stencil_final_layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

  gpu->swapchain_output.depth_operation = CRUDE_GFX_RENDER_PASS_OPERATION_CLEAR;
  gpu->swapchain_output.stencil_operation = CRUDE_GFX_RENDER_PASS_OPERATION_CLEAR;

  gpu->swapchain_output.color_final_layouts[ 0 ] = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
  gpu->swapchain_output.color_formats[ 0 ] = gpu->vk_surface_format.format;
  gpu->swapchain_output.color_operations[ 0 ] = CRUDE_GFX_RENDER_PASS_OPERATION_CLEAR;
  gpu->swapchain_output.num_color_formats = 1u;
  
  crude_stack_allocator_free_marker( creation->temporary_allocator, temporary_allocator_mark );
 }

void
crude_gfx_device_deinitialize
(
  _In_ crude_gfx_device                                   *gpu
)
{
  vkDeviceWaitIdle( gpu->vk_device );
  
  crude_gfx_unmap_buffer( gpu, gpu->dynamic_buffer );
  crude_gfx_destroy_buffer( gpu, gpu->dynamic_buffer );
  crude_gfx_destroy_texture( gpu, gpu->depth_texture );
  crude_gfx_destroy_sampler( gpu, gpu->default_sampler );
  
  for ( uint32 i = 0; i < CRUDE_ARRAY_LENGTH( gpu->resource_deletion_queue ); ++i )
  {
    crude_gfx_resource_update* resource_deletion = &gpu->resource_deletion_queue[ i ];
  
    if ( resource_deletion->current_frame == -1 )
    {
      continue;
    }

    vk_destroy_resources_instant_( gpu, resource_deletion->type, resource_deletion->handle );
  }
  
  vkDestroyQueryPool( gpu->vk_device, gpu->vk_timestamp_query_pool, gpu->vk_allocation_callbacks );
  vkDestroyDescriptorSetLayout( gpu->vk_device, gpu->vk_bindless_descriptor_set_layout, gpu->vk_allocation_callbacks );
  vkDestroyDescriptorPool( gpu->vk_device, gpu->vk_bindless_descriptor_pool, gpu->vk_allocation_callbacks );
  vk_destroy_swapchain_( gpu );

  CRUDE_ARRAY_DEINITIALIZE( gpu->queued_command_buffers );
  CRUDE_ARRAY_DEINITIALIZE( gpu->resource_deletion_queue );
  CRUDE_ARRAY_DEINITIALIZE( gpu->texture_to_update_bindless );
  
  crude_gfx_cmd_manager_deinitialize( &gpu->cmd_buffer_manager );

  for ( uint32 i = 0; i < CRUDE_GFX_MAX_SWAPCHAIN_IMAGES; ++i )
  {
    vkDestroySemaphore( gpu->vk_device, gpu->vk_image_avalivable_semaphores[ i ], gpu->vk_allocation_callbacks );
    vkDestroySemaphore( gpu->vk_device, gpu->vk_swapchain_updated_semaphore[ i ], gpu->vk_allocation_callbacks );
    vkDestroySemaphore( gpu->vk_device, gpu->vk_rendering_finished_semaphore[ i ], gpu->vk_allocation_callbacks );
    vkDestroyFence( gpu->vk_device, gpu->vk_command_buffer_executed_fences[ i ], gpu->vk_allocation_callbacks );
  }
  
  crude_resource_pool_deinitialize( &gpu->buffers );
  crude_resource_pool_deinitialize( &gpu->textures );
  crude_resource_pool_deinitialize( &gpu->render_passes );
  crude_resource_pool_deinitialize( &gpu->descriptor_set_layouts );
  crude_resource_pool_deinitialize( &gpu->pipelines );
  crude_resource_pool_deinitialize( &gpu->shaders );
  crude_resource_pool_deinitialize( &gpu->samplers );
  crude_resource_pool_deinitialize( &gpu->framebuffers );
  
  vmaDestroyAllocator( gpu->vma_allocator );
  vkDestroyDevice( gpu->vk_device, gpu->vk_allocation_callbacks );
  vkDestroySurfaceKHR( gpu->vk_instance, gpu->vk_surface, gpu->vk_allocation_callbacks );
  gpu->vkDestroyDebugUtilsMessengerEXT( gpu->vk_instance, gpu->vk_debug_utils_messenger, gpu->vk_allocation_callbacks );
  vkDestroyInstance( gpu->vk_instance, gpu->vk_allocation_callbacks );

  crude_string_buffer_deinitialize( &gpu->objects_names_string_buffer );
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
  {
    VkFence *swapchain_updated_fence = &gpu->vk_command_buffer_executed_fences[ gpu->current_frame ];
    if ( vkGetFenceStatus( gpu->vk_device, *swapchain_updated_fence ) != VK_SUCCESS )
    {
      vkWaitForFences( gpu->vk_device, 1, swapchain_updated_fence, VK_TRUE, UINT64_MAX );
    }
    vkResetFences( gpu->vk_device, 1, swapchain_updated_fence );
  }

  {
    VkResult result = vkAcquireNextImageKHR( gpu->vk_device, gpu->vk_swapchain, UINT64_MAX, gpu->vk_image_avalivable_semaphores[ gpu->current_frame ], VK_NULL_HANDLE, &gpu->vk_swapchain_image_index );
    if ( result == VK_ERROR_OUT_OF_DATE_KHR  )
    {
      CRUDE_ASSERT( false );
      //vk_resize_swapchain_( gpu );
    }
  }

  crude_gfx_cmd_manager_reset( &gpu->cmd_buffer_manager, gpu->current_frame );

  {
    uint32 used_size = gpu->dynamic_allocated_size - ( gpu->dynamic_per_frame_size * gpu->previous_frame );
    gpu->dynamic_max_per_frame_size = crude_max( used_size, gpu->dynamic_max_per_frame_size );
    gpu->dynamic_allocated_size = gpu->dynamic_per_frame_size * gpu->current_frame;
  }
}

void
crude_gfx_present
(
  _In_ crude_gfx_device                                   *gpu,
  _In_ crude_gfx_texture                                  *texture
)
{
  VkCommandBuffer                                          enqueued_command_buffers[ 4 ];

  {
    for ( uint32 i = 0; i < CRUDE_ARRAY_LENGTH( gpu->queued_command_buffers ); ++i )
    {
      crude_gfx_cmd_buffer* command_buffer = gpu->queued_command_buffers[i];
      enqueued_command_buffers[ i ] = command_buffer->vk_cmd_buffer;

      crude_gfx_cmd_end_render_pass( command_buffer );
      vkEndCommandBuffer( command_buffer->vk_cmd_buffer );
      command_buffer->is_recording = false;
      command_buffer->current_render_pass = NULL;
      command_buffer->current_framebuffer = NULL;
    }
  }

  {
    VkWriteDescriptorSet                                   bindless_descriptor_writes[ CRUDE_GFX_MAX_BINDLESS_RESOURCES ];
    VkDescriptorImageInfo                                  bindless_image_info[ CRUDE_GFX_MAX_BINDLESS_RESOURCES ];
    uint32                                                 current_write_index;

    current_write_index = 0;
    for ( int32 i = CRUDE_ARRAY_LENGTH( gpu->texture_to_update_bindless ) - 1; i >= 0; --i )
    {
      VkDescriptorImageInfo                               *descriptor_image_info;
      VkWriteDescriptorSet                                *descriptor_write;
      crude_gfx_resource_update                           *texture_to_update;
      crude_gfx_texture                                   *texture;

      texture_to_update = &gpu->texture_to_update_bindless[ i ];
      texture = crude_gfx_access_texture( gpu, ( crude_gfx_texture_handle ) { texture_to_update->handle } );
      descriptor_write = &bindless_descriptor_writes[ current_write_index ];
      memset( descriptor_write, 0, sizeof( VkWriteDescriptorSet ) );
      descriptor_write->sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
      descriptor_write->descriptorCount = 1;
      descriptor_write->dstArrayElement = texture_to_update->handle;
      descriptor_write->descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
      descriptor_write->dstSet = gpu->vk_bindless_descriptor_set;
      descriptor_write->dstBinding = CRUDE_GFX_BINDLESS_TEXTURE_BINDING;

      descriptor_image_info = &bindless_image_info[ current_write_index ];    
      if ( texture->sampler )
      {
        descriptor_image_info->sampler = texture->sampler->vk_sampler;
      }
      else
      {
        crude_gfx_sampler *default_sampler = crude_gfx_access_sampler( gpu, gpu->default_sampler );
        descriptor_image_info->sampler = default_sampler->vk_sampler;
      }
      
      CRUDE_ASSERT( texture->vk_format != VK_FORMAT_UNDEFINED );
      descriptor_image_info->imageView = texture->vk_image_view;
      descriptor_image_info->imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
      descriptor_write->pImageInfo = descriptor_image_info;
      
      CRUDE_ARRAY_DELSWAP( gpu->texture_to_update_bindless, i );

      ++current_write_index;
    }

    if ( current_write_index )
    {
      vkUpdateDescriptorSets( gpu->vk_device, current_write_index, bindless_descriptor_writes, 0, NULL );
    }
  }
  
  {
    VkSemaphore wait_semaphores[] = { gpu->vk_image_avalivable_semaphores[ gpu->current_frame ]};
    VkSemaphore signal_semaphores[] = { gpu->vk_rendering_finished_semaphore[ gpu->current_frame ]};
    VkPipelineStageFlags wait_stages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
    VkSubmitInfo submit_info = { 
      .sType                = VK_STRUCTURE_TYPE_SUBMIT_INFO,
      .waitSemaphoreCount   = 1,
      .pWaitSemaphores      = wait_semaphores,
      .pWaitDstStageMask    = wait_stages,
      .commandBufferCount   = CRUDE_ARRAY_LENGTH( gpu->queued_command_buffers ),
      .pCommandBuffers      = enqueued_command_buffers,
      .pSignalSemaphores    = signal_semaphores,
      .signalSemaphoreCount = 1,
    };
    CRUDE_GFX_HANDLE_VULKAN_RESULT( vkQueueSubmit( gpu->vk_main_queue, 1, &submit_info, VK_NULL_HANDLE ), "Failed to sumbit queue" );
  }
  
  {
    VkImageCopy region = {
      .srcSubresource = { 
        .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
        .mipLevel = 0,
        .baseArrayLayer = 0,
        .layerCount = 1,
      },
      .srcOffset = { 0 },
      .dstSubresource = { 
        .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
        .mipLevel = 0,
        .baseArrayLayer = 0,
        .layerCount = 1,
      },
      .dstOffset = { 0 },
      .extent = { gpu->vk_swapchain_width, gpu->vk_swapchain_height, 1 },
    };
    
    crude_gfx_cmd_buffer *cmd = crude_gfx_get_primary_cmd( gpu, 0, true );
    crude_gfx_cmd_add_image_barrier( cmd, texture->vk_image, CRUDE_GFX_RESOURCE_STATE_RENDER_TARGET, CRUDE_GFX_RESOURCE_STATE_COPY_SOURCE, 0, 1, false );
    crude_gfx_cmd_add_image_barrier( cmd, gpu->vk_swapchain_images[ gpu->vk_swapchain_image_index ], CRUDE_GFX_RESOURCE_STATE_PRESENT, CRUDE_GFX_RESOURCE_STATE_COPY_DEST, 0, 1, false );
    vkCmdCopyImage( cmd->vk_cmd_buffer, texture->vk_image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, gpu->vk_swapchain_images[ gpu->vk_swapchain_image_index ], VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1u, &region );
    crude_gfx_cmd_add_image_barrier( cmd, gpu->vk_swapchain_images[ gpu->vk_swapchain_image_index ], CRUDE_GFX_RESOURCE_STATE_COPY_DEST, CRUDE_GFX_RESOURCE_STATE_PRESENT, 0, 1, false );
    crude_gfx_cmd_end( cmd );
  
    VkSemaphore wait_semaphores[] = { gpu->vk_rendering_finished_semaphore[ gpu->current_frame ]};
    VkSemaphore signal_semaphores[] = { gpu->vk_swapchain_updated_semaphore[ gpu->current_frame ]};
    VkPipelineStageFlags wait_stages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
    VkSubmitInfo submit_info = { 
      .sType                = VK_STRUCTURE_TYPE_SUBMIT_INFO,
      .waitSemaphoreCount   = 1u,
      .pWaitSemaphores      = wait_semaphores,
      .pWaitDstStageMask    = wait_stages,
      .commandBufferCount   = 1u,
      .pCommandBuffers      = &cmd->vk_cmd_buffer,
      .signalSemaphoreCount = 1,
      .pSignalSemaphores    = signal_semaphores,
    };
    VkFence *swapchain_updated_fence = &gpu->vk_command_buffer_executed_fences[ gpu->current_frame ];
    CRUDE_GFX_HANDLE_VULKAN_RESULT( vkQueueSubmit( gpu->vk_main_queue, 1, &submit_info, *swapchain_updated_fence ), "Failed to sumbit queue" );
  }
  
  {
    VkSemaphore wait_semaphores[] = { gpu->vk_swapchain_updated_semaphore[ gpu->current_frame ]};
    VkSwapchainKHR swap_chains[] = { gpu->vk_swapchain };
    VkPresentInfoKHR present_info = {
      .sType              = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
      .waitSemaphoreCount = 1,
      .pWaitSemaphores    = wait_semaphores,
      .swapchainCount     = ARRAY_SIZE( swap_chains ),
      .pSwapchains        = swap_chains,
      .pImageIndices      = &gpu->vk_swapchain_image_index,
    };
  
    VkResult result = vkQueuePresentKHR( gpu->vk_main_queue, &present_info );
    CRUDE_PROFILER_MARK_FRAME;

    CRUDE_ARRAY_SET_LENGTH( gpu->queued_command_buffers, 0u );
    
    gpu->swapchain_resized_last_frame = false;
    if ( result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR )
    {
      vk_resize_swapchain_( gpu );
      return;
    }
  }

  gpu->previous_frame = gpu->current_frame;
  gpu->current_frame = ( gpu->current_frame + 1u ) % gpu->vk_swapchain_images_count;
  
  {
    for ( uint32 i = 0; i < CRUDE_ARRAY_LENGTH( gpu->resource_deletion_queue ); ++i )
    {
      crude_gfx_resource_update* resource_deletion = &gpu->resource_deletion_queue[ i ];
      
      if ( resource_deletion->current_frame != gpu->current_frame )
      {
        continue;
      }
      
      vk_destroy_resources_instant_( gpu, resource_deletion->type, resource_deletion->handle );

      resource_deletion->current_frame = UINT32_MAX;
      CRUDE_ARRAY_DELSWAP( gpu->resource_deletion_queue, i );
      --i;
    }
  }
}

crude_gfx_cmd_buffer*
crude_gfx_get_primary_cmd
(
  _In_ crude_gfx_device                                   *gpu,
  _In_ uint32                                              thread_index,
  _In_ bool                                                begin
)
{
  crude_gfx_cmd_buffer *cmd = crude_gfx_cmd_manager_get_primary_cmd( &gpu->cmd_buffer_manager, gpu->current_frame, thread_index, begin );

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
  crude_gfx_cmd_buffer *cmd = crude_gfx_cmd_manager_get_secondary_cmd( &gpu->cmd_buffer_manager, gpu->current_frame, thread_index );
  return cmd;
}

void
crude_gfx_queue_cmd
(
  _In_ crude_gfx_cmd_buffer                               *cmd
)
{
  CRUDE_ARRAY_PUSH( cmd->gpu->queued_command_buffers, cmd );
}

void*
crude_gfx_map_buffer
(
  _In_ crude_gfx_device                                   *gpu,
  _In_ crude_gfx_map_buffer_parameters const              *parameters
)
{
  if ( CRUDE_RESOURCE_HANDLE_IS_INVALID( parameters->buffer ) )
  {
    return NULL;
  }

  crude_gfx_buffer *buffer = crude_gfx_access_buffer( gpu, parameters->buffer );
  
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
  if ( CRUDE_RESOURCE_HANDLE_IS_INVALID( handle ) )
  {
    return;
  }

  crude_gfx_buffer *buffer = crude_gfx_access_buffer( gpu, handle );
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
  crude_gfx_texture* texture_vk = crude_gfx_access_texture( gpu, texture );
  crude_gfx_sampler* sampler_vk = crude_gfx_access_sampler( gpu, sampler );
  
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
  crude_gfx_pipeline *pipeline = crude_gfx_access_pipeline( gpu, pipeline_handle );
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
  if ( CRUDE_RESOURCE_HANDLE_IS_INVALID( buffer ) )
  {
    return;
  }

  const crude_gfx_buffer* buffer_data = crude_gfx_access_buffer( gpu, buffer );
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
  gpu->vkSetDebugUtilsObjectNameEXT( gpu->vk_device, &name_info );
}

bool
crude_gfx_buffer_ready
(
  _In_ crude_gfx_device                                   *gpu,
  _In_ crude_gfx_buffer_handle                             buffer_handle
)
{
  crude_gfx_buffer *buffer = crude_gfx_access_buffer( gpu, buffer_handle );
  return buffer->ready;
}

VkShaderModuleCreateInfo
crude_gfx_compile_shader
(
  _In_ char const                                         *code,
  _In_ uint32                                              code_size,
  _In_ VkShaderStageFlagBits                               stage,
  _In_ char const                                         *name,
  _In_ crude_stack_allocator                              *temporary_allocator
)
{
  VkShaderModuleCreateInfo                                 shader_create_info;
  crude_string_buffer                                      temporary_string_buffer;
  char const                                              *temp_filename;
  char                                                    *stage_define, *vulkan_binaries_path, *glsl_compiler_path, *final_spirv_filename, *arguments;

  shader_create_info = ( VkShaderModuleCreateInfo ){ VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO };
  
  temp_filename = "temp.shader";
  crude_write_file( temp_filename, code, code_size );
  
  crude_string_buffer_initialize( &temporary_string_buffer, CRUDE_RKILO( 1 ), crude_stack_allocator_pack( temporary_allocator ) );
  
  stage_define = crude_string_buffer_append_use_f( &temporary_string_buffer, "%s_%s", crude_vk_shader_stage_to_defines( stage ), name );
  {
    sizet stage_define_length = strlen( stage_define );
    for ( size_t i = 0; i < stage_define_length; ++i )
    {
      stage_define[ i ] = toupper( stage_define[ i ] );
    }
  }

  {
    char vulkan_env[ 512 ];
    crude_process_expand_environment_strings( "%VULKAN_SDK%", vulkan_env, 512 );
    vulkan_binaries_path = crude_string_buffer_append_use_f( &temporary_string_buffer, "%s\\Bin\\", vulkan_env );
  }

#if defined(_MSC_VER)
  glsl_compiler_path = crude_string_buffer_append_use_f( &temporary_string_buffer, "%sglslangValidator.exe", vulkan_binaries_path );
  final_spirv_filename = crude_string_buffer_append_use_f( &temporary_string_buffer, "shader_final.spv" );
  arguments = crude_string_buffer_append_use_f( &temporary_string_buffer, "glslangValidator.exe %s -V --target-env vulkan1.2 -o %s -S %s --D %s --D %s", temp_filename, final_spirv_filename, crude_vk_shader_stage_to_compiler_extension( stage ), stage_define, crude_vk_shader_stage_to_defines( stage ) );
#endif
  crude_process_execute( ".", glsl_compiler_path, arguments, "" );
  
  bool optimize_shaders = false;
  if ( optimize_shaders )
  {
    char* spirv_optimizer_path = crude_string_buffer_append_use_f( &temporary_string_buffer,"%sspirv-opt.exe", vulkan_binaries_path );
    char* optimized_spirv_filename = crude_string_buffer_append_use_f( &temporary_string_buffer,"shader_opt.spv" );
    char* spirv_opt_arguments = crude_string_buffer_append_use_f( &temporary_string_buffer,"spirv-opt.exe -O --preserve-bindings %s -o %s", final_spirv_filename, optimized_spirv_filename );
    
    crude_process_execute( ".", spirv_optimizer_path, spirv_opt_arguments, "" );
    crude_read_file_binary( final_spirv_filename, crude_stack_allocator_pack( temporary_allocator ), &shader_create_info.pCode, &shader_create_info.codeSize );
    crude_file_delete( optimized_spirv_filename );
  }
  else
  {
    crude_read_file_binary( final_spirv_filename, crude_stack_allocator_pack( temporary_allocator ), &shader_create_info.pCode, &shader_create_info.codeSize );
  }
  
  if ( !shader_create_info.pCode )
  {
    dump_shader_code_( code, stage, name, &temporary_string_buffer );
  }
  
  crude_file_delete( temp_filename );
  crude_file_delete( final_spirv_filename );

  return shader_create_info;
}

VkShaderModuleCreateInfo
crude_gfx_resize_framebuffer
(
  _In_ crude_gfx_device                                   *gpu,
  _In_ crude_gfx_framebuffer_handle                        framebuffer_handle,
  _In_ uint32                                              width,
  _In_ uint32                                              height
)
{
  crude_gfx_framebuffer *framebuffer = crude_gfx_access_framebuffer( gpu, framebuffer_handle );
  if ( !framebuffer )
  {
    return;
  }

  if ( !framebuffer->resize )
  {
    return;
  }
  
  uint16 new_width = width * framebuffer->scale_x;
  uint16 new_height = height * framebuffer->scale_y;
  
  for ( size_t i = 0; i < framebuffer->num_color_attachments; ++i )
  {
    crude_gfx_resize_texture( gpu, framebuffer->color_attachments[ i ], new_width, new_height );
  }
  
  if ( CRUDE_RESOURCE_HANDLE_IS_VALID( framebuffer->depth_stencil_attachment ) )
  {
    crude_gfx_resize_texture( gpu, framebuffer->depth_stencil_attachment, new_width, new_height );
  }
  
  framebuffer->width = new_width;
  framebuffer->height = new_height;
}

void
crude_gfx_resize_texture
(
  _In_ crude_gfx_device                                   *gpu,
  _In_ crude_gfx_texture_handle                            texture_handle,
  _In_ uint32                                              width,
  _In_ uint32                                              height
)
{
  crude_gfx_texture *texture = crude_gfx_access_texture( gpu, texture_handle );
  
  if ( texture->width == width && texture->height == height )
  {
    return;
  }
  
  crude_gfx_texture_handle texture_to_delete_handle = crude_gfx_obtain_texture( gpu );
  crude_gfx_texture *texture_to_delete = crude_gfx_access_texture( gpu, texture_to_delete_handle );
  
  memcpy( texture_to_delete, texture, sizeof( crude_gfx_texture ) );
  texture_to_delete->handle = texture_to_delete_handle;
  
  crude_gfx_texture_creation texture_creation = crude_gfx_texture_creation_empty();
  texture_creation.flags = texture->flags;
  texture_creation.mipmaps = texture->mipmaps;
  texture_creation.format = texture->vk_format;
  texture_creation.type = texture->type;
  texture_creation.name = texture->name;
  texture_creation.width = width;
  texture_creation.height = height;
  texture_creation.depth = 1;

  vk_create_texture_( gpu, &texture_creation, texture->handle, texture );
  
  crude_gfx_destroy_texture( gpu, texture_to_delete_handle );
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
  crude_gfx_sampler_handle handle = crude_gfx_obtain_sampler( gpu );
  if ( CRUDE_RESOURCE_HANDLE_IS_INVALID( handle ) )
  {
    return handle;
  }
  
  crude_gfx_sampler *sampler = crude_gfx_access_sampler( gpu, handle );
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
  crude_gfx_set_resource_name( gpu, VK_OBJECT_TYPE_SAMPLER, sampler->vk_sampler, creation->name );
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
  CRUDE_ARRAY_PUSH( gpu->resource_deletion_queue, sampler_update_event );
}

void
crude_gfx_destroy_sampler_instant
(
  _In_ crude_gfx_device                                   *gpu,
  _In_ crude_gfx_sampler_handle                            handle
)
{
  crude_gfx_sampler *sampler = crude_gfx_access_sampler( gpu, handle );
  if ( sampler )
  {
    vkDestroySampler( gpu->vk_device, sampler->vk_sampler, gpu->vk_allocation_callbacks );
  }
  crude_gfx_release_sampler( gpu, handle );
}

crude_gfx_texture_handle
crude_gfx_create_texture
(
  _In_ crude_gfx_device                                   *gpu,
  _In_ crude_gfx_texture_creation const                   *creation
)
{
  crude_gfx_texture_handle handle = crude_gfx_obtain_texture( gpu );
  if ( CRUDE_RESOURCE_HANDLE_IS_INVALID( handle ) )
  {
    return handle;
  }
  
  crude_gfx_texture *texture = crude_gfx_access_texture( gpu, handle );
  vk_create_texture_( gpu, creation, handle, texture );

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
    
    crude_gfx_cmd_buffer *cmd = crude_gfx_cmd_manager_get_primary_cmd( &gpu->cmd_buffer_manager, gpu->current_frame, 0u, true );
    
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
  CRUDE_ARRAY_PUSH( gpu->resource_deletion_queue, texture_update_event );

  crude_gfx_resource_update texture_update_bindless_event = { 
    .type          = CRUDE_GFX_RESOURCE_DELETION_TYPE_TEXTURE,
    .handle        = handle.index,
    .current_frame = gpu->current_frame };
  CRUDE_ARRAY_PUSH( gpu->texture_to_update_bindless, texture_update_event );
}

void
crude_gfx_destroy_texture_instant
(
  _In_ crude_gfx_device                                   *gpu,
  _In_ crude_gfx_texture_handle                            handle
)
{
  crude_gfx_texture *texture = crude_gfx_access_texture( gpu, handle );
  
  if ( texture )
  {
    vkDestroyImageView( gpu->vk_device, texture->vk_image_view, gpu->vk_allocation_callbacks );
    vmaDestroyImage( gpu->vma_allocator, texture->vk_image, texture->vma_allocation );
  }
  crude_gfx_release_texture( gpu, handle );
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
    return CRUDE_GFX_SHADER_STATE_HANDLE_INVALID;
  }
  
  crude_gfx_shader_state_handle handle = crude_gfx_obtain_shader_state( gpu );
  if ( CRUDE_RESOURCE_HANDLE_IS_INVALID( handle ) )
  {
    return handle;
  }

  uint32 compiled_shaders = 0u;

  crude_gfx_shader_state *shader_state = crude_gfx_access_shader_state( gpu, handle );
  shader_state->graphics_pipeline = true;
  shader_state->active_shaders = 0;

  for ( compiled_shaders = 0; compiled_shaders < creation->stages_count; ++compiled_shaders )
  {
    size_t temporary_allocator_marker = crude_stack_allocator_get_marker( gpu->temporary_allocator );
    crude_gfx_shader_stage const *stage = &creation->stages[ compiled_shaders ];
  
    if ( stage->type == VK_SHADER_STAGE_COMPUTE_BIT )
    {
      shader_state->graphics_pipeline = false;
    }
  
    VkShaderModuleCreateInfo shader_create_info = { .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO };
    if ( creation->spv_input )
    {
      shader_create_info.codeSize = stage->code_size;
      shader_create_info.pCode = ( uint32 const * )stage->code;
    }
    else
    {
      shader_create_info = crude_gfx_compile_shader( stage->code, stage->code_size, stage->type, creation->name, gpu->temporary_allocator );
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
      vk_reflect_shader_( gpu, shader_create_info.pCode, shader_create_info.codeSize, &shader_state->reflect );
    }
  
    crude_stack_allocator_free_marker( gpu->temporary_allocator, temporary_allocator_marker );
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
    return CRUDE_GFX_SHADER_STATE_HANDLE_INVALID;
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
  CRUDE_ARRAY_PUSH( gpu->resource_deletion_queue, shader_state_update_event );
}

void
crude_gfx_destroy_shader_state_instant
(
  _In_ crude_gfx_device                                   *gpu,
  _In_ crude_gfx_shader_state_handle                       handle
)
{
  crude_gfx_shader_state *shader_state = crude_gfx_access_shader_state( gpu, handle );
  if ( shader_state )
  {
    CRUDE_ARRAY_DEINITIALIZE( shader_state->reflect.input.vertex_attributes );
    CRUDE_ARRAY_DEINITIALIZE( shader_state->reflect.input.vertex_streams );
    for ( uint32 i = 0; i < shader_state->active_shaders; ++i )
    {
      vkDestroyShaderModule( gpu->vk_device, shader_state->shader_stage_info[ i ].module, gpu->vk_allocation_callbacks );
    }
  }
  crude_gfx_release_shader_state( gpu, handle );
}

crude_gfx_render_pass_handle
crude_gfx_create_render_pass
(
  _In_ crude_gfx_device                                   *gpu,
  _In_ crude_gfx_render_pass_creation const               *creation
)
{
  crude_gfx_render_pass_handle handle = crude_gfx_obtain_render_pass( gpu);
  if ( CRUDE_RESOURCE_HANDLE_IS_INVALID( handle ) )
  {
    return handle;
  }
  
  crude_gfx_render_pass *render_pass = crude_gfx_access_render_pass( gpu, handle );
  render_pass->num_render_targets = creation->num_render_targets;
  render_pass->name               = creation->name;
  
  for ( uint32 i = 0 ; i < creation->num_render_targets; ++i )
  {
    render_pass->output.color_final_layouts[ i ] = creation->color_final_layouts[ i ];
    render_pass->output.color_formats[ i ] = creation->color_formats[ i ];
    render_pass->output.color_operations[ i ] = creation->color_operations[ i ];
  }
  if ( creation->depth_stencil_format != VK_FORMAT_UNDEFINED )
  {
    render_pass->output.depth_stencil_final_layout = creation->depth_stencil_final_layout;
    render_pass->output.depth_stencil_format = creation->depth_stencil_format;
  }

  render_pass->output.depth_operation = creation->depth_operation;
  render_pass->output.stencil_operation = creation->stencil_operation;
  
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
  CRUDE_ARRAY_PUSH( gpu->resource_deletion_queue, render_pass_update_event );
}

void
crude_gfx_destroy_render_pass_instant
(
  _In_ crude_gfx_device                                   *gpu,
  _In_ crude_gfx_render_pass_handle                        handle
)
{
  crude_gfx_render_pass *render_pass = crude_gfx_access_render_pass( gpu, handle );
  crude_gfx_release_render_pass( gpu, handle );
}

crude_gfx_pipeline_handle
crude_gfx_create_pipeline
(
  _In_ crude_gfx_device                                   *gpu,
  _In_ crude_gfx_pipeline_creation const                  *creation
)
{
  crude_gfx_pipeline_handle handle = crude_gfx_obtain_pipeline( gpu );
  if ( CRUDE_RESOURCE_HANDLE_IS_INVALID( handle ) )
  {
    return handle;
  }

  crude_gfx_shader_state_handle shader_state = crude_gfx_create_shader_state( gpu, &creation->shaders );
  if ( CRUDE_RESOURCE_HANDLE_IS_INVALID( shader_state ) )
  {
    crude_gfx_release_pipeline( gpu, handle );
    return CRUDE_GFX_PIPELINE_HANDLE_INVALID;
  }

  crude_gfx_pipeline *pipeline = crude_gfx_access_pipeline( gpu, handle );
  crude_gfx_shader_state *shader_state_data = crude_gfx_access_shader_state( gpu, shader_state );
  
  pipeline->shader_state = shader_state;
  
  // Create VkPipelineLayout
  VkDescriptorSetLayout vk_layouts[ CRUDE_GFX_MAX_DESCRIPTOR_SET_LAYOUTS ];
  for ( uint32 i = 0; i < shader_state_data->reflect.descriptor.sets_count; ++i )
  {
    pipeline->descriptor_set_layout_handle[ i ] = crude_gfx_create_descriptor_set_layout( gpu, &shader_state_data->reflect.descriptor.sets[ i ] );
    crude_gfx_descriptor_set_layout *descriptor_set_layout = crude_gfx_access_descriptor_set_layout( gpu, pipeline->descriptor_set_layout_handle[ i ] );
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
  crude_gfx_vertex_attribute *vertex_attributes = shader_state_data->reflect.input.vertex_attributes;
  CRUDE_ASSERT( ARRAY_SIZE( vk_vertex_attributes ) >= CRUDE_ARRAY_LENGTH( vertex_attributes ) );

  if ( CRUDE_ARRAY_LENGTH( vertex_attributes ) )
  {
    for ( uint32 i = 0; i < CRUDE_ARRAY_LENGTH( vertex_attributes ); ++i )
    {
      vk_vertex_attributes[ i ] = ( VkVertexInputAttributeDescription ){
        .location = vertex_attributes[ i ].location,
        .binding = vertex_attributes[ i ].binding,
        .format = crude_gfx_to_vk_vertex_format( vertex_attributes[ i ].format ),
        .offset = vertex_attributes[ i ].offset
      };
    }
    vertex_input_info.vertexAttributeDescriptionCount = CRUDE_ARRAY_LENGTH( vertex_attributes );
    vertex_input_info.pVertexAttributeDescriptions = vk_vertex_attributes;
  }
  else
  {
    vertex_input_info.vertexAttributeDescriptionCount = 0;
    vertex_input_info.pVertexAttributeDescriptions = NULL;
  }

  VkVertexInputBindingDescription vk_vertex_bindings[ 8 ];
  crude_gfx_vertex_stream *vertex_streams = shader_state_data->reflect.input.vertex_streams;
  CRUDE_ASSERT( ARRAY_SIZE( vk_vertex_bindings ) >= CRUDE_ARRAY_LENGTH( vertex_streams ) );
  if ( CRUDE_ARRAY_LENGTH( vertex_streams ) )
  {
    for ( uint32 i = 0; i < CRUDE_ARRAY_LENGTH( vertex_streams ); ++i )
    {
      VkVertexInputRate vertex_rate = vertex_streams[ i ].input_rate == CRUDE_GFX_VERTEX_INPUT_RATE_PER_VERTEX ? VK_VERTEX_INPUT_RATE_VERTEX : VK_VERTEX_INPUT_RATE_INSTANCE;
      vk_vertex_bindings[ i ] = ( VkVertexInputBindingDescription ){
        .binding = vertex_streams[ i ].binding,
        .stride = vertex_streams[ i ].stride,
        .inputRate = vertex_rate
      };
    }
    vertex_input_info.vertexBindingDescriptionCount = CRUDE_ARRAY_LENGTH( vertex_streams );
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
    .dynamicStateCount = ARRAY_SIZE( dynamic_states ),
    .pDynamicStates = dynamic_states,
  };
  
  VkPipelineRenderingCreateInfoKHR pipeline_rendering_create_info = {
    .sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO_KHR,
    .viewMask = 0,
    .colorAttachmentCount = creation->render_pass_output.num_color_formats,
    .pColorAttachmentFormats = creation->render_pass_output.num_color_formats > 0 ? creation->render_pass_output.color_formats : NULL,
    .depthAttachmentFormat = creation->render_pass_output.depth_stencil_format,
    .stencilAttachmentFormat = VK_FORMAT_UNDEFINED,
  };

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
    .renderPass = NULL,
    .pDynamicState = &dynamic_state,
    .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
    .pStages = shader_state_data->shader_stage_info,
    .stageCount = shader_state_data->active_shaders,
    .layout = pipeline_layout,
    .pNext = &pipeline_rendering_create_info
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
  CRUDE_ARRAY_PUSH( gpu->resource_deletion_queue, pipeline_update_event );

  crude_gfx_pipeline *pipeline = crude_gfx_access_pipeline( gpu, handle );
  crude_gfx_destroy_shader_state( gpu, pipeline->shader_state );
}

void
crude_gfx_destroy_pipeline_instant
(
  _In_ crude_gfx_device                                   *gpu,
  _In_ crude_gfx_pipeline_handle                           handle
)
{
  crude_gfx_pipeline *pipeline = crude_gfx_access_pipeline( gpu, handle );

  if ( pipeline )
  {
    for ( uint32 i = 0; i < pipeline->num_active_layouts; ++i )
    {
      crude_gfx_destroy_descriptor_set_layout( gpu, pipeline->descriptor_set_layout_handle[ i ] );
    }
    vkDestroyPipeline( gpu->vk_device, pipeline->vk_pipeline, gpu->vk_allocation_callbacks );
    vkDestroyPipelineLayout( gpu->vk_device, pipeline->vk_pipeline_layout, gpu->vk_allocation_callbacks );
  }

  crude_gfx_release_pipeline( gpu, handle );
}

crude_gfx_buffer_handle
crude_gfx_create_buffer
(
  _In_ crude_gfx_device                                   *gpu,
  _In_ crude_gfx_buffer_creation const                    *creation
)
{
  crude_gfx_buffer_handle handle = crude_gfx_obtain_buffer( gpu );
  if ( CRUDE_RESOURCE_HANDLE_IS_INVALID( handle ) )
  {
      return handle;
  }

  crude_gfx_buffer *buffer = crude_gfx_access_buffer( gpu, handle );
  buffer->name = creation->name;
  buffer->size = creation->size;
  buffer->type_flags = creation->type_flags;
  buffer->usage = creation->usage;
  buffer->handle = handle;
  buffer->global_offset = 0;
  buffer->parent_buffer = CRUDE_GFX_BUFFER_HANDLE_INVALID;
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
  CRUDE_ARRAY_PUSH( gpu->resource_deletion_queue, buffer_update_event );
}

void
crude_gfx_destroy_buffer_instant
(
  _In_ crude_gfx_device                                   *gpu,
  _In_ crude_gfx_buffer_handle                             handle
)
{
  crude_gfx_buffer *buffer = crude_gfx_access_buffer( gpu, handle );

  if ( buffer && CRUDE_RESOURCE_HANDLE_IS_INVALID( buffer->parent_buffer ) )
  {
    vmaDestroyBuffer( gpu->vma_allocator, buffer->vk_buffer, buffer->vma_allocation );
  }

  crude_gfx_release_buffer( gpu, handle );
}

crude_gfx_descriptor_set_layout_handle
crude_gfx_create_descriptor_set_layout
(
  _In_ crude_gfx_device                                   *gpu,
  _In_ crude_gfx_descriptor_set_layout_creation const     *creation
)
{
  crude_gfx_descriptor_set_layout_handle handle = crude_gfx_obtain_descriptor_set_layout( gpu );
  if ( CRUDE_RESOURCE_HANDLE_IS_INVALID( handle ) )
  {
    return handle;
  }
  
  crude_gfx_descriptor_set_layout *descriptor_set_layout = crude_gfx_access_descriptor_set_layout( gpu, handle );
  
  uint8 *memory = CRUDE_ALLOCATE( gpu->allocator_container, ( sizeof( VkDescriptorSetLayoutBinding ) + sizeof( crude_gfx_descriptor_binding ) ) * creation->num_bindings );
  descriptor_set_layout->num_bindings = creation->num_bindings;
  descriptor_set_layout->bindings     = ( crude_gfx_descriptor_binding* )memory;
  descriptor_set_layout->vk_binding   = ( VkDescriptorSetLayoutBinding* )( memory + sizeof( crude_gfx_descriptor_binding ) * creation->num_bindings );
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
  CRUDE_ARRAY_PUSH( gpu->resource_deletion_queue, descriptor_set_layout_update_event );
}

void
crude_gfx_destroy_descriptor_set_layout_instant
(                                                   
  _In_ crude_gfx_device                                   *gpu,
  _In_ crude_gfx_descriptor_set_layout_handle              handle
)
{
  crude_gfx_descriptor_set_layout *descriptor_set_layout = crude_gfx_access_descriptor_set_layout( gpu, handle );
  CRUDE_DEALLOCATE( gpu->allocator_container, descriptor_set_layout->bindings );
  vkDestroyDescriptorSetLayout( gpu->vk_device, descriptor_set_layout->vk_descriptor_set_layout, gpu->vk_allocation_callbacks );
  crude_gfx_release_descriptor_set_layout( gpu, handle );
}

crude_gfx_framebuffer_handle
crude_gfx_create_framebuffer
(
  _In_ crude_gfx_device                                   *gpu,
  _In_ crude_gfx_framebuffer_creation const               *creation
)
{
  crude_gfx_framebuffer_handle handle = crude_gfx_obtain_framebuffer( gpu );
  if ( CRUDE_RESOURCE_HANDLE_IS_INVALID( handle ) )
  {
    return handle;
  }
  
  crude_gfx_framebuffer *framebuffer = crude_gfx_access_framebuffer( gpu, handle );
  framebuffer->num_color_attachments = creation->num_render_targets;
  for ( uint32 i = 0; i < creation->num_render_targets; ++i )
  {
    framebuffer->color_attachments[ i ] = creation->output_textures[ i ];
  }
  framebuffer->depth_stencil_attachment = creation->depth_stencil_texture;
  framebuffer->width = creation->width;
  framebuffer->height = creation->height;
  framebuffer->resize = creation->resize;
  framebuffer->name = creation->name;
  framebuffer->render_pass = creation->render_pass;
  framebuffer->scale_x = 1.0;
  framebuffer->scale_y = 1.0;
  
  return handle;
}

void                                      
crude_gfx_destroy_framebuffer
(                                                   
  _In_ crude_gfx_device                                   *gpu,
  _In_ crude_gfx_framebuffer_handle                        handle
)
{
  if ( handle.index >= gpu->framebuffers.pool_size )
  {
    CRUDE_LOG_ERROR( CRUDE_CHANNEL_GRAPHICS, "Trying to free invalid framebuffer %u", handle.index );
    return;
  }
  crude_gfx_resource_update framebuffer_update_event = { 
    .type          = CRUDE_GFX_RESOURCE_DELETION_TYPE_FRAMEBUFFER,
    .handle        = handle.index,
    .current_frame = gpu->current_frame };
  CRUDE_ARRAY_PUSH( gpu->resource_deletion_queue, framebuffer_update_event );
}

void
crude_gfx_destroy_framebuffer_instant
(                                                   
  _In_ crude_gfx_device                                   *gpu,
  _In_ crude_gfx_framebuffer_handle                        handle
)
{
  crude_gfx_framebuffer *framebuffer = crude_gfx_access_framebuffer( gpu, handle );
  if ( framebuffer )
  {
    for ( uint32 i = 0; i < framebuffer->num_color_attachments; ++i )
    {
      crude_gfx_destroy_texture_instant( gpu, framebuffer->color_attachments[ i ] );
    }
    
    if ( CRUDE_RESOURCE_HANDLE_IS_VALID( framebuffer->depth_stencil_attachment ) )
    {
      crude_gfx_destroy_texture_instant( gpu, framebuffer->depth_stencil_attachment );
    }
  }
  crude_gfx_release_framebuffer( gpu, handle );
}

/************************************************
 *
 * GPU Device Resource Macros
 * 
 ***********************************************/  
crude_gfx_sampler_handle
crude_gfx_obtain_sampler
(
  _In_ crude_gfx_device                                   *gpu
)
{
  return ( crude_gfx_sampler_handle ){ crude_resource_pool_obtain_resource( &gpu->samplers ) };                   
}

crude_gfx_sampler*
crude_gfx_access_sampler
(
  _In_ crude_gfx_device                                   *gpu,
  _In_ crude_gfx_sampler_handle                            handle
)
{
  return crude_resource_pool_access_resource( &gpu->samplers, handle.index );
}

void
crude_gfx_release_sampler
(
  _In_ crude_gfx_device                                   *gpu,
  _In_ crude_gfx_sampler_handle                            handle
)
{
  crude_resource_pool_release_resource( &gpu->samplers, handle.index );
}

crude_gfx_texture_handle
crude_gfx_obtain_texture
(
  _In_ crude_gfx_device                                   *gpu
)
{                                            
  return ( crude_gfx_texture_handle ) { crude_resource_pool_obtain_resource( &gpu->textures ) };
}

crude_gfx_texture*
crude_gfx_access_texture
(
  _In_ crude_gfx_device                                   *gpu,
  _In_ crude_gfx_texture_handle                            handle
)
{
  return crude_resource_pool_access_resource( &gpu->textures, handle.index );
}

void
crude_gfx_release_texture
(
  _In_ crude_gfx_device                                   *gpu,
  _In_ crude_gfx_texture_handle                            handle
)
{
  crude_resource_pool_release_resource( &gpu->textures, handle.index );
}
           
crude_gfx_render_pass_handle
crude_gfx_obtain_render_pass
(
  _In_ crude_gfx_device                                   *gpu
)
{
  return ( crude_gfx_render_pass_handle ) { crude_resource_pool_obtain_resource( &gpu->render_passes ) };
}

crude_gfx_render_pass*
crude_gfx_access_render_pass
(
  _In_ crude_gfx_device                                   *gpu,
  _In_ crude_gfx_render_pass_handle                        handle
)
{
  return crude_resource_pool_access_resource( &gpu->render_passes, handle.index );
}

void
crude_gfx_release_render_pass
(
  _In_ crude_gfx_device                                   *gpu,
  _In_ crude_gfx_render_pass_handle                        handle
)
{
  crude_resource_pool_release_resource( &gpu->render_passes, handle.index );
}

crude_gfx_shader_state_handle
crude_gfx_obtain_shader_state
(
  _In_ crude_gfx_device                                   *gpu
)
{
  return ( crude_gfx_shader_state_handle ){ crude_resource_pool_obtain_resource( &gpu->shaders ) };
}

crude_gfx_shader_state*
crude_gfx_access_shader_state
(
  _In_ crude_gfx_device                                   *gpu,
  _In_ crude_gfx_shader_state_handle                       handle
)
{
  return crude_resource_pool_access_resource( &gpu->shaders, handle.index );
}

void
crude_gfx_release_shader_state
(
  _In_ crude_gfx_device                                   *gpu,
  _In_ crude_gfx_shader_state_handle                       handle
)
{
  crude_resource_pool_release_resource( &gpu->shaders, handle.index );
}

crude_gfx_pipeline_handle
crude_gfx_obtain_pipeline
(
  _In_ crude_gfx_device                                   *gpu
)
{
  return ( crude_gfx_pipeline_handle ){ crude_resource_pool_obtain_resource( &gpu->pipelines ) };
}

crude_gfx_pipeline*
crude_gfx_access_pipeline
(
  _In_ crude_gfx_device                                   *gpu,
  _In_ crude_gfx_pipeline_handle                           handle
)
{
  return crude_resource_pool_access_resource( &gpu->pipelines, handle.index );
}

void
crude_gfx_release_pipeline
(
  _In_ crude_gfx_device                                   *gpu,
  _In_ crude_gfx_pipeline_handle                           handle
)
{
  crude_resource_pool_release_resource( &gpu->pipelines, handle.index );
}

crude_gfx_buffer_handle
crude_gfx_obtain_buffer
(
  _In_ crude_gfx_device                                   *gpu
)
{
  return ( crude_gfx_buffer_handle ){ crude_resource_pool_obtain_resource( &gpu->buffers ) };
}

crude_gfx_buffer*
crude_gfx_access_buffer
(
  _In_ crude_gfx_device                                   *gpu,
  _In_ crude_gfx_buffer_handle                             handle
)
{
  return crude_resource_pool_access_resource( &gpu->buffers, handle.index );
}

void
crude_gfx_release_buffer
(
  _In_ crude_gfx_device                                   *gpu,
  _In_ crude_gfx_buffer_handle                             handle
)
{
  crude_resource_pool_release_resource( &gpu->buffers, handle.index );
}

crude_gfx_descriptor_set_layout_handle
crude_gfx_obtain_descriptor_set_layout
(
  _In_ crude_gfx_device                                   *gpu
)
{
  return ( crude_gfx_descriptor_set_layout_handle ){ crude_resource_pool_obtain_resource( &gpu->descriptor_set_layouts ) };
}

crude_gfx_descriptor_set_layout*
crude_gfx_access_descriptor_set_layout
(
  _In_ crude_gfx_device                                   *gpu,
  _In_ crude_gfx_descriptor_set_layout_handle              handle
)
{
  return crude_resource_pool_access_resource( &gpu->descriptor_set_layouts, handle.index );
}

void
crude_gfx_release_descriptor_set_layout
(
  _In_ crude_gfx_device                                   *gpu,
  _In_ crude_gfx_descriptor_set_layout_handle              handle
)
{
  crude_resource_pool_release_resource( &gpu->descriptor_set_layouts, handle.index );
}

crude_gfx_framebuffer_handle
crude_gfx_obtain_framebuffer
(
  _In_ crude_gfx_device                                   *gpu
)
{
  return ( crude_gfx_framebuffer_handle ){ crude_resource_pool_obtain_resource( &gpu->framebuffers ) };
}

crude_gfx_framebuffer*
crude_gfx_access_framebuffer
(
  _In_ crude_gfx_device                                   *gpu,
  _In_ crude_gfx_framebuffer_handle                        handle
)
{
  return crude_resource_pool_access_resource( &gpu->framebuffers, handle.index );
}

void
crude_gfx_release_framebuffer
(
  _In_ crude_gfx_device                                   *gpu,
  _In_ crude_gfx_framebuffer_handle                        handle
)
{
  crude_resource_pool_release_resource( &gpu->framebuffers, handle.index );
}

/************************************************
 *
 * Local Vulkan Helper Functions Implementation.
 * 
 ***********************************************/
VKAPI_ATTR VkBool32
vk_debug_callback_
(
  _In_ VkDebugUtilsMessageSeverityFlagBitsEXT              messageSeverity,
  _In_ VkDebugUtilsMessageTypeFlagsEXT                     messageType,
  _In_ VkDebugUtilsMessengerCallbackDataEXT const         *pCallbackData,
  _In_ void                                               *pUserData
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

void
vk_create_instance_
(
  _In_ crude_gfx_device                                   *gpu,
  _In_ char const                                         *vk_application_name,
  _In_ uint32                                              vk_application_version,
  _In_ crude_allocator_container                           temporary_allocator
)
{
  VkDebugUtilsMessengerCreateInfoEXT                       debug_create_info;
  VkInstanceCreateInfo                                     instance_create_info;
  VkApplicationInfo                                        application;
  char const                                       *const *surface_extensions_array;
  char const                                             **instance_enabled_extensions;

  uint32                                                   surface_extensions_count, debug_extensions_count, instance_enabled_extensions_count;
  
  /* Get enabled extensions */ 
  surface_extensions_array = SDL_Vulkan_GetInstanceExtensions( &surface_extensions_count );
  debug_extensions_count = ARRAY_SIZE( vk_instance_required_extensions );

  instance_enabled_extensions_count = surface_extensions_count + debug_extensions_count;
  CRUDE_ARRAY_INITIALIZE_WITH_CAPACITY( instance_enabled_extensions, instance_enabled_extensions_count, temporary_allocator );
  CRUDE_ARRAY_SET_LENGTH( instance_enabled_extensions, instance_enabled_extensions_count );

  for ( uint32 i = 0; i < surface_extensions_count; ++i )
  {
    instance_enabled_extensions[ i ] = surface_extensions_array[ i ];
  }
  for ( uint32 i = 0; i < debug_extensions_count; ++i )
  {
    instance_enabled_extensions[ surface_extensions_count + i ] = vk_instance_required_extensions[ i ];
  }
  
  /* Setup application */ 
  application = ( VkApplicationInfo ) {
    .pApplicationName   = vk_application_name,
    .applicationVersion = vk_application_version,
    .pEngineName        = "crude_engine",
    .engineVersion      = VK_MAKE_VERSION( 1, 0, 0 ),
    .apiVersion         = VK_API_VERSION_1_3 
  };
  
  /* Initialize instance & debug_utils_messenger */ 
  instance_create_info = ( VkInstanceCreateInfo ){ 0 };
  instance_create_info.sType                    = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
  instance_create_info.pApplicationInfo         = &application;
  instance_create_info.flags                    = 0u;
  instance_create_info.ppEnabledExtensionNames  = instance_enabled_extensions;
  instance_create_info.enabledExtensionCount    = instance_enabled_extensions_count;
  instance_create_info.ppEnabledLayerNames     = instance_enabled_layers;
  instance_create_info.enabledLayerCount       = ARRAY_SIZE( instance_enabled_layers );
#ifdef VK_EXT_debug_utils
  debug_create_info = ( VkDebugUtilsMessengerCreateInfoEXT ){ 0 };
  memset( &debug_create_info, 0u, sizeof( debug_create_info ) );
  debug_create_info.sType            = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
  debug_create_info.pNext            = NULL;
  debug_create_info.flags            = 0u;
  debug_create_info.pfnUserCallback  = vk_debug_callback_;
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
  
  CRUDE_GFX_HANDLE_VULKAN_RESULT( vkCreateInstance( &instance_create_info, gpu->vk_allocation_callbacks, &gpu->vk_instance ), "failed to create instance" );
  
  gpu->vkCreateDebugUtilsMessengerEXT = ( PFN_vkCreateDebugUtilsMessengerEXT )vkGetInstanceProcAddr( gpu->vk_instance, "vkCreateDebugUtilsMessengerEXT" );
  gpu->vkDestroyDebugUtilsMessengerEXT = ( PFN_vkDestroyDebugUtilsMessengerEXT )vkGetInstanceProcAddr( gpu->vk_instance, "vkDestroyDebugUtilsMessengerEXT" );
}

void
vk_create_debug_utils_messsenger_
(
  _In_ crude_gfx_device                                   *gpu
)
{
  VkDebugUtilsMessengerCreateInfoEXT create_info = ( VkDebugUtilsMessengerCreateInfoEXT ){
    .sType            = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT,
    .pfnUserCallback  = vk_debug_callback_,
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
  CRUDE_GFX_HANDLE_VULKAN_RESULT( gpu->vkCreateDebugUtilsMessengerEXT( gpu->vk_instance, &create_info, gpu->vk_allocation_callbacks, &gpu->vk_debug_utils_messenger ), "Failed to create debug utils messenger" );
}

void
vk_create_surface_
(
  _In_ crude_gfx_device                                   *gpu
)
{
  if ( !SDL_Vulkan_CreateSurface( gpu->sdl_window, gpu->vk_instance, gpu->vk_allocation_callbacks, &gpu->vk_surface ) )
  {
    CRUDE_ABORT( CRUDE_CHANNEL_GRAPHICS, "failed to create vk_surface: %s", SDL_GetError() );
    return VK_NULL_HANDLE;
  }
}

void
vk_pick_physical_device_
(
  _In_ crude_gfx_device                                   *gpu,
  _In_ crude_allocator_container                           temporary_allocator
)
{
  VkPhysicalDevice                                        *available_physical_devices;
  VkPhysicalDevice                                         selected_physical_devices;
  VkPhysicalDeviceProperties                               selected_physical_properties;
  uint32                                                   available_physical_devices_count;

  vkEnumeratePhysicalDevices( gpu->vk_instance, &available_physical_devices_count, NULL );
  
  if ( available_physical_devices_count == 0u ) 
  {
    return VK_NULL_HANDLE;
  }
  
  CRUDE_ARRAY_INITIALIZE_WITH_LENGTH( available_physical_devices, available_physical_devices_count, temporary_allocator );
  vkEnumeratePhysicalDevices( gpu->vk_instance, &available_physical_devices_count, available_physical_devices );
  
  selected_physical_devices = VK_NULL_HANDLE;
  for ( uint32 i = 0; i < available_physical_devices_count; ++i )
  {
    VkPhysicalDeviceProperties                             current_physical_properties;
    VkPhysicalDevice                                       current_physical_device;

    int32                                                  queue_family_index;

    current_physical_device = available_physical_devices[ i ];
    vkGetPhysicalDeviceProperties( current_physical_device, &current_physical_properties );
    
    if ( current_physical_properties.deviceType != VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU )
    {
      continue;
    }
    if ( !vk_check_support_required_extensions_( current_physical_device, temporary_allocator ) )
    {
      continue;
    }
    if ( !vk_check_swap_chain_adequate_( current_physical_device, gpu->vk_surface ) )
    {
      continue;
    }
    if ( !vk_check_support_required_features_( current_physical_device ) )
    {
      continue;
    }
    
    queue_family_index = vk_get_supported_queue_family_index_( current_physical_device, gpu->vk_surface, temporary_allocator ); 
    if ( queue_family_index == -1 )
    {
      continue;
    }
    
    selected_physical_devices = current_physical_device;
    break;
  }
  
  if ( selected_physical_devices == VK_NULL_HANDLE )
  {
    CRUDE_LOG_INFO( CRUDE_CHANNEL_GRAPHICS, "Can't find suitable physical device! physical_devices_count: %i", available_physical_devices_count );
    return VK_NULL_HANDLE;
  }

  vkGetPhysicalDeviceProperties( selected_physical_devices, &selected_physical_properties );
  CRUDE_LOG_INFO( CRUDE_CHANNEL_GRAPHICS, "Selected physical device %s %i", selected_physical_properties.deviceName, selected_physical_properties.deviceType );

  gpu->vk_physical_device = selected_physical_devices;
}

void
vk_create_device_
(
  _In_ crude_gfx_device                                   *gpu,
  _In_ crude_allocator_container                           temporary_allocator
)
{
  VkPhysicalDeviceDynamicRenderingFeaturesKHR              dynamic_rendering_features;
  VkPhysicalDeviceDescriptorIndexingFeatures               indexing_features;
  VkPhysicalDeviceFeatures2                                physical_features2;
  VkDeviceCreateInfo                                       device_create_info;
  VkDeviceQueueCreateInfo                                  queue_create_infos[ 2 ];
  VkQueueFamilyProperties                                 *queue_families;
  uint32                                                   queue_family_count, main_queue_index, transfer_queue_index, compute_queue_index, present_queue_index;

  queue_family_count = 0;
  vkGetPhysicalDeviceQueueFamilyProperties( gpu->vk_physical_device, &queue_family_count, NULL );
  
  CRUDE_ARRAY_INITIALIZE_WITH_LENGTH( queue_families, queue_family_count, temporary_allocator );
  vkGetPhysicalDeviceQueueFamilyProperties( gpu->vk_physical_device, &queue_family_count, queue_families );
  
  main_queue_index = UINT32_MAX;
  transfer_queue_index = UINT32_MAX;
  compute_queue_index = UINT32_MAX;
  present_queue_index = UINT32_MAX;
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
  
  gpu->vk_main_queue_family = main_queue_index;
  gpu->vk_transfer_queue_family = transfer_queue_index;

  float const queue_priority[] = { 1.0f };

  queue_create_infos[ 0 ] = ( VkDeviceQueueCreateInfo ){
    .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
    .queueFamilyIndex = main_queue_index,
    .queueCount = 1,
    .pQueuePriorities = queue_priority,
  };
  
  if ( transfer_queue_index < queue_family_count )
  {
    queue_create_infos[ 1 ]  = ( VkDeviceQueueCreateInfo ){
    .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
    .queueFamilyIndex = transfer_queue_index,
    .queueCount = 1,
    .pQueuePriorities = queue_priority,
    };
  }
  
  dynamic_rendering_features = ( VkPhysicalDeviceDynamicRenderingFeaturesKHR ){
    .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DYNAMIC_RENDERING_FEATURES_KHR,
  };

  indexing_features = ( VkPhysicalDeviceDescriptorIndexingFeatures ) {
    .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_INDEXING_FEATURES,
    .pNext = &dynamic_rendering_features
  };

  physical_features2 = ( VkPhysicalDeviceFeatures2 ){ 
    .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2,
    .pNext = &indexing_features,
  };
  vkGetPhysicalDeviceFeatures2( gpu->vk_physical_device, &physical_features2 );

  device_create_info = ( VkDeviceCreateInfo ){
    .sType                    = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
    .pNext                    = &physical_features2,
    .flags                    = 0u,
    .queueCreateInfoCount     = transfer_queue_index < queue_family_count ? 2 : 1,
    .pQueueCreateInfos        = queue_create_infos,
    .pEnabledFeatures         = NULL,
    .enabledExtensionCount    = ARRAY_SIZE( vk_device_required_extensions ),
    .ppEnabledExtensionNames  = vk_device_required_extensions,
    .enabledLayerCount        = ARRAY_SIZE( vk_required_layers ),
    .ppEnabledLayerNames      = vk_required_layers,
  };

  CRUDE_GFX_HANDLE_VULKAN_RESULT( vkCreateDevice( gpu->vk_physical_device, &device_create_info, gpu->vk_allocation_callbacks, &gpu->vk_device ), "failed to create logic device!" );
  vkGetDeviceQueue( gpu->vk_device, main_queue_index, 0u, &gpu->vk_main_queue );
  if ( transfer_queue_index < queue_family_count )
  {
    vkGetDeviceQueue( gpu->vk_device, transfer_queue_index, 0, &gpu->vk_transfer_queue );
  }
  
  gpu->vkCmdBeginRenderingKHR = ( PFN_vkCmdBeginRenderingKHR )vkGetDeviceProcAddr( gpu->vk_device, "vkCmdBeginRenderingKHR" );
  gpu->vkCmdEndRenderingKHR = (PFN_vkCmdEndRenderingKHR)vkGetDeviceProcAddr( gpu->vk_device, "vkCmdEndRenderingKHR" );
  gpu->vkSetDebugUtilsObjectNameEXT = vkGetDeviceProcAddr( gpu->vk_device, "vkSetDebugUtilsObjectNameEXT" );
}

void
vk_create_swapchain_
( 
  _In_ crude_gfx_device                                   *gpu,
  _In_ crude_allocator_container                           temporary_allocator
)
{
  VkSwapchainCreateInfoKHR                                 swapchain_create_info;
  VkPresentModeKHR                                         selected_present_mode;
  VkPresentModeKHR                                        *available_present_modes;
  VkSurfaceFormatKHR                                      *available_formats;
  VkSurfaceCapabilitiesKHR                                 surface_capabilities;
  VkExtent2D                                               swapchain_extent;
  uint32                                                   available_formats_count, available_present_modes_count, image_count;
  uint32                                                   queue_family_indices[ 1 ];
  bool                                                     surface_format_found;

  vkGetPhysicalDeviceSurfaceCapabilitiesKHR( gpu->vk_physical_device, gpu->vk_surface, &surface_capabilities);
  
  swapchain_extent = surface_capabilities.currentExtent;
  if ( swapchain_extent.width == UINT32_MAX )
  {
    swapchain_extent.width = crude_clamp( swapchain_extent.width, surface_capabilities.minImageExtent.width, surface_capabilities.maxImageExtent.width );
    swapchain_extent.height = crude_clamp( swapchain_extent.height, surface_capabilities.minImageExtent.height, surface_capabilities.maxImageExtent.height );
  }

  gpu->vk_swapchain_width  = swapchain_extent.width;
  gpu->vk_swapchain_height = swapchain_extent.height;
  
  available_formats_count;
  vkGetPhysicalDeviceSurfaceFormatsKHR( gpu->vk_physical_device, gpu->vk_surface, &available_formats_count, NULL );

  if ( available_formats_count == 0u )
  {
    CRUDE_LOG_ERROR( CRUDE_CHANNEL_GRAPHICS, "Can't find available surface format" );
    return VK_NULL_HANDLE;
  }

  CRUDE_ARRAY_INITIALIZE_WITH_LENGTH( available_formats, available_formats_count, temporary_allocator );
  vkGetPhysicalDeviceSurfaceFormatsKHR( gpu->vk_physical_device, gpu->vk_surface, &available_formats_count, available_formats );

  surface_format_found = false;
  for ( uint32 i = 0; i < available_formats_count; ++i )
  {
    if ( available_formats[ i ].format == VK_FORMAT_B8G8R8A8_SRGB && available_formats[ i ].colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR )
    {
      gpu->vk_surface_format = available_formats[ i ];
      surface_format_found = true;
    }
  }

  if ( !surface_format_found )
  {
    CRUDE_LOG_ERROR( CRUDE_CHANNEL_GRAPHICS, "Can't find available surface format" );
    CRUDE_ARRAY_DEINITIALIZE( available_formats );
    return VK_NULL_HANDLE;
  }
  
  available_present_modes_count;
  vkGetPhysicalDeviceSurfacePresentModesKHR( gpu->vk_physical_device, gpu->vk_surface, &available_present_modes_count, NULL );
  if ( available_present_modes_count == 0u ) 
  {
    CRUDE_LOG_ERROR( CRUDE_CHANNEL_GRAPHICS, "Can't find available surface present_mode" );
    CRUDE_ARRAY_DEINITIALIZE( available_formats );
    return VK_NULL_HANDLE;
  }
  
  
  CRUDE_ARRAY_INITIALIZE_WITH_LENGTH( available_present_modes, available_present_modes_count, temporary_allocator );
  vkGetPhysicalDeviceSurfacePresentModesKHR( gpu->vk_physical_device, gpu->vk_surface, &available_present_modes_count, available_present_modes );

  selected_present_mode = VK_PRESENT_MODE_FIFO_KHR;
  for ( uint32 i = 0; i < available_present_modes_count; ++i )
  {
    if ( available_present_modes[ i ] == VK_PRESENT_MODE_MAILBOX_KHR )
    {
      selected_present_mode = VK_PRESENT_MODE_MAILBOX_KHR;
      break;
    }
  }

  image_count = ( selected_present_mode == VK_PRESENT_MODE_IMMEDIATE_KHR ? 2 : 3 );
  queue_family_indices[ 0 ] = gpu->vk_main_queue_family;
  
  swapchain_create_info = ( VkSwapchainCreateInfoKHR ) {
    .sType                  = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
    .pNext                  = NULL,
    .surface                = gpu->vk_surface,
    .minImageCount          = image_count,
    .imageFormat            = gpu->vk_surface_format.format,
    .imageColorSpace        = gpu->vk_surface_format.colorSpace,
    .imageExtent            = swapchain_extent,
    .imageArrayLayers       = 1,
    .imageUsage             = VK_IMAGE_USAGE_TRANSFER_DST_BIT,
    .imageSharingMode       = ARRAY_SIZE( queue_family_indices ) > 1 ? VK_SHARING_MODE_CONCURRENT : VK_SHARING_MODE_EXCLUSIVE,
    .queueFamilyIndexCount  = ARRAY_SIZE( queue_family_indices ),
    .pQueueFamilyIndices    = queue_family_indices,
    .preTransform           = surface_capabilities.currentTransform,
    .compositeAlpha         = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
    .presentMode            = selected_present_mode,
    .clipped                = true,
    .oldSwapchain           = VK_NULL_HANDLE,
  };
  
  CRUDE_GFX_HANDLE_VULKAN_RESULT( vkCreateSwapchainKHR( gpu->vk_device, &swapchain_create_info, gpu->vk_allocation_callbacks, &gpu->vk_swapchain ), "Failed to create swapchain!" );

  vkGetSwapchainImagesKHR( gpu->vk_device, gpu->vk_swapchain, &gpu->vk_swapchain_images_count, NULL );
  vkGetSwapchainImagesKHR( gpu->vk_device, gpu->vk_swapchain, &gpu->vk_swapchain_images_count, &gpu->vk_swapchain_images );

  VkCommandBufferBeginInfo beginInfo = { 
    .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
    .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT
  };
  
  crude_gfx_cmd_buffer *cmd = crude_gfx_get_primary_cmd( gpu, 0, false );
  vkBeginCommandBuffer( cmd->vk_cmd_buffer, &beginInfo );
  for ( size_t i = 0; i < gpu->vk_swapchain_images_count; ++i )
  {
    crude_gfx_cmd_add_image_barrier( cmd, gpu->vk_swapchain_images[ i ], CRUDE_GFX_RESOURCE_STATE_UNDEFINED, CRUDE_GFX_RESOURCE_STATE_PRESENT, 0, 1, false );
  }
  vkEndCommandBuffer( cmd->vk_cmd_buffer );

  VkSubmitInfo submitInfo = { 
    .sType              = VK_STRUCTURE_TYPE_SUBMIT_INFO,
    .commandBufferCount = 1,
    .pCommandBuffers    = &cmd->vk_cmd_buffer,
  };
  
  vkQueueSubmit( gpu->vk_main_queue, 1, &submitInfo, VK_NULL_HANDLE );
  vkQueueWaitIdle( gpu->vk_main_queue );
}

void
vk_create_vma_allocator_
(
  _In_ crude_gfx_device                                   *gpu
)
{
  VmaAllocatorCreateInfo allocator_info = {
    .physicalDevice   = gpu->vk_physical_device,
    .device           = gpu->vk_device,
    .instance         = gpu->vk_instance,
  };
  CRUDE_GFX_HANDLE_VULKAN_RESULT( vmaCreateAllocator( &allocator_info, &gpu->vma_allocator ), "Failed to create vma allocator" );
}

void
vk_create_descriptor_pool_
(
  _In_ crude_gfx_device                                   *gpu
)
{
  VkDescriptorPoolSize                                     pool_sizes_bindless[ 2 ];
  VkDescriptorPoolCreateInfo                               pool_info;
  uint32                                                   pool_count;
  VkDescriptorSetLayoutBinding                             vk_binding[ 2 ];
  VkDescriptorBindingFlags                                 bindless_flags;
  VkDescriptorBindingFlags                                 binding_flags[ 2 ];
  VkDescriptorSetLayoutBindingFlagsCreateInfoEXT           extended_info;
  VkDescriptorSetLayoutCreateInfo                          layout_info;
  VkDescriptorSetAllocateInfo                              alloc_info;

  pool_sizes_bindless[ 0 ] = ( VkDescriptorPoolSize ){ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, CRUDE_GFX_MAX_BINDLESS_RESOURCES };
  pool_sizes_bindless[ 1 ] = ( VkDescriptorPoolSize ){ VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, CRUDE_GFX_MAX_BINDLESS_RESOURCES };
  
  pool_info = ( VkDescriptorPoolCreateInfo ){
    .sType         = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
    .flags         = VK_DESCRIPTOR_POOL_CREATE_UPDATE_AFTER_BIND_BIT_EXT,
    .maxSets       = CRUDE_GFX_MAX_BINDLESS_RESOURCES * ARRAY_SIZE( pool_sizes_bindless ),
    .poolSizeCount = ARRAY_SIZE( pool_sizes_bindless ),
    .pPoolSizes    = pool_sizes_bindless,
  };
  CRUDE_GFX_HANDLE_VULKAN_RESULT( vkCreateDescriptorPool( gpu->vk_device, &pool_info, gpu->vk_allocation_callbacks, &gpu->vk_bindless_descriptor_pool ), "Failed create descriptor pool" );

  pool_count = ARRAY_SIZE( pool_sizes_bindless );
  vk_binding[ 0 ] = ( VkDescriptorSetLayoutBinding ){
    .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
    .descriptorCount = CRUDE_GFX_MAX_BINDLESS_RESOURCES,
    .binding = CRUDE_GFX_BINDLESS_TEXTURE_BINDING,
    .stageFlags = VK_SHADER_STAGE_ALL,
    .pImmutableSamplers = NULL,
  };
  vk_binding[ 1 ] = ( VkDescriptorSetLayoutBinding ){
    .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
    .descriptorCount = CRUDE_GFX_MAX_BINDLESS_RESOURCES,
    .binding = CRUDE_GFX_BINDLESS_TEXTURE_BINDING + 1,
    .stageFlags = VK_SHADER_STAGE_ALL,
    .pImmutableSamplers = NULL,
  };

  binding_flags[ 0 ] = VK_DESCRIPTOR_BINDING_UPDATE_AFTER_BIND_BIT_EXT;
  binding_flags[ 1 ] = VK_DESCRIPTOR_BINDING_UPDATE_AFTER_BIND_BIT_EXT;
  
  extended_info = ( VkDescriptorSetLayoutBindingFlagsCreateInfoEXT ){
    .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO_EXT,
    .bindingCount = pool_count,
    .pBindingFlags = binding_flags
  };

  layout_info = ( VkDescriptorSetLayoutCreateInfo ){
    .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
    .bindingCount = pool_count,
    .pBindings = vk_binding,
    .flags = VK_DESCRIPTOR_SET_LAYOUT_CREATE_UPDATE_AFTER_BIND_POOL_BIT_EXT,
    .pNext = &extended_info
  };
  
  CRUDE_GFX_HANDLE_VULKAN_RESULT( vkCreateDescriptorSetLayout( gpu->vk_device, &layout_info, gpu->vk_allocation_callbacks, &gpu->vk_bindless_descriptor_set_layout ), "Failed create descriptor set layout" );
  
  alloc_info = ( VkDescriptorSetAllocateInfo ){
    .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
    .descriptorPool = gpu->vk_bindless_descriptor_pool,
    .descriptorSetCount = 1,
    .pSetLayouts = &gpu->vk_bindless_descriptor_set_layout
  };
  
  CRUDE_GFX_HANDLE_VULKAN_RESULT( vkAllocateDescriptorSets( gpu->vk_device, &alloc_info, &gpu->vk_bindless_descriptor_set ), "Failed allocate descriptor set" );
}

void
vk_create_timestamp_query_pool_
(
  _In_ crude_gfx_device                                   *gpu
)
{
  uint32 gpu_time_queries_per_frame = 32;
  VkQueryPoolCreateInfo query_pool_create_info = { 
    .sType = VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO,
    .pNext = NULL,
    .flags = 0u,
    .queryType = VK_QUERY_TYPE_TIMESTAMP,
    .queryCount = gpu_time_queries_per_frame * 2u * CRUDE_GFX_MAX_SWAPCHAIN_IMAGES,
    .pipelineStatistics = 0u,
  };
  CRUDE_GFX_HANDLE_VULKAN_RESULT( vkCreateQueryPool( gpu->vk_device, &query_pool_create_info, gpu->vk_allocation_callbacks, &gpu->vk_timestamp_query_pool ), "Failed to create query pool" );
}

int32
vk_get_supported_queue_family_index_
(
  _In_ VkPhysicalDevice                                    vk_physical_device,
  _In_ VkSurfaceKHR                                        vk_surface,
  _In_ crude_allocator_container                           temporary_allocator
)
{
  VkQueueFamilyProperties                                 *queue_families_properties;
  uint32                                                   queue_family_count;
  int32                                                    queue_index;

  vkGetPhysicalDeviceQueueFamilyProperties( vk_physical_device, &queue_family_count, NULL );
  if ( queue_family_count == 0u )
  {
    return -1;
  }
  
  CRUDE_ARRAY_INITIALIZE_WITH_LENGTH( queue_families_properties, queue_family_count, temporary_allocator );
  vkGetPhysicalDeviceQueueFamilyProperties( vk_physical_device, &queue_family_count, queue_families_properties );
  
  queue_index = -1;
  for ( uint32 i = 0; i < queue_family_count; ++i )
  {
    if ( queue_families_properties[ i ].queueCount > 0 && queue_families_properties[ i ].queueFlags & ( VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT ) )
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
  return queue_index;
}

bool
vk_check_support_required_extensions_
(
  _In_ VkPhysicalDevice                                    vk_physical_device,
  _In_ crude_allocator_container                           temporary_allocator
)
{
  VkExtensionProperties                                   *available_extensions;
  uint32                                                   available_extensions_count;
  bool                                                     support_required_extensions;
  
  vkEnumerateDeviceExtensionProperties( vk_physical_device, NULL, &available_extensions_count, NULL );
  if ( available_extensions_count == 0u)
  {
    return false;
  }
    
  CRUDE_ARRAY_INITIALIZE_WITH_LENGTH( available_extensions, available_extensions_count, temporary_allocator );
  vkEnumerateDeviceExtensionProperties( vk_physical_device, NULL, &available_extensions_count, available_extensions );

  support_required_extensions = true;
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

  return support_required_extensions;
}

bool
vk_check_swap_chain_adequate_
(
  _In_ VkPhysicalDevice                                    vk_physical_device,
  _In_ VkSurfaceKHR                                        vk_surface
)
{
  uint32 formats_count, presents_mode_count;

  vkGetPhysicalDeviceSurfaceFormatsKHR( vk_physical_device, vk_surface, &formats_count, NULL );
  if ( formats_count == 0u )
  {
    return false;
  }

  vkGetPhysicalDeviceSurfacePresentModesKHR( vk_physical_device, vk_surface, &presents_mode_count, NULL );
  if ( presents_mode_count == 0u ) 
  {
    return false;
  }

  return true;
}

bool
vk_check_support_required_features_
(
  _In_ VkPhysicalDevice                                    vk_physical_device
)
{
  VkPhysicalDeviceFeatures features;
  vkGetPhysicalDeviceFeatures( vk_physical_device, &features );
  return features.samplerAnisotropy;
}

void
vk_destroy_swapchain_
(
  _In_ crude_gfx_device                                   *gpu
)
{
  vkDestroySwapchainKHR( gpu->vk_device, gpu->vk_swapchain, gpu->vk_allocation_callbacks );
}

void
vk_create_texture_
(
  _In_ crude_gfx_device                                   *gpu,
  _In_ crude_gfx_texture_creation const                   *creation,
  _In_ crude_gfx_texture_handle                            handle,
  _In_ crude_gfx_texture                                  *texture
)
{
  texture->width          = creation->width;
  texture->height         = creation->height;
  texture->depth          = creation->depth;
  texture->mipmaps        = creation->mipmaps;
  texture->type           = creation->type;
  texture->name           = crude_string_buffer_append_use_f( &gpu->objects_names_string_buffer, "%s", creation->name );
  texture->vk_format      = creation->format;
  texture->sampler        = NULL;
  texture->flags          = creation->flags;
  texture->handle         = handle;
  
  {
    VmaAllocationCreateInfo                                memory_info;
    VkImageCreateInfo                                      image_info;
    bool                                                   is_render_target, is_compute_used;

    image_info = ( VkImageCreateInfo ) {
      .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
      .format = texture->vk_format,
      .imageType = crude_gfx_to_vk_image_type( creation->type ),
      .extent.width = creation->width,
      .extent.height = creation->height,
      .extent.depth = creation->depth,
      .mipLevels = creation->mipmaps,
      .arrayLayers = 1,
      .samples = VK_SAMPLE_COUNT_1_BIT,
      .tiling = VK_IMAGE_TILING_OPTIMAL,
      .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
      .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED
    };

    is_render_target = ( creation->flags & CRUDE_GFX_TEXTURE_MASK_RENDER_TARGET ) == CRUDE_GFX_TEXTURE_MASK_RENDER_TARGET;
    is_compute_used = ( creation->flags & CRUDE_GFX_TEXTURE_MASK_COMPUTE ) == CRUDE_GFX_TEXTURE_MASK_COMPUTE;
    
    image_info.usage = VK_IMAGE_USAGE_SAMPLED_BIT;

    if ( is_compute_used )
    {
      image_info.usage |= VK_IMAGE_USAGE_STORAGE_BIT;
    }

    if ( crude_gfx_has_depth_or_stencil( creation->format ) )
    {
      image_info.usage |= VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
    }
    else
    {
      image_info.usage |= VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
      image_info.usage |= is_render_target ? VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT : 0;
    }

    memory_info = ( VmaAllocationCreateInfo ){
      .usage = VMA_MEMORY_USAGE_GPU_ONLY
    };
    
    if ( CRUDE_RESOURCE_HANDLE_IS_INVALID( creation->alias ) )
    {
      CRUDE_GFX_HANDLE_VULKAN_RESULT( vmaCreateImage( gpu->vma_allocator, &image_info, &memory_info, &texture->vk_image, &texture->vma_allocation, NULL ), "Failed to create image!" );
      vmaSetAllocationName( gpu->vma_allocator, texture->vma_allocation, creation->name );
    }
    else
    {
      crude_gfx_texture* alias_texture = crude_gfx_access_texture( gpu, creation->alias );
      CRUDE_ASSERT( alias_texture );
      
      texture->vma_allocation = 0;
      CRUDE_GFX_HANDLE_VULKAN_RESULT( vmaCreateAliasingImage( gpu->vma_allocator, alias_texture->vma_allocation, &image_info, &texture->vk_image ), "Failed to create aliasing image!" );
    }
    
    crude_gfx_set_resource_name( gpu, VK_OBJECT_TYPE_IMAGE, texture->vk_image, creation->name );
  }

  
  {
    VkImageViewCreateInfo info = {
      .sType    = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
      .image    = texture->vk_image,
      .viewType = crude_gfx_to_vk_image_view_type( creation->type ),
      .format   = texture->vk_format,
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
  }

  {
    crude_gfx_resource_update texture_update_event = { 
      .type          = CRUDE_GFX_RESOURCE_DELETION_TYPE_TEXTURE,
      .handle        = handle.index,
      .current_frame = gpu->current_frame
    };
    texture->vk_image_layout = VK_IMAGE_LAYOUT_UNDEFINED;
    CRUDE_ARRAY_PUSH( gpu->texture_to_update_bindless, texture_update_event );
  }
}

void
vk_resize_swapchain_
(
  _In_ crude_gfx_device                                   *gpu
)
{
  vkDeviceWaitIdle( gpu->vk_device );
  
  {
    VkSurfaceCapabilitiesKHR new_surface_capabilities;
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR( gpu->vk_physical_device, gpu->vk_surface, &new_surface_capabilities );
    
    if ( new_surface_capabilities.currentExtent.width == 0 || new_surface_capabilities.currentExtent.height == 0 )
    {
      return;
    }
  }

  vk_destroy_swapchain_( gpu );
  vkDestroySurfaceKHR( gpu->vk_instance, gpu->vk_surface, gpu->vk_allocation_callbacks );
  vk_create_surface_( gpu );

  {
    crude_allocator_container temporary_allocator = crude_stack_allocator_pack( gpu->temporary_allocator );
    uint32 marker = crude_stack_allocator_get_marker( gpu->temporary_allocator );
    vk_create_swapchain_( gpu, temporary_allocator );
    crude_stack_allocator_free_marker( gpu->temporary_allocator, marker );
  }

  gpu->swapchain_resized_last_frame = true;

  vkDeviceWaitIdle( gpu->vk_device );
}

crude_gfx_vertex_component_format
reflect_format_to_vk_format_
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
vk_reflect_shader_
(
  _In_ crude_gfx_device                                   *gpu,
  _In_ void                                               *code,
  _In_ uint32                                              code_size,
  _In_ crude_gfx_shader_reflect                           *reflect
)
{
  SpvReflectShaderModule                                   spv_reflect;
  SpvReflectResult                                         result;
  
  result = spvReflectCreateShaderModule( code_size, code, &spv_reflect );
  CRUDE_ASSERT( result == SPV_REFLECT_RESULT_SUCCESS );
  
  if ( spv_reflect.shader_stage == SPV_REFLECT_SHADER_STAGE_VERTEX_BIT )
  {
    CRUDE_ARRAY_INITIALIZE_WITH_LENGTH( reflect->input.vertex_attributes, spv_reflect.input_variable_count, gpu->allocator_container );
    CRUDE_ARRAY_INITIALIZE_WITH_LENGTH( reflect->input.vertex_streams, spv_reflect.input_variable_count, gpu->allocator_container );

    for ( uint32 input_index = 0; input_index < spv_reflect.input_variable_count; ++input_index )
    {
      SpvReflectInterfaceVariable const                   *spv_input;
      uint32                                               stride;

      spv_input = spv_reflect.input_variables[ input_index ];
      reflect->input.vertex_attributes[ input_index ] = ( crude_gfx_vertex_attribute ) {
        .location = spv_input->location,
        .binding = spv_input->location,
        .offset = 0,
        .format = reflect_format_to_vk_format_( spv_input->format )
      };
      
      stride = ( spv_input->numeric.vector.component_count * spv_input->numeric.scalar.width ) / 8;
      reflect->input.vertex_streams[ input_index ] = ( crude_gfx_vertex_stream ){
        .binding = spv_input->location,
        .stride = stride,
        .input_rate = CRUDE_GFX_VERTEX_INPUT_RATE_PER_VERTEX
      };
    }
  }

  for ( uint32 set_index = 0; set_index < spv_reflect.descriptor_set_count; ++set_index )
  {
    SpvReflectDescriptorSet const                         *spv_descriptor_set;
    crude_gfx_descriptor_set_layout_creation              *set_layout;
    
    spv_descriptor_set = &spv_reflect.descriptor_sets[ set_index ];
    set_layout = &reflect->descriptor.sets[ spv_descriptor_set->set ];
    set_layout->set_index = spv_descriptor_set->set;
    set_layout->num_bindings = 0;

    for ( uint32 binding_index = 0; binding_index < spv_descriptor_set->binding_count; ++binding_index )
    {
      SpvReflectDescriptorBinding const                   *spv_binding;
      crude_gfx_descriptor_set_layout_binding             *binding;

      spv_binding = spv_descriptor_set->bindings[ binding_index ];
      binding = &set_layout->bindings[ binding_index ];
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
vk_destroy_resources_instant_
(
  _In_ crude_gfx_device                                   *gpu,
  _In_ crude_gfx_resource_deletion_type                    type,
  _In_ crude_gfx_resource_index                            handle
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
    case CRUDE_GFX_RESOURCE_DELETION_TYPE_FRAMEBUFFER:
    {
      crude_gfx_destroy_framebuffer_instant( gpu, ( crude_gfx_framebuffer_handle ){ handle } );
      break;
    }
  }
}

void dump_shader_code_
(
  _In_ char const                                         *code,
  _In_ VkShaderStageFlagBits                               stage,
  _In_ char const                                         *name,
  _In_ crude_string_buffer                                *temporary_string_buffer
)
{
  CRUDE_LOG_ERROR( CRUDE_CHANNEL_GRAPHICS, "Error in creation of shader %s, stage %s. Writing shader:\n", name, crude_vk_shader_stage_to_defines( stage ) );
  
  char const * current_code = code;
  uint32 line_index = 1;
  while ( current_code )
  {
    char const *end_of_line = current_code;
    if ( !end_of_line || *end_of_line == 0 )
    {
      break;
    }
    while ( ( *end_of_line != '\n' ) && ( *end_of_line != '\r' ) )
    {
      ++end_of_line;
    }
    if ( *end_of_line == '\r' )
    {
      ++end_of_line;
    }
    if ( *end_of_line == '\n' )
    {
        ++end_of_line;
    }
    
    crude_string_buffer_clear( temporary_string_buffer );
    
    char *line = crude_string_buffer_append_use_f( temporary_string_buffer, current_code, 0, ( end_of_line - current_code ) );
    CRUDE_LOG_ERROR( CRUDE_CHANNEL_GRAPHICS, "%u: %s", line_index++, line );
    
    current_code = end_of_line;
  }
}