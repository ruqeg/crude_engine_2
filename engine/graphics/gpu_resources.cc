#include <vulkan/vk_enum_string_helper.h>

#include <engine/core/assert.h>
#include <engine/core/hash_map.h>

#include <engine/graphics/gpu_resources.h>

/************************************************
 *
 * GPU Resoruces Creation Empty Functions
 * 
 ***********************************************/
crude_gfx_sampler_creation
crude_gfx_sampler_creation_empty
(
)
{
  return CRUDE_COMPOUNT_EMPTY( crude_gfx_sampler_creation );
}

crude_gfx_buffer_creation
crude_gfx_buffer_creation_empty
(
)
{
  return CRUDE_COMPOUNT_EMPTY( crude_gfx_buffer_creation );
}

crude_gfx_framebuffer_creation
crude_gfx_framebuffer_creation_empty
(
)
{
  crude_gfx_framebuffer_creation creation = CRUDE_COMPOUNT_EMPTY( crude_gfx_framebuffer_creation );
  creation.depth_stencil_texture = CRUDE_GFX_TEXTURE_HANDLE_INVALID;
  creation .resize = true;
  return creation;
}

crude_gfx_pipeline_creation
crude_gfx_pipeline_creation_empty
(
)
{
  crude_gfx_pipeline_creation creation = CRUDE_COMPOUNT_EMPTY( crude_gfx_pipeline_creation );
  creation.relfect_vertex_input = true;
  creation.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
  return creation;
}

void
crude_gfx_pipeline_creation_add_blend_state
(
  _In_ crude_gfx_pipeline_creation                        *creation,
  _In_ crude_gfx_blend_state                               blend_state
)
{
  creation->blend_state.blend_states[ creation->blend_state.active_states++ ] = blend_state;
}

void
crude_gfx_pipeline_creation_add_vertex_stream
(
  _In_ crude_gfx_pipeline_creation                        *creation,
  _In_ uint32                                              binding,
  _In_ uint32                                              stride,
  _In_ crude_gfx_vertex_input_rate                         input_rate
)
{
  creation->relfect_vertex_input = false;
  creation->vertex_streams[ creation->vertex_streams_num ].binding = binding;
  creation->vertex_streams[ creation->vertex_streams_num ].stride = stride;
  creation->vertex_streams[ creation->vertex_streams_num ].input_rate = input_rate;
  ++creation->vertex_streams_num;
}

void
crude_gfx_pipeline_creation_add_vertex_attribute
(
  _In_ crude_gfx_pipeline_creation                        *creation,
  _In_ uint32                                              location,
  _In_ uint32                                              binding,
  _In_ uint32                                              offset,
  _In_ crude_gfx_vertex_component_format                   format
)
{
  creation->relfect_vertex_input = false;
  creation->vertex_attributes[ creation->vertex_attributes_num ].location = location;
  creation->vertex_attributes[ creation->vertex_attributes_num ].binding = binding;
  creation->vertex_attributes[ creation->vertex_attributes_num ].offset = offset;
  creation->vertex_attributes[ creation->vertex_attributes_num ].format = format;
  ++creation->vertex_attributes_num;
}

crude_gfx_descriptor_set_creation
crude_gfx_descriptor_set_creation_empty
(
)
{
  return CRUDE_COMPOUNT_EMPTY( crude_gfx_descriptor_set_creation );
}

crude_gfx_render_pass_creation
crude_gfx_render_pass_creation_empty
(
)
{
  return CRUDE_COMPOUNT( crude_gfx_render_pass_creation, {
    .depth_stencil_format = VK_FORMAT_UNDEFINED,
    .depth_operation = CRUDE_GFX_RENDER_PASS_OPERATION_DONT_CARE,
    .stencil_operation = CRUDE_GFX_RENDER_PASS_OPERATION_DONT_CARE
  } );
}

crude_gfx_texture_creation
crude_gfx_texture_creation_empty
(
)
{
  crude_gfx_texture_creation creation = CRUDE_COMPOUNT_EMPTY( crude_gfx_texture_creation );
  creation.width   = 1;
  creation.height  = 1;
  creation.depth   = 1;
  creation.format  = VK_FORMAT_UNDEFINED;
  creation.type    = CRUDE_GFX_TEXTURE_TYPE_TEXTURE_2D;
  creation.alias   = CRUDE_GFX_TEXTURE_HANDLE_INVALID;
  creation.subresource.array_base_layer = 0u;
  creation.subresource.array_layer_count = 1;
  creation.subresource.mip_base_level = 0;
  creation.subresource.mip_level_count = 1;
  return creation;
}

