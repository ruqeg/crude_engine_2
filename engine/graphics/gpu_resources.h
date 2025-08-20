#pragma once

#include <vulkan/vulkan.h>
#include <vma_usage.h>
#include <spirv_reflect.h>

#include <core/math.h>
#include <core/alias.h>
#include <core/array.h>
#include <core/resource_pool.h>

/************************************************
 *
 * GPU Resoruces Constants
 * 
 ***********************************************/
#define CRUDE_GFX_MAX_SWAPCHAIN_IMAGES                     3
#define CRUDE_GFX_MAX_IMAGE_OUTPUTS                        8
#define CRUDE_GFX_MAX_DESCRIPTOR_SET_LAYOUTS               8
#define CRUDE_GFX_MAX_SHADER_STAGES                        5      
#define CRUDE_GFX_MAX_DESCRIPTORS_PER_SET                  64
#define CRUDE_GFX_UBO_ALIGNMENT                            256
#define CRUDE_GFX_MAX_SET_COUNT                            32
#define CRUDE_GFX_MAX_BINDLESS_RESOURCES                   1024
#define CRUDE_GFX_BINDLESS_DESCRIPTOR_SET_INDEX            0u
#define CRUDE_GFX_MATERIAL_DESCRIPTOR_SET_INDEX            1u
#define CRUDE_GFX_BINDLESS_TEXTURE_BINDING                 10
#define CRUDE_GFX_MAX_PUSH_CONSTANTS                       1u

/************************************************
 *
 * GPU Resoruces Handles
 * 
 ***********************************************/
typedef uint32 crude_gfx_resource_index;

typedef struct crude_gfx_buffer_handle
{
  crude_gfx_resource_index                                 index;
} crude_gfx_buffer_handle;

typedef struct crude_gfx_texture_handle
{
  crude_gfx_resource_index                                 index;
} crude_gfx_texture_handle;

typedef struct crude_gfx_descriptor_set_layout_handle
{
  crude_gfx_resource_index                                 index;
} crude_gfx_descriptor_set_layout_handle;

typedef struct crude_gfx_descriptor_set_handle
{
  crude_gfx_resource_index                                 index;
} crude_gfx_descriptor_set_handle;

typedef struct crude_gfx_sampler_handle
{
  crude_gfx_resource_index                                 index;
} crude_gfx_sampler_handle;

typedef struct crude_gfx_shader_state_handle
{
  crude_gfx_resource_index                                 index;
} crude_gfx_shader_state_handle;

typedef struct crude_gfx_render_pass_handle
{
  crude_gfx_resource_index                                 index;
} crude_gfx_render_pass_handle;

typedef struct crude_gfx_pipeline_handle
{
  crude_gfx_resource_index                                 index;
} crude_gfx_pipeline_handle;

typedef struct crude_gfx_framebuffer_handle
{
  crude_gfx_resource_index                                 index;
} crude_gfx_framebuffer_handle;

/************************************************
 *
 * Invalid GPU Resoruces Handles
 * 
 ***********************************************/
#define CRUDE_GFX_BUFFER_HANDLE_INVALID                    ( CRUDE_COMPOUNT( crude_gfx_buffer_handle, { CRUDE_RESOURCE_INDEX_INVALID } ) )
#define CRUDE_GFX_TEXTURE_HANDLE_INVALID                   ( CRUDE_COMPOUNT( crude_gfx_texture_handle, { CRUDE_RESOURCE_INDEX_INVALID } ) )
#define CRUDE_GFX_SAMPLER_HANDLE_INVALID                   ( CRUDE_COMPOUNT( crude_gfx_sampler_handle, { CRUDE_RESOURCE_INDEX_INVALID } ) )
#define CRUDE_GFX_PIPELINE_HANDLE_INVALID                  ( CRUDE_COMPOUNT( crude_gfx_pipeline_handle, { CRUDE_RESOURCE_INDEX_INVALID } ) )
#define CRUDE_GFX_SHADER_STATE_HANDLE_INVALID              ( CRUDE_COMPOUNT( crude_gfx_shader_state_handle, { CRUDE_RESOURCE_INDEX_INVALID } ) )
#define CRUDE_GFX_FRAMEBUFFER_HANDLE_INVALID               ( CRUDE_COMPOUNT( crude_gfx_framebuffer_handle, { CRUDE_RESOURCE_INDEX_INVALID } ) )
#define CRUDE_GFX_RENDER_PASS_HANDLE_INVALID               ( CRUDE_COMPOUNT( crude_gfx_render_pass_handle, { CRUDE_RESOURCE_INDEX_INVALID } ) )
#define CRUDE_GFX_DESCRIPTOR_SET_LAYOUT_HANDLE_INVALID     ( CRUDE_COMPOUNT( crude_gfx_descriptor_set_layout_handle, { CRUDE_RESOURCE_INDEX_INVALID } ) )

