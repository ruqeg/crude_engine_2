#include <core/assert.h>
#include <core/array.h>
#include <core/profiler.h>
#include <graphics/gpu_device.h>

#include <graphics/command_buffer.h>


/************************************************
 *
 * Utils
 * 
 ***********************************************/
uint32 pool_from_indices
(
  _In_ crude_gfx_cmd_buffer_manager             *cmd_manager,
  _In_ uint32                                    frame_index,
  _In_ uint32                                    thread_index
)
{
  return ( frame_index * cmd_manager->num_pools_per_frame ) + thread_index;
}

/************************************************
 *
 * Command Buffer Functions
 * 
 ***********************************************/
CRUDE_API void
crude_gfx_cmd_initialize
(
  _In_ crude_gfx_cmd_buffer                               *cmd,
  _In_ crude_gfx_device                                   *gpu
)
{
  cmd->gpu = gpu;

  uint32 const global_pool_elements = 128;
  VkDescriptorPoolSize pool_sizes[] =
  {
    { VK_DESCRIPTOR_TYPE_SAMPLER, global_pool_elements },
    { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, global_pool_elements },
    { VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, global_pool_elements },
    { VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, global_pool_elements },
    { VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, global_pool_elements },
    { VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, global_pool_elements },
    { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, global_pool_elements },
    { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, global_pool_elements },
    { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, global_pool_elements },
    { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, global_pool_elements },
    { VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, global_pool_elements }
  };
  
  VkDescriptorPoolCreateInfo pool_info = {
    .sType         = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
    .flags         = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT,
    .maxSets       = global_pool_elements * CRUDE_COUNTOF( pool_sizes ),
    .poolSizeCount = CRUDE_COUNTOF( pool_sizes ),
    .pPoolSizes    = pool_sizes,
  };
  
  CRUDE_GFX_HANDLE_VULKAN_RESULT( vkCreateDescriptorPool( cmd->gpu->vk_device, &pool_info, cmd->gpu->vk_allocation_callbacks, &cmd->vk_descriptor_pool ), "Failed create descriptor pool" );
  
  crude_resource_pool_initialize( &cmd->frame_descriptor_sets, cmd->gpu->allocator_container, 256, sizeof( crude_gfx_descriptor_set ) );
  
  crude_gfx_cmd_reset( cmd );
}

CRUDE_API void
crude_gfx_cmd_deinitialize
(
  _In_ crude_gfx_cmd_buffer                               *cmd
)
{
  crude_gfx_cmd_reset( cmd );
  crude_resource_pool_deinitialize( &cmd->frame_descriptor_sets );
  vkDestroyDescriptorPool( cmd->gpu->vk_device, cmd->vk_descriptor_pool, cmd->gpu->vk_allocation_callbacks );
}

void
crude_gfx_cmd_reset
(
  _In_ crude_gfx_cmd_buffer                               *cmd
)
{
  cmd->is_recording = false;
  cmd->current_render_pass = NULL;
  
  vkResetDescriptorPool( cmd->gpu->vk_device, cmd->vk_descriptor_pool, 0 );
  
  uint32 resource_count = cmd->frame_descriptor_sets.free_indices_head;
  for ( uint32 i = 0; i < resource_count; ++i )
  {
    crude_resource_pool_release_resource( &cmd->frame_descriptor_sets, i );
  }
}

void
crude_gfx_cmd_begin_primary
(
  _In_ crude_gfx_cmd_buffer                               *cmd
)
{
  if ( cmd->is_recording )
  {
    return;
  }

  cmd->is_recording = true;

  VkCommandBufferBeginInfo begin_info = {
    .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
    .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT
  };
  vkBeginCommandBuffer( cmd->vk_cmd_buffer, &begin_info );
}

void
crude_gfx_cmd_begin_secondary
(
  _In_ crude_gfx_cmd_buffer                               *cmd,
  _In_ crude_gfx_render_pass                              *render_pass,
  _In_ crude_gfx_framebuffer                              *framebuffer
)
{
  if ( cmd->is_recording )
  {
    return;
  }
  
  VkCommandBufferInheritanceRenderingInfo rendering_info = {
    .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_INHERITANCE_RENDERING_INFO,
    .flags = VK_RENDERING_CONTENTS_SECONDARY_COMMAND_BUFFERS_BIT_KHR,
    .viewMask = 0,
    .colorAttachmentCount = render_pass->num_render_targets,
    .pColorAttachmentFormats = render_pass->num_render_targets > 0 ? render_pass->output.color_formats : NULL,
    .depthAttachmentFormat = render_pass->output.depth_stencil_format,
    .stencilAttachmentFormat = VK_FORMAT_UNDEFINED,
    .rasterizationSamples = VK_SAMPLE_COUNT_1_BIT,
  };
  VkCommandBufferInheritanceInfo inheritance = {
    .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_INHERITANCE_INFO,
    .pNext = &rendering_info,
    .renderPass = VK_NULL_HANDLE,
    .subpass = 0,
    .framebuffer = VK_NULL_HANDLE,
  };
  VkCommandBufferBeginInfo begin_info = {
    .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
    .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT | VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT,
    .pInheritanceInfo = &inheritance,
  };
  vkBeginCommandBuffer( cmd->vk_cmd_buffer, &begin_info );

  cmd->is_recording = true;
  cmd->current_render_pass = render_pass;
  cmd->current_framebuffer = framebuffer;
}

CRUDE_API void
crude_gfx_cmd_end
(
  _In_ crude_gfx_cmd_buffer                               *cmd
)
{
  if ( !cmd->is_recording )
  {
    return;
  }

  vkEndCommandBuffer( cmd->vk_cmd_buffer );
  cmd->is_recording = false;
}

