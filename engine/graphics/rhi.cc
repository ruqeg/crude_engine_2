#include <engine/core/log.h>
#include <engine/core/assert.h>

#include <engine/graphics/rhi.h>

#if CRUDE_GFX_VULKAN
#define CRUDE_GFX_RHI_HANDLE_VULKAN_RESULT( result, ... )\
{\
  if ( result != VK_SUCCESS )\
  {\
    CRUDE_ABORT( CRUDE_CHANNEL_GRAPHICS, "vulkan result isn't success: %i %s", result, ##__VA_ARGS__ );\
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
crude_gfx_resource_state_to_access_flags
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
crude_gfx_resource_state_to_image_layout
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
crude_gfx_determine_pipeline_stage_flags
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

  vk_result = g_pfn.vkQueueSubmit2KHR( queue.vk_queue, 1u, &vk_submit_info, fence.vk_fence );
  return vk_result != VK_ERROR_DEVICE_LOST && vk_result != VK_ERROR_OUT_OF_DEVICE_MEMORY && vk_result != VK_ERROR_UNKNOWN;
}

void
crude_gfx_rhi_destroy_surface
(
  _In_ crude_gfx_rhi_instance                              instance,
  _In_ crude_gfx_rhi_surface                               surface
)
{
  vkDestroySurfaceKHR( instance.vk_instance, surface.vk_surface, g_pfn.vk_allocation_callbacks );
}

void
crude_gfx_rhi_destroy_device
(
  _In_ crude_gfx_rhi_device                               *device,
  _In_ crude_gfx_rhi_instance                              instance
)
{
#if CRUDE_GRAPHICS_VALIDATION_LAYERS_ENABLED
  g_pfn.vkDestroyDebugUtilsMessengerEXT( instance.vk_instance, device->vk_debug_utils_messenger, g_pfn.vk_allocation_callbacks );
#endif /* CRUDE_GRAPHICS_VALIDATION_LAYERS_ENABLED */

  vmaDestroyAllocator( device->vma_allocator );
  vkDestroyDevice( device->vk_device, g_pfn.vk_allocation_callbacks );
}

void
crude_gfx_rhi_destroy_instance
(
  _In_ crude_gfx_rhi_instance                              instance
)
{
  vkDestroyInstance( instance.vk_instance, g_pfn.vk_allocation_callbacks );
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
crude_gfx_rhi_update_descriptor_sets
(
  _In_ crude_gfx_rhi_device                               *device,
  _In_ uint32                                              descriptor_write_count,
  _In_ crude_gfx_rhi_write_descriptor_set const           *descriptor_writes
)
{
  VkWriteDescriptorSet                                     vk_descriptor_writes[ 2048 ];
  VkDescriptorImageInfo                                    vk_image_info[ 2048 ];
  uint32                                                   current_write_index;

  CRUDE_ASSERT( descriptor_write_count < CRUDE_COUNTOF( vk_descriptor_writes ) );

  for ( int32 i = 0u; i < descriptor_write_count; ++i )
  {
    vk_image_info[ i ].imageLayout = CRUDE_CAST( VkImageLayout, descriptor_writes[ i ].image_info.image_layout );
    vk_image_info[ i ].imageView = descriptor_writes[ i ].image_info.image_view.vk_image_view;
    vk_image_info[ i ].sampler = descriptor_writes[ i ].image_info.sampler.vk_sampler;

    vk_descriptor_writes[ i ].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    vk_descriptor_writes[ i ].pNext = NULL;
    vk_descriptor_writes[ i ].dstSet = descriptor_writes[ i ].descriptor_set.vk_descriptor_set;
    vk_descriptor_writes[ i ].dstBinding = descriptor_writes[ i ].dst_binding;
    vk_descriptor_writes[ i ].dstArrayElement = descriptor_writes[ i ].dst_array_element;
    vk_descriptor_writes[ i ].descriptorCount = descriptor_writes[ i ].descriptor_count;
    vk_descriptor_writes[ i ].descriptorType = CRUDE_CAST( VkDescriptorType, descriptor_writes[ i ].descriptor_type );
    vk_descriptor_writes[ i ].pImageInfo = &vk_image_info[ i ];
    vk_descriptor_writes[ i ].pBufferInfo = NULL;
    vk_descriptor_writes[ i ].pTexelBufferView = NULL;
  }

  vkUpdateDescriptorSets( device->vk_device, descriptor_write_count, vk_descriptor_writes, 0, NULL );
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
  
  vk_semaphore_type_info = CRUDE_COMPOUNT_EMPTY( VkSemaphoreTypeCreateInfo );
  vk_semaphore_type_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_TYPE_CREATE_INFO;
  vk_semaphore_type_info.semaphoreType = VK_SEMAPHORE_TYPE_TIMELINE;

  vk_semaphore_info = CRUDE_COMPOUNT_EMPTY( VkSemaphoreCreateInfo );
  vk_semaphore_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
  vk_semaphore_info.pNext = &vk_semaphore_type_info;

  vkCreateSemaphore( device->vk_device, &vk_semaphore_info, g_pfn.vk_allocation_callbacks, &semaphore->vk_semaphore );
}

void
crude_gfx_rhi_destroy_semaphore
(
  _In_ crude_gfx_rhi_device                               *device,
  _In_ crude_gfx_rhi_semaphore                             semaphore
)
{
  vkDestroySemaphore( device->vk_device, semaphore.vk_semaphore, g_pfn.vk_allocation_callbacks );
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
  _Out_ crude_gfx_rhi_fence                               *fence
)
{
  VkFenceCreateInfo fence_info = CRUDE_COMPOUNT_EMPTY( VkFenceCreateInfo );
  fence_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
  fence_info.flags = VK_FENCE_CREATE_SIGNALED_BIT;
  vkCreateFence( device->vk_device, &fence_info, g_pfn.vk_allocation_callbacks, &fence->vk_fence );
}
  
void
crude_gfx_rhi_destroy_fence
(
  _In_ crude_gfx_rhi_device                               *device,
  _In_ crude_gfx_rhi_fence                                 fence
)
{
  vkDestroyFence( device->vk_device, fence.vk_fence, g_pfn.vk_allocation_callbacks );
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
  if ( g_pfn.vkSetDebugUtilsObjectNameEXT )
  {
    VkDebugUtilsObjectNameInfoEXT vk_name_info = CRUDE_COMPOUNT_EMPTY( VkDebugUtilsObjectNameInfoEXT );
    vk_name_info.sType        = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT;
    vk_name_info.objectType   = CRUDE_CAST( VkObjectType, object_type );
    vk_name_info.objectHandle = object_handle;
    vk_name_info.pObjectName  = object_name;
    g_pfn.vkSetDebugUtilsObjectNameEXT( device->vk_device, &vk_name_info );
  }
}

void
crude_gfx_rhi_destroy_descriptor_pool
(
  _In_ crude_gfx_rhi_device                               *device,
  _Out_ crude_gfx_rhi_descriptor_pool                     *descriptor_pool
)
{
  vkDestroyDescriptorPool( device->vk_device, descriptor_pool->vk_descriptor_pool, g_pfn.vk_allocation_callbacks );
}

void
crude_gfx_rhi_begin_command_buffer
(
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
  
  g_pfn.vkCmdBeginRenderingKHR( command_buffer.vk_command_buffer, &vk_rendering_info );
}

void
crude_gfx_rhi_command_buffer_end_rendering
(
  _In_ crude_gfx_rhi_command_buffer                        command_buffer
)
{
  g_pfn.vkCmdEndRenderingKHR( command_buffer.vk_command_buffer );
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
crude_gfx_rhi_command_buffer_copy_image
(
  _In_ crude_gfx_rhi_command_buffer                        command_buffer,
  _In_ crude_gfx_rhi_image                                 src_image,
  _In_ crude_gfx_rhi_image_layout                          src_image_layout,
  _In_ crude_gfx_rhi_image                                 dst_image,
  _In_ crude_gfx_rhi_image_layout                          dst_image_layout,
  _In_ crude_gfx_rhi_image_copy const                     *image_copy
)
{
  VkImageCopy                                              region;

  region = CRUDE_COMPOUNT_EMPTY( VkImageCopy );
  region.srcSubresource.aspectMask = image_copy->src_subresource.aspect_mask;
  region.srcSubresource.mipLevel = image_copy->src_subresource.mip_level;
  region.srcSubresource.baseArrayLayer = image_copy->src_subresource.base_array_layer;
  region.srcSubresource.layerCount = image_copy->src_subresource.layer_count;
  region.srcOffset.x = image_copy->src_offset.x;
  region.srcOffset.y = image_copy->src_offset.y;
  region.srcOffset.z = image_copy->src_offset.z;
  region.dstSubresource.aspectMask = image_copy->dst_subresource.aspect_mask;
  region.dstSubresource.mipLevel = image_copy->dst_subresource.mip_level;
  region.dstSubresource.baseArrayLayer = image_copy->dst_subresource.base_array_layer;
  region.dstSubresource.layerCount = image_copy->dst_subresource.layer_count;
  region.dstOffset.x = image_copy->dst_offset.x;
  region.dstOffset.y = image_copy->dst_offset.y;
  region.dstOffset.z = image_copy->dst_offset.z;
  region.extent.width = image_copy->extent.x;
  region.extent.height = image_copy->extent.y;
  region.extent.depth = image_copy->extent.z;

  vkCmdCopyImage( command_buffer.vk_command_buffer, src_image.vk_image, CRUDE_CAST( VkImageLayout, src_image_layout ), dst_image.vk_image, CRUDE_CAST( VkImageLayout, dst_image_layout ), 1u, &region );
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
  _In_ uint64                                              offset,
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
  _In_ uint64                                              argument_buffer_offset,
  _In_ crude_gfx_rhi_buffer                                count_buffer,
  _In_ uint64                                              count_buffer_offset,
  _In_ uint32                                              max_draw_count,
  _In_ uint32                                              stride
)
{
  vkCmdDrawIndirectCount( command_buffer.vk_command_buffer, argument_buffer.vk_buffer, argument_buffer_offset, count_buffer.vk_buffer, count_buffer_offset, max_draw_count, stride );
}

void
crude_gfx_rhi_command_buffer_draw_mesh_task
(
  _In_ crude_gfx_rhi_command_buffer                        command_buffer,
  _In_ uint32                                              group_count_x,
  _In_ uint32                                              group_count_y,
  _In_ uint32                                              group_count_z
)
{
  g_pfn.vkCmdDrawMeshTasksEXT( command_buffer.vk_command_buffer, group_count_x, group_count_y, group_count_z );
}

void
crude_gfx_rhi_command_buffer_draw_mesh_task_indirect_count
(
  _In_ crude_gfx_rhi_command_buffer                        command_buffer,
  _In_ crude_gfx_rhi_buffer                                argument_buffer,
  _In_ uint64                                              argument_buffer_offset,
  _In_ crude_gfx_rhi_buffer                                count_buffer,
  _In_ uint64                                              count_buffer_offset,
  _In_ uint32                                              max_draw_count,
  _In_ uint32                                              stride
)
{
  g_pfn.vkCmdDrawMeshTasksIndirectCountEXT( command_buffer.vk_command_buffer, argument_buffer.vk_buffer, argument_buffer_offset, count_buffer.vk_buffer, count_buffer_offset, max_draw_count, stride );
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
  vk_image_barrier.srcQueueFamilyIndex = image_memory_barriers->src_queue_family_index;
  vk_image_barrier.dstQueueFamilyIndex = image_memory_barriers->dst_queue_family_index;
  vk_image_barrier.image = image_memory_barriers->image->vk_image;
  vk_image_barrier.subresourceRange.aspectMask = image_memory_barriers->subresource_range.aspect_mask;
  vk_image_barrier.subresourceRange.baseArrayLayer = image_memory_barriers->subresource_range.base_array_layer;
  vk_image_barrier.subresourceRange.baseMipLevel = image_memory_barriers->subresource_range.base_mip_level;
  vk_image_barrier.subresourceRange.layerCount = image_memory_barriers->subresource_range.layer_count;
  vk_image_barrier.subresourceRange.levelCount = image_memory_barriers->subresource_range.level_count;
  
  vk_dependency_info.imageMemoryBarrierCount = 1u;
  vk_dependency_info.pImageMemoryBarriers = &vk_image_barrier;
 
  g_pfn.vkCmdPipelineBarrier2KHR( command_buffer.vk_command_buffer, &vk_dependency_info );
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
  vk_buffer_barrier.srcQueueFamilyIndex = buffer_memory_barriers->src_queue_family_index;
  vk_buffer_barrier.dstQueueFamilyIndex = buffer_memory_barriers->dst_queue_family_index;
  vk_buffer_barrier.buffer = buffer_memory_barriers->buffer->vk_buffer;
  vk_buffer_barrier.offset = buffer_memory_barriers->offset;
  vk_buffer_barrier.size = buffer_memory_barriers->size;
  
  vk_dependency_info.bufferMemoryBarrierCount = 1u;
  vk_dependency_info.pBufferMemoryBarriers = &vk_buffer_barrier;
 
  g_pfn.vkCmdPipelineBarrier2KHR( command_buffer.vk_command_buffer, &vk_dependency_info );
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
  
  g_pfn.vkCmdPipelineBarrier2KHR( command_buffer.vk_command_buffer, &vk_dependency_info );
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
  g_pfn.vkCmdBeginDebugUtilsLabelEXT( command_buffer.vk_command_buffer, &vk_label );
}

void
crude_gfx_rhi_command_buffer_end_debug_utils_label
(
  _In_ crude_gfx_rhi_command_buffer                        command_buffer
)
{
  g_pfn.vkCmdEndDebugUtilsLabelEXT( command_buffer.vk_command_buffer );
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
  _In_ uint64                                              dst_offset,
  _In_ uint64                                              size,
  _In_ uint32                                              data
)
{
  vkCmdFillBuffer( command_buffer.vk_command_buffer, dst_buffer.vk_buffer, dst_offset, size, data );
}

void
crude_gfx_rhi_command_buffer_trace_rays
(
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
  
  g_pfn.vkCmdTraceRaysKHR( command_buffer.vk_command_buffer, &vk_raygen_table, &vk_miss_table, &vk_hit_table, &vk_callable_table, width, height, depth );
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

#elif CRUDE_GFX_NAPI

bool
crude_gfx_rhi_format_has_depth_or_stencil
(
  _In_ crude_gfx_rhi_format                                    value
)
{
  return 0;
}

bool
crude_gfx_rhi_format_has_depth
(
  _In_ crude_gfx_rhi_format                                    value
)
{
  return 0;
}

crude_gfx_rhi_blend_factor
crude_gfx_rhi_string_to_blend_factor
(
  _In_ char const                                         *factor
)
{
  return CRUDE_GFX_RHI_BLEND_FACTOR_MAX_ENUM;
}

crude_gfx_rhi_blend_op
crude_gfx_rhi_string_to_blend_op
(
  _In_ char const                                         *op
)
{
  return CRUDE_GFX_RHI_BLEND_OP_MAX_ENUM;
}


crude_gfx_rhi_access_flags
crude_gfx_rhi_resource_state_to_access_flags
(
  _In_ crude_gfx_rhi_resource_state                            state
)
{
  return 0;
}

crude_gfx_rhi_image_layout
crude_gfx_rhi_resource_state_to_image_layout
(
  _In_ crude_gfx_rhi_resource_state                            state
)
{
  return CRUDE_GFX_RHI_IMAGE_LAYOUT_UNDEFINED;
}

crude_gfx_rhi_pipeline_stage_flags
crude_gfx_rhi_determine_pipeline_stage_flags
(
  _In_ crude_gfx_rhi_access_flags                          access_flags,
  _In_ crude_gfx_rhi_queue_type                            queue_type
)
{
  return 0;
}

void
crude_gfx_rhi_begin_command_buffer
(
  _In_ crude_gfx_rhi_device const                         *device,
  _In_ crude_gfx_command_buffer_begin_info const          *begin_info,
  _Out_ crude_gfx_rhi_command_buffer                      *buffer
)
{
}

#else
CRUDE_GFX_RHI_TO_IMPLEMENTIT
#endif