#include <engine/core/log.h>
#include <engine/core/assert.h>
#include <engine/core/array.h>
#include <engine/core/string.h>
#include <engine/core/file.h>
#include <engine/core/process.h>

#include <engine/graphics/rhi.h>

#if CRUDE_GFX_VULKAN
#define CRUDE_GFX_RHI_HANDLE_VULKAN_RESULT( result, ... )\
{\
  if ( result != VK_SUCCESS )\
  {\
    CRUDE_ABORT( CRUDE_CHANNEL_GRAPHICS, "vulkan result isn't success: %i %s", result, ##__VA_ARGS__ );\
  }\
}
#elif CRUDE_GFX_DX12
#define CRUDE_GFX_RHI_HANDLE_DX12_RESULT( result, ... )\
{\
  if ( FAILED( result ) )\
  {\
    CRUDE_ABORT( CRUDE_CHANNEL_GRAPHICS, "dx12 result isn't success: %i %s", result, ##__VA_ARGS__ );\
  }\
}
#endif

char const*
crude_gfx_rhi_resource_state_to_name
(
  _In_ crude_gfx_rhi_resource_state                        value
)
{
  switch ( value )
  {
    case CRUDE_GFX_RHI_RESOURCE_STATE_UNDEFINED:                          return "Undefined";
    case CRUDE_GFX_RHI_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER:         return "Vertex And Constant";
    case CRUDE_GFX_RHI_RESOURCE_STATE_INDEX_BUFFER:                       return "Index Buffer";
    case CRUDE_GFX_RHI_RESOURCE_STATE_RENDER_TARGET:                      return "Render Target";
    case CRUDE_GFX_RHI_RESOURCE_STATE_UNORDERED_ACCESS:                   return "UAV";
    case CRUDE_GFX_RHI_RESOURCE_STATE_DEPTH_WRITE:                        return "Depth Write";
    case CRUDE_GFX_RHI_RESOURCE_STATE_DEPTH_READ:                         return "Depth Read";
    case CRUDE_GFX_RHI_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE:          return "Non Pixel Shader Resource";
    case CRUDE_GFX_RHI_RESOURCE_STATE_PIXEL_SHADER_RESOURCE:              return "Pixel Shader Resource";
    case CRUDE_GFX_RHI_RESOURCE_STATE_SHADER_RESOURCE:                    return "Shader Resource";
    case CRUDE_GFX_RHI_RESOURCE_STATE_STREAM_OUT:                         return "Stream Out";
    case CRUDE_GFX_RHI_RESOURCE_STATE_INDIRECT_ARGUMENT:                  return "Indirect Argument";
    case CRUDE_GFX_RHI_RESOURCE_STATE_COPY_DEST:                          return "Copy Dest";
    case CRUDE_GFX_RHI_RESOURCE_STATE_COPY_SOURCE:                        return "Copy Source";
    case CRUDE_GFX_RHI_RESOURCE_STATE_GENERIC_READ:                       return "Generic Read";
    case CRUDE_GFX_RHI_RESOURCE_STATE_PRESENT:                            return "Present";
    case CRUDE_GFX_RHI_RESOURCE_STATE_COMMON:                             return "Common";
    case CRUDE_GFX_RHI_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE:  return "Raytracing";
    case CRUDE_GFX_RHI_RESOURCE_STATE_SHADING_RATE_SOURCE:                return "Shading Rate";
  }
  return "UnknownState";
}

#if CRUDE_GFX_VULKAN

#if CRUDE_GRAPHICS_VALIDATION_LAYERS_ENABLED
static char const *const vk_instance_required_debug_extensions[] =
{
  VK_EXT_DEBUG_UTILS_EXTENSION_NAME
};

static char const *const vk_required_debug_layers[] =
{
  "VK_LAYER_KHRONOS_validation"
};

static VkValidationFeatureEnableEXT const vk_features_requested[ ] =
{ 
  VK_VALIDATION_FEATURE_ENABLE_BEST_PRACTICES_EXT,
#if CRUDE_GFX_SYNCHRONIZATION_VALIDATION_ENABLE
  VK_VALIDATION_FEATURE_ENABLE_SYNCHRONIZATION_VALIDATION_EXT,
#endif
#if CRUDE_GFX_GPU_AV_ENABLE
  VK_VALIDATION_FEATURE_ENABLE_GPU_ASSISTED_EXT,
#endif
  //VK_VALIDATION_FEATURE_ENABLE_DEBUG_PRINTF_EXT,
};
#endif /* CRUDE_GRAPHICS_VALIDATION_LAYERS_ENABLED */

static char const *const crude_gfx_rhi_vk_device_required_extensions[] = 
{ 
  VK_KHR_SWAPCHAIN_EXTENSION_NAME,
#if CRUDE_GFX_NSIGHT_AFTERMATH
  VK_NV_DEVICE_DIAGNOSTIC_CHECKPOINTS_EXTENSION_NAME,
  VK_NV_DEVICE_DIAGNOSTICS_CONFIG_EXTENSION_NAME,
#endif /* CRUDE_GFX_NSIGHT_AFTERMATH */
#if CRUDE_GFX_RAY_TRACING_ENABLED
  VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME,
  VK_KHR_RAY_TRACING_PIPELINE_EXTENSION_NAME,
  VK_KHR_RAY_QUERY_EXTENSION_NAME,
  VK_KHR_RAY_TRACING_POSITION_FETCH_EXTENSION_NAME,
//#if  CRUDE_GRAPHICS_VALIDATION_LAYERS_ENABLED 
//  VK_NV_RAY_TRACING_VALIDATION_EXTENSION_NAME
//#endif /* CRUDE_GRAPHICS_VALIDATION_LAYERS_ENABLED */
#endif /* CRUDE_GFX_RAY_TRACING_ENABLED */
};

VkDescriptorPoolSize crude_gfx_rhi_vk_pool_sizes_bindless_[ ] =
{
  { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, CRUDE_GFX_BINDLESS_RESOURCES_MAX_COUNT },
  { VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, CRUDE_GFX_BINDLESS_RESOURCES_MAX_COUNT }
};

VkDescriptorPoolSize crude_gfx_rhi_vk_pool_sizes_[] =
{
  { VK_DESCRIPTOR_TYPE_SAMPLER, 100 },
  { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 100 },
  { VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 100 },
  { VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 100 },
  { VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 100 },
  { VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 100 },
  { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 100 },
  { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 11000 },
  { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 100 },
  { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 100 },
  { VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 11000 },
#if CRUDE_GFX_RAY_TRACING_ENABLED
  { VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR, 100 },
#endif
};

static VKAPI_ATTR VkBool32
crude_gfx_rhi_vk_debug_callback_
(
  _In_ VkDebugUtilsMessageSeverityFlagBitsEXT              messageSeverity,
  _In_ VkDebugUtilsMessageTypeFlagsEXT                     messageType,
  _In_ VkDebugUtilsMessengerCallbackDataEXT const         *pCallbackData,
  _In_ void                                               *pUserData
);

static bool
crude_gfx_rhi_vk_pick_physical_device_
(
  _In_ VkInstance                                          vk_instance,
  _In_ VkSurfaceKHR                                        vk_surface,
  _In_ crude_heap_allocator                               *allocator,
  _Out_ VkPhysicalDevice                                  *vk_selected_physical_devices,
  _Out_ crude_gfx_rhi_physical_device_optional_extensions *vk_selected_physical_devices_optional_extenstions
);

static int32
crude_gfx_rhi_vk_get_supported_queue_family_index_
(
  _In_ VkPhysicalDevice                                    vk_physical_device,
  _In_ VkSurfaceKHR                                        vk_surface,
  _In_ crude_heap_allocator                               *allocator
);

static bool
crude_gfx_rhi_vk_check_support_required_extensions_
(
  _In_ VkPhysicalDevice                                    vk_physical_device,
  _In_ crude_heap_allocator                               *allocator,
  _Out_opt_ char const                                   **not_supported_extension_name
);

static bool
crude_gfx_rhi_vk_check_swapchain_adequate_
(
  _In_ VkPhysicalDevice                                    vk_physical_device,
  _In_ VkSurfaceKHR                                        vk_surface
);

static bool
crude_gfx_rhi_vk_check_support_required_features_
(
  _In_ VkPhysicalDevice                                    vk_physical_device
);

#elif /* CRUDE_GFX_VULKAN */ CRUDE_GFX_DX12

#endif /* CRUDE_GFX_DX12 */

#if CRUDE_GFX_VULKAN

crude_gfx_rhi_fence
crude_gfx_rhi_fence_empty
(
)
{
  crude_gfx_rhi_fence                                      empty_fence;
  empty_fence.vk_fence = VK_NULL_HANDLE;
  return empty_fence;
}

crude_gfx_rhi_sampler
crude_gfx_rhi_sampler_empty
(
)
{
  crude_gfx_rhi_sampler                                    empty_sampler;
  empty_sampler.vk_sampler = VK_NULL_HANDLE;
  return empty_sampler;
}

crude_gfx_rhi_queue
crude_gfx_rhi_queue_empty
(
)
{
  crude_gfx_rhi_queue                                      queue;
  queue.vk_queue = VK_NULL_HANDLE;
  queue.vk_queue_family = VK_QUEUE_FAMILY_IGNORED;
  return queue;
}

crude_gfx_rhi_image_copy
crude_gfx_rhi_image_copy_empty
(
)
{
  crude_gfx_rhi_image_copy copy = CRUDE_COMPOUNT_EMPTY( crude_gfx_rhi_image_copy );
  return copy;
}

crude_gfx_rhi_viewport
crude_gfx_rhi_viewport_empty
(
)
{
  crude_gfx_rhi_viewport viewport = CRUDE_COMPOUNT_EMPTY( crude_gfx_rhi_viewport );
  return viewport;
}

bool
crude_gfx_rhi_format_has_depth_or_stencil
(
  _In_ crude_gfx_rhi_format                                    value
)
{
  return value >= CRUDE_GFX_RHI_FORMAT_D16_UNORM && value <= CRUDE_GFX_RHI_FORMAT_D32_SFLOAT_S8_UINT;
}

bool
crude_gfx_rhi_format_has_depth
(
  _In_ crude_gfx_rhi_format                                    value
)
{
  return ( value >= CRUDE_GFX_RHI_FORMAT_D16_UNORM && value < CRUDE_GFX_RHI_FORMAT_S8_UINT ) || ( value >= CRUDE_GFX_RHI_FORMAT_D16_UNORM_S8_UINT && value <= CRUDE_GFX_RHI_FORMAT_D32_SFLOAT_S8_UINT );
}

VkClearValue
crude_gfx_rhi_clear_value_to_vk_clear_value
(
  _In_ crude_gfx_rhi_clear_value                           clear_value
)
{
  VkClearValue                                             vk_clear_value;

  vk_clear_value.color.int32[ 0 ] = clear_value.color.int32[ 0 ];
  vk_clear_value.color.int32[ 1 ] = clear_value.color.int32[ 1 ];
  vk_clear_value.color.int32[ 2 ] = clear_value.color.int32[ 2 ];
  vk_clear_value.color.int32[ 3 ] = clear_value.color.int32[ 3 ];
  return vk_clear_value;
}

VkRect2D
crude_gfx_rhi_rect_2d_to_vk_rect_2d
(
  _In_ crude_gfx_rhi_rect_2d                               rect2d
)
{
  VkRect2D                                                 vk_rect;

  vk_rect.extent.width = rect2d.extent.x;
  vk_rect.extent.height = rect2d.extent.y;
  vk_rect.offset.x = rect2d.offset.x;
  vk_rect.offset.y = rect2d.offset.y;
  return vk_rect;
}

crude_gfx_rhi_access_flags
crude_gfx_rhi_resource_state_to_access_flags
(
  _In_ crude_gfx_rhi_resource_state                        state
)
{
  VkAccessFlags2 ret = 0;
  if ( state & CRUDE_GFX_RHI_RESOURCE_STATE_COPY_SOURCE )
  {
    ret |= VK_ACCESS_2_TRANSFER_READ_BIT_KHR;
  }
  if ( state & CRUDE_GFX_RHI_RESOURCE_STATE_COPY_DEST )
  {
    ret |= VK_ACCESS_2_TRANSFER_WRITE_BIT_KHR;
  }
  if ( state & CRUDE_GFX_RHI_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER )
  {
    ret |= VK_ACCESS_2_UNIFORM_READ_BIT_KHR | VK_ACCESS_2_VERTEX_ATTRIBUTE_READ_BIT_KHR;
  }
  if ( state & CRUDE_GFX_RHI_RESOURCE_STATE_INDEX_BUFFER )
  {
    ret |= VK_ACCESS_2_INDEX_READ_BIT_KHR;
  }
  if ( state & CRUDE_GFX_RHI_RESOURCE_STATE_UNORDERED_ACCESS )
  {
    ret |= VK_ACCESS_2_SHADER_READ_BIT_KHR | VK_ACCESS_2_SHADER_WRITE_BIT_KHR;
  }
  if ( state & CRUDE_GFX_RHI_RESOURCE_STATE_INDIRECT_ARGUMENT )
  {
    ret |= VK_ACCESS_2_INDIRECT_COMMAND_READ_BIT_KHR;
  }
  if ( state & CRUDE_GFX_RHI_RESOURCE_STATE_RENDER_TARGET )
  {
    ret |= VK_ACCESS_2_COLOR_ATTACHMENT_READ_BIT_KHR | VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT_KHR;
  }
  if ( state & CRUDE_GFX_RHI_RESOURCE_STATE_DEPTH_WRITE )
  {
    ret |= VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT_KHR | VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_READ_BIT_KHR;
  }
  if ( state & ( CRUDE_GFX_RHI_RESOURCE_STATE_SHADER_RESOURCE | CRUDE_GFX_RHI_RESOURCE_STATE_PIXEL_SHADER_RESOURCE ) )
  {
    ret |= VK_ACCESS_2_SHADER_READ_BIT_KHR;
  }
  if ( state & CRUDE_GFX_RHI_RESOURCE_STATE_PRESENT )
  {
    ret |= 0;
  }
  if ( state & CRUDE_GFX_RHI_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE )
  {
    ret |= VK_ACCESS_2_ACCELERATION_STRUCTURE_READ_BIT_KHR | VK_ACCESS_2_ACCELERATION_STRUCTURE_WRITE_BIT_KHR;
  }
  return ret;
}

crude_gfx_rhi_image_layout
crude_gfx_rhi_resource_state_to_image_layout
(
  _In_ crude_gfx_rhi_resource_state                        state
)
{
  if ( state & CRUDE_GFX_RHI_RESOURCE_STATE_COPY_SOURCE )         return CRUDE_GFX_RHI_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
  if ( state & CRUDE_GFX_RHI_RESOURCE_STATE_COPY_DEST )           return CRUDE_GFX_RHI_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
  if ( state & CRUDE_GFX_RHI_RESOURCE_STATE_RENDER_TARGET )       return CRUDE_GFX_RHI_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL_KHR;
  if ( state & CRUDE_GFX_RHI_RESOURCE_STATE_DEPTH_WRITE )         return CRUDE_GFX_RHI_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL_KHR;
  if ( state & CRUDE_GFX_RHI_RESOURCE_STATE_DEPTH_READ )          return CRUDE_GFX_RHI_IMAGE_LAYOUT_READ_ONLY_OPTIMAL_KHR;
  if ( state & CRUDE_GFX_RHI_RESOURCE_STATE_UNORDERED_ACCESS )    return CRUDE_GFX_RHI_IMAGE_LAYOUT_GENERAL;
  if ( state & CRUDE_GFX_RHI_RESOURCE_STATE_SHADER_RESOURCE )     return CRUDE_GFX_RHI_IMAGE_LAYOUT_READ_ONLY_OPTIMAL_KHR;
  if ( state & CRUDE_GFX_RHI_RESOURCE_STATE_PRESENT )             return CRUDE_GFX_RHI_IMAGE_LAYOUT_PRESENT_SRC_KHR;
  if ( state == CRUDE_GFX_RHI_RESOURCE_STATE_COMMON )             return CRUDE_GFX_RHI_IMAGE_LAYOUT_GENERAL;
  return CRUDE_GFX_RHI_IMAGE_LAYOUT_UNDEFINED;
}

crude_gfx_rhi_pipeline_stage_flags
crude_gfx_rhi_determine_pipeline_stage_flags
(
  _In_ crude_gfx_rhi_access_flags                          access_flags,
  _In_ crude_gfx_rhi_queue_type                            queue_type
)
{
  VkPipelineStageFlags2KHR flags = 0;
  
  switch ( queue_type )
  {
    case CRUDE_GFX_RHI_QUEUE_TYPE_GRAPHICS:
    {
      if ( access_flags & ( VK_ACCESS_2_INDEX_READ_BIT | VK_ACCESS_2_VERTEX_ATTRIBUTE_READ_BIT ) )
      {
        flags |= VK_PIPELINE_STAGE_2_VERTEX_INPUT_BIT_KHR;
      }
      if ( access_flags & ( VK_ACCESS_2_UNIFORM_READ_BIT | VK_ACCESS_2_SHADER_READ_BIT | VK_ACCESS_2_SHADER_WRITE_BIT ) )
      {
        flags |= VK_PIPELINE_STAGE_2_VERTEX_SHADER_BIT_KHR;
        flags |= VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT_KHR;
        flags |= VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT_KHR;
        //    flags |= VK_PIPELINE_STAGE_RAY_TRACING_SHADER_BIT;
      }
      if ( access_flags & VK_ACCESS_2_INPUT_ATTACHMENT_READ_BIT )
      {
        flags |= VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT_KHR;
      }
      if ( access_flags & ( VK_ACCESS_2_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT ) )
      {
        flags |= VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT_KHR;
      }
      if ( access_flags & ( VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT ) )
      {
        flags |= VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT_KHR | VK_PIPELINE_STAGE_2_LATE_FRAGMENT_TESTS_BIT_KHR;
      }
      if ( access_flags & VK_ACCESS_2_ACCELERATION_STRUCTURE_READ_BIT_KHR || access_flags & VK_ACCESS_2_ACCELERATION_STRUCTURE_WRITE_BIT_KHR )
      {
        flags |= VK_PIPELINE_STAGE_2_ACCELERATION_STRUCTURE_BUILD_BIT_KHR;
      }
      break;
    }
    case CRUDE_GFX_RHI_QUEUE_TYPE_COMPUTE:
    {
      if ( ( access_flags & ( VK_ACCESS_2_INDEX_READ_BIT | VK_ACCESS_2_VERTEX_ATTRIBUTE_READ_BIT ) ) ||
           ( access_flags & VK_ACCESS_2_INPUT_ATTACHMENT_READ_BIT ) ||
           ( access_flags & ( VK_ACCESS_2_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT ) ) ||
           ( access_flags & ( VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT ) ) )
      {
        return VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT_KHR;
      }

      if ( access_flags & ( VK_ACCESS_2_UNIFORM_READ_BIT | VK_ACCESS_2_SHADER_READ_BIT | VK_ACCESS_2_SHADER_WRITE_BIT ) )
      {
        flags |= VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT_KHR;
      }
    
      break;
    }
    case CRUDE_GFX_RHI_QUEUE_TYPE_COPY_TRANSFER:
    {
      return VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT_KHR;
    }
    default:
      break;
  }
  
  if ( access_flags & VK_ACCESS_2_INDIRECT_COMMAND_READ_BIT )
  {
    flags |= VK_PIPELINE_STAGE_2_DRAW_INDIRECT_BIT_KHR;
  }
  if ( access_flags & ( VK_ACCESS_2_TRANSFER_READ_BIT | VK_ACCESS_2_TRANSFER_WRITE_BIT ) )
  {
    flags |= VK_PIPELINE_STAGE_2_TRANSFER_BIT_KHR;
  }
  if ( access_flags & ( VK_ACCESS_2_HOST_READ_BIT | VK_ACCESS_2_HOST_WRITE_BIT ) )
  {
    flags |= VK_PIPELINE_STAGE_2_HOST_BIT_KHR;
  }
  if ( flags == 0 )
  {
    flags = VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT_KHR;
  }
  return flags;
}

crude_gfx_rhi_command_buffer_begin_info
crude_gfx_rhi_command_buffer_begin_info_empty
(
)
{
  crude_gfx_rhi_command_buffer_begin_info begin_info = CRUDE_COMPOUNT_EMPTY( crude_gfx_rhi_command_buffer_begin_info );
  return begin_info;
}

bool
crude_gfx_rhi_queue_submit
(
  _In_ crude_gfx_rhi_queue                                 queue,
  _In_ uint32                                              submit_count,
  _In_ crude_gfx_rhi_submit_info                          *submit_info,
  _In_ crude_gfx_rhi_fence                                 fence
)
{
  VkSemaphoreSubmitInfo                                    vk_wait_semaphores[ 8 ];
  VkSemaphoreSubmitInfo                                    vk_signal_semaphores[ 8 ];
  VkCommandBufferSubmitInfo                                vk_command_buffers[ 8 ];
  VkResult                                                 vk_result;
  VkSubmitInfo2                                            vk_submit_info;

  CRUDE_ASSERT( submit_info->wait_semaphore_info_count < CRUDE_COUNTOF( vk_wait_semaphores ) );
  for ( uint32 i = 0; i < submit_info->wait_semaphore_info_count; ++i )
  {
    vk_wait_semaphores[ i ] = CRUDE_COMPOUNT_EMPTY( VkSemaphoreSubmitInfo );
    vk_wait_semaphores[ i ].sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO_KHR;
    vk_wait_semaphores[ i ].semaphore = submit_info->wait_semaphore_infos[ i ].semaphore.vk_semaphore;
    vk_wait_semaphores[ i ].stageMask = submit_info->wait_semaphore_infos[ i ].stage_mask;
    vk_wait_semaphores[ i ].value = submit_info->wait_semaphore_infos[ i ].value;
    vk_wait_semaphores[ i ].deviceIndex = submit_info->wait_semaphore_infos[ i ].device_index;
  }
  
  CRUDE_ASSERT( submit_info->signal_semaphore_info_count < CRUDE_COUNTOF( vk_signal_semaphores ) );
  for ( uint32 i = 0; i < submit_info->signal_semaphore_info_count; ++i )
  {
    vk_signal_semaphores[ i ] = CRUDE_COMPOUNT_EMPTY( VkSemaphoreSubmitInfo );
    vk_signal_semaphores[ i ].sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO_KHR;
    vk_signal_semaphores[ i ].semaphore = submit_info->signal_semaphore_infos[ i ].semaphore.vk_semaphore;
    vk_signal_semaphores[ i ].stageMask = submit_info->signal_semaphore_infos[ i ].stage_mask;
    vk_signal_semaphores[ i ].value = submit_info->signal_semaphore_infos[ i ].value;
    vk_signal_semaphores[ i ].deviceIndex = submit_info->signal_semaphore_infos[ i ].device_index;
  }

  CRUDE_ASSERT( submit_info->command_buffer_info_count < CRUDE_COUNTOF( vk_command_buffers ) );
  for ( uint32 i = 0; i < submit_info->command_buffer_info_count; ++i )
  {
    vk_command_buffers[ i ] = CRUDE_COMPOUNT_EMPTY( VkCommandBufferSubmitInfo );
    vk_command_buffers[ i ].sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO_KHR;
    vk_command_buffers[ i ].commandBuffer = submit_info->command_buffer_infos[ i ].command_buffer.vk_command_buffer;
    vk_command_buffers[ i ].deviceMask  = submit_info->command_buffer_infos[ i ].device_mask;
  }
    
  vk_submit_info = CRUDE_COMPOUNT_EMPTY( VkSubmitInfo2 );
  vk_submit_info.sType                    = VK_STRUCTURE_TYPE_SUBMIT_INFO_2_KHR;
  vk_submit_info.waitSemaphoreInfoCount   = submit_info->wait_semaphore_info_count;
  vk_submit_info.pWaitSemaphoreInfos      = vk_wait_semaphores;
  vk_submit_info.commandBufferInfoCount   = submit_info->command_buffer_info_count;
  vk_submit_info.pCommandBufferInfos      = vk_command_buffers;
  vk_submit_info.signalSemaphoreInfoCount = submit_info->signal_semaphore_info_count;
  vk_submit_info.pSignalSemaphoreInfos    = vk_signal_semaphores;

  vk_result = vkQueueSubmit2( queue.vk_queue, 1u, &vk_submit_info, fence.vk_fence );
  return vk_result != VK_ERROR_DEVICE_LOST && vk_result != VK_ERROR_OUT_OF_DEVICE_MEMORY && vk_result != VK_ERROR_UNKNOWN;
}

void
crude_gfx_rhi_queue_submit_simple
(
  _In_ crude_gfx_rhi_queue                                 queue,
  _In_ crude_gfx_rhi_command_buffer                        command_buffer,
  _In_ crude_gfx_rhi_fence                                 fence
)
{
  VkSubmitInfo submit_info = CRUDE_COMPOUNT_EMPTY( VkSubmitInfo );
  submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
  submit_info.pCommandBuffers = &command_buffer.vk_command_buffer;
  submit_info.commandBufferCount = 1u;

  vkQueueSubmit( queue.vk_queue, 1, &submit_info, fence.vk_fence );
}

void
crude_gfx_rhi_wait_for_fence
(
  _In_ crude_gfx_rhi_device                               *device,
  _In_ crude_gfx_rhi_fence                                 fence
)
{
  if ( vkGetFenceStatus( device->vk_device, fence.vk_fence ) != VK_SUCCESS )
  {
    vkWaitForFences( device->vk_device, 1u, &fence.vk_fence, VK_TRUE, UINT64_MAX );
  }
}

bool
crude_gfx_rhi_queue_present
(
  _In_ crude_gfx_rhi_queue                                 queue,
  _In_ crude_gfx_rhi_semaphore                             semaphore,
  _In_ crude_gfx_rhi_swapchain                             swapchain,
  _Out_ uint32                                            *image_indices
)
{
  VkSemaphore wait_semaphores[] = { semaphore.vk_semaphore };
  VkSwapchainKHR swap_chains[] = { swapchain.vk_swapchain };

  VkPresentInfoKHR vk_present_info = CRUDE_COMPOUNT_EMPTY( VkPresentInfoKHR );
  vk_present_info.sType              = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
  vk_present_info.waitSemaphoreCount = 1;
  vk_present_info.pWaitSemaphores    = wait_semaphores;
  vk_present_info.swapchainCount     = CRUDE_COUNTOF( swap_chains );
  vk_present_info.pSwapchains        = swap_chains;
  vk_present_info.pImageIndices      = image_indices;
  
  VkResult result = vkQueuePresentKHR( queue.vk_queue, &vk_present_info );
  return ( result != VK_ERROR_OUT_OF_DATE_KHR ) && ( result != VK_SUBOPTIMAL_KHR );
}

void
crude_gfx_rhi_get_query_pool_results
(
  _In_ crude_gfx_rhi_device                               *device,
  _In_ crude_gfx_rhi_query_pool                            query_pool,
  _In_ uint32                                              first_query,
  _In_ uint32                                              query_count,
  _In_ uint64                                              data_size,
  _In_ void                                               *data,
  _In_ crude_gfx_rhi_device_size                           stride,
  _In_ crude_gfx_rhi_query_result_flags                    flags
)
{
  vkGetQueryPoolResults( device->vk_device, query_pool.vk_query_pool, first_query, query_count, data_size, data, stride, flags );
}

crude_gfx_rhi_device_address
crude_gfx_rhi_get_buffer_device_address
(
  _In_ crude_gfx_rhi_device                               *device,
  _In_ crude_gfx_rhi_buffer                                buffer
)
{
  VkBufferDeviceAddressInfo device_address_info = CRUDE_COMPOUNT_EMPTY( VkBufferDeviceAddressInfo );
  device_address_info.sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO;
  device_address_info.buffer = buffer.vk_buffer;
  return vkGetBufferDeviceAddress( device->vk_device, &device_address_info );
}

void
crude_gfx_rhi_create_surface
(
  _In_ crude_gfx_rhi_instance                              instance,
  _In_ SDL_Window                                         *window,
  _Out_ crude_gfx_rhi_surface                             *surface
)
{
  if ( !SDL_Vulkan_CreateSurface( window, instance.vk_instance, CRUDE_GFX_RHI_DEVICE_VK_ALLOCATION_CALLBACKS, &surface->vk_surface ) )
  {
    CRUDE_ABORT( CRUDE_CHANNEL_GRAPHICS, "failed to create vk_surface: %s", SDL_GetError() );
  }
}

void
crude_gfx_rhi_destroy_surface
(
  _In_ crude_gfx_rhi_instance                              instance,
  _In_ crude_gfx_rhi_surface                               surface
)
{
  vkDestroySurfaceKHR( instance.vk_instance, surface.vk_surface, CRUDE_GFX_RHI_DEVICE_VK_ALLOCATION_CALLBACKS );
}

void
crude_gfx_rhi_create_descriptor_set_layout
(
  _In_ crude_gfx_rhi_device                               *device,
  _In_ crude_gfx_rhi_descriptor_set_layout_create_info const *creation,
  _Out_ crude_gfx_rhi_descriptor_set_layout               *layout
)
{
  VkDescriptorSetLayoutCreateInfo                          vk_creation;
  VkDescriptorSetLayoutBinding                             vk_bindings[ 128 ];

  CRUDE_ASSERT( creation->binding_count < CRUDE_COUNTOF( vk_bindings ) );
  for ( uint32 i = 0; i < creation->binding_count; ++i )
  {
    vk_bindings[ i ] = CRUDE_COMPOUNT_EMPTY( VkDescriptorSetLayoutBinding );
    vk_bindings[ i ].binding = creation->bindings[ i ].binding;
    vk_bindings[ i ].descriptorType = CRUDE_CAST( VkDescriptorType, creation->bindings[ i ].descriptor_type );
    vk_bindings[ i ].descriptorCount = creation->bindings[ i ].descriptor_count;
    vk_bindings[ i ].stageFlags = creation->bindings[ i ].stage_flags;
    vk_bindings[ i ].pImmutableSamplers = NULL;
  }

  if ( creation->bindless )
  {
    VkDescriptorBindingFlags                               vk_binding_flags[ 128 ];
    VkDescriptorSetLayoutBindingFlagsCreateInfoEXT         vk_extended_info;

    for ( uint32 i = 0; i < creation->binding_count; ++i )
    {
      vk_binding_flags[ i ] = VK_DESCRIPTOR_BINDING_UPDATE_AFTER_BIND_BIT_EXT;
    }

    vk_extended_info = CRUDE_COMPOUNT_EMPTY( VkDescriptorSetLayoutBindingFlagsCreateInfoEXT );
    vk_extended_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO_EXT;
    vk_extended_info.bindingCount = creation->binding_count;
    vk_extended_info.pBindingFlags = vk_binding_flags;

    vk_creation = CRUDE_COMPOUNT_EMPTY( VkDescriptorSetLayoutCreateInfo );
    vk_creation.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    vk_creation.pNext = &vk_extended_info;
    vk_creation.flags = VK_DESCRIPTOR_SET_LAYOUT_CREATE_UPDATE_AFTER_BIND_POOL_BIT_EXT;
    vk_creation.bindingCount = creation->binding_count;
    vk_creation.pBindings = vk_bindings;

    CRUDE_GFX_RHI_HANDLE_VULKAN_RESULT( vkCreateDescriptorSetLayout( device->vk_device, &vk_creation, CRUDE_GFX_RHI_DEVICE_VK_ALLOCATION_CALLBACKS, &layout->vk_descriptor_set_layout ), "Failed create descriptor set layout" );
  }
  else
  {
    vk_creation = CRUDE_COMPOUNT_EMPTY( VkDescriptorSetLayoutCreateInfo );
    vk_creation.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    vk_creation.bindingCount = creation->binding_count;
    vk_creation.pBindings = vk_bindings;
    CRUDE_GFX_RHI_HANDLE_VULKAN_RESULT( vkCreateDescriptorSetLayout( device->vk_device, &vk_creation, CRUDE_GFX_RHI_DEVICE_VK_ALLOCATION_CALLBACKS, &layout->vk_descriptor_set_layout ), "Failed to create descriptor set layout" );
  }
}

void
crude_gfx_rhi_destroy_descriptor_set_layout
(
  _In_ crude_gfx_rhi_device                               *device,
  _In_ crude_gfx_rhi_descriptor_set_layout                 layout
)
{
  vkDestroyDescriptorSetLayout( device->vk_device, layout.vk_descriptor_set_layout, CRUDE_GFX_RHI_DEVICE_VK_ALLOCATION_CALLBACKS );
}

void
crude_gfx_rhi_create_descriptor_set
(
  _In_ crude_gfx_rhi_device                               *device,
  _In_ crude_gfx_rhi_descriptor_set_create_info const     *creation,
  _Out_ crude_gfx_rhi_descriptor_set                      *descriptor_set
)
{
  VkDescriptorSetAllocateInfo                              vk_descriptor_info;
  VkDescriptorSetVariableDescriptorCountAllocateInfoEXT    vk_count_info;
  uint32                                                   max_binding;

  vk_descriptor_info = CRUDE_COMPOUNT_EMPTY( VkDescriptorSetAllocateInfo );
  vk_descriptor_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
  vk_descriptor_info.descriptorPool = creation->descriptor_pool.vk_descriptor_pool;
  vk_descriptor_info.descriptorSetCount = 1u;
  vk_descriptor_info.pSetLayouts = &creation->descriptor_set_layout.vk_descriptor_set_layout;

  if ( creation->bindless )
  {
    max_binding = CRUDE_GFX_BINDLESS_RESOURCES_MAX_COUNT - 1;
    vk_count_info = CRUDE_COMPOUNT_EMPTY( VkDescriptorSetVariableDescriptorCountAllocateInfoEXT );
    vk_count_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_VARIABLE_DESCRIPTOR_COUNT_ALLOCATE_INFO_EXT;
    vk_count_info.descriptorSetCount = 1;
    vk_count_info.pDescriptorCounts = &max_binding;

    vk_descriptor_info.pNext = &vk_count_info;
  }

  CRUDE_GFX_RHI_HANDLE_VULKAN_RESULT( vkAllocateDescriptorSets( device->vk_device, &vk_descriptor_info, &descriptor_set->vk_descriptor_set ), "Failed to allocate descriptor set" );
}

void
crude_gfx_rhi_set_descriptor_set_debug_name
(
  _In_ crude_gfx_rhi_device                               *device,
  _In_ crude_gfx_rhi_descriptor_set                        descriptor_set,
  _In_ char const                                         *name
)
{
  crude_gfx_rhi_set_debug_utils_object_name( device, CRUDE_GFX_RHI_OBJECT_TYPE_DESCRIPTOR_SET, CRUDE_CAST( uint64, descriptor_set.vk_descriptor_set ), name );
}

void
crude_gfx_rhi_create_descriptor_pool
(
  _In_ crude_gfx_rhi_device                               *device,
  _In_ bool                                                bindless,
  _Out_ crude_gfx_rhi_descriptor_pool                     *descriptor_pool
)
{
  VkDescriptorPoolCreateInfo                               vk_creation;

  if ( bindless )
  {
    vk_creation = CRUDE_COMPOUNT_EMPTY( VkDescriptorPoolCreateInfo );
    vk_creation.sType         = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    vk_creation.flags         = VK_DESCRIPTOR_POOL_CREATE_UPDATE_AFTER_BIND_BIT_EXT | VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
    vk_creation.maxSets       = CRUDE_GFX_BINDLESS_RESOURCES_MAX_COUNT * CRUDE_COUNTOF( crude_gfx_rhi_vk_pool_sizes_bindless_ );
    vk_creation.poolSizeCount = CRUDE_COUNTOF( crude_gfx_rhi_vk_pool_sizes_bindless_ );
    vk_creation.pPoolSizes    = crude_gfx_rhi_vk_pool_sizes_bindless_;
  }
  else
  {
    vk_creation = CRUDE_COMPOUNT_EMPTY( VkDescriptorPoolCreateInfo );
    vk_creation.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    vk_creation.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
    vk_creation.maxSets = 4096;
    vk_creation.poolSizeCount = CRUDE_COUNTOF( crude_gfx_rhi_vk_pool_sizes_ );
    vk_creation.pPoolSizes = crude_gfx_rhi_vk_pool_sizes_;
  }
  CRUDE_GFX_RHI_HANDLE_VULKAN_RESULT( vkCreateDescriptorPool( device->vk_device, &vk_creation, CRUDE_GFX_RHI_DEVICE_VK_ALLOCATION_CALLBACKS, &descriptor_pool->vk_descriptor_pool ), "Failed create descriptor pool" );
}

void
crude_gfx_rhi_set_descriptor_pool_debug_name
(
  _In_ crude_gfx_rhi_device                               *device,
  _In_ crude_gfx_rhi_descriptor_pool                       descriptor_pool,
  _In_ char const                                         *name
)
{
  crude_gfx_rhi_set_debug_utils_object_name( device, CRUDE_GFX_RHI_OBJECT_TYPE_DESCRIPTOR_POOL, CRUDE_CAST( uint64, descriptor_pool.vk_descriptor_pool ), name );
}

void
crude_gfx_rhi_create_acceleration_structure
(
  _In_ crude_gfx_rhi_device                               *device,
  _In_ crude_gfx_rhi_acceleration_structure_create_info const *creation,
  _Out_ crude_gfx_rhi_acceleration_structure              *acceleration_structure
)
{
  VkAccelerationStructureCreateInfoKHR                     vk_acceleration_structure_create_info;

  vk_acceleration_structure_create_info = CRUDE_COMPOUNT_EMPTY( VkAccelerationStructureCreateInfoKHR );
  vk_acceleration_structure_create_info.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR;
  vk_acceleration_structure_create_info.buffer = creation->buffer.vk_buffer;
  vk_acceleration_structure_create_info.offset = creation->offset;
  vk_acceleration_structure_create_info.size = creation->size;
  vk_acceleration_structure_create_info.type = CRUDE_CAST( VkAccelerationStructureTypeKHR, creation->type );
  vk_acceleration_structure_create_info.deviceAddress = creation->device_address;
  CRUDE_GFX_RHI_HANDLE_VULKAN_RESULT( device->vkCreateAccelerationStructureKHR( device->vk_device, &vk_acceleration_structure_create_info, CRUDE_GFX_RHI_DEVICE_VK_ALLOCATION_CALLBACKS, &acceleration_structure->vk_acceleration_structure ), "Failed vkCreateAccelerationStructureKHR" );
}

void
crude_gfx_rhi_destroy_acceleration_structure
(
  _In_ crude_gfx_rhi_device                               *device,
  _In_ crude_gfx_rhi_acceleration_structure                acceleration_structure
)
{
  device->vkDestroyAccelerationStructureKHR( device->vk_device, acceleration_structure.vk_acceleration_structure, CRUDE_GFX_RHI_DEVICE_VK_ALLOCATION_CALLBACKS );
}

void
crude_gfx_rhi_create_command_pool
(
  _In_ crude_gfx_rhi_device                               *device,
  _In_ crude_gfx_rhi_command_pool_create_info const       *creation,
  _Out_ crude_gfx_rhi_command_pool                        *command_pool
)
{
  VkCommandPoolCreateInfo                                  vk_cmd_pool_creation;
  
  vk_cmd_pool_creation = CRUDE_COMPOUNT_EMPTY( VkCommandPoolCreateInfo );
  vk_cmd_pool_creation.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
  vk_cmd_pool_creation.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
  vk_cmd_pool_creation.queueFamilyIndex = creation->queue.vk_queue_family;
  CRUDE_GFX_RHI_HANDLE_VULKAN_RESULT( vkCreateCommandPool( device->vk_device, &vk_cmd_pool_creation, CRUDE_GFX_RHI_DEVICE_VK_ALLOCATION_CALLBACKS, &command_pool->vk_command_pool ), "Failed create command pool" );
}

void
crude_gfx_rhi_destroy_command_pool
(
  _In_ crude_gfx_rhi_device                               *device,
  _In_ crude_gfx_rhi_command_pool                          command_pool
)
{
  vkDestroyCommandPool( device->vk_device, command_pool.vk_command_pool, CRUDE_GFX_RHI_DEVICE_VK_ALLOCATION_CALLBACKS );
}

void
crude_gfx_rhi_create_query_pool
(
  _In_ crude_gfx_rhi_device                               *device,
  _In_ crude_gfx_rhi_queru_pool_create_info const         *creation,
  _Out_ crude_gfx_rhi_query_pool                          *query_pool
)
{
  VkQueryPoolCreateInfo                                    vk_creation;

  vk_creation = CRUDE_COMPOUNT_EMPTY( VkQueryPoolCreateInfo );
  vk_creation.sType = VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO;
  vk_creation.queryType = CRUDE_CAST( VkQueryType, creation->query_type );
  vk_creation.queryCount = creation->query_count;
  vk_creation.pipelineStatistics = creation->pipeline_statistics;
  CRUDE_GFX_RHI_HANDLE_VULKAN_RESULT( vkCreateQueryPool( device->vk_device, &vk_creation, CRUDE_GFX_RHI_DEVICE_VK_ALLOCATION_CALLBACKS, &query_pool->vk_query_pool ), "Failed create query pool" );     
}

void
crude_gfx_rhi_destroy_query_pool
(
  _In_ crude_gfx_rhi_device                               *device,
  _In_ crude_gfx_rhi_query_pool                            query_pool
)
{
  vkDestroyQueryPool( device->vk_device, query_pool.vk_query_pool, CRUDE_GFX_RHI_DEVICE_VK_ALLOCATION_CALLBACKS );
}

void
crude_gfx_rhi_create_command_buffer
(
  _In_ crude_gfx_rhi_device                               *device,
  _In_ crude_gfx_rhi_command_buffer_create_info const     *creation,
  _Out_ crude_gfx_rhi_command_buffer                      *command_buffer
)
{
  VkCommandBufferAllocateInfo                              vk_cmd_allocation_info;

  vk_cmd_allocation_info = CRUDE_COMPOUNT_EMPTY( VkCommandBufferAllocateInfo );
  vk_cmd_allocation_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
  vk_cmd_allocation_info.commandPool = creation->command_pool.vk_command_pool;
  vk_cmd_allocation_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
  vk_cmd_allocation_info.commandBufferCount = 1;
    
  CRUDE_GFX_RHI_HANDLE_VULKAN_RESULT( vkAllocateCommandBuffers( device->vk_device, &vk_cmd_allocation_info, &command_buffer->vk_command_buffer ), "Failed to allocate command buffer" );
}

void
crude_gfx_rhi_destroy_command_buffer
(
  _In_ crude_gfx_rhi_device                               *device,
  _In_ crude_gfx_rhi_command_buffer                        command_buffer
)
{
}

void
crude_gfx_rhi_set_command_buffer_debug_name
(
  _In_ crude_gfx_rhi_device                               *device,
  _In_ crude_gfx_rhi_command_buffer                        command_buffer,
  _In_ char const                                         *name
)
{
  crude_gfx_rhi_set_debug_utils_object_name( device, CRUDE_GFX_RHI_OBJECT_TYPE_COMMAND_BUFFER, CRUDE_CAST( uint64, command_buffer.vk_command_buffer ), name );
}

void
crude_gfx_rhi_create_device
(
  _In_ crude_gfx_rhi_instance                              instance,
  _In_ crude_gfx_rhi_surface                               surface,
  _In_ crude_heap_allocator                               *allocator,
  _Out_ crude_gfx_rhi_device                              *device
)
{
  VkQueueFamilyProperties                                 *vk_queue_families;
  char const                                              *vk_device_extensions[ 64 ];
  void                                                    *vk_next_feature;
  VkDeviceQueueCreateInfo                                  vk_queue_create_infos[ 2 ];
#if CRUDE_GFX_NSIGHT_AFTERMATH
  VkPhysicalDeviceDiagnosticsConfigFeaturesNV              vk_physical_device_diagnostics_config_features_nv;
#endif /* CRUDE_GFX_NSIGHT_AFTERMATH */
#if CRUDE_GFX_RAY_TRACING_ENABLED
#if CRUDE_GRAPHICS_VALIDATION_LAYERS_ENABLED
  VkPhysicalDeviceRayTracingValidationFeaturesNV           vk_physical_device_ray_tracing_validation_features_nv;
#endif /* CRUDE_GRAPHICS_VALIDATION_LAYERS_ENABLED */
  VkPhysicalDeviceRayTracingPositionFetchFeaturesKHR       vk_physical_device_ray_tracing_position_fetch_features;
  VkPhysicalDeviceRayQueryFeaturesKHR                      vk_physical_device_ray_query_features;
  VkPhysicalDeviceRayTracingPipelineFeaturesKHR            vk_physical_device_ray_tracing_pipeline_features;
  VkPhysicalDeviceAccelerationStructureFeaturesKHR         vk_physical_device_acceleration_structure_features;
#endif /* CRUDE_GFX_RAY_TRACING_ENABLED */
  VkPhysicalDeviceShaderAtomicInt64Features                vk_shader_atomic_int64_features;
  VkPhysicalDeviceShaderRelaxedExtendedInstructionFeaturesKHR vk_shader_relaxed_extended_instruction_features;
  VkPhysicalDeviceSynchronization2Features                 vk_synchronization_features;
  VkPhysicalDeviceDynamicRenderingFeaturesKHR              vk_dynamic_rendering_features;
  VkPhysicalDeviceFeatures2                                vk_physical_features2;
  VkPhysicalDeviceFragmentShadingRateFeaturesKHR           vk_device_features_fragment_shading_rate;
  VkPhysicalDeviceVulkan11Features                         vk_device_features_vulkan11;
  VkPhysicalDeviceVulkan12Features                         vk_device_features_vulkan12;
  VkPhysicalDeviceMeshShaderFeaturesEXT                    vk_device_features_mesh;
  VkDeviceCreateInfo                                       vk_device_creation;
  VmaAllocatorCreateInfo                                   vma_creation;
  uint32                                                   vk_queue_family_count, vk_compute_queue_index, vk_present_queue_index;
  uint32                                                   vk_device_extensions_count;

  crude_gfx_rhi_vk_pick_physical_device_( instance.vk_instance, surface.vk_surface, allocator, &device->vk_physical_device, &device->optional_extensions );

  vk_queue_family_count = 0;
  vkGetPhysicalDeviceQueueFamilyProperties( device->vk_physical_device, &vk_queue_family_count, NULL );
  
  CRUDE_ARRAY_INITIALIZE_WITH_LENGTH( vk_queue_families, vk_queue_family_count, crude_heap_allocator_pack( allocator ) );
  vkGetPhysicalDeviceQueueFamilyProperties( device->vk_physical_device, &vk_queue_family_count, vk_queue_families );
  
  device->main_queue.vk_queue_family = UINT32_MAX;
  device->transfer_queue.vk_queue_family = UINT32_MAX;
  vk_compute_queue_index = UINT32_MAX;
  vk_present_queue_index = UINT32_MAX;

  for ( uint32 family_index = 0; family_index < vk_queue_family_count; ++family_index )
  {
    VkQueueFamilyProperties                                vk_queue_family;

    vk_queue_family = vk_queue_families[ family_index ];
    
    if ( vk_queue_family.queueCount == 0 )
    {
      continue;
    }
    
    if ( ( vk_queue_family.queueFlags & ( VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT ) ) == ( VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT  ) )
    {
      device->main_queue.vk_queue_family = family_index;
    }

    if ( ( vk_queue_family.queueFlags & VK_QUEUE_COMPUTE_BIT ) == 0 && ( vk_queue_family.queueFlags & VK_QUEUE_TRANSFER_BIT ) )
    {
      device->transfer_queue.vk_queue_family = family_index;
    }
  }

  float const queue_priority[] = { 1.0f };

  vk_queue_create_infos[ 0 ] = CRUDE_COMPOUNT_EMPTY( VkDeviceQueueCreateInfo );
  vk_queue_create_infos[ 0 ].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
  vk_queue_create_infos[ 0 ].queueFamilyIndex = device->main_queue.vk_queue_family;
  vk_queue_create_infos[ 0 ].queueCount = 1;
  vk_queue_create_infos[ 0 ].pQueuePriorities = queue_priority;
  
  if ( device->transfer_queue.vk_queue_family < vk_queue_family_count )
  {
    vk_queue_create_infos[ 1 ] = CRUDE_COMPOUNT_EMPTY( VkDeviceQueueCreateInfo );
    vk_queue_create_infos[ 1 ].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    vk_queue_create_infos[ 1 ].queueFamilyIndex = device->transfer_queue.vk_queue_family;
    vk_queue_create_infos[ 1 ].queueCount = 1;
    vk_queue_create_infos[ 1 ].pQueuePriorities = queue_priority;
  }

  vk_next_feature = NULL;

  //shader_atomic_int64_features = CRUDE_COMPOUNT_EMPTY( VkPhysicalDeviceShaderAtomicInt64Features );
  //shader_atomic_int64_features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_ATOMIC_INT64_FEATURES;
  //shader_atomic_int64_features.shaderBufferInt64Atomics = true;
  
#if CRUDE_GFX_RAY_TRACING_ENABLED
  vk_physical_device_ray_tracing_position_fetch_features = CRUDE_COMPOUNT_EMPTY( VkPhysicalDeviceRayTracingPositionFetchFeaturesKHR );
  vk_physical_device_ray_tracing_position_fetch_features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_POSITION_FETCH_FEATURES_KHR;
  vk_physical_device_ray_tracing_position_fetch_features.pNext = vk_next_feature;
  vk_physical_device_ray_tracing_position_fetch_features.rayTracingPositionFetch = VK_TRUE;
  vk_next_feature = &vk_physical_device_ray_tracing_position_fetch_features;

  vk_physical_device_ray_query_features = CRUDE_COMPOUNT_EMPTY( VkPhysicalDeviceRayQueryFeaturesKHR );
  vk_physical_device_ray_query_features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_QUERY_FEATURES_KHR;
  vk_physical_device_ray_query_features.pNext = vk_next_feature;
  vk_physical_device_ray_query_features.rayQuery = VK_TRUE;
  vk_next_feature = &vk_physical_device_ray_query_features;

  vk_physical_device_ray_tracing_pipeline_features = CRUDE_COMPOUNT_EMPTY( VkPhysicalDeviceRayTracingPipelineFeaturesKHR );
  vk_physical_device_ray_tracing_pipeline_features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_FEATURES_KHR;
  vk_physical_device_ray_tracing_pipeline_features.pNext = vk_next_feature;
  vk_physical_device_ray_tracing_pipeline_features.rayTracingPipeline = VK_TRUE;
  vk_next_feature = &vk_physical_device_ray_tracing_pipeline_features;
  
  vk_physical_device_acceleration_structure_features = CRUDE_COMPOUNT_EMPTY( VkPhysicalDeviceAccelerationStructureFeaturesKHR );
  vk_physical_device_acceleration_structure_features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ACCELERATION_STRUCTURE_FEATURES_KHR;
  vk_physical_device_acceleration_structure_features.pNext = vk_next_feature;
  vk_physical_device_acceleration_structure_features.accelerationStructure = VK_TRUE;
  vk_next_feature = &vk_physical_device_acceleration_structure_features;
  
//#if CRUDE_GRAPHICS_VALIDATION_LAYERS_ENABLED
//  vk_physical_device_ray_tracing_validation_features_nv = CRUDE_COMPOUNT_EMPTY( VkPhysicalDeviceRayTracingValidationFeaturesNV );
//  vk_physical_device_ray_tracing_validation_features_nv.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_VALIDATION_FEATURES_NV;
//  vk_physical_device_ray_tracing_validation_features_nv.pNext = vk_next_feature;
//  vk_physical_device_ray_tracing_validation_features_nv.rayTracingValidation = true;
//  vk_next_feature = &vk_physical_device_ray_tracing_validation_features_nv;
//#endif /* CRUDE_GRAPHICS_VALIDATION_LAYERS_ENABLED */

#endif /* CRUDE_GFX_RAY_TRACING_ENABLED */

#if CRUDE_GFX_NSIGHT_AFTERMATH
  vk_physical_device_diagnostics_config_features_nv = CRUDE_COMPOUNT_EMPTY( VkPhysicalDeviceDiagnosticsConfigFeaturesNV );
  vk_physical_device_diagnostics_config_features_nv.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DIAGNOSTICS_CONFIG_FEATURES_NV;
  vk_physical_device_diagnostics_config_features_nv.pNext = vk_next_feature;
  vk_physical_device_diagnostics_config_features_nv.diagnosticsConfig = 
    VK_DEVICE_DIAGNOSTICS_CONFIG_ENABLE_RESOURCE_TRACKING_BIT_NV |
    VK_DEVICE_DIAGNOSTICS_CONFIG_ENABLE_AUTOMATIC_CHECKPOINTS_BIT_NV |
    VK_DEVICE_DIAGNOSTICS_CONFIG_ENABLE_SHADER_DEBUG_INFO_BIT_NV |
    VK_DEVICE_DIAGNOSTICS_CONFIG_ENABLE_SHADER_ERROR_REPORTING_BIT_NV;
  vk_next_feature = &vk_physical_device_diagnostics_config_features_nv;
#endif
  
  if ( device->optional_extensions.shader_relaxed_extended_instruction_extension_present )
  {
    vk_shader_relaxed_extended_instruction_features = CRUDE_COMPOUNT_EMPTY( VkPhysicalDeviceShaderRelaxedExtendedInstructionFeaturesKHR );
    vk_shader_relaxed_extended_instruction_features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_RELAXED_EXTENDED_INSTRUCTION_FEATURES_KHR;
    vk_shader_relaxed_extended_instruction_features.pNext = vk_next_feature;
    vk_shader_relaxed_extended_instruction_features.shaderRelaxedExtendedInstruction = VK_TRUE;
    vk_next_feature = &vk_shader_relaxed_extended_instruction_features;
  }
  
  vk_synchronization_features = CRUDE_COMPOUNT_EMPTY( VkPhysicalDeviceSynchronization2Features );
  vk_synchronization_features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SYNCHRONIZATION_2_FEATURES;
  vk_synchronization_features.pNext = vk_next_feature;
  vk_synchronization_features.synchronization2 = VK_TRUE;
  vk_next_feature = &vk_synchronization_features;

  if ( device->optional_extensions.fragment_shading_rate_extension_present )
  {
    vk_device_features_fragment_shading_rate = CRUDE_COMPOUNT_EMPTY( VkPhysicalDeviceFragmentShadingRateFeaturesKHR );
    vk_device_features_fragment_shading_rate.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FRAGMENT_SHADING_RATE_FEATURES_KHR;
    vk_device_features_fragment_shading_rate.pNext = vk_next_feature;
    vk_device_features_fragment_shading_rate.primitiveFragmentShadingRate = VK_TRUE;
    vk_next_feature = &vk_device_features_fragment_shading_rate;
  }

  vk_device_features_vulkan11 = CRUDE_COMPOUNT_EMPTY( VkPhysicalDeviceVulkan11Features );
  vk_device_features_vulkan11.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_FEATURES;
  vk_device_features_vulkan11.storageBuffer16BitAccess = VK_TRUE;
  vk_device_features_vulkan11.pNext = vk_next_feature;
  vk_next_feature = &vk_device_features_vulkan11;

  vk_device_features_vulkan12 = CRUDE_COMPOUNT_EMPTY( VkPhysicalDeviceVulkan12Features );
  vk_device_features_vulkan12.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES;
  vk_device_features_vulkan12.drawIndirectCount = VK_TRUE;
  vk_device_features_vulkan12.pNext = vk_next_feature;
  vk_next_feature = &vk_device_features_vulkan12;
  
  if ( device->optional_extensions.mesh_shaders_extension_present )
  {
    vk_device_features_mesh = CRUDE_COMPOUNT_EMPTY( VkPhysicalDeviceMeshShaderFeaturesEXT );
    vk_device_features_mesh.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MESH_SHADER_FEATURES_EXT;
    vk_device_features_mesh.pNext = vk_next_feature;
    vk_device_features_mesh.taskShader = VK_TRUE;
    vk_device_features_mesh.meshShader = VK_TRUE;
    vk_device_features_mesh.multiviewMeshShader = VK_TRUE;
    vk_device_features_mesh.primitiveFragmentShadingRateMeshShader = VK_TRUE;
    vk_next_feature = &vk_device_features_mesh;
  }
  
  vk_dynamic_rendering_features = CRUDE_COMPOUNT_EMPTY( VkPhysicalDeviceDynamicRenderingFeaturesKHR );
  vk_dynamic_rendering_features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DYNAMIC_RENDERING_FEATURES_KHR;
  vk_dynamic_rendering_features.pNext = vk_next_feature;
  vk_next_feature = &vk_dynamic_rendering_features;

  vk_physical_features2 = CRUDE_COMPOUNT_EMPTY( VkPhysicalDeviceFeatures2 );
  vk_physical_features2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
  vk_physical_features2.pNext = vk_next_feature;
#if CRUDE_GFX_SAMPLE_RATE_SHADING
  vk_physical_features2.features.sampleRateShading = VK_TRUE;
#endif
  vkGetPhysicalDeviceFeatures2( device->vk_physical_device, &vk_physical_features2 );

  CRUDE_ASSERT( ( 10 + CRUDE_COUNTOF( crude_gfx_rhi_vk_device_required_extensions ) < CRUDE_COUNTOF( vk_device_extensions ) ) );

  vk_device_extensions_count = 0u;
  for ( uint32 i = 0; i < CRUDE_COUNTOF( crude_gfx_rhi_vk_device_required_extensions ); ++i )
  {
    vk_device_extensions[ vk_device_extensions_count++ ] = crude_gfx_rhi_vk_device_required_extensions[ i ];
  }
  
  if ( device->optional_extensions.mesh_shaders_extension_present )
  {
    vk_device_extensions[ vk_device_extensions_count++ ] = VK_EXT_MESH_SHADER_EXTENSION_NAME;
  }

  if ( device->optional_extensions.fragment_shading_rate_extension_present )
  {
    vk_device_extensions[ vk_device_extensions_count++ ] = VK_KHR_FRAGMENT_SHADING_RATE_EXTENSION_NAME;
  }

  if ( device->optional_extensions.deferred_host_operations_extension_present )
  {
    vk_device_extensions[ vk_device_extensions_count++ ] = VK_KHR_DEFERRED_HOST_OPERATIONS_EXTENSION_NAME;
  }

  if ( device->optional_extensions.shader_relaxed_extended_instruction_extension_present )
  {
    vk_device_extensions[ vk_device_extensions_count++ ] = VK_KHR_SHADER_RELAXED_EXTENDED_INSTRUCTION_EXTENSION_NAME;
  }

  vk_device_creation = CRUDE_COMPOUNT_EMPTY( VkDeviceCreateInfo );
  vk_device_creation.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
  vk_device_creation.pNext = &vk_physical_features2;
  vk_device_creation.flags = 0;
  vk_device_creation.queueCreateInfoCount = CRUDE_STATIC_CAST( uint32, device->transfer_queue.vk_queue_family < vk_queue_family_count ? 2 : 1 );
  vk_device_creation.pQueueCreateInfos = vk_queue_create_infos;
#if CRUDE_GRAPHICS_VALIDATION_LAYERS_ENABLED
  vk_device_creation.enabledLayerCount = CRUDE_COUNTOF( vk_required_debug_layers );
  vk_device_creation.ppEnabledLayerNames = vk_required_debug_layers;
#endif
  vk_device_creation.enabledExtensionCount = vk_device_extensions_count;
  vk_device_creation.ppEnabledExtensionNames = vk_device_extensions;
  vk_device_creation.pEnabledFeatures = NULL;

  CRUDE_GFX_RHI_HANDLE_VULKAN_RESULT( vkCreateDevice( device->vk_physical_device, &vk_device_creation, CRUDE_GFX_RHI_DEVICE_VK_ALLOCATION_CALLBACKS, &device->vk_device ), "Failed to create logic device!" );
  
  
  device->vkCmdDrawMeshTasksIndirectCountEXT = ( PFN_vkCmdDrawMeshTasksIndirectCountEXT )vkGetDeviceProcAddr( device->vk_device, "vkCmdDrawMeshTasksIndirectCountEXT" );
  device->vkCmdDrawMeshTasksEXT = ( PFN_vkCmdDrawMeshTasksEXT )vkGetDeviceProcAddr( device->vk_device, "vkCmdDrawMeshTasksEXT" );

#if CRUDE_GRAPHICS_VALIDATION_LAYERS_ENABLED
  device->vkSetDebugUtilsObjectNameEXT = ( PFN_vkSetDebugUtilsObjectNameEXT )vkGetDeviceProcAddr( device->vk_device, "vkSetDebugUtilsObjectNameEXT" );
  device->vkCmdBeginDebugUtilsLabelEXT = ( PFN_vkCmdBeginDebugUtilsLabelEXT )vkGetDeviceProcAddr( device->vk_device, "vkCmdBeginDebugUtilsLabelEXT" );
  device->vkCmdEndDebugUtilsLabelEXT = ( PFN_vkCmdEndDebugUtilsLabelEXT )vkGetDeviceProcAddr( device->vk_device, "vkCmdEndDebugUtilsLabelEXT" );
#endif /* CRUDE_GRAPHICS_VALIDATION_LAYERS_ENABLED */

#if CRUDE_GFX_RAY_TRACING_ENABLED
  device->vkGetAccelerationStructureBuildSizesKHR = ( PFN_vkGetAccelerationStructureBuildSizesKHR )vkGetDeviceProcAddr( device->vk_device, "vkGetAccelerationStructureBuildSizesKHR" );
  device->vkCreateAccelerationStructureKHR = ( PFN_vkCreateAccelerationStructureKHR )vkGetDeviceProcAddr( device->vk_device, "vkCreateAccelerationStructureKHR" );
  device->vkCmdBuildAccelerationStructuresKHR = ( PFN_vkCmdBuildAccelerationStructuresKHR )vkGetDeviceProcAddr( device->vk_device, "vkCmdBuildAccelerationStructuresKHR" );
  device->vkGetAccelerationStructureDeviceAddressKHR = ( PFN_vkGetAccelerationStructureDeviceAddressKHR )vkGetDeviceProcAddr( device->vk_device, "vkGetAccelerationStructureDeviceAddressKHR" );
  device->vkCreateRayTracingPipelinesKHR = ( PFN_vkCreateRayTracingPipelinesKHR )vkGetDeviceProcAddr( device->vk_device, "vkCreateRayTracingPipelinesKHR" );
  device->vkGetRayTracingShaderGroupHandlesKHR = ( PFN_vkGetRayTracingShaderGroupHandlesKHR )vkGetDeviceProcAddr( device->vk_device, "vkGetRayTracingShaderGroupHandlesKHR" );
  device->vkCmdTraceRaysKHR = ( PFN_vkCmdTraceRaysKHR )vkGetDeviceProcAddr( device->vk_device, "vkCmdTraceRaysKHR" );
  device->vkDestroyAccelerationStructureKHR = ( PFN_vkDestroyAccelerationStructureKHR )vkGetDeviceProcAddr( device->vk_device, "vkDestroyAccelerationStructureKHR" );
#endif /* CRUDE_GFX_RAY_TRACING_ENABLED */
  
  vma_creation = CRUDE_COMPOUNT_EMPTY( VmaAllocatorCreateInfo );
  vma_creation.physicalDevice = device->vk_physical_device;
  vma_creation.device = device->vk_device;
  vma_creation.instance = instance.vk_instance;
  vma_creation.flags = VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT;
  CRUDE_GFX_RHI_HANDLE_VULKAN_RESULT( vmaCreateAllocator( &vma_creation, &device->vma_allocator ), "Failed to create vma allocator" );
  
  vkGetDeviceQueue( device->vk_device, device->main_queue.vk_queue_family, 0u, &device->main_queue.vk_queue );

  if ( device->transfer_queue.vk_queue_family < vk_queue_family_count )
  {
    vkGetDeviceQueue( device->vk_device, device->transfer_queue.vk_queue_family, 0u, &device->transfer_queue.vk_queue );
  }
  else
  {
    device->transfer_queue = device->main_queue;
  }

  CRUDE_ARRAY_DEINITIALIZE( vk_queue_families );
}

void
crude_gfx_rhi_destroy_device
(
  _In_ crude_gfx_rhi_device                               *device,
  _In_ crude_gfx_rhi_instance                              instance
)
{
  vmaDestroyAllocator( device->vma_allocator );
  vkDestroyDevice( device->vk_device, CRUDE_GFX_RHI_DEVICE_VK_ALLOCATION_CALLBACKS );
}

void
crude_gfx_rhi_create_instance
(
  _Out_ crude_gfx_rhi_instance                            *instance
)
{
#if CRUDE_GRAPHICS_VALIDATION_LAYERS_ENABLED
  PFN_vkCreateDebugUtilsMessengerEXT                       vkCreateDebugUtilsMessengerEXT;
#endif
  VkInstanceCreateInfo                                     vk_instance_creation;
  VkApplicationInfo                                        vk_application;
  VkDebugUtilsMessengerCreateInfoEXT                       vk_debug_creation;
  VkValidationFeaturesEXT                                  vk_validation_features;
  char const                                       *const *surface_extensions_array;
  char const                                              *instance_enabled_extensions[ 256 ];
  uint32                                                   debug_extensions_count, instance_enabled_extension_index, surface_extensions_count;
  
#if CRUDE_GRAPHICS_VALIDATION_LAYERS_ENABLED
  debug_extensions_count = CRUDE_COUNTOF( vk_instance_required_debug_extensions );
#else
  debug_extensions_count = 0u;
#endif

  surface_extensions_array = SDL_Vulkan_GetInstanceExtensions( &surface_extensions_count );
  CRUDE_ASSERT( ( surface_extensions_count + debug_extensions_count ) < CRUDE_COUNTOF( instance_enabled_extensions ) );

  instance_enabled_extension_index = 0u;
  for ( uint32 i = 0; i < surface_extensions_count; ++i )
  {
    instance_enabled_extensions[ instance_enabled_extension_index++ ] = surface_extensions_array[ i ];
  }
  
#if CRUDE_GRAPHICS_VALIDATION_LAYERS_ENABLED
  for ( uint32 i = 0; i < debug_extensions_count; ++i )
  {
    instance_enabled_extensions[ instance_enabled_extension_index++ ] = vk_instance_required_debug_extensions[ i ];
  }
#endif

  vk_application = CRUDE_COMPOUNT_EMPTY( VkApplicationInfo );
  vk_application.pApplicationName = "crude_game";
  vk_application.applicationVersion = VK_MAKE_VERSION( 1, 0, 0 );
  vk_application.pEngineName = "crude_engine";
  vk_application.engineVersion = VK_MAKE_VERSION( 1, 0, 0 );
  vk_application.apiVersion = VK_API_VERSION_1_3;
  
  vk_instance_creation = CRUDE_COMPOUNT_EMPTY( VkInstanceCreateInfo );
  vk_instance_creation.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
  vk_instance_creation.pApplicationInfo = &vk_application;
  vk_instance_creation.flags = 0u;
  vk_instance_creation.ppEnabledExtensionNames = instance_enabled_extensions;
  vk_instance_creation.enabledExtensionCount = instance_enabled_extension_index;

#if CRUDE_GRAPHICS_VALIDATION_LAYERS_ENABLED
  vk_instance_creation.ppEnabledLayerNames = vk_required_debug_layers;
  vk_instance_creation.enabledLayerCount = CRUDE_COUNTOF( vk_required_debug_layers );

#if VK_EXT_debug_utils
  vk_debug_creation = CRUDE_COMPOUNT_EMPTY( VkDebugUtilsMessengerCreateInfoEXT );
  vk_debug_creation.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
  vk_debug_creation.pNext = NULL;
  vk_debug_creation.flags = 0u;
  vk_debug_creation.pfnUserCallback = crude_gfx_rhi_vk_debug_callback_;
  vk_debug_creation.pUserData = NULL;
  vk_debug_creation.messageSeverity =
    VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
    VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT |
    VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
    VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
  vk_debug_creation.messageType =
    VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
    VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
    VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;

  vk_validation_features = CRUDE_COMPOUNT_EMPTY( VkValidationFeaturesEXT );
  vk_validation_features.sType = VK_STRUCTURE_TYPE_VALIDATION_FEATURES_EXT;
  vk_validation_features.pNext = &vk_debug_creation; 
  vk_validation_features.enabledValidationFeatureCount = CRUDE_COUNTOF( vk_features_requested );
  vk_validation_features.pEnabledValidationFeatures = vk_features_requested;

  vk_instance_creation.pNext = &vk_validation_features;
#endif /* VK_EXT_debug_utils */

#endif /* CRUDE_GRAPHICS_VALIDATION_LAYERS_ENABLED */

  CRUDE_GFX_RHI_HANDLE_VULKAN_RESULT( vkCreateInstance( &vk_instance_creation, CRUDE_GFX_RHI_DEVICE_VK_ALLOCATION_CALLBACKS, &instance->vk_instance ), "Failed to create instance" );
  
#if CRUDE_GRAPHICS_VALIDATION_LAYERS_ENABLED
  vkCreateDebugUtilsMessengerEXT = ( PFN_vkCreateDebugUtilsMessengerEXT )vkGetInstanceProcAddr( instance->vk_instance, "vkCreateDebugUtilsMessengerEXT" );
  CRUDE_GFX_RHI_HANDLE_VULKAN_RESULT( vkCreateDebugUtilsMessengerEXT( instance->vk_instance, &vk_debug_creation, CRUDE_GFX_RHI_DEVICE_VK_ALLOCATION_CALLBACKS, &instance->vk_debug_utils_messenger ), "Failed to create debug utils messenger" );
#endif
}

void
crude_gfx_rhi_destroy_instance
(
  _In_ crude_gfx_rhi_instance                              instance
)
{

#if CRUDE_GRAPHICS_VALIDATION_LAYERS_ENABLED
  PFN_vkDestroyDebugUtilsMessengerEXT                      vkDestroyDebugUtilsMessengerEXT;
  
  vkDestroyDebugUtilsMessengerEXT = ( PFN_vkDestroyDebugUtilsMessengerEXT )vkGetInstanceProcAddr( instance.vk_instance, "vkDestroyDebugUtilsMessengerEXT" );
  vkDestroyDebugUtilsMessengerEXT( instance.vk_instance, instance.vk_debug_utils_messenger, CRUDE_GFX_RHI_DEVICE_VK_ALLOCATION_CALLBACKS );
#endif /* CRUDE_GRAPHICS_VALIDATION_LAYERS_ENABLED */

  vkDestroyInstance( instance.vk_instance, CRUDE_GFX_RHI_DEVICE_VK_ALLOCATION_CALLBACKS );
}

void
crude_gfx_rhi_create_buffer
(
  _In_ crude_gfx_rhi_device                               *device,
  _In_ crude_gfx_rhi_buffer_create_info const             *creation,
  _Out_ crude_gfx_rhi_buffer                              *buffer
)
{
  VkBufferCreateInfo                                       vk_creation;
  VmaAllocationCreateInfo                                  vma_creation;
  VmaAllocationInfo                                        vma_allocation_info;
  
  vk_creation = CRUDE_COMPOUNT_EMPTY( VkBufferCreateInfo );
  vk_creation.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
  vk_creation.size = creation->size;
  vk_creation.usage = creation->usage;
  
  vma_creation = CRUDE_COMPOUNT_EMPTY( VmaAllocationCreateInfo );
  vma_creation.flags = VMA_ALLOCATION_CREATE_STRATEGY_BEST_FIT_BIT;

  if ( creation->persistent )
  {
    vma_creation.flags |= VMA_ALLOCATION_CREATE_MAPPED_BIT;
  }

  if ( creation->device_only )
  {
    vma_creation.usage = VMA_MEMORY_USAGE_GPU_ONLY;
  }
  else
  {
    vma_creation.usage = VMA_MEMORY_USAGE_GPU_TO_CPU;
  }
  
  CRUDE_GFX_RHI_HANDLE_VULKAN_RESULT( vmaCreateBuffer( device->vma_allocator, &vk_creation, &vma_creation, &buffer->vk_buffer, &buffer->vma_allocation, &vma_allocation_info ),  "Failed to create buffer" );
  
  if ( creation->persistent )
  {
    buffer->mapped_data = vma_allocation_info.pMappedData;
  }
}

void
crude_gfx_rhi_destroy_buffer
(
  _In_ crude_gfx_rhi_device                               *device,
  _In_ crude_gfx_rhi_buffer                                buffer
)
{
  vmaDestroyBuffer( device->vma_allocator, buffer.vk_buffer, buffer.vma_allocation );
}

void
crude_gfx_rhi_set_buffer_debug_name
(
  _In_ crude_gfx_rhi_device                               *device,
  _In_ crude_gfx_rhi_buffer                                buffer,
  _In_ char const                                         *name
)
{
  crude_gfx_rhi_set_debug_utils_object_name( device, CRUDE_GFX_RHI_OBJECT_TYPE_BUFFER, CRUDE_CAST( uint64, buffer.vk_buffer ), name );
}

void
crude_gfx_rhi_map_buffer
(
  _In_ crude_gfx_rhi_device                               *device,
  _In_ crude_gfx_rhi_buffer                                buffer,
  _Out_ void                                             **data
)
{
  CRUDE_GFX_RHI_HANDLE_VULKAN_RESULT( vmaMapMemory( device->vma_allocator, buffer.vma_allocation, data ), "Failed vmaMapMemory" );
}

void
crude_gfx_rhi_unmap_buffer
(
  _In_ crude_gfx_rhi_device                               *device,
  _In_ crude_gfx_rhi_buffer                                buffer
)
{
  vmaUnmapMemory( device->vma_allocator, buffer.vma_allocation );
}

void
crude_gfx_rhi_create_image
(
  _In_ crude_gfx_rhi_device                               *device,
  _In_ crude_gfx_rhi_image_create_info const              *creation,
  _Out_ crude_gfx_rhi_image                               *image
)
{
  VmaAllocationCreateInfo                                  vma_creation;
  VkImageCreateInfo                                        vk_creation;
  bool                                                     is_render_target, is_compute_used;

  vk_creation = CRUDE_COMPOUNT_EMPTY( VkImageCreateInfo );
  vk_creation.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
  vk_creation.imageType = CRUDE_CAST( VkImageType, creation->image_type );
  vk_creation.format = CRUDE_CAST( VkFormat, creation->format );
  vk_creation.extent.width = creation->extent.x;
  vk_creation.extent.height = creation->extent.y;
  vk_creation.extent.depth = creation->extent.z;
  vk_creation.mipLevels = creation->mip_levels;
  vk_creation.arrayLayers = creation->array_layers;
  vk_creation.tiling = CRUDE_CAST( VkImageTiling, creation->tiling );
  vk_creation.sharingMode = CRUDE_CAST( VkSharingMode, creation->sharing_mode );
  vk_creation.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  vk_creation.samples = CRUDE_CAST( VkSampleCountFlagBits, creation->samples );
  vk_creation.usage = creation->usage;
  
  if ( creation->alias_image )
  {
    image->vma_allocation = 0;
    CRUDE_GFX_RHI_HANDLE_VULKAN_RESULT( vmaCreateAliasingImage( device->vma_allocator, creation->alias_image->vma_allocation, &vk_creation, &image->vk_image ), "Failed to create aliasing image!" );
  }
  else
  {
    vma_creation = CRUDE_COMPOUNT_EMPTY( VmaAllocationCreateInfo );
    vma_creation.usage = VMA_MEMORY_USAGE_GPU_ONLY;
    
    CRUDE_GFX_RHI_HANDLE_VULKAN_RESULT( vmaCreateImage( device->vma_allocator, &vk_creation, &vma_creation, &image->vk_image, &image->vma_allocation, NULL ), "Failed to create image!" );
  }
}

void
crude_gfx_rhi_destroy_image
(
  _In_ crude_gfx_rhi_device                               *device,
  _In_ crude_gfx_rhi_image                                 image
)
{
  vmaDestroyImage( device->vma_allocator, image.vk_image, image.vma_allocation );
}

void
crude_gfx_rhi_set_image_allocation_debug_name
(
  _In_ crude_gfx_rhi_device                               *device,
  _In_ crude_gfx_rhi_image                                 image,
  _In_ char const                                         *name
)
{
#if CRUDE_GRAPHICS_VALIDATION_LAYERS_ENABLED
  vmaSetAllocationName( device->vma_allocator, image.vma_allocation, name );
#endif /* CRUDE_GRAPHICS_VALIDATION_LAYERS_ENABLED */
}

void
crude_gfx_rhi_set_image_debug_name
(
  _In_ crude_gfx_rhi_device                               *device,
  _In_ crude_gfx_rhi_image                                 image,
  _In_ char const                                         *name
)
{
  crude_gfx_rhi_set_debug_utils_object_name( device, CRUDE_GFX_RHI_OBJECT_TYPE_IMAGE, CRUDE_CAST( uint64, image.vk_image ), name );
}

void
crude_gfx_rhi_create_image_view
(
  _In_ crude_gfx_rhi_device                               *device,
  _In_ crude_gfx_rhi_image_view_create_info const         *creation,
  _Out_ crude_gfx_rhi_image_view                          *image_view
)
{
  VkImageViewCreateInfo                                    vk_creation;
  
  vk_creation = CRUDE_COMPOUNT_EMPTY( VkImageViewCreateInfo );
  vk_creation.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
  vk_creation.flags = creation->flags;
  vk_creation.image = creation->image.vk_image;
  vk_creation.viewType = CRUDE_CAST( VkImageViewType, creation->view_type );
  vk_creation.format = CRUDE_CAST( VkFormat, creation->format );
  vk_creation.components.r = CRUDE_CAST( VkComponentSwizzle, creation->components.r );
  vk_creation.components.g = CRUDE_CAST( VkComponentSwizzle, creation->components.g );
  vk_creation.components.b = CRUDE_CAST( VkComponentSwizzle, creation->components.b );
  vk_creation.components.a = CRUDE_CAST( VkComponentSwizzle, creation->components.a );
  vk_creation.subresourceRange.aspectMask = creation->subresource_range.aspect_mask;
  vk_creation.subresourceRange.baseArrayLayer = creation->subresource_range.base_array_layer;
  vk_creation.subresourceRange.baseMipLevel = creation->subresource_range.base_mip_level;
  vk_creation.subresourceRange.layerCount = creation->subresource_range.layer_count;
  vk_creation.subresourceRange.levelCount = creation->subresource_range.level_count;
  
  CRUDE_GFX_RHI_HANDLE_VULKAN_RESULT( vkCreateImageView( device->vk_device, &vk_creation, CRUDE_GFX_RHI_DEVICE_VK_ALLOCATION_CALLBACKS, &image_view->vk_image_view ), "Failed to create image view!" );
}

void
crude_gfx_rhi_destroy_image_view
(
  _In_ crude_gfx_rhi_device                               *device,
  _In_ crude_gfx_rhi_image_view                            image_view
)
{
  vkDestroyImageView( device->vk_device, image_view.vk_image_view, CRUDE_GFX_RHI_DEVICE_VK_ALLOCATION_CALLBACKS );
}

void
crude_gfx_rhi_set_image_view_debug_name
(
  _In_ crude_gfx_rhi_device                               *device,
  _In_ crude_gfx_rhi_image_view                            image_view,
  _In_ char const                                         *name
)
{
  crude_gfx_rhi_set_debug_utils_object_name( device, CRUDE_GFX_RHI_OBJECT_TYPE_IMAGE_VIEW, CRUDE_CAST( uint64, image_view.vk_image_view ), name );
}

void
crude_gfx_rhi_create_sampler
(
  _In_ crude_gfx_rhi_device                               *device,
  _In_ crude_gfx_rhi_sampler_create_info const            *creation,
  _Out_ crude_gfx_rhi_sampler                             *sampler
)
{
  VkSamplerCreateInfo                                      vk_sampler_create_info;
  VkSamplerReductionModeCreateInfoEXT                      create_info_reduction;

  vk_sampler_create_info = CRUDE_COMPOUNT_EMPTY( VkSamplerCreateInfo );
  vk_sampler_create_info.sType                   = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
  vk_sampler_create_info.magFilter               = CRUDE_CAST( VkFilter, creation->mag_filter );
  vk_sampler_create_info.minFilter               = CRUDE_CAST( VkFilter, creation->min_filter );
  vk_sampler_create_info.mipmapMode              = CRUDE_CAST( VkSamplerMipmapMode, creation->mipmap_mode );
  vk_sampler_create_info.addressModeU            = CRUDE_CAST( VkSamplerAddressMode, creation->address_mode_u );
  vk_sampler_create_info.addressModeV            = CRUDE_CAST( VkSamplerAddressMode, creation->address_mode_v );
  vk_sampler_create_info.addressModeW            = CRUDE_CAST( VkSamplerAddressMode, creation->address_mode_w );
  vk_sampler_create_info.anisotropyEnable        = creation->anisotropy_enable;
  vk_sampler_create_info.compareEnable           = creation->compare_enable;
  vk_sampler_create_info.borderColor             = CRUDE_CAST( VkBorderColor, creation->border_color );
  vk_sampler_create_info.unnormalizedCoordinates = creation->unnormalized_coordinates;
  vk_sampler_create_info.minLod                  = creation->min_lod;
  vk_sampler_create_info.maxLod                  = creation->max_lod;

  create_info_reduction = CRUDE_COMPOUNT_EMPTY( VkSamplerReductionModeCreateInfoEXT );
  create_info_reduction.sType = VK_STRUCTURE_TYPE_SAMPLER_REDUCTION_MODE_CREATE_INFO_EXT;
  if ( creation->reduction_mode != VK_SAMPLER_REDUCTION_MODE_WEIGHTED_AVERAGE_EXT )
  {
    create_info_reduction.reductionMode = CRUDE_CAST( VkSamplerReductionMode, creation->reduction_mode );
    vk_sampler_create_info.pNext = &create_info_reduction;
  }

  CRUDE_GFX_RHI_HANDLE_VULKAN_RESULT( vkCreateSampler( device->vk_device, &vk_sampler_create_info, CRUDE_GFX_RHI_DEVICE_VK_ALLOCATION_CALLBACKS, &sampler->vk_sampler ), "Failed vkCreateSampler" );  
}

void
crude_gfx_rhi_destroy_sampler
(
  _In_ crude_gfx_rhi_device                               *device,
  _In_ crude_gfx_rhi_sampler                               sampler
)
{
  vkDestroySampler( device->vk_device, sampler.vk_sampler, CRUDE_GFX_RHI_DEVICE_VK_ALLOCATION_CALLBACKS );
}

void
crude_gfx_rhi_set_sampler_debug_name
(
  _In_ crude_gfx_rhi_device                               *device,
  _In_ crude_gfx_rhi_sampler                               sampler,
  _In_ char const                                         *name
)
{
  crude_gfx_rhi_set_debug_utils_object_name( device, CRUDE_GFX_RHI_OBJECT_TYPE_SAMPLER, CRUDE_CAST( uint64, sampler.vk_sampler ), name );
}

bool
crude_gfx_rhi_create_shader_module
(
  _In_ crude_gfx_rhi_device                               *device,
  _In_ crude_gfx_rhi_shader_module_create_info const      *creation,
  _In_ crude_heap_allocator                               *allocator,
  _Out_ crude_gfx_rhi_shader_module                       *shader_module
)
{
  VkShaderModuleCreateInfo                                 vk_creation;
  VkResult                                                 vk_result;

  vk_creation = CRUDE_COMPOUNT_EMPTY( VkShaderModuleCreateInfo );
  vk_creation.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
  vk_creation.flags = creation->flags;
  vk_creation.pCode = creation->code;
  vk_creation.codeSize = creation->code_size;

  vk_result = vkCreateShaderModule( device->vk_device, &vk_creation, CRUDE_GFX_RHI_DEVICE_VK_ALLOCATION_CALLBACKS, &shader_module->vk_shader_module );
  return vk_result == VK_SUCCESS;
}

void
crude_gfx_rhi_destroy_shader_module
(
  _In_ crude_gfx_rhi_device                               *device,
  _In_ crude_gfx_rhi_shader_module                         shader_module
)
{
  vkDestroyShaderModule( device->vk_device, shader_module.vk_shader_module, CRUDE_GFX_RHI_DEVICE_VK_ALLOCATION_CALLBACKS );
}

void
crude_gfx_rhi_set_shader_module_debug_name
(
  _In_ crude_gfx_rhi_device                               *device,
  _Out_ crude_gfx_rhi_shader_module                        shader_module,
  _In_ char const                                         *name
)
{
  crude_gfx_rhi_set_debug_utils_object_name( device, CRUDE_GFX_RHI_OBJECT_TYPE_SHADER_MODULE, CRUDE_CAST( uint64, shader_module.vk_shader_module ), name );
}

void
crude_gfx_rhi_create_pipeline_layout
(
  _In_ crude_gfx_rhi_device                               *device,
  _In_ crude_gfx_rhi_pipeline_layout_create_info const    *creation,
  _Out_ crude_gfx_rhi_pipeline_layout                     *pipeline_layout
)
{
  VkDescriptorSetLayout                                    vk_layouts[ 4 ];
  VkPushConstantRange                                      vk_push_constant;
  VkPipelineLayoutCreateInfo                               vk_creation;

  CRUDE_ASSERT( creation->set_layout_count < CRUDE_COUNTOF( vk_layouts ) );
  for ( uint32 i = 0; i < creation->set_layout_count; ++i )
  {
    vk_layouts[ i ] = creation->set_layouts[ i ].vk_descriptor_set_layout;
  }

  vk_push_constant = CRUDE_COMPOUNT_EMPTY( VkPushConstantRange );
  vk_push_constant.offset = creation->push_constant_range.offset;
  vk_push_constant.size = creation->push_constant_range.size;
  vk_push_constant.stageFlags = creation->push_constant_range.stage_flags;

  vk_creation = CRUDE_COMPOUNT_EMPTY( VkPipelineLayoutCreateInfo );
  vk_creation.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
  vk_creation.setLayoutCount = creation->set_layout_count;
  vk_creation.pSetLayouts = vk_layouts;
  vk_creation.pushConstantRangeCount = creation->has_push_constant_range ? 1 : 0;
  vk_creation.pPushConstantRanges = &vk_push_constant;
  CRUDE_GFX_RHI_HANDLE_VULKAN_RESULT( vkCreatePipelineLayout( device->vk_device, &vk_creation, CRUDE_GFX_RHI_DEVICE_VK_ALLOCATION_CALLBACKS, &pipeline_layout->vk_pipeline_layout ), "Failed to create pipeline layout" );
}

void
crude_gfx_rhi_destroy_pipeline_layout
(
  _In_ crude_gfx_rhi_device                               *device,
  _In_ crude_gfx_rhi_pipeline_layout                       pipeline_layout
)
{
  vkDestroyPipelineLayout( device->vk_device, pipeline_layout.vk_pipeline_layout, CRUDE_GFX_RHI_DEVICE_VK_ALLOCATION_CALLBACKS );
}

void
crude_gfx_rhi_set_pipeline_layout_debug_name
(
  _In_ crude_gfx_rhi_device                               *device,
  _In_ crude_gfx_rhi_pipeline_layout                       pipeline_layout,
  _In_ char const                                         *name
)
{
  crude_gfx_rhi_set_debug_utils_object_name( device, CRUDE_GFX_RHI_OBJECT_TYPE_PIPELINE_LAYOUT, CRUDE_CAST( uint64, pipeline_layout.vk_pipeline_layout ), name );
}

void
crude_gfx_rhi_create_task_pipeline
(
  _In_ crude_gfx_rhi_device                               *device,
  _In_ crude_gfx_rhi_task_pipeline_create_info const      *creation,
  _Out_ crude_gfx_rhi_pipeline                            *pipeline
)
{
  VkDynamicState                                           vk_dynamic_states[ ] = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };

  VkGraphicsPipelineCreateInfo                             vk_pipeline_info;  
  VkPipelineInputAssemblyStateCreateInfo                   vk_input_assembly;
  VkPipelineViewportStateCreateInfo                        vk_viewport_state;
  VkPipelineDynamicStateCreateInfo                         vk_dynamic_state;
  VkPipelineRenderingCreateInfoKHR                         vk_pipeline_rendering_create_info;
  VkPipelineRasterizationStateCreateInfo                   vk_rasterizer;
  VkPipelineMultisampleStateCreateInfo                     vk_multisampling;
  VkPipelineDepthStencilStateCreateInfo                    vk_depth_stencil;
  VkPipelineColorBlendStateCreateInfo                      vk_color_blending;
  VkPipelineColorBlendAttachmentState                      vk_color_blend_attachment[ 8 ];
  VkFormat                                                 vk_color_attachment_formats[ 8 ];
  VkPipelineShaderStageCreateInfo                          vk_stages[ 8 ];

  vk_input_assembly = CRUDE_COMPOUNT_EMPTY( VkPipelineInputAssemblyStateCreateInfo );
  vk_input_assembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
  vk_input_assembly.topology = CRUDE_CAST( VkPrimitiveTopology, creation->input_assembly_state->topology );
  vk_input_assembly.primitiveRestartEnable = creation->input_assembly_state->primitive_restart_enable;

  for ( uint32 i = 0; i < creation->color_blend_state->attachments_count; ++i )
  {
    vk_color_blend_attachment[ i ] = CRUDE_COMPOUNT_EMPTY( VkPipelineColorBlendAttachmentState );
    vk_color_blend_attachment[ i ].blendEnable = creation->color_blend_state->attachments[ i ].blend_enable;
    vk_color_blend_attachment[ i ].colorWriteMask = creation->color_blend_state->attachments[ i ].color_write_mask;
    vk_color_blend_attachment[ i ].srcColorBlendFactor = CRUDE_CAST( VkBlendFactor, creation->color_blend_state->attachments[ i ].src_color_blend_factor );
    vk_color_blend_attachment[ i ].dstColorBlendFactor = CRUDE_CAST( VkBlendFactor, creation->color_blend_state->attachments[ i ].dst_color_blend_factor );
    vk_color_blend_attachment[ i ].colorBlendOp = CRUDE_CAST( VkBlendOp, creation->color_blend_state->attachments[ i ].color_blend_op );
    vk_color_blend_attachment[ i ].srcAlphaBlendFactor = CRUDE_CAST( VkBlendFactor, creation->color_blend_state->attachments[ i ].src_alpha_blend_factor );
    vk_color_blend_attachment[ i ].dstAlphaBlendFactor = CRUDE_CAST( VkBlendFactor, creation->color_blend_state->attachments[ i ].dst_alpha_blend_factor );
    vk_color_blend_attachment[ i ].alphaBlendOp = CRUDE_CAST( VkBlendOp, creation->color_blend_state->attachments[ i ].alpha_blend_op );
  }
    
  vk_color_blending = CRUDE_COMPOUNT_EMPTY( VkPipelineColorBlendStateCreateInfo );
  vk_color_blending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
  vk_color_blending.logicOpEnable = creation->color_blend_state->logic_op_enable ? VK_TRUE : VK_FALSE;
  vk_color_blending.logicOp = CRUDE_CAST( VkLogicOp, creation->color_blend_state->logic_op );
  vk_color_blending.attachmentCount = creation->color_blend_state->attachments_count;
  vk_color_blending.pAttachments = vk_color_blend_attachment;
  vk_color_blending.blendConstants[ 0 ] = creation->color_blend_state->blend_constants[ 0 ];
  vk_color_blending.blendConstants[ 1 ] = creation->color_blend_state->blend_constants[ 1 ];
  vk_color_blending.blendConstants[ 2 ] = creation->color_blend_state->blend_constants[ 2 ];
  vk_color_blending.blendConstants[ 3 ] = creation->color_blend_state->blend_constants[ 3 ];
    
  vk_depth_stencil = CRUDE_COMPOUNT_EMPTY( VkPipelineDepthStencilStateCreateInfo );
  vk_depth_stencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
  vk_depth_stencil.depthTestEnable = creation->depth_stencil_state->depth_test_enable ? VK_TRUE : VK_FALSE;
  vk_depth_stencil.depthWriteEnable = creation->depth_stencil_state->depth_write_enable ? VK_TRUE : VK_FALSE;
  vk_depth_stencil.depthCompareOp = CRUDE_CAST( VkCompareOp, creation->depth_stencil_state->depth_compare_op );
  vk_depth_stencil.stencilTestEnable = creation->depth_stencil_state->stencil_test_enable ? VK_TRUE : VK_FALSE;

  vk_multisampling = CRUDE_COMPOUNT_EMPTY( VkPipelineMultisampleStateCreateInfo );
  vk_multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
  vk_multisampling.rasterizationSamples = CRUDE_CAST( VkSampleCountFlagBits, creation->multisample_state->rasterization_samples );
  vk_multisampling.pSampleMask = NULL;
  vk_multisampling.alphaToCoverageEnable = creation->multisample_state->alpha_to_coverage_enable ? VK_TRUE : VK_FALSE;
  vk_multisampling.alphaToOneEnable = creation->multisample_state->alpha_to_one_enable ? VK_TRUE : VK_FALSE;
  vk_multisampling.sampleShadingEnable = creation->multisample_state->sample_shading_enable ? VK_TRUE : VK_FALSE;
  vk_multisampling.minSampleShading = creation->multisample_state->min_sample_shading;
    
  vk_rasterizer = CRUDE_COMPOUNT_EMPTY( VkPipelineRasterizationStateCreateInfo );
  vk_rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
  vk_rasterizer.depthClampEnable = creation->rasterization_state->depth_clamp_enable ? VK_TRUE : VK_FALSE;
  vk_rasterizer.rasterizerDiscardEnable = creation->rasterization_state->rasterizer_discard_enable ? VK_TRUE : VK_FALSE;
  vk_rasterizer.polygonMode = CRUDE_CAST( VkPolygonMode, creation->rasterization_state->polygon_mode );
  vk_rasterizer.cullMode = CRUDE_STATIC_CAST( VkCullModeFlags, creation->rasterization_state->cull_mode );
  vk_rasterizer.frontFace = CRUDE_CAST( VkFrontFace, creation->rasterization_state->front_face );
  vk_rasterizer.depthBiasEnable = creation->rasterization_state->depth_bias_enable ? VK_TRUE : VK_FALSE;
  vk_rasterizer.depthBiasConstantFactor = creation->rasterization_state->depth_bias_constant_factor;
  vk_rasterizer.depthBiasClamp = creation->rasterization_state->depth_bias_clamp;
  vk_rasterizer.depthBiasSlopeFactor = creation->rasterization_state->depth_bias_slope_factor;
  vk_rasterizer.lineWidth = creation->rasterization_state->line_width;
    
  vk_viewport_state = CRUDE_COMPOUNT_EMPTY( VkPipelineViewportStateCreateInfo ); 
  vk_viewport_state.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
  vk_viewport_state.viewportCount = creation->viewport_state->viewport_count;
  vk_viewport_state.scissorCount = creation->viewport_state->scissor_count;
    
  vk_dynamic_state = CRUDE_COMPOUNT_EMPTY( VkPipelineDynamicStateCreateInfo ); 
  vk_dynamic_state.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
  vk_dynamic_state.dynamicStateCount = CRUDE_COUNTOF( vk_dynamic_states );
  vk_dynamic_state.pDynamicStates = vk_dynamic_states;
    
  CRUDE_ASSERT( creation->rendering_state->color_attachment_count < CRUDE_COUNTOF( vk_color_attachment_formats ) );
  for ( uint32 i = 0; i < creation->rendering_state->color_attachment_count; ++i )
  {
    vk_color_attachment_formats[ i ] = CRUDE_CAST( VkFormat, creation->rendering_state->color_attachment_formats[ i ] );
  }

  vk_pipeline_rendering_create_info = CRUDE_COMPOUNT_EMPTY( VkPipelineRenderingCreateInfoKHR );
  vk_pipeline_rendering_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO_KHR;
  vk_pipeline_rendering_create_info.viewMask = creation->rendering_state->view_mask;
  vk_pipeline_rendering_create_info.colorAttachmentCount = creation->rendering_state->color_attachment_count;
  vk_pipeline_rendering_create_info.pColorAttachmentFormats = vk_color_attachment_formats;
  vk_pipeline_rendering_create_info.depthAttachmentFormat = CRUDE_CAST( VkFormat, creation->rendering_state->depth_attachment_format );
  vk_pipeline_rendering_create_info.stencilAttachmentFormat = CRUDE_CAST( VkFormat, creation->rendering_state->stencil_attachment_format );
  
  CRUDE_ASSERT( creation->stage_count < CRUDE_COUNTOF( vk_stages ) );
  for ( uint32 i = 0; i < creation->stage_count; ++i )
  {
    vk_stages[ i ] = CRUDE_COMPOUNT_EMPTY( VkPipelineShaderStageCreateInfo );
    vk_stages[ i ].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vk_stages[ i ].module = creation->stages[ i ].rhi_module.vk_shader_module;
    vk_stages[ i ].stage  = CRUDE_CAST( VkShaderStageFlagBits, creation->stages[ i ].stage );
    vk_stages[ i ].pName  = creation->stages[ i ].name;
  }

  vk_pipeline_info = CRUDE_COMPOUNT_EMPTY( VkGraphicsPipelineCreateInfo );
  vk_pipeline_info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
  vk_pipeline_info.pNext = &vk_pipeline_rendering_create_info;
  vk_pipeline_info.stageCount = creation->stage_count;
  vk_pipeline_info.pStages = vk_stages;
  vk_pipeline_info.pVertexInputState = NULL;
  vk_pipeline_info.pInputAssemblyState = &vk_input_assembly;
  vk_pipeline_info.pViewportState = &vk_viewport_state;
  vk_pipeline_info.pRasterizationState = &vk_rasterizer;
  vk_pipeline_info.pMultisampleState = &vk_multisampling;
  vk_pipeline_info.pDepthStencilState = &vk_depth_stencil;
  vk_pipeline_info.pColorBlendState = &vk_color_blending;
  vk_pipeline_info.pDynamicState = &vk_dynamic_state;
  vk_pipeline_info.layout = creation->pipeline_layout.vk_pipeline_layout;
  vk_pipeline_info.renderPass = NULL;
    
  CRUDE_GFX_RHI_HANDLE_VULKAN_RESULT( vkCreateGraphicsPipelines( device->vk_device, VK_NULL_HANDLE, 1, &vk_pipeline_info, CRUDE_GFX_RHI_DEVICE_VK_ALLOCATION_CALLBACKS, &pipeline->vk_pipeline ), "Failed to create task pipeline" );
}

void
crude_gfx_rhi_create_classic_pipeline
(
  _In_ crude_gfx_rhi_device                               *device,
  _In_ crude_gfx_rhi_classic_pipeline_create_info const   *creation,
  _Out_ crude_gfx_rhi_pipeline                            *pipeline
)
{
  VkDynamicState                                           vk_dynamic_states[ ] = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };

  VkGraphicsPipelineCreateInfo                             vk_pipeline_info;  
  VkPipelineInputAssemblyStateCreateInfo                   vk_input_assembly;
  VkPipelineViewportStateCreateInfo                        vk_viewport_state;
  VkPipelineDynamicStateCreateInfo                         vk_dynamic_state;
  VkPipelineRenderingCreateInfoKHR                         vk_pipeline_rendering_create_info;
  VkPipelineRasterizationStateCreateInfo                   vk_rasterizer;
  VkPipelineMultisampleStateCreateInfo                     vk_multisampling;
  VkPipelineDepthStencilStateCreateInfo                    vk_depth_stencil;
  VkPipelineColorBlendStateCreateInfo                      vk_color_blending;
  VkPipelineVertexInputStateCreateInfo                     vk_vertex_input_info;
  VkVertexInputAttributeDescription                        vk_vertex_attributes[ 8 ];
  VkVertexInputBindingDescription                          vk_vertex_bindings[ 8 ];
  VkPipelineColorBlendAttachmentState                      vk_color_blend_attachment[ 8 ];
  VkFormat                                                 vk_color_attachment_formats[ 8 ];
  VkPipelineShaderStageCreateInfo                          vk_stages[ 8 ];

  vk_vertex_input_info = CRUDE_COMPOUNT_EMPTY( VkPipelineVertexInputStateCreateInfo );
  vk_vertex_input_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    
  for ( uint32 i = 0; i < creation->vertex_input_state->vertex_attribute_description_count; ++i )
  {
    vk_vertex_attributes[ i ] = CRUDE_COMPOUNT_EMPTY( VkVertexInputAttributeDescription );
    vk_vertex_attributes[ i ].location = creation->vertex_input_state->vertex_attribute_descriptions[ i ].location;
    vk_vertex_attributes[ i ].binding = creation->vertex_input_state->vertex_attribute_descriptions[ i ].binding;
    vk_vertex_attributes[ i ].format = CRUDE_CAST( VkFormat, creation->vertex_input_state->vertex_attribute_descriptions[ i ].format );
    vk_vertex_attributes[ i ].offset = creation->vertex_input_state->vertex_attribute_descriptions[ i ].offset;
  }
  vk_vertex_input_info.vertexAttributeDescriptionCount = creation->vertex_input_state->vertex_attribute_description_count;
  vk_vertex_input_info.pVertexAttributeDescriptions = vk_vertex_attributes;
  
  for ( uint32 i = 0; i < creation->vertex_input_state->vertex_binding_description_count; ++i )
  {
    vk_vertex_bindings[ i ] = CRUDE_COMPOUNT_EMPTY( VkVertexInputBindingDescription );
    vk_vertex_bindings[ i ].binding = creation->vertex_input_state->vertex_binding_descriptions[ i ].binding;
    vk_vertex_bindings[ i ].stride = creation->vertex_input_state->vertex_binding_descriptions[ i ].stride;
    vk_vertex_bindings[ i ].inputRate = CRUDE_CAST( VkVertexInputRate, creation->vertex_input_state->vertex_binding_descriptions[ i ].input_rate );
  }
  vk_vertex_input_info.vertexBindingDescriptionCount = creation->vertex_input_state->vertex_binding_description_count;
  vk_vertex_input_info.pVertexBindingDescriptions = vk_vertex_bindings;

  vk_input_assembly = CRUDE_COMPOUNT_EMPTY( VkPipelineInputAssemblyStateCreateInfo );
  vk_input_assembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
  vk_input_assembly.topology = CRUDE_CAST( VkPrimitiveTopology, creation->input_assembly_state->topology );
  vk_input_assembly.primitiveRestartEnable = creation->input_assembly_state->primitive_restart_enable;

  for ( uint32 i = 0; i < creation->color_blend_state->attachments_count; ++i )
  {
    vk_color_blend_attachment[ i ] = CRUDE_COMPOUNT_EMPTY( VkPipelineColorBlendAttachmentState );
    vk_color_blend_attachment[ i ].blendEnable = creation->color_blend_state->attachments[ i ].blend_enable;
    vk_color_blend_attachment[ i ].colorWriteMask = creation->color_blend_state->attachments[ i ].color_write_mask;
    vk_color_blend_attachment[ i ].srcColorBlendFactor = CRUDE_CAST( VkBlendFactor, creation->color_blend_state->attachments[ i ].src_color_blend_factor );
    vk_color_blend_attachment[ i ].dstColorBlendFactor = CRUDE_CAST( VkBlendFactor, creation->color_blend_state->attachments[ i ].dst_color_blend_factor );
    vk_color_blend_attachment[ i ].colorBlendOp = CRUDE_CAST( VkBlendOp, creation->color_blend_state->attachments[ i ].color_blend_op );
    vk_color_blend_attachment[ i ].srcAlphaBlendFactor = CRUDE_CAST( VkBlendFactor, creation->color_blend_state->attachments[ i ].src_alpha_blend_factor );
    vk_color_blend_attachment[ i ].dstAlphaBlendFactor = CRUDE_CAST( VkBlendFactor, creation->color_blend_state->attachments[ i ].dst_alpha_blend_factor );
    vk_color_blend_attachment[ i ].alphaBlendOp = CRUDE_CAST( VkBlendOp, creation->color_blend_state->attachments[ i ].alpha_blend_op );
  }
    
  vk_color_blending = CRUDE_COMPOUNT_EMPTY( VkPipelineColorBlendStateCreateInfo );
  vk_color_blending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
  vk_color_blending.logicOpEnable = creation->color_blend_state->logic_op_enable ? VK_TRUE : VK_FALSE;
  vk_color_blending.logicOp = CRUDE_CAST( VkLogicOp, creation->color_blend_state->logic_op );
  vk_color_blending.attachmentCount = creation->color_blend_state->attachments_count;
  vk_color_blending.pAttachments = vk_color_blend_attachment;
  vk_color_blending.blendConstants[ 0 ] = creation->color_blend_state->blend_constants[ 0 ];
  vk_color_blending.blendConstants[ 1 ] = creation->color_blend_state->blend_constants[ 1 ];
  vk_color_blending.blendConstants[ 2 ] = creation->color_blend_state->blend_constants[ 2 ];
  vk_color_blending.blendConstants[ 3 ] = creation->color_blend_state->blend_constants[ 3 ];
    
  vk_depth_stencil = CRUDE_COMPOUNT_EMPTY( VkPipelineDepthStencilStateCreateInfo );
  vk_depth_stencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
  vk_depth_stencil.depthTestEnable = creation->depth_stencil_state->depth_test_enable ? VK_TRUE : VK_FALSE;
  vk_depth_stencil.depthWriteEnable = creation->depth_stencil_state->depth_write_enable ? VK_TRUE : VK_FALSE;
  vk_depth_stencil.depthCompareOp = CRUDE_CAST( VkCompareOp, creation->depth_stencil_state->depth_compare_op );
  vk_depth_stencil.stencilTestEnable = creation->depth_stencil_state->stencil_test_enable ? VK_TRUE : VK_FALSE;

  vk_multisampling = CRUDE_COMPOUNT_EMPTY( VkPipelineMultisampleStateCreateInfo );
  vk_multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
  vk_multisampling.rasterizationSamples = CRUDE_CAST( VkSampleCountFlagBits, creation->multisample_state->rasterization_samples );
  vk_multisampling.pSampleMask = NULL;
  vk_multisampling.alphaToCoverageEnable = creation->multisample_state->alpha_to_coverage_enable ? VK_TRUE : VK_FALSE;
  vk_multisampling.alphaToOneEnable = creation->multisample_state->alpha_to_one_enable ? VK_TRUE : VK_FALSE;
  vk_multisampling.sampleShadingEnable = creation->multisample_state->sample_shading_enable ? VK_TRUE : VK_FALSE;
  vk_multisampling.minSampleShading = creation->multisample_state->min_sample_shading;
    
  vk_rasterizer = CRUDE_COMPOUNT_EMPTY( VkPipelineRasterizationStateCreateInfo );
  vk_rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
  vk_rasterizer.depthClampEnable = creation->rasterization_state->depth_clamp_enable ? VK_TRUE : VK_FALSE;
  vk_rasterizer.rasterizerDiscardEnable = creation->rasterization_state->rasterizer_discard_enable ? VK_TRUE : VK_FALSE;
  vk_rasterizer.polygonMode = CRUDE_CAST( VkPolygonMode, creation->rasterization_state->polygon_mode );
  vk_rasterizer.cullMode = CRUDE_STATIC_CAST( VkCullModeFlags, creation->rasterization_state->cull_mode );
  vk_rasterizer.frontFace = CRUDE_CAST( VkFrontFace, creation->rasterization_state->front_face );
  vk_rasterizer.depthBiasEnable = creation->rasterization_state->depth_bias_enable ? VK_TRUE : VK_FALSE;
  vk_rasterizer.depthBiasConstantFactor = creation->rasterization_state->depth_bias_constant_factor;
  vk_rasterizer.depthBiasClamp = creation->rasterization_state->depth_bias_clamp;
  vk_rasterizer.depthBiasSlopeFactor = creation->rasterization_state->depth_bias_slope_factor;
  vk_rasterizer.lineWidth = creation->rasterization_state->line_width;
    
  vk_viewport_state = CRUDE_COMPOUNT_EMPTY( VkPipelineViewportStateCreateInfo ); 
  vk_viewport_state.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
  vk_viewport_state.viewportCount = creation->viewport_state->viewport_count;
  vk_viewport_state.scissorCount = creation->viewport_state->scissor_count;
    
  vk_dynamic_state = CRUDE_COMPOUNT_EMPTY( VkPipelineDynamicStateCreateInfo ); 
  vk_dynamic_state.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
  vk_dynamic_state.dynamicStateCount = CRUDE_COUNTOF( vk_dynamic_states );
  vk_dynamic_state.pDynamicStates = vk_dynamic_states;
    
  CRUDE_ASSERT( creation->rendering_state->color_attachment_count < CRUDE_COUNTOF( vk_color_attachment_formats ) );
  for ( uint32 i = 0; i < creation->rendering_state->color_attachment_count; ++i )
  {
    vk_color_attachment_formats[ i ] = CRUDE_CAST( VkFormat, creation->rendering_state->color_attachment_formats[ i ] );
  }

  vk_pipeline_rendering_create_info = CRUDE_COMPOUNT_EMPTY( VkPipelineRenderingCreateInfoKHR );
  vk_pipeline_rendering_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO_KHR;
  vk_pipeline_rendering_create_info.viewMask = creation->rendering_state->view_mask;
  vk_pipeline_rendering_create_info.colorAttachmentCount = creation->rendering_state->color_attachment_count;
  vk_pipeline_rendering_create_info.pColorAttachmentFormats = vk_color_attachment_formats;
  vk_pipeline_rendering_create_info.depthAttachmentFormat = CRUDE_CAST( VkFormat, creation->rendering_state->depth_attachment_format );
  vk_pipeline_rendering_create_info.stencilAttachmentFormat = CRUDE_CAST( VkFormat, creation->rendering_state->stencil_attachment_format );
  
  CRUDE_ASSERT( creation->stage_count < CRUDE_COUNTOF( vk_stages ) );
  for ( uint32 i = 0; i < creation->stage_count; ++i )
  {
    vk_stages[ i ] = CRUDE_COMPOUNT_EMPTY( VkPipelineShaderStageCreateInfo );
    vk_stages[ i ].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vk_stages[ i ].module = creation->stages[ i ].rhi_module.vk_shader_module;
    vk_stages[ i ].stage  = CRUDE_CAST( VkShaderStageFlagBits, creation->stages[ i ].stage );
    vk_stages[ i ].pName  = creation->stages[ i ].name;
  }

  vk_pipeline_info = CRUDE_COMPOUNT_EMPTY( VkGraphicsPipelineCreateInfo );
  vk_pipeline_info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
  vk_pipeline_info.pNext = &vk_pipeline_rendering_create_info;
  vk_pipeline_info.stageCount = creation->stage_count;
  vk_pipeline_info.pStages = vk_stages;
  vk_pipeline_info.pVertexInputState = &vk_vertex_input_info;
  vk_pipeline_info.pInputAssemblyState = &vk_input_assembly;
  vk_pipeline_info.pViewportState = &vk_viewport_state;
  vk_pipeline_info.pRasterizationState = &vk_rasterizer;
  vk_pipeline_info.pMultisampleState = &vk_multisampling;
  vk_pipeline_info.pDepthStencilState = &vk_depth_stencil;
  vk_pipeline_info.pColorBlendState = &vk_color_blending;
  vk_pipeline_info.pDynamicState = &vk_dynamic_state;
  vk_pipeline_info.layout = creation->pipeline_layout.vk_pipeline_layout;
  vk_pipeline_info.renderPass = NULL;
    
  CRUDE_GFX_RHI_HANDLE_VULKAN_RESULT( vkCreateGraphicsPipelines( device->vk_device, VK_NULL_HANDLE, 1, &vk_pipeline_info, CRUDE_GFX_RHI_DEVICE_VK_ALLOCATION_CALLBACKS, &pipeline->vk_pipeline ), "Failed to create geometry pipeline" );
}