void
crude_gfx_cmd_end_render_pass
(
  _In_ crude_gfx_cmd_buffer                               *cmd
)
{
  if ( cmd->is_recording && cmd->current_render_pass != NULL && cmd->current_framebuffer != NULL )
  {
    cmd->gpu->vkCmdEndRenderingKHR( cmd->vk_cmd_buffer );
    cmd->current_render_pass = NULL;
    cmd->current_framebuffer = NULL;
  }
}

void
crude_gfx_cmd_bind_render_pass
(
  _In_ crude_gfx_cmd_buffer                               *cmd,
  _In_ crude_gfx_render_pass_handle                        render_pass_handle,
  _In_ crude_gfx_framebuffer_handle                        framebuffer_handle,
  _In_ bool                                                use_secondary
)
{
  cmd->is_recording = true;
  
  crude_gfx_render_pass *render_pass = crude_gfx_access_render_pass( cmd->gpu, render_pass_handle );
  if ( !render_pass )
  {
    CRUDE_LOG_ERROR( CRUDE_CHANNEL_GRAPHICS, "Failed to bind render pass! Invalid render pass %u!", render_pass_handle.index );
    return;
  }

  if ( render_pass == cmd->current_render_pass )
  {
    CRUDE_LOG_WARNING( CRUDE_CHANNEL_GRAPHICS, "Binding same render pass %s %u!", render_pass->name, render_pass_handle.index );
    return;
  }

  if ( cmd->current_render_pass )
  {
    crude_gfx_cmd_end_render_pass( cmd );
  }
  
  crude_gfx_framebuffer *framebuffer = crude_gfx_access_framebuffer( cmd->gpu, framebuffer_handle );
  
  // !TODO tmp allocator?
  VkRenderingAttachmentInfoKHR *color_attachments_info;
  CRUDE_ARRAY_INITIALIZE_WITH_LENGTH( color_attachments_info, framebuffer->num_color_attachments, cmd->gpu->allocator_container );
  memset( color_attachments_info, 0, sizeof( VkRenderingAttachmentInfoKHR ) * framebuffer->num_color_attachments );
  
  for ( uint32 i = 0; i < framebuffer->num_color_attachments; ++i )
  {
    crude_gfx_texture *texture = crude_gfx_access_texture( cmd->gpu, framebuffer->color_attachments[ i ] );
    
    VkAttachmentLoadOp color_op;
    switch ( render_pass->output.color_operations[ i ] )
    {
      case CRUDE_GFX_RENDER_PASS_OPERATION_LOAD:
        color_op = VK_ATTACHMENT_LOAD_OP_LOAD;
        break;
      case CRUDE_GFX_RENDER_PASS_OPERATION_CLEAR:
        color_op = VK_ATTACHMENT_LOAD_OP_CLEAR;
        break;
      default:
        color_op = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        break;
    }
    
    VkRenderingAttachmentInfoKHR *color_attachment_info = &color_attachments_info[ i ];
    color_attachment_info->sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO_KHR;
    color_attachment_info->imageView = texture->vk_image_view;
    color_attachment_info->imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    color_attachment_info->resolveMode = VK_RESOLVE_MODE_NONE;
    color_attachment_info->loadOp = color_op;
    color_attachment_info->storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    color_attachment_info->clearValue = render_pass->output.color_operations[ i ] == CRUDE_GFX_RENDER_PASS_OPERATION_CLEAR ? cmd->clears[ 0 ] : CRUDE_COMPOUNT_EMPTY( VkClearValue );
  }
  
  VkRenderingAttachmentInfoKHR depth_attachment_info = { 
    .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO_KHR
  };
  
  bool has_depth_attachment = CRUDE_RESOURCE_HANDLE_IS_VALID( framebuffer->depth_stencil_attachment );
  if ( has_depth_attachment )
  {
    crude_gfx_texture *texture = crude_gfx_access_texture( cmd->gpu, framebuffer->depth_stencil_attachment );
  
    VkAttachmentLoadOp depth_op;
    switch ( render_pass->output.depth_operation )
    {
      case CRUDE_GFX_RENDER_PASS_OPERATION_LOAD:
        depth_op = VK_ATTACHMENT_LOAD_OP_LOAD;
        break;
      case CRUDE_GFX_RENDER_PASS_OPERATION_CLEAR:
        depth_op = VK_ATTACHMENT_LOAD_OP_CLEAR;
        break;
      default:
        depth_op = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        break;
    }
    
    depth_attachment_info.imageView = texture->vk_image_view;
    depth_attachment_info.imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    depth_attachment_info.resolveMode = VK_RESOLVE_MODE_NONE;
    depth_attachment_info.loadOp = depth_op;
    depth_attachment_info.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    depth_attachment_info.clearValue = render_pass->output.depth_operation == CRUDE_GFX_RENDER_PASS_OPERATION_CLEAR ? cmd->clears[ 1 ] : CRUDE_COMPOUNT_EMPTY( VkClearValue );
  }
  
  VkRenderingInfoKHR rendering_info = {
    .sType = VK_STRUCTURE_TYPE_RENDERING_INFO_KHR,
    .renderArea = { 0, 0, framebuffer->width, framebuffer->height },
    .layerCount = 1,
    .viewMask = 0,
    .colorAttachmentCount = framebuffer->num_color_attachments,
    .pColorAttachments = framebuffer->num_color_attachments > 0 ? color_attachments_info : NULL,
    .pDepthAttachment =  has_depth_attachment ? &depth_attachment_info : NULL,
    .pStencilAttachment = NULL,
  };

  if ( use_secondary )
  {
    rendering_info.flags = VK_RENDERING_CONTENTS_SECONDARY_COMMAND_BUFFERS_BIT_KHR;
  }
  
  cmd->gpu->vkCmdBeginRenderingKHR( cmd->vk_cmd_buffer, &rendering_info );
  
  CRUDE_ARRAY_DEINITIALIZE( color_attachments_info );
  
  cmd->current_render_pass = render_pass;
  cmd->current_framebuffer = framebuffer;
}

