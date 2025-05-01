#include <vulkan/vk_enum_string_helper.h>

#include <core/assert.h>

#include <graphics/gpu_resources.h>

void
crude_gfx_reset_render_pass_output
(
  _In_ crude_gfx_render_pass_output                       *output
)
{
  output->num_color_formats = 0;
  for ( uint32 i = 0; i < CRUDE_GFX_MAX_IMAGE_OUTPUTS; ++i )
  {
    output->color_formats[ i ] = VK_FORMAT_UNDEFINED;
  }
  output->depth_stencil_format = VK_FORMAT_UNDEFINED;
  output->color_operation = output->depth_operation = output->stencil_operation = CRUDE_GFX_RENDER_PASS_OPERATION_DONT_CARE;
}

VkImageType
crude_gfx_to_vk_image_type
(
  _In_ crude_gfx_texture_type                              type
)
{
  static VkImageType s_vk_target[ CRUDE_GFX_TEXTURE_TYPE_TEXTURE_COUNT ] = { VK_IMAGE_TYPE_1D, VK_IMAGE_TYPE_2D, VK_IMAGE_TYPE_3D, VK_IMAGE_TYPE_1D, VK_IMAGE_TYPE_2D, VK_IMAGE_TYPE_3D };
  return s_vk_target[ type ];
}

bool
crude_gfx_has_depth_or_stencil
(
  _In_ VkFormat                                            value
)
{
  return value >= VK_FORMAT_D16_UNORM && value <= VK_FORMAT_D32_SFLOAT_S8_UINT;
}

bool
crude_gfx_has_depth
(
  _In_ VkFormat                                            value
)
{
  return ( value >= VK_FORMAT_D16_UNORM && value < VK_FORMAT_S8_UINT ) || ( value >= VK_FORMAT_D16_UNORM_S8_UINT && value <= VK_FORMAT_D32_SFLOAT_S8_UINT );
}

VkImageViewType
crude_gfx_to_vk_image_view_type
(
  _In_ crude_gfx_texture_type                              type
)
{
  static VkImageViewType s_vk_data[ CRUDE_GFX_TEXTURE_TYPE_TEXTURE_COUNT ] = { VK_IMAGE_VIEW_TYPE_1D, VK_IMAGE_VIEW_TYPE_2D, VK_IMAGE_VIEW_TYPE_3D, VK_IMAGE_VIEW_TYPE_1D_ARRAY, VK_IMAGE_VIEW_TYPE_2D_ARRAY, VK_IMAGE_VIEW_TYPE_CUBE_ARRAY };
  return s_vk_data[ type ];
}

VkFormat
crude_gfx_to_vk_vertex_format
(
  _In_ crude_gfx_vertex_component_format                   value
)
{
  static VkFormat s_vk_vertex_formats[ CRUDE_GFX_VERTEX_COMPONENT_FORMAT_COUNT ] =
  {
    VK_FORMAT_R32_SFLOAT, VK_FORMAT_R32G32_SFLOAT, VK_FORMAT_R32G32B32_SFLOAT, VK_FORMAT_R32G32B32A32_SFLOAT, /*MAT4 TODO*/VK_FORMAT_R32G32B32A32_SFLOAT,
    VK_FORMAT_R8_SINT, VK_FORMAT_R8G8B8A8_SNORM, VK_FORMAT_R8_UINT, VK_FORMAT_R8G8B8A8_UINT, VK_FORMAT_R16G16_SINT, VK_FORMAT_R16G16_SNORM,
    VK_FORMAT_R16G16B16A16_SINT, VK_FORMAT_R16G16B16A16_SNORM, VK_FORMAT_R32_UINT, VK_FORMAT_R32G32_UINT, VK_FORMAT_R32G32B32A32_UINT
  };

  return s_vk_vertex_formats[ value ];
}

VkAccessFlags
crude_gfx_resource_state_to_vk_access_flags
(
  _In_ crude_gfx_resource_state                            state
)
{
  VkAccessFlags ret = 0;
  if ( state & CRUDE_GFX_RESOURCE_STATE_COPY_SOURCE )
  {
    ret |= VK_ACCESS_TRANSFER_READ_BIT;
  }
  if ( state & CRUDE_GFX_RESOURCE_STATE_COPY_DEST )
  {
    ret |= VK_ACCESS_TRANSFER_WRITE_BIT;
  }
  if ( state & CRUDE_GFX_RESOURCE_STATE_PRESENT )
  {
    ret |= VK_ACCESS_MEMORY_READ_BIT;
  }
  if ( state & CRUDE_GFX_RESOURCE_STATE_SHADER_RESOURCE )
  {
    ret |= VK_ACCESS_SHADER_READ_BIT;
  }
  return ret;
}

