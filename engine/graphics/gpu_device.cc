#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>

#include <core/profiler.h>
#include <core/string.h>
#include <core/file.h>
#include <core/process.h>
#include <graphics/gpu_profiler.h>

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
  VK_KHR_DYNAMIC_RENDERING_EXTENSION_NAME,
  VK_KHR_SYNCHRONIZATION_2_EXTENSION_NAME,
  VK_KHR_TIMELINE_SEMAPHORE_EXTENSION_NAME,
  VK_EXT_MESH_SHADER_EXTENSION_NAME,
  VK_KHR_SHADER_DRAW_PARAMETERS_EXTENSION_NAME,
  VK_KHR_FRAGMENT_SHADING_RATE_EXTENSION_NAME
};

#ifdef CRUDE_GRAPHICS_VALIDATION_LAYERS_ENABLED
static char const *const vk_instance_required_debug_extensions[] =
{
  VK_EXT_DEBUG_UTILS_EXTENSION_NAME
};

static char const *const vk_required_debug_layers[] =
{
  "VK_LAYER_KHRONOS_validation"
};
#endif /* CRUDE_GRAPHICS_VALIDATION_LAYERS_ENABLED */

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
  _In_ void const                                         *code,
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
  gpu->mesh_shaders_extension_present = false;

  crude_string_buffer_initialize( &gpu->objects_names_string_buffer, CRUDE_RMEGA( 1 ), gpu->allocator_container );

  vk_create_instance_( gpu, creation->vk_application_name, creation->vk_application_version, temporary_allocator );
#ifdef CRUDE_GRAPHICS_VALIDATION_LAYERS_ENABLED
  vk_create_debug_utils_messsenger_( gpu );
#endif /* CRUDE_GRAPHICS_VALIDATION_LAYERS_ENABLED */
  vk_create_surface_( gpu );
  vk_pick_physical_device_( gpu, temporary_allocator );
  vk_create_device_( gpu, temporary_allocator );
  vk_create_vma_allocator_( gpu );
  
  {
    uint32 gpu_time_queries_per_frame = 32;
    uint32 num_pools = creation->num_threads * CRUDE_GFX_MAX_SWAPCHAIN_IMAGES;
    gpu->num_threads = creation->num_threads;
    CRUDE_ARRAY_INITIALIZE_WITH_LENGTH( gpu->thread_frame_pools, num_pools, gpu->allocator_container );
    
    gpu->gpu_time_queries_manager = CRUDE_STATIC_CAST( crude_gfx_gpu_time_queries_manager*, CRUDE_ALLOCATE( gpu->allocator_container, sizeof( crude_gfx_gpu_time_queries_manager ) ) );
    crude_gfx_gpu_time_queries_manager_initialize( gpu->gpu_time_queries_manager, gpu->thread_frame_pools, gpu->allocator_container, gpu_time_queries_per_frame, creation->num_threads , CRUDE_GFX_MAX_SWAPCHAIN_IMAGES );
    
    for ( uint32 i = 0; i < CRUDE_ARRAY_LENGTH( gpu->thread_frame_pools ); ++i )
    {
      crude_gfx_gpu_thread_frame_pools *pool = &gpu->thread_frame_pools[ i ];
      pool->time_queries = &gpu->gpu_time_queries_manager->query_trees[ i ];
    
      VkCommandPoolCreateInfo cmd_pool_info = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
        .flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
        .queueFamilyIndex = gpu->vk_main_queue_family,
      };
      
      vkCreateCommandPool( gpu->vk_device, &cmd_pool_info, gpu->vk_allocation_callbacks, &pool->vk_command_pool );
      
      /* Create timestamp query pool used for GPU timings */
      VkQueryPoolCreateInfo timestamp_pool_info = {
        .sType = VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO, 
        .queryType = VK_QUERY_TYPE_TIMESTAMP,
        .queryCount = gpu_time_queries_per_frame * 2u,
      };
      vkCreateQueryPool( gpu->vk_device, &timestamp_pool_info, gpu->vk_allocation_callbacks, &pool->vk_timestamp_query_pool );
      
      /* Create pipeline statistics query pool */
      VkQueryPoolCreateInfo statistics_pool_info = {
        .sType = VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO, 
        .queryType = VK_QUERY_TYPE_PIPELINE_STATISTICS,
        .queryCount = 7,
        .pipelineStatistics = VK_QUERY_PIPELINE_STATISTIC_INPUT_ASSEMBLY_VERTICES_BIT |
          VK_QUERY_PIPELINE_STATISTIC_INPUT_ASSEMBLY_PRIMITIVES_BIT |
          VK_QUERY_PIPELINE_STATISTIC_VERTEX_SHADER_INVOCATIONS_BIT |
          VK_QUERY_PIPELINE_STATISTIC_CLIPPING_INVOCATIONS_BIT |
          VK_QUERY_PIPELINE_STATISTIC_CLIPPING_PRIMITIVES_BIT |
          VK_QUERY_PIPELINE_STATISTIC_FRAGMENT_SHADER_INVOCATIONS_BIT |
          VK_QUERY_PIPELINE_STATISTIC_COMPUTE_SHADER_INVOCATIONS_BIT
      };
      vkCreateQueryPool( gpu->vk_device, &statistics_pool_info, gpu->vk_allocation_callbacks, &pool->vk_pipeline_stats_query_pool);
    }
  }

  crude_resource_pool_initialize( &gpu->buffers, gpu->allocator_container, 4096, sizeof( crude_gfx_buffer ) );
  crude_resource_pool_initialize( &gpu->textures, gpu->allocator_container, 512, sizeof( crude_gfx_texture ) );
  crude_resource_pool_initialize( &gpu->render_passes, gpu->allocator_container, 256, sizeof( crude_gfx_render_pass ) );
  crude_resource_pool_initialize( &gpu->descriptor_set_layouts, gpu->allocator_container, 128, sizeof( crude_gfx_descriptor_set_layout ) );
  crude_resource_pool_initialize( &gpu->descriptor_sets, gpu->allocator_container, 128, sizeof( crude_gfx_descriptor_set ) );
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
  crude_stack_allocator_free_marker( creation->temporary_allocator, temporary_allocator_mark );
  
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
    
    buffer_map = CRUDE_COMPOUNT( crude_gfx_map_buffer_parameters, { .buffer = gpu->dynamic_buffer } );
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
  crude_gfx_destroy_sampler( gpu, gpu->default_sampler );
  crude_gfx_destroy_descriptor_set_layout( gpu, gpu->bindless_descriptor_set_layout_handle );
  crude_gfx_destroy_descriptor_set( gpu, gpu->bindless_descriptor_set_handle );
  
  for ( uint32 i = 0; i < CRUDE_ARRAY_LENGTH( gpu->resource_deletion_queue ); ++i )
  {
    crude_gfx_resource_update* resource_deletion = &gpu->resource_deletion_queue[ i ];
  
    if ( resource_deletion->current_frame == -1 )
    {
      continue;
    }

    vk_destroy_resources_instant_( gpu, resource_deletion->type, resource_deletion->handle );
  }
  
  vkDestroyDescriptorPool( gpu->vk_device, gpu->vk_descriptor_pool, gpu->vk_allocation_callbacks );
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
  crude_resource_pool_deinitialize( &gpu->descriptor_sets );
  crude_resource_pool_deinitialize( &gpu->descriptor_set_layouts );
  crude_resource_pool_deinitialize( &gpu->pipelines );
  crude_resource_pool_deinitialize( &gpu->shaders );
  crude_resource_pool_deinitialize( &gpu->samplers );
  crude_resource_pool_deinitialize( &gpu->framebuffers );
  
  {
    for ( uint32 i = 0; i < CRUDE_ARRAY_LENGTH( gpu->thread_frame_pools ); ++i )
    {
      crude_gfx_gpu_thread_frame_pools *pool = &gpu->thread_frame_pools[ i ];
      vkDestroyCommandPool( gpu->vk_device, pool->vk_command_pool, gpu->vk_allocation_callbacks );
      vkDestroyQueryPool( gpu->vk_device, pool->vk_timestamp_query_pool, gpu->vk_allocation_callbacks );
      vkDestroyQueryPool( gpu->vk_device, pool->vk_pipeline_stats_query_pool, gpu->vk_allocation_callbacks );
    }
  }
  CRUDE_ARRAY_DEINITIALIZE( gpu->thread_frame_pools );
  CRUDE_DEALLOCATE( gpu->allocator_container, gpu->gpu_time_queries_manager );

  vmaDestroyAllocator( gpu->vma_allocator );
  vkDestroyDevice( gpu->vk_device, gpu->vk_allocation_callbacks );
  vkDestroySurfaceKHR( gpu->vk_instance, gpu->vk_surface, gpu->vk_allocation_callbacks );
#ifdef CRUDE_GRAPHICS_VALIDATION_LAYERS_ENABLED
  gpu->vkDestroyDebugUtilsMessengerEXT( gpu->vk_instance, gpu->vk_debug_utils_messenger, gpu->vk_allocation_callbacks );
