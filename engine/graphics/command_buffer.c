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
crude_gfx_initialize_cmd
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
    .maxSets       = global_pool_elements * CRUDE_STACK_ARRAY_SIZE( pool_sizes ),
    .poolSizeCount = CRUDE_STACK_ARRAY_SIZE( pool_sizes ),
    .pPoolSizes    = pool_sizes,
  };
  
  CRUDE_GFX_HANDLE_VULKAN_RESULT( vkCreateDescriptorPool( cmd->gpu->vk_device, &pool_info, cmd->gpu->vk_allocation_callbacks, &cmd->vk_descriptor_pool ), "Failed create descriptor pool" );
  
  crude_initialize_resource_pool( &cmd->frame_descriptor_sets, cmd->gpu->allocator, 256, sizeof( crude_gfx_descriptor_set ) );
  
  crude_gfx_cmd_reset( cmd );
}

CRUDE_API void
crude_gfx_deinitialize_cmd
(
  _In_ crude_gfx_cmd_buffer                               *cmd
)
{
  crude_gfx_cmd_reset( cmd );
  crude_deinitialize_resource_pool( &cmd->frame_descriptor_sets );
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
    CRUDE_RELEASE_RESOURCE( cmd->frame_descriptor_sets, ( crude_gfx_descriptor_set_handle){ i } );
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
  _In_ crude_gfx_render_pass                              *render_pass
)
{
  if ( cmd->is_recording )
  {
    return;
  }

  VkCommandBufferInheritanceInfo inheritance = {
    .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_INHERITANCE_INFO,
    .renderPass = render_pass->vk_render_pass,
    .subpass = 0,
    .framebuffer = render_pass->vk_frame_buffer,
  };

  VkCommandBufferBeginInfo begin_info = {
    .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
    .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT | VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT,
    .pInheritanceInfo = &inheritance,
  };

  vkBeginCommandBuffer( cmd->vk_cmd_buffer, &begin_info );

  cmd->is_recording = true;
  cmd->current_render_pass = render_pass;
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
  if ( cmd->is_recording && cmd->current_render_pass )
  {
    vkCmdEndRenderPass( cmd->vk_cmd_buffer );
    cmd->current_render_pass = NULL;
  }
}

void
crude_gfx_cmd_bind_render_pass
(
  _In_ crude_gfx_cmd_buffer                               *cmd,
  _In_ crude_gfx_render_pass_handle                        handle,
  _In_ bool                                                use_secondary
)
{
  cmd->is_recording = true;
  
  crude_gfx_render_pass *render_pass = CRUDE_GFX_ACCESS_RENDER_PASS( cmd->gpu, handle );
  if ( !render_pass )
  {
    CRUDE_LOG_ERROR( CRUDE_CHANNEL_GRAPHICS, "Failed to bind render pass! Invalid render pass %u!", handle.index );
    return;
  }

  if ( render_pass == cmd->current_render_pass )
  {
    CRUDE_LOG_WARNING( CRUDE_CHANNEL_GRAPHICS, "Binding same render pass %s %u!", render_pass->name, handle.index );
    return;
  }

  if ( cmd->current_render_pass && ( cmd->current_render_pass->type != CRUDE_GFX_RENDER_PASS_TYPE_COMPUTE ) )
  {
    vkCmdEndRenderPass( cmd->vk_cmd_buffer );
  }
  
  if ( render_pass->type != CRUDE_GFX_RENDER_PASS_TYPE_COMPUTE )
  {
    VkRenderPassBeginInfo render_pass_begin = {
      .sType             = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
      .framebuffer       = render_pass->type == CRUDE_GFX_RENDER_PASS_TYPE_SWAPCHAIN ? cmd->gpu->vk_swapchain_framebuffers[ cmd->gpu->vk_swapchain_image_index ] : render_pass->vk_frame_buffer,
      .renderPass        = render_pass->vk_render_pass,
      .renderArea.offset = { 0, 0 },
      .renderArea.extent = { render_pass->width, render_pass->height },
      .clearValueCount   = 2,
      .pClearValues      = cmd->clears,
    };
    
    vkCmdBeginRenderPass( cmd->vk_cmd_buffer, &render_pass_begin, use_secondary ? VK_SUBPASS_CONTENTS_SECONDARY_COMMAND_BUFFERS : VK_SUBPASS_CONTENTS_INLINE );
  }

  cmd->current_render_pass = render_pass;
}

