#pragma once

#include <vulkan/vulkan.h>
#include <vma_usage.h>
#include <spirv_reflect.h>

#include <core/math.h>
#include <core/resource_pool.h>

typedef uint32 crude_resource_handle;

typedef struct crude_buffer_handle
{
  crude_resource_handle index;
} crude_buffer_handle;

typedef struct crude_texture_handle
{
  crude_resource_handle index;
} crude_texture_handle;

typedef struct crude_descriptor_set_layout_handle
{
  crude_resource_handle index;
} crude_descriptor_set_layout_handle;

typedef struct crude_descriptor_set_handle
{
  crude_resource_handle index;
} crude_descriptor_set_handle;

typedef struct crude_sampler_handle
{
  crude_resource_handle index;
} crude_sampler_handle;

typedef struct crude_shader_state_handle
{
  crude_resource_handle index;
} crude_shader_state_handle;

typedef struct crude_render_pass_handle
{
  crude_resource_handle index;
} crude_render_pass_handle;

typedef struct crude_pipeline_handle
{
  crude_resource_handle index;
} crude_pipeline_handle;

#define CRUDE_MAX_SWAPCHAIN_IMAGES          3
#define CRUDE_MAX_IMAGE_OUTPUTS             8          
#define CRUDE_MAX_DESCRIPTOR_SET_LAYOUTS    8 
#define CRUDE_MAX_SHADER_STAGES             5      
#define CRUDE_MAX_DESCRIPTORS_PER_SET       16
#define CRUDE_MAX_VERTEX_STREAMS            16
#define CRUDE_MAX_VERTEX_ATTRIBUTES         16
#define CRUDE_UBO_ALIGNMENT                 256
#define CRUDE_MAX_SET_COUNT                 32

typedef enum crude_resource_usage_type 
{
  CRUDE_RESOURCE_USAGE_TYPE_IMMUTABLE, 
  CRUDE_RESOURCE_USAGE_TYPE_DYNAMIC,
  CRUDE_RESOURCE_USAGE_TYPE_STREAM,
  CRUDE_RESOURCE_USAGE_TYPE_COUNT
} crude_resource_usage_type;

typedef enum crude_texture_type
{
  CRUDE_TEXTURE_TYPE_TEXTURE_1D,
  CRUDE_TEXTURE_TYPE_TEXTURE_2D,
  CRUDE_TEXTURE_TYPE_TEXTURE_3D,
  CRUDE_TEXTURE_TYPE_TEXTURE_1D_ARRAY,
  CRUDE_TEXTURE_TYPE_TEXTURE_2D_ARRAY,
  CRUDE_TEXTURE_TYPE_TEXTURE_CUBE_ARRAY,
  CRUDE_TEXTURE_TYPE_TEXTURE_COUNT,
 } crude_texture_type;

typedef enum crude_color_write_enabled
{
  CRUDE_COLOR_WRITE_ENABLED_RED,
  CRUDE_COLOR_WRITE_ENABLED_GREEN,
  CRUDE_COLOR_WRITE_ENABLED_BLUE,
  CRUDE_COLOR_WRITE_ENABLED_ALPHA,
  CRUDE_COLOR_WRITE_ENABLED_ALL,
  CRUDE_COLOR_WRITE_ENABLED_COUNT
} crude_color_write_enabled;

typedef enum crude_fill_mode
{
  CRUDE_FILL_MODE_WIREFRAME,
  CRUDE_FILL_MODE_SOLID,
  CRUDE_FILL_MODE_POINT,
  CRUDE_FILL_MODE_COUNT
} crude_fill_mode;

typedef enum crude_render_pass_type
{
  CRUDE_RENDER_PASS_TYPE_GEOMETRY,
  CRUDE_RENDER_PASS_TYPE_SWAPCHAIN,
  CRUDE_RENDER_PASS_TYPE_COMPUTE
} crude_render_pass_type;

typedef enum crude_render_pass_operation
{
  CRUDE_RENDER_PASS_OPERATION_DONT_CARE,
  CRUDE_RENDER_PASS_OPERATION_LOAD,
  CRUDE_RENDER_PASS_OPERATION_CLEAR,
  CRUDE_RENDER_PASS_OPERATION_COUNT
} crude_render_pass_operation;