/************************************************
 *
 * GPU Resoruces Enums
 * 
 ***********************************************/
typedef enum crude_gfx_resource_state
{
  CRUDE_GFX_RESOURCE_STATE_UNDEFINED = 0,
  CRUDE_GFX_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER = 0x1,
  CRUDE_GFX_RESOURCE_STATE_INDEX_BUFFER = 0x2,
  CRUDE_GFX_RESOURCE_STATE_RENDER_TARGET = 0x4,
  CRUDE_GFX_RESOURCE_STATE_UNORDERED_ACCESS = 0x8,
  CRUDE_GFX_RESOURCE_STATE_DEPTH_WRITE = 0x10,
  CRUDE_GFX_RESOURCE_STATE_DEPTH_READ = 0x20,
  CRUDE_GFX_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE = 0x40,
  CRUDE_GFX_RESOURCE_STATE_PIXEL_SHADER_RESOURCE = 0x80,
  CRUDE_GFX_RESOURCE_STATE_SHADER_RESOURCE = 0x40 | 0x80,
  CRUDE_GFX_RESOURCE_STATE_STREAM_OUT = 0x100,
  CRUDE_GFX_RESOURCE_STATE_INDIRECT_ARGUMENT = 0x200,
  CRUDE_GFX_RESOURCE_STATE_COPY_DEST = 0x400,
  CRUDE_GFX_RESOURCE_STATE_COPY_SOURCE = 0x800,
  CRUDE_GFX_RESOURCE_STATE_GENERIC_READ = ( ( ( ( ( 0x1 | 0x2 ) | 0x40 ) | 0x80 ) | 0x200 ) | 0x800 ),
  CRUDE_GFX_RESOURCE_STATE_PRESENT = 0x1000,
  CRUDE_GFX_RESOURCE_STATE_COMMON = 0x2000,
  CRUDE_GFX_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE = 0x4000,
  CRUDE_GFX_RESOURCE_STATE_SHADING_RATE_SOURCE = 0x8000,
} crude_gfx_resource_state;

typedef enum crude_gfx_resource_usage_type 
{
  CRUDE_GFX_RESOURCE_USAGE_TYPE_IMMUTABLE, 
  CRUDE_GFX_RESOURCE_USAGE_TYPE_DYNAMIC,
  CRUDE_GFX_RESOURCE_USAGE_TYPE_STREAM,
  CRUDE_GFX_RESOURCE_USAGE_TYPE_COUNT
} crude_gfx_resource_usage_type;

typedef enum crude_gfx_texture_type
{
  CRUDE_GFX_TEXTURE_TYPE_TEXTURE_1D,
  CRUDE_GFX_TEXTURE_TYPE_TEXTURE_2D,
  CRUDE_GFX_TEXTURE_TYPE_TEXTURE_3D,
  CRUDE_GFX_TEXTURE_TYPE_TEXTURE_1D_ARRAY,
  CRUDE_GFX_TEXTURE_TYPE_TEXTURE_2D_ARRAY,
  CRUDE_GFX_TEXTURE_TYPE_TEXTURE_CUBE_ARRAY,
  CRUDE_GFX_TEXTURE_TYPE_TEXTURE_COUNT,
 } crude_gfx_texture_type;

typedef enum crude_gfx_color_write_enabled
{
  CRUDE_GFX_COLOR_WRITE_ENABLED_RED,
  CRUDE_GFX_COLOR_WRITE_ENABLED_GREEN,
  CRUDE_GFX_COLOR_WRITE_ENABLED_BLUE,
  CRUDE_GFX_COLOR_WRITE_ENABLED_ALPHA,
  CRUDE_GFX_COLOR_WRITE_ENABLED_ALL,
  CRUDE_GFX_COLOR_WRITE_ENABLED_COUNT
} crude_gfx_color_write_enabled;

typedef enum crude_gfx_fill_mode
{
  CRUDE_GFX_FILL_MODE_WIREFRAME,
  CRUDE_GFX_FILL_MODE_SOLID,
  CRUDE_GFX_FILL_MODE_POINT,
  CRUDE_GFX_FILL_MODE_COUNT
} crude_gfx_fill_mode;

