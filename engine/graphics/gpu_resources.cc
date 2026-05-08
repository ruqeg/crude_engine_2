#include <engine/core/assert.h>
#include <engine/core/hashmapstr.h>
#include <engine/graphics/rhi.h>

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
  creation.topology = CRUDE_GFX_RHI_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
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
    .depth_stencil_format = CRUDE_GFX_RHI_FORMAT_UNDEFINED,
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
  creation.format  = CRUDE_GFX_RHI_FORMAT_UNDEFINED;
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
    output.color_formats[ i ] = CRUDE_GFX_RHI_FORMAT_UNDEFINED;
    output.color_final_layouts[ i ] = CRUDE_GFX_RHI_IMAGE_LAYOUT_UNDEFINED;
    output.color_operations[ i ] = CRUDE_GFX_RENDER_PASS_OPERATION_DONT_CARE;
  }
  output.depth_stencil_format = CRUDE_GFX_RHI_FORMAT_UNDEFINED;
  output.depth_operation = output.stencil_operation = CRUDE_GFX_RENDER_PASS_OPERATION_DONT_CARE;

  return output;
}

void
crude_gfx_render_pass_output_add_color
(
  _In_ crude_gfx_render_pass_output                       *output,
  _In_ crude_gfx_rhi_format                                    color_format,
  _In_ crude_gfx_rhi_image_layout                              color_final_layout,
  _In_ crude_gfx_render_pass_operation                     color_operation
)
{
  output->color_formats[ output->num_color_formats ] = CRUDE_GFX_RHI_FORMAT_UNDEFINED;
  output->color_final_layouts[ output->num_color_formats ] = CRUDE_GFX_RHI_IMAGE_LAYOUT_UNDEFINED;
  output->color_operations[ output->num_color_formats ] = CRUDE_GFX_RENDER_PASS_OPERATION_DONT_CARE;
  ++output->num_color_formats;
}