typedef enum crude_resource_deletion_type
{
  CRUDE_RESOURCE_DELETION_TYPE_BUFFER,
  CRUDE_RESOURCE_DELETION_TYPE_TEXTURE,
  CRUDE_RESOURCE_DELETION_TYPE_PIPELINE,
  CRUDE_RESOURCE_DELETION_TYPE_SAMPLER,
  CRUDE_RESOURCE_DELETION_TYPE_DESCRIPTOR_SET_LAYOUT,
  CRUDE_RESOURCE_DELETION_TYPE_DESCRIPTOR_SET,
  CRUDE_RESOURCE_DELETION_TYPE_RENDER_PASS,
  CRUDE_RESOURCE_DELETION_TYPE_SHADER_STATE,
  CRUDE_RESOURCE_DELETION_TYPE_COUNT
} crude_resource_deletion_type;

typedef enum crude_queue_type
{
  CRUDE_QUEUE_TYPE_GRAPHICS,
  CRUDE_QUEUE_TYPE_COMPUTE,
  CRUDE_QUEUE_TYPE_COPY_TRANSFER,
  CRUDE_QUEUE_TYPE_COUNT
} crude_queue_type;

typedef enum crude_texture_flag
{
  CRUDE_TEXTURE_FLAG_DEFAULT,
  CRUDE_TEXTURE_FLAG_RENDER_TARGET,
  CRUDE_TEXTURE_FLAG_COMPUTE,
  CRUDE_TEXTURE_FLAG_COUNT,
} crude_texture_flag;

typedef enum crude_texture_mask
{
  CRUDE_TEXTURE_MASK_DEFAULT = 1 << 0,
  CRUDE_TEXTURE_MASK_RENDER_TARGET = 1 << 1,
  CRUDE_TEXTURE_MASK_COMPUTE = 1 << 2,
} crude_texture_mask;

typedef enum crude_vertex_component_format
{
  CRUDE_VERTEX_COMPONENT_FORMAT_FLOAT,
  CRUDE_VERTEX_COMPONENT_FORMAT_FLOAT2,
  CRUDE_VERTEX_COMPONENT_FORMAT_FLOAT3,
  CRUDE_VERTEX_COMPONENT_FORMAT_FLOAT4,
  CRUDE_VERTEX_COMPONENT_FORMAT_MAT4,
  CRUDE_VERTEX_COMPONENT_FORMAT_BYTE,
  CRUDE_VERTEX_COMPONENT_FORMAT_BYTE4N,
  CRUDE_VERTEX_COMPONENT_FORMAT_UBYTE,
  CRUDE_VERTEX_COMPONENT_FORMAT_UBYTE4N,
  CRUDE_VERTEX_COMPONENT_FORMAT_SHORT2,
  CRUDE_VERTEX_COMPONENT_FORMAT_SHORT2N,
  CRUDE_VERTEX_COMPONENT_FORMAT_SHORT4,
  CRUDE_VERTEX_COMPONENT_FORMAT_SHORT4N,
  CRUDE_VERTEX_COMPONENT_FORMAT_UINT,
  CRUDE_VERTEX_COMPONENT_FORMAT_UINT2,
  CRUDE_VERTEX_COMPONENT_FORMAT_UINT4,
  CRUDE_VERTEX_COMPONENT_FORMAT_COUNT
} crude_vertex_component_format;
    
typedef enum crude_vertex_input_rate
{
  CRUDE_VERTEX_INPUT_RATE_PER_VERTEX,
  CRUDE_VERTEX_INPUT_RATE_PER_INSTANCE,
  CRUDE_VERTEX_INPUT_RATE_COUNT,
} crude_vertex_input_rate;

typedef struct crude_rect2d_int
{
  int16                                x;
  int16                                y;
  uint16                               width;
  uint16                               height;
} crude_rect2d_int;

typedef struct crude_viewport
{
  crude_rect2d_int                     rect;
  float32                              min_depth;
  float32                              max_depth;
} crude_viewport;

typedef struct crude_stencil_operation_state
{
  VkStencilOp                          fail;
  VkStencilOp                          pass;
  VkStencilOp                          depth_fail;
  VkCompareOp                          compare;
  uint32                               compare_mask;
  uint32                               write_mask;
  uint32                               reference;
} crude_stencil_operation_state;

