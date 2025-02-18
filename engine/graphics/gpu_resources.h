#pragma once

#include <vulkan/vulkan.h>
#include <vma_usage.h>

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

#define CRUDE_MAX_IMAGE_OUTPUTS             8          
#define CRUDE_MAX_DESCRIPTOR_SET_LAYOUTS    8 
#define CRUDE_MAX_SHADER_STAGES             5      
#define CRUDE_MAX_DESCRIPTORS_PER_SET       16

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
  crude_resource_handle               *resources;
  crude_sampler_handle                *samplers;
  uint16                              *bindings;
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

typedef struct crude_render_pass_output
{
  VkFormat                             color_formats[ CRUDE_MAX_IMAGE_OUTPUTS ];
  VkFormat                             depth_stencil_format;
  uint32                               num_color_formats;
  crude_render_pass_operation          color_operation;
  crude_render_pass_operation          depth_operation;
  crude_render_pass_operation          stencil_operation;
} crude_render_pass_output;

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
} crude_shader_state;

typedef struct crude_resource_update
{
  crude_resource_deletion_type         type;
  crude_resource_handle                handle;
  uint32                               current_frame;
} crude_resource_update;

CRUDE_API void crude_reset_render_pass_output( _In_ crude_render_pass_output *output );
CRUDE_API VkImageType crude_to_vk_image_type( _In_ crude_texture_type type );
CRUDE_API bool crude_has_depth_or_stencil( _In_ VkFormat value );
CRUDE_API bool crude_has_depth( _In_ VkFormat value );
CRUDE_API VkImageViewType crude_to_vk_image_view_type( _In_ crude_texture_type type );