typedef enum crude_gfx_render_pass_type
{
  CRUDE_GFX_RENDER_PASS_TYPE_GEOMETRY,
  CRUDE_GFX_RENDER_PASS_TYPE_SWAPCHAIN,
  CRUDE_GFX_RENDER_PASS_TYPE_COMPUTE
} crude_gfx_render_pass_type;

typedef enum crude_gfx_render_pass_operation
{
  CRUDE_GFX_RENDER_PASS_OPERATION_DONT_CARE,
  CRUDE_GFX_RENDER_PASS_OPERATION_LOAD,
  CRUDE_GFX_RENDER_PASS_OPERATION_CLEAR,
  CRUDE_GFX_RENDER_PASS_OPERATION_COUNT
} crude_gfx_render_pass_operation;

typedef enum crude_gfx_resource_deletion_type
{
  CRUDE_GFX_RESOURCE_DELETION_TYPE_BUFFER,
  CRUDE_GFX_RESOURCE_DELETION_TYPE_TEXTURE,
  CRUDE_GFX_RESOURCE_DELETION_TYPE_PIPELINE,
  CRUDE_GFX_RESOURCE_DELETION_TYPE_SAMPLER,
  CRUDE_GFX_RESOURCE_DELETION_TYPE_DESCRIPTOR_SET_LAYOUT,
  CRUDE_GFX_RESOURCE_DELETION_TYPE_DESCRIPTOR_SET,
  CRUDE_GFX_RESOURCE_DELETION_TYPE_RENDER_PASS,
  CRUDE_GFX_RESOURCE_DELETION_TYPE_SHADER_STATE,
  CRUDE_GFX_RESOURCE_DELETION_TYPE_FRAMEBUFFER,
  CRUDE_GFX_RESOURCE_DELETION_TYPE_COUNT
} crude_gfx_resource_deletion_type;

typedef enum crude_gfx_queue_type
{
  CRUDE_GFX_QUEUE_TYPE_GRAPHICS,
  CRUDE_GFX_QUEUE_TYPE_COMPUTE,
  CRUDE_GFX_QUEUE_TYPE_COPY_TRANSFER,
  CRUDE_GFX_QUEUE_TYPE_COUNT
} crude_gfx_queue_type;

typedef enum crude_gfx_texture_mask
{
  CRUDE_GFX_TEXTURE_MASK_DEFAULT = 1 << 0,
  CRUDE_GFX_TEXTURE_MASK_RENDER_TARGET = 1 << 1,
  CRUDE_GFX_TEXTURE_MASK_COMPUTE = 1 << 2,
} crude_gfx_texture_mask;

typedef enum crude_gfx_vertex_component_format
{
  CRUDE_GFX_VERTEX_COMPONENT_FORMAT_FLOAT,
  CRUDE_GFX_VERTEX_COMPONENT_FORMAT_FLOAT2,
  CRUDE_GFX_VERTEX_COMPONENT_FORMAT_FLOAT3,
  CRUDE_GFX_VERTEX_COMPONENT_FORMAT_FLOAT4,
  CRUDE_GFX_VERTEX_COMPONENT_FORMAT_MAT4,
  CRUDE_GFX_VERTEX_COMPONENT_FORMAT_BYTE,
  CRUDE_GFX_VERTEX_COMPONENT_FORMAT_BYTE4N,
  CRUDE_GFX_VERTEX_COMPONENT_FORMAT_UBYTE,
  CRUDE_GFX_VERTEX_COMPONENT_FORMAT_UBYTE4N,
  CRUDE_GFX_VERTEX_COMPONENT_FORMAT_SHORT2,
  CRUDE_GFX_VERTEX_COMPONENT_FORMAT_SHORT2N,
  CRUDE_GFX_VERTEX_COMPONENT_FORMAT_SHORT4,
  CRUDE_GFX_VERTEX_COMPONENT_FORMAT_SHORT4N,
  CRUDE_GFX_VERTEX_COMPONENT_FORMAT_UINT,
  CRUDE_GFX_VERTEX_COMPONENT_FORMAT_UINT2,
  CRUDE_GFX_VERTEX_COMPONENT_FORMAT_UINT4,
  CRUDE_GFX_VERTEX_COMPONENT_FORMAT_COUNT
} crude_gfx_vertex_component_format;
    
typedef enum crude_gfx_vertex_input_rate
{
  CRUDE_GFX_VERTEX_INPUT_RATE_PER_VERTEX,
  CRUDE_GFX_VERTEX_INPUT_RATE_PER_INSTANCE,
  CRUDE_GFX_VERTEX_INPUT_RATE_COUNT,
} crude_gfx_vertex_input_rate;