void
crude_gfx_cmd_bind_pipeline
(
  _In_ crude_gfx_cmd_buffer                               *cmd,
  _In_ crude_gfx_pipeline_handle                           handle
)
{
  crude_gfx_pipeline *pipeline = CRUDE_GFX_ACCESS_PIPELINE( cmd->gpu, handle );
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
      vk_viewport.width = cmd->current_render_pass->width * 1.f;
      vk_viewport.y = cmd->current_render_pass->height * 1.f;
      vk_viewport.height = -cmd->current_render_pass->height * 1.f;
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
  crude_gfx_descriptor_set *descriptor_set = CRUDE_ACCESS_RESOURCE( cmd->frame_descriptor_sets, crude_gfx_descriptor_set, handle );
  
  uint32 num_offsets = 0u;
  uint32 offsets_cache[ 8 ];
  for ( uint32 i = 0; i < descriptor_set->layout->num_bindings; ++i )
  {
    if ( descriptor_set->layout->bindings[ i ].type == VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER )
    {
        CRUDE_ASSERT( num_offsets < CRUDE_STACK_ARRAY_SIZE( offsets_cache ) );
        const uint32 resource_index = descriptor_set->bindings[ i ];
        crude_gfx_buffer_handle buffer_handle = { descriptor_set->resources[ resource_index ] };
        crude_gfx_buffer *buffer = CRUDE_GFX_ACCESS_BUFFER( cmd->gpu, buffer_handle );
        offsets_cache[ num_offsets++ ] = buffer->global_offset;
    }
  }
  vkCmdBindDescriptorSets( cmd->vk_cmd_buffer, cmd->current_pipeline->vk_bind_point, cmd->current_pipeline->vk_pipeline_layout, 0u, 1u, &descriptor_set->vk_descriptor_set, num_offsets, offsets_cache );
  vkCmdBindDescriptorSets( cmd->vk_cmd_buffer, cmd->current_pipeline->vk_bind_point, cmd->current_pipeline->vk_pipeline_layout, 1u, 1u, &cmd->gpu->vk_bindless_descriptor_set, 0u, NULL );
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
crude_gfx_cmd_bind_vertex_buffer
(
  _In_ crude_gfx_cmd_buffer                               *cmd,
  _In_ crude_gfx_buffer_handle                             handle,
  _In_ uint32                                              binding,
  _In_ uint32                                              offset
)
{
  crude_gfx_buffer *buffer = CRUDE_GFX_ACCESS_BUFFER( cmd->gpu, handle );
  VkDeviceSize offsets[] = { offset };
  
  VkBuffer vk_buffer = buffer->vk_buffer;
  
  if ( CRUDE_GFX_IS_HANDLE_VALID( buffer->parent_buffer ) )
  {
    crude_gfx_buffer *parent_buffer = CRUDE_GFX_ACCESS_BUFFER( cmd->gpu, buffer->parent_buffer );
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
  crude_gfx_buffer *buffer = CRUDE_GFX_ACCESS_BUFFER( cmd->gpu, handle );
  
  VkBuffer vk_buffer = buffer->vk_buffer;
  
  if ( CRUDE_GFX_IS_HANDLE_VALID( buffer->parent_buffer ) )
  {
    crude_gfx_buffer *parent_buffer = CRUDE_GFX_ACCESS_BUFFER( cmd->gpu, buffer->parent_buffer );
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
  CRUDE_PROFILER_ZONE_NAME( "CreateLocalDescriptorSet" );
  crude_gfx_descriptor_set_handle handle = { CRUDE_OBTAIN_RESOURCE( cmd->frame_descriptor_sets ) };
  if ( CRUDE_GFX_IS_HANDLE_INVALID( handle ) )
  {
    CRUDE_PROFILER_END;
    return handle;
  }
  
  crude_gfx_descriptor_set *descriptor_set = CRUDE_ACCESS_RESOURCE( cmd->frame_descriptor_sets, crude_gfx_descriptor_set , handle );
  crude_gfx_descriptor_set_layout *descriptor_set_layout = CRUDE_GFX_ACCESS_DESCRIPTOR_SET_LAYOUT( cmd->gpu, creation->layout );
  
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
      crude_gfx_buffer *buffer = CRUDE_GFX_ACCESS_BUFFER( cmd->gpu, ( crude_gfx_buffer_handle ){ creation->resources[ i ] } );
      CRUDE_ASSERT( buffer );
      
      descriptor_write[ i ].descriptorType = ( buffer->usage = CRUDE_GFX_RESOURCE_USAGE_TYPE_DYNAMIC ) ? VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC : VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;

      if ( CRUDE_GFX_IS_HANDLE_VALID( buffer->parent_buffer ) )
      {
        crude_gfx_buffer *parent_buffer = CRUDE_GFX_ACCESS_BUFFER( cmd->gpu, buffer->parent_buffer );
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
  CRUDE_PROFILER_END;
  return handle;
}

void
crude_gfx_cmd_add_image_barrier
(
  _In_ crude_gfx_cmd_buffer                               *cmd,
  _In_ VkImage                                             image,
  _In_ crude_gfx_resource_state                            old_state,
  _In_ crude_gfx_resource_state                            new_state,
  _In_ uint32                                              base_mip_level,
  _In_ uint32                                              mip_count,
  _In_ bool                                                is_depth
)
{
  VkImageMemoryBarrier barrier = {
    .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
    .image = image,
    .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
    .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
    .subresourceRange.aspectMask = is_depth ? VK_IMAGE_ASPECT_DEPTH_BIT : VK_IMAGE_ASPECT_COLOR_BIT,
    .subresourceRange.baseArrayLayer = 0,
    .subresourceRange.layerCount = 1,
    .subresourceRange.levelCount = mip_count,

    .subresourceRange.baseMipLevel = base_mip_level,
    .oldLayout = crude_gfx_resource_state_to_vk_image_layout( old_state ),
    .newLayout = crude_gfx_resource_state_to_vk_image_layout( new_state ),
    .srcAccessMask = crude_gfx_resource_state_to_vk_access_flags( old_state ),
    .dstAccessMask = crude_gfx_resource_state_to_vk_access_flags( new_state ),
  };
  
  VkPipelineStageFlags source_stage_mask = crude_gfx_determine_pipeline_stage_flags( barrier.srcAccessMask, CRUDE_GFX_QUEUE_TYPE_GRAPHICS );
  VkPipelineStageFlags destination_stage_mask = crude_gfx_determine_pipeline_stage_flags( barrier.dstAccessMask, CRUDE_GFX_QUEUE_TYPE_GRAPHICS );
  
  vkCmdPipelineBarrier( cmd->vk_cmd_buffer, source_stage_mask, destination_stage_mask, 0, 0, NULL, 0, NULL, 1, &barrier );
}

VkImageLayout
crude_gfx_cmd_add_image_barrier_ext
(
  _In_ crude_gfx_cmd_buffer                               *cmd,
  _In_ VkImage                                             image,
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
  VkImageMemoryBarrier barrier = {
    .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
    .image = image,
    .srcQueueFamilyIndex = source_queue_family,
    .dstQueueFamilyIndex = destination_family,
    .subresourceRange.aspectMask = is_depth ? VK_IMAGE_ASPECT_DEPTH_BIT : VK_IMAGE_ASPECT_COLOR_BIT,
    .subresourceRange.baseArrayLayer = 0,
    .subresourceRange.layerCount = 1,
    .subresourceRange.levelCount = mip_count,

    .subresourceRange.baseMipLevel = base_mip_level,
    .oldLayout = crude_gfx_resource_state_to_vk_image_layout( old_state ),
    .newLayout = crude_gfx_resource_state_to_vk_image_layout( new_state ),
    .srcAccessMask = crude_gfx_resource_state_to_vk_access_flags( old_state ),
    .dstAccessMask = crude_gfx_resource_state_to_vk_access_flags( new_state ),
  };
  
  VkPipelineStageFlags source_stage_mask = crude_gfx_determine_pipeline_stage_flags( barrier.srcAccessMask, source_queue_type );
  VkPipelineStageFlags destination_stage_mask = crude_gfx_determine_pipeline_stage_flags( barrier.dstAccessMask, destination_queue_type );
  
  vkCmdPipelineBarrier( cmd->vk_cmd_buffer, source_stage_mask, destination_stage_mask, 0, 0, NULL, 0, NULL, 1, &barrier );
  return barrier.newLayout;
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
  crude_gfx_texture *texture = CRUDE_GFX_ACCESS_TEXTURE( cmd->gpu, texture_handle );
  crude_gfx_buffer *staging_buffer = CRUDE_GFX_ACCESS_BUFFER( cmd->gpu, staging_buffer_handle );
  uint32 image_size = texture->width * texture->height * 4u;
  memcpy( staging_buffer->mapped_data + staging_buffer_offset, texture_data, image_size );

  VkBufferImageCopy region = {
    .bufferOffset = staging_buffer_offset,
    .bufferRowLength = 0,
    .bufferImageHeight = 0,
    
    .imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
    .imageSubresource.mipLevel = 0,
    .imageSubresource.baseArrayLayer = 0,
    .imageSubresource.layerCount = 1,
    
    .imageOffset = { 0, 0, 0 },
    .imageExtent = { texture->width, texture->height, texture->depth },
  };

  crude_gfx_cmd_add_image_barrier( cmd, texture->vk_image, CRUDE_GFX_RESOURCE_STATE_UNDEFINED, CRUDE_GFX_RESOURCE_STATE_COPY_DEST, 0, 1, false );
  vkCmdCopyBufferToImage( cmd->vk_cmd_buffer, staging_buffer->vk_buffer, texture->vk_image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region );
  crude_gfx_cmd_add_image_barrier_ext( cmd, texture->vk_image, CRUDE_GFX_RESOURCE_STATE_COPY_DEST, CRUDE_GFX_RESOURCE_STATE_COPY_SOURCE, 0, 1, false, cmd->gpu->vk_transfer_queue_family, cmd->gpu->vk_main_queue_family, CRUDE_GFX_QUEUE_TYPE_COPY_TRANSFER, CRUDE_GFX_QUEUE_TYPE_GRAPHICS );

  texture->vk_image_layout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
}

void
crude_gfx_cmd_upload_buffer_data
(
  _In_ crude_gfx_cmd_buffer                               *cmd,
  _In_ crude_gfx_buffer_handle                             src_buffer,
  _In_ crude_gfx_buffer_handle                             dst_buffer
)
{
  crude_gfx_buffer* src = CRUDE_GFX_ACCESS_BUFFER( cmd->gpu, src_buffer );
  crude_gfx_buffer* dst = CRUDE_GFX_ACCESS_BUFFER( cmd->gpu, dst_buffer );
  
  CRUDE_ASSERT( src->size == dst->size );
  
  uint32 copy_size = src->size;

  VkBufferCopy region = {
    .srcOffset = 0,
    .dstOffset = 0,
    .size = copy_size,
  };
  
  vkCmdCopyBuffer( cmd->vk_cmd_buffer, src->vk_buffer, dst->vk_buffer, 1, &region );
}

/************************************************
 *
 * Command Buffer Manager Functions
 * 
 ***********************************************/
void
crude_gfx_initialize_cmd_manager
(
  _In_ crude_gfx_cmd_buffer_manager                       *cmd_manager,
  _In_ crude_gfx_device                                   *gpu,
  _In_ uint32                                              num_threads
)
{
  cmd_manager->gpu = gpu;
  cmd_manager->num_pools_per_frame = num_threads;

  cmd_manager->num_primary_cmd_buffers_per_thread = 1;
  cmd_manager->num_secondary_cmd_buffer_per_pool = 5;

  uint32 total_pools = cmd_manager->num_pools_per_frame * CRUDE_GFX_MAX_SWAPCHAIN_IMAGES;
  CRUDE_ARRAY_SET_LENGTH( cmd_manager->vk_cmd_pools, total_pools );

  for ( uint32 i = 0; i < total_pools; ++i )
  {
    VkCommandPoolCreateInfo cmd_pool_info = {
      .sType            = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
      .queueFamilyIndex = gpu->vk_main_queue_family,
      .flags            = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
    };
    
    CRUDE_GFX_HANDLE_VULKAN_RESULT( vkCreateCommandPool( gpu->vk_device, &cmd_pool_info, gpu->vk_allocation_callbacks, &cmd_manager->vk_cmd_pools[ i ] ), "Failed to create command pool" );
  }
  
  CRUDE_ARRAY_SET_LENGTH( cmd_manager->num_used_primary_cmd_buffers_per_frame, total_pools );
  CRUDE_ARRAY_SET_LENGTH( cmd_manager->num_used_secondary_cmd_buffers_per_frame, total_pools );
  for ( uint32 i = 0; i < total_pools; ++i )
  {
    cmd_manager->num_used_primary_cmd_buffers_per_frame[ i ] = 0;
    cmd_manager->num_used_secondary_cmd_buffers_per_frame[ i ] = 0;
  }
  
  uint32 total_buffers = total_pools * cmd_manager->num_primary_cmd_buffers_per_thread;
  CRUDE_ARRAY_SET_LENGTH( cmd_manager->primary_cmd_buffers, total_pools );
  
  for ( uint32 i = 0; i < total_buffers; i++ )
  {
    uint32 frame_index = i / ( cmd_manager->num_primary_cmd_buffers_per_thread * cmd_manager->num_pools_per_frame );
    uint32 thread_index = ( i / cmd_manager->num_primary_cmd_buffers_per_thread ) % cmd_manager->num_pools_per_frame;
    uint32 pool_index = pool_from_indices( cmd_manager, frame_index, thread_index );

    VkCommandBufferAllocateInfo cmd = {
      .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
      .commandPool = cmd_manager->vk_cmd_pools[ pool_index ],
      .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
      .commandBufferCount = 1,
    };
    
    crude_gfx_cmd_buffer *current_cmd_buffer = &cmd_manager->primary_cmd_buffers[ i ];
    CRUDE_GFX_HANDLE_VULKAN_RESULT( vkAllocateCommandBuffers( gpu->vk_device, &cmd, &current_cmd_buffer->vk_cmd_buffer ), "Failed to allocate command buffer" );
    crude_gfx_initialize_cmd( current_cmd_buffer, gpu );
  }
  
  uint32 total_secondary_buffers = total_pools * cmd_manager->num_secondary_cmd_buffer_per_pool;
  CRUDE_ARRAY_SET_LENGTH( cmd_manager->secondary_cmd_buffers, total_secondary_buffers );

  for ( uint32 pool_index = 0; pool_index < total_pools; ++pool_index )
  {
    VkCommandBufferAllocateInfo cmd = {
      .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO, 
      .commandPool = cmd_manager->vk_cmd_pools[ pool_index ], 
      .level = VK_COMMAND_BUFFER_LEVEL_SECONDARY,
      .commandBufferCount = cmd_manager->num_secondary_cmd_buffer_per_pool
    };
    
    VkCommandBuffer *secondary_buffers = NULL;
    CRUDE_ARRAY_SET_LENGTH( secondary_buffers, cmd_manager->num_secondary_cmd_buffer_per_pool );

    vkAllocateCommandBuffers( gpu->vk_device, &cmd, secondary_buffers );
    
    for ( uint32 second_cmd_index = 0; second_cmd_index < cmd_manager->num_secondary_cmd_buffer_per_pool; ++second_cmd_index )
    {
      crude_gfx_cmd_buffer cmd;
      cmd.vk_cmd_buffer = secondary_buffers[ second_cmd_index ];
      crude_gfx_initialize_cmd( &cmd, gpu );
      
      cmd_manager->secondary_cmd_buffers[ pool_index * cmd_manager->num_secondary_cmd_buffer_per_pool + second_cmd_index ] = cmd;
    }

    CRUDE_ARRAY_FREE( secondary_buffers );
  }
}

void
crude_gfx_deinitialize_cmd_manager
(
  _In_ crude_gfx_cmd_buffer_manager                       *cmd_manager
)
{
  for ( uint32 i = 0; i < CRUDE_ARRAY_LENGTH( cmd_manager->primary_cmd_buffers ) ; ++i )
  {
    crude_gfx_deinitialize_cmd( &cmd_manager->primary_cmd_buffers[ i ] );
  }
  for ( uint32 i = 0; i < CRUDE_ARRAY_LENGTH( cmd_manager->secondary_cmd_buffers ) ; ++i )
  {
    crude_gfx_deinitialize_cmd( &cmd_manager->secondary_cmd_buffers[ i ] );
  }
  for ( uint32 i = 0; i < CRUDE_ARRAY_LENGTH( cmd_manager->vk_cmd_pools ) ; ++i )
  {
    vkDestroyCommandPool( cmd_manager->gpu->vk_device, cmd_manager->vk_cmd_pools[ i ], cmd_manager->gpu->vk_allocation_callbacks );
  }
  CRUDE_ARRAY_FREE( cmd_manager->vk_cmd_pools  );
  CRUDE_ARRAY_FREE( cmd_manager->primary_cmd_buffers );
  CRUDE_ARRAY_FREE( cmd_manager->secondary_cmd_buffers );
  CRUDE_ARRAY_FREE( cmd_manager->num_used_primary_cmd_buffers_per_frame );
  CRUDE_ARRAY_FREE( cmd_manager->num_used_secondary_cmd_buffers_per_frame );
}

void
crude_gfx_cmd_manager_reset
(
  _In_ crude_gfx_cmd_buffer_manager                       *cmd_manager,
  _In_ uint32                                              frame
)
{
  CRUDE_PROFILER_ZONE_NAME( "ResetCommandBufferManager" );
  for ( uint32 i = 0; i < cmd_manager->num_pools_per_frame; ++i )
  {
    uint32 pool_index = pool_from_indices( cmd_manager, frame, i );
    vkResetCommandPool( cmd_manager->gpu->vk_device, cmd_manager->vk_cmd_pools[ pool_index ], 0 );
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
  CRUDE_PROFILER_END;
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