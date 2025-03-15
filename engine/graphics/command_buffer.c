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
    vkCmdEndRenderPass( cmd->vk_handle );
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
    
    vkCmdBeginRenderPass( cmd->vk_handle, &render_pass_begin, VK_SUBPASS_CONTENTS_INLINE );
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

  vkCmdBindPipeline( cmd->vk_handle, pipeline->vk_bind_point, pipeline->vk_pipeline );
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
  
  vkCmdSetViewport( cmd->vk_handle, 0, 1, &vk_viewport);
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
  
  vkCmdSetScissor( cmd->vk_handle, 0, 1, &vk_scissor );
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
  vkCmdDraw( cmd->vk_handle, vertex_count, instance_count, first_vertex, first_instance );
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
  vkCmdDrawIndexed( cmd->vk_handle, index_count, instance_count, first_index, vertex_offset, first_instance );
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
  
  vkCmdBindVertexBuffers( cmd->vk_handle, binding, 1, &vk_buffer, offsets );
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

  vkCmdBindIndexBuffer( cmd->vk_handle, vk_buffer, offset, VK_INDEX_TYPE_UINT16  );
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
    
    CRUDE_GFX_HANDLE_VULKAN_RESULT( vkCreateCommandPool( gpu->vk_device, &cmd_pool_info, gpu->vk_allocation_callbacks, &cmd_manager->vk_command_pools[ i ] ), "Failed to create command pool" );
  }
  
  for ( uint32 i = 0; i < CRUDE_COMMAND_BUFFER_MANAGER_MAX_BUFFERS; ++i )
  {
    crude_command_buffer *command_buffer = &cmd_manager->command_buffers[ i ];
    VkCommandBufferAllocateInfo cmd_allocation_info = { 
      .sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
      .commandPool        = cmd_manager->vk_command_pools[ i / CRUDE_COMMAND_BUFFER_MANAGER_BUFFER_PER_POOL ],
      .level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
      .commandBufferCount = 1,
    };
    CRUDE_GFX_HANDLE_VULKAN_RESULT( vkAllocateCommandBuffers( gpu->vk_device, &cmd_allocation_info, &command_buffer->vk_handle ), "Failed to allocate command buffer" );
    
    command_buffer->gpu = gpu;
    crude_gfx_cmd_reset( command_buffer );
  }
}

void
crude_gfx_deinitialize_cmd_manager
(
  _In_ crude_command_buffer_manager *cmd_manager
)
{
  for ( uint32 i = 0; i < CRUDE_COMMAND_BUFFER_MANAGER_MAX_POOLS; ++i )
  {
    vkDestroyCommandPool( cmd_manager->gpu->vk_device, cmd_manager->vk_command_pools[ i ], cmd_manager->gpu->vk_allocation_callbacks );
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
    vkResetCommandPool( cmd_manager->gpu->vk_device, cmd_manager->vk_command_pools[ frame * CRUDE_COMMAND_BUFFER_MANAGER_MAX_THREADS + i ], 0 );
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
  crude_command_buffer *command_buffer = &cmd_manager->command_buffers[ frame * CRUDE_COMMAND_BUFFER_MANAGER_BUFFER_PER_POOL ];

  if ( begin )
  {  
    crude_gfx_cmd_reset( command_buffer );

    VkCommandBufferBeginInfo begin_info = {
      .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
      .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT
    };
    vkBeginCommandBuffer( command_buffer->vk_handle, &begin_info );
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
  crude_command_buffer *command_buffer = &cmd_manager->command_buffers[ frame * CRUDE_COMMAND_BUFFER_MANAGER_BUFFER_PER_POOL + 1 ];
  return command_buffer;
}