void
crude_gfx_rhi_create_compute_pipeline
(
  _In_ crude_gfx_rhi_device                               *device,
  _In_ crude_gfx_rhi_compute_pipeline_create_info const   *creation,
  _Out_ crude_gfx_rhi_pipeline                            *pipeline
)
{
  VkComputePipelineCreateInfo                            vk_pipeline_info;
  
  vk_pipeline_info = CRUDE_COMPOUNT_EMPTY( VkComputePipelineCreateInfo );
  vk_pipeline_info.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
  vk_pipeline_info.stage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
  vk_pipeline_info.stage.pName = creation->stage.name;
  vk_pipeline_info.stage.module = creation->stage.rhi_module.vk_shader_module;
  vk_pipeline_info.stage.stage = CRUDE_CAST( VkShaderStageFlagBits, creation->stage.stage );
  vk_pipeline_info.layout = creation->pipeline_layout.vk_pipeline_layout;
  
  CRUDE_GFX_RHI_HANDLE_VULKAN_RESULT( vkCreateComputePipelines( device->vk_device, VK_NULL_HANDLE, 1u, &vk_pipeline_info, CRUDE_GFX_RHI_DEVICE_VK_ALLOCATION_CALLBACKS, &pipeline->vk_pipeline ), "Failed to create copmute pipeline" );
}