typedef enum crude_gfx_draw_flags
{
  CRUDE_GFX_DRAW_FLAGS_ALPHA_MASK = 1 << 0,
  CRUDE_GFX_DRAW_FLAGS_TRANSPARENT_MASK = 2 << 0,
} crude_gfx_draw_flags;

typedef enum crude_gfx_pipeline_type
{
  CRUDE_GFX_PIPELINE_TYPE_GRAPHICS,
  CRUDE_GFX_PIPELINE_TYPE_COMPUTE,
  CRUDE_GFX_PIPELINE_TYPE_COUNT
} crude_gfx_pipeline_type;

/************************************************
 *
 * GPU Resoruces Structs
 * 
 ***********************************************/
typedef struct crude_gfx_rect2d_int
{
  int16                                                    x;
  int16                                                    y;
  uint16                                                   width;
  uint16                                                   height;
} crude_gfx_rect2d_int;

typedef struct crude_gfx_viewport
{
  crude_gfx_rect2d_int                                     rect;
  float32                                                  min_depth;
  float32                                                  max_depth;
} crude_gfx_viewport;

typedef struct crude_gfx_stencil_operation_state
{
  VkStencilOp                                              fail;
  VkStencilOp                                              pass;
  VkStencilOp                                              depth_fail;
  VkCompareOp                                              compare;
  uint32                                                   compare_mask;
  uint32                                                   write_mask;
  uint32                                                   reference;
} crude_gfx_stencil_operation_state;

typedef struct crude_gfx_blend_state
{
  VkBlendFactor                                            source_color;
  VkBlendFactor                                            destination_color;
  VkBlendOp                                                color_operation;
  VkBlendFactor                                            source_alpha;
  VkBlendFactor                                            destination_alpha;
  VkBlendOp                                                alpha_operation;
  crude_gfx_color_write_enabled                            color_write_mask;
  uint8                                                    blend_enabled   : 1;
  uint8                                                    separate_blend  : 1;
  uint8                                                    pad             : 6;
} crude_gfx_blend_state;


typedef struct crude_gfx_sampler_creation
{
  VkFilter                                                 min_filter;
  VkFilter                                                 mag_filter;
  VkSamplerMipmapMode                                      mip_filter;
  VkSamplerAddressMode                                     address_mode_u;
  VkSamplerAddressMode                                     address_mode_v;
  VkSamplerAddressMode                                     address_mode_w;
  VkSamplerReductionMode                                   reduction_mode;
  const char*                                              name;
} crude_gfx_sampler_creation;

typedef struct crude_gfx_buffer_creation
{
  VkBufferUsageFlags                                       type_flags;
  crude_gfx_resource_usage_type                            usage;
  uint32                                                   size;
  bool                                                     persistent;
  void                                                    *initial_data;
  char const                                              *name;
  bool                                                     device_only;
} crude_gfx_buffer_creation;

typedef struct crude_gfx_render_pass_creation
{
  uint16                                                   num_render_targets;
  
  VkFormat                                                 color_formats[ CRUDE_GFX_MAX_IMAGE_OUTPUTS ];
  crude_gfx_render_pass_operation                          color_operations[ CRUDE_GFX_MAX_IMAGE_OUTPUTS ];
  VkImageLayout                                            color_final_layouts[ CRUDE_GFX_MAX_IMAGE_OUTPUTS ];
  
  VkFormat                                                 depth_stencil_format;
  VkImageLayout                                            depth_stencil_final_layout;
  
  crude_gfx_render_pass_operation                          depth_operation;
  crude_gfx_render_pass_operation                          stencil_operation;
  
  char const                                              *name;
} crude_gfx_render_pass_creation;

typedef struct crude_gfx_framebuffer_creation
{
  crude_gfx_render_pass_handle                             render_pass;
  char const                                              *name;
  crude_gfx_texture_handle                                 output_textures[ CRUDE_GFX_MAX_IMAGE_OUTPUTS ];
  crude_gfx_texture_handle                                 depth_stencil_texture;
  uint32                                                   num_render_targets;
  bool                                                     manual_resources_free;
  uint16                                                   width;
  uint16                                                   height;
  uint8                                                    resize;
} crude_gfx_framebuffer_creation;

typedef struct crude_gfx_depth_stencil_creation
{
  crude_gfx_stencil_operation_state                        front;
  crude_gfx_stencil_operation_state                        back;
  VkCompareOp                                              depth_comparison;
  uint8                                                    depth_enable        : 1;
  uint8                                                    depth_write_enable  : 1;
  uint8                                                    stencil_enable      : 1;
  uint8                                                    pad                 : 5;
} crude_gfx_depth_stencil_creation;