typedef struct crude_blend_state
{
  VkBlendFactor                        source_color;
  VkBlendFactor                        destination_color;
  VkBlendOp                            color_operation;
  VkBlendFactor                        source_alpha;
  VkBlendFactor                        destination_alpha;
  VkBlendOp                            alpha_operation;
  crude_color_write_enabled            color_write_mask;
  uint8                                blend_enabled   : 1;
  uint8                                separate_blend  : 1;
  uint8                                pad             : 6;
} crude_blend_state;


typedef struct crude_sampler_creation
{
  VkFilter                             min_filter;
  VkFilter                             mag_filter;
  VkSamplerMipmapMode                  mip_filter;
  VkSamplerAddressMode                 address_mode_u;
  VkSamplerAddressMode                 address_mode_v;
  VkSamplerAddressMode                 address_mode_w;
  const char*                          name;
} crude_sampler_creation;

typedef struct crude_buffer_creation
{
  VkBufferUsageFlags                   type_flags;
  crude_resource_usage_type            usage;
  uint32                               size;
  void                                *initial_data;
  char const                          *name;
} crude_buffer_creation;

typedef struct crude_render_pass_creation
{
  uint16                               num_render_targets;
  crude_render_pass_type               type;
  crude_texture_handle                 output_textures[ CRUDE_MAX_IMAGE_OUTPUTS ];
  crude_texture_handle                 depth_stencil_texture;
  float32                              scale_x;
  float32                              scale_y;
  uint8                                resize;
  crude_render_pass_operation          color_operation;
  crude_render_pass_operation          depth_operation;
  crude_render_pass_operation          stencil_operation;
  char const                          *name;
} crude_render_pass_creation;

typedef struct crude_depth_stencil_creation
{
  crude_stencil_operation_state        front;
  crude_stencil_operation_state        back;
  VkCompareOp                          depth_comparison;
  uint8                                depth_enable        : 1;
  uint8                                depth_write_enable  : 1;
  uint8                                stencil_enable      : 1;
  uint8                                pad                 : 5;
} crude_depth_stencil_creation;

typedef struct crude_blend_state_creation
{
  crude_blend_state                    blend_states[ CRUDE_MAX_IMAGE_OUTPUTS ];
  uint32                               active_states;
} crude_blend_state_creation;

typedef struct crude_rasterization_creation
{
  VkCullModeFlagBits                   cull_mode;
  VkFrontFace                          front;
  crude_fill_mode                      fill;
} crude_rasterization_creation;

typedef struct crude_texture_creation
{
  void                                *initial_data;
  uint16                               width;
  uint16                               height;
  uint16                               depth;
  uint8                                mipmaps;
  uint8                                flags;
  VkFormat                             format;
  crude_texture_type                   type;
  char const                          *name;
} crude_texture_creation;

typedef struct crude_render_pass_output
{
  VkFormat                             color_formats[ CRUDE_MAX_IMAGE_OUTPUTS ];
  VkFormat                             depth_stencil_format;
  uint32                               num_color_formats;
  crude_render_pass_operation          color_operation;
  crude_render_pass_operation          depth_operation;
  crude_render_pass_operation          stencil_operation;
} crude_render_pass_output;

typedef struct crude_vertex_attribute
{
  uint16                               location;
  uint16                               binding;
  uint32                               offset;
  crude_vertex_component_format        format;
} crude_vertex_attribute;

typedef struct crude_vertex_stream
{
  uint16                               binding;
  uint16                               stride;
  crude_vertex_input_rate              input_rate;
} crude_vertex_stream;

typedef struct crude_vertex_input_creation
{
  uint32                               num_vertex_streams;
  uint32                               num_vertex_attributes;
  crude_vertex_stream                  vertex_streams[ CRUDE_MAX_VERTEX_STREAMS ];
  crude_vertex_attribute               vertex_attributes[ CRUDE_MAX_VERTEX_ATTRIBUTES ];
} crude_vertex_input_creation;

typedef struct crude_shader_stage
{
  char const                          *code;
  uint32                               code_size;
  VkShaderStageFlagBits                type;
} crude_shader_stage; 

