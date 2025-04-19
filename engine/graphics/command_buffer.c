#include <graphics/gpu_device.h>
#include <core/log.h>
#include <core/assert.h>

#include <graphics/command_buffer.h>

void
crude_gfx_cmd_reset
(
  _In_ crude_command_buffer *cmd
)
{
  cmd->is_recording = false;
  cmd->current_render_pass = NULL;
  
  vkResetDescriptorPool( cmd->gpu->vk_device, cmd->vk_descriptor_pool, 0 );
  
  uint32 resource_count = cmd->frame_descriptor_sets.free_indices_head;
  for ( uint32 i = 0; i < resource_count; ++i )
  {
    CRUDE_RELEASE_RESOURCE( cmd->frame_descriptor_sets, ( crude_descriptor_set_handle){ i } );
  }
}

void
crude_gfx_cmd_bind_render_pass
(
  _In_ crude_command_buffer     *cmd,
  _In_ crude_render_pass_handle  handle
)
{
  cmd->is_recording = true;
  
  crude_render_pass *render_pass = CRUDE_GFX_GPU_ACCESS_RENDER_PASS( cmd->gpu, handle );
  if ( !render_pass )
  {
    CRUDE_LOG_ERROR( CRUDE_CHANNEL_GRAPHICS, "Failed to bind render pass! Invalid render pass %u!", handle.index );
  }

  if ( render_pass == cmd->current_render_pass )
  {
    CRUDE_LOG_WARNING( CRUDE_CHANNEL_GRAPHICS, "Binding same render pass %s %u!", render_pass->name, handle.index );
    return;
  }

  if ( cmd->current_render_pass && ( cmd->current_render_pass->type != CRUDE_RENDER_PASS_TYPE_COMPUTE ) )
  {
    vkCmdEndRenderPass( cmd->vk_cmd_buffer );
  }
  
  if ( render_pass->type != CRUDE_RENDER_PASS_TYPE_COMPUTE )
  {
    VkRenderPassBeginInfo render_pass_begin = {
      .sType             = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
      .framebuffer       = render_pass->type == CRUDE_RENDER_PASS_TYPE_SWAPCHAIN ? cmd->gpu->vk_swapchain_framebuffers[ cmd->gpu->vk_swapchain_image_index ] : render_pass->vk_frame_buffer,
      .renderPass        = render_pass->vk_render_pass,
      .renderArea.offset = { 0, 0 },
      .renderArea.extent = { render_pass->width, render_pass->height },
      .clearValueCount   = 2,
      .pClearValues      = cmd->clears,
    };
    
    vkCmdBeginRenderPass( cmd->vk_cmd_buffer, &render_pass_begin, VK_SUBPASS_CONTENTS_INLINE );
  }
  
  cmd->current_render_pass = render_pass;
}

void
crude_gfx_cmd_bind_pipeline
(
  _In_ crude_command_buffer   *cmd,
  _In_ crude_pipeline_handle   handle
)
{
  crude_pipeline *pipeline = CRUDE_GFX_GPU_ACCESS_PIPELINE( cmd->gpu, handle );
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
  _In_ crude_command_buffer     *cmd,
  _In_opt_ crude_viewport const *viewport
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
  _In_ crude_command_buffer         *cmd,
  _In_ uint32                        index,
  _In_ VkClearValue                  clear
)
{
  cmd->clears[ index ] = clear;
}

void
crude_gfx_cmd_set_scissor
(
  _In_ crude_command_buffer       *cmd,
  _In_opt_ crude_rect2d_int const *rect
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
  _In_ crude_command_buffer         *cmd,
  _In_ crude_descriptor_set_handle   handle
)
{
  crude_descriptor_set *descriptor_set = CRUDE_ACCESS_RESOURCE( cmd->frame_descriptor_sets, crude_descriptor_set, handle );
  
  uint32 num_offsets = 0u;
  uint32 offsets_cache[ 8 ];
  for ( uint32 i = 0; i < descriptor_set->layout->num_bindings; ++i )
  {
    if ( descriptor_set->layout->bindings[ i ].type == VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER )
    {
        CRUDE_ASSERT( num_offsets < ARRAY_SIZE( offsets_cache ) );
        const uint32 resource_index = descriptor_set->bindings[ i ];
        crude_buffer_handle buffer_handle = { descriptor_set->resources[ resource_index ] };
        crude_buffer *buffer = CRUDE_GFX_GPU_ACCESS_BUFFER( cmd->gpu, buffer_handle );
        offsets_cache[ num_offsets++ ] = buffer->global_offset;
    }
  }
  vkCmdBindDescriptorSets( cmd->vk_cmd_buffer, cmd->current_pipeline->vk_bind_point, cmd->current_pipeline->vk_pipeline_layout, 0u, 1u, &descriptor_set->vk_descriptor_set, num_offsets, offsets_cache );
  vkCmdBindDescriptorSets( cmd->vk_cmd_buffer, cmd->current_pipeline->vk_bind_point, cmd->current_pipeline->vk_pipeline_layout, 1u, 1u, &cmd->gpu->vk_bindless_descriptor_set, 0u, NULL );
}