void
crude_gfx_rhi_create_ray_tracing_pipeline
(
  _In_ crude_gfx_rhi_device                               *device,
  _In_ crude_gfx_rhi_ray_tracing_pipeline_create_info const *creation,
  _Out_ crude_gfx_rhi_pipeline                            *pipeline
)
{
#if CRUDE_GFX_RAY_TRACING_ENABLED
  VkRayTracingPipelineCreateInfoKHR                        vk_pipeline_info;
  VkPipelineShaderStageCreateInfo                          vk_stages[ 8 ];
  VkRayTracingShaderGroupCreateInfoKHR                     vk_groups[ 8 ];

  CRUDE_ASSERT( creation->stage_count < CRUDE_COUNTOF( vk_stages ) );
  for ( uint32 i = 0; i < creation->stage_count; ++i )
  {
    vk_stages[ i ] = CRUDE_COMPOUNT_EMPTY( VkPipelineShaderStageCreateInfo );
    vk_stages[ i ].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vk_stages[ i ].module = creation->stages[ i ].rhi_module.vk_shader_module;
    vk_stages[ i ].stage  = CRUDE_CAST( VkShaderStageFlagBits, creation->stages[ i ].stage );
    vk_stages[ i ].pName  = creation->stages[ i ].name;
  }

  CRUDE_ASSERT( creation->group_count < CRUDE_COUNTOF( vk_groups ) );
  for ( uint32 i = 0; i < creation->group_count; ++i )
  {
    vk_groups[ i ] = CRUDE_COMPOUNT_EMPTY( VkRayTracingShaderGroupCreateInfoKHR );
    vk_groups[ i ].sType = VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR;
    vk_groups[ i ].anyHitShader = creation->groups[ i ].any_hit_shader;
    vk_groups[ i ].closestHitShader = creation->groups[ i ].closest_hit_shader;
    vk_groups[ i ].generalShader = creation->groups[ i ].general_shader;
    vk_groups[ i ].intersectionShader = creation->groups[ i ].intersection_shader;
    vk_groups[ i ].type = CRUDE_CAST( VkRayTracingShaderGroupTypeKHR, creation->groups[ i ].type );
  }

  vk_pipeline_info = CRUDE_COMPOUNT_EMPTY( VkRayTracingPipelineCreateInfoKHR );
  vk_pipeline_info.sType = VK_STRUCTURE_TYPE_RAY_TRACING_PIPELINE_CREATE_INFO_KHR;
  vk_pipeline_info.stageCount = creation->stage_count;
  vk_pipeline_info.pStages = vk_stages;
  vk_pipeline_info.groupCount = creation->group_count;
  vk_pipeline_info.pGroups = vk_groups;
  vk_pipeline_info.maxPipelineRayRecursionDepth = creation->max_pipeline_ray_recursion_depth;
  vk_pipeline_info.layout = creation->pipeline_layout.vk_pipeline_layout;
  
  CRUDE_GFX_RHI_HANDLE_VULKAN_RESULT( device->vkCreateRayTracingPipelinesKHR( device->vk_device, VK_NULL_HANDLE, VK_NULL_HANDLE, 1, &vk_pipeline_info, CRUDE_GFX_RHI_DEVICE_VK_ALLOCATION_CALLBACKS, &pipeline->vk_pipeline ), "Failed to create pipeline" );
#endif /* CRUDE_GFX_RAY_TRACING_ENABLED */
}

void
crude_gfx_rhi_destroy_pipeline
(
  _In_ crude_gfx_rhi_device                               *device,
  _In_ crude_gfx_rhi_pipeline                              pipeline
)
{
  vkDestroyPipeline( device->vk_device, pipeline.vk_pipeline, CRUDE_GFX_RHI_DEVICE_VK_ALLOCATION_CALLBACKS );
}

void
crude_gfx_rhi_get_ray_tracing_shader_group_handles
(
  _In_ crude_gfx_rhi_device                               *device,
  _In_ crude_gfx_rhi_pipeline                              pipeline,
  _In_ uint32                                              first_group,
  _In_ uint32                                              group_count,
  _In_ uint32                                              data_size,
  _Out_ void                                              *data
)
{
  CRUDE_GFX_RHI_HANDLE_VULKAN_RESULT( device->vkGetRayTracingShaderGroupHandlesKHR( device->vk_device, pipeline.vk_pipeline, first_group, group_count, data_size, data ), "Failed to get ray tracing shader group handles" );
}