crude_gfx_texture_view_creation
crude_gfx_texture_view_creation_empty
(
)
{
  crude_gfx_texture_view_creation creation = CRUDE_COMPOUNT_EMPTY( crude_gfx_texture_view_creation );
  creation.subresource.array_base_layer = 0u;
  creation.subresource.array_layer_count = 1;
  creation.subresource.mip_base_level = 0;
  creation.subresource.mip_level_count = 1;
  creation.parent_texture_handle = CRUDE_GFX_TEXTURE_HANDLE_INVALID;
  return creation;
}

crude_gfx_render_pass_output
crude_gfx_render_pass_output_empty
(
)
{
  crude_gfx_render_pass_output output;

  output.num_color_formats = 0;
  for ( uint32 i = 0; i < CRUDE_GFX_IMAGE_OUTPUTS_MAX_COUNT; ++i )
  {
    output.color_formats[ i ] = VK_FORMAT_UNDEFINED;
    output.color_final_layouts[ i ] = VK_IMAGE_LAYOUT_UNDEFINED;
    output.color_operations[ i ] = CRUDE_GFX_RENDER_PASS_OPERATION_DONT_CARE;
  }
  output.depth_stencil_format = VK_FORMAT_UNDEFINED;
  output.depth_operation = output.stencil_operation = CRUDE_GFX_RENDER_PASS_OPERATION_DONT_CARE;

  return output;
}

void
crude_gfx_render_pass_output_add_color
(
  _In_ crude_gfx_render_pass_output                       *output,
  _In_ VkFormat                                            color_format,
  _In_ VkImageLayout                                       color_final_layout,
  _In_ crude_gfx_render_pass_operation                     color_operation
)
{
  output->color_formats[ output->num_color_formats ] = VK_FORMAT_UNDEFINED;
  output->color_final_layouts[ output->num_color_formats ] = VK_IMAGE_LAYOUT_UNDEFINED;
  output->color_operations[ output->num_color_formats ] = CRUDE_GFX_RENDER_PASS_OPERATION_DONT_CARE;
  ++output->num_color_formats;
}

void
crude_gfx_render_pass_output_set_depth
(
  _In_ crude_gfx_render_pass_output                       *output,
  _In_ VkFormat                                            depth_stencil_format,
  _In_ VkImageLayout                                       depth_stencil_final_layout,
  _In_ crude_gfx_render_pass_operation                     depth_operation,
  _In_ crude_gfx_render_pass_operation                     stencil_operation
)
{
  output->depth_stencil_format = depth_stencil_format;
  output->depth_stencil_final_layout = depth_stencil_final_layout;
  output->depth_operation = depth_operation;
  output->stencil_operation = stencil_operation;
}

void
crude_gfx_descriptor_set_creation_add_buffer
(
  _In_ crude_gfx_descriptor_set_creation                  *creation,
  _In_ crude_gfx_buffer_handle                             buffer,
  _In_ uint16                                              binding
)
{
  creation->samplers[ creation->num_resources ] = CRUDE_GFX_SAMPLER_HANDLE_INVALID;
  creation->bindings[ creation->num_resources ] = binding;
  creation->resources[ creation->num_resources++ ] = buffer.index;
}

void
crude_gfx_descriptor_set_creation_add_acceleration_structure
(
  _In_ crude_gfx_descriptor_set_creation                  *creation,
  _In_ VkAccelerationStructureKHR                          vk_acceleration_structure,
  _In_ uint16                                              binding
)
{
#if CRUDE_GRAPHICS_RAY_TRACING_ENABLED
  creation->vk_acceleration_structure = vk_acceleration_structure;
  creation->bindings[ creation->num_resources++ ] = binding;
#else
  CRUDE_ASSERTM( CRUDE_CHANNEL_GRAPHICS, false, "Can't add acceleration structure to descriptor set. CRUDE_GRAPHICS_RAY_TRACING_ENABLED wasn't enabled" );
#endif /* CRUDE_GRAPHICS_RAY_TRACING_ENABLED*/
}

void
crude_gfx_descriptor_set_creation_add_texture
(
  _In_ crude_gfx_descriptor_set_creation                  *creation,
  _In_ crude_gfx_texture_handle                            texture,
  _In_ uint16                                              binding
)
{
  creation->samplers[ creation->num_resources ] = CRUDE_GFX_SAMPLER_HANDLE_INVALID;
  creation->bindings[ creation->num_resources ] = binding;
  creation->resources[ creation->num_resources++ ] = texture.index;
}