typedef struct crude_gfx_blend_state_creation
{
  crude_gfx_blend_state                                    blend_states[ CRUDE_GFX_MAX_IMAGE_OUTPUTS ];
  uint32                                                   active_states;
} crude_gfx_blend_state_creation;

typedef struct crude_gfx_rasterization_creation
{
  VkCullModeFlagBits                                       cull_mode;
  VkFrontFace                                              front;
  crude_gfx_fill_mode                                      fill;
} crude_gfx_rasterization_creation;

typedef struct crude_gfx_texture_subresource
{
  uint16                                                   mip_base_level;
  uint16                                                   mip_level_count;
  uint16                                                   array_base_layer;
  uint16                                                   array_layer_count;
} crude_gfx_texture_subresource;

typedef struct crude_gfx_texture_creation
{
  void                                                    *initial_data;
  uint16                                                   width;
  uint16                                                   height;
  uint16                                                   depth;
  crude_gfx_texture_subresource                            subresource;
  uint8                                                    flags;
  VkFormat                                                 format;
  crude_gfx_texture_type                                   type;
  char const                                              *name;
  crude_gfx_texture_handle                                 alias;
} crude_gfx_texture_creation;

typedef struct crude_gfx_texture_view_creation
{
  crude_gfx_texture_handle                                 parent_texture_handle;
  VkImageViewType                                          view_type;
  crude_gfx_texture_subresource                            subresource;
  char const                                              *name;
} crude_gfx_texture_view_creation;

typedef struct crude_gfx_render_pass_output
{
  VkFormat                                                 color_formats[ CRUDE_GFX_MAX_IMAGE_OUTPUTS ];
  VkImageLayout                                            color_final_layouts[ CRUDE_GFX_MAX_IMAGE_OUTPUTS ];
  crude_gfx_render_pass_operation                          color_operations[ CRUDE_GFX_MAX_IMAGE_OUTPUTS ];

  VkFormat                                                 depth_stencil_format;
  VkImageLayout                                            depth_stencil_final_layout;

  uint32                                                   num_color_formats;

  crude_gfx_render_pass_operation                          depth_operation;
  crude_gfx_render_pass_operation                          stencil_operation;
} crude_gfx_render_pass_output;

typedef struct crude_gfx_vertex_attribute
{
  uint32                                                   location;
  uint32                                                   binding;
  uint32                                                   offset;
  crude_gfx_vertex_component_format                        format;
} crude_gfx_vertex_attribute;

typedef struct crude_gfx_vertex_stream
{
  uint32                                                   binding;
  uint32                                                   stride;
  crude_gfx_vertex_input_rate                              input_rate;
} crude_gfx_vertex_stream;

typedef struct crude_gfx_shader_stage
{
  char const                                              *code;
  uint32                                                   code_size;
  VkShaderStageFlagBits                                    type;
} crude_gfx_shader_stage; 

typedef struct crude_gfx_shader_state_creation
{
  crude_gfx_shader_stage                                   stages[ CRUDE_GFX_MAX_SHADER_STAGES ];
  char const                                              *name;
  uint32                                                   stages_count;
  uint32                                                   spv_input;
} crude_gfx_shader_state_creation;

typedef struct crude_gfx_viewport_state
{
  uint32                                                   num_viewports;
  uint32                                                   num_scissors;
  crude_gfx_viewport                                      *dev_viewport;
  crude_gfx_rect2d_int                                    *scissors;
} crude_gfx_viewport_state;

typedef struct crude_gfx_descriptor_set_layout_binding
{
  VkDescriptorType                                         type;
  uint16                                                   start;
  uint16                                                   count;
  char const                                              *name;
} crude_gfx_descriptor_set_layout_binding;

typedef struct crude_gfx_descriptor_set_layout_creation
{
  crude_gfx_descriptor_set_layout_binding                  bindings[ CRUDE_GFX_MAX_DESCRIPTORS_PER_SET ];
  uint32                                                   num_bindings;
  uint32                                                   set_index;
  char const                                              *name;
  bool                                                     bindless;
} crude_gfx_descriptor_set_layout_creation;

typedef struct crude_gfx_pipeline_creation
{
  crude_gfx_rasterization_creation                         rasterization;
  crude_gfx_depth_stencil_creation                         depth_stencil;
  crude_gfx_blend_state_creation                           blend_state;
  crude_gfx_shader_state_creation                          shaders;
  crude_gfx_render_pass_output                             render_pass_output;
  bool                                                     relfect_vertex_input;
  crude_gfx_vertex_stream                                  vertex_streams[ 8 ];
  crude_gfx_vertex_attribute                               vertex_attributes[ 8 ];
  uint32                                                   vertex_streams_num;
  uint32                                                   vertex_attributes_num;
  VkPrimitiveTopology                                      topology;
  crude_gfx_viewport_state const                          *dev_viewport;
  char const                                              *name;
} crude_gfx_pipeline_creation;