void
crude_gfx_rhi_set_pipeline_debug_name
(
  _In_ crude_gfx_rhi_device                               *device,
  _In_ crude_gfx_rhi_pipeline                              pipeline,
  _In_ char const                                         *name
)
{
  crude_gfx_rhi_set_debug_utils_object_name( device, CRUDE_GFX_RHI_OBJECT_TYPE_PIPELINE, CRUDE_CAST( uint64, pipeline.vk_pipeline ), name );
}

void
crude_gfx_rhi_create_swapchain
(
  _In_ crude_gfx_rhi_device                               *device,
  _In_ crude_gfx_rhi_swapchain_create_info const          *creation,
  _In_ crude_heap_allocator                               *allocator,
  _Out_ crude_gfx_rhi_swapchain                           *swapchain,
  _Out_ uint32                                            *swapchain_images_count,
  _Out_ XMFLOAT2                                          *swapchain_extent,
  _Out_ crude_gfx_rhi_image                                swapchain_images[ CRUDE_GFX_SWAPCHAIN_IMAGES_MAX_COUNT ]
)
{
  VkPresentModeKHR                                        *vk_available_present_modes;
  VkSurfaceFormatKHR                                      *vk_available_formats;
  VkImage                                                  vk_swapchain_images[ CRUDE_GFX_SWAPCHAIN_IMAGES_MAX_COUNT ];
  VkSwapchainCreateInfoKHR                                 vk_swapchain_creation;
  VkSurfaceCapabilitiesKHR                                 vk_surface_capabilities;
  VkPresentModeKHR                                         vk_selected_present_mode;
  VkSurfaceFormatKHR                                       vk_surface_format;
  VkExtent2D                                               vk_swapchain_extent;
  bool                                                     vk_surface_format_found;
  uint32                                                   vk_available_formats_count, vk_available_present_modes_count, vk_image_count;
  uint32                                                   vk_queue_family_indices;

  vkGetPhysicalDeviceSurfaceCapabilitiesKHR( device->vk_physical_device, creation->surface.vk_surface, &vk_surface_capabilities );
  
  vk_swapchain_extent = vk_surface_capabilities.currentExtent;
  if ( vk_swapchain_extent.width == UINT32_MAX )
  {
    vk_swapchain_extent.width = CRUDE_CLAMP( vk_swapchain_extent.width, vk_surface_capabilities.minImageExtent.width, vk_surface_capabilities.maxImageExtent.width );
    vk_swapchain_extent.height = CRUDE_CLAMP( vk_swapchain_extent.height, vk_surface_capabilities.minImageExtent.height, vk_surface_capabilities.maxImageExtent.height );
  }
  
  vk_available_formats_count;
  vkGetPhysicalDeviceSurfaceFormatsKHR( device->vk_physical_device, creation->surface.vk_surface, &vk_available_formats_count, NULL );

  if ( vk_available_formats_count == 0u )
  {
    CRUDE_LOG_ERROR( CRUDE_CHANNEL_GRAPHICS, "Can't find available surface format! (available_formats_count == 0u)" );
  }

  CRUDE_ARRAY_INITIALIZE_WITH_LENGTH( vk_available_formats, vk_available_formats_count, crude_heap_allocator_pack( allocator ) );
  vkGetPhysicalDeviceSurfaceFormatsKHR( device->vk_physical_device, creation->surface.vk_surface, &vk_available_formats_count, vk_available_formats );

  vk_surface_format_found = false;
  for ( uint32 i = 0; i < vk_available_formats_count; ++i )
  {
    CRUDE_LOG_INFO( CRUDE_CHANNEL_GRAPHICS, "Available surface formats: format %i color_space: %i", vk_available_formats[ i ].format, vk_available_formats[ i ].colorSpace );
    if ( vk_available_formats[ i ].format == VK_FORMAT_R8G8B8A8_UNORM && vk_available_formats[ i ].colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR )
    {
      vk_surface_format = vk_available_formats[ i ];
      vk_surface_format_found = true;
    }
  }

  if ( !vk_surface_format_found )
  {
    CRUDE_LOG_ERROR( CRUDE_CHANNEL_GRAPHICS, "Can't find requested surface format" );
    CRUDE_ARRAY_DEINITIALIZE( vk_available_formats );
  }
  
  vk_available_present_modes_count;
  vkGetPhysicalDeviceSurfacePresentModesKHR( device->vk_physical_device, creation->surface.vk_surface, &vk_available_present_modes_count, NULL );
  if ( vk_available_present_modes_count == 0u ) 
  {
    CRUDE_LOG_ERROR( CRUDE_CHANNEL_GRAPHICS, "Can't find available surface present_mode" );
    CRUDE_ARRAY_DEINITIALIZE( vk_available_formats );
  }
  
  
  CRUDE_ARRAY_INITIALIZE_WITH_LENGTH( vk_available_present_modes, vk_available_present_modes_count, crude_heap_allocator_pack( allocator ) );
  vkGetPhysicalDeviceSurfacePresentModesKHR( device->vk_physical_device, creation->surface.vk_surface, &vk_available_present_modes_count, vk_available_present_modes );

  vk_selected_present_mode = VK_PRESENT_MODE_FIFO_KHR;
  for ( uint32 i = 0; i < vk_available_present_modes_count; ++i )
  {
    if ( vk_available_present_modes[ i ] == VK_PRESENT_MODE_MAILBOX_KHR )
    {
      vk_selected_present_mode = VK_PRESENT_MODE_MAILBOX_KHR;
      break;
    }
  }

  vk_image_count = ( vk_selected_present_mode == VK_PRESENT_MODE_IMMEDIATE_KHR ? 2 : 3 );
  vk_queue_family_indices = device->main_queue.vk_queue_family;
  
  vk_swapchain_creation = CRUDE_COMPOUNT_EMPTY( VkSwapchainCreateInfoKHR );
  vk_swapchain_creation.sType                  = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
  vk_swapchain_creation.pNext                  = NULL;
  vk_swapchain_creation.surface                = creation->surface.vk_surface;
  vk_swapchain_creation.minImageCount          = vk_image_count;
  vk_swapchain_creation.imageFormat            = vk_surface_format.format;
  vk_swapchain_creation.imageColorSpace        = vk_surface_format.colorSpace;
  vk_swapchain_creation.imageExtent            = vk_swapchain_extent;
  vk_swapchain_creation.imageArrayLayers       = 1;
  vk_swapchain_creation.imageUsage             = VK_IMAGE_USAGE_TRANSFER_DST_BIT;
  vk_swapchain_creation.imageSharingMode       = VK_SHARING_MODE_EXCLUSIVE; // VK_SHARING_MODE_CONCURRENT use if multiple queues
  vk_swapchain_creation.queueFamilyIndexCount  = 1u;  // VK_SHARING_MODE_CONCURRENT use if multiple queues
  vk_swapchain_creation.pQueueFamilyIndices    = &vk_queue_family_indices;
  vk_swapchain_creation.preTransform           = vk_surface_capabilities.currentTransform;
  vk_swapchain_creation.compositeAlpha         = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
  vk_swapchain_creation.presentMode            = vk_selected_present_mode;
  vk_swapchain_creation.clipped                = true;
  vk_swapchain_creation.oldSwapchain           = VK_NULL_HANDLE;
  
  CRUDE_GFX_RHI_HANDLE_VULKAN_RESULT( vkCreateSwapchainKHR( device->vk_device, &vk_swapchain_creation, CRUDE_GFX_RHI_DEVICE_VK_ALLOCATION_CALLBACKS, &swapchain->vk_swapchain ), "Failed to create swapchain!" );

  vkGetSwapchainImagesKHR( device->vk_device, swapchain->vk_swapchain, swapchain_images_count, NULL );
  vkGetSwapchainImagesKHR( device->vk_device, swapchain->vk_swapchain, swapchain_images_count, vk_swapchain_images );
  
  swapchain_extent->x = vk_swapchain_extent.width;
  swapchain_extent->y = vk_swapchain_extent.height;

  for ( uint32 i = 0; i < CRUDE_GFX_SWAPCHAIN_IMAGES_MAX_COUNT; ++i )
  {
    swapchain_images[ i ].vk_image = vk_swapchain_images[ i ];
    swapchain_images[ i ].vma_allocation = NULL;
  }

  CRUDE_ARRAY_DEINITIALIZE( vk_available_present_modes );
  CRUDE_ARRAY_DEINITIALIZE( vk_available_formats );
}

void
crude_gfx_rhi_destroy_swapchain
(
  _In_ crude_gfx_rhi_device                               *device,
  _In_ crude_gfx_rhi_swapchain                             swapchain
)
{
  vkDestroySwapchainKHR( device->vk_device, swapchain.vk_swapchain, CRUDE_GFX_RHI_DEVICE_VK_ALLOCATION_CALLBACKS );
}

crude_gfx_rhi_queue
crude_gfx_rhi_device_get_graphics_queue
(
  _In_ crude_gfx_rhi_device                               *device
)
{
  return device->main_queue;
}

crude_gfx_rhi_queue
crude_gfx_rhi_device_get_transfer_queue
(
  _In_ crude_gfx_rhi_device                               *device
)
{
  return device->transfer_queue;
}

void
crude_gfx_rhi_update_descriptor_set
(
  _In_ crude_gfx_rhi_device                               *device,
  _In_ crude_gfx_rhi_descriptor_set                        descriptor_set,
  _In_ crude_gfx_rhi_write_descriptor_set                 *write_descripor_sets,
  _In_ uint32                                              write_descripor_sets_count
)
{
#if CRUDE_GFX_RAY_TRACING_ENABLED
  VkWriteDescriptorSetAccelerationStructureKHR             vk_acceleration_structure_info[ CRUDE_GFX_BINDLESS_RESOURCES_MAX_COUNT ];
#endif
  VkWriteDescriptorSet                                     vk_descriptor_write[ CRUDE_GFX_BINDLESS_RESOURCES_MAX_COUNT ];
  VkDescriptorBufferInfo                                   vk_buffer_info[ CRUDE_GFX_BINDLESS_RESOURCES_MAX_COUNT ];
  VkDescriptorImageInfo                                    vk_image_info[ CRUDE_GFX_BINDLESS_RESOURCES_MAX_COUNT ];

  for ( uint32 i = 0; i < write_descripor_sets_count; i++ )
  {
    vk_descriptor_write[ i ] = CRUDE_COMPOUNT_EMPTY( VkWriteDescriptorSet );
    vk_descriptor_write[ i ].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    vk_descriptor_write[ i ].pNext = NULL;
    vk_descriptor_write[ i ].dstSet = descriptor_set.vk_descriptor_set;
    vk_descriptor_write[ i ].dstBinding = write_descripor_sets[ i ].dst_binding;
    vk_descriptor_write[ i ].dstArrayElement = write_descripor_sets[ i ].dst_array_element;
    vk_descriptor_write[ i ].descriptorCount = write_descripor_sets[ i ].descriptor_count;
    vk_descriptor_write[ i ].descriptorType = CRUDE_CAST( VkDescriptorType, write_descripor_sets[ i ].descriptor_type );
    vk_descriptor_write[ i ].pImageInfo = NULL;
    vk_descriptor_write[ i ].pBufferInfo = NULL;
    vk_descriptor_write[ i ].pTexelBufferView = NULL;

    if ( write_descripor_sets[ i ].image_info )
    {
      vk_image_info[ i ].imageLayout = CRUDE_CAST( VkImageLayout, write_descripor_sets[ i ].image_info->image_layout );
      vk_image_info[ i ].imageView = write_descripor_sets[ i ].image_info->image_view.vk_image_view;
      vk_image_info[ i ].sampler = write_descripor_sets[ i ].image_info->sampler.vk_sampler;
      
      vk_descriptor_write[ i ].pImageInfo = &vk_image_info[ i ];
    }
    
    if ( write_descripor_sets[ i ].buffer_info )
    {
      vk_buffer_info[ i ].buffer = write_descripor_sets[ i ].buffer_info->buffer.vk_buffer;
      vk_buffer_info[ i ].offset = write_descripor_sets[ i ].buffer_info->offset;
      vk_buffer_info[ i ].range = write_descripor_sets[ i ].buffer_info->range;
      
      vk_descriptor_write[ i ].pBufferInfo = &vk_buffer_info[ i ];
    }

#if CRUDE_GFX_RAY_TRACING_ENABLED
    if ( write_descripor_sets[ i ].acceleration_info )
    {
      vk_descriptor_write[ i ].descriptorType = VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR;
      
      vk_acceleration_structure_info[ i ] = CRUDE_COMPOUNT_EMPTY( VkWriteDescriptorSetAccelerationStructureKHR );
      vk_acceleration_structure_info[ i ].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET_ACCELERATION_STRUCTURE_KHR;
      vk_acceleration_structure_info[ i ].accelerationStructureCount = 1;
      vk_acceleration_structure_info[ i ].pAccelerationStructures = &write_descripor_sets[ i ].acceleration_info->acceleration_sturcture.vk_acceleration_structure;

      vk_descriptor_write[ i ].pNext = &vk_acceleration_structure_info[ i ];
#endif /* CRUDE_GFX_RAY_TRACING_ENABLED */
    }
  }

  vkUpdateDescriptorSets( device->vk_device, write_descripor_sets_count, vk_descriptor_write, 0, NULL );
}

crude_gfx_rhi_physical_device_optional_extensions const*
crude_gfx_rhi_get_device_optional_extensions
(
  _In_ crude_gfx_rhi_device                               *device
)
{
  return &device->optional_extensions;
}

void*
crude_gfx_rhi_get_buffer_mapped_data
(
  _In_ crude_gfx_rhi_buffer                                buffer
)
{
  return buffer.mapped_data;
}

void
crude_gfx_rhi_wait_semaphore
(
  _In_ crude_gfx_rhi_device                               *device,
  _In_ crude_gfx_rhi_semaphore                             semaphore,
  _In_ uint64                                              value
)
{
  VkSemaphoreWaitInfo                                      vk_semaphore_wait_info;

  vk_semaphore_wait_info = CRUDE_COMPOUNT_EMPTY( VkSemaphoreWaitInfo );
  vk_semaphore_wait_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_WAIT_INFO;
  vk_semaphore_wait_info.semaphoreCount = 1;
  vk_semaphore_wait_info.pSemaphores = &semaphore.vk_semaphore;
  vk_semaphore_wait_info.pValues = &value;
  
  CRUDE_GFX_RHI_HANDLE_VULKAN_RESULT( vkWaitSemaphores( device->vk_device, &vk_semaphore_wait_info, UINT64_MAX ), "Failed vkWaitSemaphores" );
}

XMFLOAT2
crude_gfx_rhi_get_surface_extent
(
  _In_ crude_gfx_rhi_device                               *device,
  _In_ crude_gfx_rhi_surface                               surface
)
{
  XMFLOAT2                                                 swapchain_extent;
  VkSurfaceCapabilitiesKHR                                 vk_surface_capabilities;

  vkGetPhysicalDeviceSurfaceCapabilitiesKHR( device->vk_physical_device, surface.vk_surface, &vk_surface_capabilities );
  
  swapchain_extent.x = vk_surface_capabilities.currentExtent.width;
  swapchain_extent.y = vk_surface_capabilities.currentExtent.height;

  return swapchain_extent;
}

float32
crude_gfx_rhi_get_timestamp_period
(
  _In_ crude_gfx_rhi_device                               *device
)
{
  VkPhysicalDeviceProperties                               vk_physical_properties;

  vkGetPhysicalDeviceProperties( device->vk_physical_device, &vk_physical_properties );

  return vk_physical_properties.limits.timestampPeriod;
}

bool
crude_gfx_rhi_acquire_next_image
(
  _In_ crude_gfx_rhi_device                               *device,
  _In_ crude_gfx_rhi_swapchain                             swapchain,
  _In_ uint64                                              timeout,
  _In_ crude_gfx_rhi_semaphore                             semaphore,
  _Out_ uint32                                            *image_index
)
{
  VkResult                                                 vk_result;

  vk_result = vkAcquireNextImageKHR( device->vk_device, swapchain.vk_swapchain, timeout, semaphore.vk_semaphore, VK_NULL_HANDLE, image_index );
  return ( vk_result != VK_ERROR_OUT_OF_DATE_KHR );
}

void
crude_gfx_rhi_wait_idle
(
  _In_ crude_gfx_rhi_device                               *device
)
{
  vkDeviceWaitIdle( device->vk_device );
}

void
crude_gfx_rhi_create_semaphore
(
  _In_ crude_gfx_rhi_device                               *device,
  _In_ bool                                                timeline,
  _Out_ crude_gfx_rhi_semaphore                           *semaphore
)
{
  VkSemaphoreCreateInfo                                    vk_semaphore_info;
  VkSemaphoreTypeCreateInfo                                vk_semaphore_type_info;

  vk_semaphore_info = CRUDE_COMPOUNT_EMPTY( VkSemaphoreCreateInfo );
  vk_semaphore_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

  if ( timeline )
  {
    vk_semaphore_type_info = CRUDE_COMPOUNT_EMPTY( VkSemaphoreTypeCreateInfo );
    vk_semaphore_type_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_TYPE_CREATE_INFO;
    vk_semaphore_type_info.semaphoreType = VK_SEMAPHORE_TYPE_TIMELINE;
    vk_semaphore_info.pNext = &vk_semaphore_type_info;
  }

  vkCreateSemaphore( device->vk_device, &vk_semaphore_info, CRUDE_GFX_RHI_DEVICE_VK_ALLOCATION_CALLBACKS, &semaphore->vk_semaphore );
}

void
crude_gfx_rhi_destroy_semaphore
(
  _In_ crude_gfx_rhi_device                               *device,
  _In_ crude_gfx_rhi_semaphore                             semaphore
)
{
  vkDestroySemaphore( device->vk_device, semaphore.vk_semaphore, CRUDE_GFX_RHI_DEVICE_VK_ALLOCATION_CALLBACKS );
}

void
crude_gfx_rhi_set_semaphore_debug_name
(
  _In_ crude_gfx_rhi_device                               *device,
  _In_ crude_gfx_rhi_semaphore                             semaphore,
  _In_ char const                                         *name
)
{
  crude_gfx_rhi_set_debug_utils_object_name( device, CRUDE_GFX_RHI_OBJECT_TYPE_SEMAPHORE, CRUDE_CAST( uint64, semaphore.vk_semaphore ), name );
}

void
crude_gfx_rhi_create_fence
(
  _In_ crude_gfx_rhi_device                               *device,
  _In_ bool                                                signaled,
  _Out_ crude_gfx_rhi_fence                               *fence
)
{
  VkFenceCreateInfo fence_info = CRUDE_COMPOUNT_EMPTY( VkFenceCreateInfo );
  fence_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
  fence_info.flags = signaled ? VK_FENCE_CREATE_SIGNALED_BIT : 0;
  vkCreateFence( device->vk_device, &fence_info, CRUDE_GFX_RHI_DEVICE_VK_ALLOCATION_CALLBACKS, &fence->vk_fence );
}
  
void
crude_gfx_rhi_destroy_fence
(
  _In_ crude_gfx_rhi_device                               *device,
  _In_ crude_gfx_rhi_fence                                 fence
)
{
  vkDestroyFence( device->vk_device, fence.vk_fence, CRUDE_GFX_RHI_DEVICE_VK_ALLOCATION_CALLBACKS );
}

void
crude_gfx_rhi_reset_fence
(
  _In_ crude_gfx_rhi_device                               *device,
  _In_ crude_gfx_rhi_fence                                *fence
)
{
  vkResetFences( device->vk_device, 1, &fence->vk_fence );
}

void
crude_gfx_rhi_set_fence_debug_name
(
  _In_ crude_gfx_rhi_device                               *device,
  _In_ crude_gfx_rhi_fence                                 fence,
  _In_ char const                                         *name
)
{
  crude_gfx_rhi_set_debug_utils_object_name( device, CRUDE_GFX_RHI_OBJECT_TYPE_FENCE, CRUDE_CAST( uint64, fence.vk_fence ), name );
}

void
crude_gfx_rhi_set_debug_utils_object_name
(
  _In_ crude_gfx_rhi_device                               *device,
  _In_ crude_gfx_rhi_object_type                           object_type,
  _In_ uint64                                              object_handle,
  _In_ char const                                         *object_name
)
{
  if ( device->vkSetDebugUtilsObjectNameEXT )
  {
    VkDebugUtilsObjectNameInfoEXT vk_name_info = CRUDE_COMPOUNT_EMPTY( VkDebugUtilsObjectNameInfoEXT );
    vk_name_info.sType        = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT;
    vk_name_info.objectType   = CRUDE_CAST( VkObjectType, object_type );
    vk_name_info.objectHandle = object_handle;
    vk_name_info.pObjectName  = object_name;
    device->vkSetDebugUtilsObjectNameEXT( device->vk_device, &vk_name_info );
  }
}

void
crude_gfx_rhi_destroy_descriptor_pool
(
  _In_ crude_gfx_rhi_device                               *device,
  _Out_ crude_gfx_rhi_descriptor_pool                     *descriptor_pool
)
{
  vkDestroyDescriptorPool( device->vk_device, descriptor_pool->vk_descriptor_pool, CRUDE_GFX_RHI_DEVICE_VK_ALLOCATION_CALLBACKS );
}


void
crude_gfx_rhi_get_device_memory_budget
(
  _In_ crude_gfx_rhi_device                               *device,
  _Out_ crude_gfx_rhi_device_memory_budget                *budget
)
{
  VmaBudget                                              gpu_memory_heap_budgets[ VK_MAX_MEMORY_HEAPS ];
  
  crude_memory_set(gpu_memory_heap_budgets, 0u, sizeof(gpu_memory_heap_budgets));
  vmaGetHeapBudgets( device->vma_allocator, gpu_memory_heap_budgets);
  
  budget->allocated = 0;
  budget->used = 0;

  for ( uint32 i = 0; i < VK_MAX_MEMORY_HEAPS; ++i )
  {
    budget->used += gpu_memory_heap_budgets[i].usage;
    budget->allocated += gpu_memory_heap_budgets[i].budget;
  }
}

void
crude_gfx_rhi_get_device_ray_tracing_pipeline_properties
(
  _In_ crude_gfx_rhi_device                               *device,
  _Out_ crude_gfx_rhi_device_ray_tracing_pipeline_properties *ray_tracing_properties
)
{
  VkPhysicalDeviceProperties2                              vk_physical_device_properties_2;
  VkPhysicalDeviceRayTracingPipelinePropertiesKHR          vk_ray_tracing_pipeline_properties;
    
  vk_physical_device_properties_2 = CRUDE_COMPOUNT_EMPTY( VkPhysicalDeviceProperties2 );
  vk_physical_device_properties_2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2;

#if CRUDE_GFX_RAY_TRACING_ENABLED
  vk_ray_tracing_pipeline_properties = CRUDE_COMPOUNT_EMPTY( VkPhysicalDeviceRayTracingPipelinePropertiesKHR );
  vk_ray_tracing_pipeline_properties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_PROPERTIES_KHR;

  vk_physical_device_properties_2.pNext = &vk_ray_tracing_pipeline_properties;
#endif /* CRUDE_GFX_RAY_TRACING_ENABLED */

  vkGetPhysicalDeviceProperties2( device->vk_physical_device, &vk_physical_device_properties_2 );

  ray_tracing_properties->shader_group_handle_alignment = vk_ray_tracing_pipeline_properties.shaderGroupHandleAlignment;
  ray_tracing_properties->shader_group_handle_size = vk_ray_tracing_pipeline_properties.shaderGroupHandleSize;
}

void
crude_gfx_rhi_get_device_name
(
  _In_ crude_gfx_rhi_device                               *device,
  _Out_ char                                               name[ 256 ]
)
{
  VkPhysicalDeviceProperties                             vk_physical_properties;

  vkGetPhysicalDeviceProperties( device->vk_physical_device, &vk_physical_properties );

  crude_string_copy( name, vk_physical_properties.deviceName ? vk_physical_properties.deviceName : "Unknown", sizeof( name ) );
}

void
crude_gfx_rhi_get_acceleration_structure_build_sizes
(
  _In_ crude_gfx_rhi_device                               *device,
  _In_ crude_heap_allocator                               *allocator,
  _In_ crude_gfx_rhi_acceleration_structure_build_type     build_type,
  _In_ crude_gfx_rhi_acceleration_structure_build_geometry_info const *build_info,
  _In_ uint32 const                                       *max_primitives_count,
  _Out_ crude_gfx_rhi_acceleration_structure_build_sizes_info *build_size_info
)
{
  VkAccelerationStructureGeometryKHR                      *vk_acceleration_structure_geometries;
  VkAccelerationStructureBuildSizesInfoKHR                 vk_acceleration_structure_build_sizes_info;
  VkAccelerationStructureBuildGeometryInfoKHR              vk_acceleration_build_geometry_infos;
  
  CRUDE_ARRAY_INITIALIZE_WITH_LENGTH( vk_acceleration_structure_geometries, build_info->geometry_count, crude_heap_allocator_pack( allocator ) );

  for ( uint32 i = 0; i < build_info->geometry_count; ++i )
  {
    vk_acceleration_structure_geometries[ i ] = CRUDE_COMPOUNT_EMPTY( VkAccelerationStructureGeometryKHR );
    vk_acceleration_structure_geometries[ i ].sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR;
    vk_acceleration_structure_geometries[ i ].flags = build_info->geometries[ i ].flags;
    vk_acceleration_structure_geometries[ i ].geometryType = CRUDE_CAST( VkGeometryTypeKHR, build_info->geometries[ i ].geometry_type );

    switch ( build_info->geometries[ i ].geometry_type )
    {
    case CRUDE_GFX_RHI_GEOMETRY_TYPE_AABBS_KHR:
    {
      vk_acceleration_structure_geometries[ i ].geometry.aabbs.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_AABBS_DATA_KHR;
      vk_acceleration_structure_geometries[ i ].geometry.aabbs.data.deviceAddress = build_info->geometries[ i ].geometry.aabbs.data.device_address;
      vk_acceleration_structure_geometries[ i ].geometry.aabbs.stride = build_info->geometries[ i ].geometry.aabbs.stride;
      break;
    }
    case CRUDE_GFX_RHI_GEOMETRY_TYPE_INSTANCES_KHR:
    {
      vk_acceleration_structure_geometries[ i ].geometry.instances.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_INSTANCES_DATA_KHR;
      vk_acceleration_structure_geometries[ i ].geometry.instances.arrayOfPointers = build_info->geometries[ i ].geometry.instances.array_of_pointers ? VK_TRUE : VK_FALSE;
      vk_acceleration_structure_geometries[ i ].geometry.instances.data.deviceAddress = build_info->geometries[ i ].geometry.instances.data.device_address;
      break;
    }
    case CRUDE_GFX_RHI_GEOMETRY_TYPE_TRIANGLES_KHR:
    {
      vk_acceleration_structure_geometries[ i ].geometry.triangles.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_TRIANGLES_DATA_KHR;
      vk_acceleration_structure_geometries[ i ].geometry.triangles.vertexFormat = CRUDE_CAST( VkFormat, build_info->geometries[ i ].geometry.triangles.vertex_format );
      vk_acceleration_structure_geometries[ i ].geometry.triangles.vertexData.deviceAddress = build_info->geometries[ i ].geometry.triangles.vertex_data.device_address;
      vk_acceleration_structure_geometries[ i ].geometry.triangles.vertexStride = build_info->geometries[ i ].geometry.triangles.vertex_stride;
      vk_acceleration_structure_geometries[ i ].geometry.triangles.maxVertex = build_info->geometries[ i ].geometry.triangles.max_vertex;
      vk_acceleration_structure_geometries[ i ].geometry.triangles.indexType = CRUDE_CAST( VkIndexType, build_info->geometries[ i ].geometry.triangles.index_type );
      vk_acceleration_structure_geometries[ i ].geometry.triangles.indexData.deviceAddress = build_info->geometries[ i ].geometry.triangles.index_data.device_address;
      vk_acceleration_structure_geometries[ i ].geometry.triangles.transformData.deviceAddress = build_info->geometries[ i ].geometry.triangles.transform_data.device_address;
      break;
    }
    }
  }

  vk_acceleration_build_geometry_infos = CRUDE_COMPOUNT_EMPTY( VkAccelerationStructureBuildGeometryInfoKHR );
  vk_acceleration_build_geometry_infos.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
  vk_acceleration_build_geometry_infos.type = CRUDE_CAST( VkAccelerationStructureTypeKHR, build_info->type );
  vk_acceleration_build_geometry_infos.flags = build_info->flags;
  vk_acceleration_build_geometry_infos.mode = CRUDE_CAST( VkBuildAccelerationStructureModeKHR, build_info->mode );
  vk_acceleration_build_geometry_infos.srcAccelerationStructure = build_info->src_acceleration_structure.vk_acceleration_structure;
  vk_acceleration_build_geometry_infos.dstAccelerationStructure = build_info->dst_acceleration_structure.vk_acceleration_structure;
  vk_acceleration_build_geometry_infos.geometryCount = build_info->geometry_count;
  vk_acceleration_build_geometry_infos.pGeometries = vk_acceleration_structure_geometries;
  vk_acceleration_build_geometry_infos.ppGeometries = NULL;
  vk_acceleration_build_geometry_infos.scratchData.deviceAddress = build_info->scratch_data.device_address;

  vk_acceleration_structure_build_sizes_info = CRUDE_COMPOUNT_EMPTY( VkAccelerationStructureBuildSizesInfoKHR );
  vk_acceleration_structure_build_sizes_info.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR;

  device->vkGetAccelerationStructureBuildSizesKHR( device->vk_device, CRUDE_CAST( VkAccelerationStructureBuildTypeKHR, build_type ), &vk_acceleration_build_geometry_infos, max_primitives_count, &vk_acceleration_structure_build_sizes_info );

  build_size_info->acceleration_structure_size = vk_acceleration_structure_build_sizes_info.accelerationStructureSize;
  build_size_info->build_scratch_size = vk_acceleration_structure_build_sizes_info.buildScratchSize;
  build_size_info->update_scratch_size = vk_acceleration_structure_build_sizes_info.updateScratchSize;

  CRUDE_ARRAY_DEINITIALIZE( vk_acceleration_structure_geometries );
}

void
crude_gfx_rhi_reset_command_pool
(
  _In_ crude_gfx_rhi_device                               *device,
  _In_ crude_gfx_rhi_command_pool                          command_pool
)
{
  vkResetCommandPool( device->vk_device, command_pool.vk_command_pool, 0 );
}

void
crude_gfx_rhi_begin_command_buffer
(
  _In_ crude_gfx_rhi_command_pool                          command_pool,
  _In_ crude_gfx_rhi_command_buffer                        command_buffer,
  _In_ crude_gfx_rhi_command_buffer_begin_info const      *begin_info
)
{
  VkCommandBufferBeginInfo                                 vk_begin_info;

  vk_begin_info = CRUDE_COMPOUNT_EMPTY( VkCommandBufferBeginInfo );
  vk_begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
  vk_begin_info.flags = begin_info->flags;
  CRUDE_GFX_RHI_HANDLE_VULKAN_RESULT( vkBeginCommandBuffer( command_buffer.vk_command_buffer, &vk_begin_info ), "Failed vkBeginCommandBuffer" );
}

void
crude_gfx_rhi_end_command_buffer
(
  _In_ crude_gfx_rhi_command_buffer                        command_buffer
)
{
  CRUDE_GFX_RHI_HANDLE_VULKAN_RESULT( vkEndCommandBuffer( command_buffer.vk_command_buffer ), "Failed vkEndCommandBuffer" );
}

void
crude_gfx_rhi_command_buffer_begin_rendering
(
  _In_ crude_gfx_rhi_command_buffer                        command_buffer,
  _In_ crude_gfx_rhi_rendering_info const                 *rendering_info
)
{
  VkRenderingAttachmentInfoKHR                             vk_color_attachments[ 8 ];
  VkRenderingAttachmentInfoKHR                             vk_depth_attachment;
  VkRenderingInfoKHR                                       vk_rendering_info;
  uint32                                                   temporary_allocator_marker;
  bool                                                     has_depth_attachment;
 
  CRUDE_ASSERT( rendering_info->color_attachment_count < CRUDE_COUNTOF( vk_color_attachments ) );

  for ( uint32 i = 0; i < rendering_info->color_attachment_count; ++i )
  {
    crude_gfx_rhi_rendering_attachment_info const         *color_attachment_info;
    VkRenderingAttachmentInfoKHR                          *vk_color_attachment_info;

    color_attachment_info = &rendering_info->color_attachments[ i ];
    vk_color_attachment_info = &vk_color_attachments[ i ];

    *vk_color_attachment_info = CRUDE_COMPOUNT_EMPTY( VkRenderingAttachmentInfoKHR );
    vk_color_attachment_info->sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO_KHR;
    vk_color_attachment_info->clearValue = crude_gfx_rhi_clear_value_to_vk_clear_value( color_attachment_info->clear_value );
    vk_color_attachment_info->imageLayout = CRUDE_CAST( VkImageLayout, color_attachment_info->image_layout );
    vk_color_attachment_info->imageView = color_attachment_info->image_view.vk_image_view;
    vk_color_attachment_info->loadOp = CRUDE_CAST( VkAttachmentLoadOp, color_attachment_info->load_op );
    vk_color_attachment_info->pNext = color_attachment_info->next;
    vk_color_attachment_info->resolveImageLayout = color_attachment_info->resolve_image_layout.vk_image_layout;
    vk_color_attachment_info->resolveImageView = color_attachment_info->resolve_image_view.vk_image_view;
    vk_color_attachment_info->resolveMode = CRUDE_CAST( VkResolveModeFlagBits, color_attachment_info->resolve_mode );
    vk_color_attachment_info->storeOp = CRUDE_CAST( VkAttachmentStoreOp, color_attachment_info->store_op );
  }
  
  vk_depth_attachment = CRUDE_COMPOUNT_EMPTY( VkRenderingAttachmentInfoKHR );
  vk_depth_attachment.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO_KHR;
  vk_depth_attachment.imageView = rendering_info->depth_attachment.image_view.vk_image_view;
  vk_depth_attachment.imageLayout = CRUDE_CAST( VkImageLayout, rendering_info->depth_attachment.image_layout );
  vk_depth_attachment.resolveMode = CRUDE_CAST( VkResolveModeFlagBits, rendering_info->depth_attachment.resolve_mode );
  vk_depth_attachment.loadOp = CRUDE_CAST( VkAttachmentLoadOp, rendering_info->depth_attachment.load_op );
  vk_depth_attachment.storeOp = CRUDE_CAST( VkAttachmentStoreOp, rendering_info->depth_attachment.store_op );
  vk_depth_attachment.clearValue = crude_gfx_rhi_clear_value_to_vk_clear_value( rendering_info->depth_attachment.clear_value );
  
  vk_rendering_info = CRUDE_COMPOUNT_EMPTY( VkRenderingInfoKHR );
  vk_rendering_info.sType = VK_STRUCTURE_TYPE_RENDERING_INFO_KHR;
  vk_rendering_info.renderArea = crude_gfx_rhi_rect_2d_to_vk_rect_2d( rendering_info->render_area );
  vk_rendering_info.layerCount = rendering_info->layer_count;
  vk_rendering_info.viewMask = rendering_info->view_mask;
  vk_rendering_info.colorAttachmentCount = rendering_info->color_attachment_count;
  vk_rendering_info.pColorAttachments = rendering_info->color_attachment_count > 0 ? vk_color_attachments : NULL;
  vk_rendering_info.pDepthAttachment = rendering_info->has_depth_attachment ? &vk_depth_attachment : 0;
  vk_rendering_info.flags = rendering_info->flags;
  
  vkCmdBeginRendering( command_buffer.vk_command_buffer, &vk_rendering_info );
}

void
crude_gfx_rhi_command_buffer_end_rendering
(
  _In_ crude_gfx_rhi_command_buffer                        command_buffer
)
{
  vkCmdEndRendering( command_buffer.vk_command_buffer );
}

void
crude_gfx_rhi_command_buffer_bind_pipeline
(
  _In_ crude_gfx_rhi_command_buffer                        command_buffer,
  _In_ crude_gfx_rhi_pipeline                              pipeline,
  _In_ crude_gfx_rhi_pipeline_bind_point                   pipeline_bind_point
)
{
  vkCmdBindPipeline( command_buffer.vk_command_buffer, CRUDE_CAST( VkPipelineBindPoint, pipeline_bind_point ), pipeline.vk_pipeline );
}

void
crude_gfx_rhi_command_buffer_set_viewport
(
  _In_ crude_gfx_rhi_command_buffer                        command_buffer,
  _In_ crude_gfx_rhi_viewport const                       *viewport
)
{
  VkViewport                                               vk_viewport;
  
  vk_viewport.x = viewport->x;
  vk_viewport.width = viewport->width;
  vk_viewport.y = viewport->y;
  vk_viewport.height = viewport->height;
  vk_viewport.minDepth = viewport->min_depth;
  vk_viewport.maxDepth = viewport->max_depth;

  vkCmdSetViewport( command_buffer.vk_command_buffer, 0, 1, &vk_viewport);
}

void
crude_gfx_rhi_command_buffer_set_scissor
(
  _In_ crude_gfx_rhi_command_buffer                        command_buffer,
  _In_ crude_gfx_rhi_scissor const                        *scissor
)
{
  VkRect2D vk_scissor;
  
  vk_scissor.offset.x = scissor->offset.x;
  vk_scissor.offset.y = scissor->offset.y;
  vk_scissor.extent.width = scissor->extent.x;
  vk_scissor.extent.height = scissor->extent.y;
  vkCmdSetScissor( command_buffer.vk_command_buffer, 0, 1, &vk_scissor );
}

void
crude_gfx_rhi_command_buffer_draw
(
  _In_ crude_gfx_rhi_command_buffer                        command_buffer,
  _In_ uint32                                              vertex_count,
  _In_ uint32                                              instance_count,
  _In_ uint32                                              first_vertex,
  _In_ uint32                                              first_instance
)
{
  vkCmdDraw( command_buffer.vk_command_buffer, vertex_count, instance_count, first_vertex, first_instance );
}

void
crude_gfx_rhi_command_buffer_draw_indirect
(
  _In_ crude_gfx_rhi_command_buffer                        command_buffer,
  _In_ crude_gfx_rhi_buffer                                buffer,
  _In_ crude_gfx_rhi_device_size                           offset,
  _In_ uint32                                              draw_count,
  _In_ uint32                                              stride
)
{
  vkCmdDrawIndirect( command_buffer.vk_command_buffer, buffer.vk_buffer, offset, draw_count, stride );
}


void
crude_gfx_rhi_command_buffer_draw_indirect_count
(
  _In_ crude_gfx_rhi_command_buffer                        command_buffer,
  _In_ crude_gfx_rhi_buffer                                argument_buffer,
  _In_ crude_gfx_rhi_device_size                           argument_buffer_offset,
  _In_ crude_gfx_rhi_buffer                                count_buffer,
  _In_ crude_gfx_rhi_device_size                           count_buffer_offset,
  _In_ uint32                                              max_draw_count,
  _In_ uint32                                              stride
)
{
  vkCmdDrawIndirectCount( command_buffer.vk_command_buffer, argument_buffer.vk_buffer, argument_buffer_offset, count_buffer.vk_buffer, count_buffer_offset, max_draw_count, stride );
}