void
crude_gfx_descriptor_set_layout_creation_add_binding
(
  _In_ crude_gfx_descriptor_set_layout_creation           *creation,
  _In_ crude_gfx_descriptor_set_layout_binding             binding
)
{
  creation->bindings[ creation->num_bindings++ ] = binding;
}

void
crude_gfx_shader_state_creation_add_stage
(
  _In_ crude_gfx_shader_state_creation                    *creation,
  _In_ char const                                         *code,
  _In_ uint64                                              code_size,
  _In_ VkShaderStageFlagBits                               type
)
{
  creation->stages[ creation->stages_count ].code = code;
  creation->stages[ creation->stages_count ].code_size = code_size;
  creation->stages[ creation->stages_count ].type = type;
  ++creation->stages_count;
}

uint32
crude_gfx_technique_get_pass_index
(
  _In_ crude_gfx_technique                                *technique,
  _In_ char const                                         *name
)
{
  uint64 hashed_name = crude_hash_string( name, 0 );
  uint32 index = CRUDE_HASHMAP_GET( technique->name_hashed_to_pass_index, hashed_name )->value;
  return index;
}

uint16
crude_gfx_technique_pass_get_binding_index
(
  _In_ crude_gfx_technique_pass                           *technique_pass,
  _In_ char const                                         *name
)
{
  CRUDE_ABORT( CRUDE_CHANNEL_GRAPHICS, "technique_pass->name_hashed_to_descriptor_index is not implemented yet! lookt at crude_gfx_renderer_create_technique" );
  uint64 hashed_name = crude_hash_string( name, 0 );
  uint32 index = CRUDE_HASHMAP_GET( technique_pass->name_hashed_to_descriptor_index, hashed_name )->value;
  return index;
}

void
crude_gfx_technique_creation_add_pass
(
  _In_ crude_gfx_technique_creation                       *creation,
  _In_ crude_gfx_pipeline_handle                           pipeline
)
{
  creation->passes[ creation->passes_count ].pipeline = pipeline;
  ++creation->passes_count;
}

/************************************************
 *
 * GPU Resoruces Functions
 * 
 ***********************************************/
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
    VK_FORMAT_R32_SFLOAT, VK_FORMAT_R32G32_SFLOAT, VK_FORMAT_R32G32B32_SFLOAT, VK_FORMAT_R32G32B32A32_SFLOAT, VK_FORMAT_R32G32B32A32_SFLOAT,
    VK_FORMAT_R8_SINT, VK_FORMAT_R8G8B8A8_SNORM, VK_FORMAT_R8_UINT, VK_FORMAT_R8G8B8A8_UINT, VK_FORMAT_R16G16_SINT, VK_FORMAT_R16G16_SNORM,
    VK_FORMAT_R16G16B16A16_SINT, VK_FORMAT_R16G16B16A16_SNORM, VK_FORMAT_R32_UINT, VK_FORMAT_R32G32_UINT, VK_FORMAT_R32G32B32A32_UINT
  };

  return s_vk_vertex_formats[ value ];
}

CRUDE_API VkAccessFlags2
crude_gfx_resource_state_to_vk_access_flags2
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
    ret |= VK_ACCESS_ACCELERATION_STRUCTURE_READ_BIT_NV | VK_ACCESS_ACCELERATION_STRUCTURE_WRITE_BIT_NV;
  }
  return ret;
}

VkImageLayout
crude_gfx_resource_state_to_vk_image_layout2
(
  _In_ crude_gfx_resource_state                            state
)
{
  if ( state & CRUDE_GFX_RESOURCE_STATE_COPY_SOURCE )         return VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
  if ( state & CRUDE_GFX_RESOURCE_STATE_COPY_DEST )           return VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
  if ( state & CRUDE_GFX_RESOURCE_STATE_RENDER_TARGET )       return VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL_KHR;
  if ( state & CRUDE_GFX_RESOURCE_STATE_DEPTH_WRITE )         return VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL_KHR;
  if ( state & CRUDE_GFX_RESOURCE_STATE_DEPTH_READ )          return VK_IMAGE_LAYOUT_READ_ONLY_OPTIMAL_KHR;
  if ( state & CRUDE_GFX_RESOURCE_STATE_UNORDERED_ACCESS )    return VK_IMAGE_LAYOUT_GENERAL;
  if ( state & CRUDE_GFX_RESOURCE_STATE_SHADER_RESOURCE )     return VK_IMAGE_LAYOUT_READ_ONLY_OPTIMAL_KHR;
  if ( state & CRUDE_GFX_RESOURCE_STATE_PRESENT )             return VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
  if ( state == CRUDE_GFX_RESOURCE_STATE_COMMON )             return VK_IMAGE_LAYOUT_GENERAL;
  return VK_IMAGE_LAYOUT_UNDEFINED;
}