typedef struct crude_gfx_descriptor_set_creation
{
  crude_gfx_resource_index                                 resources[ CRUDE_GFX_MAX_DESCRIPTORS_PER_SET ];
  crude_gfx_sampler_handle                                 samplers[ CRUDE_GFX_MAX_DESCRIPTORS_PER_SET ];
  uint16                                                   bindings[ CRUDE_GFX_MAX_DESCRIPTORS_PER_SET ];
  crude_gfx_descriptor_set_layout_handle                   layout;
  uint32                                                   num_resources;
  char const                                              *name;
} crude_gfx_descriptor_set_creation;

typedef struct crude_gfx_shader_descriptor_reflect
{
  uint32                                                   sets_count;
  crude_gfx_descriptor_set_layout_creation                 sets[ CRUDE_GFX_MAX_SET_COUNT ];
} crude_gfx_shader_descriptor_reflect;

typedef struct crude_gfx_shader_input_reflect
{
  crude_gfx_vertex_stream                                 *vertex_streams;
  crude_gfx_vertex_attribute                              *vertex_attributes;
} crude_gfx_shader_input_reflect;

typedef struct crude_gfx_shader_push_constant_reflect
{
  uint32                                                   stride;
} crude_gfx_shader_push_constant_reflect;

typedef struct crude_gfx_shader_reflect
{
  crude_gfx_shader_descriptor_reflect                      descriptor;
  crude_gfx_shader_input_reflect                           input;
  crude_gfx_shader_push_constant_reflect                   push_constant;
} crude_gfx_shader_reflect;

typedef struct crude_gfx_buffer
{
  VkBuffer                                                 vk_buffer;
  VmaAllocation                                            vma_allocation;
  VkDeviceMemory                                           vk_device_memory;
  VkDeviceSize                                             vk_device_size;
  VkBufferUsageFlags                                       type_flags;
  crude_gfx_resource_usage_type                            usage;
  uint32                                                   size;
  uint32                                                   global_offset;
  crude_gfx_buffer_handle                                  handle;
  crude_gfx_buffer_handle                                  parent_buffer;
  char const                                              *name;
  uint8                                                   *mapped_data;
  bool                                                     ready;
} crude_gfx_buffer;

typedef struct crude_gfx_sampler
{
  VkSampler                                                vk_sampler;
  VkFilter                                                 min_filter;
  VkFilter                                                 mag_filter;
  VkSamplerMipmapMode                                      mip_filter;
  VkSamplerAddressMode                                     address_mode_u;
  VkSamplerAddressMode                                     address_mode_v;
  VkSamplerAddressMode                                     address_mode_w;
  char const                                              *name;
} crude_gfx_sampler;

typedef struct crude_gfx_texture
{
  VkImage                                                  vk_image;
  VkImageView                                              vk_image_view;
  VkFormat                                                 vk_format;
  VmaAllocation                                            vma_allocation;
  uint16                                                   width;
  uint16                                                   height;
  uint16                                                   depth;
  crude_gfx_texture_subresource                            subresource;
  uint8                                                    flags;
  crude_gfx_texture_handle                                 handle;
  crude_gfx_texture_type                                   type;
  crude_gfx_sampler                                       *sampler;
  char const                                              *name;
  crude_gfx_resource_state                                 state;
  bool                                                     ready;
  crude_gfx_texture_handle                                 parent_texture_handle;
} crude_gfx_texture;

typedef struct crude_gfx_descriptor_binding
{
  VkDescriptorType                                         type;
  uint16                                                   start;
  uint16                                                   count;
  uint16                                                   set;
  const char                                              *name;
} crude_gfx_descriptor_binding;

typedef struct crude_gfx_descriptor_set_layout
{
  VkDescriptorPool                                         vk_descriptor_pool;
  VkDescriptorSetLayout                                    vk_descriptor_set_layout;
  VkDescriptorSetLayoutBinding                            *vk_binding;
  crude_gfx_descriptor_binding                            *bindings;
  uint8                                                    binding_to_index[ CRUDE_GFX_MAX_DESCRIPTORS_PER_SET ];
  uint16                                                   num_bindings;
  uint16                                                   set_index;
  crude_gfx_descriptor_set_layout_handle                   handle;
  bool                                                     bindless;
} crude_gfx_descriptor_set_layout;