void
crude_gfx_cmd_bind_pipeline
(
  _In_ crude_gfx_cmd_buffer                               *cmd,
  _In_ crude_gfx_pipeline_handle                           handle
)
{
  crude_gfx_pipeline *pipeline = crude_gfx_access_pipeline( cmd->gpu, handle );
  if ( !pipeline )
  {
    CRUDE_LOG_ERROR( CRUDE_CHANNEL_GRAPHICS, "Failed to bind pipeline! Invalid pipeline %u!", handle.index );
    return;
  }

  vkCmdBindPipeline( cmd->vk_cmd_buffer, pipeline->vk_bind_point, pipeline->vk_pipeline );
  cmd->current_pipeline = pipeline;
}

void
crude_gfx_cmd_set_viewport
(
  _In_ crude_gfx_cmd_buffer                               *cmd,
  _In_opt_ crude_gfx_viewport const                       *viewport
)
{
  VkViewport vk_viewport;
  
  if ( viewport )
  {
    vk_viewport.x = viewport->rect.x * 1.f;
    vk_viewport.width = viewport->rect.width * 1.f;
    vk_viewport.y = viewport->rect.height * 1.f - viewport->rect.y;
    vk_viewport.height = -viewport->rect.height * 1.f;
    vk_viewport.minDepth = viewport->min_depth;
    vk_viewport.maxDepth = viewport->max_depth;
  }
  else
  {
    vk_viewport.x = 0.f;
    
    if ( cmd->current_render_pass )
    {
      vk_viewport.width = cmd->current_framebuffer->width * 1.f;
      vk_viewport.y = cmd->current_framebuffer->height * 1.f;
      vk_viewport.height = -cmd->current_framebuffer->height * 1.f;
    }
    else
    {
      vk_viewport.width = cmd->gpu->vk_swapchain_width * 1.f;
      vk_viewport.y = cmd->gpu->vk_swapchain_height * 1.f;
      vk_viewport.height = -cmd->gpu->vk_swapchain_height * 1.f;
    }
    vk_viewport.minDepth = 0.0f;
    vk_viewport.maxDepth = 1.0f;
  }
  
  vkCmdSetViewport( cmd->vk_cmd_buffer, 0, 1, &vk_viewport);
}

void
crude_gfx_cmd_set_clear_color
(
  _In_ crude_gfx_cmd_buffer                               *cmd,
  _In_ uint32                                              index,
  _In_ VkClearValue                                        clear
)
{
  cmd->clears[ index ] = clear;
}

void
crude_gfx_cmd_set_scissor
(
  _In_ crude_gfx_cmd_buffer                               *cmd,
  _In_opt_ crude_gfx_rect2d_int const                     *rect
)
{
  VkRect2D vk_scissor;
  
  if ( rect )
  {
    vk_scissor.offset.x = rect->x;
    vk_scissor.offset.y = rect->y;
    vk_scissor.extent.width = rect->width;
    vk_scissor.extent.height = rect->height;
  }
  else
  {
    vk_scissor.offset.x = 0;
    vk_scissor.offset.y = 0;
    vk_scissor.extent.width = cmd->gpu->vk_swapchain_width;
    vk_scissor.extent.height = cmd->gpu->vk_swapchain_height;
  }
  
  vkCmdSetScissor( cmd->vk_cmd_buffer, 0, 1, &vk_scissor );
}

void
crude_gfx_cmd_bind_local_descriptor_set
(
  _In_ crude_gfx_cmd_buffer                               *cmd,
  _In_ crude_gfx_descriptor_set_handle                     handle
)
{
  crude_gfx_descriptor_set *descriptor_set = CRUDE_REINTERPRET_CAST( crude_gfx_descriptor_set*, crude_resource_pool_access_resource( &cmd->frame_descriptor_sets, handle.index ) );
  crude_gfx_descriptor_set *bindless_descriptor_set = crude_gfx_access_descriptor_set( cmd->gpu, cmd->gpu->bindless_descriptor_set_handle );
  
  uint32 num_offsets = 0u;
  uint32 offsets_cache[ 8 ];
  for ( uint32 i = 0; i < descriptor_set->layout->num_bindings; ++i )
  {
    if ( descriptor_set->layout->bindings[ i ].type == VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER )
    {
        CRUDE_ASSERT( num_offsets < CRUDE_COUNTOF( offsets_cache ) );
        const uint32 resource_index = descriptor_set->bindings[ i ];
        crude_gfx_buffer_handle buffer_handle = { descriptor_set->resources[ resource_index ] };
        crude_gfx_buffer *buffer = crude_gfx_access_buffer( cmd->gpu, buffer_handle );
        offsets_cache[ num_offsets++ ] = buffer->global_offset;
    }
  }
  vkCmdBindDescriptorSets( cmd->vk_cmd_buffer, cmd->current_pipeline->vk_bind_point, cmd->current_pipeline->vk_pipeline_layout, CRUDE_GFX_BINDLESS_DESCRIPTOR_SET_INDEX, 1u, &bindless_descriptor_set->vk_descriptor_set, 0u, NULL );
  vkCmdBindDescriptorSets( cmd->vk_cmd_buffer, cmd->current_pipeline->vk_bind_point, cmd->current_pipeline->vk_pipeline_layout, CRUDE_GFX_MATERIAL_DESCRIPTOR_SET_INDEX, 1u, &descriptor_set->vk_descriptor_set, num_offsets, offsets_cache );
}

