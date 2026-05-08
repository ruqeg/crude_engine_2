#include <engine/graphics/rhi.h>

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


crude_gfx_rhi_access_flags
crude_gfx_resource_state_to_access_flags
(
  _In_ crude_gfx_resource_state                            state
)
{
  VkAccessFlags2 ret = 0;
  if ( state & CRUDE_GFX_RESOURCE_STATE_COPY_SOURCE )
  {
    ret |= VK_ACCESS_2_TRANSFER_READ_BIT_KHR;
  }
  if ( state & CRUDE_GFX_RESOURCE_STATE_COPY_DEST )
  {
    ret |= VK_ACCESS_2_TRANSFER_WRITE_BIT_KHR;
  }
  if ( state & CRUDE_GFX_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER )
  {
    ret |= VK_ACCESS_2_UNIFORM_READ_BIT_KHR | VK_ACCESS_2_VERTEX_ATTRIBUTE_READ_BIT_KHR;
  }
  if ( state & CRUDE_GFX_RESOURCE_STATE_INDEX_BUFFER )
  {
    ret |= VK_ACCESS_2_INDEX_READ_BIT_KHR;
  }
  if ( state & CRUDE_GFX_RESOURCE_STATE_UNORDERED_ACCESS )
  {
    ret |= VK_ACCESS_2_SHADER_READ_BIT_KHR | VK_ACCESS_2_SHADER_WRITE_BIT_KHR;
  }
  if ( state & CRUDE_GFX_RESOURCE_STATE_INDIRECT_ARGUMENT )
  {
    ret |= VK_ACCESS_2_INDIRECT_COMMAND_READ_BIT_KHR;
  }
  if ( state & CRUDE_GFX_RESOURCE_STATE_RENDER_TARGET )
  {
    ret |= VK_ACCESS_2_COLOR_ATTACHMENT_READ_BIT_KHR | VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT_KHR;
  }
  if ( state & CRUDE_GFX_RESOURCE_STATE_DEPTH_WRITE )
  {
    ret |= VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT_KHR | VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_READ_BIT_KHR;
  }
  if ( state & ( CRUDE_GFX_RESOURCE_STATE_SHADER_RESOURCE | CRUDE_GFX_RESOURCE_STATE_PIXEL_SHADER_RESOURCE ) )
  {
    ret |= VK_ACCESS_2_SHADER_READ_BIT_KHR;
  }
  if ( state & CRUDE_GFX_RESOURCE_STATE_PRESENT )
  {
    ret |= 0;
  }
  if ( state & CRUDE_GFX_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE )
  {
    ret |= VK_ACCESS_2_ACCELERATION_STRUCTURE_READ_BIT_KHR | VK_ACCESS_2_ACCELERATION_STRUCTURE_WRITE_BIT_KHR;
  }
  return ret;
}

crude_gfx_rhi_image_layout
crude_gfx_resource_state_to_image_layout
(
  _In_ crude_gfx_resource_state                            state
)
{
  if ( state & CRUDE_GFX_RESOURCE_STATE_COPY_SOURCE )         return CRUDE_GFX_RHI_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
  if ( state & CRUDE_GFX_RESOURCE_STATE_COPY_DEST )           return CRUDE_GFX_RHI_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
  if ( state & CRUDE_GFX_RESOURCE_STATE_RENDER_TARGET )       return CRUDE_GFX_RHI_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL_KHR;
  if ( state & CRUDE_GFX_RESOURCE_STATE_DEPTH_WRITE )         return CRUDE_GFX_RHI_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL_KHR;
  if ( state & CRUDE_GFX_RESOURCE_STATE_DEPTH_READ )          return CRUDE_GFX_RHI_IMAGE_LAYOUT_READ_ONLY_OPTIMAL_KHR;
  if ( state & CRUDE_GFX_RESOURCE_STATE_UNORDERED_ACCESS )    return CRUDE_GFX_RHI_IMAGE_LAYOUT_GENERAL;
  if ( state & CRUDE_GFX_RESOURCE_STATE_SHADER_RESOURCE )     return CRUDE_GFX_RHI_IMAGE_LAYOUT_READ_ONLY_OPTIMAL_KHR;
  if ( state & CRUDE_GFX_RESOURCE_STATE_PRESENT )             return CRUDE_GFX_RHI_IMAGE_LAYOUT_PRESENT_SRC_KHR;
  if ( state == CRUDE_GFX_RESOURCE_STATE_COMMON )             return CRUDE_GFX_RHI_IMAGE_LAYOUT_GENERAL;
  return CRUDE_GFX_RHI_IMAGE_LAYOUT_UNDEFINED;
}

crude_gfx_rhi_pipeline_stage_flags
crude_gfx_determine_pipeline_stage_flags
(
  _In_ crude_gfx_rhi_access_flags                              access_flags,
  _In_ crude_gfx_queue_type                                queue_type
)
{
  VkPipelineStageFlags2KHR flags = 0;
  
  switch ( queue_type )
  {
    case CRUDE_GFX_QUEUE_TYPE_GRAPHICS:
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
    case CRUDE_GFX_QUEUE_TYPE_COMPUTE:
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
    case CRUDE_GFX_QUEUE_TYPE_COPY_TRANSFER:
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

#else
CRUDE_GFX_RHI_TO_IMPLEMENTIT
#endif