void
crude_gfx_rhi_command_buffer_draw_mesh_task
(
  _In_ crude_gfx_rhi_device                               *device,
  _In_ crude_gfx_rhi_command_buffer                        command_buffer,
  _In_ uint32                                              group_count_x,
  _In_ uint32                                              group_count_y,
  _In_ uint32                                              group_count_z
)
{
  device->vkCmdDrawMeshTasksEXT( command_buffer.vk_command_buffer, group_count_x, group_count_y, group_count_z );
}

void
crude_gfx_rhi_command_buffer_draw_mesh_task_indirect_count
(
  _In_ crude_gfx_rhi_device                               *device,
  _In_ crude_gfx_rhi_command_buffer                        command_buffer,
  _In_ crude_gfx_rhi_buffer                                argument_buffer,
  _In_ crude_gfx_rhi_device_size                           argument_buffer_offset,
  _In_ crude_gfx_rhi_buffer                                count_buffer,
  _In_ crude_gfx_rhi_device_size                           count_buffer_offset,
  _In_ uint32                                              max_draw_count,
  _In_ uint32                                              stride
)
{
  device->vkCmdDrawMeshTasksIndirectCountEXT( command_buffer.vk_command_buffer, argument_buffer.vk_buffer, argument_buffer_offset, count_buffer.vk_buffer, count_buffer_offset, max_draw_count, stride );
}

void
crude_gfx_rhi_command_buffer_dispatch
(
  _In_ crude_gfx_rhi_command_buffer                        command_buffer,
  _In_ uint32                                              group_count_x,
  _In_ uint32                                              group_count_y,
  _In_ uint32                                              group_count_z
)
{
  vkCmdDispatch( command_buffer.vk_command_buffer, group_count_x, group_count_y, group_count_z );
}

void
crude_gfx_rhi_command_buffer_bind_descriptor_sets
(
  _In_ crude_gfx_rhi_command_buffer                        command_buffer,
  _In_ crude_gfx_rhi_pipeline_bind_point                   pipeline_bind_point,
  _In_ crude_gfx_rhi_pipeline_layout                       pipeline_layout,
  _In_ uint32                                              set,
  _In_ crude_gfx_rhi_descriptor_set const                  descriptor_set
)
{
  vkCmdBindDescriptorSets(
    command_buffer.vk_command_buffer,
    CRUDE_CAST( VkPipelineBindPoint, pipeline_bind_point ),
    pipeline_layout.vk_pipeline_layout,
    set,
    1u, &descriptor_set.vk_descriptor_set,
    0u, NULL );
}

void
crude_gfx_rhi_command_buffer_pipeline_image_barrier
(
  _In_ crude_gfx_rhi_command_buffer                        command_buffer,
  _In_ crude_gfx_rhi_image_memory_barrier const           *image_memory_barriers
)
{
  VkDependencyInfoKHR                                      vk_dependency_info;
  VkImageMemoryBarrier2                                    vk_image_barrier;
  
  CRUDE_ASSERTM( CRUDE_CHANNEL_GRAPHICS, image_memory_barriers->image.vk_image, "Can't add image barrier to the image! image is VK_NULL_HANDLE!" );

  vk_dependency_info = CRUDE_COMPOUNT_EMPTY( VkDependencyInfoKHR );
  vk_dependency_info.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO_KHR;

  vk_image_barrier = CRUDE_COMPOUNT_EMPTY( VkImageMemoryBarrier2 );
  vk_image_barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2_KHR;
  vk_image_barrier.srcStageMask = image_memory_barriers->src_stage_mask;
  vk_image_barrier.srcAccessMask = image_memory_barriers->src_access_mask;
  vk_image_barrier.dstStageMask = image_memory_barriers->dst_stage_mask;
  vk_image_barrier.dstAccessMask = image_memory_barriers->dst_access_mask;
  vk_image_barrier.oldLayout = CRUDE_CAST( VkImageLayout, image_memory_barriers->old_layout );
  vk_image_barrier.newLayout = CRUDE_CAST( VkImageLayout, image_memory_barriers->new_layout );
  vk_image_barrier.srcQueueFamilyIndex = image_memory_barriers->src_queue.vk_queue_family;
  vk_image_barrier.dstQueueFamilyIndex = image_memory_barriers->dst_queue.vk_queue_family;
  vk_image_barrier.image = image_memory_barriers->image.vk_image;
  vk_image_barrier.subresourceRange.aspectMask = image_memory_barriers->subresource_range.aspect_mask;
  vk_image_barrier.subresourceRange.baseArrayLayer = image_memory_barriers->subresource_range.base_array_layer;
  vk_image_barrier.subresourceRange.baseMipLevel = image_memory_barriers->subresource_range.base_mip_level;
  vk_image_barrier.subresourceRange.layerCount = image_memory_barriers->subresource_range.layer_count;
  vk_image_barrier.subresourceRange.levelCount = image_memory_barriers->subresource_range.level_count;
  
  vk_dependency_info.imageMemoryBarrierCount = 1u;
  vk_dependency_info.pImageMemoryBarriers = &vk_image_barrier;
 
  vkCmdPipelineBarrier2( command_buffer.vk_command_buffer, &vk_dependency_info );
}

void
crude_gfx_rhi_command_buffer_pipeline_buffer_barrier
(
  _In_ crude_gfx_rhi_command_buffer                        command_buffer,
  _In_ crude_gfx_rhi_buffer_memory_barrier                *buffer_memory_barriers
)
{
  VkDependencyInfoKHR                                      vk_dependency_info;
  VkBufferMemoryBarrier2KHR                                vk_buffer_barrier;
  
  vk_dependency_info = CRUDE_COMPOUNT_EMPTY( VkDependencyInfoKHR );
  vk_dependency_info.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO_KHR;

  vk_buffer_barrier = CRUDE_COMPOUNT_EMPTY( VkBufferMemoryBarrier2KHR );
  vk_buffer_barrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER_2_KHR;
  vk_buffer_barrier.srcStageMask = buffer_memory_barriers->src_stage_mask;
  vk_buffer_barrier.srcAccessMask = buffer_memory_barriers->src_access_mask;
  vk_buffer_barrier.dstStageMask = buffer_memory_barriers->dst_stage_mask;
  vk_buffer_barrier.dstAccessMask = buffer_memory_barriers->dst_access_mask;
  vk_buffer_barrier.srcQueueFamilyIndex = buffer_memory_barriers->src_queue.vk_queue_family;
  vk_buffer_barrier.dstQueueFamilyIndex = buffer_memory_barriers->dst_queue.vk_queue_family;
  vk_buffer_barrier.buffer = buffer_memory_barriers->buffer->vk_buffer;
  vk_buffer_barrier.offset = buffer_memory_barriers->offset;
  vk_buffer_barrier.size = buffer_memory_barriers->size;
  
  vk_dependency_info.bufferMemoryBarrierCount = 1u;
  vk_dependency_info.pBufferMemoryBarriers = &vk_buffer_barrier;
 
  vkCmdPipelineBarrier2( command_buffer.vk_command_buffer, &vk_dependency_info );
}

void
crude_gfx_rhi_command_buffer_pipeline_global_barrier
(
  _In_ crude_gfx_rhi_command_buffer                        command_buffer
)
{
  VkMemoryBarrier2KHR                                      vk_barrier;
  VkDependencyInfoKHR                                      vk_dependency_info;

  vk_barrier = CRUDE_COMPOUNT_EMPTY( VkMemoryBarrier2KHR );
  vk_barrier.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER_2_KHR;
  vk_barrier.srcStageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT_KHR;
  vk_barrier.srcAccessMask = VK_ACCESS_2_MEMORY_READ_BIT_KHR | VK_ACCESS_2_MEMORY_WRITE_BIT_KHR;
  vk_barrier.dstStageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT_KHR;
  vk_barrier.dstAccessMask = VK_ACCESS_2_MEMORY_READ_BIT_KHR | VK_ACCESS_2_MEMORY_WRITE_BIT_KHR;

  vk_dependency_info = CRUDE_COMPOUNT_EMPTY( VkDependencyInfoKHR );
  vk_dependency_info.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO_KHR;
  vk_dependency_info.memoryBarrierCount = 1;
  vk_dependency_info.pMemoryBarriers = &vk_barrier;
  
  vkCmdPipelineBarrier2( command_buffer.vk_command_buffer, &vk_dependency_info );
}

void
crude_gfx_rhi_command_buffer_copy_buffer_to_image
(
  _In_ crude_gfx_rhi_command_buffer                        command_buffer,
  _In_ crude_gfx_rhi_buffer                                buffer,
  _In_ crude_gfx_rhi_image                                 image,
  _In_ crude_gfx_rhi_buffer_image_copy const              *region
)
{
  VkBufferImageCopy                                        vk_region;

  vk_region = CRUDE_COMPOUNT_EMPTY( VkBufferImageCopy );
  vk_region.bufferOffset = region->buffer_offset;
  vk_region.bufferRowLength = region->buffer_row_length;
  vk_region.bufferImageHeight = region->buffer_image_height;
  vk_region.imageSubresource.aspectMask = region->image_subresource.aspect_mask;
  vk_region.imageSubresource.mipLevel = region->image_subresource.mip_level;
  vk_region.imageSubresource.baseArrayLayer = region->image_subresource.base_array_layer;
  vk_region.imageSubresource.layerCount = region->image_subresource.layer_count;
  vk_region.imageOffset.x = region->image_offset.x;
  vk_region.imageOffset.y = region->image_offset.y;
  vk_region.imageOffset.z = region->image_offset.z;
  vk_region.imageExtent.width = region->image_extent.x;
  vk_region.imageExtent.height = region->image_extent.y;
  vk_region.imageExtent.depth = region->image_extent.z;
  
  vkCmdCopyBufferToImage( command_buffer.vk_command_buffer, buffer.vk_buffer, image.vk_image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &vk_region );
}

void
crude_gfx_rhi_command_buffer_copy_buffer
(
  _In_ crude_gfx_rhi_command_buffer                        command_buffer,
  _In_ crude_gfx_rhi_buffer                                src_buffer,
  _In_ crude_gfx_rhi_buffer                                dst_buffer,
  _In_ crude_gfx_rhi_buffer_copy const                    *region
)
{
  VkBufferCopy                                             vk_region;

  vk_region = CRUDE_COMPOUNT_EMPTY( VkBufferCopy );
  vk_region.srcOffset = region->src_offset;
  vk_region.dstOffset = region->dst_offset;
  vk_region.size = region->size;

  vkCmdCopyBuffer( command_buffer.vk_command_buffer, src_buffer.vk_buffer, dst_buffer.vk_buffer, 1, &vk_region );
}

void
crude_gfx_rhi_command_buffer_write_timestamp
(
  _In_ crude_gfx_rhi_command_buffer                        command_buffer,
  _In_ crude_gfx_rhi_pipeline_stage_flags                  pipeline_stage,
  _In_ crude_gfx_rhi_query_pool                            query_pool,
  _In_ uint32                                              query
)
{
  vkCmdWriteTimestamp( command_buffer.vk_command_buffer, CRUDE_CAST( VkPipelineStageFlagBits, pipeline_stage ), query_pool.vk_query_pool, query );
}

void
crude_gfx_rhi_command_buffer_begin_debug_utils_label
(
  _In_ crude_gfx_rhi_device                               *device,
  _In_ crude_gfx_rhi_command_buffer                        command_buffer,
  _In_ crude_gfx_rhi_debug_utils_label const              *debug_utils_label
)
{
  VkDebugUtilsLabelEXT                                     vk_label;
  
  vk_label = CRUDE_COMPOUNT_EMPTY( VkDebugUtilsLabelEXT );
  vk_label.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_LABEL_EXT;
  vk_label.pLabelName = debug_utils_label->label_name;
  vk_label.color[ 0 ] = debug_utils_label->color[ 0 ];
  vk_label.color[ 1 ] = debug_utils_label->color[ 1 ];
  vk_label.color[ 2 ] = debug_utils_label->color[ 2 ];
  vk_label.color[ 3 ] = debug_utils_label->color[ 3 ];
  device->vkCmdBeginDebugUtilsLabelEXT( command_buffer.vk_command_buffer, &vk_label );
}

void
crude_gfx_rhi_command_buffer_end_debug_utils_label
(
  _In_ crude_gfx_rhi_device                               *device,
  _In_ crude_gfx_rhi_command_buffer                        command_buffer
)
{
  device->vkCmdEndDebugUtilsLabelEXT( command_buffer.vk_command_buffer );
}

void
crude_gfx_rhi_command_buffer_push_constant
(
  _In_ crude_gfx_rhi_command_buffer                        command_buffer,
  _In_ crude_gfx_rhi_pipeline_layout                       layout,
  _In_ crude_gfx_rhi_shader_stage_flags                    stage_flags,
  _In_ uint32                                              offset,
  _In_ uint32                                              size,
  _In_ void const                                         *values
)
{
  vkCmdPushConstants( command_buffer.vk_command_buffer, layout.vk_pipeline_layout, stage_flags, 0u, size, values );
}

void
crude_gfx_rhi_command_buffer_fill_buffer
(
  _In_ crude_gfx_rhi_command_buffer                        command_buffer,
  _In_ crude_gfx_rhi_buffer                                dst_buffer,
  _In_ crude_gfx_rhi_device_size                           dst_offset,
  _In_ crude_gfx_rhi_device_size                           size,
  _In_ uint32                                              data
)
{
  vkCmdFillBuffer( command_buffer.vk_command_buffer, dst_buffer.vk_buffer, dst_offset, size, data );
}

void
crude_gfx_rhi_command_buffer_trace_rays
(
  _In_ crude_gfx_rhi_device                               *device,
  _In_ crude_gfx_rhi_command_buffer                        command_buffer,
  _In_ crude_gfx_rhi_strided_device_address_region const  *raygen_shader_binding_table,
  _In_ crude_gfx_rhi_strided_device_address_region const  *miss_shader_binding_table,
  _In_ crude_gfx_rhi_strided_device_address_region const  *hit_shader_binding_table,
  _In_ crude_gfx_rhi_strided_device_address_region const  *callable_shader_binding_table,
  _In_ uint32                                              width,
  _In_ uint32                                              height,
  _In_ uint32                                              depth
)
{
  VkStridedDeviceAddressRegionKHR                          vk_raygen_table, vk_hit_table, vk_miss_table, vk_callable_table;
  
  vk_raygen_table = CRUDE_COMPOUNT_EMPTY( VkStridedDeviceAddressRegionKHR );
  vk_raygen_table.deviceAddress = raygen_shader_binding_table->device_address;
  vk_raygen_table.stride = raygen_shader_binding_table->stride;
  vk_raygen_table.size = raygen_shader_binding_table->size;
  
  vk_hit_table = CRUDE_COMPOUNT_EMPTY( VkStridedDeviceAddressRegionKHR );
  vk_hit_table.deviceAddress = hit_shader_binding_table->device_address;
  vk_hit_table.stride = hit_shader_binding_table->stride;
  vk_hit_table.size = hit_shader_binding_table->size;
  
  vk_miss_table = CRUDE_COMPOUNT_EMPTY( VkStridedDeviceAddressRegionKHR );
  vk_miss_table.deviceAddress = miss_shader_binding_table->device_address;
  vk_miss_table.stride = miss_shader_binding_table->stride;
  vk_miss_table.size = miss_shader_binding_table->size;

  vk_callable_table = CRUDE_COMPOUNT_EMPTY( VkStridedDeviceAddressRegionKHR );
  
  device->vkCmdTraceRaysKHR( command_buffer.vk_command_buffer, &vk_raygen_table, &vk_miss_table, &vk_hit_table, &vk_callable_table, width, height, depth );
}

void
crude_gfx_rhi_command_buffer_begin_query
(
  _In_ crude_gfx_rhi_command_buffer                        command_buffer,
  _In_ crude_gfx_rhi_query_pool                            query_pool,
  _In_ uint32                                              query,
  _In_ crude_gfx_rhi_query_control_flags                   flags
)
{
  vkCmdBeginQuery( command_buffer.vk_command_buffer, query_pool.vk_query_pool, query, flags );
}

void
crude_gfx_rhi_command_buffer_end_query
(
  _In_ crude_gfx_rhi_command_buffer                        command_buffer,
  _In_ crude_gfx_rhi_query_pool                            query_pool,
  _In_ uint32                                              query
)
{
  vkCmdEndQuery( command_buffer.vk_command_buffer, query_pool.vk_query_pool, query );
}

void
crude_gfx_rhi_command_buffer_reset_query_pool
(
  _In_ crude_gfx_rhi_command_buffer                        command_buffer,
  _In_ crude_gfx_rhi_query_pool                            query_pool,
  _In_ uint32                                              first_query,
  _In_ uint32                                              query_count
)
{
  vkCmdResetQueryPool( command_buffer.vk_command_buffer, query_pool.vk_query_pool, first_query, query_count );
}

void
crude_gfx_rhi_command_buffer_copy_image
(
  _In_ crude_gfx_rhi_command_buffer                        command_buffer,
  _In_ crude_gfx_rhi_image                                 src_image,
  _In_ crude_gfx_rhi_image_layout                          src_image_layout,
  _In_ crude_gfx_rhi_image                                 dst_image,
  _In_ crude_gfx_rhi_image_layout                          dst_image_layout,
  _In_ crude_gfx_rhi_image_copy const                     *region
)
{
  VkImageCopy                                              vk_image_copy;

  vk_image_copy = CRUDE_COMPOUNT_EMPTY( VkImageCopy );
  vk_image_copy.srcSubresource.aspectMask = region->src_subresource.aspect_mask;
  vk_image_copy.srcSubresource.mipLevel = region->src_subresource.mip_level;
  vk_image_copy.srcSubresource.baseArrayLayer = region->src_subresource.base_array_layer;
  vk_image_copy.srcSubresource.layerCount = region->src_subresource.layer_count;
  vk_image_copy.srcOffset.x = region->src_offset.x;
  vk_image_copy.srcOffset.y = region->src_offset.y;
  vk_image_copy.srcOffset.z = region->src_offset.z;
  vk_image_copy.dstSubresource.aspectMask = region->dst_subresource.aspect_mask;
  vk_image_copy.dstSubresource.mipLevel = region->dst_subresource.mip_level;
  vk_image_copy.dstSubresource.baseArrayLayer = region->dst_subresource.base_array_layer;
  vk_image_copy.dstSubresource.layerCount = region->dst_subresource.layer_count;
  vk_image_copy.dstOffset.x = region->dst_offset.x;
  vk_image_copy.dstOffset.y = region->dst_offset.y;
  vk_image_copy.dstOffset.z = region->dst_offset.z;
  vk_image_copy.extent.width = region->extent.x;
  vk_image_copy.extent.height = region->extent.y;
  vk_image_copy.extent.depth = region->extent.z;

  vkCmdCopyImage(
    command_buffer.vk_command_buffer,
    src_image.vk_image, CRUDE_CAST( VkImageLayout, src_image_layout ),
    dst_image.vk_image, CRUDE_CAST( VkImageLayout, dst_image_layout ),
    1u, &vk_image_copy );
}

void
crude_gfx_rhi_command_buffer_blit_image
(
  _In_ crude_gfx_rhi_command_buffer                        command_buffer,
  _In_ crude_gfx_rhi_image                                 src_image,
  _In_ crude_gfx_rhi_image_layout                          src_image_layout,
  _In_ crude_gfx_rhi_image                                 dst_image,
  _In_ crude_gfx_rhi_image_layout                          dst_image_layout,
  _In_ crude_gfx_rhi_image_blit const                     *region,
  _In_ crude_gfx_rhi_filter                                filter
)
{
  VkImageBlit blit_region = CRUDE_COMPOUNT_EMPTY( VkImageBlit );

  blit_region.srcSubresource.aspectMask = region->src_subresource.aspect_mask;
  blit_region.srcSubresource.mipLevel = region->src_subresource.mip_level;
  blit_region.srcSubresource.baseArrayLayer = region->src_subresource.base_array_layer;
  blit_region.srcSubresource.layerCount = region->src_subresource.layer_count;
  
  blit_region.srcOffsets[ 0 ].x = region->src_offsets[ 0 ].x;
  blit_region.srcOffsets[ 0 ].y = region->src_offsets[ 0 ].y;
  blit_region.srcOffsets[ 0 ].z = region->src_offsets[ 0 ].z;
  blit_region.srcOffsets[ 1 ].x = region->src_offsets[ 1 ].x;
  blit_region.srcOffsets[ 1 ].y = region->src_offsets[ 1 ].y;
  blit_region.srcOffsets[ 1 ].z = region->src_offsets[ 1 ].z;

  blit_region.dstSubresource.aspectMask = region->dst_subresource.aspect_mask;
  blit_region.dstSubresource.mipLevel = region->dst_subresource.mip_level;
  blit_region.dstSubresource.baseArrayLayer = region->dst_subresource.base_array_layer;
  blit_region.dstSubresource.layerCount = region->dst_subresource.layer_count;
  
  blit_region.dstOffsets[ 0 ].x = region->dst_offsets[ 0 ].x;
  blit_region.dstOffsets[ 0 ].y = region->dst_offsets[ 0 ].y;
  blit_region.dstOffsets[ 0 ].z = region->dst_offsets[ 0 ].z;
  blit_region.dstOffsets[ 1 ].x = region->dst_offsets[ 1 ].x;
  blit_region.dstOffsets[ 1 ].y = region->dst_offsets[ 1 ].y;
  blit_region.dstOffsets[ 1 ].z = region->dst_offsets[ 1 ].z;

  vkCmdBlitImage( command_buffer.vk_command_buffer,
    src_image.vk_image, CRUDE_CAST( VkImageLayout, src_image_layout ),
    dst_image.vk_image, CRUDE_CAST( VkImageLayout, dst_image_layout ),
    1, &blit_region,
    CRUDE_CAST( VkFilter, filter ) );
}

void
crude_gfx_rhi_reset_command_buffer
(
  _In_ crude_gfx_rhi_command_buffer                        command_buffer
)
{
  vkResetCommandBuffer( command_buffer.vk_command_buffer, VK_COMMAND_BUFFER_RESET_RELEASE_RESOURCES_BIT );
}

void
crude_gfx_rhi_command_buffer_build_acceleration_structures
(
  _In_ crude_gfx_rhi_device                                                   *device,
  _In_ crude_heap_allocator                                                   *allocator,
  _In_ crude_gfx_rhi_command_buffer                                            command_buffer,
  _In_ uint32                                                                  info_count,
  _In_ crude_gfx_rhi_acceleration_structure_build_geometry_info const         *infos,
  _In_ crude_gfx_rhi_acceleration_structure_build_range_info const            *build_range_infos
)
{
  VkAccelerationStructureBuildRangeInfoKHR               **vk_acceleration_structure_build_range_infos;
  VkAccelerationStructureGeometryKHR                     **vk_acceleration_structure_geometries;
  VkAccelerationStructureBuildGeometryInfoKHR             *vk_acceleration_build_geometry_infos;
  
  CRUDE_ARRAY_INITIALIZE_WITH_LENGTH( vk_acceleration_build_geometry_infos, info_count, crude_heap_allocator_pack( allocator ) );
  CRUDE_ARRAY_INITIALIZE_WITH_LENGTH( vk_acceleration_structure_geometries, info_count, crude_heap_allocator_pack( allocator ) );
  CRUDE_ARRAY_INITIALIZE_WITH_LENGTH( vk_acceleration_structure_build_range_infos, info_count, crude_heap_allocator_pack( allocator ) );

  for ( uint32 i = 0; i < info_count; ++i )
  {
    CRUDE_ARRAY_INITIALIZE_WITH_LENGTH( vk_acceleration_structure_geometries[ i ], infos[ i ].geometry_count, crude_heap_allocator_pack( allocator ) );

    for ( uint32 gi = 0u; gi < infos[ i ].geometry_count; ++gi )
    {
      crude_gfx_rhi_acceleration_structure_geometry const *geometry;
      VkAccelerationStructureGeometryKHR                  *vk_geometry;

      geometry = &infos->geometries[ gi ];
      vk_geometry = &vk_acceleration_structure_geometries[ i ][ gi ];

      *vk_geometry = CRUDE_COMPOUNT_EMPTY( VkAccelerationStructureGeometryKHR );
      vk_geometry->sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR;
      vk_geometry->flags = geometry->flags;
      vk_geometry->geometryType = CRUDE_CAST( VkGeometryTypeKHR, geometry->geometry_type );

      switch ( infos->geometries[ i ].geometry_type )
      {
      case CRUDE_GFX_RHI_GEOMETRY_TYPE_AABBS_KHR:
      {
        vk_geometry->geometry.aabbs.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_AABBS_DATA_KHR;
        vk_geometry->geometry.aabbs.data.deviceAddress = geometry->geometry.aabbs.data.device_address;
        vk_geometry->geometry.aabbs.stride = geometry->geometry.aabbs.stride;
        break;
      }
      case CRUDE_GFX_RHI_GEOMETRY_TYPE_INSTANCES_KHR:
      {
        vk_geometry->geometry.instances.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_INSTANCES_DATA_KHR;
        vk_geometry->geometry.instances.arrayOfPointers = geometry->geometry.instances.array_of_pointers ? VK_TRUE : VK_FALSE;
        vk_geometry->geometry.instances.data.deviceAddress = geometry->geometry.instances.data.device_address;
        break;
      }
      case CRUDE_GFX_RHI_GEOMETRY_TYPE_TRIANGLES_KHR:
      {
        vk_geometry->geometry.triangles.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_TRIANGLES_DATA_KHR;
        vk_geometry->geometry.triangles.vertexFormat = CRUDE_CAST( VkFormat, geometry->geometry.triangles.vertex_format );
        vk_geometry->geometry.triangles.vertexData.deviceAddress = geometry->geometry.triangles.vertex_data.device_address;
        vk_geometry->geometry.triangles.vertexStride = geometry->geometry.triangles.vertex_stride;
        vk_geometry->geometry.triangles.maxVertex = geometry->geometry.triangles.max_vertex;
        vk_geometry->geometry.triangles.indexType = CRUDE_CAST( VkIndexType, geometry->geometry.triangles.index_type );
        vk_geometry->geometry.triangles.indexData.deviceAddress = geometry->geometry.triangles.index_data.device_address;
        vk_geometry->geometry.triangles.transformData.deviceAddress = geometry->geometry.triangles.transform_data.device_address;
        break;
      }
      }
    }

    vk_acceleration_build_geometry_infos[ i ] = CRUDE_COMPOUNT_EMPTY( VkAccelerationStructureBuildGeometryInfoKHR );
    vk_acceleration_build_geometry_infos[ i ].sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
    vk_acceleration_build_geometry_infos[ i ].type = CRUDE_CAST( VkAccelerationStructureTypeKHR, infos->type );
    vk_acceleration_build_geometry_infos[ i ].flags = infos->flags;
    vk_acceleration_build_geometry_infos[ i ].mode = CRUDE_CAST( VkBuildAccelerationStructureModeKHR, infos->mode );
    vk_acceleration_build_geometry_infos[ i ].srcAccelerationStructure = infos->src_acceleration_structure.vk_acceleration_structure;
    vk_acceleration_build_geometry_infos[ i ].dstAccelerationStructure = infos->dst_acceleration_structure.vk_acceleration_structure;
    vk_acceleration_build_geometry_infos[ i ].geometryCount = infos->geometry_count;
    vk_acceleration_build_geometry_infos[ i ].pGeometries = vk_acceleration_structure_geometries[ i ];
    vk_acceleration_build_geometry_infos[ i ].ppGeometries = NULL;
    vk_acceleration_build_geometry_infos[ i ].scratchData.deviceAddress = infos->scratch_data.device_address;
  }
  
  for ( uint32 i = 0; i < info_count; ++i )
  {
    vk_acceleration_structure_build_range_infos[ i ] = CRUDE_CAST( VkAccelerationStructureBuildRangeInfoKHR*, crude_heap_allocator_allocate( allocator, sizeof( VkAccelerationStructureBuildRangeInfoKHR ) ) );
    vk_acceleration_structure_build_range_infos[ i ]->firstVertex = build_range_infos[ i ].first_vertex;
    vk_acceleration_structure_build_range_infos[ i ]->primitiveCount = build_range_infos[ i ].primitive_count;
    vk_acceleration_structure_build_range_infos[ i ]->primitiveOffset = build_range_infos[ i ].primitive_offset;
    vk_acceleration_structure_build_range_infos[ i ]->transformOffset = build_range_infos[ i ].transform_offset;
  }

  device->vkCmdBuildAccelerationStructuresKHR( command_buffer.vk_command_buffer, 1u, vk_acceleration_build_geometry_infos, vk_acceleration_structure_build_range_infos );

  for ( uint32 i = 0; i < info_count; ++i )
  {
    crude_heap_allocator_deallocate( allocator, vk_acceleration_structure_build_range_infos[ i ] );
    CRUDE_ARRAY_DEINITIALIZE( vk_acceleration_structure_geometries[ i ] );
  }

  CRUDE_ARRAY_DEINITIALIZE( vk_acceleration_build_geometry_infos );
  CRUDE_ARRAY_DEINITIALIZE( vk_acceleration_structure_geometries );
  CRUDE_ARRAY_DEINITIALIZE( vk_acceleration_structure_build_range_infos );
}

char const*
crude_gfx_rhi_current_graphics_api_str
(
)
{
  return "Vulkan";
}

#if CRUDE_GRAPHICS_VALIDATION_LAYERS_ENABLED
VKAPI_ATTR VkBool32
crude_gfx_rhi_vk_debug_callback_
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
  //else if ( pCallbackData->pMessageIdName && strcmp( "WARNING-DEBUG-PRINTF", pCallbackData->pMessageIdName ) == 0 ) // !TODO
  //{
  //  CRUDE_LOG_INFO( CRUDE_CHANNEL_GRAPHICS, "%s", pCallbackData->pMessage );
  //}
  return VK_FALSE;
}
#endif /* CRUDE_GRAPHICS_VALIDATION_LAYERS_ENABLED */

bool
crude_gfx_rhi_vk_pick_physical_device_
(
  _In_ VkInstance                                          vk_instance,
  _In_ VkSurfaceKHR                                        vk_surface,
  _In_ crude_heap_allocator                               *allocator,
  _Out_ VkPhysicalDevice                                  *vk_selected_physical_devices,
  _Out_ crude_gfx_rhi_physical_device_optional_extensions *vk_selected_physical_devices_optional_extenstions
)
{
  VkExtensionProperties                                   *vk_available_extensions;
  uint32                                                   vk_available_extensions_count;
  VkPhysicalDevice                                         vk_available_physical_devices[ 8 ];
  VkPhysicalDeviceProperties                               vk_selected_physical_properties;
  uint32                                                   vk_available_physical_devices_count;

  vkEnumeratePhysicalDevices( vk_instance, &vk_available_physical_devices_count, NULL );
  
  if ( vk_available_physical_devices_count == 0u ) 
  {
    return false;
  }

  CRUDE_ASSERT( vk_available_physical_devices_count < CRUDE_COUNTOF( vk_available_physical_devices ) );
  
  vkEnumeratePhysicalDevices( vk_instance, &vk_available_physical_devices_count, vk_available_physical_devices );
  
  *vk_selected_physical_devices = VK_NULL_HANDLE;
  for ( uint32 try_picking = 0; try_picking < 2; ++try_picking )
  {
    bool looking_for_discrete_gpu = ( try_picking == 0 );
    bool looking_for_any_gpu = ( try_picking == 1 );

    for ( uint32 i = 0; i < vk_available_physical_devices_count; ++i )
    {
      char const                                            *not_supported_extension_name;
      VkPhysicalDeviceProperties                             vk_current_physical_properties;
      VkPhysicalDevice                                       vk_current_physical_device;
      int32                                                  vk_queue_family_index;

      vk_current_physical_device = vk_available_physical_devices[ i ];
      vkGetPhysicalDeviceProperties( vk_current_physical_device, &vk_current_physical_properties );
      
      not_supported_extension_name = "";

      if ( looking_for_discrete_gpu )
      {
        if ( vk_current_physical_properties.deviceType != VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU )
        {
          continue;
        }
      }

      if ( !crude_gfx_rhi_vk_check_support_required_extensions_( vk_current_physical_device, allocator, &not_supported_extension_name ) )
      {
        CRUDE_LOG_ERROR( CRUDE_CHANNEL_GRAPHICS, "%s physical device doesn't support requested extension \"%s\"!", vk_current_physical_properties.deviceName ? vk_current_physical_properties.deviceName : "Unknown", not_supported_extension_name ? not_supported_extension_name : "" );
        continue;
      }
      if ( !crude_gfx_rhi_vk_check_swapchain_adequate_( vk_current_physical_device, vk_surface ) )
      {
        CRUDE_LOG_ERROR( CRUDE_CHANNEL_GRAPHICS, "%s physical device doesn't support requested swap chain adequate!", vk_current_physical_properties.deviceName ? vk_current_physical_properties.deviceName : "Unknown" );
        continue;
      }
      if ( !crude_gfx_rhi_vk_check_support_required_features_( vk_current_physical_device ) )
      {
        CRUDE_LOG_ERROR( CRUDE_CHANNEL_GRAPHICS, "%s physical device doesn't support requested swap chain adequate!", vk_current_physical_properties.deviceName ? vk_current_physical_properties.deviceName : "Unknown" );
        continue;
      }
      
      vk_queue_family_index = crude_gfx_rhi_vk_get_supported_queue_family_index_( vk_current_physical_device, vk_surface, allocator ); 
      if ( vk_queue_family_index == -1 )
      {
        CRUDE_LOG_ERROR( CRUDE_CHANNEL_GRAPHICS, "%s physical device doesn't support requested queue family indices!", vk_current_physical_properties.deviceName ? vk_current_physical_properties.deviceName : "Unknown" );
        continue;
      }
      
      *vk_selected_physical_devices = vk_current_physical_device;

      try_picking = 2;
      break;
    }
  }
  

  if ( *vk_selected_physical_devices == VK_NULL_HANDLE )
  {
    CRUDE_LOG_ERROR( CRUDE_CHANNEL_GRAPHICS, "I don't fucking know why any physical device doesn't supported!" );
    return false;
  }

  vkGetPhysicalDeviceProperties( *vk_selected_physical_devices, &vk_selected_physical_properties );
  CRUDE_LOG_INFO( CRUDE_CHANNEL_GRAPHICS, "Selected physical device %s %i", vk_selected_physical_properties.deviceName, vk_selected_physical_properties.deviceType );

  vk_available_extensions_count = 0;
  vkEnumerateDeviceExtensionProperties( *vk_selected_physical_devices, NULL, &vk_available_extensions_count, NULL );
  CRUDE_ARRAY_INITIALIZE_WITH_LENGTH( vk_available_extensions, vk_available_extensions_count, crude_heap_allocator_pack( allocator ) );
  vkEnumerateDeviceExtensionProperties( *vk_selected_physical_devices, NULL, &vk_available_extensions_count, vk_available_extensions );
  
  for ( size_t i = 0; i < vk_available_extensions_count; ++i )
  {
#if !CRUDE_GRAPHICS_MESH_SHADER_DISBLED
    if ( crude_string_cmp( vk_available_extensions[ i ].extensionName, VK_EXT_MESH_SHADER_EXTENSION_NAME ) == 0 )
    {
      vk_selected_physical_devices_optional_extenstions->mesh_shaders_extension_present = true;
      continue;
    }
#endif
#if !CRUDE_GRAPHICS_FRAGMENT_SHADING_RATE_DISBLED
    if ( crude_string_cmp( vk_available_extensions[ i ].extensionName, VK_KHR_FRAGMENT_SHADING_RATE_EXTENSION_NAME ) == 0 )
    {
      vk_selected_physical_devices_optional_extenstions->fragment_shading_rate_extension_present = true;
      continue;
    }
#endif
#if !CRUDE_GRAPHICS_DEFERRED_HOST_OPERATIONS_DISBLED
    if ( crude_string_cmp( vk_available_extensions[ i ].extensionName, VK_KHR_DEFERRED_HOST_OPERATIONS_EXTENSION_NAME ) == 0 )
    {
      vk_selected_physical_devices_optional_extenstions->deferred_host_operations_extension_present = true;
      continue;
    }
#endif
#if !CRUDE_GRAPHICS_SHADER_RELAXED_EXTENDED_INSTRUCTION_DISBLED
    if ( crude_string_cmp( vk_available_extensions[ i ].extensionName, VK_KHR_SHADER_RELAXED_EXTENDED_INSTRUCTION_EXTENSION_NAME ) == 0 )
    {
      vk_selected_physical_devices_optional_extenstions->shader_relaxed_extended_instruction_extension_present = true;
      continue;
    }
#endif
  }

  CRUDE_ARRAY_DEINITIALIZE( vk_available_extensions );

  return true;
}

int32
crude_gfx_rhi_vk_get_supported_queue_family_index_
(
  _In_ VkPhysicalDevice                                    vk_physical_device,
  _In_ VkSurfaceKHR                                        vk_surface,
  _In_ crude_heap_allocator                               *allocator
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
  
  CRUDE_ARRAY_INITIALIZE_WITH_LENGTH( queue_families_properties, queue_family_count, crude_heap_allocator_pack( allocator ) );
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

  CRUDE_ARRAY_DEINITIALIZE( queue_families_properties );

  return queue_index;
}

bool
crude_gfx_rhi_vk_check_support_required_extensions_
(
  _In_ VkPhysicalDevice                                    vk_physical_device,
  _In_ crude_heap_allocator                               *allocator,
  _Out_opt_ char const                                   **not_supported_extension_name
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
    
  CRUDE_ARRAY_INITIALIZE_WITH_LENGTH( available_extensions, available_extensions_count, crude_heap_allocator_pack( allocator ) );
  vkEnumerateDeviceExtensionProperties( vk_physical_device, NULL, &available_extensions_count, available_extensions );

  support_required_extensions = true;
  for ( uint32 i = 0; i < CRUDE_COUNTOF( crude_gfx_rhi_vk_device_required_extensions ); ++i )
  {
    bool extension_found = false;
    for ( uint32 k = 0; k < available_extensions_count; ++k )
    {
      if ( strcmp( crude_gfx_rhi_vk_device_required_extensions[i], available_extensions[k].extensionName ) == 0 )
      {
        extension_found = true;
        break;
      }
    }
    if ( !extension_found )
    {
      if ( not_supported_extension_name )
      {
        *not_supported_extension_name = crude_gfx_rhi_vk_device_required_extensions[ i ];
      }
      support_required_extensions = false;
      break;
    }
  }
  
  CRUDE_ARRAY_DEINITIALIZE( available_extensions );
  return support_required_extensions;
}

bool
crude_gfx_rhi_vk_check_swapchain_adequate_
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
crude_gfx_rhi_vk_check_support_required_features_
(
  _In_ VkPhysicalDevice                                    vk_physical_device
)
{
  VkPhysicalDeviceFeatures features;
  vkGetPhysicalDeviceFeatures( vk_physical_device, &features );
  return features.samplerAnisotropy;
}