typedef struct crude_gfx_descriptor_set
{
  VkDescriptorSet                                          vk_descriptor_set;
  crude_gfx_resource_index                                 resources[ CRUDE_GFX_MAX_DESCRIPTORS_PER_SET ];
  crude_gfx_sampler_handle                                 samplers[ CRUDE_GFX_MAX_DESCRIPTORS_PER_SET ];
  uint16                                                   bindings[ CRUDE_GFX_MAX_DESCRIPTORS_PER_SET ];
  crude_gfx_descriptor_set_layout const                   *layout;
  uint32                                                   num_resources;
  char const                                              *name;
} crude_gfx_descriptor_set;

typedef struct crude_gfx_pipeline
{
  VkPipeline                                               vk_pipeline;
  VkPipelineLayout                                         vk_pipeline_layout;
  VkPipelineBindPoint                                      vk_bind_point;
  crude_gfx_shader_state_handle                            shader_state;
  crude_gfx_descriptor_set_layout const                   *descriptor_set_layout[ CRUDE_GFX_MAX_DESCRIPTOR_SET_LAYOUTS ];
  crude_gfx_descriptor_set_layout_handle                   descriptor_set_layout_handle[ CRUDE_GFX_MAX_DESCRIPTOR_SET_LAYOUTS ];
  uint32                                                   num_active_layouts;
  crude_gfx_depth_stencil_creation                         depth_stencil;
  crude_gfx_blend_state_creation                           blend_state;
  crude_gfx_rasterization_creation                         rasterization;
  crude_gfx_pipeline_handle                                handle;
  bool                                                     graphics_pipeline;
  char const                                               *name;
} crude_gfx_pipeline;

typedef struct crude_gfx_render_pass
{
  crude_gfx_render_pass_output                             output;
  uint8                                                    num_render_targets;
  char const                                              *name;
} crude_gfx_render_pass;

typedef struct crude_gfx_framebuffer
{
  crude_gfx_render_pass_handle                             render_pass;
  uint16                                                   width;
  uint16                                                   height;
  float32                                                  scale_x;
  float32                                                  scale_y;
  crude_gfx_texture_handle                                 color_attachments[ CRUDE_GFX_MAX_IMAGE_OUTPUTS ];
  crude_gfx_texture_handle                                 depth_stencil_attachment;
  uint32                                                   num_color_attachments;
  uint8                                                    resize;
  bool                                                     manual_resources_free;
  char const                                              *name;
} crude_gfx_framebuffer;

typedef struct crude_gfx_shader_state
{
  VkPipelineShaderStageCreateInfo                          shader_stage_info[ CRUDE_GFX_MAX_SHADER_STAGES ];
  const char                                              *name;
  uint32                                                   active_shaders;
  crude_gfx_pipeline_type                                  pipeline_type;
  crude_gfx_shader_reflect                                 reflect;
} crude_gfx_shader_state;

typedef struct crude_gfx_buffer_description
{
  void                                                    *native_handle;
  char const                                              *name;
  VkBufferUsageFlags                                       type_flags;
  crude_gfx_resource_usage_type                            usage;
  uint32                                                   size;
  crude_gfx_buffer_handle                                  parent_handle;
} crude_gfx_buffer_description;

typedef struct crude_gfx_map_buffer_parameters
{
  crude_gfx_buffer_handle                                  buffer;
  uint32                                                   offset;
  uint32                                                   size;
} crude_gfx_map_buffer_parameters;

typedef struct crude_gfx_resource_update
{
  crude_gfx_resource_deletion_type                         type;
  crude_gfx_resource_index                                 handle;
  uint32                                                   current_frame;
} crude_gfx_resource_update;

/************************************************
 *
 * GPU Resoruces Creation Empty Functions
 * 
 ***********************************************/
CRUDE_API crude_gfx_sampler_creation
crude_gfx_sampler_creation_empty
(
);

CRUDE_API crude_gfx_buffer_creation
crude_gfx_buffer_creation_empty
(
);

CRUDE_API crude_gfx_framebuffer_creation
crude_gfx_framebuffer_creation_empty
(
);

CRUDE_API crude_gfx_pipeline_creation
crude_gfx_pipeline_creation_empty
(
);

CRUDE_API void
crude_gfx_pipeline_creation_add_blend_state
(
  _In_ crude_gfx_pipeline_creation                        *creation,
  _In_ crude_gfx_blend_state                               blend_state
);