void
crude_gfx_render_pass_output_set_depth
(
  _In_ crude_gfx_render_pass_output                       *output,
  _In_ crude_gfx_rhi_format                                    depth_stencil_format,
  _In_ crude_gfx_rhi_image_layout                              depth_stencil_final_layout,
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
  _In_ crude_gfx_rhi_acceleration_structure                acceleration_structure,
  _In_ uint16                                              binding
)
{
#if CRUDE_GFX_RAY_TRACING_ENABLED
  creation->acceleration_structure = acceleration_structure;
  creation->bindings[ creation->num_resources++ ] = binding;
#else
  CRUDE_ASSERTM( CRUDE_CHANNEL_GRAPHICS, false, "Can't add acceleration structure to descriptor set. CRUDE_GRAPHICS_RAY_TRACING_ENABLED wasn't enabled" );
#endif /* CRUDE_GFX_RAY_TRACING_ENABLED */
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
  _In_ crude_gfx_rhi_shader_stage_flag_bits                    type
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
  uint32 index = CRUDE_HASHMAPSTR_GET( technique->name_to_pass_index, name )->value;
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
  uint32 index = CRUDE_HASHMAPSTR_GET( technique_pass->name_hashed_to_descriptor_index, name )->value;
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
crude_gfx_rhi_image_type
crude_gfx_texture_type_to_image_type
(
  _In_ crude_gfx_texture_type                              type
)
{
  static crude_gfx_rhi_image_type s_target[ CRUDE_GFX_TEXTURE_TYPE_TEXTURE_COUNT ] =
  {
    CRUDE_GFX_RHI_IMAGE_TYPE_1D, CRUDE_GFX_RHI_IMAGE_TYPE_2D, CRUDE_GFX_RHI_IMAGE_TYPE_3D,
    CRUDE_GFX_RHI_IMAGE_TYPE_1D, CRUDE_GFX_RHI_IMAGE_TYPE_2D, CRUDE_GFX_RHI_IMAGE_TYPE_3D
  };
  return s_target[ type ];
}

crude_gfx_rhi_image_view_type
crude_gfx_texture_type_to_image_view_type
(
  _In_ crude_gfx_texture_type                              type
)
{
  static crude_gfx_rhi_image_view_type s_data[ CRUDE_GFX_TEXTURE_TYPE_TEXTURE_COUNT ] =
  {
    CRUDE_GFX_RHI_IMAGE_VIEW_TYPE_1D, CRUDE_GFX_RHI_IMAGE_VIEW_TYPE_2D, CRUDE_GFX_RHI_IMAGE_VIEW_TYPE_3D,
    CRUDE_GFX_RHI_IMAGE_VIEW_TYPE_1D_ARRAY, CRUDE_GFX_RHI_IMAGE_VIEW_TYPE_2D_ARRAY, CRUDE_GFX_RHI_IMAGE_VIEW_TYPE_CUBE_ARRAY
  };
  return s_data[ type ];
}

crude_gfx_rhi_format
crude_gfx_to_vertex_format
(
  _In_ crude_gfx_vertex_component_format                   value
)
{
  static crude_gfx_rhi_format s_vk_vertex_formats[ CRUDE_GFX_VERTEX_COMPONENT_FORMAT_COUNT ] =
  {
    CRUDE_GFX_RHI_FORMAT_R32_SFLOAT,
    CRUDE_GFX_RHI_FORMAT_R32G32_SFLOAT,
    CRUDE_GFX_RHI_FORMAT_R32G32B32_SFLOAT,
    CRUDE_GFX_RHI_FORMAT_R32G32B32A32_SFLOAT,
    CRUDE_GFX_RHI_FORMAT_R32G32B32A32_SFLOAT,
    CRUDE_GFX_RHI_FORMAT_R8_SINT,
    CRUDE_GFX_RHI_FORMAT_R8G8B8A8_SNORM,
    CRUDE_GFX_RHI_FORMAT_R8_UINT,
    CRUDE_GFX_RHI_FORMAT_R8G8B8A8_UINT,
    CRUDE_GFX_RHI_FORMAT_R16G16_SINT,
    CRUDE_GFX_RHI_FORMAT_R16G16_SNORM,
    CRUDE_GFX_RHI_FORMAT_R16G16B16A16_SINT,
    CRUDE_GFX_RHI_FORMAT_R16G16B16A16_SNORM,
    CRUDE_GFX_RHI_FORMAT_R32_UINT,
    CRUDE_GFX_RHI_FORMAT_R32G32_UINT,
    CRUDE_GFX_RHI_FORMAT_R32G32B32A32_UINT
  };

  return s_vk_vertex_formats[ value ];
}

crude_gfx_rhi_format
crude_gfx_string_to_format
(
  _In_ char const                                         *format
)
{
  if ( strcmp( format, "FORMAT_R16G16B16A16_SFLOAT" ) == 0 ) return CRUDE_GFX_RHI_FORMAT_R16G16B16A16_SFLOAT;
  if ( strcmp( format, "FORMAT_D32_SFLOAT" ) == 0 ) return CRUDE_GFX_RHI_FORMAT_D32_SFLOAT;
  if ( strcmp( format, "FORMAT_D16_UNORM" ) == 0 ) return CRUDE_GFX_RHI_FORMAT_D16_UNORM;
  if ( strcmp( format, "FORMAT_R8G8B8_SRGB" ) == 0 ) return CRUDE_GFX_RHI_FORMAT_R8G8B8_SRGB;
  if ( strcmp( format, "FORMAT_R8G8B8A8_SRGB" ) == 0 ) return CRUDE_GFX_RHI_FORMAT_R8G8B8A8_SRGB;
  if ( strcmp( format, "FORMAT_R8G8B8A8_UNORM" ) == 0 ) return CRUDE_GFX_RHI_FORMAT_R8G8B8A8_UNORM;
  if ( strcmp( format, "FORMAT_R16G16_SNORM" ) == 0 ) return CRUDE_GFX_RHI_FORMAT_R16G16_SNORM;
  if ( strcmp( format, "FORMAT_R16G16_UNORM" ) == 0 ) return CRUDE_GFX_RHI_FORMAT_R16G16_UNORM;
  if ( strcmp( format, "FORMAT_R16G16B16A16_SNORM" ) == 0 ) return CRUDE_GFX_RHI_FORMAT_R16G16B16A16_SNORM;
  if ( strcmp( format, "FORMAT_R16G16B16A16_UNORM" ) == 0 ) return CRUDE_GFX_RHI_FORMAT_R16G16B16A16_UNORM;
  if ( strcmp( format, "FORMAT_R16_UINT" ) == 0 ) return CRUDE_GFX_RHI_FORMAT_R16_UINT;
  CRUDE_ASSERT( false );
  return CRUDE_GFX_RHI_FORMAT_UNDEFINED;
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

crude_gfx_rhi_primitive_topology
crude_gfx_string_to_primitive_topology
( 
  _In_ char const                                         *format
)
{
  if ( strcmp( format, "TRIANGLE_LIST" ) == 0 ) return CRUDE_GFX_RHI_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
  if ( strcmp( format, "LINE_LIST" ) == 0 ) return CRUDE_GFX_RHI_PRIMITIVE_TOPOLOGY_LINE_LIST;
  CRUDE_ASSERT( false );
  return CRUDE_GFX_RHI_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
}

char const*
crude_gfx_shader_stage_to_defines
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

char const*
crude_gfx_shader_stage_to_compiler_extension
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