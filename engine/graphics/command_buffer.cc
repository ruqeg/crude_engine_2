#include <engine/core/assert.h>
#include <engine/core/array.h>
#include <engine/core/profiler.h>
#include <engine/graphics/gpu_device.h>
#include <engine/graphics/gpu_profiler.h>

#include <engine/graphics/command_buffer.h>


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
  crude_gfx_cmd_reset( cmd );
}

CRUDE_API void
crude_gfx_cmd_deinitialize
(
  _In_ crude_gfx_cmd_buffer                               *cmd
)
{
  crude_gfx_cmd_reset( cmd );
}

void
crude_gfx_cmd_reset
(
  _In_ crude_gfx_cmd_buffer                               *cmd
)
{
  cmd->is_recording = false;
  cmd->current_render_pass = NULL;
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
  
  VkCommandBufferInheritanceRenderingInfo rendering_info = CRUDE_COMPOUNT_EMPTY( VkCommandBufferInheritanceRenderingInfo );
  rendering_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_INHERITANCE_RENDERING_INFO;
  rendering_info.flags = VK_RENDERING_CONTENTS_SECONDARY_COMMAND_BUFFERS_BIT_KHR;
  rendering_info.viewMask = 0;
  rendering_info.colorAttachmentCount = render_pass->num_render_targets;
  rendering_info.pColorAttachmentFormats = render_pass->num_render_targets > 0 ? render_pass->output.color_formats : NULL;
  rendering_info.depthAttachmentFormat = render_pass->output.depth_stencil_format;
  rendering_info.stencilAttachmentFormat = VK_FORMAT_UNDEFINED;
  rendering_info.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

  VkCommandBufferInheritanceInfo inheritance = CRUDE_COMPOUNT_EMPTY( VkCommandBufferInheritanceInfo );
  inheritance.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_INHERITANCE_INFO;
  inheritance.pNext = &rendering_info;
  inheritance.renderPass = VK_NULL_HANDLE;
  inheritance.subpass = 0;
  inheritance.framebuffer = VK_NULL_HANDLE;

  VkCommandBufferBeginInfo begin_info = CRUDE_COMPOUNT_EMPTY( VkCommandBufferBeginInfo );
  begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
  begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT | VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT;
  begin_info.pInheritanceInfo = &inheritance;
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
  crude_gfx_framebuffer                                   *framebuffer;
  crude_gfx_render_pass                                   *render_pass;
  VkRenderingAttachmentInfoKHR                            *vk_color_attachments_info;
  VkRenderingAttachmentInfoKHR                             vk_depth_attachment_info;
  VkRenderingInfoKHR                                       vk_rendering_info;
  uint32                                                   temporary_allocator_marker;
  bool                                                     has_depth_attachment;
  
  temporary_allocator_marker = crude_stack_allocator_get_marker( cmd->gpu->temporary_allocator );

  cmd->is_recording = true;
  
  render_pass = crude_gfx_access_render_pass( cmd->gpu, render_pass_handle );
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
  
  framebuffer = crude_gfx_access_framebuffer( cmd->gpu, framebuffer_handle );
  
  CRUDE_ARRAY_INITIALIZE_WITH_LENGTH( vk_color_attachments_info, framebuffer->num_color_attachments, crude_stack_allocator_pack( cmd->gpu->temporary_allocator ) );
  crude_memory_set( vk_color_attachments_info, 0, sizeof( VkRenderingAttachmentInfoKHR ) * framebuffer->num_color_attachments );
  
  for ( uint32 i = 0; i < framebuffer->num_color_attachments; ++i )
  {
    VkRenderingAttachmentInfoKHR                          *color_attachment_info;
    crude_gfx_texture                                     *texture;
    VkAttachmentLoadOp                                     color_op;

    texture = crude_gfx_access_texture( cmd->gpu, framebuffer->color_attachments[ i ] );
    
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
    
    color_attachment_info = &vk_color_attachments_info[ i ];
    color_attachment_info->sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO_KHR;
    color_attachment_info->imageView = texture->vk_image_view;
    color_attachment_info->imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    color_attachment_info->resolveMode = VK_RESOLVE_MODE_NONE;
    color_attachment_info->loadOp = color_op;
    color_attachment_info->storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    color_attachment_info->clearValue = render_pass->output.color_operations[ i ] == CRUDE_GFX_RENDER_PASS_OPERATION_CLEAR ? cmd->clears[ i ] : CRUDE_COMPOUNT_EMPTY( VkClearValue );
  }
  
  vk_depth_attachment_info = CRUDE_COMPOUNT_EMPTY( VkRenderingAttachmentInfoKHR );
  vk_depth_attachment_info.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO_KHR;
  
  has_depth_attachment = CRUDE_RESOURCE_HANDLE_IS_VALID( framebuffer->depth_stencil_attachment );
  if ( has_depth_attachment )
  {
    crude_gfx_texture                                     *texture;
    VkAttachmentLoadOp                                     depth_op;

    texture = crude_gfx_access_texture( cmd->gpu, framebuffer->depth_stencil_attachment );
  
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
    
    vk_depth_attachment_info.imageView = texture->vk_image_view;
    vk_depth_attachment_info.imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    vk_depth_attachment_info.resolveMode = VK_RESOLVE_MODE_NONE;
    vk_depth_attachment_info.loadOp = depth_op;
    vk_depth_attachment_info.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    vk_depth_attachment_info.clearValue = render_pass->output.depth_operation == CRUDE_GFX_RENDER_PASS_OPERATION_CLEAR ? cmd->clears[ CRUDE_GRAPHICS_DEPTH_AND_STENCIL_CLEAR_COLOR_INDEX ] : CRUDE_COMPOUNT_EMPTY( VkClearValue );
  }
  
  vk_rendering_info = CRUDE_COMPOUNT_EMPTY( VkRenderingInfoKHR );
  vk_rendering_info.sType = VK_STRUCTURE_TYPE_RENDERING_INFO_KHR;
  vk_rendering_info.renderArea = CRUDE_COMPOUNT( VkRect2D, { 0, 0, framebuffer->width, framebuffer->height } );
  vk_rendering_info.layerCount = 1;
  vk_rendering_info.viewMask = 0;
  vk_rendering_info.colorAttachmentCount = framebuffer->num_color_attachments;
  vk_rendering_info.pColorAttachments = framebuffer->num_color_attachments > 0 ? vk_color_attachments_info : NULL;
  vk_rendering_info.pDepthAttachment =  has_depth_attachment ? &vk_depth_attachment_info : NULL;
  vk_rendering_info.pStencilAttachment = NULL;

  if ( use_secondary )
  {
    vk_rendering_info.flags = VK_RENDERING_CONTENTS_SECONDARY_COMMAND_BUFFERS_BIT_KHR;
  }
  
  cmd->gpu->vkCmdBeginRenderingKHR( cmd->vk_cmd_buffer, &vk_rendering_info );
  
  cmd->current_render_pass = render_pass;
  cmd->current_framebuffer = framebuffer;
  
  crude_stack_allocator_free_marker( cmd->gpu->temporary_allocator, temporary_allocator_marker );
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
  _In_opt_ crude_gfx_viewport const                       *dev_viewport
)
{
  VkViewport vk_viewport;
  
  if ( dev_viewport )
  {
    vk_viewport.x = dev_viewport->rect.x * 1.f;
    vk_viewport.width = dev_viewport->rect.width * 1.f;
    vk_viewport.y = dev_viewport->rect.height * 1.f - dev_viewport->rect.y;
    vk_viewport.height = -dev_viewport->rect.height * 1.f;
    vk_viewport.minDepth = dev_viewport->min_depth;
    vk_viewport.maxDepth = dev_viewport->max_depth;
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
crude_gfx_cmd_set_clear_color_f32
(
  _In_ crude_gfx_cmd_buffer                               *cmd,
  _In_ float32                                             r,
  _In_ float32                                             g,
  _In_ float32                                             b,
  _In_ float32                                             a,
  _In_ uint32                                              index
)
{
  cmd->clears[ index ].color.float32[ 0 ] = r;
  cmd->clears[ index ].color.float32[ 1 ] = g;
  cmd->clears[ index ].color.float32[ 2 ] = b;
  cmd->clears[ index ].color.float32[ 3 ] = a;
}

void
crude_gfx_cmd_set_clear_depth_and_stencil
(
  _In_ crude_gfx_cmd_buffer                               *cmd,
  _In_ float32                                             depth,
  _In_ float32                                             stencil
)
{
  cmd->clears[ CRUDE_GRAPHICS_DEPTH_AND_STENCIL_CLEAR_COLOR_INDEX ].depthStencil.depth = depth;
  cmd->clears[ CRUDE_GRAPHICS_DEPTH_AND_STENCIL_CLEAR_COLOR_INDEX ].depthStencil.stencil = stencil;
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
crude_gfx_cmd_draw_inderect
(
  _In_ crude_gfx_cmd_buffer                               *cmd,
  _In_ crude_gfx_buffer_handle                             buffer_handle,
  _In_ uint32                                              offset,
  _In_ uint32                                              draw_count,
  _In_ uint32                                              stride
)
{
  crude_gfx_buffer *buffer = crude_gfx_access_buffer( cmd->gpu, buffer_handle );
  vkCmdDrawIndirect( cmd->vk_cmd_buffer, buffer->vk_buffer, offset, draw_count, stride );
}

void
crude_gfx_cmd_draw_mesh_task
(
  _In_ crude_gfx_cmd_buffer                               *cmd,
  _In_ uint32                                              group_count_x,
  _In_ uint32                                              group_count_y,
  _In_ uint32                                              group_count_z
)
{
  cmd->gpu->vkCmdDrawMeshTasksEXT( cmd->vk_cmd_buffer, group_count_x, group_count_y, group_count_z );
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
crude_gfx_cmd_bind_bindless_descriptor_set
(
  _In_ crude_gfx_cmd_buffer                               *cmd
)
{
  crude_gfx_descriptor_set                                *bindless_descriptor_set;

  bindless_descriptor_set = crude_gfx_access_descriptor_set( cmd->gpu, cmd->gpu->bindless_descriptor_set_handle );
  vkCmdBindDescriptorSets( cmd->vk_cmd_buffer, cmd->current_pipeline->vk_bind_point, cmd->current_pipeline->vk_pipeline_layout, CRUDE_GRAPHICS_BINDLESS_DESCRIPTOR_SET_INDEX, 1u, &bindless_descriptor_set->vk_descriptor_set, 0u, NULL );
}

void
crude_gfx_cmd_add_buffer_barrier
(
  _In_ crude_gfx_cmd_buffer                               *cmd,
  _In_ crude_gfx_buffer_handle                             buffer_handle,
  _In_ crude_gfx_resource_state                            old_state,
  _In_ crude_gfx_resource_state                            new_state
)
{
  crude_gfx_buffer                                        *buffer;
  VkBufferMemoryBarrier2KHR                                barrier;
  VkDependencyInfoKHR                                      dependency_info;

  buffer = crude_gfx_access_buffer( cmd->gpu, buffer_handle );

  barrier = CRUDE_COMPOUNT_EMPTY( VkBufferMemoryBarrier2KHR );
  barrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER_2_KHR;
  barrier.srcAccessMask = crude_gfx_resource_state_to_vk_access_flags2( old_state );
  barrier.srcStageMask = crude_gfx_determine_pipeline_stage_flags2( barrier.srcAccessMask, CRUDE_GFX_QUEUE_TYPE_GRAPHICS );
  barrier.dstAccessMask = crude_gfx_resource_state_to_vk_access_flags2( new_state );
  barrier.dstStageMask = crude_gfx_determine_pipeline_stage_flags2( barrier.dstAccessMask, CRUDE_GFX_QUEUE_TYPE_GRAPHICS );
  barrier.buffer = buffer->vk_buffer;
  barrier.offset = 0;
  barrier.size = buffer->size;
  
  dependency_info = CRUDE_COMPOUNT_EMPTY( VkDependencyInfoKHR );
  dependency_info.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO_KHR;
  dependency_info.bufferMemoryBarrierCount = 1;
  dependency_info.pBufferMemoryBarriers = &barrier;
  
  cmd->gpu->vkCmdPipelineBarrier2KHR( cmd->vk_cmd_buffer, &dependency_info );
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
  crude_gfx_cmd_add_image_barrier_ext3( cmd, vk_image, old_state, new_state, base_mip_level, mip_count, is_depth, VK_QUEUE_FAMILY_IGNORED, VK_QUEUE_FAMILY_IGNORED, CRUDE_GFX_QUEUE_TYPE_GRAPHICS, CRUDE_GFX_QUEUE_TYPE_GRAPHICS );
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
  crude_gfx_cmd_add_image_barrier_ext5( cmd, vk_image, old_state, new_state, base_mip_level, mip_count, 0u, 1u, is_depth, source_queue_family, destination_family, source_queue_type, destination_queue_type );
}

void
crude_gfx_cmd_add_image_barrier_ext4
(
  _In_ crude_gfx_cmd_buffer                               *cmd,
  _In_ crude_gfx_texture                                  *texture,
  _In_ crude_gfx_resource_state                            new_state,
  _In_ uint32                                              base_mip_level,
  _In_ uint32                                              mip_count,
  _In_ uint32                                              base_array_layer,
  _In_ uint32                                              array_layer_count,
  _In_ bool                                                is_depth
)
{
  crude_gfx_cmd_add_image_barrier_ext5( cmd, texture->vk_image, texture->state, new_state, base_mip_level, mip_count, 0u, 1u, is_depth, VK_QUEUE_FAMILY_IGNORED, VK_QUEUE_FAMILY_IGNORED, CRUDE_GFX_QUEUE_TYPE_GRAPHICS, CRUDE_GFX_QUEUE_TYPE_GRAPHICS );
  texture->state = new_state;
}

void
crude_gfx_cmd_add_image_barrier_ext5
(
  _In_ crude_gfx_cmd_buffer                               *cmd,
  _In_ VkImage                                             vk_image,
  _In_ crude_gfx_resource_state                            old_state,
  _In_ crude_gfx_resource_state                            new_state,
  _In_ uint32                                              base_mip_level,
  _In_ uint32                                              mip_count,
  _In_ uint32                                              base_array_layer,
  _In_ uint32                                              array_layer_count,
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
  barrier.subresourceRange.baseArrayLayer = base_array_layer;
  barrier.subresourceRange.layerCount = array_layer_count;

  VkDependencyInfo dependency_info = CRUDE_COMPOUNT_EMPTY( VkDependencyInfo );
  dependency_info.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO_KHR;
  dependency_info.imageMemoryBarrierCount = 1;
  dependency_info.pImageMemoryBarriers = &barrier;

  cmd->gpu->vkCmdPipelineBarrier2KHR( cmd->vk_cmd_buffer, &dependency_info );
}

void
crude_gfx_cmd_global_debug_barrier
(
  _In_ crude_gfx_cmd_buffer                               *cmd
)
{
  VkMemoryBarrier2KHR                                      vk_barrier;
  VkDependencyInfoKHR                                      vk_dependency_info;

  vk_barrier = CRUDE_COMPOUNT_EMPTY( VkMemoryBarrier2KHR );
  vk_barrier.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER_2_KHR;
  vk_barrier.srcStageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT_KHR;
  vk_barrier.srcAccessMask = VK_ACCESS_2_MEMORY_READ_BIT_KHR | VK_ACCESS_2_MEMORY_WRITE_BIT_KHR;
  vk_barrier.dstStageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT_KHR;
  vk_barrier.dstAccessMask = VK_ACCESS_2_MEMORY_READ_BIT_KHR | VK_ACCESS_2_MEMORY_WRITE_BIT_KHR;

  vk_dependency_info = CRUDE_COMPOUNT_EMPTY( VkDependencyInfoKHR );
  vk_dependency_info.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO_KHR;
  vk_dependency_info.memoryBarrierCount = 1;
  vk_dependency_info.pMemoryBarriers = &vk_barrier;

  cmd->gpu->vkCmdPipelineBarrier2KHR( cmd->vk_cmd_buffer, &vk_dependency_info );
}

void
crude_gfx_cmd_memory_copy_to_texture
(
  _In_ crude_gfx_cmd_buffer                               *cmd,
  _In_ crude_gfx_texture_handle                            texture_handle,
  _In_ crude_gfx_memory_allocation                         memory_allocation
)
{
  crude_gfx_texture                                       *texture;
  crude_gfx_buffer                                        *buffer;
  VkBufferImageCopy                                        vk_region;

  texture = crude_gfx_access_texture( cmd->gpu, texture_handle );
  buffer = crude_gfx_access_buffer( cmd->gpu, memory_allocation.buffer_handle );

  vk_region = CRUDE_COMPOUNT_EMPTY( VkBufferImageCopy );
  vk_region.bufferOffset = 0;
  vk_region.bufferRowLength = 0;
  vk_region.bufferImageHeight = 0;
  vk_region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  vk_region.imageSubresource.mipLevel = 0;
  vk_region.imageSubresource.baseArrayLayer = 0;
  vk_region.imageSubresource.layerCount = 1;
  vk_region.imageOffset = CRUDE_COMPOUNT( VkOffset3D, { 0, 0, 0 } );
  vk_region.imageExtent = CRUDE_COMPOUNT( VkExtent3D, { texture->width, texture->height, texture->depth }  );

  crude_gfx_cmd_add_image_barrier( cmd, texture, CRUDE_GFX_RESOURCE_STATE_COPY_DEST, 0, 1, false );
  vkCmdCopyBufferToImage( cmd->vk_cmd_buffer, buffer->vk_buffer, texture->vk_image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &vk_region );
  crude_gfx_cmd_add_image_barrier_ext( cmd, texture, CRUDE_GFX_RESOURCE_STATE_COPY_SOURCE, 0, 1, false, cmd->gpu->vk_transfer_queue_family, cmd->gpu->vk_main_queue_family, CRUDE_GFX_QUEUE_TYPE_COPY_TRANSFER, CRUDE_GFX_QUEUE_TYPE_GRAPHICS );
}

void
crude_gfx_cmd_memory_copy
(
  _In_ crude_gfx_cmd_buffer                               *cmd,
  _In_ crude_gfx_memory_allocation                         src_memory_allocation,
  _In_ crude_gfx_memory_allocation                         dst_memory_allocation,
  _In_ uint64                                              src_offset,
  _In_ uint64                                              dst_offset
)
{
  crude_gfx_buffer                                        *src_buffer;
  crude_gfx_buffer                                        *dst_buffer;
  VkBufferCopy                                             vk_region;

  src_buffer = crude_gfx_access_buffer( cmd->gpu, src_memory_allocation.buffer_handle );
  dst_buffer = crude_gfx_access_buffer( cmd->gpu, dst_memory_allocation.buffer_handle );

  CRUDE_ASSERTM( CRUDE_CHANNEL_GRAPHICS, dst_memory_allocation.size, "%s dst buffer size == 0", dst_buffer->name ? dst_buffer->name : "unknown" )
  CRUDE_ASSERTM( CRUDE_CHANNEL_GRAPHICS, src_memory_allocation.size <= dst_memory_allocation.size, "%s src buffer size > %s dst buffer size", src_buffer->name ? src_buffer->name : "unknown", dst_buffer->name ? dst_buffer->name : "unknown" )

  vk_region = CRUDE_COMPOUNT_EMPTY( VkBufferCopy );
  vk_region.srcOffset = src_offset + src_memory_allocation.offset;
  vk_region.dstOffset = dst_offset + dst_memory_allocation.offset;
  vk_region.size = src_memory_allocation.size;

  //CRUDE_ASSERTM( CRUDE_CHANNEL_GRAPHICS, src_memory_allocation.aligned_size - vk_region.srcOffset <= dst_memory_allocation.aligned_size - vk_region.dstOffset, "%s src buffer size < %s dst buffer size - offset", src_buffer->name ? src_buffer->name : "unknown", dst_buffer->name ? dst_buffer->name : "unknown" )

  vkCmdCopyBuffer( cmd->vk_cmd_buffer, src_buffer->vk_buffer, dst_buffer->vk_buffer, 1, &vk_region );
}

void
crude_gfx_cmd_push_marker
(
  _In_ crude_gfx_cmd_buffer                               *cmd,
  _In_ char const                                         *name
)
{
#if CRUDE_GPU_PROFILER
  crude_gfx_gpu_time_query *time_query = crude_gfx_gpu_time_query_tree_push( cmd->thread_frame_pool->time_queries, name );
  vkCmdWriteTimestamp( cmd->vk_cmd_buffer, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, cmd->thread_frame_pool->vk_timestamp_query_pool, time_query->start_query_index );
#endif
#if CRUDE_GRAPHICS_VALIDATION_LAYERS_ENABLED
  VkDebugUtilsLabelEXT vk_label = CRUDE_COMPOUNT_EMPTY( VkDebugUtilsLabelEXT );
  vk_label.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_LABEL_EXT;
  vk_label.pLabelName = name;
  vk_label.color[ 0 ] = 1.0f;
  vk_label.color[ 1 ] = 1.0f;
  vk_label.color[ 2 ] = 1.0f;
  vk_label.color[ 3 ] = 1.0f;
  cmd->gpu->vkCmdBeginDebugUtilsLabelEXT( cmd->vk_cmd_buffer, &vk_label );
#endif /* CRUDE_GRAPHICS_VALIDATION_LAYERS_ENABLED */
}

void
crude_gfx_cmd_pop_marker
(
  _In_ crude_gfx_cmd_buffer                               *cmd
)
{
#if CRUDE_GPU_PROFILER
  crude_gfx_gpu_time_query *time_query = crude_gfx_gpu_time_query_tree_pop( cmd->thread_frame_pool->time_queries );
  vkCmdWriteTimestamp( cmd->vk_cmd_buffer, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, cmd->thread_frame_pool->vk_timestamp_query_pool, time_query->end_query_index );
#endif
#if CRUDE_GRAPHICS_VALIDATION_LAYERS_ENABLED
  cmd->gpu->vkCmdEndDebugUtilsLabelEXT( cmd->vk_cmd_buffer );
#endif /* CRUDE_GRAPHICS_VALIDATION_LAYERS_ENABLED */
}

void
crude_gfx_cmd_push_constant
(
  _In_ crude_gfx_cmd_buffer                               *cmd,
  _In_ void const                                         *data,
  _In_ uint64                                              size
)
{
  vkCmdPushConstants( cmd->vk_cmd_buffer, cmd->current_pipeline->vk_pipeline_layout, VK_SHADER_STAGE_ALL, 0, size, data );
}

void
crude_gfx_cmd_fill_buffer
(
  _In_ crude_gfx_cmd_buffer                               *cmd,
  _In_ crude_gfx_buffer_handle                             handle,
  _In_ uint32                                              value
)
{
  crude_gfx_buffer                                        *buffer;

  buffer = crude_gfx_access_buffer( cmd->gpu, handle );
  vkCmdFillBuffer( cmd->vk_cmd_buffer, buffer->vk_buffer, 0, buffer->size, value );
}

void
crude_gfx_cmd_trace_rays
(
  _In_ crude_gfx_cmd_buffer                               *cmd,
  _In_ crude_gfx_pipeline_handle                           pipeline_handle,
  _In_ uint32                                              width,
  _In_ uint32                                              height,
  _In_ uint32                                              depth
)
{
#if CRUDE_GRAPHICS_RAY_TRACING_ENABLED
  crude_gfx_pipeline                                      *pipeline;
  VkStridedDeviceAddressRegionKHR                          raygen_table, hit_table, miss_table, callable_table;
  uint32                                                   shader_group_handle_size;

  pipeline = crude_gfx_access_pipeline( cmd->gpu, pipeline_handle );

  shader_group_handle_size = cmd->gpu->ray_tracing_pipeline_properties.shaderGroupHandleSize;

  raygen_table = CRUDE_COMPOUNT_EMPTY( VkStridedDeviceAddressRegionKHR );
  raygen_table.deviceAddress = crude_gfx_get_buffer_device_address( cmd->gpu, pipeline->shader_binding_table_raygen );
  raygen_table.stride = shader_group_handle_size;
  raygen_table.size = shader_group_handle_size;
  
  hit_table = CRUDE_COMPOUNT_EMPTY( VkStridedDeviceAddressRegionKHR );
  hit_table.deviceAddress = crude_gfx_get_buffer_device_address( cmd->gpu, pipeline->shader_binding_table_hit );
  hit_table.stride = shader_group_handle_size;
  hit_table.size = shader_group_handle_size;
  
  miss_table = CRUDE_COMPOUNT_EMPTY( VkStridedDeviceAddressRegionKHR );
  miss_table.deviceAddress = crude_gfx_get_buffer_device_address( cmd->gpu, pipeline->shader_binding_table_miss );
  miss_table.stride = shader_group_handle_size;
  miss_table.size = shader_group_handle_size;

  callable_table = CRUDE_COMPOUNT_EMPTY( VkStridedDeviceAddressRegionKHR );
  
  cmd->gpu->vkCmdTraceRaysKHR( cmd->vk_cmd_buffer, &raygen_table, &miss_table, &hit_table, &callable_table, width, height, depth );
#else
  CRUDE_ASSERTM( CRUDE_CHANNEL_GRAPHICS, false, "Can proccess crude_gfx_cmd_trace_rays, CRUDE_GRAPHICS_RAY_TRACING_ENABLED wasn't enabled" );
#endif /* CRUDE_GRAPHICS_RAY_TRACING_ENABLED*/
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
  crude_allocator_container                                temporary_allocator_container;
  uint32                                                   total_pools;
  uint32                                                   total_buffers;
  uint32                                                   temporary_allocator_mark;
  uint32                                                   total_secondary_buffers;

  cmd_manager->gpu = gpu;
  cmd_manager->num_pools_per_frame = num_threads;
  cmd_manager->num_primary_cmd_buffers_per_thread = 3;
  cmd_manager->num_secondary_cmd_buffer_per_pool = 5;

  temporary_allocator_container = crude_stack_allocator_pack( gpu->temporary_allocator );

  total_pools = cmd_manager->num_pools_per_frame * CRUDE_GRAPHICS_MAX_SWAPCHAIN_IMAGES;

  CRUDE_ARRAY_INITIALIZE_WITH_LENGTH( cmd_manager->num_used_primary_cmd_buffers_per_frame, total_pools, gpu->allocator_container );
  CRUDE_ARRAY_INITIALIZE_WITH_LENGTH( cmd_manager->num_used_secondary_cmd_buffers_per_frame, total_pools, gpu->allocator_container );
  for ( uint32 i = 0; i < total_pools; ++i )
  {
    cmd_manager->num_used_primary_cmd_buffers_per_frame[ i ] = 0;
    cmd_manager->num_used_secondary_cmd_buffers_per_frame[ i ] = 0;
  }
  
  total_buffers = total_pools * cmd_manager->num_primary_cmd_buffers_per_thread;
  CRUDE_ARRAY_INITIALIZE_WITH_LENGTH( cmd_manager->primary_cmd_buffers, total_buffers, gpu->allocator_container );
  
  for ( uint32 i = 0; i < total_buffers; i++ )
  {
    crude_gfx_cmd_buffer                                  *current_cmd_buffer;
    VkCommandBufferAllocateInfo                            allocate_info;
    uint32                                                 frame_index, thread_index, pool_index;

    frame_index = i / ( cmd_manager->num_primary_cmd_buffers_per_thread * cmd_manager->num_pools_per_frame );
    thread_index = ( i / cmd_manager->num_primary_cmd_buffers_per_thread ) % cmd_manager->num_pools_per_frame;
    pool_index = pool_from_indices( cmd_manager, frame_index, thread_index );

    allocate_info = CRUDE_COMPOUNT_EMPTY( VkCommandBufferAllocateInfo );
    allocate_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocate_info.commandPool = cmd_manager->gpu->thread_frame_pools[ pool_index ].vk_command_pool;
    allocate_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocate_info.commandBufferCount = 1;
    
    current_cmd_buffer = &cmd_manager->primary_cmd_buffers[ i ];
    current_cmd_buffer->thread_frame_pool = &gpu->thread_frame_pools[ pool_index ];
    CRUDE_GFX_HANDLE_VULKAN_RESULT( vkAllocateCommandBuffers( gpu->vk_device, &allocate_info, &current_cmd_buffer->vk_cmd_buffer ), "Failed to allocate command buffer" );
    crude_gfx_cmd_initialize( current_cmd_buffer, gpu );

    char const *resource_name = crude_string_buffer_append_use_f( &gpu->objects_names_string_buffer, "primary_cmd frame: %i thread: %i pool: %i", frame_index, thread_index, pool_index );
    crude_gfx_set_resource_name( gpu, VK_OBJECT_TYPE_COMMAND_BUFFER, ( uint64 )current_cmd_buffer->vk_cmd_buffer, resource_name );
  }
  
  total_secondary_buffers = total_pools * cmd_manager->num_secondary_cmd_buffer_per_pool;
  CRUDE_ARRAY_INITIALIZE_WITH_LENGTH( cmd_manager->secondary_cmd_buffers, total_secondary_buffers, gpu->allocator_container );

  temporary_allocator_mark = crude_stack_allocator_get_marker( gpu->temporary_allocator );
  for ( uint32 pool_index = 0; pool_index < total_pools; ++pool_index )
  {
    VkCommandBufferAllocateInfo                            allocate_info;
    VkCommandBuffer                                       *secondary_buffers;

    allocate_info = CRUDE_COMPOUNT_EMPTY( VkCommandBufferAllocateInfo );
    allocate_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocate_info.commandPool = cmd_manager->gpu->thread_frame_pools[ pool_index ].vk_command_pool;
    allocate_info.level = VK_COMMAND_BUFFER_LEVEL_SECONDARY;
    allocate_info.commandBufferCount = cmd_manager->num_secondary_cmd_buffer_per_pool;
    
    secondary_buffers = NULL;
    CRUDE_ARRAY_INITIALIZE_WITH_LENGTH( secondary_buffers, cmd_manager->num_secondary_cmd_buffer_per_pool, temporary_allocator_container );

    vkAllocateCommandBuffers( gpu->vk_device, &allocate_info, secondary_buffers );
    for ( uint32 second_cmd_index = 0; second_cmd_index < cmd_manager->num_secondary_cmd_buffer_per_pool; ++second_cmd_index )
    {
      crude_gfx_cmd_buffer                                 cmd;

      cmd.vk_cmd_buffer = secondary_buffers[ second_cmd_index ];
      cmd.thread_frame_pool = &gpu->thread_frame_pools[ pool_index ];
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
  CRUDE_PROFILER_ZONE_NAME( "crude_gfx_cmd_manager_reset" );
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
  CRUDE_PROFILER_ZONE_END;
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
    CRUDE_ASSERT( !cmd->is_recording );

    crude_gfx_cmd_reset( cmd );
    crude_gfx_cmd_begin_primary( cmd );
    CRUDE_ASSERT( current_used_buffer < cmd_manager->num_primary_cmd_buffers_per_thread );
    cmd_manager->num_used_primary_cmd_buffers_per_frame[ pool_index ] = current_used_buffer + 1;
    
#if CRUDE_GPU_PROFILER
    crude_gfx_gpu_thread_frame_pools *thread_pools = cmd->thread_frame_pool;
    crude_gfx_gpu_time_query_tree_reset( thread_pools->time_queries );
    vkCmdResetQueryPool( cmd->vk_cmd_buffer, thread_pools->vk_timestamp_query_pool, 0, thread_pools->time_queries->time_queries_count * 2 );
    vkCmdResetQueryPool( cmd->vk_cmd_buffer, thread_pools->vk_pipeline_stats_query_pool, 0, CRUDE_GFX_GPU_PIPELINE_STATISTICS_COUNT );
    vkCmdBeginQuery( cmd->vk_cmd_buffer, thread_pools->vk_pipeline_stats_query_pool, 0, 0 );
#endif
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