VkPipelineStageFlags2
crude_gfx_determine_pipeline_stage_flags2
(
  _In_ VkAccessFlags2                                      access_flags,
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

VkFormat
crude_gfx_string_to_vk_format
(
  _In_ char const                                         *format
)
{
  if ( strcmp( format, "VK_FORMAT_R16G16B16A16_SFLOAT" ) == 0 ) return VK_FORMAT_R16G16B16A16_SFLOAT;
  if ( strcmp( format, "VK_FORMAT_D32_SFLOAT" ) == 0 ) return VK_FORMAT_D32_SFLOAT;
  if ( strcmp( format, "VK_FORMAT_D16_UNORM" ) == 0 ) return VK_FORMAT_D16_UNORM;
  if ( strcmp( format, "VK_FORMAT_R8G8B8_SRGB" ) == 0 ) return VK_FORMAT_R8G8B8_SRGB;
  if ( strcmp( format, "VK_FORMAT_R8G8B8A8_SRGB" ) == 0 ) return VK_FORMAT_R8G8B8A8_SRGB;
  if ( strcmp( format, "VK_FORMAT_R8G8B8A8_UNORM" ) == 0 ) return VK_FORMAT_R8G8B8A8_UNORM;
  if ( strcmp( format, "VK_FORMAT_R16G16_SNORM" ) == 0 ) return VK_FORMAT_R16G16_SNORM;
  if ( strcmp( format, "VK_FORMAT_R16G16B16A16_SNORM" ) == 0 ) return VK_FORMAT_R16G16B16A16_SNORM;
  if ( strcmp( format, "VK_FORMAT_R16G16B16A16_UNORM" ) == 0 ) return VK_FORMAT_R16G16B16A16_UNORM;
  if ( strcmp( format, "VK_FORMAT_R16_UINT" ) == 0 ) return VK_FORMAT_R16_UINT;
  // !OPTTODO HASH STRING
  CRUDE_ASSERT( false );
  return VK_FORMAT_UNDEFINED;
}

crude_gfx_vertex_component_format
crude_gfx_to_vertex_component_format
( 
  _In_ char const                                         *format
)
{
  if ( strcmp( format, "FLOAT" ) == 0 ) return CRUDE_GFX_VERTEX_COMPONENT_FORMAT_FLOAT;
  if ( strcmp( format, "FLOAT2" ) == 0 ) return CRUDE_GFX_VERTEX_COMPONENT_FORMAT_FLOAT2;
  if ( strcmp( format, "FLOAT3" ) == 0 ) return CRUDE_GFX_VERTEX_COMPONENT_FORMAT_FLOAT3;
  if ( strcmp( format, "FLOAT4" ) == 0 ) return CRUDE_GFX_VERTEX_COMPONENT_FORMAT_FLOAT4;
  if ( strcmp( format, "FLOAT4" ) == 0 ) return CRUDE_GFX_VERTEX_COMPONENT_FORMAT_FLOAT4;
  if ( strcmp( format, "BYTE" ) == 0 ) return CRUDE_GFX_VERTEX_COMPONENT_FORMAT_BYTE;
  if ( strcmp( format, "UBYTE" ) == 0 ) return CRUDE_GFX_VERTEX_COMPONENT_FORMAT_UBYTE;
  if ( strcmp( format, "UINT" ) == 0 ) return CRUDE_GFX_VERTEX_COMPONENT_FORMAT_UINT;
  if ( strcmp( format, "UBYTE4N" ) == 0 ) return CRUDE_GFX_VERTEX_COMPONENT_FORMAT_UBYTE4N;
  // !OPTTODO HASH STRING
  CRUDE_ASSERT( false );
  return CRUDE_GFX_VERTEX_COMPONENT_FORMAT_COUNT;
}

VkPrimitiveTopology
crude_gfx_string_to_vk_primitive_topology
( 
  _In_ char const                                         *format
)
{
  if ( strcmp( format, "TRIANGLE_LIST" ) == 0 ) return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
  if ( strcmp( format, "LINE_LIST" ) == 0 ) return VK_PRIMITIVE_TOPOLOGY_LINE_LIST;
  // !OPTTODO HASH STRING
  CRUDE_ASSERT( false );
  return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
}

char const*
crude_gfx_vk_shader_stage_to_defines
(
  _In_ VkShaderStageFlagBits                              value
)
{
  switch ( value )
  {
    case VK_SHADER_STAGE_VERTEX_BIT:          return "CRUDE_STAGE_VERTEX";
    case VK_SHADER_STAGE_FRAGMENT_BIT:        return "CRUDE_STAGE_FRAGMENT";
    case VK_SHADER_STAGE_COMPUTE_BIT:         return "CRUDE_STAGE_COMPUTE";
    case VK_SHADER_STAGE_MESH_BIT_EXT:        return "CRUDE_STAGE_MESH";
    case VK_SHADER_STAGE_TASK_BIT_EXT:        return "CRUDE_STAGE_TASK";
    case VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR: return "CRUDE_CLOSEST_HIT";
    case VK_SHADER_STAGE_RAYGEN_BIT_KHR:      return "CRUDE_RAYGEN";
    case VK_SHADER_STAGE_MISS_BIT_KHR:        return "CRUDE_MISS";
  }
   return "";
}

char const*
crude_gfx_vk_shader_stage_to_compiler_extension
(
  _In_ VkShaderStageFlagBits                               value
)
{
  switch ( value )
  {
    case VK_SHADER_STAGE_VERTEX_BIT:          return "vert";
    case VK_SHADER_STAGE_MESH_BIT_EXT:        return "mesh";
    case VK_SHADER_STAGE_TASK_BIT_EXT:        return "task";
    case VK_SHADER_STAGE_FRAGMENT_BIT:        return "frag";
    case VK_SHADER_STAGE_COMPUTE_BIT:         return "comp";
    case VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR: return "rchit";
    case VK_SHADER_STAGE_RAYGEN_BIT_KHR:      return "rgen";
    case VK_SHADER_STAGE_MISS_BIT_KHR:        return "rmiss";
  }
   return "";
}

char const*
crude_gfx_resource_state_to_name
(
  _In_ crude_gfx_resource_state                            value
)
{
  switch ( value )
  {
    case CRUDE_GFX_RESOURCE_STATE_UNDEFINED:                          return "Undefined";
    case CRUDE_GFX_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER:         return "Vertex And Constant";
    case CRUDE_GFX_RESOURCE_STATE_INDEX_BUFFER:                       return "Index Buffer";
    case CRUDE_GFX_RESOURCE_STATE_RENDER_TARGET:                      return "Render Target";
    case CRUDE_GFX_RESOURCE_STATE_UNORDERED_ACCESS:                   return "UAV";
    case CRUDE_GFX_RESOURCE_STATE_DEPTH_WRITE:                        return "Depth Write";
    case CRUDE_GFX_RESOURCE_STATE_DEPTH_READ:                         return "Depth Read";
    case CRUDE_GFX_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE:          return "Non Pixel Shader Resource";
    case CRUDE_GFX_RESOURCE_STATE_PIXEL_SHADER_RESOURCE:              return "Pixel Shader Resource";
    case CRUDE_GFX_RESOURCE_STATE_SHADER_RESOURCE:                    return "Shader Resource";
    case CRUDE_GFX_RESOURCE_STATE_STREAM_OUT:                         return "Stream Out";
    case CRUDE_GFX_RESOURCE_STATE_INDIRECT_ARGUMENT:                  return "Indirect Argument";
    case CRUDE_GFX_RESOURCE_STATE_COPY_DEST:                          return "Copy Dest";
    case CRUDE_GFX_RESOURCE_STATE_COPY_SOURCE:                        return "Copy Source";
    case CRUDE_GFX_RESOURCE_STATE_GENERIC_READ:                       return "Generic Read";
    case CRUDE_GFX_RESOURCE_STATE_PRESENT:                            return "Present";
    case CRUDE_GFX_RESOURCE_STATE_COMMON:                             return "Common";
    case CRUDE_GFX_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE:  return "Raytracing";
    case CRUDE_GFX_RESOURCE_STATE_SHADING_RATE_SOURCE:                return "Shading Rate";
  }
  return "UnknownState";
}

crude_gfx_render_pass_operation
crude_gfx_string_to_render_pass_operation
(
  _In_ char const                                         *op
)
{
  if ( strcmp( op, "clear" ) == 0 )
  {
    return CRUDE_GFX_RENDER_PASS_OPERATION_CLEAR;
  }
  else if ( strcmp( op, "load" ) == 0 )
  {
    return CRUDE_GFX_RENDER_PASS_OPERATION_LOAD;
  }
  
  CRUDE_ASSERT( false );
  return CRUDE_GFX_RENDER_PASS_OPERATION_DONT_CARE;
}