void
crude_gfx_cmd_draw
(
  _In_ crude_command_buffer *cmd,
  _In_ uint32                first_vertex,
  _In_ uint32                vertex_count,
  _In_ uint32                first_instance,
  _In_ uint32                instance_count
)
{
  vkCmdDraw( cmd->vk_cmd_buffer, vertex_count, instance_count, first_vertex, first_instance );
}

void
crude_gfx_cmd_draw_indexed
(
  _In_ crude_command_buffer         *cmd,
  _In_ uint32                        index_count,
  _In_ uint32                        instance_count,
  _In_ uint32                        first_index,
  _In_ int32                         vertex_offset,
  _In_ uint32                        first_instance
)
{
  vkCmdDrawIndexed( cmd->vk_cmd_buffer, index_count, instance_count, first_index, vertex_offset, first_instance );
}

void
crude_gfx_cmd_bind_vertex_buffer
(
  _In_ crude_command_buffer         *cmd,
  _In_ crude_buffer_handle           handle,
  _In_ uint32                        binding,
  _In_ uint32                        offset
)
{
  crude_buffer *buffer = CRUDE_GFX_GPU_ACCESS_BUFFER( cmd->gpu, handle );
  VkDeviceSize offsets[] = { offset };
  
  VkBuffer vk_buffer = buffer->vk_buffer;
  
  if ( buffer->parent_buffer.index != CRUDE_RESOURCE_INVALID_INDEX )
  {
    crude_buffer *parent_buffer = CRUDE_GFX_GPU_ACCESS_BUFFER( cmd->gpu, buffer->parent_buffer );
    vk_buffer = parent_buffer->vk_buffer;
    offsets[ 0 ] = buffer->global_offset;
  }
  
  vkCmdBindVertexBuffers( cmd->vk_cmd_buffer, binding, 1, &vk_buffer, offsets );
}

CRUDE_API void
crude_gfx_cmd_bind_index_buffer
(
  _In_ crude_command_buffer         *cmd,
  _In_ crude_buffer_handle           handle,
  _In_ uint32                        offset
)
{
  crude_buffer *buffer = CRUDE_GFX_GPU_ACCESS_BUFFER( cmd->gpu, handle );
  
  VkBuffer vk_buffer = buffer->vk_buffer;
  
  if ( buffer->parent_buffer.index != CRUDE_RESOURCE_INVALID_INDEX )
  {
    crude_buffer *parent_buffer = CRUDE_GFX_GPU_ACCESS_BUFFER( cmd->gpu, buffer->parent_buffer );
    vk_buffer = parent_buffer->vk_buffer;
    offset = buffer->global_offset;
  }

  vkCmdBindIndexBuffer( cmd->vk_cmd_buffer, vk_buffer, offset, VK_INDEX_TYPE_UINT16  );
}