#elif CRUDE_GFX_NAPI

crude_gfx_rhi_fence
crude_gfx_rhi_fence_empty
(
)
{
  return CRUDE_COMPOUNT_EMPTY( crude_gfx_rhi_fence );
}

crude_gfx_rhi_sampler
crude_gfx_rhi_sampler_empty
(
)
{
  return CRUDE_COMPOUNT_EMPTY( crude_gfx_rhi_sampler );
}

crude_gfx_rhi_queue
crude_gfx_rhi_queue_empty
(
)
{
  return CRUDE_COMPOUNT_EMPTY( crude_gfx_rhi_queue );
}

crude_gfx_rhi_image_copy
crude_gfx_rhi_image_copy_empty
(
)
{
  return CRUDE_COMPOUNT_EMPTY( crude_gfx_rhi_image_copy );
}

crude_gfx_rhi_viewport
crude_gfx_rhi_viewport_empty
(
)
{
  return CRUDE_COMPOUNT_EMPTY( crude_gfx_rhi_viewport );
}

bool
crude_gfx_rhi_format_has_depth_or_stencil
( 
  _In_ crude_gfx_rhi_format                                value
)
{
  return false;
}

bool
crude_gfx_rhi_format_has_depth
( 
  _In_ crude_gfx_rhi_format                                value
)
{
  return false;
}

crude_gfx_rhi_access_flags
crude_gfx_rhi_resource_state_to_access_flags
(
  _In_ crude_gfx_rhi_resource_state                        state
)
{
  return CRUDE_COMPOUNT_EMPTY( crude_gfx_rhi_access_flags );
}

crude_gfx_rhi_image_layout
crude_gfx_rhi_resource_state_to_image_layout
(
  _In_ crude_gfx_rhi_resource_state                        state
)
{
  return CRUDE_COMPOUNT_EMPTY( crude_gfx_rhi_image_layout );
}

crude_gfx_rhi_pipeline_stage_flags
crude_gfx_rhi_determine_pipeline_stage_flags
(
  _In_ crude_gfx_rhi_access_flags                          access_flags,
  _In_ crude_gfx_rhi_queue_type                            queue_type
)
{
  return CRUDE_COMPOUNT_EMPTY( crude_gfx_rhi_pipeline_stage_flags );
}

crude_gfx_rhi_command_buffer_begin_info
crude_gfx_rhi_command_buffer_begin_info_empty
(
)
{
  return CRUDE_COMPOUNT_EMPTY( crude_gfx_rhi_command_buffer_begin_info );
}

bool
crude_gfx_rhi_queue_submit
(
  _In_ crude_gfx_rhi_queue                                 queue,
  _In_ uint32                                              submit_count,
  _In_ crude_gfx_rhi_submit_info                          *submit_info,
  _In_ crude_gfx_rhi_fence                                 fence
)
{
  return true;
}

void
crude_gfx_rhi_queue_submit_simple
(
  _In_ crude_gfx_rhi_queue                                 queue,
  _In_ crude_gfx_rhi_command_buffer                        command_buffer,
  _In_ crude_gfx_rhi_fence                                 fence
)
{
}

void
crude_gfx_rhi_wait_for_fence
(
  _In_ crude_gfx_rhi_device                               *device,
  _In_ crude_gfx_rhi_fence                                 fence
)
{
}

bool
crude_gfx_rhi_queue_present
(
  _In_ crude_gfx_rhi_queue                                 queue,
  _In_ crude_gfx_rhi_semaphore                             semaphore,
  _In_ crude_gfx_rhi_swapchain                             swapchain,
  _Out_ uint32                                            *image_indices
)
{
  return true;
}

void
crude_gfx_rhi_get_query_pool_results
(
  _In_ crude_gfx_rhi_device                               *device,
  _In_ crude_gfx_rhi_query_pool                            query_pool,
  _In_ uint32                                              first_query,
  _In_ uint32                                              query_count,
  _In_ uint64                                              data_size,
  _In_ void                                               *data,
  _In_ crude_gfx_rhi_device_size                           stride,
  _In_ crude_gfx_rhi_query_result_flags                    flags
)
{
}

crude_gfx_rhi_device_address
crude_gfx_rhi_get_buffer_device_address
(
  _In_ crude_gfx_rhi_device                               *device,
  _In_ crude_gfx_rhi_buffer                                buffer
)
{
  return CRUDE_COMPOUNT_EMPTY( crude_gfx_rhi_device_address );
}

void
crude_gfx_rhi_create_surface
(
  _In_ crude_gfx_rhi_instance                              instance,
  _In_ SDL_Window                                         *window,
  _Out_ crude_gfx_rhi_surface                             *surface
)
{
}

void
crude_gfx_rhi_destroy_surface
(
  _In_ crude_gfx_rhi_instance                              instance,
  _In_ crude_gfx_rhi_surface                               surface
)
{
}

void
crude_gfx_rhi_create_device
(
  _In_ crude_gfx_rhi_instance                              instance,
  _In_ crude_gfx_rhi_surface                               surface,
  _In_ crude_heap_allocator                               *allocator,
  _Out_ crude_gfx_rhi_device                              *device
)
{
}

void
crude_gfx_rhi_destroy_device
(
  _In_ crude_gfx_rhi_device                               *device,
  _In_ crude_gfx_rhi_instance                              instance
)
{
}

void
crude_gfx_rhi_create_instance
(
  _Out_ crude_gfx_rhi_instance                            *instance
)
{
}

void
crude_gfx_rhi_destroy_instance
(
  _In_ crude_gfx_rhi_instance                              instance
)
{
}

void
crude_gfx_rhi_create_buffer
(
  _In_ crude_gfx_rhi_device                               *device,
  _In_ crude_gfx_rhi_buffer_create_info const             *creation,
  _Out_ crude_gfx_rhi_buffer                              *buffer
)
{
}

void
crude_gfx_rhi_destroy_buffer
(
  _In_ crude_gfx_rhi_device                               *device,
  _In_ crude_gfx_rhi_buffer                                buffer
)
{
}

void
crude_gfx_rhi_set_buffer_debug_name
(
  _In_ crude_gfx_rhi_device                               *device,
  _In_ crude_gfx_rhi_buffer                                buffer,
  _In_ char const                                         *name
)
{
}

void
crude_gfx_rhi_map_buffer
(
  _In_ crude_gfx_rhi_device                               *device,
  _In_ crude_gfx_rhi_buffer                                buffer,
  _Out_ void                                             **data
)
{
}

void
crude_gfx_rhi_unmap_buffer
(
  _In_ crude_gfx_rhi_device                               *device,
  _In_ crude_gfx_rhi_buffer                                buffer
)
{
}

void
crude_gfx_rhi_create_image
(
  _In_ crude_gfx_rhi_device                               *device,
  _In_ crude_gfx_rhi_image_create_info const              *creation,
  _Out_ crude_gfx_rhi_image                               *image
)
{
}

void
crude_gfx_rhi_destroy_image
(
  _In_ crude_gfx_rhi_device                               *device,
  _In_ crude_gfx_rhi_image                                 image
)
{
}

void
crude_gfx_rhi_set_image_allocation_debug_name
(
  _In_ crude_gfx_rhi_device                               *device,
  _In_ crude_gfx_rhi_image                                 image,
  _In_ char const                                         *name
)
{
}

void
crude_gfx_rhi_set_image_debug_name
(
  _In_ crude_gfx_rhi_device                               *device,
  _In_ crude_gfx_rhi_image                                 image,
  _In_ char const                                         *name
)
{
}

void
crude_gfx_rhi_create_image_view
(
  _In_ crude_gfx_rhi_device                               *device,
  _In_ crude_gfx_rhi_image_view_create_info const         *creation,
  _Out_ crude_gfx_rhi_image_view                          *image_view
)
{
}

void
crude_gfx_rhi_destroy_image_view
(
  _In_ crude_gfx_rhi_device                               *device,
  _In_ crude_gfx_rhi_image_view                            image_view
)
{
}

void
crude_gfx_rhi_set_image_view_debug_name
(
  _In_ crude_gfx_rhi_device                               *device,
  _In_ crude_gfx_rhi_image_view                            image_view,
  _In_ char const                                         *name
)
{
}

void
crude_gfx_rhi_create_sampler
(
  _In_ crude_gfx_rhi_device                               *device,
  _In_ crude_gfx_rhi_sampler_create_info const            *creation,
  _Out_ crude_gfx_rhi_sampler                             *sampler
)
{
}

void
crude_gfx_rhi_destroy_sampler
(
  _In_ crude_gfx_rhi_device                               *device,
  _In_ crude_gfx_rhi_sampler                               sampler
)
{
}

void
crude_gfx_rhi_set_sampler_debug_name
(
  _In_ crude_gfx_rhi_device                               *device,
  _In_ crude_gfx_rhi_sampler                               sampler,
  _In_ char const                                         *name
)
{
}

bool
crude_gfx_rhi_create_shader_module
(
  _In_ crude_gfx_rhi_device                               *device,
  _In_ crude_gfx_rhi_shader_module_create_info const      *creation,
  _In_ crude_heap_allocator                               *allocator,
  _Out_ crude_gfx_rhi_shader_module                       *shader_module
)
{
  return true;
}

void
crude_gfx_rhi_destroy_shader_module
(
  _In_ crude_gfx_rhi_device                               *device,
  _In_ crude_gfx_rhi_shader_module                         shader_module
)
{
}

void
crude_gfx_rhi_set_shader_module_debug_name
(
  _In_ crude_gfx_rhi_device                               *device,
  _Out_ crude_gfx_rhi_shader_module                        shader_module,
  _In_ char const                                         *name
)
{
}

void
crude_gfx_rhi_create_pipeline_layout
(
  _In_ crude_gfx_rhi_device                               *device,
  _In_ crude_gfx_rhi_pipeline_layout_create_info const    *creation,
  _Out_ crude_gfx_rhi_pipeline_layout                     *pipeline_layout
)
{
}

void
crude_gfx_rhi_destroy_pipeline_layout
(
  _In_ crude_gfx_rhi_device                               *device,
  _In_ crude_gfx_rhi_pipeline_layout                       pipeline_layout
)
{
}

void
crude_gfx_rhi_set_pipeline_layout_debug_name
(
  _In_ crude_gfx_rhi_device                               *device,
  _In_ crude_gfx_rhi_pipeline_layout                       pipeline_layout,
  _In_ char const                                         *name
)
{
}

void
crude_gfx_rhi_create_classic_pipeline
(
  _In_ crude_gfx_rhi_device                               *device,
  _In_ crude_gfx_rhi_classic_pipeline_create_info const   *creation,
  _Out_ crude_gfx_rhi_pipeline                            *pipeline
)
{
}

void
crude_gfx_rhi_create_compute_pipeline
(
  _In_ crude_gfx_rhi_device                               *device,
  _In_ crude_gfx_rhi_compute_pipeline_create_info const   *creation,
  _Out_ crude_gfx_rhi_pipeline                            *pipeline
)
{
}

void
crude_gfx_rhi_create_ray_tracing_pipeline
(
  _In_ crude_gfx_rhi_device                               *device,
  _In_ crude_gfx_rhi_ray_tracing_pipeline_create_info const *creation,
  _Out_ crude_gfx_rhi_pipeline                            *pipeline
)
{
}

void
crude_gfx_rhi_destroy_pipeline
(
  _In_ crude_gfx_rhi_device                               *device,
  _In_ crude_gfx_rhi_pipeline                              pipeline
)
{
}

void
crude_gfx_rhi_get_ray_tracing_shader_group_handles
(
  _In_ crude_gfx_rhi_device                               *device,
  _In_ crude_gfx_rhi_pipeline                              pipeline,
  _In_ uint32                                              first_group,
  _In_ uint32                                              group_count,
  _In_ uint32                                              data_size,
  _Out_ void                                              *data
)
{
}

void
crude_gfx_rhi_set_pipeline_debug_name
(
  _In_ crude_gfx_rhi_device                               *device,
  _In_ crude_gfx_rhi_pipeline                              pipeline,
  _In_ char const                                         *name
)
{
}

void
crude_gfx_rhi_create_swapchain
(
  _In_ crude_gfx_rhi_device                               *device,
  _In_ crude_gfx_rhi_swapchain_create_info const          *creation,
  _In_ crude_heap_allocator                               *allocator,
  _Out_ crude_gfx_rhi_swapchain                           *swapchain,
  _Out_ uint32                                            *swapchain_images_count,
  _Out_ XMFLOAT2                                          *swapchain_extent,
  _Out_ crude_gfx_rhi_image                                swapchain_images[ CRUDE_GFX_SWAPCHAIN_IMAGES_MAX_COUNT ]
)
{
  *swapchain_images_count = 3;
}

void
crude_gfx_rhi_destroy_swapchain
(
  _In_ crude_gfx_rhi_device                               *device,
  _In_ crude_gfx_rhi_swapchain                             swapchain
)
{
}

void
crude_gfx_rhi_create_descriptor_set_layout
(
  _In_ crude_gfx_rhi_device                               *device,
  _In_ crude_gfx_rhi_descriptor_set_layout_create_info const *creation,
  _Out_ crude_gfx_rhi_descriptor_set_layout               *layout
)
{
}

void
crude_gfx_rhi_destroy_descriptor_set_layout
(
  _In_ crude_gfx_rhi_device                               *device,
  _In_ crude_gfx_rhi_descriptor_set_layout                 layout
)
{
}

void
crude_gfx_rhi_create_descriptor_set
(
  _In_ crude_gfx_rhi_device                               *device,
  _In_ crude_gfx_rhi_descriptor_set_create_info const     *creation,
  _Out_ crude_gfx_rhi_descriptor_set                      *descriptor_set
)
{
}

void
crude_gfx_rhi_set_descriptor_set_debug_name
(
  _In_ crude_gfx_rhi_device                               *device,
  _In_ crude_gfx_rhi_descriptor_set                        descriptor_set,
  _In_ char const                                         *name
)
{
}

void
crude_gfx_rhi_create_descriptor_pool
(
  _In_ crude_gfx_rhi_device                               *device,
  _In_ bool                                                bindless,
  _Out_ crude_gfx_rhi_descriptor_pool                     *descriptor_pool
)
{
}

void
crude_gfx_rhi_set_descriptor_pool_debug_name
(
  _In_ crude_gfx_rhi_device                               *device,
  _In_ crude_gfx_rhi_descriptor_pool                       descriptor_pool,
  _In_ char const                                         *name
)
{
}

void
crude_gfx_rhi_create_acceleration_structure
(
  _In_ crude_gfx_rhi_device                               *device,
  _In_ crude_gfx_rhi_acceleration_structure_create_info const *creation,
  _Out_ crude_gfx_rhi_acceleration_structure              *acceleration_structure
)
{
}

void
crude_gfx_rhi_destroy_acceleration_structure
(
  _In_ crude_gfx_rhi_device                               *device,
  _In_ crude_gfx_rhi_acceleration_structure                acceleration_structure
)
{
}

void
crude_gfx_rhi_create_command_pool
(
  _In_ crude_gfx_rhi_device                               *device,
  _In_ crude_gfx_rhi_command_pool_create_info const       *creation,
  _Out_ crude_gfx_rhi_command_pool                        *command_pool
)
{
}

void
crude_gfx_rhi_destroy_command_pool
(
  _In_ crude_gfx_rhi_device                               *device,
  _In_ crude_gfx_rhi_command_pool                          command_pool
)
{
}

void
crude_gfx_rhi_create_query_pool
(
  _In_ crude_gfx_rhi_device                               *device,
  _In_ crude_gfx_rhi_queru_pool_create_info const         *creation,
  _Out_ crude_gfx_rhi_query_pool                          *query_pool
)
{
}

void
crude_gfx_rhi_destroy_query_pool
(
  _In_ crude_gfx_rhi_device                               *device,
  _In_ crude_gfx_rhi_query_pool                            query_pool
)
{
}

void
crude_gfx_rhi_create_command_buffer
(
  _In_ crude_gfx_rhi_device                               *device,
  _In_ crude_gfx_rhi_command_buffer_create_info const     *creation,
  _Out_ crude_gfx_rhi_command_buffer                      *command_buffer
)
{
}

void
crude_gfx_rhi_destroy_command_buffer
(
  _In_ crude_gfx_rhi_device                               *device,
  _In_ crude_gfx_rhi_command_buffer                        command_buffer
)
{
}

void
crude_gfx_rhi_set_command_buffer_debug_name
(
  _In_ crude_gfx_rhi_device                               *device,
  _In_ crude_gfx_rhi_command_buffer                        command_buffer,
  _In_ char const                                         *name
)
{
}

crude_gfx_rhi_queue
crude_gfx_rhi_device_get_graphics_queue
(
  _In_ crude_gfx_rhi_device                               *device
)
{
  return crude_gfx_rhi_queue_empty( );
}

crude_gfx_rhi_queue
crude_gfx_rhi_device_get_transfer_queue
(
  _In_ crude_gfx_rhi_device                               *device
)
{
  return crude_gfx_rhi_queue_empty( );
}

void
crude_gfx_rhi_update_descriptor_set
(
  _In_ crude_gfx_rhi_device                               *device,
  _In_ crude_gfx_rhi_descriptor_set                        descriptor_set,
  _In_ crude_gfx_rhi_write_descriptor_set                 *write_descripor_sets,
  _In_ uint32                                              write_descripor_sets_count
)
{
}

crude_gfx_rhi_physical_device_optional_extensions const*
crude_gfx_rhi_get_device_optional_extensions
(
  _In_ crude_gfx_rhi_device                               *device
)
{
  return &device->optional_extensions;
}

void*
crude_gfx_rhi_get_buffer_mapped_data
(
  _In_ crude_gfx_rhi_buffer                                buffer
)
{
  return NULL;
}

void
crude_gfx_rhi_wait_semaphore
(
  _In_ crude_gfx_rhi_device                               *device,
  _In_ crude_gfx_rhi_semaphore                             semaphore,
  _In_ uint64                                              value
)
{
}

XMFLOAT2
crude_gfx_rhi_get_surface_extent
(
  _In_ crude_gfx_rhi_device                               *device,
  _In_ crude_gfx_rhi_surface                               surface
)
{
  return CRUDE_COMPOUNT_EMPTY( XMFLOAT2 );
}

float32
crude_gfx_rhi_get_timestamp_period
(
  _In_ crude_gfx_rhi_device                               *device
)
{
  return 0;
}

bool
crude_gfx_rhi_acquire_next_image
(
  _In_ crude_gfx_rhi_device                               *device,
  _In_ crude_gfx_rhi_swapchain                             swapchain,
  _In_ uint64                                              timeout,
  _In_ crude_gfx_rhi_semaphore                             semaphore,
  _Out_ uint32                                            *image_index
)
{
  return true;
}

void
crude_gfx_rhi_wait_idle
(
  _In_ crude_gfx_rhi_device                               *device
)
{
}

void
crude_gfx_rhi_create_semaphore
(
  _In_ crude_gfx_rhi_device                               *device,
  _In_ bool                                                timeline,
  _Out_ crude_gfx_rhi_semaphore                           *semaphore
)
{
}

void
crude_gfx_rhi_destroy_semaphore
(
  _In_ crude_gfx_rhi_device                               *device,
  _In_ crude_gfx_rhi_semaphore                             semaphore
)
{
}

void
crude_gfx_rhi_set_semaphore_debug_name
(
  _In_ crude_gfx_rhi_device                               *device,
  _In_ crude_gfx_rhi_semaphore                             semaphore,
  _In_ char const                                         *name
)
{
}

void
crude_gfx_rhi_create_fence
(
  _In_ crude_gfx_rhi_device                               *device,
  _In_ bool                                                signaled,
  _Out_ crude_gfx_rhi_fence                               *fence
)
{
}

void
crude_gfx_rhi_destroy_fence
(
  _In_ crude_gfx_rhi_device                               *device,
  _In_ crude_gfx_rhi_fence                                 fence
)
{
}

void
crude_gfx_rhi_reset_fence
(
  _In_ crude_gfx_rhi_device                               *device,
  _In_ crude_gfx_rhi_fence                                *fence
)
{
}

void
crude_gfx_rhi_set_fence_debug_name
(
  _In_ crude_gfx_rhi_device                               *device,
  _In_ crude_gfx_rhi_fence                                 fence,
  _In_ char const                                         *name
)
{
}

void
crude_gfx_rhi_set_debug_utils_object_name
(
  _In_ crude_gfx_rhi_device                               *device,
  _In_ crude_gfx_rhi_object_type                           object_type,
  _In_ uint64                                              object_handle,
  _In_ char const                                         *object_name
)
{
}

void
crude_gfx_rhi_destroy_descriptor_pool
(
  _In_ crude_gfx_rhi_device                               *device,
  _Out_ crude_gfx_rhi_descriptor_pool                     *descriptor_pool
)
{
}

void
crude_gfx_rhi_get_device_memory_budget
(
  _In_ crude_gfx_rhi_device                               *device,
  _Out_ crude_gfx_rhi_device_memory_budget                *budget
)
{
}

void
crude_gfx_rhi_get_device_ray_tracing_pipeline_properties
(
  _In_ crude_gfx_rhi_device                               *device,
  _Out_ crude_gfx_rhi_device_ray_tracing_pipeline_properties *ray_tracing_properties
)
{
}

void
crude_gfx_rhi_get_device_name
(
  _In_ crude_gfx_rhi_device                               *device,
  _Out_ char                                               name[ 256 ]
)
{
}

void
crude_gfx_rhi_get_acceleration_structure_build_sizes
(
  _In_ crude_gfx_rhi_device                               *device,
  _In_ crude_heap_allocator                               *allocator,
  _In_ crude_gfx_rhi_acceleration_structure_build_type     build_type,
  _In_ crude_gfx_rhi_acceleration_structure_build_geometry_info const *build_info,
  _In_ uint32 const                                       *max_primitives_count,
  _Out_ crude_gfx_rhi_acceleration_structure_build_sizes_info *build_size_info
)
{
}

void
crude_gfx_rhi_reset_command_pool
(
  _In_ crude_gfx_rhi_device                               *device,
  _In_ crude_gfx_rhi_command_pool                          command_pool
)
{
}

void
crude_gfx_rhi_begin_command_buffer
(
  _In_ crude_gfx_rhi_command_pool                          command_pool,
  _In_ crude_gfx_rhi_command_buffer                        command_buffer,
  _In_ crude_gfx_rhi_command_buffer_begin_info const      *begin_info
)
{
}

void
crude_gfx_rhi_end_command_buffer
(
  _In_ crude_gfx_rhi_command_buffer                        command_buffer
)
{
}

void
crude_gfx_rhi_command_buffer_begin_rendering
(
  _In_ crude_gfx_rhi_command_buffer                        command_buffer,
  _In_ crude_gfx_rhi_rendering_info const                 *rendering_info
)
{
}

void
crude_gfx_rhi_command_buffer_end_rendering
(
  _In_ crude_gfx_rhi_command_buffer                        command_buffer
)
{
}

void
crude_gfx_rhi_command_buffer_bind_pipeline
(
  _In_ crude_gfx_rhi_command_buffer                        command_buffer,
  _In_ crude_gfx_rhi_pipeline                              rhi_pipeline,
  _In_ crude_gfx_rhi_pipeline_bind_point                   rhi_pipeline_bind_point
)
{
}

void
crude_gfx_rhi_command_buffer_set_viewport
(
  _In_ crude_gfx_rhi_command_buffer                        command_buffer,
  _In_ crude_gfx_rhi_viewport const                       *viewport
)
{
}

void
crude_gfx_rhi_command_buffer_set_scissor
(
  _In_ crude_gfx_rhi_command_buffer                        command_buffer,
  _In_ crude_gfx_rhi_scissor const                        *scissor
)
{
}

void
crude_gfx_rhi_command_buffer_draw
(
  _In_ crude_gfx_rhi_command_buffer                        command_buffer,
  _In_ uint32                                              vertex_count,
  _In_ uint32                                              instance_count,
  _In_ uint32                                              first_vertex,
  _In_ uint32                                              first_instance
)
{
}

void
crude_gfx_rhi_command_buffer_draw_indirect
(
  _In_ crude_gfx_rhi_command_buffer                        command_buffer,
  _In_ crude_gfx_rhi_buffer                                buffer,
  _In_ crude_gfx_rhi_device_size                           offset,
  _In_ uint32                                              draw_count,
  _In_ uint32                                              stride
)
{
}

void
crude_gfx_rhi_command_buffer_draw_indirect_count
(
  _In_ crude_gfx_rhi_command_buffer                        command_buffer,
  _In_ crude_gfx_rhi_buffer                                argument_buffer,
  _In_ crude_gfx_rhi_device_size                           argument_buffer_offset,
  _In_ crude_gfx_rhi_buffer                                count_buffer,
  _In_ crude_gfx_rhi_device_size                           count_buffer_offset,
  _In_ uint32                                              max_draw_count,
  _In_ uint32                                              stride
)
{
}

void
crude_gfx_rhi_command_buffer_draw_mesh_task
(
  _In_ crude_gfx_rhi_device                               *device,
  _In_ crude_gfx_rhi_command_buffer                        command_buffer,
  _In_ uint32                                              group_count_x,
  _In_ uint32                                              group_count_y,
  _In_ uint32                                              group_count_z
)
{
}

void
crude_gfx_rhi_command_buffer_draw_mesh_task_indirect_count
(
  _In_ crude_gfx_rhi_device                               *device,
  _In_ crude_gfx_rhi_command_buffer                        command_buffer,
  _In_ crude_gfx_rhi_buffer                                argument_buffer,
  _In_ crude_gfx_rhi_device_size                           argument_buffer_offset,
  _In_ crude_gfx_rhi_buffer                                count_buffer,
  _In_ crude_gfx_rhi_device_size                           count_buffer_offset,
  _In_ uint32                                              max_draw_count,
  _In_ uint32                                              stride
)
{
}

void
crude_gfx_rhi_command_buffer_dispatch
(
  _In_ crude_gfx_rhi_command_buffer                        command_buffer,
  _In_ uint32                                              group_count_x,
  _In_ uint32                                              group_count_y,
  _In_ uint32                                              group_count_z
)
{
}

void
crude_gfx_rhi_command_buffer_bind_descriptor_sets
(
  _In_ crude_gfx_rhi_command_buffer                        command_buffer,
  _In_ crude_gfx_rhi_pipeline_bind_point                   pipeline_bind_point,
  _In_ crude_gfx_rhi_pipeline_layout                       pipeline_layout,
  _In_ uint32                                              set,
  _In_ crude_gfx_rhi_descriptor_set                        descriptor_set
)
{
}

void
crude_gfx_rhi_command_buffer_pipeline_image_barrier
(
  _In_ crude_gfx_rhi_command_buffer                        command_buffer,
  _In_ crude_gfx_rhi_image_memory_barrier const           *image_memory_barriers
)
{
}

void
crude_gfx_rhi_command_buffer_pipeline_buffer_barrier
(
  _In_ crude_gfx_rhi_command_buffer                        command_buffer,
  _In_ crude_gfx_rhi_buffer_memory_barrier                *buffer_memory_barriers
)
{
}

void
crude_gfx_rhi_command_buffer_pipeline_global_barrier
(
  _In_ crude_gfx_rhi_command_buffer                        command_buffer
)
{
}

void
crude_gfx_rhi_command_buffer_copy_buffer_to_image
(
  _In_ crude_gfx_rhi_command_buffer                        command_buffer,
  _In_ crude_gfx_rhi_buffer                                buffer,
  _In_ crude_gfx_rhi_image                                 image,
  _In_ crude_gfx_rhi_buffer_image_copy const              *region
)
{
}

void
crude_gfx_rhi_command_buffer_copy_buffer
(
  _In_ crude_gfx_rhi_command_buffer                        command_buffer,
  _In_ crude_gfx_rhi_buffer                                src_buffer,
  _In_ crude_gfx_rhi_buffer                                dst_buffer,
  _In_ crude_gfx_rhi_buffer_copy const                    *region
)
{
}

void
crude_gfx_rhi_command_buffer_write_timestamp
(
  _In_ crude_gfx_rhi_command_buffer                        command_buffer,
  _In_ crude_gfx_rhi_pipeline_stage_flags                  pipeline_stage,
  _In_ crude_gfx_rhi_query_pool                            query_pool,
  _In_ uint32                                              query
)
{
}

void
crude_gfx_rhi_command_buffer_begin_debug_utils_label
(
  _In_ crude_gfx_rhi_device                               *device,
  _In_ crude_gfx_rhi_command_buffer                        command_buffer,
  _In_ crude_gfx_rhi_debug_utils_label const              *debug_utils_label
)
{
}

void
crude_gfx_rhi_command_buffer_end_debug_utils_label
(
  _In_ crude_gfx_rhi_device                               *device,
  _In_ crude_gfx_rhi_command_buffer                        command_buffer
)
{
}

void
crude_gfx_rhi_command_buffer_push_constant
(
  _In_ crude_gfx_rhi_command_buffer                        command_buffer,
  _In_ crude_gfx_rhi_pipeline_layout                       layout,
  _In_ crude_gfx_rhi_shader_stage_flags                    stage_flags,
  _In_ uint32                                              offset,
  _In_ uint32                                              size,
  _In_ void const                                         *values
)
{
}

void
crude_gfx_rhi_command_buffer_fill_buffer
(
  _In_ crude_gfx_rhi_command_buffer                        command_buffer,
  _In_ crude_gfx_rhi_buffer                                dst_buffer,
  _In_ crude_gfx_rhi_device_size                           dst_offset,
  _In_ crude_gfx_rhi_device_size                           size,
  _In_ uint32                                              data
)
{
}

void
crude_gfx_rhi_command_buffer_trace_rays
(
  _In_ crude_gfx_rhi_device                               *device,
  _In_ crude_gfx_rhi_command_buffer                        command_buffer,
  _In_ crude_gfx_rhi_strided_device_address_region const  *raygen_shader_binding_table,
  _In_ crude_gfx_rhi_strided_device_address_region const  *miss_shader_binding_table,
  _In_ crude_gfx_rhi_strided_device_address_region const  *hit_shader_binding_table,
  _In_ crude_gfx_rhi_strided_device_address_region const  *callable_shader_binding_table,
  _In_ uint32                                              width,
  _In_ uint32                                              height,
  _In_ uint32                                              depth
)
{
}

void
crude_gfx_rhi_command_buffer_begin_query
(
  _In_ crude_gfx_rhi_command_buffer                        command_buffer,
  _In_ crude_gfx_rhi_query_pool                            query_pool,
  _In_ uint32                                              query,
  _In_ crude_gfx_rhi_query_control_flags                   flags
)
{
}

void
crude_gfx_rhi_command_buffer_end_query
(
  _In_ crude_gfx_rhi_command_buffer                        command_buffer,
  _In_ crude_gfx_rhi_query_pool                            query_pool,
  _In_ uint32                                              query
)
{
}

void
crude_gfx_rhi_command_buffer_reset_query_pool
(
  _In_ crude_gfx_rhi_command_buffer                        command_buffer,
  _In_ crude_gfx_rhi_query_pool                            query_pool,
  _In_ uint32                                              first_query,
  _In_ uint32                                              query_count
)
{
}

void
crude_gfx_rhi_command_buffer_copy_image
(
  _In_ crude_gfx_rhi_command_buffer                        command_buffer,
  _In_ crude_gfx_rhi_image                                 src_image,
  _In_ crude_gfx_rhi_image_layout                          src_image_layout,
  _In_ crude_gfx_rhi_image                                 dst_image,
  _In_ crude_gfx_rhi_image_layout                          dst_image_layout,
  _In_ crude_gfx_rhi_image_copy const                     *region
)
{
}

void
crude_gfx_rhi_command_buffer_blit_image
(
  _In_ crude_gfx_rhi_command_buffer                        command_buffer,
  _In_ crude_gfx_rhi_image                                 src_image,
  _In_ crude_gfx_rhi_image_layout                          src_image_layout,
  _In_ crude_gfx_rhi_image                                 dst_image,
  _In_ crude_gfx_rhi_image_layout                          dst_image_layout,
  _In_ crude_gfx_rhi_image_blit const                     *region,
  _In_ crude_gfx_rhi_filter                                filter
)
{
}

void
crude_gfx_rhi_reset_command_buffer
(
  _In_ crude_gfx_rhi_command_buffer                        command_buffer
)
{
}

void
crude_gfx_rhi_command_buffer_build_acceleration_structures
(
  _In_ crude_gfx_rhi_device                                                   *device,
  _In_ crude_heap_allocator                                                   *allocator,
  _In_ crude_gfx_rhi_command_buffer                                            command_buffer,
  _In_ uint32                                                                  info_count,
  _In_ crude_gfx_rhi_acceleration_structure_build_geometry_info const         *infos,
  _In_ crude_gfx_rhi_acceleration_structure_build_range_info const            *build_range_infos
)
{
}

char const*
crude_gfx_rhi_current_graphics_api_str
(
)
{
  return "None";
}


#elif CRUDE_GFX_DX12

static bool
crude_gfx_rhi_dx12_pick_physical_device_
(
  _In_ crude_heap_allocator                               *allocator,
  _In_ IDXGIFactory6                                      *dxgi_factory,
  _Out_ ID3D12Device2                                    **dx12_device,
  _Out_ IDXGIAdapter4                                    **dx12_adapter,
  _Out_ uint32                                            *dx12_device_index,
  _Out_ crude_gfx_rhi_physical_device_optional_extensions *selected_physical_devices_optional_extenstions
);

crude_gfx_rhi_fence
crude_gfx_rhi_fence_empty
(
)
{
  return CRUDE_COMPOUNT_EMPTY( crude_gfx_rhi_fence );
}

crude_gfx_rhi_sampler
crude_gfx_rhi_sampler_empty
(
)
{
  return CRUDE_COMPOUNT_EMPTY( crude_gfx_rhi_sampler );
}

crude_gfx_rhi_queue
crude_gfx_rhi_queue_empty
(
)
{
  return CRUDE_COMPOUNT_EMPTY( crude_gfx_rhi_queue );
}

crude_gfx_rhi_image_copy
crude_gfx_rhi_image_copy_empty
(
)
{
  return CRUDE_COMPOUNT_EMPTY( crude_gfx_rhi_image_copy );
}

crude_gfx_rhi_viewport
crude_gfx_rhi_viewport_empty
(
)
{
  return CRUDE_COMPOUNT_EMPTY( crude_gfx_rhi_viewport );
}

bool
crude_gfx_rhi_format_has_depth_or_stencil
( 
  _In_ crude_gfx_rhi_format                                value
)
{
  return false;
}

bool
crude_gfx_rhi_format_has_depth
( 
  _In_ crude_gfx_rhi_format                                value
)
{
  return false;
}

crude_gfx_rhi_access_flags
crude_gfx_rhi_resource_state_to_access_flags
(
  _In_ crude_gfx_rhi_resource_state                        state
)
{
  return CRUDE_COMPOUNT_EMPTY( crude_gfx_rhi_access_flags );
}

crude_gfx_rhi_image_layout
crude_gfx_rhi_resource_state_to_image_layout
(
  _In_ crude_gfx_rhi_resource_state                        state
)
{
  return CRUDE_COMPOUNT_EMPTY( crude_gfx_rhi_image_layout );
}

crude_gfx_rhi_pipeline_stage_flags
crude_gfx_rhi_determine_pipeline_stage_flags
(
  _In_ crude_gfx_rhi_access_flags                          access_flags,
  _In_ crude_gfx_rhi_queue_type                            queue_type
)
{
  return CRUDE_COMPOUNT_EMPTY( crude_gfx_rhi_pipeline_stage_flags );
}

crude_gfx_rhi_command_buffer_begin_info
crude_gfx_rhi_command_buffer_begin_info_empty
(
)
{
  return CRUDE_COMPOUNT_EMPTY( crude_gfx_rhi_command_buffer_begin_info );
}

bool
crude_gfx_rhi_queue_submit
(
  _In_ crude_gfx_rhi_queue                                 queue,
  _In_ uint32                                              submit_count,
  _In_ crude_gfx_rhi_submit_info                          *submit_info,
  _In_ crude_gfx_rhi_fence                                 fence
)
{
  return true;
}

void
crude_gfx_rhi_queue_submit_simple
(
  _In_ crude_gfx_rhi_queue                                 queue,
  _In_ crude_gfx_rhi_command_buffer                        command_buffer,
  _In_ crude_gfx_rhi_fence                                 fence
)
{
}

void
crude_gfx_rhi_wait_for_fence
(
  _In_ crude_gfx_rhi_device                               *device,
  _In_ crude_gfx_rhi_fence                                 fence
)
{
}

bool
crude_gfx_rhi_queue_present
(
  _In_ crude_gfx_rhi_queue                                 queue,
  _In_ crude_gfx_rhi_semaphore                             semaphore,
  _In_ crude_gfx_rhi_swapchain                             swapchain,
  _Out_ uint32                                            *image_indices
)
{
  return true;
}

void
crude_gfx_rhi_get_query_pool_results
(
  _In_ crude_gfx_rhi_device                               *device,
  _In_ crude_gfx_rhi_query_pool                            query_pool,
  _In_ uint32                                              first_query,
  _In_ uint32                                              query_count,
  _In_ uint64                                              data_size,
  _In_ void                                               *data,
  _In_ crude_gfx_rhi_device_size                           stride,
  _In_ crude_gfx_rhi_query_result_flags                    flags
)
{
}

crude_gfx_rhi_device_address
crude_gfx_rhi_get_buffer_device_address
(
  _In_ crude_gfx_rhi_device                               *device,
  _In_ crude_gfx_rhi_buffer                                buffer
)
{
  return CRUDE_COMPOUNT_EMPTY( crude_gfx_rhi_device_address );
}

void
crude_gfx_rhi_create_surface
(
  _In_ crude_gfx_rhi_instance                              instance,
  _In_ SDL_Window                                         *window,
  _Out_ crude_gfx_rhi_surface                             *surface
)
{
  surface->sdl_window = window;
}

void
crude_gfx_rhi_destroy_surface
(
  _In_ crude_gfx_rhi_instance                              instance,
  _In_ crude_gfx_rhi_surface                               surface
)
{
}