#endif /* CRUDE_GRAPHICS_VALIDATION_LAYERS_ENABLED */
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
    }
  }

  {
    VkWriteDescriptorSet                                   bindless_descriptor_writes[ CRUDE_GFX_MAX_BINDLESS_RESOURCES ];
    VkDescriptorImageInfo                                  bindless_image_info[ CRUDE_GFX_MAX_BINDLESS_RESOURCES ];
    uint32                                                 current_write_index;

    current_write_index = 0;
    for ( int32 i = CRUDE_ARRAY_LENGTH( gpu->texture_to_update_bindless ) - 1; i >= 0; --i )
    {
      crude_gfx_descriptor_set                            *bindless_descriptor_set;
      VkDescriptorImageInfo                               *descriptor_image_info;
      VkWriteDescriptorSet                                *descriptor_write;
      crude_gfx_resource_update                           *texture_to_update;
      crude_gfx_texture                                   *texture;
      
      bindless_descriptor_set = crude_gfx_access_descriptor_set( gpu, gpu->bindless_descriptor_set_handle );
      texture_to_update = &gpu->texture_to_update_bindless[ i ];
      texture = crude_gfx_access_texture( gpu, CRUDE_COMPOUNT( crude_gfx_texture_handle, { texture_to_update->handle } ) );

      descriptor_write = &bindless_descriptor_writes[ current_write_index ];
      memset( descriptor_write, 0, sizeof( VkWriteDescriptorSet ) );
      descriptor_write->sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
      descriptor_write->descriptorCount = 1;
      descriptor_write->dstArrayElement = texture_to_update->handle;
      descriptor_write->descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
      descriptor_write->dstSet = bindless_descriptor_set->vk_descriptor_set;
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
      
      if ( texture_to_update->type == CRUDE_GFX_RESOURCE_DELETION_TYPE_TEXTURE )
      {
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
    VkSemaphoreSubmitInfo signal_semaphores[] = {
      { VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO_KHR, NULL, gpu->vk_rendering_finished_semaphore[ gpu->current_frame ], 0, VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT_KHR, 0 },
    };
    VkCommandBufferSubmitInfo command_buffers[] = {
      { VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO_KHR, NULL, enqueued_command_buffers[ 0 ], 0 },
      { VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO_KHR, NULL, enqueued_command_buffers[ 1 ], 0 },
      { VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO_KHR, NULL, enqueued_command_buffers[ 2 ], 0 },
      { VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO_KHR, NULL, enqueued_command_buffers[ 3 ], 0 },
    };
    
    VkSubmitInfo2 submit_info = {
      .sType                    = VK_STRUCTURE_TYPE_SUBMIT_INFO_2_KHR,
      .commandBufferInfoCount   = CRUDE_ARRAY_LENGTH( gpu->queued_command_buffers ),
      .pCommandBufferInfos      = command_buffers,
      .signalSemaphoreInfoCount = CRUDE_COUNTOF( signal_semaphores ),
      .pSignalSemaphoreInfos    = signal_semaphores
    };
    
    CRUDE_GFX_HANDLE_VULKAN_RESULT( gpu->vkQueueSubmit2KHR( gpu->vk_main_queue, 1, &submit_info, VK_NULL_HANDLE ), "Failed to sumbit queue" );
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
    
    crude_gfx_cmd_buffer *cmd = crude_gfx_get_primary_cmd( gpu, 1, true );
    crude_gfx_cmd_add_image_barrier( cmd, texture, CRUDE_GFX_RESOURCE_STATE_COPY_SOURCE, 0, 1, false );
    crude_gfx_cmd_add_image_barrier_ext2( cmd, gpu->vk_swapchain_images[ gpu->vk_swapchain_image_index ], CRUDE_GFX_RESOURCE_STATE_PRESENT, CRUDE_GFX_RESOURCE_STATE_COPY_DEST, 0, 1, false );
    vkCmdCopyImage( cmd->vk_cmd_buffer, texture->vk_image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, gpu->vk_swapchain_images[ gpu->vk_swapchain_image_index ], VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1u, &region );
    crude_gfx_cmd_add_image_barrier_ext2( cmd, gpu->vk_swapchain_images[ gpu->vk_swapchain_image_index ], CRUDE_GFX_RESOURCE_STATE_COPY_DEST, CRUDE_GFX_RESOURCE_STATE_PRESENT, 0, 1, false );
    crude_gfx_cmd_end( cmd );
  
    {
      VkSemaphoreSubmitInfo wait_semaphores[] = {
        { VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO_KHR, NULL, gpu->vk_rendering_finished_semaphore[ gpu->current_frame ], 0, VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT, 0 },
        { VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO_KHR, NULL, gpu->vk_image_avalivable_semaphores[ gpu->current_frame ], 0, VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT_KHR, 0 },
      };

      VkSemaphoreSubmitInfo signal_semaphores[] = {
        { VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO_KHR, NULL, gpu->vk_swapchain_updated_semaphore[ gpu->current_frame ], 0, VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT, 0 },
      };
      VkCommandBufferSubmitInfo command_buffers[] = {
        { VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO_KHR, NULL, cmd->vk_cmd_buffer, 0 },
      };

      VkSubmitInfo2 submit_info = {
        .sType                    = VK_STRUCTURE_TYPE_SUBMIT_INFO_2_KHR,    
        .waitSemaphoreInfoCount   = CRUDE_COUNTOF( wait_semaphores ),
        .pWaitSemaphoreInfos      = wait_semaphores,
        .commandBufferInfoCount   = CRUDE_COUNTOF( command_buffers ),
        .pCommandBufferInfos      = command_buffers,
        .signalSemaphoreInfoCount = CRUDE_COUNTOF( signal_semaphores ),
        .pSignalSemaphoreInfos    = signal_semaphores
      };
    
      VkFence swapchain_updated_fence = gpu->vk_command_buffer_executed_fences[ gpu->current_frame ];
      CRUDE_GFX_HANDLE_VULKAN_RESULT( gpu->vkQueueSubmit2KHR( gpu->vk_main_queue, 1, &submit_info, swapchain_updated_fence ), "Failed to sumbit queue" );
    }
  }
  
  {
    VkSemaphore wait_semaphores[] = { gpu->vk_swapchain_updated_semaphore[ gpu->current_frame ] };
    VkSwapchainKHR swap_chains[] = { gpu->vk_swapchain };
    VkPresentInfoKHR present_info = {
      .sType              = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
      .waitSemaphoreCount = 1,
      .pWaitSemaphores    = wait_semaphores,
      .swapchainCount     = CRUDE_COUNTOF( swap_chains ),
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

  crude_gfx_buffer* buffer_data = crude_gfx_access_buffer( gpu, buffer );
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
#ifdef CRUDE_GRAPHICS_VALIDATION_LAYERS_ENABLED
  VkDebugUtilsObjectNameInfoEXT name_info = {
    .sType        = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT,
    .objectType   = type,
    .objectHandle = handle,
    .pObjectName  = name,
  };
  gpu->vkSetDebugUtilsObjectNameEXT( gpu->vk_device, &name_info );
#endif /* CRUDE_GRAPHICS_VALIDATION_LAYERS_ENABLED */
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

bool
crude_gfx_texture_ready
(
  _In_ crude_gfx_device                                   *gpu,
  _In_ crude_gfx_texture_handle                            texture_handle
)
{
  crude_gfx_texture *texture = crude_gfx_access_texture( gpu, texture_handle );
  return texture->ready;
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
  char                                                    *vulkan_binaries_path, *glsl_compiler_path, *final_spirv_filename, *arguments;
  uint8                                                   *spirv_code;
  uint32                                                   spirv_codesize;
  
  spirv_code = NULL;
  spirv_codesize = 0u;

  temp_filename = "temp.shader";
  crude_write_file( temp_filename, code, code_size );
  
  crude_string_buffer_initialize( &temporary_string_buffer, CRUDE_RKILO( 1 ), crude_stack_allocator_pack( temporary_allocator ) );
  
  {
    char vulkan_env[ 512 ];
    crude_process_expand_environment_strings( "%VULKAN_SDK%", vulkan_env, 512 );
    vulkan_binaries_path = crude_string_buffer_append_use_f( &temporary_string_buffer, "%s\\Bin\\", vulkan_env );
  }

#if defined(_MSC_VER)
  glsl_compiler_path = crude_string_buffer_append_use_f( &temporary_string_buffer, "%sglslangValidator.exe", vulkan_binaries_path );
  final_spirv_filename = crude_string_buffer_append_use_f( &temporary_string_buffer, "shader_final.spv" );
  arguments = crude_string_buffer_append_use_f( &temporary_string_buffer, "glslangValidator.exe %s -V --target-env vulkan1.2 --glsl-version 460 -o %s -S %s --D %s", temp_filename, final_spirv_filename, crude_gfx_vk_shader_stage_to_compiler_extension( stage ), crude_gfx_vk_shader_stage_to_defines( stage ) );
#endif
  crude_process_execute( ".", glsl_compiler_path, arguments, "" );
  
  bool optimize_shaders = false;
  if ( optimize_shaders )
  {
    char* spirv_optimizer_path = crude_string_buffer_append_use_f( &temporary_string_buffer,"%sspirv-opt.exe", vulkan_binaries_path );
    char* optimized_spirv_filename = crude_string_buffer_append_use_f( &temporary_string_buffer,"shader_opt.spv" );
    char* spirv_opt_arguments = crude_string_buffer_append_use_f( &temporary_string_buffer,"spirv-opt.exe -O --preserve-bindings %s -o %s", final_spirv_filename, optimized_spirv_filename );
    
    crude_process_execute( ".", spirv_optimizer_path, spirv_opt_arguments, "" );
    crude_read_file_binary( final_spirv_filename, crude_stack_allocator_pack( temporary_allocator ), &spirv_code, &spirv_codesize );
    crude_file_delete( optimized_spirv_filename );
  }
  else
  {
    crude_read_file_binary( final_spirv_filename, crude_stack_allocator_pack( temporary_allocator ), &spirv_code, &spirv_codesize );
  }

  if ( !spirv_code )
  {
    CRUDE_LOG_ERROR( CRUDE_CHANNEL_GRAPHICS, "Error in creation of shader %s, stage %s. Writing shader:\n", name, crude_gfx_vk_shader_stage_to_defines( stage ) );
  }
  
  crude_file_delete( temp_filename );
  crude_file_delete( final_spirv_filename );
  
  shader_create_info = CRUDE_COMPOUNT( VkShaderModuleCreateInfo, {
    .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
    .codeSize = spirv_codesize,
    .pCode = CRUDE_REINTERPRET_CAST( uint32 const*, spirv_code ),
  } );
  return shader_create_info;
}

void
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
    .magFilter                = creation->mag_filter,
    .minFilter                = creation->min_filter,
    .mipmapMode               = creation->mip_filter,
    .addressModeU             = creation->address_mode_u,
    .addressModeV             = creation->address_mode_v,
    .addressModeW             = creation->address_mode_w,
    .anisotropyEnable         = 0,
    .compareEnable            = 0,
    .borderColor              = VK_BORDER_COLOR_INT_OPAQUE_WHITE,
    .unnormalizedCoordinates  = 0,
  };
  
  CRUDE_GFX_HANDLE_VULKAN_RESULT( vkCreateSampler( gpu->vk_device, &create_info, gpu->vk_allocation_callbacks, &sampler->vk_sampler ), "Failed to create sampler" );
  crude_gfx_set_resource_name( gpu, VK_OBJECT_TYPE_SAMPLER, CRUDE_CAST( uint64, sampler->vk_sampler ), creation->name );
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
      .imageSubresource = {
        .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
        .mipLevel = 0,
        .baseArrayLayer = 0,
        .layerCount = 1,
      },
      .imageOffset = { 0, 0, 0 },
      .imageExtent = { creation->width, creation->height, creation->depth }
    };
    
    crude_gfx_cmd_add_image_barrier( cmd, texture, CRUDE_GFX_RESOURCE_STATE_COPY_DEST, 0, 1, false );
    
    vkCmdCopyBufferToImage( cmd->vk_cmd_buffer, staging_buffer, texture->vk_image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region );
    
    if ( creation->mipmaps > 1 )
    {
      crude_gfx_cmd_add_image_barrier( cmd, texture, CRUDE_GFX_RESOURCE_STATE_COPY_SOURCE, 0, 1, false );
    }
    
    int32 w = creation->width;
    int32 h = creation->height;
    
    for ( int32 mip_index = 1; mip_index < creation->mipmaps; ++mip_index )
    {
      crude_gfx_cmd_add_image_barrier( cmd, texture, CRUDE_GFX_RESOURCE_STATE_COPY_DEST, mip_index, 1, false );
    
      VkImageBlit blit_region =
      { 
        .srcSubresource = {
          .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
          .mipLevel = CRUDE_STATIC_CAST( uint32, mip_index - 1 ),
          .baseArrayLayer = 0,
          .layerCount = 1,
        },
        .srcOffsets = {
          { 0, 0, 0 },
          { w, h, 1 },
        },
        .dstSubresource = {
          .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
          .mipLevel = CRUDE_STATIC_CAST( uint32, mip_index ),
          .baseArrayLayer = 0,
          .layerCount = 1,
        },
        .dstOffsets = {
          { 0, 0, 0 },
          { w / 2, h / 2, 1 },
        }
      };
    
      w /= 2;
      h /= 2;
    
      vkCmdBlitImage( cmd->vk_cmd_buffer, texture->vk_image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, texture->vk_image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &blit_region, VK_FILTER_LINEAR );
    
      crude_gfx_cmd_add_image_barrier( cmd, texture, CRUDE_GFX_RESOURCE_STATE_COPY_SOURCE, mip_index, 1, false );
    }
    
    crude_gfx_cmd_add_image_barrier( cmd, texture, CRUDE_GFX_RESOURCE_STATE_SHADER_RESOURCE, 0, creation->mipmaps, false );
    
    crude_gfx_cmd_end( cmd );
    
    {
      VkCommandBufferSubmitInfo command_buffers[] = {
        { VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO_KHR, NULL, cmd->vk_cmd_buffer, 0 },
      };

      VkSubmitInfo2 submit_info = {
        .sType                    = VK_STRUCTURE_TYPE_SUBMIT_INFO_2_KHR,
        .commandBufferInfoCount   = CRUDE_COUNTOF( command_buffers ),
        .pCommandBufferInfos      = command_buffers,
      };
    
      CRUDE_GFX_HANDLE_VULKAN_RESULT( gpu->vkQueueSubmit2KHR( gpu->vk_main_queue, 1, &submit_info, VK_NULL_HANDLE ), "Failed to sumbit queue" );
    }

    vkQueueWaitIdle( gpu->vk_main_queue);
    vmaDestroyBuffer( gpu->vma_allocator, staging_buffer, staging_allocation );

    vkResetCommandBuffer( cmd->vk_cmd_buffer, VK_COMMAND_BUFFER_RESET_RELEASE_RESOURCES_BIT );
  }

  texture->ready = true;

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
  shader_state->pipeline_type = CRUDE_GFX_PIPELINE_TYPE_GRAPHICS;
  shader_state->active_shaders = 0;
  
  for ( compiled_shaders = 0; compiled_shaders < creation->stages_count; ++compiled_shaders )
  {
    size_t temporary_allocator_marker = crude_stack_allocator_get_marker( gpu->temporary_allocator );
    crude_gfx_shader_stage const *stage = &creation->stages[ compiled_shaders ];
  
    if ( stage->type == VK_SHADER_STAGE_COMPUTE_BIT )
    {
      shader_state->pipeline_type = CRUDE_GFX_PIPELINE_TYPE_COMPUTE;
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
  
    CRUDE_ASSERTM( CRUDE_CHANNEL_GRAPHICS, shader_create_info.pCode && shader_create_info.codeSize, "Shader code contains an error or empty!" );

    if ( !shader_create_info.pCode || !shader_create_info.codeSize )
    {
      return CRUDE_GFX_SHADER_STATE_HANDLE_INVALID;
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
    ++render_pass->output.num_color_formats;
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
  crude_gfx_pipeline                                      *pipeline;
  crude_gfx_shader_state                                  *shader_state;
  VkDescriptorSetLayout                                    vk_layouts[ CRUDE_GFX_MAX_DESCRIPTOR_SET_LAYOUTS ];
  VkPipelineLayoutCreateInfo                               vk_pipeline_layout_info;
  crude_gfx_pipeline_handle                                pipeline_handle;
  crude_gfx_shader_state_handle                            shader_state_handle;

  pipeline_handle = crude_gfx_obtain_pipeline( gpu );
  if ( CRUDE_RESOURCE_HANDLE_IS_INVALID( pipeline_handle ) )
  {
    return pipeline_handle;
  }

  shader_state_handle = crude_gfx_create_shader_state( gpu, &creation->shaders );
  if ( CRUDE_RESOURCE_HANDLE_IS_INVALID( shader_state_handle ) )
  {
    crude_gfx_release_pipeline( gpu, pipeline_handle );
    return CRUDE_GFX_PIPELINE_HANDLE_INVALID;
  }

  shader_state = crude_gfx_access_shader_state( gpu, shader_state_handle );

  pipeline = crude_gfx_access_pipeline( gpu, pipeline_handle );
  
  pipeline->name = creation->name;
  pipeline->shader_state = shader_state_handle;
  
  for ( uint32 i = 0; i < shader_state->reflect.descriptor.sets_count; ++i )
  {
    /* First set for bindless */
    if ( i == CRUDE_GFX_BINDLESS_DESCRIPTOR_SET_INDEX )
    {
      crude_gfx_descriptor_set_layout *bindless_descriptor_set_layout = crude_gfx_access_descriptor_set_layout( gpu, gpu->bindless_descriptor_set_layout_handle );
      vk_layouts[ i ] = bindless_descriptor_set_layout->vk_descriptor_set_layout;
      pipeline->descriptor_set_layout_handle[ i ] = CRUDE_GFX_DESCRIPTOR_SET_LAYOUT_HANDLE_INVALID;
    }
    else
    {
      pipeline->descriptor_set_layout_handle[ i ] = crude_gfx_create_descriptor_set_layout( gpu, &shader_state->reflect.descriptor.sets[ i ] );
      crude_gfx_descriptor_set_layout *descriptor_set_layout = crude_gfx_access_descriptor_set_layout( gpu, pipeline->descriptor_set_layout_handle[ i ] );
      vk_layouts[ i ] = descriptor_set_layout->vk_descriptor_set_layout;
      pipeline->descriptor_set_layout[ i ] = descriptor_set_layout;
    }
  }

  vk_pipeline_layout_info = CRUDE_COMPOUNT_EMPTY( VkPipelineLayoutCreateInfo );
  vk_pipeline_layout_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
  vk_pipeline_layout_info.setLayoutCount = shader_state->reflect.descriptor.sets_count;
  vk_pipeline_layout_info.pSetLayouts = vk_layouts;
  CRUDE_GFX_HANDLE_VULKAN_RESULT( vkCreatePipelineLayout( gpu->vk_device, &vk_pipeline_layout_info, gpu->vk_allocation_callbacks, &pipeline->vk_pipeline_layout ), "Failed to create pipeline layout" );

  pipeline->num_active_layouts = shader_state->reflect.descriptor.sets_count;

  if ( shader_state->pipeline_type == CRUDE_GFX_PIPELINE_TYPE_GRAPHICS )
  {
    VkDynamicState                                         vk_dynamic_states[ ] = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };

    VkGraphicsPipelineCreateInfo                           vk_pipeline_info;
    VkViewport                                             vk_viewport;
    VkRect2D                                               vk_scissor;
    VkPipelineViewportStateCreateInfo                      vk_viewport_state;
    VkPipelineDynamicStateCreateInfo                       vk_dynamic_state;
    VkPipelineRenderingCreateInfoKHR                       vk_pipeline_rendering_create_info;
    VkPipelineRasterizationStateCreateInfo                 vk_rasterizer;
    VkPipelineMultisampleStateCreateInfo                   vk_multisampling;
    VkPipelineDepthStencilStateCreateInfo                  vk_depth_stencil;
    VkPipelineColorBlendStateCreateInfo                    vk_color_blending;
    VkPipelineInputAssemblyStateCreateInfo                 vk_input_assembly;
    VkPipelineVertexInputStateCreateInfo                   vk_vertex_input_info;
    VkVertexInputAttributeDescription                      vk_vertex_attributes[ 8 ];
    VkVertexInputBindingDescription                        vk_vertex_bindings[ 8 ];
    VkPipelineColorBlendAttachmentState                    vk_color_blend_attachment[ 8 ];
    crude_gfx_vertex_stream const                         *vertex_streams;
    crude_gfx_vertex_attribute const                      *vertex_attributes;
    uint32                                                 vertex_attributes_num, vertex_streams_num;

    vk_vertex_input_info = CRUDE_COMPOUNT_EMPTY( VkPipelineVertexInputStateCreateInfo );
    vk_vertex_input_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    
    if ( creation->relfect_vertex_input )
    {
      vertex_attributes = shader_state->reflect.input.vertex_attributes;
      vertex_streams = shader_state->reflect.input.vertex_streams;
      vertex_attributes_num = CRUDE_ARRAY_LENGTH( vertex_attributes );
      vertex_streams_num = CRUDE_ARRAY_LENGTH( vertex_streams );
      CRUDE_ASSERT( CRUDE_COUNTOF( vk_vertex_attributes ) >= vertex_attributes_num );
      CRUDE_ASSERT( CRUDE_COUNTOF( vk_vertex_bindings ) >= vertex_streams_num );
    }
    else
    {
      vertex_attributes = creation->vertex_attributes;
      vertex_streams = creation->vertex_streams;
      vertex_attributes_num = creation->vertex_attributes_num;
      vertex_streams_num = creation->vertex_streams_num;
    }

    if ( vertex_attributes_num )
    {
      for ( uint32 i = 0; i < vertex_attributes_num; ++i )
      {
        vk_vertex_attributes[ i ] = CRUDE_COMPOUNT_EMPTY( VkVertexInputAttributeDescription );
        vk_vertex_attributes[ i ].location = vertex_attributes[ i ].location;
        vk_vertex_attributes[ i ].binding = vertex_attributes[ i ].binding;
        vk_vertex_attributes[ i ].format = crude_gfx_to_vk_vertex_format( vertex_attributes[ i ].format );
        vk_vertex_attributes[ i ].offset = vertex_attributes[ i ].offset;
      }
      vk_vertex_input_info.vertexAttributeDescriptionCount = vertex_attributes_num;
      vk_vertex_input_info.pVertexAttributeDescriptions = vk_vertex_attributes;
    }
    else
    {
      vk_vertex_input_info.vertexAttributeDescriptionCount = 0;
      vk_vertex_input_info.pVertexAttributeDescriptions = NULL;
    }

    if ( vertex_streams_num )
    {
      for ( uint32 i = 0; i < vertex_streams_num; ++i )
      {
        vk_vertex_bindings[ i ] = CRUDE_COMPOUNT_EMPTY( VkVertexInputBindingDescription );
        vk_vertex_bindings[ i ].binding = vertex_streams[ i ].binding;
        vk_vertex_bindings[ i ].stride = vertex_streams[ i ].stride;
        vk_vertex_bindings[ i ].inputRate = vertex_streams[ i ].input_rate == CRUDE_GFX_VERTEX_INPUT_RATE_PER_VERTEX ? VK_VERTEX_INPUT_RATE_VERTEX : VK_VERTEX_INPUT_RATE_INSTANCE;
      }
      vk_vertex_input_info.vertexBindingDescriptionCount = vertex_streams_num;
      vk_vertex_input_info.pVertexBindingDescriptions = vk_vertex_bindings;
    }
    else
    {
      vk_vertex_input_info.vertexBindingDescriptionCount = 0;
      vk_vertex_input_info.pVertexBindingDescriptions = NULL;
    }
    
    vk_input_assembly = CRUDE_COMPOUNT_EMPTY( VkPipelineInputAssemblyStateCreateInfo );
    vk_input_assembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    vk_input_assembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    vk_input_assembly.primitiveRestartEnable = VK_FALSE;

    if ( creation->blend_state.active_states )
    {
      for ( uint32 i = 0; i < creation->blend_state.active_states; ++i )
      {
        crude_gfx_blend_state const *blend_state = &creation->blend_state.blend_states[i];
    
        vk_color_blend_attachment[i].colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
        vk_color_blend_attachment[i].blendEnable = blend_state->blend_enabled ? VK_TRUE : VK_FALSE;
        vk_color_blend_attachment[i].srcColorBlendFactor = blend_state->source_color;
        vk_color_blend_attachment[i].dstColorBlendFactor = blend_state->destination_color;
        vk_color_blend_attachment[i].colorBlendOp = blend_state->color_operation;
        
        if ( blend_state->separate_blend )
        {
          vk_color_blend_attachment[i].srcAlphaBlendFactor = blend_state->source_alpha;
          vk_color_blend_attachment[i].dstAlphaBlendFactor = blend_state->destination_alpha;
          vk_color_blend_attachment[i].alphaBlendOp = blend_state->alpha_operation;
        }
        else
        {
          vk_color_blend_attachment[i].srcAlphaBlendFactor = blend_state->source_color;
          vk_color_blend_attachment[i].dstAlphaBlendFactor = blend_state->destination_color;
          vk_color_blend_attachment[i].alphaBlendOp = blend_state->color_operation;
        }
      }
    }
    else
    {
      memset( &vk_color_blend_attachment[ 0 ], 0u, sizeof( vk_color_blend_attachment[ 0 ] ) );
      vk_color_blend_attachment[ 0 ].blendEnable = VK_FALSE;
      vk_color_blend_attachment[ 0 ].colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    }
    
    vk_color_blending = CRUDE_COMPOUNT_EMPTY( VkPipelineColorBlendStateCreateInfo );
    vk_color_blending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    vk_color_blending.logicOpEnable = VK_FALSE;
    vk_color_blending.logicOp = VK_LOGIC_OP_COPY;
    vk_color_blending.attachmentCount = creation->blend_state.active_states ? creation->blend_state.active_states : 1;
    vk_color_blending.pAttachments = vk_color_blend_attachment;
    vk_color_blending.blendConstants[ 0 ] = 0.0f;
    vk_color_blending.blendConstants[ 1 ] = 0.0f;
    vk_color_blending.blendConstants[ 2 ] = 0.0f;
    vk_color_blending.blendConstants[ 3 ] = 0.0f;
    
    vk_depth_stencil = CRUDE_COMPOUNT_EMPTY( VkPipelineDepthStencilStateCreateInfo );
    vk_depth_stencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    vk_depth_stencil.depthTestEnable = creation->depth_stencil.depth_enable ? VK_TRUE : VK_FALSE;
    vk_depth_stencil.depthWriteEnable = creation->depth_stencil.depth_write_enable ? VK_TRUE : VK_FALSE;
    vk_depth_stencil.depthCompareOp = creation->depth_stencil.depth_comparison;
    vk_depth_stencil.stencilTestEnable = creation->depth_stencil.stencil_enable ? VK_TRUE : VK_FALSE;
    
    if ( creation->depth_stencil.stencil_enable )
    {
      CRUDE_ABORT( CRUDE_CHANNEL_GRAPHICS, "TODO" );
    }
    
    vk_multisampling = CRUDE_COMPOUNT_EMPTY( VkPipelineMultisampleStateCreateInfo );
    vk_multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    vk_multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
    vk_multisampling.sampleShadingEnable = VK_FALSE;
    vk_multisampling.minSampleShading = 1.0f;
    vk_multisampling.pSampleMask = NULL;
    vk_multisampling.alphaToCoverageEnable = VK_FALSE;
    vk_multisampling.alphaToOneEnable = VK_FALSE;
    
    vk_rasterizer = CRUDE_COMPOUNT_EMPTY( VkPipelineRasterizationStateCreateInfo );
    vk_rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    vk_rasterizer.depthClampEnable = VK_FALSE;
    vk_rasterizer.rasterizerDiscardEnable = VK_FALSE;
    vk_rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
    vk_rasterizer.cullMode = CRUDE_STATIC_CAST( VkCullModeFlags, creation->rasterization.cull_mode );
    vk_rasterizer.frontFace = creation->rasterization.front;
    vk_rasterizer.depthBiasEnable = VK_FALSE;
    vk_rasterizer.depthBiasConstantFactor = 0.0f;
    vk_rasterizer.depthBiasClamp = 0.0f;
    vk_rasterizer.depthBiasSlopeFactor = 0.0f;
    vk_rasterizer.lineWidth = 1.0f;
    
    vk_viewport = CRUDE_COMPOUNT_EMPTY( VkViewport );
    vk_viewport.x = 0.0f;
    vk_viewport.y = 0.0f;
    vk_viewport.width = CRUDE_STATIC_CAST( float32, gpu->vk_swapchain_width );
    vk_viewport.height = CRUDE_STATIC_CAST( float32, gpu->vk_swapchain_height );
    vk_viewport.minDepth = 0.0f;
    vk_viewport.maxDepth = 1.0f;
    
    vk_scissor = CRUDE_COMPOUNT_EMPTY( VkRect2D );
    vk_scissor.offset = { 0, 0 };
    vk_scissor.extent = { gpu->vk_swapchain_width, gpu->vk_swapchain_height };

    vk_viewport_state = CRUDE_COMPOUNT_EMPTY( VkPipelineViewportStateCreateInfo ); 
    vk_viewport_state.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    vk_viewport_state.viewportCount = 1;
    vk_viewport_state.pViewports = &vk_viewport;
    vk_viewport_state.scissorCount = 1;
    vk_viewport_state.pScissors = &vk_scissor;
    
    vk_dynamic_state = CRUDE_COMPOUNT_EMPTY( VkPipelineDynamicStateCreateInfo ); 
    vk_dynamic_state.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    vk_dynamic_state.dynamicStateCount = CRUDE_COUNTOF( vk_dynamic_states );
    vk_dynamic_state.pDynamicStates = vk_dynamic_states;
    
    vk_pipeline_rendering_create_info = CRUDE_COMPOUNT_EMPTY( VkPipelineRenderingCreateInfoKHR );
    vk_pipeline_rendering_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO_KHR;
    vk_pipeline_rendering_create_info.viewMask = 0;
    vk_pipeline_rendering_create_info.colorAttachmentCount = creation->render_pass_output.num_color_formats;
    vk_pipeline_rendering_create_info.pColorAttachmentFormats = creation->render_pass_output.num_color_formats > 0 ? creation->render_pass_output.color_formats : NULL;
    vk_pipeline_rendering_create_info.depthAttachmentFormat = creation->render_pass_output.depth_stencil_format;
    vk_pipeline_rendering_create_info.stencilAttachmentFormat = VK_FORMAT_UNDEFINED;
    
    vk_pipeline_info = CRUDE_COMPOUNT_EMPTY( VkGraphicsPipelineCreateInfo );
    vk_pipeline_info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    vk_pipeline_info.pNext = &vk_pipeline_rendering_create_info;
    vk_pipeline_info.stageCount = shader_state->active_shaders;
    vk_pipeline_info.pStages = shader_state->shader_stage_info;
    vk_pipeline_info.pVertexInputState = &vk_vertex_input_info;
    vk_pipeline_info.pInputAssemblyState = &vk_input_assembly;
    vk_pipeline_info.pViewportState = &vk_viewport_state;
    vk_pipeline_info.pRasterizationState = &vk_rasterizer;
    vk_pipeline_info.pMultisampleState = &vk_multisampling;
    vk_pipeline_info.pDepthStencilState = &vk_depth_stencil;
    vk_pipeline_info.pColorBlendState = &vk_color_blending;
    vk_pipeline_info.pDynamicState = &vk_dynamic_state;
    vk_pipeline_info.layout = pipeline->vk_pipeline_layout;
    vk_pipeline_info.renderPass = NULL;
    
    CRUDE_GFX_HANDLE_VULKAN_RESULT( vkCreateGraphicsPipelines( gpu->vk_device, VK_NULL_HANDLE, 1, &vk_pipeline_info, gpu->vk_allocation_callbacks, &pipeline->vk_pipeline ), "Failed to create pipeline" );
    
    pipeline->vk_bind_point = VK_PIPELINE_BIND_POINT_GRAPHICS;
  }
  else if ( shader_state->pipeline_type == CRUDE_GFX_PIPELINE_TYPE_COMPUTE )
  {
    VkComputePipelineCreateInfo                            vk_pipeline_info;

    vk_pipeline_info = CRUDE_COMPOUNT_EMPTY( VkComputePipelineCreateInfo );
    vk_pipeline_info.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
    vk_pipeline_info.stage = shader_state->shader_stage_info[ 0 ];
    vk_pipeline_info.layout = pipeline->vk_pipeline_layout;

    CRUDE_GFX_HANDLE_VULKAN_RESULT( vkCreateComputePipelines( gpu->vk_device, VK_NULL_HANDLE, 1u, &vk_pipeline_info, gpu->vk_allocation_callbacks, &pipeline->vk_pipeline ), "Failed to create pipeline" );
    
    pipeline->vk_bind_point = VK_PIPELINE_BIND_POINT_COMPUTE;
  }

  return pipeline_handle;
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
      if ( i != CRUDE_GFX_BINDLESS_DESCRIPTOR_SET_INDEX )
      {
        crude_gfx_destroy_descriptor_set_layout( gpu, pipeline->descriptor_set_layout_handle[ i ] );
      }
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
    .size = creation->size > 0 ? creation->size : 1,
    .usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT | creation->type_flags,
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
  crude_gfx_descriptor_set_layout                       *descriptor_set_layout;
  crude_gfx_descriptor_set_layout_handle                 descriptor_set_layout_handle;
  VkDescriptorSetLayoutCreateInfo                        vk_layout_info;
  uint8                                                 *memory;
  uint32                                                 used_bindings;
  uint32                                                 tempory_allocator_marker;

  tempory_allocator_marker = crude_stack_allocator_get_marker( gpu->temporary_allocator );

  descriptor_set_layout_handle = crude_gfx_obtain_descriptor_set_layout( gpu );
  if ( CRUDE_RESOURCE_HANDLE_IS_INVALID( descriptor_set_layout_handle ) )
  {
    return descriptor_set_layout_handle;
  }
  
  descriptor_set_layout = crude_gfx_access_descriptor_set_layout( gpu, descriptor_set_layout_handle );
  
  memory = CRUDE_CAST( uint8*, CRUDE_ALLOCATE( gpu->allocator_container, ( sizeof( VkDescriptorSetLayoutBinding ) + sizeof( crude_gfx_descriptor_binding ) ) * creation->num_bindings ) );
  descriptor_set_layout->num_bindings = creation->num_bindings;
  descriptor_set_layout->bindings     = ( crude_gfx_descriptor_binding* )memory;
  descriptor_set_layout->vk_binding   = ( VkDescriptorSetLayoutBinding* )( memory + sizeof( crude_gfx_descriptor_binding ) * creation->num_bindings );
  descriptor_set_layout->handle       = descriptor_set_layout_handle;
  descriptor_set_layout->set_index    = creation->set_index;
  descriptor_set_layout->bindless = creation->bindless;
  
  used_bindings = 0;
  for ( uint32 i = 0; i < creation->num_bindings; ++i )
  {
    crude_gfx_descriptor_binding                        *binding;
    crude_gfx_descriptor_set_layout_binding const       *input_binding;
    VkDescriptorSetLayoutBinding                        *vk_binding;
  
    binding = &descriptor_set_layout->bindings[ i ];
    memset( binding, 0, sizeof( *binding ) );
  
    input_binding = &creation->bindings[ i ];
    binding->start = ( input_binding->start == UINT16_MAX ) ? i : input_binding->start;
    binding->count = input_binding->count;
    binding->type = input_binding->type;
    binding->name = input_binding->name;
    binding->set = descriptor_set_layout->set_index;
    
    vk_binding = &descriptor_set_layout->vk_binding[ used_bindings++ ];
    vk_binding->binding = binding->start;
    vk_binding->descriptorType = input_binding->type;
    vk_binding->descriptorType = vk_binding->descriptorType == VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER ? VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC : vk_binding->descriptorType;
    vk_binding->descriptorCount = input_binding->count;
    vk_binding->stageFlags = VK_SHADER_STAGE_ALL;
    vk_binding->pImmutableSamplers = NULL;
  }

  if ( creation->bindless )
  {
    VkDescriptorBindingFlags                              *vk_binding_flags;
    VkDescriptorSetLayoutBindingFlagsCreateInfoEXT         vk_extended_info;

    CRUDE_ARRAY_INITIALIZE_WITH_LENGTH( vk_binding_flags, creation->num_bindings, crude_stack_allocator_pack( gpu->temporary_allocator ) );
    for ( uint32 i = 0; i < creation->num_bindings; ++i )
    {
      vk_binding_flags[ i ] = VK_DESCRIPTOR_BINDING_UPDATE_AFTER_BIND_BIT_EXT;
    }

    vk_extended_info = CRUDE_COMPOUNT_EMPTY( VkDescriptorSetLayoutBindingFlagsCreateInfoEXT );
    vk_extended_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO_EXT;
    vk_extended_info.bindingCount = used_bindings;
    vk_extended_info.pBindingFlags = vk_binding_flags;

    vk_layout_info = CRUDE_COMPOUNT_EMPTY( VkDescriptorSetLayoutCreateInfo );
    vk_layout_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    vk_layout_info.pNext = &vk_extended_info;
    vk_layout_info.flags = VK_DESCRIPTOR_SET_LAYOUT_CREATE_UPDATE_AFTER_BIND_POOL_BIT_EXT;
    vk_layout_info.bindingCount = used_bindings;
    vk_layout_info.pBindings = descriptor_set_layout->vk_binding;

    CRUDE_GFX_HANDLE_VULKAN_RESULT( vkCreateDescriptorSetLayout( gpu->vk_device, &vk_layout_info, gpu->vk_allocation_callbacks, &descriptor_set_layout->vk_descriptor_set_layout ), "Failed create descriptor set layout" );
  }
  else
  {
    vk_layout_info = CRUDE_COMPOUNT_EMPTY( VkDescriptorSetLayoutCreateInfo );
    vk_layout_info.sType        = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    vk_layout_info.bindingCount = used_bindings;
    vk_layout_info.pBindings    = descriptor_set_layout->vk_binding;
    CRUDE_GFX_HANDLE_VULKAN_RESULT( vkCreateDescriptorSetLayout( gpu->vk_device, &vk_layout_info, gpu->vk_allocation_callbacks, &descriptor_set_layout->vk_descriptor_set_layout ), "Failed to create descriptor set layout" );
  }

  crude_stack_allocator_free_marker( gpu->temporary_allocator, tempory_allocator_marker );

  return descriptor_set_layout_handle;
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


crude_gfx_descriptor_set_handle
crude_gfx_create_descriptor_set
(
  _In_ crude_gfx_device                                   *gpu,
  _In_ crude_gfx_descriptor_set_creation const            *creation
)
{
  crude_gfx_descriptor_set_handle handle = crude_gfx_obtain_descriptor_set( gpu );
  if ( CRUDE_RESOURCE_HANDLE_IS_INVALID( handle ) )
  {
    return handle;
  }
  
  crude_gfx_descriptor_set *descriptor_set = crude_gfx_access_descriptor_set( gpu, handle );
  crude_gfx_descriptor_set_layout *descriptor_set_layout = crude_gfx_access_descriptor_set_layout( gpu, creation->layout );

  CRUDE_ASSERT( creation->name );
  descriptor_set->name = creation->name;
  
  VkDescriptorSetAllocateInfo vk_descriptor_info = {
    .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
    .descriptorPool = descriptor_set_layout->bindless ? gpu->vk_bindless_descriptor_pool : gpu->vk_descriptor_pool,
    .descriptorSetCount = 1u,
    .pSetLayouts = &descriptor_set_layout->vk_descriptor_set_layout
  };

  if ( descriptor_set_layout->bindless )
  {
    uint32 max_binding = CRUDE_GFX_MAX_BINDLESS_RESOURCES - 1;
    VkDescriptorSetVariableDescriptorCountAllocateInfoEXT count_info = CRUDE_COMPOUNT_EMPTY( VkDescriptorSetVariableDescriptorCountAllocateInfoEXT );
    count_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_VARIABLE_DESCRIPTOR_COUNT_ALLOCATE_INFO_EXT;
    count_info.descriptorSetCount = 1;
    count_info.pDescriptorCounts = &max_binding;
    vk_descriptor_info.pNext = &count_info;
    CRUDE_GFX_HANDLE_VULKAN_RESULT( vkAllocateDescriptorSets( gpu->vk_device, &vk_descriptor_info, &descriptor_set->vk_descriptor_set ) );
  }
  else
  {
    CRUDE_GFX_HANDLE_VULKAN_RESULT( vkAllocateDescriptorSets( gpu->vk_device, &vk_descriptor_info, &descriptor_set->vk_descriptor_set ), "Failed to allocate descriptor set: %s", creation->name ? creation->name : "#noname" );
  }

  VkWriteDescriptorSet descriptor_write[ 8 ];
  VkDescriptorBufferInfo buffer_info[ 8 ];
  VkDescriptorImageInfo image_info[ 8 ];

  uint32 num_resources = 0u;
  for ( uint32 i = 0; i < creation->num_resources; i++ )
  {
    crude_gfx_descriptor_binding const *binding = &descriptor_set_layout->bindings[ creation->bindings[ i ] ];
    
    if ( binding->type == VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER || binding->type == VK_DESCRIPTOR_TYPE_STORAGE_IMAGE )
    {
      continue;
    }

    descriptor_write[ i ].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptor_write[ i ].pNext = NULL;
    descriptor_write[ i ].dstSet = descriptor_set->vk_descriptor_set;
    descriptor_write[ i ].dstBinding = binding->start;
    descriptor_write[ i ].dstArrayElement = 0u;
    descriptor_write[ i ].descriptorCount = 1u;
    descriptor_write[ i ].descriptorType = binding->type;
    descriptor_write[ i ].pImageInfo = NULL;
    descriptor_write[ i ].pBufferInfo = NULL;
    descriptor_write[ i ].pTexelBufferView = NULL;

    switch ( binding->type )
    {
    case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER:
    {
      crude_gfx_buffer *buffer = crude_gfx_access_buffer( gpu, CRUDE_COMPOUNT( crude_gfx_buffer_handle, { creation->resources[ i ] } ) );
      CRUDE_ASSERT( buffer );
      
      descriptor_write[ i ].descriptorType = ( buffer->usage = CRUDE_GFX_RESOURCE_USAGE_TYPE_DYNAMIC ) ? VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC : VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;

      if ( CRUDE_RESOURCE_HANDLE_IS_VALID( buffer->parent_buffer ) )
      {
        crude_gfx_buffer *parent_buffer = crude_gfx_access_buffer( gpu, buffer->parent_buffer );
        buffer_info[ i ].buffer = parent_buffer->vk_buffer;
      }
      else
      {
        buffer_info[ i ].buffer = buffer->vk_buffer;
      }

      buffer_info[ i ].offset = 0;
      buffer_info[ i ].range = buffer->size;

      descriptor_write[ i ].pBufferInfo = &buffer_info[ i ];
      break;
    }
    case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER:
    {
      crude_gfx_buffer *buffer = crude_gfx_access_buffer( gpu, CRUDE_COMPOUNT( crude_gfx_buffer_handle, { creation->resources[ i ] } ) );
      CRUDE_ASSERT( buffer );
      
      descriptor_write[ i ].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;

      if ( CRUDE_RESOURCE_HANDLE_IS_VALID( buffer->parent_buffer ) )
      {
        crude_gfx_buffer *parent_buffer = crude_gfx_access_buffer( gpu, buffer->parent_buffer );
        buffer_info[ i ].buffer = parent_buffer->vk_buffer;
      }
      else
      {
        buffer_info[ i ].buffer = buffer->vk_buffer;
      }

      buffer_info[ i ].offset = 0;
      buffer_info[ i ].range = buffer->size;

      descriptor_write[ i ].pBufferInfo = &buffer_info[ i ];
      break;
    }
    }

    ++num_resources;
  }

  for ( uint32 i = 0; i < num_resources; i++ )
  {
    descriptor_set->resources[ i ] = creation->resources[ i ];
    descriptor_set->samplers[ i ] = creation->samplers[ i ];
    descriptor_set->bindings[ i ] = creation->bindings[ i ];
  }

  descriptor_set->layout = descriptor_set_layout;

  crude_gfx_set_resource_name( gpu, VK_OBJECT_TYPE_DESCRIPTOR_SET, CRUDE_CAST( uint64, descriptor_set->vk_descriptor_set ), creation->name );

  vkUpdateDescriptorSets( gpu->vk_device, num_resources, descriptor_write, 0, NULL );
  return handle;
}

void                                      
crude_gfx_destroy_descriptor_set
(                                                   
  _In_ crude_gfx_device                                   *gpu,
  _In_ crude_gfx_descriptor_set_handle                     handle
)
{
  if ( handle.index >= gpu->descriptor_sets.pool_size )
  {
    CRUDE_LOG_ERROR( CRUDE_CHANNEL_GRAPHICS, "Trying to free invalid descriptor set %u", handle.index );
    return;
  }
  crude_gfx_resource_update descriptor_set_update_event = { 
    .type          = CRUDE_GFX_RESOURCE_DELETION_TYPE_DESCRIPTOR_SET,
    .handle        = handle.index,
    .current_frame = gpu->current_frame };
  CRUDE_ARRAY_PUSH( gpu->resource_deletion_queue, descriptor_set_update_event );
}

void
crude_gfx_destroy_descriptor_set_instant
(                                                   
  _In_ crude_gfx_device                                   *gpu,
  _In_ crude_gfx_descriptor_set_handle                     handle
)
{
  crude_gfx_descriptor_set *descriptor_set = crude_gfx_access_descriptor_set( gpu, handle );

  //if ( descriptor_set->layout->bindless )
  //{
  //  vkFreeDescriptorSets( gpu->vk_device, gpu->vk_bindless_descriptor_pool, 1u, &descriptor_set->vk_descriptor_set );
  //}
  //else
  //{
  //  vkFreeDescriptorSets( gpu->vk_device, gpu->vk_descriptor_pool, 1u, &descriptor_set->vk_descriptor_set );
  //}

  crude_gfx_release_descriptor_set( gpu, handle );
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
  framebuffer->manual_resources_free = creation->manual_resources_free;
  
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
  if ( framebuffer && !framebuffer->manual_resources_free )
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
  return CRUDE_COMPOUNT( crude_gfx_sampler_handle, { crude_resource_pool_obtain_resource( &gpu->samplers ) } );
}

crude_gfx_sampler*
crude_gfx_access_sampler
(
  _In_ crude_gfx_device                                   *gpu,
  _In_ crude_gfx_sampler_handle                            handle
)
{
  return CRUDE_REINTERPRET_CAST( crude_gfx_sampler*, crude_resource_pool_access_resource( &gpu->samplers, handle.index ) );
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
  return CRUDE_COMPOUNT( crude_gfx_texture_handle, { crude_resource_pool_obtain_resource( &gpu->textures ) } );
}

crude_gfx_texture*
crude_gfx_access_texture
(
  _In_ crude_gfx_device                                   *gpu,
  _In_ crude_gfx_texture_handle                            handle
)
{
  return CRUDE_REINTERPRET_CAST( crude_gfx_texture*, crude_resource_pool_access_resource( &gpu->textures, handle.index ) );
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
  return CRUDE_COMPOUNT( crude_gfx_render_pass_handle, { crude_resource_pool_obtain_resource( &gpu->render_passes ) } );
}

crude_gfx_render_pass*
crude_gfx_access_render_pass
(
  _In_ crude_gfx_device                                   *gpu,
  _In_ crude_gfx_render_pass_handle                        handle
)
{
  return CRUDE_REINTERPRET_CAST( crude_gfx_render_pass*, crude_resource_pool_access_resource( &gpu->render_passes, handle.index ) );
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
  return CRUDE_COMPOUNT( crude_gfx_shader_state_handle, { crude_resource_pool_obtain_resource( &gpu->shaders ) } );
}

crude_gfx_shader_state*
crude_gfx_access_shader_state
(
  _In_ crude_gfx_device                                   *gpu,
  _In_ crude_gfx_shader_state_handle                       handle
)
{
  return CRUDE_REINTERPRET_CAST( crude_gfx_shader_state*, crude_resource_pool_access_resource( &gpu->shaders, handle.index ) );
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
  return CRUDE_COMPOUNT( crude_gfx_pipeline_handle,  { crude_resource_pool_obtain_resource( &gpu->pipelines ) } );
}

crude_gfx_pipeline*
crude_gfx_access_pipeline
(
  _In_ crude_gfx_device                                   *gpu,
  _In_ crude_gfx_pipeline_handle                           handle
)
{
  return CRUDE_REINTERPRET_CAST( crude_gfx_pipeline*, crude_resource_pool_access_resource( &gpu->pipelines, handle.index ) );
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
  return CRUDE_COMPOUNT( crude_gfx_buffer_handle, { crude_resource_pool_obtain_resource( &gpu->buffers ) } );
}

crude_gfx_buffer*
crude_gfx_access_buffer
(
  _In_ crude_gfx_device                                   *gpu,
  _In_ crude_gfx_buffer_handle                             handle
)
{
  return CRUDE_REINTERPRET_CAST( crude_gfx_buffer*, crude_resource_pool_access_resource( &gpu->buffers, handle.index ) );
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
  return CRUDE_COMPOUNT( crude_gfx_descriptor_set_layout_handle, { crude_resource_pool_obtain_resource( &gpu->descriptor_set_layouts ) } );
}

crude_gfx_descriptor_set_layout*
crude_gfx_access_descriptor_set_layout
(
  _In_ crude_gfx_device                                   *gpu,
  _In_ crude_gfx_descriptor_set_layout_handle              handle
)
{
  return CRUDE_REINTERPRET_CAST( crude_gfx_descriptor_set_layout*, crude_resource_pool_access_resource( &gpu->descriptor_set_layouts, handle.index ) );
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

crude_gfx_descriptor_set_handle
crude_gfx_obtain_descriptor_set
(
  _In_ crude_gfx_device                                   *gpu
)
{
  return CRUDE_COMPOUNT( crude_gfx_descriptor_set_handle, { crude_resource_pool_obtain_resource( &gpu->descriptor_sets ) } );
}

crude_gfx_descriptor_set*
crude_gfx_access_descriptor_set
(
  _In_ crude_gfx_device                                   *gpu,
  _In_ crude_gfx_descriptor_set_handle                     handle
)
{
  return CRUDE_REINTERPRET_CAST( crude_gfx_descriptor_set*, crude_resource_pool_access_resource( &gpu->descriptor_sets, handle.index ) );
}

void
crude_gfx_release_descriptor_set
(
  _In_ crude_gfx_device                                   *gpu,
  _In_ crude_gfx_descriptor_set_handle                     handle
)
{
  crude_resource_pool_release_resource( &gpu->descriptor_sets, handle.index );
}

crude_gfx_framebuffer_handle
crude_gfx_obtain_framebuffer
(
  _In_ crude_gfx_device                                   *gpu
)
{
  return CRUDE_COMPOUNT( crude_gfx_framebuffer_handle, { crude_resource_pool_obtain_resource( &gpu->framebuffers ) } );
}

crude_gfx_framebuffer*
crude_gfx_access_framebuffer
(
  _In_ crude_gfx_device                                   *gpu,
  _In_ crude_gfx_framebuffer_handle                        handle
)
{
  return CRUDE_REINTERPRET_CAST( crude_gfx_framebuffer*, crude_resource_pool_access_resource( &gpu->framebuffers, handle.index ) );
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
  VkValidationFeaturesEXT                                  validation_features;
  VkApplicationInfo                                        application;
  char const                                       *const *surface_extensions_array;
  char const                                             **instance_enabled_extensions;

  uint32                                                   surface_extensions_count;
  
  /* Get enabled extensions */ 
  surface_extensions_array = SDL_Vulkan_GetInstanceExtensions( &surface_extensions_count );

  CRUDE_ARRAY_INITIALIZE_WITH_CAPACITY( instance_enabled_extensions, surface_extensions_count, temporary_allocator );

  for ( uint32 i = 0; i < surface_extensions_count; ++i )
  {
    CRUDE_ARRAY_PUSH( instance_enabled_extensions, surface_extensions_array[ i ] );
  }
  
#ifdef CRUDE_GRAPHICS_VALIDATION_LAYERS_ENABLED
  {
    uint32 debug_extensions_count = CRUDE_COUNTOF( vk_instance_required_debug_extensions );
    for ( uint32 i = 0; i < debug_extensions_count; ++i )
    {
      CRUDE_ARRAY_PUSH( instance_enabled_extensions, vk_instance_required_debug_extensions[ i ] );
    }
  }
#endif /* CRUDE_GRAPHICS_VALIDATION_LAYERS_ENABLED */

  /* Setup application */ 
  application = CRUDE_COMPOUNT( VkApplicationInfo, {
    .pApplicationName   = vk_application_name,
    .applicationVersion = vk_application_version,
    .pEngineName        = "crude_engine",
    .engineVersion      = VK_MAKE_VERSION( 1, 0, 0 ),
    .apiVersion         = VK_API_VERSION_1_3 
  } );
  
  /* Initialize instance & debug_utils_messenger */ 
  instance_create_info = CRUDE_COMPOUNT_EMPTY( VkInstanceCreateInfo );
  instance_create_info.sType                    = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
  instance_create_info.pApplicationInfo         = &application;
  instance_create_info.flags                    = 0u;
  instance_create_info.ppEnabledExtensionNames  = instance_enabled_extensions;
  instance_create_info.enabledExtensionCount    = CRUDE_ARRAY_LENGTH( instance_enabled_extensions );
#ifdef CRUDE_GRAPHICS_VALIDATION_LAYERS_ENABLED
  instance_create_info.ppEnabledLayerNames     = vk_required_debug_layers;
  instance_create_info.enabledLayerCount       = CRUDE_COUNTOF( vk_required_debug_layers );
#ifdef VK_EXT_debug_utils
  debug_create_info = CRUDE_COMPOUNT_EMPTY( VkDebugUtilsMessengerCreateInfoEXT );
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

  VkValidationFeatureEnableEXT const features_requested[] = { VK_VALIDATION_FEATURE_ENABLE_GPU_ASSISTED_EXT, VK_VALIDATION_FEATURE_ENABLE_SYNCHRONIZATION_VALIDATION_EXT, VK_VALIDATION_FEATURE_ENABLE_BEST_PRACTICES_EXT };
  validation_features = CRUDE_COMPOUNT_EMPTY( VkValidationFeaturesEXT );
  validation_features.sType = VK_STRUCTURE_TYPE_VALIDATION_FEATURES_EXT;
  validation_features.pNext = &debug_create_info; 
  validation_features.enabledValidationFeatureCount = CRUDE_COUNTOF( features_requested );
  validation_features.pEnabledValidationFeatures = features_requested;

  instance_create_info.pNext = &validation_features;
#endif /* VK_EXT_debug_utils */
#endif /* CRUDE_GRAPHICS_VALIDATION_LAYERS_ENABLED */
  
  CRUDE_GFX_HANDLE_VULKAN_RESULT( vkCreateInstance( &instance_create_info, gpu->vk_allocation_callbacks, &gpu->vk_instance ), "failed to create instance" );
  
#ifdef CRUDE_GRAPHICS_VALIDATION_LAYERS_ENABLED
  gpu->vkCreateDebugUtilsMessengerEXT = ( PFN_vkCreateDebugUtilsMessengerEXT )vkGetInstanceProcAddr( gpu->vk_instance, "vkCreateDebugUtilsMessengerEXT" );
  gpu->vkDestroyDebugUtilsMessengerEXT = ( PFN_vkDestroyDebugUtilsMessengerEXT )vkGetInstanceProcAddr( gpu->vk_instance, "vkDestroyDebugUtilsMessengerEXT" );
#endif /* CRUDE_GRAPHICS_VALIDATION_LAYERS_ENABLED */
}

void
vk_create_debug_utils_messsenger_
(
  _In_ crude_gfx_device                                   *gpu
)
{
  VkDebugUtilsMessengerCreateInfoEXT create_info = CRUDE_COMPOUNT( VkDebugUtilsMessengerCreateInfoEXT, {
    .sType            = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT,
    .pNext            = NULL,
    .flags            = 0u,
    .messageSeverity  =
      VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
      VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT |
      VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
      VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT,
    .messageType      =
      VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
      VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
      VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT,
    .pfnUserCallback  = vk_debug_callback_,
    .pUserData        = NULL,
  } );
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
    return;
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

  gpu->vk_physical_device = VK_NULL_HANDLE;

  vkEnumeratePhysicalDevices( gpu->vk_instance, &available_physical_devices_count, NULL );
  
  if ( available_physical_devices_count == 0u ) 
  {
    return;
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
    return;
  }

  vkGetPhysicalDeviceProperties( selected_physical_devices, &selected_physical_properties );
  CRUDE_LOG_INFO( CRUDE_CHANNEL_GRAPHICS, "Selected physical device %s %i", selected_physical_properties.deviceName, selected_physical_properties.deviceType );

  {
    VkExtensionProperties                                   *available_extensions;
    uint32                                                   available_extensions_count;

    available_extensions_count = 0;
    vkEnumerateDeviceExtensionProperties( selected_physical_devices, nullptr, &available_extensions_count, nullptr );
    CRUDE_ARRAY_INITIALIZE_WITH_LENGTH( available_extensions, available_extensions_count, temporary_allocator );
    vkEnumerateDeviceExtensionProperties( selected_physical_devices, NULL, &available_extensions_count, available_extensions );

    for ( size_t i = 0; i < available_extensions_count; ++i )
    {
      if ( !strcmp( available_extensions[ i ].extensionName, VK_EXT_MESH_SHADER_EXTENSION_NAME ) )
      {
        gpu->mesh_shaders_extension_present = true;
        continue;
      }
    }
  }

  gpu->vk_physical_device = selected_physical_devices;
}

void
vk_create_device_
(
  _In_ crude_gfx_device                                   *gpu,
  _In_ crude_allocator_container                           temporary_allocator
)
{
  VkPhysicalDevice16BitStorageFeatures                     bit16_storage_features;
  VkPhysicalDevice8BitStorageFeatures                      bit_storage_features;
  VkPhysicalDeviceSynchronization2Features                 synchronization_features;
  VkPhysicalDeviceDynamicRenderingFeaturesKHR              dynamic_rendering_features;
  VkPhysicalDeviceDescriptorIndexingFeatures               indexing_features;
  VkPhysicalDeviceFeatures2                                physical_features2;
  VkPhysicalDeviceTimelineSemaphoreFeatures                timeline_semaphore_features;
  VkPhysicalDeviceMultiviewFeaturesKHR                     device_features_multivew;
  VkPhysicalDeviceFragmentShadingRateFeaturesKHR           device_features_fragment_shading_rate;
  VkPhysicalDeviceMeshShaderFeaturesEXT                    device_features_mesh;
  VkDeviceCreateInfo                                       device_create_info;
  VkDeviceQueueCreateInfo                                  queue_create_infos[ 2 ];
  VkQueueFamilyProperties                                 *queue_families;
  char const                                             **device_extensions;
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

  queue_create_infos[ 0 ] = CRUDE_COMPOUNT_EMPTY( VkDeviceQueueCreateInfo );
  queue_create_infos[ 0 ].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
  queue_create_infos[ 0 ].queueFamilyIndex = main_queue_index;
  queue_create_infos[ 0 ].queueCount = 1;
  queue_create_infos[ 0 ].pQueuePriorities = queue_priority;
  
  if ( transfer_queue_index < queue_family_count )
  {
    queue_create_infos[ 1 ] = CRUDE_COMPOUNT_EMPTY( VkDeviceQueueCreateInfo );
    queue_create_infos[ 1 ].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queue_create_infos[ 1 ].queueFamilyIndex = transfer_queue_index;
    queue_create_infos[ 1 ].queueCount = 1;
    queue_create_infos[ 1 ].pQueuePriorities = queue_priority;
  }

  bit16_storage_features = CRUDE_COMPOUNT_EMPTY( VkPhysicalDevice16BitStorageFeatures );
  bit16_storage_features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_16BIT_STORAGE_FEATURES;
  bit16_storage_features.storageBuffer16BitAccess = VK_TRUE;

  bit_storage_features = CRUDE_COMPOUNT_EMPTY( VkPhysicalDevice8BitStorageFeatures );
  bit_storage_features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_8BIT_STORAGE_FEATURES;
  bit_storage_features.pNext = &bit16_storage_features;
  bit_storage_features.storageBuffer8BitAccess = VK_TRUE;

  synchronization_features = CRUDE_COMPOUNT_EMPTY( VkPhysicalDeviceSynchronization2Features );
  synchronization_features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SYNCHRONIZATION_2_FEATURES;
  synchronization_features.pNext = &bit_storage_features;
  synchronization_features.synchronization2 = VK_TRUE;
  
  device_features_multivew = CRUDE_COMPOUNT_EMPTY( VkPhysicalDeviceMultiviewFeaturesKHR );
  device_features_multivew.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MULTIVIEW_FEATURES_KHR;
  device_features_multivew.pNext = &synchronization_features;
  device_features_multivew.multiview = VK_TRUE;

  device_features_fragment_shading_rate = CRUDE_COMPOUNT_EMPTY( VkPhysicalDeviceFragmentShadingRateFeaturesKHR );
  device_features_fragment_shading_rate.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FRAGMENT_SHADING_RATE_FEATURES_KHR;
  device_features_fragment_shading_rate.pNext = &device_features_multivew;
  device_features_fragment_shading_rate.primitiveFragmentShadingRate = VK_TRUE;

  device_features_mesh = CRUDE_COMPOUNT_EMPTY( VkPhysicalDeviceMeshShaderFeaturesEXT );
  device_features_mesh.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MESH_SHADER_FEATURES_EXT;
  device_features_mesh.pNext = &device_features_fragment_shading_rate;
  device_features_mesh.taskShader = VK_TRUE;
  device_features_mesh.meshShader = VK_TRUE;
  device_features_mesh.multiviewMeshShader = VK_TRUE;
  device_features_mesh.primitiveFragmentShadingRateMeshShader = VK_TRUE;

  timeline_semaphore_features = CRUDE_COMPOUNT_EMPTY( VkPhysicalDeviceTimelineSemaphoreFeatures );
  timeline_semaphore_features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_TIMELINE_SEMAPHORE_FEATURES;
  timeline_semaphore_features.pNext = &device_features_mesh;
  
  dynamic_rendering_features = CRUDE_COMPOUNT_EMPTY( VkPhysicalDeviceDynamicRenderingFeaturesKHR );
  dynamic_rendering_features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DYNAMIC_RENDERING_FEATURES_KHR;
  dynamic_rendering_features.pNext = &timeline_semaphore_features;

  indexing_features = CRUDE_COMPOUNT_EMPTY( VkPhysicalDeviceDescriptorIndexingFeatures );
  indexing_features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_INDEXING_FEATURES;
  indexing_features.pNext = &dynamic_rendering_features;

  physical_features2 = CRUDE_COMPOUNT_EMPTY( VkPhysicalDeviceFeatures2 );
  physical_features2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
  physical_features2.pNext = &indexing_features;
  vkGetPhysicalDeviceFeatures2( gpu->vk_physical_device, &physical_features2 );


  CRUDE_ARRAY_INITIALIZE_WITH_CAPACITY( device_extensions, CRUDE_COUNTOF( vk_device_required_extensions ), temporary_allocator );
  for ( uint32 i = 0; i < CRUDE_COUNTOF( vk_device_required_extensions ); ++i )
  {
    CRUDE_ARRAY_PUSH( device_extensions, vk_device_required_extensions[ i ] );
  }
  if ( gpu->mesh_shaders_extension_present )
  {
    CRUDE_ARRAY_PUSH( device_extensions, VK_EXT_MESH_SHADER_EXTENSION_NAME );
  }

  device_create_info = CRUDE_COMPOUNT_EMPTY( VkDeviceCreateInfo );
  device_create_info.sType                    = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
  device_create_info.pNext                    = &physical_features2;
  device_create_info.flags                    = 0;
  device_create_info.queueCreateInfoCount     = CRUDE_STATIC_CAST( uint32, transfer_queue_index < queue_family_count ? 2 : 1 );
  device_create_info.pQueueCreateInfos        = queue_create_infos;
#ifdef CRUDE_GRAPHICS_VALIDATION_LAYERS_ENABLED
  device_create_info.enabledLayerCount        = CRUDE_COUNTOF( vk_required_debug_layers );
  device_create_info.ppEnabledLayerNames      = vk_required_debug_layers;
#endif /* CRUDE_GRAPHICS_VALIDATION_LAYERS_ENABLED */
  device_create_info.enabledExtensionCount    = CRUDE_ARRAY_LENGTH( device_extensions );
  device_create_info.ppEnabledExtensionNames  = device_extensions;
  device_create_info.pEnabledFeatures         = NULL;

  CRUDE_GFX_HANDLE_VULKAN_RESULT( vkCreateDevice( gpu->vk_physical_device, &device_create_info, gpu->vk_allocation_callbacks, &gpu->vk_device ), "failed to create logic device!" );
  vkGetDeviceQueue( gpu->vk_device, main_queue_index, 0u, &gpu->vk_main_queue );
  if ( transfer_queue_index < queue_family_count )
  {
    vkGetDeviceQueue( gpu->vk_device, transfer_queue_index, 0, &gpu->vk_transfer_queue );
  }
  
  gpu->vkCmdDrawMeshTasksIndirectCountEXT = ( PFN_vkCmdDrawMeshTasksIndirectCountEXT )vkGetDeviceProcAddr( gpu->vk_device, "vkCmdDrawMeshTasksIndirectCountEXT" );
  gpu->vkCmdDrawMeshTasksEXT = ( PFN_vkCmdDrawMeshTasksEXT )vkGetDeviceProcAddr( gpu->vk_device, "vkCmdDrawMeshTasksEXT" );
  gpu->vkCmdBeginRenderingKHR = ( PFN_vkCmdBeginRenderingKHR )vkGetDeviceProcAddr( gpu->vk_device, "vkCmdBeginRenderingKHR" );
  gpu->vkCmdEndRenderingKHR = ( PFN_vkCmdEndRenderingKHR )vkGetDeviceProcAddr( gpu->vk_device, "vkCmdEndRenderingKHR" );
  gpu->vkSetDebugUtilsObjectNameEXT = ( PFN_vkSetDebugUtilsObjectNameEXT )vkGetDeviceProcAddr( gpu->vk_device, "vkSetDebugUtilsObjectNameEXT" );
  gpu->vkCmdPipelineBarrier2KHR = ( PFN_vkCmdPipelineBarrier2KHR )vkGetDeviceProcAddr( gpu->vk_device, "vkCmdPipelineBarrier2KHR" );
  gpu->vkQueueSubmit2KHR = ( PFN_vkQueueSubmit2KHR )vkGetDeviceProcAddr( gpu->vk_device, "vkQueueSubmit2KHR" );
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
  }
  
  available_present_modes_count;
  vkGetPhysicalDeviceSurfacePresentModesKHR( gpu->vk_physical_device, gpu->vk_surface, &available_present_modes_count, NULL );
  if ( available_present_modes_count == 0u ) 
  {
    CRUDE_LOG_ERROR( CRUDE_CHANNEL_GRAPHICS, "Can't find available surface present_mode" );
    CRUDE_ARRAY_DEINITIALIZE( available_formats );
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
  
  swapchain_create_info = CRUDE_COMPOUNT( VkSwapchainCreateInfoKHR, {
    .sType                  = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
    .pNext                  = NULL,
    .surface                = gpu->vk_surface,
    .minImageCount          = image_count,
    .imageFormat            = gpu->vk_surface_format.format,
    .imageColorSpace        = gpu->vk_surface_format.colorSpace,
    .imageExtent            = swapchain_extent,
    .imageArrayLayers       = 1,
    .imageUsage             = VK_IMAGE_USAGE_TRANSFER_DST_BIT,
    .imageSharingMode       = CRUDE_COUNTOF( queue_family_indices ) > 1 ? VK_SHARING_MODE_CONCURRENT : VK_SHARING_MODE_EXCLUSIVE,
    .queueFamilyIndexCount  = CRUDE_COUNTOF( queue_family_indices ),
    .pQueueFamilyIndices    = queue_family_indices,
    .preTransform           = surface_capabilities.currentTransform,
    .compositeAlpha         = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
    .presentMode            = selected_present_mode,
    .clipped                = true,
    .oldSwapchain           = VK_NULL_HANDLE,
  } );
  
  CRUDE_GFX_HANDLE_VULKAN_RESULT( vkCreateSwapchainKHR( gpu->vk_device, &swapchain_create_info, gpu->vk_allocation_callbacks, &gpu->vk_swapchain ), "Failed to create swapchain!" );

  vkGetSwapchainImagesKHR( gpu->vk_device, gpu->vk_swapchain, &gpu->vk_swapchain_images_count, NULL );
  vkGetSwapchainImagesKHR( gpu->vk_device, gpu->vk_swapchain, &gpu->vk_swapchain_images_count, &gpu->vk_swapchain_images[0] );

  VkCommandBufferBeginInfo beginInfo = { 
    .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
    .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT
  };
  
  crude_gfx_cmd_buffer *cmd = crude_gfx_get_primary_cmd( gpu, 0, false );
  vkBeginCommandBuffer( cmd->vk_cmd_buffer, &beginInfo );
  for ( size_t i = 0; i < gpu->vk_swapchain_images_count; ++i )
  {
    crude_gfx_cmd_add_image_barrier_ext2( cmd, gpu->vk_swapchain_images[ i ], CRUDE_GFX_RESOURCE_STATE_UNDEFINED, CRUDE_GFX_RESOURCE_STATE_PRESENT, 0, 1, false );
  }
  vkEndCommandBuffer( cmd->vk_cmd_buffer );

  {
    VkCommandBufferSubmitInfo command_buffers[] = {
      { VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO_KHR, NULL, cmd->vk_cmd_buffer, 0 },
    };
  
    VkSubmitInfo2 submit_info = {
      .sType                    = VK_STRUCTURE_TYPE_SUBMIT_INFO_2_KHR,
      .commandBufferInfoCount   = CRUDE_COUNTOF( command_buffers ),
      .pCommandBufferInfos      = command_buffers,
    };
  
    CRUDE_GFX_HANDLE_VULKAN_RESULT( gpu->vkQueueSubmit2KHR( gpu->vk_main_queue, 1, &submit_info, VK_NULL_HANDLE ), "Failed to sumbit queue" );
  }

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
  {
    VkDescriptorPoolCreateInfo                               pool_info;
    uint32                                                   pool_count;
    VkDescriptorSetLayoutBinding                             vk_binding[ 2 ];
    VkDescriptorBindingFlags                                 bindless_flags;

    {
      VkDescriptorPoolSize pool_sizes[] =
      {
        { VK_DESCRIPTOR_TYPE_SAMPLER, 10 },
        { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 10 },
        { VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 10 },
        { VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 10 },
        { VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 10 },
        { VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 10 },
        { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 10 },
        { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 10 },
        { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 10 },
        { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 10 },
        { VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 10 }
      };
      pool_info = CRUDE_COMPOUNT_EMPTY( VkDescriptorPoolCreateInfo );
      pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
      pool_info.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
      pool_info.maxSets = 4096;
      pool_info.poolSizeCount = CRUDE_COUNTOF( pool_sizes );
      pool_info.pPoolSizes = pool_sizes;
      CRUDE_GFX_HANDLE_VULKAN_RESULT( vkCreateDescriptorPool( gpu->vk_device, &pool_info, gpu->vk_allocation_callbacks, &gpu->vk_descriptor_pool ), "Failed create descriptor pool" );
    }
    {
      VkDescriptorPoolSize pool_sizes_bindless[] = {
        { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, CRUDE_GFX_MAX_BINDLESS_RESOURCES },
        { VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, CRUDE_GFX_MAX_BINDLESS_RESOURCES }
      };
      pool_info = CRUDE_COMPOUNT_EMPTY( VkDescriptorPoolCreateInfo );
      pool_info.sType         = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
      pool_info.flags         = VK_DESCRIPTOR_POOL_CREATE_UPDATE_AFTER_BIND_BIT_EXT | VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
      pool_info.maxSets       = CRUDE_GFX_MAX_BINDLESS_RESOURCES * CRUDE_COUNTOF( pool_sizes_bindless );
      pool_info.poolSizeCount = CRUDE_COUNTOF( pool_sizes_bindless );
      pool_info.pPoolSizes    = pool_sizes_bindless;
      CRUDE_GFX_HANDLE_VULKAN_RESULT( vkCreateDescriptorPool( gpu->vk_device, &pool_info, gpu->vk_allocation_callbacks, &gpu->vk_bindless_descriptor_pool ), "Failed create descriptor pool" );
    }
  }
  {
    crude_gfx_descriptor_set_layout_creation creation = CRUDE_COMPOUNT_EMPTY( crude_gfx_descriptor_set_layout_creation );
    creation.name = "bindless_descriptor_set_layout";
    creation.bindless = true;
    creation.set_index = 0u;
    crude_gfx_descriptor_set_layout_creation_add_binding( &creation, CRUDE_COMPOUNT( crude_gfx_descriptor_set_layout_binding, {
      .type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
      .start = CRUDE_GFX_BINDLESS_TEXTURE_BINDING,
      .count = CRUDE_GFX_MAX_BINDLESS_RESOURCES,
      .name = "",
    } ) );
    crude_gfx_descriptor_set_layout_creation_add_binding( &creation, CRUDE_COMPOUNT( crude_gfx_descriptor_set_layout_binding, {
      .type = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
      .start = CRUDE_GFX_BINDLESS_TEXTURE_BINDING + 1,
      .count = CRUDE_GFX_MAX_BINDLESS_RESOURCES,
      .name = "",
    } ) );
    gpu->bindless_descriptor_set_layout_handle = crude_gfx_create_descriptor_set_layout( gpu, &creation );
  }
  {
    crude_gfx_descriptor_set_creation creation = crude_gfx_descriptor_set_creation_empty();
    creation.name = "bindless_descriptor_set";
    creation.layout = gpu->bindless_descriptor_set_layout_handle;
    gpu->bindless_descriptor_set_handle = crude_gfx_create_descriptor_set( gpu, &creation );
  }
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
  for ( uint32 i = 0; i < CRUDE_COUNTOF( vk_device_required_extensions ); ++i )
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

    image_info = CRUDE_COMPOUNT( VkImageCreateInfo, {
      .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
      .imageType = crude_gfx_to_vk_image_type( creation->type ),
      .format = texture->vk_format,
      .extent = {
        .width = creation->width,
        .height = creation->height,
        .depth = creation->depth,
      },
      .mipLevels = creation->mipmaps,
      .arrayLayers = 1,
      .samples = VK_SAMPLE_COUNT_1_BIT,
      .tiling = VK_IMAGE_TILING_OPTIMAL,
      .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
      .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED
    } );

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

    memory_info = CRUDE_COMPOUNT( VmaAllocationCreateInfo, { .usage = VMA_MEMORY_USAGE_GPU_ONLY } );
    
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
    
    crude_gfx_set_resource_name( gpu, VK_OBJECT_TYPE_IMAGE, CRUDE_CAST( uint64, texture->vk_image ), creation->name );
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
    crude_gfx_set_resource_name( gpu, VK_OBJECT_TYPE_IMAGE_VIEW, CRUDE_CAST( uint64, texture->vk_image_view ), creation->name );
  }

  {
    crude_gfx_resource_update texture_update_event = { 
      .type          = CRUDE_GFX_RESOURCE_DELETION_TYPE_TEXTURE,
      .handle        = handle.index,
      .current_frame = gpu->current_frame
    };
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
    case SPV_REFLECT_FORMAT_R32_SINT            : return CRUDE_GFX_VERTEX_COMPONENT_FORMAT_UINT;
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
  _In_ void const                                         *code,
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
    CRUDE_ARRAY_INITIALIZE_WITH_CAPACITY( reflect->input.vertex_attributes, spv_reflect.input_variable_count, gpu->allocator_container );
    CRUDE_ARRAY_INITIALIZE_WITH_CAPACITY( reflect->input.vertex_streams, spv_reflect.input_variable_count, gpu->allocator_container );

    for ( uint32 input_index = 0; input_index < spv_reflect.input_variable_count; ++input_index )
    {
      SpvReflectInterfaceVariable const                   *spv_input;
      uint32                                               stride;

      spv_input = spv_reflect.input_variables[ input_index ];
      
      if ( spv_input->decoration_flags & SPV_REFLECT_DECORATION_BUILT_IN )
      {
        continue;
      }

      CRUDE_ARRAY_PUSH( reflect->input.vertex_attributes, CRUDE_COMPOUNT( crude_gfx_vertex_attribute, {
        .location = spv_input->location,
        .binding = spv_input->location,
        .offset = 0,
        .format = reflect_format_to_vk_format_( spv_input->format )
      } ) );
      
      stride = ( spv_input->numeric.vector.component_count * spv_input->numeric.scalar.width ) / 8;
      CRUDE_ARRAY_PUSH( reflect->input.vertex_streams, CRUDE_COMPOUNT( crude_gfx_vertex_stream, {
        .binding = spv_input->location,
        .stride = stride,
        .input_rate = CRUDE_GFX_VERTEX_INPUT_RATE_PER_VERTEX
      } ) );
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
        case SPV_REFLECT_DESCRIPTOR_TYPE_STORAGE_BUFFER:
        {
          reflect->descriptor.sets_count = crude_max( reflect->descriptor.sets_count, ( spv_descriptor_set->set + 1 ) );
          binding->type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
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
      crude_gfx_destroy_sampler_instant( gpu, CRUDE_COMPOUNT( crude_gfx_sampler_handle, { handle } ) );
      break;
    }
    case CRUDE_GFX_RESOURCE_DELETION_TYPE_TEXTURE:
    {
      crude_gfx_destroy_texture_instant( gpu, CRUDE_COMPOUNT( crude_gfx_texture_handle, { handle } ) );
      break;
    }
    case CRUDE_GFX_RESOURCE_DELETION_TYPE_RENDER_PASS:
    {
      crude_gfx_destroy_render_pass_instant( gpu, CRUDE_COMPOUNT( crude_gfx_render_pass_handle, { handle } ) );
      break;
    }
    case CRUDE_GFX_RESOURCE_DELETION_TYPE_SHADER_STATE:
    {
      crude_gfx_destroy_shader_state_instant( gpu, CRUDE_COMPOUNT( crude_gfx_shader_state_handle, { handle } ) );
      break;
    }
    case CRUDE_GFX_RESOURCE_DELETION_TYPE_PIPELINE:
    {
      crude_gfx_destroy_pipeline_instant( gpu, CRUDE_COMPOUNT( crude_gfx_pipeline_handle, { handle } ) );
      break;
    }
    case CRUDE_GFX_RESOURCE_DELETION_TYPE_BUFFER:
    {
      crude_gfx_destroy_buffer_instant( gpu, CRUDE_COMPOUNT( crude_gfx_buffer_handle, { handle } ) );
      break;
    }
    case CRUDE_GFX_RESOURCE_DELETION_TYPE_DESCRIPTOR_SET_LAYOUT:
    {
      crude_gfx_destroy_descriptor_set_layout_instant( gpu, CRUDE_COMPOUNT( crude_gfx_descriptor_set_layout_handle, { handle } ) );
      break;
    }
    case CRUDE_GFX_RESOURCE_DELETION_TYPE_DESCRIPTOR_SET:
    {
      crude_gfx_destroy_descriptor_set_instant( gpu, CRUDE_COMPOUNT( crude_gfx_descriptor_set_handle, { handle } ) );
      break;
    }
    case CRUDE_GFX_RESOURCE_DELETION_TYPE_FRAMEBUFFER:
    {
      crude_gfx_destroy_framebuffer_instant( gpu, CRUDE_COMPOUNT( crude_gfx_framebuffer_handle, { handle } ) );
      break;
    }
  }
}