crude_descriptor_set_handle
crude_gfx_cmd_create_local_descriptor_set
(
  _In_ crude_command_buffer                  *cmd,
  _In_ crude_descriptor_set_creation const   *creation
)
{
  crude_descriptor_set_handle handle = { CRUDE_OBTAIN_RESOURCE( cmd->frame_descriptor_sets ) };
  if ( handle.index == CRUDE_RESOURCE_INVALID_INDEX )
  {
    return handle;
  }
  
  crude_descriptor_set *descriptor_set = CRUDE_ACCESS_RESOURCE( cmd->frame_descriptor_sets, crude_descriptor_set , handle );
  crude_descriptor_set_layout *descriptor_set_layout = CRUDE_GFX_GPU_ACCESS_DESCRIPTOR_SET_LAYOUT( cmd->gpu, creation->layout );
  
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
    crude_descriptor_binding const *binding = &descriptor_set_layout->bindings[ creation->bindings[ i ] ];
    
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
      crude_buffer *buffer = CRUDE_GFX_GPU_ACCESS_BUFFER( cmd->gpu, ( crude_buffer_handle ){ creation->resources[ i ] } );
      CRUDE_ASSERT( buffer );
      
      descriptor_write[ i ].descriptorType = ( buffer->usage = CRUDE_RESOURCE_USAGE_TYPE_DYNAMIC ) ? VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC : VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;

      if ( buffer->parent_buffer.index != CRUDE_RESOURCE_INVALID_INDEX )
      {
        crude_buffer *parent_buffer = CRUDE_GFX_GPU_ACCESS_BUFFER( cmd->gpu, buffer->parent_buffer );
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
crude_gfx_initialize_cmd_manager
(
  _In_ crude_command_buffer_manager *cmd_manager,
  _In_ crude_gpu_device             *gpu
)
{
  cmd_manager->gpu = gpu;
  
  for ( uint32 i = 0; i < CRUDE_COMMAND_BUFFER_MANAGER_MAX_POOLS; ++i )
  {
    VkCommandPoolCreateInfo cmd_pool_info = {
      .sType            = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
      .queueFamilyIndex = gpu->vk_queue_family_index,
      .flags            = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
    };
    
    CRUDE_GFX_HANDLE_VULKAN_RESULT( vkCreateCommandPool( gpu->vk_device, &cmd_pool_info, gpu->vk_allocation_callbacks, &cmd_manager->vk_cmd_pools[ i ] ), "Failed to create command pool" );
  }
  
  for ( uint32 i = 0; i < CRUDE_COMMAND_BUFFER_MANAGER_MAX_BUFFERS; ++i )
  {
    crude_command_buffer *command_buffer = &cmd_manager->cmd_buffers[ i ];
    
    command_buffer->gpu = gpu;

    VkCommandBufferAllocateInfo cmd_allocation_info = { 
      .sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
      .commandPool        = cmd_manager->vk_cmd_pools[ i / CRUDE_COMMAND_BUFFER_MANAGER_BUFFER_PER_POOL ],
      .level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
      .commandBufferCount = 1,
    };
    CRUDE_GFX_HANDLE_VULKAN_RESULT( vkAllocateCommandBuffers( gpu->vk_device, &cmd_allocation_info, &command_buffer->vk_cmd_buffer ), "Failed to allocate command buffer" );
    
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
      .maxSets       = global_pool_elements * ARRAY_SIZE( pool_sizes ),
      .poolSizeCount = ARRAY_SIZE( pool_sizes ),
      .pPoolSizes    = pool_sizes,
    };

    CRUDE_GFX_HANDLE_VULKAN_RESULT( vkCreateDescriptorPool( gpu->vk_device, &pool_info, gpu->vk_allocation_callbacks, &command_buffer->vk_descriptor_pool ), "Failed create descriptor pool" );

    crude_initialize_resource_pool( &command_buffer->frame_descriptor_sets, gpu->allocator, 256, sizeof( crude_descriptor_set ) );

    crude_gfx_cmd_reset( command_buffer );
  }
}

void
crude_gfx_deinitialize_cmd_manager
(
  _In_ crude_command_buffer_manager *cmd_manager
)
{
  for ( uint32 i = 0; i < CRUDE_COMMAND_BUFFER_MANAGER_MAX_BUFFERS; ++i )
  {
    crude_gfx_cmd_reset( &cmd_manager->cmd_buffers[ i ] );
    crude_deinitialize_resource_pool( &cmd_manager->cmd_buffers[ i ].frame_descriptor_sets );
    vkDestroyDescriptorPool( cmd_manager->gpu->vk_device, cmd_manager->cmd_buffers[ i ].vk_descriptor_pool, cmd_manager->gpu->vk_allocation_callbacks );
  }
  for ( uint32 i = 0; i < CRUDE_MAX_SWAPCHAIN_IMAGES; ++i )
  {
    vkDestroyCommandPool( cmd_manager->gpu->vk_device, cmd_manager->vk_cmd_pools[ i ], cmd_manager->gpu->vk_allocation_callbacks );
  }
}

void
crude_gfx_reset_cmd_manager
(
  _In_ crude_command_buffer_manager *cmd_manager,
  _In_ uint32                        frame
)
{
  for ( uint32 i = 0; i < CRUDE_COMMAND_BUFFER_MANAGER_MAX_THREADS; ++i )
  {
    vkResetCommandPool( cmd_manager->gpu->vk_device, cmd_manager->vk_cmd_pools[ frame + i * CRUDE_MAX_SWAPCHAIN_IMAGES ], 0 );
  }
}

crude_command_buffer*
crude_gfx_cmd_manager_get_cmd_buffer
(
  _In_ crude_command_buffer_manager *cmd_manager,
  _In_ uint32                        frame,
  _In_ bool                          begin
)
{
  crude_command_buffer *command_buffer = &cmd_manager->cmd_buffers[ frame * CRUDE_COMMAND_BUFFER_MANAGER_BUFFER_PER_POOL ];

  if ( begin )
  {  
    crude_gfx_cmd_reset( command_buffer );

    VkCommandBufferBeginInfo begin_info = {
      .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
      .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT
    };
    vkBeginCommandBuffer( command_buffer->vk_cmd_buffer, &begin_info );
  }
  
  return command_buffer;
}

crude_command_buffer*
crude_gfx_cmd_manager_get_cmd_buffer_instant
(
  _In_ crude_command_buffer_manager *cmd_manager,
  _In_ uint32                        frame
)
{
  crude_command_buffer *command_buffer = &cmd_manager->cmd_buffers[ frame * CRUDE_COMMAND_BUFFER_MANAGER_BUFFER_PER_POOL + 1 ];
  return command_buffer;
}