void
crude_gfx_cmd_draw
(
  _In_ crude_gfx_cmd_buffer                               *cmd,
  _In_ uint32                                              first_vertex,
  _In_ uint32                                              vertex_count,
  _In_ uint32                                              first_instance,
  _In_ uint32                                              instance_count
)
{
  vkCmdDraw( cmd->vk_cmd_buffer, vertex_count, instance_count, first_vertex, first_instance );
}

void
crude_gfx_cmd_draw_indexed
(
  _In_ crude_gfx_cmd_buffer                               *cmd,
  _In_ uint32                                              index_count,
  _In_ uint32                                              instance_count,
  _In_ uint32                                              first_index,
  _In_ int32                                               vertex_offset,
  _In_ uint32                                              first_instance
)
{
  vkCmdDrawIndexed( cmd->vk_cmd_buffer, index_count, instance_count, first_index, vertex_offset, first_instance );
}

void
crude_gfx_cmd_draw_mesh_task_indirect_count
(
  _In_ crude_gfx_cmd_buffer                               *cmd,
  _In_ crude_gfx_buffer_handle                             argument_buffer_handle,
  _In_ uint32                                              argument_offset,
  _In_ crude_gfx_buffer_handle                             count_buffer_handle,
  _In_ uint32                                              count_offset,
  _In_ uint32                                              max_draws,
  _In_ uint32                                              stride
)
{
  crude_gfx_buffer *argument_buffer = crude_gfx_access_buffer( cmd->gpu, argument_buffer_handle );
  crude_gfx_buffer *count_buffer = crude_gfx_access_buffer( cmd->gpu, count_buffer_handle );
  
  cmd->gpu->vkCmdDrawMeshTasksIndirectCountEXT( cmd->vk_cmd_buffer, argument_buffer->vk_buffer, argument_offset, count_buffer->vk_buffer, count_offset, max_draws, stride );
}

void
crude_gfx_cmd_dispatch
(
  _In_ crude_gfx_cmd_buffer                               *cmd,
  _In_ uint32                                              group_count_x,
  _In_ uint32                                              group_count_y,
  _In_ uint32                                              group_count_z
)
{
  vkCmdDispatch( cmd->vk_cmd_buffer, group_count_x, group_count_y, group_count_z );
}

void
crude_gfx_cmd_bind_vertex_buffer
(
  _In_ crude_gfx_cmd_buffer                               *cmd,
  _In_ crude_gfx_buffer_handle                             handle,
  _In_ uint32                                              binding,
  _In_ uint32                                              offset
)
{
  crude_gfx_buffer *buffer = crude_gfx_access_buffer( cmd->gpu, handle );
  VkDeviceSize offsets[] = { offset };
  
  VkBuffer vk_buffer = buffer->vk_buffer;
  
  if ( CRUDE_RESOURCE_HANDLE_IS_VALID( buffer->parent_buffer ) )
  {
    crude_gfx_buffer *parent_buffer = crude_gfx_access_buffer( cmd->gpu, buffer->parent_buffer );
    vk_buffer = parent_buffer->vk_buffer;
    offsets[ 0 ] = buffer->global_offset;
  }
  
  vkCmdBindVertexBuffers( cmd->vk_cmd_buffer, binding, 1, &vk_buffer, offsets );
}

CRUDE_API void
crude_gfx_cmd_bind_index_buffer
(
  _In_ crude_gfx_cmd_buffer                               *cmd,
  _In_ crude_gfx_buffer_handle                             handle,
  _In_ uint32                                              offset
)
{
  crude_gfx_buffer *buffer = crude_gfx_access_buffer( cmd->gpu, handle );
  
  VkBuffer vk_buffer = buffer->vk_buffer;
  
  if ( CRUDE_RESOURCE_HANDLE_IS_VALID( buffer->parent_buffer ) )
  {
    crude_gfx_buffer *parent_buffer = crude_gfx_access_buffer( cmd->gpu, buffer->parent_buffer );
    vk_buffer = parent_buffer->vk_buffer;
    offset = buffer->global_offset;
  }

  vkCmdBindIndexBuffer( cmd->vk_cmd_buffer, vk_buffer, offset, VK_INDEX_TYPE_UINT16  );
}