CRUDE_API void
crude_gfx_pipeline_creation_add_vertex_stream
(
  _In_ crude_gfx_pipeline_creation                        *creation,
  _In_ uint32                                              binding,
  _In_ uint32                                              stride,
  _In_ crude_gfx_vertex_input_rate                         input_rate
);

CRUDE_API void
crude_gfx_pipeline_creation_add_vertex_attribute
(
  _In_ crude_gfx_pipeline_creation                        *creation,
  _In_ uint32                                              location,
  _In_ uint32                                              binding,
  _In_ uint32                                              offset,
  _In_ crude_gfx_vertex_component_format                   format
);

CRUDE_API crude_gfx_descriptor_set_creation
crude_gfx_descriptor_set_creation_empty
(
);

CRUDE_API crude_gfx_render_pass_creation
crude_gfx_render_pass_creation_empty
(
);

CRUDE_API crude_gfx_texture_creation
crude_gfx_texture_creation_empty
(
);

CRUDE_API crude_gfx_texture_view_creation
crude_gfx_texture_view_creation_empty
(
);

CRUDE_API crude_gfx_render_pass_output
crude_gfx_render_pass_output_empty
(
);

CRUDE_API void
crude_gfx_render_pass_output_add_color
(
  _In_ crude_gfx_render_pass_output                       *output,
  _In_ VkFormat                                            color_format,
  _In_ VkImageLayout                                       color_final_layout,
  _In_ crude_gfx_render_pass_operation                     color_operation
);

CRUDE_API void
crude_gfx_descriptor_set_creation_add_buffer
(
  _In_ crude_gfx_descriptor_set_creation                  *creation,
  _In_ crude_gfx_buffer_handle                             buffer,
  _In_ uint16                                              binding
);

CRUDE_API void
crude_gfx_descriptor_set_creation_add_texture
(
  _In_ crude_gfx_descriptor_set_creation                  *creation,
  _In_ crude_gfx_texture_handle                            texture,
  _In_ uint16                                              binding
);

CRUDE_API void
crude_gfx_descriptor_set_layout_creation_add_binding
(
  _In_ crude_gfx_descriptor_set_layout_creation           *creation,
  _In_ crude_gfx_descriptor_set_layout_binding             binding
);

CRUDE_API void
crude_gfx_shader_state_creation_add_stage
(
  _In_ crude_gfx_shader_state_creation                    *creation,
  _In_ char const                                         *code,
  _In_ uint64                                              code_size,
  _In_ VkShaderStageFlagBits                               type
);

/************************************************
 *
 * GPU Resoruces Functions
 * 
 ***********************************************/
CRUDE_API VkImageType
crude_gfx_to_vk_image_type
( 
  _In_ crude_gfx_texture_type                              type
);

CRUDE_API bool
crude_gfx_has_depth_or_stencil
( 
  _In_ VkFormat                                            value
);

CRUDE_API bool
crude_gfx_has_depth
( 
  _In_ VkFormat                                            value
);

CRUDE_API VkImageViewType
crude_gfx_to_vk_image_view_type
( 
  _In_ crude_gfx_texture_type                              type
);

CRUDE_API VkFormat
crude_gfx_to_vk_vertex_format
( 
  _In_ crude_gfx_vertex_component_format                   value
);

CRUDE_API VkAccessFlags2
crude_gfx_resource_state_to_vk_access_flags2
(
  _In_ crude_gfx_resource_state                            state
);

CRUDE_API VkImageLayout
crude_gfx_resource_state_to_vk_image_layout2
(
  _In_ crude_gfx_resource_state                            state
);

CRUDE_API VkPipelineStageFlags2
crude_gfx_determine_pipeline_stage_flags2
(
  _In_ VkAccessFlags2                                      access_flags,
  _In_ crude_gfx_queue_type                                queue_type
);

CRUDE_API VkFormat
crude_gfx_string_to_vk_format
(
  _In_ char const                                         *format
);

CRUDE_API crude_gfx_vertex_component_format
crude_gfx_to_vertex_component_format
( 
  _In_ char const                                         *format
);

CRUDE_API VkPrimitiveTopology
crude_gfx_string_to_vk_primitive_topology
( 
  _In_ char const                                         *format
);

CRUDE_API char const*
crude_gfx_vk_shader_stage_to_defines
(
  _In_ VkShaderStageFlagBits                              value
);

CRUDE_API char const*
crude_gfx_vk_shader_stage_to_compiler_extension
(
  _In_ VkShaderStageFlagBits                               value
);

CRUDE_API char const*
crude_gfx_resource_state_to_name
(
  _In_ crude_gfx_resource_state                            value
);