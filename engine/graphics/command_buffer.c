#include <graphics/gpu_device.h>

#include <graphics/command_buffer.h>

void crude_reset_command_buffer(
  _In_ crude_command_buffer *command_buffer )
{
  command_buffer->is_recording = false;
  command_buffer->current_render_pass = NULL;
}

void crude_bind_render_pass(
  _In_ crude_command_buffer     *command_buffer,
  _In_ crude_render_pass_handle  handle )
{
  command_buffer->is_recording = true;

  crude_render_pass *render_pass = crude_resource_pool_access_resource( &command_buffer->gpu->render_passes, handle.index );
  
  if ( command_buffer->current_render_pass && ( command_buffer->current_render_pass->type != CRUDE_RENDER_PASS_TYPE_COMPUTE ) && ( render_pass != command_buffer->current_render_pass ) )
  {
    vkCmdEndRenderPass( command_buffer->vk_command_buffer );
  }
  
  if ( render_pass != command_buffer->current_render_pass && ( render_pass->type != CRUDE_RENDER_PASS_TYPE_COMPUTE ) )
  {
    VkRenderPassBeginInfo render_pass_begin = {
      .sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
      .framebuffer = render_pass->type == CRUDE_RENDER_PASS_TYPE_SWAPCHAIN ? command_buffer->gpu->vk_swapchain_framebuffers[ command_buffer->gpu->vk_swapchain_image_index ] : render_pass->vk_frame_buffer,
      .renderPass = render_pass->vk_render_pass,
      .renderArea.offset = { 0, 0 },
      .renderArea.extent = { render_pass->width, render_pass->height },
      .clearValueCount = 2,
      .pClearValues = command_buffer->clears,
    };
    
    vkCmdBeginRenderPass( command_buffer->vk_command_buffer, &render_pass_begin, VK_SUBPASS_CONTENTS_INLINE );
  }
  
  command_buffer->current_render_pass = render_pass;
}

void crude_bind_pipeline(
  _In_ crude_command_buffer   *command_buffer,
  _In_ crude_pipeline_handle   handle )
{
  crude_pipeline *pipeline = crude_resource_pool_access_resource( &command_buffer->gpu->pipelines, handle.index );
  vkCmdBindPipeline( command_buffer->vk_command_buffer, pipeline->vk_bind_point, pipeline->vk_pipeline );
  command_buffer->current_pipeline = pipeline;
}

void crude_set_viewport( _In_ crude_command_buffer *command_buffer, _In_opt_ crude_viewport const *viewport )
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
    
    if ( command_buffer->current_render_pass )
    {
      vk_viewport.width = command_buffer->current_render_pass->width * 1.f;
      vk_viewport.y = command_buffer->current_render_pass->height * 1.f;
      vk_viewport.height = -command_buffer->current_render_pass->height * 1.f;
    }
    else
    {
      vk_viewport.width = command_buffer->gpu->swapchain_width * 1.f;
      vk_viewport.y = command_buffer->gpu->swapchain_height * 1.f;
      vk_viewport.height = -command_buffer->gpu->swapchain_height * 1.f;
    }
    vk_viewport.minDepth = 0.0f;
    vk_viewport.maxDepth = 1.0f;
  }
  
  vkCmdSetViewport( command_buffer->vk_command_buffer, 0, 1, &vk_viewport);
}

void crude_set_scissor( _In_ crude_command_buffer *command_buffer, _In_opt_ crude_rect2d_int const *rect )
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
    vk_scissor.extent.width = command_buffer->gpu->swapchain_width;
    vk_scissor.extent.height = command_buffer->gpu->swapchain_height;
  }
  
  vkCmdSetScissor( command_buffer->vk_command_buffer, 0, 1, &vk_scissor );
}

void crude_command_buffer_draw(
  _In_ crude_command_buffer *command_buffer,
  _In_ uint32                first_vertex,
  _In_ uint32                vertex_count,
  _In_ uint32                first_instance,
  _In_ uint32                instance_count )
{
  vkCmdDraw( command_buffer->vk_command_buffer, vertex_count, instance_count, first_vertex, first_instance );
}