crude_gfx_descriptor_set_handle
crude_gfx_cmd_create_local_descriptor_set
(
  _In_ crude_gfx_cmd_buffer                               *cmd,
  _In_ crude_gfx_descriptor_set_creation const            *creation
)
{
  crude_gfx_descriptor_set_handle handle = { crude_resource_pool_obtain_resource( &cmd->frame_descriptor_sets ) };
  if ( CRUDE_RESOURCE_HANDLE_IS_INVALID( handle ) )
  {
    return handle;
  }
  
  crude_gfx_descriptor_set *descriptor_set = CRUDE_REINTERPRET_CAST( crude_gfx_descriptor_set*, crude_resource_pool_access_resource( &cmd->frame_descriptor_sets, handle.index ) );
  crude_gfx_descriptor_set_layout *descriptor_set_layout = crude_gfx_access_descriptor_set_layout( cmd->gpu, creation->layout );
  
  CRUDE_ASSERT( creation->name );
  descriptor_set->name = creation->name;

  VkDescriptorSetAllocateInfo vk_descriptor_info = {
    .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
    .descriptorPool = cmd->vk_descriptor_pool,
    .descriptorSetCount = 1u,
    .pSetLayouts = &descriptor_set_layout->vk_descriptor_set_layout
  };
  CRUDE_GFX_HANDLE_VULKAN_RESULT( vkAllocateDescriptorSets( cmd->gpu->vk_device, &vk_descriptor_info, &descriptor_set->vk_descriptor_set ), "Failed to allocate descriptor set: %s", creation->name ? creation->name : "#noname" );

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
      crude_gfx_buffer *buffer = crude_gfx_access_buffer( cmd->gpu, CRUDE_COMPOUNT( crude_gfx_buffer_handle, { creation->resources[ i ] } ) );
      CRUDE_ASSERT( buffer );
      
      descriptor_write[ i ].descriptorType = ( buffer->usage = CRUDE_GFX_RESOURCE_USAGE_TYPE_DYNAMIC ) ? VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC : VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;

      if ( CRUDE_RESOURCE_HANDLE_IS_VALID( buffer->parent_buffer ) )
      {
        crude_gfx_buffer *parent_buffer = crude_gfx_access_buffer( cmd->gpu, buffer->parent_buffer );
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

  vkUpdateDescriptorSets( cmd->gpu->vk_device, num_resources, descriptor_write, 0, NULL );
  return handle;
}

void
crude_gfx_cmd_bind_descriptor_set
(
  _In_ crude_gfx_cmd_buffer                               *cmd,
  _In_ crude_gfx_descriptor_set_handle                     handle
)
{
  crude_gfx_descriptor_set *descriptor_set = crude_gfx_access_descriptor_set( cmd->gpu, handle );
  crude_gfx_descriptor_set *bindless_descriptor_set = crude_gfx_access_descriptor_set( cmd->gpu, cmd->gpu->bindless_descriptor_set_handle );
  
  uint32 num_offsets = 0u;
  uint32 offsets_cache[ 8 ];
  for ( uint32 i = 0; i < descriptor_set->layout->num_bindings; ++i )
  {
    if ( descriptor_set->layout->bindings[ i ].type == VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER )
    {
        CRUDE_ASSERT( num_offsets < CRUDE_COUNTOF( offsets_cache ) );
        const uint32 resource_index = descriptor_set->bindings[ i ];
        crude_gfx_buffer_handle buffer_handle = { descriptor_set->resources[ resource_index ] };
        crude_gfx_buffer *buffer = crude_gfx_access_buffer( cmd->gpu, buffer_handle );
        offsets_cache[ num_offsets++ ] = buffer->global_offset;
    }
  }
  vkCmdBindDescriptorSets( cmd->vk_cmd_buffer, cmd->current_pipeline->vk_bind_point, cmd->current_pipeline->vk_pipeline_layout, CRUDE_GFX_BINDLESS_DESCRIPTOR_SET_INDEX, 1u, &bindless_descriptor_set->vk_descriptor_set, 0u, NULL );
  vkCmdBindDescriptorSets( cmd->vk_cmd_buffer, cmd->current_pipeline->vk_bind_point, cmd->current_pipeline->vk_pipeline_layout, CRUDE_GFX_MATERIAL_DESCRIPTOR_SET_INDEX, 1u, &descriptor_set->vk_descriptor_set, num_offsets, offsets_cache );
}

void
crude_gfx_cmd_add_image_barrier
(
  _In_ crude_gfx_cmd_buffer                               *cmd,
  _In_ crude_gfx_texture                                  *texture,
  _In_ crude_gfx_resource_state                            new_state,
  _In_ uint32                                              base_mip_level,
  _In_ uint32                                              mip_count,
  _In_ bool                                                is_depth
)
{
  crude_gfx_cmd_add_image_barrier_ext2( cmd, texture->vk_image, texture->state, new_state, base_mip_level, mip_count, is_depth );
  texture->state = new_state;
}

void
crude_gfx_cmd_add_image_barrier_ext
(
  _In_ crude_gfx_cmd_buffer                               *cmd,
  _In_ crude_gfx_texture                                  *texture,
  _In_ crude_gfx_resource_state                            new_state,
  _In_ uint32                                              base_mip_level,
  _In_ uint32                                              mip_count,
  _In_ bool                                                is_depth,
  _In_ uint32                                              source_queue_family,
  _In_ uint32                                              destination_family,
  _In_ crude_gfx_queue_type                                source_queue_type,
  _In_ crude_gfx_queue_type                                destination_queue_type
)
{
  crude_gfx_cmd_add_image_barrier_ext3( cmd, texture->vk_image, texture->state, new_state, base_mip_level, mip_count, is_depth, source_queue_family, destination_family, source_queue_type, destination_queue_type );
  texture->state = new_state;
}

void
crude_gfx_cmd_add_image_barrier_ext2
(
  _In_ crude_gfx_cmd_buffer                               *cmd,
  _In_ VkImage                                             vk_image,
  _In_ crude_gfx_resource_state                            old_state,
  _In_ crude_gfx_resource_state                            new_state,
  _In_ uint32                                              base_mip_level,
  _In_ uint32                                              mip_count,
  _In_ bool                                                is_depth
)
{
  CRUDE_ASSERTM( CRUDE_CHANNEL_GRAPHICS, vk_image != VK_NULL_HANDLE, "Can't add image barrier to the image! image is VK_NULL_HANDLE!" );

  VkAccessFlags2 src_access_mask = crude_gfx_resource_state_to_vk_access_flags2( old_state );
  VkAccessFlags2 dst_access_mask = crude_gfx_resource_state_to_vk_access_flags2( new_state );

  VkImageMemoryBarrier2 barrier = CRUDE_COMPOUNT_EMPTY( VkImageMemoryBarrier2 );
  barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2_KHR;
  barrier.srcStageMask = crude_gfx_determine_pipeline_stage_flags2( src_access_mask, CRUDE_GFX_QUEUE_TYPE_GRAPHICS );
  barrier.srcAccessMask = src_access_mask;
  barrier.dstStageMask = crude_gfx_determine_pipeline_stage_flags2( dst_access_mask, CRUDE_GFX_QUEUE_TYPE_GRAPHICS );
  barrier.dstAccessMask = dst_access_mask;
  barrier.oldLayout = crude_gfx_resource_state_to_vk_image_layout2( old_state );
  barrier.newLayout = crude_gfx_resource_state_to_vk_image_layout2( new_state );
  barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  barrier.image = vk_image;
  barrier.subresourceRange.aspectMask = CRUDE_STATIC_CAST( VkImageAspectFlags, is_depth ? VK_IMAGE_ASPECT_DEPTH_BIT : VK_IMAGE_ASPECT_COLOR_BIT );
  barrier.subresourceRange.baseMipLevel = base_mip_level;
  barrier.subresourceRange.levelCount = mip_count;
  barrier.subresourceRange.baseArrayLayer = 0;
  barrier.subresourceRange.layerCount = 1;

  VkDependencyInfo dependency_info = CRUDE_COMPOUNT_EMPTY( VkDependencyInfo );
  dependency_info.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO_KHR;
  dependency_info.imageMemoryBarrierCount = 1;
  dependency_info.pImageMemoryBarriers = &barrier;

  cmd->gpu->vkCmdPipelineBarrier2KHR( cmd->vk_cmd_buffer, &dependency_info );
}

void
crude_gfx_cmd_add_image_barrier_ext3
(
  _In_ crude_gfx_cmd_buffer                               *cmd,
  _In_ VkImage                                             vk_image,
  _In_ crude_gfx_resource_state                            old_state,
  _In_ crude_gfx_resource_state                            new_state,
  _In_ uint32                                              base_mip_level,
  _In_ uint32                                              mip_count,
  _In_ bool                                                is_depth,
  _In_ uint32                                              source_queue_family,
  _In_ uint32                                              destination_family,
  _In_ crude_gfx_queue_type                                source_queue_type,
  _In_ crude_gfx_queue_type                                destination_queue_type
)
{
  //CRUDE_LOG_INFO( CRUDE_CHANNEL_GRAPHICS, "Transitioning Texture %s from %s to %s", texture->name, crude_gfx_resource_state_to_name( texture->state ), crude_gfx_resource_state_to_name( new_state ) );
  CRUDE_ASSERTM( CRUDE_CHANNEL_GRAPHICS, vk_image != VK_NULL_HANDLE, "Can't add image barrier to the image! image is VK_NULL_HANDLE!" );
  
  VkAccessFlags2 src_access_mask = crude_gfx_resource_state_to_vk_access_flags2( old_state );
  VkAccessFlags2 dst_access_mask = crude_gfx_resource_state_to_vk_access_flags2( new_state );
  VkImageMemoryBarrier2 barrier = CRUDE_COMPOUNT_EMPTY( VkImageMemoryBarrier2 );
  barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2_KHR;
  barrier.srcStageMask = crude_gfx_determine_pipeline_stage_flags2( src_access_mask, source_queue_type );
  barrier.srcAccessMask = src_access_mask;
  barrier.dstStageMask = crude_gfx_determine_pipeline_stage_flags2( dst_access_mask, destination_queue_type );
  barrier.dstAccessMask = dst_access_mask;
  barrier.oldLayout = crude_gfx_resource_state_to_vk_image_layout2( old_state );
  barrier.newLayout = crude_gfx_resource_state_to_vk_image_layout2( new_state );
  barrier.srcQueueFamilyIndex = source_queue_family;
  barrier.dstQueueFamilyIndex = destination_family;
  barrier.image = vk_image;
  barrier.subresourceRange.aspectMask = is_depth ? VK_IMAGE_ASPECT_DEPTH_BIT : VK_IMAGE_ASPECT_COLOR_BIT;
  barrier.subresourceRange.baseMipLevel = base_mip_level;
  barrier.subresourceRange.levelCount = mip_count;
  barrier.subresourceRange.baseArrayLayer = 0;
  barrier.subresourceRange.layerCount = 1;

  VkDependencyInfo dependency_info = CRUDE_COMPOUNT_EMPTY( VkDependencyInfo );
  dependency_info.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO_KHR;
  dependency_info.imageMemoryBarrierCount = 1;
  dependency_info.pImageMemoryBarriers = &barrier;

  cmd->gpu->vkCmdPipelineBarrier2KHR( cmd->vk_cmd_buffer, &dependency_info );
}

void
crude_gfx_cmd_upload_texture_data
(
  _In_ crude_gfx_cmd_buffer                               *cmd,
  _In_ crude_gfx_texture_handle                            texture_handle,
  _In_ void                                               *texture_data,
  _In_ crude_gfx_buffer_handle                             staging_buffer_handle,
  _In_ uint64                                              staging_buffer_offset
)
{
  crude_gfx_texture *texture = crude_gfx_access_texture( cmd->gpu, texture_handle );
  crude_gfx_buffer *staging_buffer = crude_gfx_access_buffer( cmd->gpu, staging_buffer_handle );
  uint32 image_size = texture->width * texture->height * 4u;
  memcpy( staging_buffer->mapped_data + staging_buffer_offset, texture_data, image_size );

  VkBufferImageCopy region = {
    .bufferOffset = staging_buffer_offset,
    .bufferRowLength = 0,
    .bufferImageHeight = 0,
    
    .imageSubresource = {
      .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
      .mipLevel = 0,
      .baseArrayLayer = 0,
      .layerCount = 1,
    },
    
    .imageOffset = { 0, 0, 0 },
    .imageExtent = { texture->width, texture->height, texture->depth },
  };

  crude_gfx_cmd_add_image_barrier( cmd, texture, CRUDE_GFX_RESOURCE_STATE_COPY_DEST, 0, 1, false );
  vkCmdCopyBufferToImage( cmd->vk_cmd_buffer, staging_buffer->vk_buffer, texture->vk_image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region );
  crude_gfx_cmd_add_image_barrier_ext( cmd, texture, CRUDE_GFX_RESOURCE_STATE_COPY_SOURCE, 0, 1, false, cmd->gpu->vk_transfer_queue_family, cmd->gpu->vk_main_queue_family, CRUDE_GFX_QUEUE_TYPE_COPY_TRANSFER, CRUDE_GFX_QUEUE_TYPE_GRAPHICS );
}

void
crude_gfx_cmd_upload_buffer_data
(
  _In_ crude_gfx_cmd_buffer                               *cmd,
  _In_ crude_gfx_buffer_handle                             src_buffer,
  _In_ crude_gfx_buffer_handle                             dst_buffer
)
{
  crude_gfx_buffer* src = crude_gfx_access_buffer( cmd->gpu, src_buffer );
  crude_gfx_buffer* dst = crude_gfx_access_buffer( cmd->gpu, dst_buffer );
  
  CRUDE_ASSERT( src->size == dst->size );
  
  uint32 copy_size = src->size;

  VkBufferCopy region = {
    .srcOffset = 0,
    .dstOffset = 0,
    .size = copy_size,
  };
  
  vkCmdCopyBuffer( cmd->vk_cmd_buffer, src->vk_buffer, dst->vk_buffer, 1, &region );
}

void
crude_gfx_cmd_push_marker
(
  _In_ crude_gfx_cmd_buffer                               *cmd,
  _In_ char const                                         *name
)
{
  VkDebugUtilsLabelEXT vk_label = CRUDE_COMPOUNT_EMPTY( VkDebugUtilsLabelEXT );
  vk_label.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_LABEL_EXT;
  vk_label.pLabelName = name;
  vk_label.color[ 0 ] = 1.0f;
  vk_label.color[ 1 ] = 1.0f;
  vk_label.color[ 2 ] = 1.0f;
  vk_label.color[ 3 ] = 1.0f;
  cmd->gpu->vkCmdBeginDebugUtilsLabelEXT( cmd->vk_cmd_buffer, &vk_label );
}

void
crude_gfx_cmd_pop_marker
(
  _In_ crude_gfx_cmd_buffer                               *cmd
)
{
  cmd->gpu->vkCmdEndDebugUtilsLabelEXT( cmd->vk_cmd_buffer );
}

/************************************************
 *
 * Command Buffer Manager Functions
 * 
 ***********************************************/
void
crude_gfx_cmd_manager_initialize
(
  _In_ crude_gfx_cmd_buffer_manager                       *cmd_manager,
  _In_ crude_gfx_device                                   *gpu,
  _In_ uint32                                              num_threads
)
{
  crude_allocator_container temporary_allocator_container = crude_stack_allocator_pack( gpu->temporary_allocator );

  cmd_manager->gpu = gpu;
  cmd_manager->num_pools_per_frame = num_threads;

  cmd_manager->num_primary_cmd_buffers_per_thread = 3;
  cmd_manager->num_secondary_cmd_buffer_per_pool = 5;

  uint32 total_pools = cmd_manager->num_pools_per_frame * CRUDE_GFX_MAX_SWAPCHAIN_IMAGES;

  CRUDE_ARRAY_INITIALIZE_WITH_LENGTH( cmd_manager->num_used_primary_cmd_buffers_per_frame, total_pools, gpu->allocator_container );
  CRUDE_ARRAY_INITIALIZE_WITH_LENGTH( cmd_manager->num_used_secondary_cmd_buffers_per_frame, total_pools, gpu->allocator_container );
  for ( uint32 i = 0; i < total_pools; ++i )
  {
    cmd_manager->num_used_primary_cmd_buffers_per_frame[ i ] = 0;
    cmd_manager->num_used_secondary_cmd_buffers_per_frame[ i ] = 0;
  }
  
  uint32 total_buffers = total_pools * cmd_manager->num_primary_cmd_buffers_per_thread;
  CRUDE_ARRAY_INITIALIZE_WITH_LENGTH( cmd_manager->primary_cmd_buffers, total_buffers, gpu->allocator_container );
  
  for ( uint32 i = 0; i < total_buffers; i++ )
  {
    uint32 frame_index = i / ( cmd_manager->num_primary_cmd_buffers_per_thread * cmd_manager->num_pools_per_frame );
    uint32 thread_index = ( i / cmd_manager->num_primary_cmd_buffers_per_thread ) % cmd_manager->num_pools_per_frame;
    uint32 pool_index = pool_from_indices( cmd_manager, frame_index, thread_index );

    VkCommandBufferAllocateInfo cmd = {
      .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
      .commandPool = cmd_manager->gpu->thread_frame_pools[ pool_index ].vk_command_pool,
      .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
      .commandBufferCount = 1,
    };
    
    crude_gfx_cmd_buffer *current_cmd_buffer = &cmd_manager->primary_cmd_buffers[ i ];
    CRUDE_GFX_HANDLE_VULKAN_RESULT( vkAllocateCommandBuffers( gpu->vk_device, &cmd, &current_cmd_buffer->vk_cmd_buffer ), "Failed to allocate command buffer" );
    crude_gfx_cmd_initialize( current_cmd_buffer, gpu );
  }
  
  uint32 total_secondary_buffers = total_pools * cmd_manager->num_secondary_cmd_buffer_per_pool;
  CRUDE_ARRAY_INITIALIZE_WITH_LENGTH( cmd_manager->secondary_cmd_buffers, total_secondary_buffers, gpu->allocator_container );

  uint32 temporary_allocator_mark = crude_stack_allocator_get_marker( gpu->temporary_allocator );
  for ( uint32 pool_index = 0; pool_index < total_pools; ++pool_index )
  {
    VkCommandBufferAllocateInfo cmd = {
      .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO, 
      .commandPool = cmd_manager->gpu->thread_frame_pools[ pool_index ].vk_command_pool, 
      .level = VK_COMMAND_BUFFER_LEVEL_SECONDARY,
      .commandBufferCount = cmd_manager->num_secondary_cmd_buffer_per_pool
    };
    
    VkCommandBuffer *secondary_buffers = NULL;
    CRUDE_ARRAY_INITIALIZE_WITH_LENGTH( secondary_buffers, cmd_manager->num_secondary_cmd_buffer_per_pool, temporary_allocator_container );

    vkAllocateCommandBuffers( gpu->vk_device, &cmd, secondary_buffers );
    for ( uint32 second_cmd_index = 0; second_cmd_index < cmd_manager->num_secondary_cmd_buffer_per_pool; ++second_cmd_index )
    {
      crude_gfx_cmd_buffer cmd;
      cmd.vk_cmd_buffer = secondary_buffers[ second_cmd_index ];
      crude_gfx_cmd_initialize( &cmd, gpu );
      
      cmd_manager->secondary_cmd_buffers[ pool_index * cmd_manager->num_secondary_cmd_buffer_per_pool + second_cmd_index ] = cmd;
    }
  }
  crude_stack_allocator_free_marker( gpu->temporary_allocator, temporary_allocator_mark );
}

void
crude_gfx_cmd_manager_deinitialize
(
  _In_ crude_gfx_cmd_buffer_manager                       *cmd_manager
)
{
  for ( uint32 i = 0; i < CRUDE_ARRAY_LENGTH( cmd_manager->primary_cmd_buffers ) ; ++i )
  {
    crude_gfx_cmd_deinitialize( &cmd_manager->primary_cmd_buffers[ i ] );
  }
  for ( uint32 i = 0; i < CRUDE_ARRAY_LENGTH( cmd_manager->secondary_cmd_buffers ) ; ++i )
  {
    crude_gfx_cmd_deinitialize( &cmd_manager->secondary_cmd_buffers[ i ] );
  }
  CRUDE_ARRAY_DEINITIALIZE( cmd_manager->primary_cmd_buffers );
  CRUDE_ARRAY_DEINITIALIZE( cmd_manager->secondary_cmd_buffers );
  CRUDE_ARRAY_DEINITIALIZE( cmd_manager->num_used_primary_cmd_buffers_per_frame );
  CRUDE_ARRAY_DEINITIALIZE( cmd_manager->num_used_secondary_cmd_buffers_per_frame );
}

void
crude_gfx_cmd_manager_reset
(
  _In_ crude_gfx_cmd_buffer_manager                       *cmd_manager,
  _In_ uint32                                              frame
)
{
  for ( uint32 i = 0; i < cmd_manager->num_pools_per_frame; ++i )
  {
    uint32 pool_index = pool_from_indices( cmd_manager, frame, i );
    vkResetCommandPool( cmd_manager->gpu->vk_device, cmd_manager->gpu->thread_frame_pools[ pool_index ].vk_command_pool, 0 );
  }

  for ( uint32 i = 0; i < cmd_manager->num_pools_per_frame; ++i )
  {
    uint32 pool_index = pool_from_indices( cmd_manager, frame, i );
    
    for ( uint32 i = 0; i < cmd_manager->num_used_secondary_cmd_buffers_per_frame[ pool_index ]; ++i )
    {
      crude_gfx_cmd_buffer *secondary_cmd = &cmd_manager->secondary_cmd_buffers[ ( pool_index * cmd_manager->num_secondary_cmd_buffer_per_pool ) + i ];
      crude_gfx_cmd_reset( secondary_cmd );
    }
    cmd_manager->num_used_secondary_cmd_buffers_per_frame[ pool_index ] = 0;

    cmd_manager->num_used_primary_cmd_buffers_per_frame[ pool_index ] = 0;
  }
}

crude_gfx_cmd_buffer*
crude_gfx_cmd_manager_get_primary_cmd
(
  _In_ crude_gfx_cmd_buffer_manager                       *cmd_manager,
  _In_ uint32                                              frame,
  _In_ uint32                                              thread_index,
  _In_ bool                                                begin
)
{
  uint32 pool_index = pool_from_indices( cmd_manager, frame, thread_index );
  uint32 current_used_buffer = cmd_manager->num_used_primary_cmd_buffers_per_frame[ pool_index ];
  uint32 cmd_index = ( pool_index * cmd_manager->num_primary_cmd_buffers_per_thread ) + current_used_buffer;
  crude_gfx_cmd_buffer *cmd = &cmd_manager->primary_cmd_buffers[ cmd_index ];
  
  if ( begin )
  {  
    crude_gfx_cmd_reset( cmd );
    crude_gfx_cmd_begin_primary( cmd );
    CRUDE_ASSERT( current_used_buffer < cmd_manager->num_primary_cmd_buffers_per_thread );
    cmd_manager->num_used_primary_cmd_buffers_per_frame[ pool_index ] = current_used_buffer + 1;
  }
  
  return cmd;
}

crude_gfx_cmd_buffer*
crude_gfx_cmd_manager_get_secondary_cmd
(
  _In_ crude_gfx_cmd_buffer_manager                       *cmd_manager,
  _In_ uint32                                              frame,
  _In_ uint32                                              thread_index
)
{
  uint32 pool_index = pool_from_indices( cmd_manager, frame, thread_index );
  uint32 current_used_buffer = cmd_manager->num_used_secondary_cmd_buffers_per_frame[ pool_index ];
  uint32 cmd_index = ( pool_index * cmd_manager->num_secondary_cmd_buffer_per_pool ) + current_used_buffer;

  CRUDE_ASSERT( current_used_buffer < cmd_manager->num_secondary_cmd_buffer_per_pool );
  cmd_manager->num_used_secondary_cmd_buffers_per_frame[ pool_index ] = current_used_buffer + 1;
  
  crude_gfx_cmd_buffer *cmd = &cmd_manager->secondary_cmd_buffers[ cmd_index ];
  return cmd;
}