VkImageLayout
crude_gfx_resource_state_to_vk_image_layout
(
  _In_ crude_gfx_resource_state                            state
)
{
  if ( state & CRUDE_GFX_RESOURCE_STATE_COPY_SOURCE )
  {
    return VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
  }
  if ( state & CRUDE_GFX_RESOURCE_STATE_COPY_DEST )
  {
    return VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
  }
  if ( state & CRUDE_GFX_RESOURCE_STATE_SHADER_RESOURCE )
  {
    return VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
  }
  if ( state & CRUDE_GFX_RESOURCE_STATE_PRESENT )
  {
    return VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
  }
  return VK_IMAGE_LAYOUT_UNDEFINED;
}

CRUDE_API VkPipelineStageFlags
crude_gfx_determine_pipeline_stage_flags
(
  _In_ VkAccessFlags                                       access_flags,
  _In_ crude_gfx_queue_type                                queue_type
)
{
  VkPipelineStageFlags flags = 0;
  
  switch ( queue_type )
  {
    case CRUDE_GFX_QUEUE_TYPE_GRAPHICS:
    {
      if ( access_flags & ( VK_ACCESS_INDEX_READ_BIT | VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT ) )
        flags |= VK_PIPELINE_STAGE_VERTEX_INPUT_BIT;
      
      if ( access_flags & ( VK_ACCESS_UNIFORM_READ_BIT | VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT ) )
      {
        flags |= VK_PIPELINE_STAGE_VERTEX_SHADER_BIT;
        flags |= VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
        flags |= VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
        //flags |= VK_PIPELINE_STAGE_RAY_TRACING_SHADER_BIT_KHR;
      }
      
      if ( access_flags & VK_ACCESS_INPUT_ATTACHMENT_READ_BIT )
      {
        flags |= VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
      }

      if ( access_flags & ( VK_ACCESS_ACCELERATION_STRUCTURE_READ_BIT_KHR | VK_ACCESS_ACCELERATION_STRUCTURE_WRITE_BIT_KHR ) )
      {
        flags |= VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_KHR;
      }

      if ( access_flags & ( VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT ) )
      {
        flags |= VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
      }

      if ( access_flags & VK_ACCESS_FRAGMENT_SHADING_RATE_ATTACHMENT_READ_BIT_KHR )
      {
        flags = VK_PIPELINE_STAGE_FRAGMENT_SHADING_RATE_ATTACHMENT_BIT_KHR;
      }

      if ( access_flags & ( VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT ) )
      {
        flags |= VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
      }
      break;
    }
    case CRUDE_GFX_QUEUE_TYPE_COMPUTE:
    {
      if ( ( access_flags & ( VK_ACCESS_INDEX_READ_BIT | VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT ) ) ||
           ( access_flags & VK_ACCESS_INPUT_ATTACHMENT_READ_BIT ) ||
           ( access_flags & ( VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT ) ) ||
           ( access_flags & ( VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT ) ) )
      {
        return VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;
      }

      if ( access_flags & ( VK_ACCESS_UNIFORM_READ_BIT | VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT ) )
      {
          flags |= VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
      }
      break;
    }
    case CRUDE_GFX_QUEUE_TYPE_COPY_TRANSFER:
    {
      return VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;
    }
  }
  
  if ( access_flags & VK_ACCESS_INDIRECT_COMMAND_READ_BIT )
  {
    flags |= VK_PIPELINE_STAGE_DRAW_INDIRECT_BIT;
  }

  if ( access_flags & ( VK_ACCESS_TRANSFER_READ_BIT | VK_ACCESS_TRANSFER_WRITE_BIT ) )
  {
    flags |= VK_PIPELINE_STAGE_TRANSFER_BIT;
  }
  
  if ( access_flags & ( VK_ACCESS_HOST_READ_BIT | VK_ACCESS_HOST_WRITE_BIT ) )
  {
    flags |= VK_PIPELINE_STAGE_HOST_BIT;
  }
  
  if ( flags == 0 )
  {
    flags = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
  }

  return flags;
}

VkFormat
crude_string_to_vk_format
(
  _In_ char const                                         *format
)
{
  // !TODO HASH STRING
  CRUDE_ASSERT( false );
  return VK_FORMAT_UNDEFINED;
}