typedef struct crude_shader_state_creation
{
  crude_shader_stage                   stages[ CRUDE_MAX_SHADER_STAGES ];
  char const                          *name;
  uint32                               stages_count;
  uint32                               spv_input;
} crude_shader_state_creation;

typedef struct crude_viewport_state
{
  uint32                               num_viewports;
  uint32                               num_scissors;
  crude_viewport                      *viewport;
  crude_rect2d_int                    *scissors;
} crude_viewport_state;

typedef struct crude_descriptor_set_layout_binding
{
  VkDescriptorType                     type;
  uint16                               start;
  uint16                               count;
  char const                          *name;
} crude_descriptor_set_layout_binding;

typedef struct crude_descriptor_set_layout_creation
{
  crude_descriptor_set_layout_binding  bindings[ CRUDE_MAX_DESCRIPTORS_PER_SET ];
  uint32                               num_bindings;
  uint32                               set_index;
  char const                          *name;
} crude_descriptor_set_layout_creation;

typedef struct crude_pipeline_creation
{
  crude_rasterization_creation         rasterization;
  crude_depth_stencil_creation         depth_stencil;
  crude_blend_state_creation           blend_state;
  crude_vertex_input_creation          vertex_input;
  crude_shader_state_creation          shaders;
  crude_render_pass_output             render_pass;
  crude_viewport_state const          *viewport;
  char const                          *name;
} crude_pipeline_creation;

typedef struct crude_descriptor_set_creation
{
  crude_resource_handle                resources[ CRUDE_MAX_DESCRIPTORS_PER_SET ];
  crude_sampler_handle                 samplers[ CRUDE_MAX_DESCRIPTORS_PER_SET ];
  uint16                               bindings[ CRUDE_MAX_DESCRIPTORS_PER_SET ];
  crude_descriptor_set_layout_handle   layout;
  uint32                               num_resources;
  char const                          *name;
} crude_descriptor_set_creation;

typedef struct crude_shader_descriptor_parse
{
  uint32                               sets_count;
  crude_descriptor_set_layout_creation sets[ CRUDE_MAX_SET_COUNT ];
} crude_shader_descriptor_parse;

typedef struct crude_shader_parse
{
  crude_shader_descriptor_parse        descriptor;
} crude_shader_parse;

typedef struct crude_buffer
{
  VkBuffer                             vk_buffer;
  VmaAllocation                        vma_allocation;
  VkDeviceMemory                       vk_device_memory;
  VkDeviceSize                         vk_device_size;
  VkBufferUsageFlags                   type_flags;
  crude_resource_usage_type            usage;
  uint32                               size;
  uint32                               global_offset;
  crude_buffer_handle                  handle;
  crude_buffer_handle                  parent_buffer;
  char const                          *name;
} crude_buffer;

typedef struct crude_sampler
{
  VkSampler                            vk_sampler;
  VkFilter                             min_filter;
  VkFilter                             mag_filter;
  VkSamplerMipmapMode                  mip_filter;
  VkSamplerAddressMode                 address_mode_u;
  VkSamplerAddressMode                 address_mode_v;
  VkSamplerAddressMode                 address_mode_w;
  char const                          *name;
} crude_sampler;

typedef struct crude_texture
{
  VkImage                              vk_image;
  VkImageView                          vk_image_view;
  VkFormat                             vk_format;
  VkImageLayout                        vk_image_layout;
  VmaAllocation                        vma_allocation;
  uint16                               width;
  uint16                               height;
  uint16                               depth;
  uint8                                mipmaps;
  uint8                                flags;
  crude_texture_handle                 handle;
  crude_texture_type                   type;
  crude_sampler                       *sampler;
  char const                          *name;
} crude_texture;

typedef struct crude_descriptor_binding
{
  VkDescriptorType                     type;
  uint16                               start;
  uint16                               count;
  uint16                               set;
  const char                          *name;
} crude_descriptor_binding;

typedef struct crude_descriptor_set_layout
{
  VkDescriptorSetLayout                vk_descriptor_set_layout;
  VkDescriptorSetLayoutBinding        *vk_binding;
  crude_descriptor_binding            *bindings;
  uint16                               num_bindings;
  uint16                               set_index;
  crude_descriptor_set_layout_handle   handle;
} crude_descriptor_set_layout;