void
crude_gfx_rhi_create_device
(
  _In_ crude_gfx_rhi_instance                              instance,
  _In_ crude_gfx_rhi_surface                               surface,
  _In_ crude_heap_allocator                               *allocator,
  _Out_ crude_gfx_rhi_device                              *device
)
{
  UINT                                                     dxgi_factory_flags;
  D3D12_COMMAND_QUEUE_DESC                                 dx_queue_creation;
  
  dxgi_factory_flags = 0u;
#if CRUDE_GRAPHICS_VALIDATION_LAYERS_ENABLED
  dxgi_factory_flags |= DXGI_CREATE_FACTORY_DEBUG;
#endif /* CRUDE_GRAPHICS_VALIDATION_LAYERS_ENABLED */

  CRUDE_GFX_RHI_HANDLE_DX12_RESULT( CreateDXGIFactory2( dxgi_factory_flags, IID_PPV_ARGS( &device->dxgi_factory ) ), "Failed create dxgi factory" );
  
  crude_gfx_rhi_dx12_pick_physical_device_( allocator, device->dxgi_factory, &device->dx12_device, &device->dx12_adapter, &device->dx12_device_index, &device->optional_extensions );
    
  dx_queue_creation = CRUDE_COMPOUNT_EMPTY( D3D12_COMMAND_QUEUE_DESC );
  dx_queue_creation.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
  dx_queue_creation.Type = D3D12_COMMAND_LIST_TYPE_COPY;
  CRUDE_GFX_RHI_HANDLE_DX12_RESULT( device->dx12_device->CreateCommandQueue( &dx_queue_creation, IID_PPV_ARGS( &device->transfer_queue.dx12_queue ) ), "Failed to transfer create queue factory" );
  
  dx_queue_creation = CRUDE_COMPOUNT_EMPTY( D3D12_COMMAND_QUEUE_DESC );
  dx_queue_creation.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
  dx_queue_creation.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
  CRUDE_GFX_RHI_HANDLE_DX12_RESULT( device->dx12_device->CreateCommandQueue( &dx_queue_creation, IID_PPV_ARGS( &device->main_queue.dx12_queue ) ), "Failed to create main queue factory" );
}

void
crude_gfx_rhi_destroy_device
(
  _In_ crude_gfx_rhi_device                               *device,
  _In_ crude_gfx_rhi_instance                              instance
)
{
  device->main_queue.dx12_queue->Release( );
  device->transfer_queue.dx12_queue->Release( );
  device->dx12_device->Release( );
  device->dx12_adapter->Release( );
  device->dxgi_factory->Release( );
}

void
crude_gfx_rhi_create_instance
(
  _Out_ crude_gfx_rhi_instance                            *instance
)
{
#if CRUDE_GRAPHICS_VALIDATION_LAYERS_ENABLED
  instance->dx12_debug_controller = NULL;
  if ( SUCCEEDED( D3D12GetDebugInterface( IID_PPV_ARGS( &instance->dx12_debug_controller ) ) ) )
  {
    instance->dx12_debug_controller->EnableDebugLayer( );
  }
#endif /* CRUDE_GRAPHICS_VALIDATION_LAYERS_ENABLED */
}

void
crude_gfx_rhi_destroy_instance
(
  _In_ crude_gfx_rhi_instance                              instance
)
{
  instance.dx12_debug_controller->Release( );
}

void
crude_gfx_rhi_create_buffer
(
  _In_ crude_gfx_rhi_device                               *device,
  _In_ crude_gfx_rhi_buffer_create_info const             *creation,
  _Out_ crude_gfx_rhi_buffer                              *buffer
)
{
}

void
crude_gfx_rhi_destroy_buffer
(
  _In_ crude_gfx_rhi_device                               *device,
  _In_ crude_gfx_rhi_buffer                                buffer
)
{
}

void
crude_gfx_rhi_set_buffer_debug_name
(
  _In_ crude_gfx_rhi_device                               *device,
  _In_ crude_gfx_rhi_buffer                                buffer,
  _In_ char const                                         *name
)
{
}

void
crude_gfx_rhi_map_buffer
(
  _In_ crude_gfx_rhi_device                               *device,
  _In_ crude_gfx_rhi_buffer                                buffer,
  _Out_ void                                             **data
)
{
}

void
crude_gfx_rhi_unmap_buffer
(
  _In_ crude_gfx_rhi_device                               *device,
  _In_ crude_gfx_rhi_buffer                                buffer
)
{
}

void
crude_gfx_rhi_create_image
(
  _In_ crude_gfx_rhi_device                               *device,
  _In_ crude_gfx_rhi_image_create_info const              *creation,
  _Out_ crude_gfx_rhi_image                               *image
)
{
}

void
crude_gfx_rhi_destroy_image
(
  _In_ crude_gfx_rhi_device                               *device,
  _In_ crude_gfx_rhi_image                                 image
)
{
}

void
crude_gfx_rhi_set_image_allocation_debug_name
(
  _In_ crude_gfx_rhi_device                               *device,
  _In_ crude_gfx_rhi_image                                 image,
  _In_ char const                                         *name
)
{
}

void
crude_gfx_rhi_set_image_debug_name
(
  _In_ crude_gfx_rhi_device                               *device,
  _In_ crude_gfx_rhi_image                                 image,
  _In_ char const                                         *name
)
{
}

void
crude_gfx_rhi_create_image_view
(
  _In_ crude_gfx_rhi_device                               *device,
  _In_ crude_gfx_rhi_image_view_create_info const         *creation,
  _Out_ crude_gfx_rhi_image_view                          *image_view
)
{
}

void
crude_gfx_rhi_destroy_image_view
(
  _In_ crude_gfx_rhi_device                               *device,
  _In_ crude_gfx_rhi_image_view                            image_view
)
{
}

void
crude_gfx_rhi_set_image_view_debug_name
(
  _In_ crude_gfx_rhi_device                               *device,
  _In_ crude_gfx_rhi_image_view                            image_view,
  _In_ char const                                         *name
)
{
}

void
crude_gfx_rhi_create_sampler
(
  _In_ crude_gfx_rhi_device                               *device,
  _In_ crude_gfx_rhi_sampler_create_info const            *creation,
  _Out_ crude_gfx_rhi_sampler                             *sampler
)
{
}

void
crude_gfx_rhi_destroy_sampler
(
  _In_ crude_gfx_rhi_device                               *device,
  _In_ crude_gfx_rhi_sampler                               sampler
)
{
}

void
crude_gfx_rhi_set_sampler_debug_name
(
  _In_ crude_gfx_rhi_device                               *device,
  _In_ crude_gfx_rhi_sampler                               sampler,
  _In_ char const                                         *name
)
{
}

bool
crude_gfx_rhi_create_shader_module
(
  _In_ crude_gfx_rhi_device                               *device,
  _In_ crude_gfx_rhi_shader_module_create_info const      *creation,
  _In_ crude_heap_allocator                               *allocator,
  _Out_ crude_gfx_rhi_shader_module                       *shader_module
)
{
  shader_module->allocator = allocator;
  shader_module->code = crude_heap_allocator_allocate( allocator, creation->code_size );
  shader_module->code_size = creation->code_size;
  crude_memory_copy( shader_module->code, creation->code, creation->code_size );
  return true;
}

void
crude_gfx_rhi_destroy_shader_module
(
  _In_ crude_gfx_rhi_device                               *device,
  _In_ crude_gfx_rhi_shader_module                         shader_module
)
{
  crude_heap_allocator_deallocate( shader_module.allocator, shader_module.code );
}

void
crude_gfx_rhi_set_shader_module_debug_name
(
  _In_ crude_gfx_rhi_device                               *device,
  _Out_ crude_gfx_rhi_shader_module                        shader_module,
  _In_ char const                                         *name
)
{
}

void
crude_gfx_rhi_create_pipeline_layout
(
  _In_ crude_gfx_rhi_device                               *device,
  _In_ crude_gfx_rhi_pipeline_layout_create_info const    *creation,
  _Out_ crude_gfx_rhi_pipeline_layout                     *pipeline_layout
)
{
  D3D12_ROOT_SIGNATURE_DESC                                dx12_root_signature_description;
  D3D12_DESCRIPTOR_RANGE                                   dx12_descriptor_ranges[ 5 ];
  D3D12_ROOT_PARAMETER                                     dx12_root_parameters[ 5 ];
  uint32                                                   dx12_parameters_count;
  ID3DBlob                                                *dx12_serialized_blob;
  
  dx12_parameters_count = 0; //creation->set_layout_count;
  
  //for ( uint32 i = 0; i < dx12_parameters_count; ++i )
  //{
  //  dx12_descriptor_ranges[ i ] = CRUDE_COMPOUNT_EMPTY( D3D12_DESCRIPTOR_RANGE );
  //  dx12_descriptor_ranges[ i ].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
  //  dx12_descriptor_ranges[ i ].NumDescriptors = creation->set_layouts[ i ].num_descriptors;
  //  dx12_descriptor_ranges[ i ].BaseShaderRegister = creation->set_layouts[ i ].base_register;
  //  dx12_descriptor_ranges[ i ].RegisterSpace = creation->set_layouts[ i ].register_space;
  //  dx12_descriptor_ranges[ i ].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;
  //
  //  dx12_root_parameters[ i ] = CRUDE_COMPOUNT_EMPTY( D3D12_ROOT_PARAMETER );
  //  dx12_root_parameters[ i ].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
  //  dx12_root_parameters[ i ].DescriptorTable.NumDescriptorRanges = 1;
  //  dx12_root_parameters[ i ].DescriptorTable.pDescriptorRanges = &dx12_descriptor_ranges[ i ];
  //  dx12_root_parameters[ i ].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
  //}
  
  if ( creation->has_push_constant_range )
  {
    dx12_root_parameters[ dx12_parameters_count ] = CRUDE_COMPOUNT_EMPTY( D3D12_ROOT_PARAMETER );
    dx12_root_parameters[ dx12_parameters_count].ParameterType = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;
    dx12_root_parameters[ dx12_parameters_count].Constants.ShaderRegister = 0;
    dx12_root_parameters[ dx12_parameters_count].Constants.RegisterSpace = 0;
    dx12_root_parameters[ dx12_parameters_count].Constants.Num32BitValues = ( creation->push_constant_range.size + 3 ) / 4;
    dx12_root_parameters[ dx12_parameters_count].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
    ++dx12_parameters_count;
  }
  
  dx12_root_signature_description = CRUDE_COMPOUNT_EMPTY( D3D12_ROOT_SIGNATURE_DESC );
  dx12_root_signature_description.NumParameters = 0;//dx12_parameters_count;
  dx12_root_signature_description.pParameters = NULL;//dx12_root_parameters;
  dx12_root_signature_description.NumStaticSamplers = 0;
  dx12_root_signature_description.pStaticSamplers = NULL;
  dx12_root_signature_description.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
  
  CRUDE_GFX_RHI_HANDLE_DX12_RESULT( D3D12SerializeRootSignature( &dx12_root_signature_description, D3D_ROOT_SIGNATURE_VERSION_1, &dx12_serialized_blob, NULL ), "Failed D3D12SerializeRootSignature" );
  CRUDE_GFX_RHI_HANDLE_DX12_RESULT( device->dx12_device->CreateRootSignature( 0, dx12_serialized_blob->GetBufferPointer( ), dx12_serialized_blob->GetBufferSize( ), IID_PPV_ARGS( &pipeline_layout->dx12_root_signature ) ), "Failed CreateRootSignature" );
  
  dx12_serialized_blob->Release( );
}

void
crude_gfx_rhi_destroy_pipeline_layout
(
  _In_ crude_gfx_rhi_device                               *device,
  _In_ crude_gfx_rhi_pipeline_layout                       pipeline_layout
)
{
  pipeline_layout.dx12_root_signature->Release( );
}

void
crude_gfx_rhi_set_pipeline_layout_debug_name
(
  _In_ crude_gfx_rhi_device                               *device,
  _In_ crude_gfx_rhi_pipeline_layout                       pipeline_layout,
  _In_ char const                                         *name
)
{
}

void
crude_gfx_rhi_create_task_pipeline
(
  _In_ crude_gfx_rhi_device                               *device,
  _In_ crude_gfx_rhi_task_pipeline_create_info const      *creation,
  _Out_ crude_gfx_rhi_pipeline                            *pipeline
)
{
  D3D12_PIPELINE_STATE_STREAM_DESC                         stream_description;
  D3DX12_MESH_SHADER_PIPELINE_STATE_DESC                   dx12_pipeline_creation;
  CD3DX12_PIPELINE_MESH_STATE_STREAM                       dx12_pipeline_mesh_stream;

  dx12_pipeline_creation = CRUDE_COMPOUNT_EMPTY( D3DX12_MESH_SHADER_PIPELINE_STATE_DESC );

  dx12_pipeline_creation.BlendState = CRUDE_COMPOUNT_EMPTY( D3D12_BLEND_DESC );
  dx12_pipeline_creation.BlendState.AlphaToCoverageEnable = creation->multisample_state->alpha_to_coverage_enable ? TRUE : FALSE;
  dx12_pipeline_creation.BlendState.IndependentBlendEnable = TRUE;
  for ( uint32 i = 0; i < creation->color_blend_state->attachments_count; ++i )
  {
    crude_gfx_rhi_pipeline_color_blend_attachment_state const       *rhi_attachment_state;

    rhi_attachment_state = &creation->color_blend_state->attachments[ i ];

    dx12_pipeline_creation.BlendState.RenderTarget[ i ] = CRUDE_COMPOUNT_EMPTY( D3D12_RENDER_TARGET_BLEND_DESC );
    dx12_pipeline_creation.BlendState.RenderTarget[ i ].BlendEnable = rhi_attachment_state->blend_enable ? TRUE : FALSE;
    dx12_pipeline_creation.BlendState.RenderTarget[ i ].LogicOpEnable = creation->color_blend_state->logic_op_enable ? TRUE : FALSE;
    dx12_pipeline_creation.BlendState.RenderTarget[ i ].SrcBlend = CRUDE_CAST( D3D12_BLEND, rhi_attachment_state->src_color_blend_factor );
    dx12_pipeline_creation.BlendState.RenderTarget[ i ].DestBlend = CRUDE_CAST( D3D12_BLEND, rhi_attachment_state->dst_color_blend_factor );
    dx12_pipeline_creation.BlendState.RenderTarget[ i ].BlendOp = CRUDE_CAST( D3D12_BLEND_OP, rhi_attachment_state->color_blend_op );
    dx12_pipeline_creation.BlendState.RenderTarget[ i ].SrcBlendAlpha = CRUDE_CAST( D3D12_BLEND, rhi_attachment_state->src_alpha_blend_factor );
    dx12_pipeline_creation.BlendState.RenderTarget[ i ].DestBlendAlpha = CRUDE_CAST( D3D12_BLEND, rhi_attachment_state->dst_alpha_blend_factor );
    dx12_pipeline_creation.BlendState.RenderTarget[ i ].BlendOpAlpha = CRUDE_CAST( D3D12_BLEND_OP, rhi_attachment_state->alpha_blend_op );
    dx12_pipeline_creation.BlendState.RenderTarget[ i ].RenderTargetWriteMask = rhi_attachment_state->color_write_mask;
  }

  dx12_pipeline_creation.RasterizerState = CRUDE_COMPOUNT_EMPTY( D3D12_RASTERIZER_DESC );
  dx12_pipeline_creation.RasterizerState.FillMode = CRUDE_CAST( D3D12_FILL_MODE, creation->rasterization_state->polygon_mode );
  dx12_pipeline_creation.RasterizerState.CullMode = ( creation->rasterization_state->cull_mode & CRUDE_GFX_RHI_CULL_MODE_FRONT_BIT ) ? D3D12_CULL_MODE_FRONT : ( ( creation->rasterization_state->cull_mode & CRUDE_GFX_RHI_CULL_MODE_BACK_BIT ) ? D3D12_CULL_MODE_BACK : D3D12_CULL_MODE_NONE );
  dx12_pipeline_creation.RasterizerState.FrontCounterClockwise = ( creation->rasterization_state->front_face == CRUDE_GFX_RHI_FRONT_FACE_COUNTER_CLOCKWISE );
  dx12_pipeline_creation.RasterizerState.DepthBias = creation->rasterization_state->depth_bias_constant_factor;
  dx12_pipeline_creation.RasterizerState.DepthBiasClamp = creation->rasterization_state->depth_bias_clamp;
  dx12_pipeline_creation.RasterizerState.SlopeScaledDepthBias = creation->rasterization_state->depth_bias_slope_factor;
  dx12_pipeline_creation.RasterizerState.DepthClipEnable = creation->rasterization_state->depth_clamp_enable ? FALSE : TRUE;
  dx12_pipeline_creation.RasterizerState.MultisampleEnable = creation->multisample_state->rasterization_samples > 1 ? TRUE : FALSE;
  dx12_pipeline_creation.RasterizerState.AntialiasedLineEnable = FALSE;
  dx12_pipeline_creation.RasterizerState.ForcedSampleCount = 0;

  dx12_pipeline_creation.DepthStencilState = CRUDE_COMPOUNT_EMPTY( D3D12_DEPTH_STENCIL_DESC );
  dx12_pipeline_creation.DepthStencilState.DepthEnable = creation->depth_stencil_state->depth_test_enable ? TRUE : FALSE;
  dx12_pipeline_creation.DepthStencilState.DepthWriteMask = creation->depth_stencil_state->depth_write_enable ? D3D12_DEPTH_WRITE_MASK_ALL : D3D12_DEPTH_WRITE_MASK_ZERO;
  dx12_pipeline_creation.DepthStencilState.DepthFunc = CRUDE_CAST( D3D12_COMPARISON_FUNC, creation->depth_stencil_state->depth_compare_op );
  dx12_pipeline_creation.DepthStencilState.StencilEnable = creation->depth_stencil_state->stencil_test_enable ? TRUE : FALSE;
  dx12_pipeline_creation.DepthStencilState.FrontFace.StencilFailOp = D3D12_STENCIL_OP_KEEP;
  dx12_pipeline_creation.DepthStencilState.FrontFace.StencilDepthFailOp = D3D12_STENCIL_OP_KEEP;
  dx12_pipeline_creation.DepthStencilState.FrontFace.StencilPassOp = D3D12_STENCIL_OP_KEEP;
  dx12_pipeline_creation.DepthStencilState.FrontFace.StencilFunc = D3D12_COMPARISON_FUNC_ALWAYS;
  dx12_pipeline_creation.DepthStencilState.BackFace = dx12_pipeline_creation.DepthStencilState.FrontFace;

  dx12_pipeline_creation.SampleDesc.Count = creation->multisample_state->rasterization_samples;
  dx12_pipeline_creation.SampleDesc.Quality = 0;

  dx12_pipeline_creation.pRootSignature = creation->pipeline_layout.dx12_root_signature;
  for ( uint32 i = 0; i < creation->stage_count; ++i )
  {
    switch ( creation->stages[ i ].stage )
    {
    case CRUDE_GFX_RHI_SHADER_STAGE_TASK_BIT_EXT:
    {
      dx12_pipeline_creation.AS.pShaderBytecode = creation->stages[ i ].rhi_module.code;
      dx12_pipeline_creation.AS.BytecodeLength = creation->stages[ i ].rhi_module.code_size;
      break;
    }
    case CRUDE_GFX_RHI_SHADER_STAGE_MESH_BIT_EXT:
    {
      dx12_pipeline_creation.MS.pShaderBytecode = creation->stages[ i ].rhi_module.code;
      dx12_pipeline_creation.MS.BytecodeLength = creation->stages[ i ].rhi_module.code_size;
      break;
    }
    case CRUDE_GFX_RHI_SHADER_STAGE_FRAGMENT_BIT:
    {
      dx12_pipeline_creation.PS.pShaderBytecode = creation->stages[ i ].rhi_module.code;
      dx12_pipeline_creation.PS.BytecodeLength = creation->stages[ i ].rhi_module.code_size;
      break;
    }
    default:
    {
      break;
    }
    }
  }
  
  dx12_pipeline_creation.SampleMask = UINT_MAX;
  dx12_pipeline_creation.PrimitiveTopologyType = CRUDE_CAST( D3D12_PRIMITIVE_TOPOLOGY_TYPE, creation->input_assembly_state->topology );
  
  dx12_pipeline_creation.NumRenderTargets = creation->rendering_state->color_attachment_count;
  for ( uint32 i = 0; i < creation->rendering_state->color_attachment_count; ++i )
  {
    dx12_pipeline_creation.RTVFormats[ i ] = CRUDE_CAST( DXGI_FORMAT, creation->rendering_state->color_attachment_formats[ i ] );
  }
  dx12_pipeline_creation.DSVFormat = CRUDE_CAST( DXGI_FORMAT, creation->rendering_state->depth_attachment_format );
  
  dx12_pipeline_mesh_stream = dx12_pipeline_creation;

  stream_description = CRUDE_COMPOUNT_EMPTY( D3D12_PIPELINE_STATE_STREAM_DESC );
  stream_description.SizeInBytes = sizeof( dx12_pipeline_mesh_stream );
  stream_description.pPipelineStateSubobjectStream = &dx12_pipeline_mesh_stream;

  CRUDE_GFX_RHI_HANDLE_DX12_RESULT( device->dx12_device->CreatePipelineState( &stream_description, IID_PPV_ARGS( &pipeline->dx12_pipeline ) ), "Failed to create graphics pipeline" );
}

void
crude_gfx_rhi_create_classic_pipeline
(
  _In_ crude_gfx_rhi_device                               *device,
  _In_ crude_gfx_rhi_classic_pipeline_create_info const   *creation,
  _Out_ crude_gfx_rhi_pipeline                            *pipeline
)
{
  D3D12_GRAPHICS_PIPELINE_STATE_DESC                       dx12_pipeline_creation;
  D3D12_INPUT_ELEMENT_DESC                                 dx12_input_elements[ 10 ];
  uint32                                                   dx12_input_elements_count;

  dx12_input_elements_count = creation->vertex_input_state->vertex_attribute_description_count;
  CRUDE_ASSERT( dx12_input_elements_count < CRUDE_COUNTOF( dx12_input_elements ) );
  CRUDE_ASSERT( creation->vertex_input_state->vertex_attribute_description_count == creation->vertex_input_state->vertex_binding_description_count );
  
  dx12_pipeline_creation = CRUDE_COMPOUNT_EMPTY( D3D12_GRAPHICS_PIPELINE_STATE_DESC );

  for ( uint32 i = 0; i < dx12_input_elements_count; ++i )
  {
    crude_gfx_rhi_pipeline_vertex_input_attribute_description const *rhi_vertex_attribute_description;
    crude_gfx_rhi_vertex_input_binding_description const            *rhi_vertex_input_binding_description;

    rhi_vertex_attribute_description = &creation->vertex_input_state->vertex_attribute_descriptions[ i ];
    rhi_vertex_input_binding_description = &creation->vertex_input_state->vertex_binding_descriptions[ i ];

    dx12_input_elements[ i ] = CRUDE_COMPOUNT_EMPTY( D3D12_INPUT_ELEMENT_DESC );
    dx12_input_elements[ i ].SemanticName = "UNKNOWN";
    dx12_input_elements[ i ].SemanticIndex = i;
    dx12_input_elements[ i ].Format = CRUDE_CAST( DXGI_FORMAT, rhi_vertex_attribute_description->format );
    dx12_input_elements[ i ].InputSlot = rhi_vertex_attribute_description->binding;
    dx12_input_elements[ i ].AlignedByteOffset = rhi_vertex_attribute_description->offset;
    dx12_input_elements[ i ].InputSlotClass = ( rhi_vertex_input_binding_description->input_rate == CRUDE_GFX_RHI_VERTEX_INPUT_RATE_VERTEX ) ? D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA : D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA;
    dx12_input_elements[ i ].InstanceDataStepRate = rhi_vertex_input_binding_description->stride;
  }
  dx12_pipeline_creation.InputLayout.pInputElementDescs = dx12_input_elements;
  dx12_pipeline_creation.InputLayout.NumElements = dx12_input_elements_count;

  dx12_pipeline_creation.BlendState = CRUDE_COMPOUNT_EMPTY( D3D12_BLEND_DESC );
  dx12_pipeline_creation.BlendState.AlphaToCoverageEnable = creation->multisample_state->alpha_to_coverage_enable ? TRUE : FALSE;
  dx12_pipeline_creation.BlendState.IndependentBlendEnable = TRUE;
  for ( uint32 i = 0; i < creation->color_blend_state->attachments_count; ++i )
  {
    crude_gfx_rhi_pipeline_color_blend_attachment_state const       *rhi_attachment_state;

    rhi_attachment_state = &creation->color_blend_state->attachments[ i ];

    dx12_pipeline_creation.BlendState.RenderTarget[ i ] = CRUDE_COMPOUNT_EMPTY( D3D12_RENDER_TARGET_BLEND_DESC );
    dx12_pipeline_creation.BlendState.RenderTarget[ i ].BlendEnable = rhi_attachment_state->blend_enable ? TRUE : FALSE;
    dx12_pipeline_creation.BlendState.RenderTarget[ i ].LogicOpEnable = creation->color_blend_state->logic_op_enable ? TRUE : FALSE;
    dx12_pipeline_creation.BlendState.RenderTarget[ i ].SrcBlend = CRUDE_CAST( D3D12_BLEND, rhi_attachment_state->src_color_blend_factor );
    dx12_pipeline_creation.BlendState.RenderTarget[ i ].DestBlend = CRUDE_CAST( D3D12_BLEND, rhi_attachment_state->dst_color_blend_factor );
    dx12_pipeline_creation.BlendState.RenderTarget[ i ].BlendOp = CRUDE_CAST( D3D12_BLEND_OP, rhi_attachment_state->color_blend_op );
    dx12_pipeline_creation.BlendState.RenderTarget[ i ].SrcBlendAlpha = CRUDE_CAST( D3D12_BLEND, rhi_attachment_state->src_alpha_blend_factor );
    dx12_pipeline_creation.BlendState.RenderTarget[ i ].DestBlendAlpha = CRUDE_CAST( D3D12_BLEND, rhi_attachment_state->dst_alpha_blend_factor );
    dx12_pipeline_creation.BlendState.RenderTarget[ i ].BlendOpAlpha = CRUDE_CAST( D3D12_BLEND_OP, rhi_attachment_state->alpha_blend_op );
    dx12_pipeline_creation.BlendState.RenderTarget[ i ].RenderTargetWriteMask = rhi_attachment_state->color_write_mask;
  }

  dx12_pipeline_creation.RasterizerState = CRUDE_COMPOUNT_EMPTY( D3D12_RASTERIZER_DESC );
  dx12_pipeline_creation.RasterizerState.FillMode = CRUDE_CAST( D3D12_FILL_MODE, creation->rasterization_state->polygon_mode );
  dx12_pipeline_creation.RasterizerState.CullMode = ( creation->rasterization_state->cull_mode & CRUDE_GFX_RHI_CULL_MODE_FRONT_BIT ) ? D3D12_CULL_MODE_FRONT : ( ( creation->rasterization_state->cull_mode & CRUDE_GFX_RHI_CULL_MODE_BACK_BIT ) ? D3D12_CULL_MODE_BACK : D3D12_CULL_MODE_NONE );
  dx12_pipeline_creation.RasterizerState.FrontCounterClockwise = ( creation->rasterization_state->front_face == CRUDE_GFX_RHI_FRONT_FACE_COUNTER_CLOCKWISE );
  dx12_pipeline_creation.RasterizerState.DepthBias = creation->rasterization_state->depth_bias_constant_factor;
  dx12_pipeline_creation.RasterizerState.DepthBiasClamp = creation->rasterization_state->depth_bias_clamp;
  dx12_pipeline_creation.RasterizerState.SlopeScaledDepthBias = creation->rasterization_state->depth_bias_slope_factor;
  dx12_pipeline_creation.RasterizerState.DepthClipEnable = creation->rasterization_state->depth_clamp_enable ? FALSE : TRUE;
  dx12_pipeline_creation.RasterizerState.MultisampleEnable = creation->multisample_state->rasterization_samples > 1 ? TRUE : FALSE;
  dx12_pipeline_creation.RasterizerState.AntialiasedLineEnable = FALSE;
  dx12_pipeline_creation.RasterizerState.ForcedSampleCount = 0;

  dx12_pipeline_creation.DepthStencilState = CRUDE_COMPOUNT_EMPTY( D3D12_DEPTH_STENCIL_DESC );
  dx12_pipeline_creation.DepthStencilState.DepthEnable = creation->depth_stencil_state->depth_test_enable ? TRUE : FALSE;
  dx12_pipeline_creation.DepthStencilState.DepthWriteMask = creation->depth_stencil_state->depth_write_enable ? D3D12_DEPTH_WRITE_MASK_ALL : D3D12_DEPTH_WRITE_MASK_ZERO;
  dx12_pipeline_creation.DepthStencilState.DepthFunc = CRUDE_CAST( D3D12_COMPARISON_FUNC, creation->depth_stencil_state->depth_compare_op );
  dx12_pipeline_creation.DepthStencilState.StencilEnable = creation->depth_stencil_state->stencil_test_enable ? TRUE : FALSE;
  dx12_pipeline_creation.DepthStencilState.FrontFace.StencilFailOp = D3D12_STENCIL_OP_KEEP;
  dx12_pipeline_creation.DepthStencilState.FrontFace.StencilDepthFailOp = D3D12_STENCIL_OP_KEEP;
  dx12_pipeline_creation.DepthStencilState.FrontFace.StencilPassOp = D3D12_STENCIL_OP_KEEP;
  dx12_pipeline_creation.DepthStencilState.FrontFace.StencilFunc = D3D12_COMPARISON_FUNC_ALWAYS;
  dx12_pipeline_creation.DepthStencilState.BackFace = dx12_pipeline_creation.DepthStencilState.FrontFace;

  dx12_pipeline_creation.SampleDesc.Count = creation->multisample_state->rasterization_samples;
  dx12_pipeline_creation.SampleDesc.Quality = 0;

  dx12_pipeline_creation.pRootSignature = creation->pipeline_layout.dx12_root_signature;
  for ( uint32 i = 0; i < creation->stage_count; ++i )
  {
    switch ( creation->stages[ i ].stage )
    {
    case CRUDE_GFX_RHI_SHADER_STAGE_VERTEX_BIT:
    {
      dx12_pipeline_creation.VS.pShaderBytecode = creation->stages[ i ].rhi_module.code;
      dx12_pipeline_creation.VS.BytecodeLength = creation->stages[ i ].rhi_module.code_size;
      break;
    }
    case CRUDE_GFX_RHI_SHADER_STAGE_FRAGMENT_BIT:
    {
      dx12_pipeline_creation.PS.pShaderBytecode = creation->stages[ i ].rhi_module.code;
      dx12_pipeline_creation.PS.BytecodeLength = creation->stages[ i ].rhi_module.code_size;
      break;
    }
    default:
    {
      break;
    }
    }
  }
  
  dx12_pipeline_creation.SampleMask = UINT_MAX;
  dx12_pipeline_creation.PrimitiveTopologyType = CRUDE_CAST( D3D12_PRIMITIVE_TOPOLOGY_TYPE, creation->input_assembly_state->topology );
  
  dx12_pipeline_creation.NumRenderTargets = creation->rendering_state->color_attachment_count;
  for ( uint32 i = 0; i < creation->rendering_state->color_attachment_count; ++i )
  {
    dx12_pipeline_creation.RTVFormats[ i ] = CRUDE_CAST( DXGI_FORMAT, creation->rendering_state->color_attachment_formats[ i ] );
  }
  dx12_pipeline_creation.DSVFormat = CRUDE_CAST( DXGI_FORMAT, creation->rendering_state->depth_attachment_format );
  
  CRUDE_GFX_RHI_HANDLE_DX12_RESULT( device->dx12_device->CreateGraphicsPipelineState( &dx12_pipeline_creation, IID_PPV_ARGS( &pipeline->dx12_pipeline ) ), "Failed to create graphics pipeline" );
}

void
crude_gfx_rhi_create_compute_pipeline
(
  _In_ crude_gfx_rhi_device                               *device,
  _In_ crude_gfx_rhi_compute_pipeline_create_info const   *creation,
  _Out_ crude_gfx_rhi_pipeline                            *pipeline
)
{
}

void
crude_gfx_rhi_create_ray_tracing_pipeline
(
  _In_ crude_gfx_rhi_device                               *device,
  _In_ crude_gfx_rhi_ray_tracing_pipeline_create_info const *creation,
  _Out_ crude_gfx_rhi_pipeline                            *pipeline
)
{
}

void
crude_gfx_rhi_destroy_pipeline
(
  _In_ crude_gfx_rhi_device                               *device,
  _In_ crude_gfx_rhi_pipeline                              pipeline
)
{
  pipeline.dx12_pipeline->Release( );
}

void
crude_gfx_rhi_get_ray_tracing_shader_group_handles
(
  _In_ crude_gfx_rhi_device                               *device,
  _In_ crude_gfx_rhi_pipeline                              pipeline,
  _In_ uint32                                              first_group,
  _In_ uint32                                              group_count,
  _In_ uint32                                              data_size,
  _Out_ void                                              *data
)
{
}

void
crude_gfx_rhi_set_pipeline_debug_name
(
  _In_ crude_gfx_rhi_device                               *device,
  _In_ crude_gfx_rhi_pipeline                              pipeline,
  _In_ char const                                         *name
)
{
}

void
crude_gfx_rhi_create_swapchain
(
  _In_ crude_gfx_rhi_device                               *device,
  _In_ crude_gfx_rhi_swapchain_create_info const          *creation,
  _In_ crude_heap_allocator                               *allocator,
  _Out_ crude_gfx_rhi_swapchain                           *swapchain,
  _Out_ uint32                                            *swapchain_images_count,
  _Out_ XMFLOAT2                                          *swapchain_extent,
  _Out_ crude_gfx_rhi_image                                swapchain_images[ CRUDE_GFX_SWAPCHAIN_IMAGES_MAX_COUNT ]
)
{
  DXGI_SWAP_CHAIN_DESC1                                    dxgi_swapchain_creation;
  HWND                                                     hwnd;
  SDL_PropertiesID                                         sdl_window_properties;
  int32                                                    window_width, window_height;
  
  *swapchain_images_count = 3;

  SDL_GetWindowSize( creation->surface.sdl_window, &window_width, &window_height );
  
  swapchain_extent->x = window_width;
  swapchain_extent->y = window_height;

  dxgi_swapchain_creation = CRUDE_COMPOUNT_EMPTY( DXGI_SWAP_CHAIN_DESC1 );
  dxgi_swapchain_creation.Width = window_width;
  dxgi_swapchain_creation.Height = window_height;
  dxgi_swapchain_creation.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
  dxgi_swapchain_creation.SampleDesc.Count = 1;
  dxgi_swapchain_creation.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
  dxgi_swapchain_creation.BufferCount = *swapchain_images_count;
  dxgi_swapchain_creation.Scaling = DXGI_SCALING_STRETCH;
  dxgi_swapchain_creation.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
  dxgi_swapchain_creation.Flags = 0;
  
  sdl_window_properties = SDL_GetWindowProperties( creation->surface.sdl_window );
  hwnd = CRUDE_CAST( HWND, SDL_GetPointerProperty( sdl_window_properties, SDL_PROP_WINDOW_WIN32_HWND_POINTER, NULL ) );

  device->dxgi_factory->CreateSwapChainForHwnd( device->main_queue.dx12_queue, hwnd, &dxgi_swapchain_creation, NULL, NULL, &swapchain->dxgi_swapchain );
  
  CRUDE_GFX_RHI_HANDLE_DX12_RESULT( swapchain->dxgi_swapchain->GetBuffer( 0, IID_PPV_ARGS( &swapchain_images[ 0 ].dx12_resource ) ), "Faied to acquire swapchain image 0" );
  CRUDE_GFX_RHI_HANDLE_DX12_RESULT( swapchain->dxgi_swapchain->GetBuffer( 1, IID_PPV_ARGS( &swapchain_images[ 1 ].dx12_resource ) ), "Faied to acquire swapchain image 1" );
  CRUDE_GFX_RHI_HANDLE_DX12_RESULT( swapchain->dxgi_swapchain->GetBuffer( 2, IID_PPV_ARGS( &swapchain_images[ 2 ].dx12_resource ) ), "Faied to acquire swapchain image 2" );
}

void
crude_gfx_rhi_destroy_swapchain
(
  _In_ crude_gfx_rhi_device                               *device,
  _In_ crude_gfx_rhi_swapchain                             swapchain
)
{
  CRUDE_ASSERT( false && "We need to release swapchain_images" );
  swapchain.dxgi_swapchain->Release( );
}

void
crude_gfx_rhi_create_descriptor_set_layout
(
  _In_ crude_gfx_rhi_device                               *device,
  _In_ crude_gfx_rhi_descriptor_set_layout_create_info const *creation,
  _Out_ crude_gfx_rhi_descriptor_set_layout               *layout
)
{
}

void
crude_gfx_rhi_destroy_descriptor_set_layout
(
  _In_ crude_gfx_rhi_device                               *device,
  _In_ crude_gfx_rhi_descriptor_set_layout                 layout
)
{
}

void
crude_gfx_rhi_create_descriptor_set
(
  _In_ crude_gfx_rhi_device                               *device,
  _In_ crude_gfx_rhi_descriptor_set_create_info const     *creation,
  _Out_ crude_gfx_rhi_descriptor_set                      *descriptor_set
)
{
}

void
crude_gfx_rhi_set_descriptor_set_debug_name
(
  _In_ crude_gfx_rhi_device                               *device,
  _In_ crude_gfx_rhi_descriptor_set                        descriptor_set,
  _In_ char const                                         *name
)
{
}

void
crude_gfx_rhi_create_descriptor_pool
(
  _In_ crude_gfx_rhi_device                               *device,
  _In_ bool                                                bindless,
  _Out_ crude_gfx_rhi_descriptor_pool                     *descriptor_pool
)
{
}

void
crude_gfx_rhi_set_descriptor_pool_debug_name
(
  _In_ crude_gfx_rhi_device                               *device,
  _In_ crude_gfx_rhi_descriptor_pool                       descriptor_pool,
  _In_ char const                                         *name
)
{
}

void
crude_gfx_rhi_create_acceleration_structure
(
  _In_ crude_gfx_rhi_device                               *device,
  _In_ crude_gfx_rhi_acceleration_structure_create_info const *creation,
  _Out_ crude_gfx_rhi_acceleration_structure              *acceleration_structure
)
{
}

void
crude_gfx_rhi_destroy_acceleration_structure
(
  _In_ crude_gfx_rhi_device                               *device,
  _In_ crude_gfx_rhi_acceleration_structure                acceleration_structure
)
{
}

void
crude_gfx_rhi_create_command_pool
(
  _In_ crude_gfx_rhi_device                               *device,
  _In_ crude_gfx_rhi_command_pool_create_info const       *creation,
  _Out_ crude_gfx_rhi_command_pool                        *command_pool
)
{
  CRUDE_GFX_RHI_HANDLE_DX12_RESULT( device->dx12_device->CreateCommandAllocator( D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS( &command_pool->dx12_command_allocator ) ), "Failed create comand allocator" );
}

void
crude_gfx_rhi_destroy_command_pool
(
  _In_ crude_gfx_rhi_device                               *device,
  _In_ crude_gfx_rhi_command_pool                          command_pool
)
{
  command_pool.dx12_command_allocator->Release( );
}

void
crude_gfx_rhi_create_query_pool
(
  _In_ crude_gfx_rhi_device                               *device,
  _In_ crude_gfx_rhi_queru_pool_create_info const         *creation,
  _Out_ crude_gfx_rhi_query_pool                          *query_pool
)
{
}

void
crude_gfx_rhi_destroy_query_pool
(
  _In_ crude_gfx_rhi_device                               *device,
  _In_ crude_gfx_rhi_query_pool                            query_pool
)
{
}

