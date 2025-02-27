#include <graphics/gpu_device.h>
#include <core/log.h>

#include <graphics/command_buffer.h>

void crude_gfx_cmd_reset
(
  _In_ crude_command_buffer *cmd
)
{
  cmd->is_recording = false;
  cmd->current_render_pass = NULL;
}

void crude_gfx_cmd_bind_render_pass
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

void crude_gfx_cmd_bind_pipeline
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
      vk_viewport.width = cmd->gpu->swapchain_width * 1.f;
      vk_viewport.y = cmd->gpu->swapchain_height * 1.f;
      vk_viewport.height = -cmd->gpu->swapchain_height * 1.f;
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
    vk_scissor.extent.width = cmd->gpu->swapchain_width;
    vk_scissor.extent.height = cmd->gpu->swapchain_height;
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