typedef struct crude_descriptor_set
{
  VkDescriptorSet                      vk_descriptor_set;
  crude_resource_handle                resources[ CRUDE_MAX_DESCRIPTORS_PER_SET ];
  crude_sampler_handle                 samplers[ CRUDE_MAX_DESCRIPTORS_PER_SET ];
  uint16                               bindings[ CRUDE_MAX_DESCRIPTORS_PER_SET ];
  crude_descriptor_set_layout const   *layout;
  uint32                               num_resources;
} crude_descriptor_set;

typedef struct crude_pipeline
{
  VkPipeline                           vk_pipeline;
  VkPipelineLayout                     vk_pipeline_layout;
  VkPipelineBindPoint                  vk_bind_point;
  crude_shader_state_handle            shader_state;
  crude_descriptor_set_layout const   *descriptor_set_layout[ CRUDE_MAX_DESCRIPTOR_SET_LAYOUTS ];
  crude_descriptor_set_layout_handle   descriptor_set_layout_handle[ CRUDE_MAX_DESCRIPTOR_SET_LAYOUTS ];
  uint32                               num_active_layouts;
  crude_depth_stencil_creation         depth_stencil;
  crude_blend_state_creation           blend_state;
  crude_rasterization_creation         rasterization;
  crude_pipeline_handle                handle;
  bool                                 graphics_pipeline;
} crude_pipeline;

typedef struct crude_render_pass
{
  VkRenderPass                         vk_render_pass;
  VkFramebuffer                        vk_frame_buffer;
  crude_render_pass_output             output;  
  crude_texture_handle                 output_textures[ CRUDE_MAX_IMAGE_OUTPUTS ];
  crude_texture_handle                 output_depth;
  crude_render_pass_type               type;
  float32                              scale_x;
  float32                              scale_y;
  uint16                               width;
  uint16                               height;
  uint16                               dispatch_x;
  uint16                               dispatch_y;
  uint16                               dispatch_z;
  uint8                                resize;
  uint8                                num_render_targets;
  char const                          *name;
} crude_render_pass;

typedef struct crude_shader_state
{
  VkPipelineShaderStageCreateInfo      shader_stage_info[ CRUDE_MAX_SHADER_STAGES ];
  const char                          *name;
  uint32                               active_shaders;
  bool                                 graphics_pipeline;
  crude_shader_parse                   parse;
} crude_shader_state;

typedef struct crude_buffer_description
{
  void                                *native_handle;
  char const                          *name;
  VkBufferUsageFlags                   type_flags;
  crude_resource_usage_type            usage;
  uint32                               size;
  crude_buffer_handle                  parent_handle;
} crude_buffer_description;

typedef struct crude_map_buffer_parameters
{
  crude_buffer_handle                  buffer;
  uint32                               offset;
  uint32                               size;
} crude_map_buffer_parameters;

typedef struct crude_resource_update
{
  crude_resource_deletion_type         type;
  crude_resource_handle                handle;
  uint32                               current_frame;
} crude_resource_update;

typedef struct crude_shader_frame_constants
{
  crude_float4x4a world_to_view;
  crude_float4x4a view_to_clip;
} crude_shader_frame_constants;

typedef struct crude_shader_mesh_constants
{
  crude_float4x4a  modelToWorld;
  crude_uint4a     textures;
  crude_float4a    base_color_factor;
  crude_float3a    metallic_roughness_occlusion_factor;
  crude_float1a    alpha_cutoff;
  crude_uint1a     flags;
} crude_shader_mesh_constants;

CRUDE_API void
crude_reset_render_pass_output
( 
  _In_ crude_render_pass_output *output
);

CRUDE_API VkImageType
crude_to_vk_image_type
( 
  _In_ crude_texture_type type
);

CRUDE_API bool
crude_has_depth_or_stencil
( 
  _In_ VkFormat value
);

CRUDE_API bool
crude_has_depth
( 
  _In_ VkFormat value
);

CRUDE_API VkImageViewType
crude_to_vk_image_view_type
( 
  _In_ crude_texture_type type
);

CRUDE_API VkFormat
crude_to_vk_vertex_format
( 
  _In_ crude_vertex_component_format value
);