void
crude_gfx_rhi_create_command_buffer
(
  _In_ crude_gfx_rhi_device                               *device,
  _In_ crude_gfx_rhi_command_buffer_create_info const     *creation,
  _Out_ crude_gfx_rhi_command_buffer                      *command_buffer
)
{
  CRUDE_GFX_RHI_HANDLE_DX12_RESULT( device->dx12_device->CreateCommandList( 0, D3D12_COMMAND_LIST_TYPE_DIRECT, creation->command_pool.dx12_command_allocator, NULL, IID_PPV_ARGS( &command_buffer->dx12_command_list ) ), "Failed to create commmand list" );
  command_buffer->dx12_command_list->Close( );
}

void
crude_gfx_rhi_destroy_command_buffer
(
  _In_ crude_gfx_rhi_device                               *device,
  _In_ crude_gfx_rhi_command_buffer                        command_buffer
)
{
  command_buffer.dx12_command_list->Release( );
}

void
crude_gfx_rhi_set_command_buffer_debug_name
(
  _In_ crude_gfx_rhi_device                               *device,
  _In_ crude_gfx_rhi_command_buffer                        command_buffer,
  _In_ char const                                         *name
)
{
  WCHAR                                                    wname[ 1024 ];
  mbstowcs( wname, name, sizeof( wname ) - 1 );
  command_buffer.dx12_command_list->SetName( wname );
}

crude_gfx_rhi_queue
crude_gfx_rhi_device_get_graphics_queue
(
  _In_ crude_gfx_rhi_device                               *device
)
{
  return crude_gfx_rhi_queue_empty( );
}

crude_gfx_rhi_queue
crude_gfx_rhi_device_get_transfer_queue
(
  _In_ crude_gfx_rhi_device                               *device
)
{
  return crude_gfx_rhi_queue_empty( );
}

void
crude_gfx_rhi_update_descriptor_set
(
  _In_ crude_gfx_rhi_device                               *device,
  _In_ crude_gfx_rhi_descriptor_set                        descriptor_set,
  _In_ crude_gfx_rhi_write_descriptor_set                 *write_descripor_sets,
  _In_ uint32                                              write_descripor_sets_count
)
{
}

crude_gfx_rhi_physical_device_optional_extensions const*
crude_gfx_rhi_get_device_optional_extensions
(
  _In_ crude_gfx_rhi_device                               *device
)
{
  return &device->optional_extensions;
}

void*
crude_gfx_rhi_get_buffer_mapped_data
(
  _In_ crude_gfx_rhi_buffer                                buffer
)
{
  return NULL;
}

void
crude_gfx_rhi_wait_semaphore
(
  _In_ crude_gfx_rhi_device                               *device,
  _In_ crude_gfx_rhi_semaphore                             semaphore,
  _In_ uint64                                              value
)
{
}

XMFLOAT2
crude_gfx_rhi_get_surface_extent
(
  _In_ crude_gfx_rhi_device                               *device,
  _In_ crude_gfx_rhi_surface                               surface
)
{
  return CRUDE_COMPOUNT_EMPTY( XMFLOAT2 );
}

float32
crude_gfx_rhi_get_timestamp_period
(
  _In_ crude_gfx_rhi_device                               *device
)
{
  return 0;
}

bool
crude_gfx_rhi_acquire_next_image
(
  _In_ crude_gfx_rhi_device                               *device,
  _In_ crude_gfx_rhi_swapchain                             swapchain,
  _In_ uint64                                              timeout,
  _In_ crude_gfx_rhi_semaphore                             semaphore,
  _Out_ uint32                                            *image_index
)
{
  return true;
}

void
crude_gfx_rhi_wait_idle
(
  _In_ crude_gfx_rhi_device                               *device
)
{
}

void
crude_gfx_rhi_create_semaphore
(
  _In_ crude_gfx_rhi_device                               *device,
  _In_ bool                                                timeline,
  _Out_ crude_gfx_rhi_semaphore                           *semaphore
)
{
}

void
crude_gfx_rhi_destroy_semaphore
(
  _In_ crude_gfx_rhi_device                               *device,
  _In_ crude_gfx_rhi_semaphore                             semaphore
)
{
}

void
crude_gfx_rhi_set_semaphore_debug_name
(
  _In_ crude_gfx_rhi_device                               *device,
  _In_ crude_gfx_rhi_semaphore                             semaphore,
  _In_ char const                                         *name
)
{
}

void
crude_gfx_rhi_create_fence
(
  _In_ crude_gfx_rhi_device                               *device,
  _In_ bool                                                signaled,
  _Out_ crude_gfx_rhi_fence                               *fence
)
{
}

void
crude_gfx_rhi_destroy_fence
(
  _In_ crude_gfx_rhi_device                               *device,
  _In_ crude_gfx_rhi_fence                                 fence
)
{
}

void
crude_gfx_rhi_reset_fence
(
  _In_ crude_gfx_rhi_device                               *device,
  _In_ crude_gfx_rhi_fence                                *fence
)
{
}

void
crude_gfx_rhi_set_fence_debug_name
(
  _In_ crude_gfx_rhi_device                               *device,
  _In_ crude_gfx_rhi_fence                                 fence,
  _In_ char const                                         *name
)
{
}

void
crude_gfx_rhi_set_debug_utils_object_name
(
  _In_ crude_gfx_rhi_device                               *device,
  _In_ crude_gfx_rhi_object_type                           object_type,
  _In_ uint64                                              object_handle,
  _In_ char const                                         *object_name
)
{
}

void
crude_gfx_rhi_destroy_descriptor_pool
(
  _In_ crude_gfx_rhi_device                               *device,
  _Out_ crude_gfx_rhi_descriptor_pool                     *descriptor_pool
)
{
}

void
crude_gfx_rhi_get_device_memory_budget
(
  _In_ crude_gfx_rhi_device                               *device,
  _Out_ crude_gfx_rhi_device_memory_budget                *budget
)
{
}

void
crude_gfx_rhi_get_device_ray_tracing_pipeline_properties
(
  _In_ crude_gfx_rhi_device                               *device,
  _Out_ crude_gfx_rhi_device_ray_tracing_pipeline_properties *ray_tracing_properties
)
{
}

void
crude_gfx_rhi_get_device_name
(
  _In_ crude_gfx_rhi_device                               *device,
  _Out_ char                                               name[ 256 ]
)
{
}

void
crude_gfx_rhi_get_acceleration_structure_build_sizes
(
  _In_ crude_gfx_rhi_device                               *device,
  _In_ crude_heap_allocator                               *allocator,
  _In_ crude_gfx_rhi_acceleration_structure_build_type     build_type,
  _In_ crude_gfx_rhi_acceleration_structure_build_geometry_info const *build_info,
  _In_ uint32 const                                       *max_primitives_count,
  _Out_ crude_gfx_rhi_acceleration_structure_build_sizes_info *build_size_info
)
{
}

void
crude_gfx_rhi_reset_command_pool
(
  _In_ crude_gfx_rhi_device                               *device,
  _In_ crude_gfx_rhi_command_pool                          command_pool
)
{
}

void
crude_gfx_rhi_begin_command_buffer
(
  _In_ crude_gfx_rhi_command_pool                          command_pool,
  _In_ crude_gfx_rhi_command_buffer                        command_buffer,
  _In_ crude_gfx_rhi_command_buffer_begin_info const      *begin_info
)
{
  CRUDE_GFX_RHI_HANDLE_DX12_RESULT( command_buffer.dx12_command_list->Reset( command_pool.dx12_command_allocator, NULL ), "Failed restecommand list" );
}

void
crude_gfx_rhi_end_command_buffer
(
  _In_ crude_gfx_rhi_command_buffer                        command_buffer
)
{
  CRUDE_GFX_RHI_HANDLE_DX12_RESULT( command_buffer.dx12_command_list->Close( ), "Failed close command list" );
}

void
crude_gfx_rhi_command_buffer_begin_rendering
(
  _In_ crude_gfx_rhi_command_buffer                        command_buffer,
  _In_ crude_gfx_rhi_rendering_info const                 *rendering_info
)
{
}

void
crude_gfx_rhi_command_buffer_end_rendering
(
  _In_ crude_gfx_rhi_command_buffer                        command_buffer
)
{
}

void
crude_gfx_rhi_command_buffer_bind_pipeline
(
  _In_ crude_gfx_rhi_command_buffer                        command_buffer,
  _In_ crude_gfx_rhi_pipeline                              rhi_pipeline,
  _In_ crude_gfx_rhi_pipeline_bind_point                   rhi_pipeline_bind_point
)
{
  //command_buffer.dx12_command_list->SetGraphicsRootSignature( );
}

void
crude_gfx_rhi_command_buffer_set_viewport
(
  _In_ crude_gfx_rhi_command_buffer                        command_buffer,
  _In_ crude_gfx_rhi_viewport const                       *viewport
)
{
}

void
crude_gfx_rhi_command_buffer_set_scissor
(
  _In_ crude_gfx_rhi_command_buffer                        command_buffer,
  _In_ crude_gfx_rhi_scissor const                        *scissor
)
{
}

void
crude_gfx_rhi_command_buffer_draw
(
  _In_ crude_gfx_rhi_command_buffer                        command_buffer,
  _In_ uint32                                              vertex_count,
  _In_ uint32                                              instance_count,
  _In_ uint32                                              first_vertex,
  _In_ uint32                                              first_instance
)
{
}

void
crude_gfx_rhi_command_buffer_draw_indirect
(
  _In_ crude_gfx_rhi_command_buffer                        command_buffer,
  _In_ crude_gfx_rhi_buffer                                buffer,
  _In_ crude_gfx_rhi_device_size                           offset,
  _In_ uint32                                              draw_count,
  _In_ uint32                                              stride
)
{
}

void
crude_gfx_rhi_command_buffer_draw_indirect_count
(
  _In_ crude_gfx_rhi_command_buffer                        command_buffer,
  _In_ crude_gfx_rhi_buffer                                argument_buffer,
  _In_ crude_gfx_rhi_device_size                           argument_buffer_offset,
  _In_ crude_gfx_rhi_buffer                                count_buffer,
  _In_ crude_gfx_rhi_device_size                           count_buffer_offset,
  _In_ uint32                                              max_draw_count,
  _In_ uint32                                              stride
)
{
}

void
crude_gfx_rhi_command_buffer_draw_mesh_task
(
  _In_ crude_gfx_rhi_device                               *device,
  _In_ crude_gfx_rhi_command_buffer                        command_buffer,
  _In_ uint32                                              group_count_x,
  _In_ uint32                                              group_count_y,
  _In_ uint32                                              group_count_z
)
{
}

void
crude_gfx_rhi_command_buffer_draw_mesh_task_indirect_count
(
  _In_ crude_gfx_rhi_device                               *device,
  _In_ crude_gfx_rhi_command_buffer                        command_buffer,
  _In_ crude_gfx_rhi_buffer                                argument_buffer,
  _In_ crude_gfx_rhi_device_size                           argument_buffer_offset,
  _In_ crude_gfx_rhi_buffer                                count_buffer,
  _In_ crude_gfx_rhi_device_size                           count_buffer_offset,
  _In_ uint32                                              max_draw_count,
  _In_ uint32                                              stride
)
{
}

void
crude_gfx_rhi_command_buffer_dispatch
(
  _In_ crude_gfx_rhi_command_buffer                        command_buffer,
  _In_ uint32                                              group_count_x,
  _In_ uint32                                              group_count_y,
  _In_ uint32                                              group_count_z
)
{
}

void
crude_gfx_rhi_command_buffer_bind_descriptor_sets
(
  _In_ crude_gfx_rhi_command_buffer                        command_buffer,
  _In_ crude_gfx_rhi_pipeline_bind_point                   pipeline_bind_point,
  _In_ crude_gfx_rhi_pipeline_layout                       pipeline_layout,
  _In_ uint32                                              set,
  _In_ crude_gfx_rhi_descriptor_set                        descriptor_set
)
{
}

void
crude_gfx_rhi_command_buffer_pipeline_image_barrier
(
  _In_ crude_gfx_rhi_command_buffer                        command_buffer,
  _In_ crude_gfx_rhi_image_memory_barrier const           *image_memory_barriers
)
{
}

void
crude_gfx_rhi_command_buffer_pipeline_buffer_barrier
(
  _In_ crude_gfx_rhi_command_buffer                        command_buffer,
  _In_ crude_gfx_rhi_buffer_memory_barrier                *buffer_memory_barriers
)
{
}

void
crude_gfx_rhi_command_buffer_pipeline_global_barrier
(
  _In_ crude_gfx_rhi_command_buffer                        command_buffer
)
{
}

void
crude_gfx_rhi_command_buffer_copy_buffer_to_image
(
  _In_ crude_gfx_rhi_command_buffer                        command_buffer,
  _In_ crude_gfx_rhi_buffer                                buffer,
  _In_ crude_gfx_rhi_image                                 image,
  _In_ crude_gfx_rhi_buffer_image_copy const              *region
)
{
}

void
crude_gfx_rhi_command_buffer_copy_buffer
(
  _In_ crude_gfx_rhi_command_buffer                        command_buffer,
  _In_ crude_gfx_rhi_buffer                                src_buffer,
  _In_ crude_gfx_rhi_buffer                                dst_buffer,
  _In_ crude_gfx_rhi_buffer_copy const                    *region
)
{
}

void
crude_gfx_rhi_command_buffer_write_timestamp
(
  _In_ crude_gfx_rhi_command_buffer                        command_buffer,
  _In_ crude_gfx_rhi_pipeline_stage_flags                  pipeline_stage,
  _In_ crude_gfx_rhi_query_pool                            query_pool,
  _In_ uint32                                              query
)
{
}

void
crude_gfx_rhi_command_buffer_begin_debug_utils_label
(
  _In_ crude_gfx_rhi_device                               *device,
  _In_ crude_gfx_rhi_command_buffer                        command_buffer,
  _In_ crude_gfx_rhi_debug_utils_label const              *debug_utils_label
)
{
}

void
crude_gfx_rhi_command_buffer_end_debug_utils_label
(
  _In_ crude_gfx_rhi_device                               *device,
  _In_ crude_gfx_rhi_command_buffer                        command_buffer
)
{
}

void
crude_gfx_rhi_command_buffer_push_constant
(
  _In_ crude_gfx_rhi_command_buffer                        command_buffer,
  _In_ crude_gfx_rhi_pipeline_layout                       layout,
  _In_ crude_gfx_rhi_shader_stage_flags                    stage_flags,
  _In_ uint32                                              offset,
  _In_ uint32                                              size,
  _In_ void const                                         *values
)
{
}

void
crude_gfx_rhi_command_buffer_fill_buffer
(
  _In_ crude_gfx_rhi_command_buffer                        command_buffer,
  _In_ crude_gfx_rhi_buffer                                dst_buffer,
  _In_ crude_gfx_rhi_device_size                           dst_offset,
  _In_ crude_gfx_rhi_device_size                           size,
  _In_ uint32                                              data
)
{
}

void
crude_gfx_rhi_command_buffer_trace_rays
(
  _In_ crude_gfx_rhi_device                               *device,
  _In_ crude_gfx_rhi_command_buffer                        command_buffer,
  _In_ crude_gfx_rhi_strided_device_address_region const  *raygen_shader_binding_table,
  _In_ crude_gfx_rhi_strided_device_address_region const  *miss_shader_binding_table,
  _In_ crude_gfx_rhi_strided_device_address_region const  *hit_shader_binding_table,
  _In_ crude_gfx_rhi_strided_device_address_region const  *callable_shader_binding_table,
  _In_ uint32                                              width,
  _In_ uint32                                              height,
  _In_ uint32                                              depth
)
{
}

void
crude_gfx_rhi_command_buffer_begin_query
(
  _In_ crude_gfx_rhi_command_buffer                        command_buffer,
  _In_ crude_gfx_rhi_query_pool                            query_pool,
  _In_ uint32                                              query,
  _In_ crude_gfx_rhi_query_control_flags                   flags
)
{
}

void
crude_gfx_rhi_command_buffer_end_query
(
  _In_ crude_gfx_rhi_command_buffer                        command_buffer,
  _In_ crude_gfx_rhi_query_pool                            query_pool,
  _In_ uint32                                              query
)
{
}

void
crude_gfx_rhi_command_buffer_reset_query_pool
(
  _In_ crude_gfx_rhi_command_buffer                        command_buffer,
  _In_ crude_gfx_rhi_query_pool                            query_pool,
  _In_ uint32                                              first_query,
  _In_ uint32                                              query_count
)
{
}

void
crude_gfx_rhi_command_buffer_copy_image
(
  _In_ crude_gfx_rhi_command_buffer                        command_buffer,
  _In_ crude_gfx_rhi_image                                 src_image,
  _In_ crude_gfx_rhi_image_layout                          src_image_layout,
  _In_ crude_gfx_rhi_image                                 dst_image,
  _In_ crude_gfx_rhi_image_layout                          dst_image_layout,
  _In_ crude_gfx_rhi_image_copy const                     *region
)
{
}

void
crude_gfx_rhi_command_buffer_blit_image
(
  _In_ crude_gfx_rhi_command_buffer                        command_buffer,
  _In_ crude_gfx_rhi_image                                 src_image,
  _In_ crude_gfx_rhi_image_layout                          src_image_layout,
  _In_ crude_gfx_rhi_image                                 dst_image,
  _In_ crude_gfx_rhi_image_layout                          dst_image_layout,
  _In_ crude_gfx_rhi_image_blit const                     *region,
  _In_ crude_gfx_rhi_filter                                filter
)
{
}

void
crude_gfx_rhi_reset_command_buffer
(
  _In_ crude_gfx_rhi_command_buffer                        command_buffer
)
{
}

void
crude_gfx_rhi_command_buffer_build_acceleration_structures
(
  _In_ crude_gfx_rhi_device                                                   *device,
  _In_ crude_heap_allocator                                                   *allocator,
  _In_ crude_gfx_rhi_command_buffer                                            command_buffer,
  _In_ uint32                                                                  info_count,
  _In_ crude_gfx_rhi_acceleration_structure_build_geometry_info const         *infos,
  _In_ crude_gfx_rhi_acceleration_structure_build_range_info const            *build_range_infos
)
{
}

char const*
crude_gfx_rhi_current_graphics_api_str
(
)
{
  return "DX12";
}

bool
crude_gfx_rhi_dx12_pick_physical_device_
(
  _In_ crude_heap_allocator                               *allocator,
  _In_ IDXGIFactory6                                      *dxgi_factory,
  _Out_ ID3D12Device2                                    **dx12_device,
  _Out_ IDXGIAdapter4                                    **dx12_adapter,
  _Out_ uint32                                            *dx12_device_index,
  _Out_ crude_gfx_rhi_physical_device_optional_extensions *selected_physical_devices_optional_extenstions
)
{
  typedef struct dxgi_adapter_packed
  {
    IDXGIAdapter4                                         *dxgi_adapter;
    uint32                                                 index;
  } dxgi_adapter_packed;

  dxgi_adapter_packed                                     *dxgi_adapters_packed;
  ID3D12Device                                            *dx12_selected_device;
  char                                                     addapter_name[ 128 ];
  DXGI_ADAPTER_DESC3                                       dxgi_adapter_description;

  *dx12_device = NULL;
  *dx12_adapter = NULL;
  
  CRUDE_ARRAY_INITIALIZE_WITH_CAPACITY( dxgi_adapters_packed, 16, crude_heap_allocator_pack( allocator ) );

  for ( uint32 i = 0; i < 1000; ++i)
  {
    IDXGIAdapter1                                         *dxgi_picked_adapter1;
    IDXGIAdapter4                                         *dxgi_picked_adapter4;
    dxgi_adapter_packed                                    dxgi_picked_adapter_packed;

    dxgi_picked_adapter1 = NULL;
    if ( FAILED( dxgi_factory->EnumAdapters1( i, &dxgi_picked_adapter1 ) ) )
    {
      if ( dxgi_picked_adapter1 )
      {
        dxgi_picked_adapter1->Release();
      }
      break;
    }

    if ( FAILED( dxgi_picked_adapter1->QueryInterface( IID_PPV_ARGS( &dxgi_picked_adapter4 ) ) ) )
    {
      if ( dxgi_picked_adapter4 )
      {
        dxgi_picked_adapter4->Release( );
      }
      dxgi_picked_adapter1->Release( );
      continue;
    }

    dxgi_picked_adapter1->Release( );

    dxgi_picked_adapter_packed.dxgi_adapter = dxgi_picked_adapter4;
    dxgi_picked_adapter_packed.index = i;

    CRUDE_ARRAY_PUSH( dxgi_adapters_packed, dxgi_picked_adapter_packed );
  }

  if ( CRUDE_ARRAY_LENGTH( dxgi_adapters_packed ) == 0 )
  {
    goto cleanup_failed;
  }

  dx12_selected_device = NULL;

  for ( uint32 try_picking = 0; try_picking < 2; ++try_picking )
  {
    bool looking_for_discrete_gpu = ( try_picking == 0 );
    bool looking_for_any_gpu = ( try_picking == 1 );

    for ( uint32 i = 0; i < CRUDE_ARRAY_LENGTH( dxgi_adapters_packed ); ++i )
    {
      IDXGIAdapter4                                       *dxgi_picked_adapter4;
      ID3D12Device2                                       *dx12_test_device;
      D3D12_FEATURE_DATA_SHADER_MODEL                      dx12_shader_module;
      D3D12_FEATURE_DATA_D3D12_OPTIONS7                    dx12_feature_data_options7;
      D3D12_FEATURE_DATA_D3D12_OPTIONS6                    dx12_feature_data_options6;
      DXGI_ADAPTER_DESC3                                   dxgi_description;
      UINT                                                 dx12_output_count;

      dxgi_picked_adapter4 = dxgi_adapters_packed[ i ].dxgi_adapter;

      dxgi_description = CRUDE_COMPOUNT_EMPTY( DXGI_ADAPTER_DESC3 );

      if ( FAILED( dxgi_picked_adapter4->GetDesc3( &dxgi_description ) ) )
      {
        continue;
      }

      if ( dxgi_description.Flags & DXGI_ADAPTER_FLAG3_SOFTWARE )
      {
        continue;
      }
      
      if ( looking_for_discrete_gpu )
      {
        if ( dxgi_description.DedicatedVideoMemory == 0 )
        {
          continue;
        }
      }

      dx12_output_count = 0u;

      for ( UINT output_index = 0; ; ++output_index )
      {
        IDXGIOutput                                       *dxgi_output;

        dxgi_output = NULL;
        if ( FAILED( dxgi_picked_adapter4->EnumOutputs( output_index, &dxgi_output ) ) )
        {
          break;
        }

        ++dx12_output_count;
        dxgi_output->Release( );
      }

      if ( dx12_output_count == 0 )
      {
        continue;
      }

      dx12_test_device = NULL;
      if ( FAILED( D3D12CreateDevice( dxgi_picked_adapter4, D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS( &dx12_test_device ) ) ) )
      {
        if ( dx12_test_device )
        {
          dx12_test_device->Release( );
        }
        continue;
      }
      
      dx12_feature_data_options7 = CRUDE_COMPOUNT_EMPTY( D3D12_FEATURE_DATA_D3D12_OPTIONS7 );
      dx12_feature_data_options6 = CRUDE_COMPOUNT_EMPTY( D3D12_FEATURE_DATA_D3D12_OPTIONS6 );
      
      selected_physical_devices_optional_extenstions->deferred_host_operations_extension_present = true;
      selected_physical_devices_optional_extenstions->shader_relaxed_extended_instruction_extension_present = true;
      selected_physical_devices_optional_extenstions->mesh_shaders_extension_present = false;
      
      if ( SUCCEEDED( dx12_test_device->CheckFeatureSupport( D3D12_FEATURE_D3D12_OPTIONS7, &dx12_feature_data_options7, sizeof( dx12_feature_data_options7 ) ) ) )
      {
        selected_physical_devices_optional_extenstions->mesh_shaders_extension_present = ( dx12_feature_data_options7.MeshShaderTier != D3D12_MESH_SHADER_TIER_NOT_SUPPORTED );
      }
            
      selected_physical_devices_optional_extenstions->fragment_shading_rate_extension_present = false;
      
      if ( SUCCEEDED( dx12_test_device->CheckFeatureSupport( D3D12_FEATURE_D3D12_OPTIONS5, &dx12_feature_data_options6, sizeof( dx12_feature_data_options6 ) ) ) )
      {
        selected_physical_devices_optional_extenstions->fragment_shading_rate_extension_present = ( dx12_feature_data_options6.VariableShadingRateTier != D3D12_VARIABLE_SHADING_RATE_TIER_NOT_SUPPORTED );
      }
      
      dx12_shader_module = CRUDE_COMPOUNT_EMPTY( D3D12_FEATURE_DATA_SHADER_MODEL );
      dx12_shader_module.HighestShaderModel = D3D_SHADER_MODEL_6_0;

      //if (  SUCCEEDED( dx12_test_device->CheckFeatureSupport( D3D12_FEATURE_SHADER_MODEL, &dx12_shader_module, sizeof( dx12_shader_module ) ) ) )
      //{
      //  shaderModel6x = ( dx12_shader_module.HighestShaderModel >= D3D_SHADER_MODEL_6_0 );
      //}

      *dx12_adapter = dxgi_picked_adapter4;
      *dx12_device = dx12_test_device;
      *dx12_device_index = dxgi_adapters_packed[ i ].index;

      try_picking = 2;
      break;
    }
  }
  
  for ( uint32 i = 0; i < CRUDE_ARRAY_LENGTH( dxgi_adapters_packed ); ++i )
  {
    if ( dxgi_adapters_packed[ i ].dxgi_adapter != *dx12_adapter )
    {
      dxgi_adapters_packed[ i ].dxgi_adapter->Release();
    }
  }

  if ( *dx12_adapter == NULL || *dx12_device == NULL )
  {
    goto cleanup_failed;
  }

  dxgi_adapter_description = CRUDE_COMPOUNT_EMPTY( DXGI_ADAPTER_DESC3 );
  ( *dx12_adapter )->GetDesc3( &dxgi_adapter_description );
  
  wcstombs( addapter_name, dxgi_adapter_description.Description, sizeof( dxgi_adapter_description.Description ) - 1 );
  CRUDE_LOG_INFO( CRUDE_CHANNEL_GRAPHICS, "Selected adapter %ls, DedicatedVideoMemory=%llu", addapter_name, dxgi_adapter_description.DedicatedVideoMemory );
  
  CRUDE_ARRAY_DEINITIALIZE( dxgi_adapters_packed, 16, crude_heap_allocator_pack( allocator ) );
  return true;
  
cleanup_failed:
  CRUDE_ARRAY_DEINITIALIZE( dxgi_adapters_packed, 16, crude_heap_allocator_pack( allocator ) );
  return false;
}

#else
CRUDE_GFX_RHI_TO_IMPLEMENTIT
#endif

void
crude_gfx_rhi_compile_shader_glsl_to_spirv
(
  _In_ crude_gfx_rhi_compile_glsl_to_spirv_description const *desc,
  _In_ crude_heap_allocator                               *allocator,
  _Out_ char                                             **spirv_absolute_filepath
)
{
  char const                                              *glsl_absolute_filepath;
  char const                                              *glsl_compiler_absolute_filepath;
  char const                                              *glsl_compiler_arguments;
  char const                                              *glsl_compiler_optional_arguments;
  char const                                              *spirv_optimizer_absolute_filepath;
  char const                                              *spirv_optimizer_arguments;
  char const                                              *spirv_optimized_absolute_filepath;
  char const                                              *spirv_debug_absolute_filepath;
  char const                                              *vk_binaries_path;
  char                                                     pass_name_upper[ CRUDE_GFX_PASS_NAME_MAX_LENGTH ];
  char                                                     vk_env[ 512 ];
  crude_string_buffer                                      temporary_string_buffer;
  uint32                                                   i;

  crude_string_buffer_initialize( &temporary_string_buffer, CRUDE_RKILO( 2 ), crude_heap_allocator_pack( allocator ) );

  glsl_absolute_filepath = crude_string_buffer_append_use_f(
    &temporary_string_buffer,
    "%s\\%s.%s",
    desc->temporary_absolute_directory,
    desc->pass_name ? desc->pass_name : "unknown",
    crude_gfx_rhi_shader_stage_to_compiler_extension( desc->stage ) );

  crude_write_file( glsl_absolute_filepath, desc->code, desc->code_size );

  pass_name_upper[ 0 ] = 0;
  for ( i = 0; desc->pass_name && desc->pass_name[ i ] != '\0'; i++ )
  {
    pass_name_upper[ i ] = toupper( desc->pass_name[ i ] );
  }
  pass_name_upper[ i ] = 0;

  CRUDE_ASSERT( i < CRUDE_GFX_PASS_NAME_MAX_LENGTH );

  crude_process_expand_environment_strings( "%VULKAN_SDK%", vk_env, 512 );
  vk_binaries_path = crude_string_buffer_append_use_f( &temporary_string_buffer, "%s\\Bin\\", vk_env );

#if defined( _MSC_VER )
  glsl_compiler_absolute_filepath = crude_string_buffer_append_use_f(
    &temporary_string_buffer,
    "%sglslangValidator.exe",
    vk_binaries_path );

  spirv_debug_absolute_filepath = crude_string_buffer_append_use_f(
    &temporary_string_buffer,
    "%s\\%s",
    desc->temporary_absolute_directory,
    "spirv_debug.spv" ); 
  
  if ( desc->optimized )
  {
    glsl_compiler_optional_arguments = "";
  }
  else
  {
#if CRUDE_DEVELOP
    glsl_compiler_optional_arguments = "-gVS --D CRUDE_DEVELOP=1";
#else /* CRUDE_DEVELOP */
    glsl_compiler_optional_arguments = "-gVS";
#endif /* CRUDE_DEVELOP */
  }

  glsl_compiler_arguments = crude_string_buffer_append_use_f(
    &temporary_string_buffer,
    "glslangValidator.exe %s -V --target-env vulkan1.2 --glsl-version 460 -o %s -S %s --D %s --D %s --D %s %s",
    glsl_absolute_filepath,
    spirv_debug_absolute_filepath,
    crude_gfx_rhi_shader_stage_to_compiler_extension( desc->stage ),
    crude_gfx_rhi_shader_stage_to_defines( desc->stage ),
    pass_name_upper,
    crude_gfx_rhi_current_graphics_api_str( ),
    glsl_compiler_optional_arguments );

#else /*_MSC_VER */
  CRUDE_ASSERT( false );
#endif /*_MSC_VER */

  crude_process_execute( ".", glsl_compiler_absolute_filepath, glsl_compiler_arguments, "" );
  
  if ( desc->optimized )
  {
    spirv_optimized_absolute_filepath = crude_string_buffer_append_use_f(
      &temporary_string_buffer,
      "%s\\%s.%s.shader_opt.spv",
      desc->compiled_absolute_directory,
      desc->pass_name ? desc->pass_name : "unknown",
      crude_gfx_rhi_shader_stage_to_compiler_extension( desc->stage ) );

    spirv_optimizer_absolute_filepath = crude_string_buffer_append_use_f( &temporary_string_buffer, "%sspirv-opt.exe", vk_binaries_path );
    spirv_optimizer_arguments = crude_string_buffer_append_use_f( &temporary_string_buffer, "spirv-opt.exe --preserve-bindings --relax-block-layout --scalar-block-layout -O %s -o %s", final_spirv_filename, optimized_spirv_filename );

    crude_process_execute( ".", spirv_optimizer_absolute_filepath, spirv_optimizer_arguments, "" );
  }
  
  if ( desc->optimized )
  {
    uint32 length = crude_string_length( spirv_optimized_absolute_filepath );
    *spirv_absolute_filepath = CRUDE_CAST( char*, crude_heap_allocator_allocate( allocator, length ) );
    crude_string_copy( *spirv_absolute_filepath, spirv_optimized_absolute_filepath, length );
  }
  else
  {
    uint32 length = crude_string_length( spirv_debug_absolute_filepath );
    *spirv_absolute_filepath = CRUDE_CAST( char*, crude_heap_allocator_allocate( allocator, length ) );
    crude_string_copy( *spirv_absolute_filepath, spirv_debug_absolute_filepath, length );
  }

  crude_string_buffer_deinitialize( &temporary_string_buffer );
}

char const*
crude_gfx_rhi_shader_stage_to_compiler_extension
(
  _In_ crude_gfx_rhi_shader_stage_flag_bits                    value
)
{
  switch ( value )
  {
    case CRUDE_GFX_RHI_SHADER_STAGE_VERTEX_BIT:          return "vert";
    case CRUDE_GFX_RHI_SHADER_STAGE_MESH_BIT_EXT:        return "mesh";
    case CRUDE_GFX_RHI_SHADER_STAGE_TASK_BIT_EXT:        return "task";
    case CRUDE_GFX_RHI_SHADER_STAGE_FRAGMENT_BIT:        return "frag";
    case CRUDE_GFX_RHI_SHADER_STAGE_COMPUTE_BIT:         return "comp";
    case CRUDE_GFX_RHI_SHADER_STAGE_CLOSEST_HIT_BIT_KHR: return "rchit";
    case CRUDE_GFX_RHI_SHADER_STAGE_RAYGEN_BIT_KHR:      return "rgen";
    case CRUDE_GFX_RHI_SHADER_STAGE_MISS_BIT_KHR:        return "rmiss";
  }
   return "";
}

crude_gfx_rhi_blend_factor
crude_gfx_rhi_string_to_blend_factor
(
  _In_ char const                                         *factor
)
{
  if ( strcmp( factor, "ZERO" ) == 0 )                     return CRUDE_GFX_RHI_BLEND_FACTOR_ZERO;
  if ( strcmp( factor, "ONE" ) == 0 )                      return CRUDE_GFX_RHI_BLEND_FACTOR_ONE;
  if ( strcmp( factor, "SRC_COLOR" ) == 0 )                return CRUDE_GFX_RHI_BLEND_FACTOR_SRC_COLOR;
  if ( strcmp( factor, "ONE_MINUS_SRC_COLOR" ) == 0 )      return CRUDE_GFX_RHI_BLEND_FACTOR_ONE_MINUS_SRC_COLOR;
  if ( strcmp( factor, "DST_COLOR" ) == 0 )                return CRUDE_GFX_RHI_BLEND_FACTOR_DST_COLOR;
  if ( strcmp( factor, "ONE_MINUS_DST_COLOR" ) == 0 )      return CRUDE_GFX_RHI_BLEND_FACTOR_ONE_MINUS_DST_COLOR;
  if ( strcmp( factor, "SRC_ALPHA" ) == 0 )                return CRUDE_GFX_RHI_BLEND_FACTOR_SRC_ALPHA;
  if ( strcmp( factor, "ONE_MINUS_SRC_ALPHA" ) == 0 )      return CRUDE_GFX_RHI_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
  if ( strcmp( factor, "DST_ALPHA" ) == 0 )                return CRUDE_GFX_RHI_BLEND_FACTOR_DST_ALPHA;
  if ( strcmp( factor, "ONE_MINUS_DST_ALPHA" ) == 0 )      return CRUDE_GFX_RHI_BLEND_FACTOR_ONE_MINUS_DST_ALPHA;
  if ( strcmp( factor, "CONSTANT_COLOR" ) == 0 )           return CRUDE_GFX_RHI_BLEND_FACTOR_CONSTANT_COLOR;
  if ( strcmp( factor, "ONE_MINUS_CONSTANT_COLOR" ) == 0 ) return CRUDE_GFX_RHI_BLEND_FACTOR_ONE_MINUS_CONSTANT_COLOR;
  if ( strcmp( factor, "CONSTANT_ALPHA" ) == 0 )           return CRUDE_GFX_RHI_BLEND_FACTOR_CONSTANT_ALPHA;
  if ( strcmp( factor, "ONE_MINUS_CONSTANT_ALPHA" ) == 0 ) return CRUDE_GFX_RHI_BLEND_FACTOR_ONE_MINUS_CONSTANT_ALPHA;
  if ( strcmp( factor, "SRC_ALPHA_SATURATE" ) == 0 )       return CRUDE_GFX_RHI_BLEND_FACTOR_SRC_ALPHA_SATURATE;
  if ( strcmp( factor, "SRC1_COLOR" ) == 0 )               return CRUDE_GFX_RHI_BLEND_FACTOR_SRC1_COLOR;
  if ( strcmp( factor, "ONE_MINUS_SRC1_COLOR" ) == 0 )     return CRUDE_GFX_RHI_BLEND_FACTOR_ONE_MINUS_SRC1_COLOR;
  if ( strcmp( factor, "SRC1_ALPHA" ) == 0 )               return CRUDE_GFX_RHI_BLEND_FACTOR_SRC1_ALPHA;
  if ( strcmp( factor, "ONE_MINUS_SRC1_ALPHA" ) == 0 )     return CRUDE_GFX_RHI_BLEND_FACTOR_ONE_MINUS_SRC1_ALPHA;
  
  return CRUDE_GFX_RHI_BLEND_FACTOR_ONE;
}

crude_gfx_rhi_blend_op
crude_gfx_rhi_string_to_blend_op
(
  _In_ char const                                         *op
)
{
  if ( strcmp( op, "ADD" ) == 0 )                          return CRUDE_GFX_RHI_BLEND_OP_ADD;
  if ( strcmp( op, "SUBTRACT" ) == 0 )                     return CRUDE_GFX_RHI_BLEND_OP_SUBTRACT;
  if ( strcmp( op, "REVERSE_SUBTRACT" ) == 0 )             return CRUDE_GFX_RHI_BLEND_OP_REVERSE_SUBTRACT;
  if ( strcmp( op, "MIN" ) == 0 )                          return CRUDE_GFX_RHI_BLEND_OP_MIN;
  if ( strcmp( op, "MAX" ) == 0 )                          return CRUDE_GFX_RHI_BLEND_OP_MAX;

  return CRUDE_GFX_RHI_BLEND_OP_ADD;
}

char const*
crude_gfx_rhi_shader_stage_to_defines
(
  _In_ crude_gfx_rhi_shader_stage_flag_bits                    value
)
{
  switch ( value )
  {
    case CRUDE_GFX_RHI_SHADER_STAGE_VERTEX_BIT:          return "CRUDE_STAGE_VERTEX";
    case CRUDE_GFX_RHI_SHADER_STAGE_FRAGMENT_BIT:        return "CRUDE_STAGE_FRAGMENT";
    case CRUDE_GFX_RHI_SHADER_STAGE_COMPUTE_BIT:         return "CRUDE_STAGE_COMPUTE";
    case CRUDE_GFX_RHI_SHADER_STAGE_MESH_BIT_EXT:        return "CRUDE_STAGE_MESH";
    case CRUDE_GFX_RHI_SHADER_STAGE_TASK_BIT_EXT:        return "CRUDE_STAGE_TASK";
    case CRUDE_GFX_RHI_SHADER_STAGE_CLOSEST_HIT_BIT_KHR: return "CRUDE_CLOSEST_HIT";
    case CRUDE_GFX_RHI_SHADER_STAGE_RAYGEN_BIT_KHR:      return "CRUDE_RAYGEN";
    case CRUDE_GFX_RHI_SHADER_STAGE_MISS_BIT_KHR:        return "CRUDE_MISS";
  }
   return "";
}