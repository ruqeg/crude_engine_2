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
  crude_gfx_rhi_command_buffer_begin_info                  begin_info;

  if ( cmd->is_recording )
  {
    return;
  }

  cmd->is_recording = true;

  begin_info = crude_gfx_rhi_command_buffer_begin_info_empty( );
  begin_info.flags = CRUDE_GFX_RHI_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
  crude_gfx_rhi_begin_command_buffer( cmd->rhi_cmd_buffer, &begin_info );

  //CRUDE_LOG_INFO( CRUDE_CHANNEL_GRAPHICS, "crude_gfx_cmd_begin_primary( %s )", cmd->name );
}


void
crude_gfx_cmd_end
(
  _In_ crude_gfx_cmd_buffer                               *cmd
)
{
  if ( !cmd->is_recording )
  {
    //CRUDE_LOG_WARNING( CRUDE_CHANNEL_GRAPHICS, "crude_gfx_cmd_end( %s ) [!cmd->is_recording]", cmd->name );
    return;
  }

  //CRUDE_LOG_INFO( CRUDE_CHANNEL_GRAPHICS, "crude_gfx_cmd_end( %s )", cmd->name );

  crude_gfx_rhi_end_command_buffer( cmd->rhi_cmd_buffer );
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
    crude_gfx_rhi_command_buffer_end_rendering( cmd->rhi_cmd_buffer );
    cmd->current_render_pass = NULL;
    cmd->current_framebuffer = NULL;
  }
}

void
crude_gfx_cmd_bind_render_pass
(
  _In_ crude_gfx_cmd_buffer                               *cmd,
  _In_ crude_gfx_render_pass_handle                        render_pass_handle,
  _In_ crude_gfx_framebuffer_handle                        framebuffer_handle
)
{
  crude_gfx_framebuffer                                   *framebuffer;
  crude_gfx_render_pass                                   *render_pass;
  crude_gfx_rhi_rendering_info                             rendering_info;

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
  
  rendering_info = CRUDE_COMPOUNT_EMPTY( crude_gfx_rhi_rendering_info );
  
  rendering_info.render_area.offset.x = 0;
  rendering_info.render_area.offset.y = 0;
  rendering_info.render_area.extent.x = framebuffer->width;
  rendering_info.render_area.extent.y = framebuffer->height;
  rendering_info.layer_count = 1;
  rendering_info.view_mask = 0;
  
  rendering_info.color_attachment_count = framebuffer->num_color_attachments;
  for ( uint32 i = 0; i < rendering_info.color_attachment_count; ++i )
  {
    crude_gfx_rhi_rendering_attachment_info               *color_attachment_info;
    crude_gfx_texture                                     *texture;
    crude_gfx_rhi_attachment_load_op                       color_op;

    texture = crude_gfx_access_texture( cmd->gpu, framebuffer->color_attachments[ i ] );
    
    switch ( render_pass->output.color_operations[ i ] )
    {
      case CRUDE_GFX_RENDER_PASS_OPERATION_LOAD:
        color_op = CRUDE_GFX_RHI_ATTACHMENT_LOAD_OP_LOAD;
        break;
      case CRUDE_GFX_RENDER_PASS_OPERATION_CLEAR:
        color_op = CRUDE_GFX_RHI_ATTACHMENT_LOAD_OP_CLEAR;
        break;
      default:
        color_op = CRUDE_GFX_RHI_ATTACHMENT_LOAD_OP_DONT_CARE; // !TODO VK_ATTACHMENT_LOAD_OP_DONT_CARE for multisample https://docs.vulkan.org/samples/latest/samples/performance/msaa/README.html
        break;
    }
    
    color_attachment_info = &rendering_info.color_attachments[ i ];
    *color_attachment_info = CRUDE_COMPOUNT_EMPTY( crude_gfx_rhi_rendering_attachment_info );
    color_attachment_info->image_view = texture->rhi_image_view;
    color_attachment_info->image_layout = CRUDE_GFX_RHI_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    color_attachment_info->resolve_mode = CRUDE_GFX_RHI_RESOLVE_MODE_NONE;
    color_attachment_info->load_op = color_op;
    color_attachment_info->store_op = CRUDE_GFX_RHI_ATTACHMENT_STORE_OP_STORE; // !TODO STORE_OP_DONT_CARE for multisample
    color_attachment_info->clear_value = render_pass->output.color_operations[ i ] == CRUDE_GFX_RENDER_PASS_OPERATION_CLEAR ? cmd->clears[ i ] : CRUDE_COMPOUNT_EMPTY( crude_gfx_rhi_clear_value );
  }
  
  rendering_info.depth_attachment = CRUDE_COMPOUNT_EMPTY( crude_gfx_rhi_rendering_attachment_info );
  
  rendering_info.has_depth_attachment = CRUDE_RESOURCE_HANDLE_IS_VALID( framebuffer->depth_stencil_attachment );
  if ( rendering_info.has_depth_attachment )
  {
    crude_gfx_texture                                     *texture;
    crude_gfx_rhi_attachment_load_op                       depth_op;

    texture = crude_gfx_access_texture( cmd->gpu, framebuffer->depth_stencil_attachment );
  
    switch ( render_pass->output.depth_operation )
    {
      case CRUDE_GFX_RENDER_PASS_OPERATION_LOAD:
        depth_op = CRUDE_GFX_RHI_ATTACHMENT_LOAD_OP_LOAD;
        break;
      case CRUDE_GFX_RENDER_PASS_OPERATION_CLEAR:
        depth_op = CRUDE_GFX_RHI_ATTACHMENT_LOAD_OP_CLEAR;
        break;
      default:
        depth_op = CRUDE_GFX_RHI_ATTACHMENT_LOAD_OP_DONT_CARE;
        break;
    }
    
    rendering_info.depth_attachment.image_view = texture->rhi_image_view;
    rendering_info.depth_attachment.image_layout = CRUDE_GFX_RHI_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    rendering_info.depth_attachment.resolve_mode = CRUDE_GFX_RHI_RESOLVE_MODE_NONE;
    rendering_info.depth_attachment.load_op = depth_op;
    rendering_info.depth_attachment.store_op = CRUDE_GFX_RHI_ATTACHMENT_STORE_OP_STORE;
    rendering_info.depth_attachment.clear_value = render_pass->output.depth_operation == CRUDE_GFX_RENDER_PASS_OPERATION_CLEAR ? cmd->clears[ CRUDE_GFX_DEPTH_AND_STENCIL_CLEAR_COLOR_INDEX ] : CRUDE_COMPOUNT_EMPTY( crude_gfx_rhi_clear_value );
  }
  
  crude_gfx_rhi_command_buffer_begin_rendering( cmd->rhi_cmd_buffer, &rendering_info );
  
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

  crude_gfx_rhi_command_buffer_bind_pipeline( cmd->rhi_cmd_buffer, pipeline->rhi_pipeline, pipeline->bind_point );
  cmd->current_pipeline = pipeline;
}

void
crude_gfx_cmd_copy_texture
(
  _In_ crude_gfx_cmd_buffer                               *cmd,
  _In_ crude_gfx_texture_handle                            src_handle,
  _In_ crude_gfx_texture_handle                            dst_handle
)
{
  crude_gfx_texture                                       *src;
  crude_gfx_texture                                       *dst;
  crude_gfx_rhi_image_copy                                 image_copy;

  src = crude_gfx_access_texture( cmd->gpu, src_handle );
  dst = crude_gfx_access_texture( cmd->gpu, dst_handle );

  image_copy = crude_gfx_rhi_image_copy_empty( );
  image_copy.src_subresource.aspect_mask = CRUDE_GFX_RHI_IMAGE_ASPECT_COLOR_BIT;
  image_copy.src_subresource.mip_level = 0;
  image_copy.src_subresource.base_array_layer = 0;
  image_copy.src_subresource.layer_count = 1;
  image_copy.dst_subresource.aspect_mask = CRUDE_GFX_RHI_IMAGE_ASPECT_COLOR_BIT;
  image_copy.dst_subresource.mip_level = 0;
  image_copy.dst_subresource.base_array_layer = 0;
  image_copy.dst_subresource.layer_count = 1;
  image_copy.extent.x = dst->width;
  image_copy.extent.y = dst->height;
  image_copy.extent.z = 1;

  CRUDE_ASSERT( src->width == dst->width && src->height == dst->height );

  crude_gfx_rhi_command_buffer_copy_image(
    cmd->rhi_cmd_buffer,
    src->rhi_image,
    crude_gfx_rhi_resource_state_to_image_layout( src->state ),
    dst->rhi_image,
    crude_gfx_rhi_resource_state_to_image_layout( dst->state ),
    &image_copy );
}

void
crude_gfx_cmd_set_viewport
(
  _In_ crude_gfx_cmd_buffer                               *cmd,
  _In_opt_ crude_gfx_viewport const                       *viewport
)
{
  crude_gfx_rhi_viewport                                   rhi_viewport;
  
  if ( viewport )
  {
    rhi_viewport.x = viewport->rect.x * 1.f;
    rhi_viewport.width = viewport->rect.width * 1.f;
    rhi_viewport.y = viewport->rect.height * 1.f - viewport->rect.y;
    rhi_viewport.height = -viewport->rect.height * 1.f;
    rhi_viewport.min_depth = viewport->min_depth;
    rhi_viewport.max_depth = viewport->max_depth;
  }
  else
  {
    rhi_viewport.x = 0.f;
    
    if ( cmd->current_render_pass )
    {
      rhi_viewport.width = cmd->current_framebuffer->width * 1.f;
      rhi_viewport.y = cmd->current_framebuffer->height * 1.f;
      rhi_viewport.height = -cmd->current_framebuffer->height * 1.f;
    }
    else
    {
      rhi_viewport.width = cmd->gpu->renderer_size.x * 1.f;
      rhi_viewport.y = cmd->gpu->renderer_size.y * 1.f;
      rhi_viewport.height = -cmd->gpu->renderer_size.y * 1.f;
    }
    rhi_viewport.min_depth = 0.0f;
    rhi_viewport.max_depth = 1.0f;
  }
  
  crude_gfx_rhi_command_buffer_set_viewport( cmd->rhi_cmd_buffer, &rhi_viewport );
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
  cmd->clears[ CRUDE_GFX_DEPTH_AND_STENCIL_CLEAR_COLOR_INDEX ].depth_stencil.depth = depth;
  cmd->clears[ CRUDE_GFX_DEPTH_AND_STENCIL_CLEAR_COLOR_INDEX ].depth_stencil.stencil = stencil;
}

void
crude_gfx_cmd_set_scissor
(
  _In_ crude_gfx_cmd_buffer                               *cmd,
  _In_opt_ crude_gfx_rect2d_int const                     *rect
)
{
  crude_gfx_rhi_scissor                                    scissor;
  
  if ( rect )
  {
    scissor.offset.x = rect->x;
    scissor.offset.y = rect->y;
    scissor.extent.x = rect->width;
    scissor.extent.y = rect->height;
  }
  else
  {
    scissor.offset.x = 0;
    scissor.offset.y = 0;
    scissor.extent.x = cmd->gpu->renderer_size.x;
    scissor.extent.y = cmd->gpu->renderer_size.y;
  }
  
#if CRUDE_GFX_VULKAN
  CRUDE_ASSERTM( CRUDE_CHANNEL_GRAPHICS, scissor.extent.x > 0 && scissor.extent.y > 0 && scissor.offset.x >= 0 && scissor.offset.y >= 0, "vk_scissor issues!" );
#elif CRUDE_GFX_DX12
#elif CRUDE_GFX_NAPI
#else
  CRUDE_GFX_RHI_TO_IMPLEMENTIT
#endif
  
  crude_gfx_rhi_command_buffer_set_scissor( cmd->rhi_cmd_buffer, &scissor );
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
  crude_gfx_rhi_command_buffer_draw( cmd->rhi_cmd_buffer, vertex_count, instance_count, first_vertex, first_instance );
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
  crude_gfx_rhi_command_buffer_draw_indirect( cmd->rhi_cmd_buffer, buffer->rhi_buffer, offset, draw_count, stride );
}

void
crude_gfx_cmd_draw_indirect_count
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
  
  crude_gfx_rhi_command_buffer_draw_indirect_count(
    cmd->rhi_cmd_buffer,
    argument_buffer->rhi_buffer, argument_offset,
    count_buffer->rhi_buffer, count_offset,
    max_draws, stride );
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
  crude_gfx_rhi_command_buffer_draw_mesh_task( &cmd->gpu->rhi_device, cmd->rhi_cmd_buffer, group_count_x, group_count_y, group_count_z );
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
  
  crude_gfx_rhi_command_buffer_draw_mesh_task_indirect_count( &cmd->gpu->rhi_device, cmd->rhi_cmd_buffer, argument_buffer->rhi_buffer, argument_offset, count_buffer->rhi_buffer, count_offset, max_draws, stride );
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
  crude_gfx_rhi_command_buffer_dispatch( cmd->rhi_cmd_buffer, group_count_x, group_count_y, group_count_z );
}

void
crude_gfx_cmd_bind_bindless_descriptor_set
(
  _In_ crude_gfx_cmd_buffer                               *cmd
)
{
  crude_gfx_descriptor_set                                *bindless_descriptor_set;

  bindless_descriptor_set = crude_gfx_access_descriptor_set( cmd->gpu, cmd->gpu->bindless_descriptor_set_handle );
  crude_gfx_rhi_command_buffer_bind_descriptor_sets( cmd->rhi_cmd_buffer, cmd->current_pipeline->bind_point, cmd->current_pipeline->rhi_pipeline_layout, CRUDE_BINDLESS_DESCRIPTOR_SET_INDEX, bindless_descriptor_set->rhi_descriptor_set );
}

void
crude_gfx_cmd_bind_acceleration_structure_descriptor_set
(
  _In_ crude_gfx_cmd_buffer                               *cmd,
  _In_ crude_gfx_descriptor_set_handle                     handle
)
{
  crude_gfx_descriptor_set                                *descriptor_set;

  descriptor_set = crude_gfx_access_descriptor_set( cmd->gpu, handle );
  crude_gfx_rhi_command_buffer_bind_descriptor_sets( cmd->rhi_cmd_buffer, cmd->current_pipeline->bind_point, cmd->current_pipeline->rhi_pipeline_layout, CRUDE_ACCELERATION_STRUCTURE_DESCRIPTOR_SET_INDEX, descriptor_set->rhi_descriptor_set );
}

void
crude_gfx_cmd_add_buffer_barrier
(
  _In_ crude_gfx_cmd_buffer                               *cmd,
  _In_ crude_gfx_buffer_handle                             buffer_handle,
  _In_ crude_gfx_rhi_resource_state                        old_state,
  _In_ crude_gfx_rhi_resource_state                        new_state
)
{
  crude_gfx_buffer                                        *buffer;
  crude_gfx_rhi_buffer_memory_barrier                      buffer_memory_barrier;

  buffer = crude_gfx_access_buffer( cmd->gpu, buffer_handle );

  buffer_memory_barrier = CRUDE_COMPOUNT_EMPTY( crude_gfx_rhi_buffer_memory_barrier );
  
  buffer_memory_barrier.src_queue = crude_gfx_rhi_queue_empty( );
  buffer_memory_barrier.dst_queue = crude_gfx_rhi_queue_empty( );

  buffer_memory_barrier.src_access_mask = crude_gfx_rhi_resource_state_to_access_flags( old_state );
  buffer_memory_barrier.src_stage_mask = crude_gfx_rhi_determine_pipeline_stage_flags(
    buffer_memory_barrier.src_access_mask, CRUDE_GFX_RHI_QUEUE_TYPE_GRAPHICS );
  
  buffer_memory_barrier.dst_access_mask = crude_gfx_rhi_resource_state_to_access_flags( new_state );
  buffer_memory_barrier.dst_stage_mask = crude_gfx_rhi_determine_pipeline_stage_flags(
    buffer_memory_barrier.dst_access_mask, CRUDE_GFX_RHI_QUEUE_TYPE_GRAPHICS );
  
  buffer_memory_barrier.buffer = &buffer->rhi_buffer;
  buffer_memory_barrier.offset = 0;
  buffer_memory_barrier.size = buffer->size;
  
  crude_gfx_rhi_command_buffer_pipeline_buffer_barrier( cmd->rhi_cmd_buffer, &buffer_memory_barrier );
}

void
crude_gfx_cmd_add_image_barrier
(
  _In_ crude_gfx_cmd_buffer                               *cmd,
  _In_ crude_gfx_texture                                  *texture,
  _In_ crude_gfx_rhi_resource_state                        new_state,
  _In_ uint32                                              base_mip_level,
  _In_ uint32                                              mip_count,
  _In_ bool                                                is_depth
)
{
  crude_gfx_cmd_add_image_barrier_ext2( cmd, texture->rhi_image, texture->state, new_state, base_mip_level, mip_count, is_depth );
  texture->state = new_state;
}

void
crude_gfx_cmd_add_image_barrier_ext
(
  _In_ crude_gfx_cmd_buffer                               *cmd,
  _In_ crude_gfx_texture                                  *texture,
  _In_ crude_gfx_rhi_resource_state                        new_state,
  _In_ uint32                                              base_mip_level,
  _In_ uint32                                              mip_count,
  _In_ bool                                                is_depth,
  _In_ crude_gfx_rhi_queue                                 source_queue,
  _In_ crude_gfx_rhi_queue                                 destination_queue,
  _In_ crude_gfx_rhi_queue_type                            source_queue_type,
  _In_ crude_gfx_rhi_queue_type                            destination_queue_type
)
{
  crude_gfx_cmd_add_image_barrier_ext3( cmd, texture->rhi_image, texture->state, new_state, base_mip_level, mip_count, is_depth, source_queue, destination_queue, source_queue_type, destination_queue_type );
  texture->state = new_state;
}

void
crude_gfx_cmd_add_image_barrier_ext2
(
  _In_ crude_gfx_cmd_buffer                               *cmd,
  _In_ crude_gfx_rhi_image                                 rhi_image,
  _In_ crude_gfx_rhi_resource_state                        old_state,
  _In_ crude_gfx_rhi_resource_state                        new_state,
  _In_ uint32                                              base_mip_level,
  _In_ uint32                                              mip_count,
  _In_ bool                                                is_depth
)
{
  crude_gfx_cmd_add_image_barrier_ext3( cmd, rhi_image, old_state, new_state, base_mip_level, mip_count, is_depth, crude_gfx_rhi_queue_empty( ), crude_gfx_rhi_queue_empty( ), CRUDE_GFX_RHI_QUEUE_TYPE_GRAPHICS, CRUDE_GFX_RHI_QUEUE_TYPE_GRAPHICS );
}

void
crude_gfx_cmd_add_image_barrier_ext3
(
  _In_ crude_gfx_cmd_buffer                               *cmd,
  _In_ crude_gfx_rhi_image                                 rhi_image,
  _In_ crude_gfx_rhi_resource_state                        old_state,
  _In_ crude_gfx_rhi_resource_state                        new_state,
  _In_ uint32                                              base_mip_level,
  _In_ uint32                                              mip_count,
  _In_ bool                                                is_depth,
  _In_ crude_gfx_rhi_queue                                 source_queue,
  _In_ crude_gfx_rhi_queue                                 destination_queue,
  _In_ crude_gfx_rhi_queue_type                            source_queue_type,
  _In_ crude_gfx_rhi_queue_type                            destination_queue_type
)
{
  crude_gfx_cmd_add_image_barrier_ext5( cmd, rhi_image, old_state, new_state, base_mip_level, mip_count, 0u, 1u, is_depth, source_queue, destination_queue, source_queue_type, destination_queue_type );
}

void
crude_gfx_cmd_add_image_barrier_ext4
(
  _In_ crude_gfx_cmd_buffer                               *cmd,
  _In_ crude_gfx_texture                                  *texture,
  _In_ crude_gfx_rhi_resource_state                        new_state,
  _In_ uint32                                              base_mip_level,
  _In_ uint32                                              mip_count,
  _In_ uint32                                              base_array_layer,
  _In_ uint32                                              array_layer_count,
  _In_ bool                                                is_depth
)
{
  crude_gfx_cmd_add_image_barrier_ext5( cmd, texture->rhi_image, texture->state, new_state, base_mip_level, mip_count, 0u, 1u, is_depth, crude_gfx_rhi_queue_empty( ), crude_gfx_rhi_queue_empty( ), CRUDE_GFX_RHI_QUEUE_TYPE_GRAPHICS, CRUDE_GFX_RHI_QUEUE_TYPE_GRAPHICS );
  texture->state = new_state;
}

void
crude_gfx_cmd_add_image_barrier_ext5
(
  _In_ crude_gfx_cmd_buffer                               *cmd,
  _In_ crude_gfx_rhi_image                                 rhi_image,
  _In_ crude_gfx_rhi_resource_state                        old_state,
  _In_ crude_gfx_rhi_resource_state                        new_state,
  _In_ uint32                                              base_mip_level,
  _In_ uint32                                              mip_count,
  _In_ uint32                                              base_array_layer,
  _In_ uint32                                              array_layer_count,
  _In_ bool                                                is_depth,
  _In_ crude_gfx_rhi_queue                                 source_queue,
  _In_ crude_gfx_rhi_queue                                 destination,
  _In_ crude_gfx_rhi_queue_type                            source_queue_type,
  _In_ crude_gfx_rhi_queue_type                            destination_queue_type
)
{
  crude_gfx_rhi_image_memory_barrier                       image_memory_barrier;
  crude_gfx_rhi_access_flags                               src_access_mask, dst_access_mask;

  //CRUDE_LOG_INFO( CRUDE_CHANNEL_GRAPHICS, "Transitioning Texture %s from %s to %s", texture->name, crude_gfx_resource_state_to_name( texture->state ), crude_gfx_resource_state_to_name( new_state ) );
  
  src_access_mask = crude_gfx_rhi_resource_state_to_access_flags( old_state );
  dst_access_mask = crude_gfx_rhi_resource_state_to_access_flags( new_state );

  image_memory_barrier.src_stage_mask = crude_gfx_rhi_determine_pipeline_stage_flags( src_access_mask, source_queue_type );
  image_memory_barrier.src_access_mask = src_access_mask;
  image_memory_barrier.dst_stage_mask = crude_gfx_rhi_determine_pipeline_stage_flags( dst_access_mask, destination_queue_type );
  image_memory_barrier.dst_access_mask = dst_access_mask;
  image_memory_barrier.old_layout = crude_gfx_rhi_resource_state_to_image_layout( old_state );
  image_memory_barrier.new_layout = crude_gfx_rhi_resource_state_to_image_layout( new_state );
  image_memory_barrier.src_queue = source_queue;
  image_memory_barrier.dst_queue = destination;
  image_memory_barrier.image = rhi_image;
  image_memory_barrier.subresource_range.aspect_mask = is_depth ? CRUDE_GFX_RHI_IMAGE_ASPECT_DEPTH_BIT : CRUDE_GFX_RHI_IMAGE_ASPECT_COLOR_BIT;
  image_memory_barrier.subresource_range.base_mip_level = base_mip_level;
  image_memory_barrier.subresource_range.level_count = mip_count;
  image_memory_barrier.subresource_range.base_array_layer = base_array_layer;
  image_memory_barrier.subresource_range.layer_count = array_layer_count;

  crude_gfx_rhi_command_buffer_pipeline_image_barrier( cmd->rhi_cmd_buffer, &image_memory_barrier );
}

void
crude_gfx_cmd_global_debug_barrier
(
  _In_ crude_gfx_cmd_buffer                               *cmd
)
{
  crude_gfx_rhi_command_buffer_pipeline_global_barrier( cmd->rhi_cmd_buffer );
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
  crude_gfx_rhi_buffer_image_copy                          region;

  texture = crude_gfx_access_texture( cmd->gpu, texture_handle );
  buffer = crude_gfx_access_buffer( cmd->gpu, memory_allocation.buffer_handle );

  region = CRUDE_COMPOUNT_EMPTY( crude_gfx_rhi_buffer_image_copy );
  region.buffer_offset = 0;
  region.buffer_row_length = 0;
  region.buffer_image_height = 0;
  region.image_subresource.aspect_mask = CRUDE_GFX_RHI_IMAGE_ASPECT_COLOR_BIT;
  region.image_subresource.mip_level = 0;
  region.image_subresource.base_array_layer = 0;
  region.image_subresource.layer_count = 1;
  region.image_offset.x = 0;
  region.image_offset.y = 0;
  region.image_offset.z = 0;
  region.image_extent.x = texture->width;
  region.image_extent.y = texture->height;
  region.image_extent.z = 1;
  
  crude_gfx_cmd_add_image_barrier( cmd, texture, CRUDE_GFX_RHI_RESOURCE_STATE_COPY_DEST, 0, 1, false );
  crude_gfx_rhi_command_buffer_copy_buffer_to_image( cmd->rhi_cmd_buffer, buffer->rhi_buffer, texture->rhi_image, &region );
  crude_gfx_cmd_add_image_barrier_ext( cmd, texture, CRUDE_GFX_RHI_RESOURCE_STATE_COPY_SOURCE, 0, 1, false, cmd->gpu->rhi_transfer_queue, cmd->gpu->rhi_main_queue, CRUDE_GFX_RHI_QUEUE_TYPE_COPY_TRANSFER, CRUDE_GFX_RHI_QUEUE_TYPE_GRAPHICS );
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
  crude_gfx_rhi_buffer_copy                                region;

  src_buffer = crude_gfx_access_buffer( cmd->gpu, src_memory_allocation.buffer_handle );
  dst_buffer = crude_gfx_access_buffer( cmd->gpu, dst_memory_allocation.buffer_handle );

  CRUDE_ASSERTM( CRUDE_CHANNEL_GRAPHICS, dst_memory_allocation.size, "%s dst buffer size == 0", dst_buffer->name ? dst_buffer->name : "unknown" )
  CRUDE_ASSERTM( CRUDE_CHANNEL_GRAPHICS, src_memory_allocation.size <= dst_memory_allocation.size, "%s src buffer size > %s dst buffer size", src_buffer->name ? src_buffer->name : "unknown", dst_buffer->name ? dst_buffer->name : "unknown" )

  region = CRUDE_COMPOUNT_EMPTY( crude_gfx_rhi_buffer_copy );
  region.src_offset = src_offset + src_memory_allocation.offset;
  region.dst_offset = dst_offset + dst_memory_allocation.offset;
  region.size = src_memory_allocation.size;

  //CRUDE_ASSERTM( CRUDE_CHANNEL_GRAPHICS, src_memory_allocation.aligned_size - vk_region.srcOffset <= dst_memory_allocation.aligned_size - vk_region.dstOffset, "%s src buffer size < %s dst buffer size - offset", src_buffer->name ? src_buffer->name : "unknown", dst_buffer->name ? dst_buffer->name : "unknown" )

  crude_gfx_rhi_command_buffer_copy_buffer( cmd->rhi_cmd_buffer, src_buffer->rhi_buffer, dst_buffer->rhi_buffer, &region );
}

void
crude_gfx_cmd_push_marker
(
  _In_ crude_gfx_cmd_buffer                               *cmd,
  _In_ char const                                         *name
)
{
#if CRUDE_GFX_GPU_PROFILER
  crude_gfx_cmd_pool                                      *cmd_pool;
  crude_gfx_gpu_time_query                                *time_query;

  cmd_pool = crude_gfx_access_cmd_pool( cmd->gpu, cmd->cmd_pool );

  if ( cmd_pool->profiler.enabled )
  {
    time_query = crude_gfx_gpu_time_query_tree_push( cmd_pool->profiler.time_queries_trees, name );
    crude_gfx_rhi_command_buffer_write_timestamp( cmd->rhi_cmd_buffer, CRUDE_GFX_RHI_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, cmd_pool->profiler.rhi_timestamp_query_pool, time_query->start_query_index );
  }
#endif

#if CRUDE_GRAPHICS_VALIDATION_LAYERS_ENABLED
  crude_gfx_rhi_debug_utils_label label = CRUDE_COMPOUNT_EMPTY( crude_gfx_rhi_debug_utils_label );
  label.label_name = name;
  label.color[ 0 ] = 1.0f;
  label.color[ 1 ] = 1.0f;
  label.color[ 2 ] = 1.0f;
  label.color[ 3 ] = 1.0f;
  crude_gfx_rhi_command_buffer_begin_debug_utils_label( &cmd->gpu->rhi_device, cmd->rhi_cmd_buffer, &label );
#endif
  
#if CRUDE_GFX_NSIGHT_AFTERMATH
    //// A helper for setting a checkpoint marker
    //auto setCheckpointMarker = [this](vk::CommandBuffer commandBuffer, const std::string& markerData)
    //{
    //    // App is responsible for handling marker memory, and for resolving the memory at crash dump generation time.
    //    // The actual "const void* pCheckpointMarker" passed to setCheckpointNV in this case can be any uniquely identifying value that the app can resolve to the marker data later.
    //    // For this sample, we will use this approach to generating a unique marker value:
    //    // We keep a ringbuffer with a marker history of the last c_markerFrameHistory frames (currently 4).
    //    unsigned int markerMapIndex = frameNumber % GpuCrashTracker::c_markerFrameHistory;
    //    auto& currentFrameMarkerMap = markerMap[markerMapIndex];
    //    // Take the index into the ringbuffer, multiply by 10000, and add the total number of markers logged so far in the current frame, +1 to avoid a value of zero.
    //    size_t markerID = markerMapIndex * 10000 + currentFrameMarkerMap.size() + 1;
    //    // This value is the unique identifier we will pass to Aftermath and internally associate with the marker data in the map.
    //    currentFrameMarkerMap[markerID] = markerData;
    //    commandBuffer.setCheckpointNV((const void*)markerID);
    //    // For example, if we are on frame 625, markerMapIndex = 625 % 4 = 1...
    //    // The first marker for the frame will have markerID = 1 * 10000 + 0 + 1 = 10001.
    //    // The 15th marker for the frame will have markerID = 1 * 10000 + 14 + 1 = 10015.
    //    // On the next frame, 626, markerMapIndex = 626 % 4 = 2.
    //    // The first marker for this frame will have markerID = 2 * 10000 + 0 + 1 = 20001.
    //    // The 15th marker for the frame will have markerID = 2 * 10000 + 14 + 1 = 20015.
    //    // So with this scheme, we can safely have up to 10000 markers per frame, and can guarantee a unique markerID for each one.
    //    // There are many ways to generate and track markers and unique marker identifiers!
    //};
    //// clear the marker map for the current frame before writing any markers
    //markerMap[frameNumber % GpuCrashTracker::c_markerFrameHistory].clear();
    //
    //// A helper that prepends the frame number to a string
    //auto createMarkerStringForFrame = [this](const char* markerString) {
    //    std::stringstream ss;
    //    ss << "Frame " << frameNumber << ": " << markerString;
    //    return ss.str();
    //};
    //// Insert a device diagnostic checkpoint into the command stream
    //setCheckpointMarker(commandBuffer, createMarkerStringForFrame("Draw Cube"));
#endif
}

void
crude_gfx_cmd_pop_marker
(
  _In_ crude_gfx_cmd_buffer                               *cmd
)
{
#if CRUDE_GFX_GPU_PROFILER
  crude_gfx_cmd_pool                                      *cmd_pool;
  crude_gfx_gpu_time_query                                *time_query;

  cmd_pool = crude_gfx_access_cmd_pool( cmd->gpu, cmd->cmd_pool );

  if ( cmd_pool->profiler.enabled )
  {
    time_query = crude_gfx_gpu_time_query_tree_pop( cmd_pool->profiler.time_queries_trees );
    
    crude_gfx_rhi_command_buffer_write_timestamp( cmd->rhi_cmd_buffer, CRUDE_GFX_RHI_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, cmd_pool->profiler.rhi_timestamp_query_pool, time_query->end_query_index );
  }
#endif

#if CRUDE_GRAPHICS_VALIDATION_LAYERS_ENABLED
  crude_gfx_rhi_command_buffer_end_debug_utils_label( &cmd->gpu->rhi_device, cmd->rhi_cmd_buffer );
#endif
}

void
crude_gfx_cmd_push_constant
(
  _In_ crude_gfx_cmd_buffer                               *cmd,
  _In_ void const                                         *data,
  _In_ uint64                                              size
)
{
  crude_gfx_rhi_command_buffer_push_constant( cmd->rhi_cmd_buffer, cmd->current_pipeline->rhi_pipeline_layout, CRUDE_GFX_RHI_SHADER_STAGE_ALL, 0, size, data );
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
  crude_gfx_rhi_command_buffer_fill_buffer( cmd->rhi_cmd_buffer, buffer->rhi_buffer, 0, buffer->size, value );
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
#if CRUDE_GFX_RAY_TRACING_ENABLED
  crude_gfx_pipeline                                      *pipeline;
  crude_gfx_rhi_strided_device_address_region              raygen_table, hit_table, miss_table, callable_table;
  uint32                                                   shader_group_handle_size;

  pipeline = crude_gfx_access_pipeline( cmd->gpu, pipeline_handle );

  shader_group_handle_size = cmd->gpu->ray_tracing_pipeline_properties.shader_group_handle_size;

  raygen_table = CRUDE_COMPOUNT_EMPTY( crude_gfx_rhi_strided_device_address_region );
  raygen_table.device_address = crude_gfx_get_buffer_device_address( cmd->gpu, pipeline->shader_binding_table_raygen );
  raygen_table.stride = shader_group_handle_size;
  raygen_table.size = shader_group_handle_size;
  
  hit_table = CRUDE_COMPOUNT_EMPTY( crude_gfx_rhi_strided_device_address_region );
  hit_table.device_address = crude_gfx_get_buffer_device_address( cmd->gpu, pipeline->shader_binding_table_hit );
  hit_table.stride = shader_group_handle_size;
  hit_table.size = shader_group_handle_size;
  
  miss_table = CRUDE_COMPOUNT_EMPTY( crude_gfx_rhi_strided_device_address_region );
  miss_table.device_address = crude_gfx_get_buffer_device_address( cmd->gpu, pipeline->shader_binding_table_miss );
  miss_table.stride = shader_group_handle_size;
  miss_table.size = shader_group_handle_size;

  callable_table = CRUDE_COMPOUNT_EMPTY( crude_gfx_rhi_strided_device_address_region );
  
  crude_gfx_rhi_command_buffer_trace_rays( &cmd->gpu->rhi_device, cmd->rhi_cmd_buffer, &raygen_table, &miss_table, &hit_table, &callable_table, width, height, depth );
#else
  CRUDE_ASSERTM( CRUDE_CHANNEL_GRAPHICS, false, "Can proccess crude_gfx_cmd_trace_rays, CRUDE_GFX_RAY_TRACING_ENABLED wasn't enabled" );
#endif /* CRUDE_GFX_RAY_TRACING_ENABLED */
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
  _In_ uint32                                              num_pools_per_frame,
  _In_ uint32                                              num_primary_cmd_buffers_per_pool
)
{
  uint32                                                   total_pools;
  uint32                                                   total_buffers;

  cmd_manager->gpu = gpu;
  cmd_manager->num_pools_per_frame = num_pools_per_frame;
  cmd_manager->num_primary_cmd_buffers_per_thread = num_primary_cmd_buffers_per_pool;

  total_pools = CRUDE_GFX_SWAPCHAIN_IMAGES_MAX_COUNT * cmd_manager->num_pools_per_frame;

  CRUDE_ARRAY_INITIALIZE_WITH_LENGTH( cmd_manager->num_used_primary_cmd_buffers_per_frame, total_pools, crude_heap_allocator_pack( gpu->allocator ) );
  for ( uint32 i = 0; i < total_pools; ++i )
  {
    cmd_manager->num_used_primary_cmd_buffers_per_frame[ i ] = 0;
  }
  
  total_buffers = total_pools * cmd_manager->num_primary_cmd_buffers_per_thread;
  CRUDE_ARRAY_INITIALIZE_WITH_LENGTH( cmd_manager->primary_cmd_buffers, total_buffers, crude_heap_allocator_pack( gpu->allocator ) );
  
  for ( uint32 i = 0; i < total_buffers; i++ )
  {
    crude_gfx_cmd_buffer_creation                          cmd_buffer_creation;
    uint32                                                 frame_index, thread_index, pool_index;

    frame_index = i / ( cmd_manager->num_primary_cmd_buffers_per_thread * cmd_manager->num_pools_per_frame );
    thread_index = ( i / cmd_manager->num_primary_cmd_buffers_per_thread ) % cmd_manager->num_pools_per_frame;
    pool_index = pool_from_indices( cmd_manager, frame_index, thread_index );
    
    cmd_buffer_creation = CRUDE_COMPOUNT_EMPTY( crude_gfx_cmd_buffer_creation );
    crude_snprintf( cmd_buffer_creation.name, sizeof( cmd_buffer_creation.name ), "primary_cmd frame: %i thread: %i pool: %i", frame_index, thread_index, pool_index );
    cmd_buffer_creation.cmd_pool = cmd_manager->gpu->thread_frame_pools[ pool_index ];
    cmd_manager->primary_cmd_buffers[ i ] = crude_gfx_create_cmd_buffer( gpu, &cmd_buffer_creation );
  }
}

void
crude_gfx_cmd_manager_deinitialize
(
  _In_ crude_gfx_cmd_buffer_manager                       *cmd_manager
)
{
  for ( uint32 i = 0; i < CRUDE_ARRAY_LENGTH( cmd_manager->primary_cmd_buffers ) ; ++i )
  {
    crude_gfx_destroy_cmd_buffer_instant( cmd_manager->gpu, cmd_manager->primary_cmd_buffers[ i ] );
  }
  CRUDE_ARRAY_DEINITIALIZE( cmd_manager->primary_cmd_buffers );
  CRUDE_ARRAY_DEINITIALIZE( cmd_manager->num_used_primary_cmd_buffers_per_frame );
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
    crude_gfx_reset_cmd_pool( cmd_manager->gpu, cmd_manager->gpu->thread_frame_pools[ pool_index ] );
  }

  for ( uint32 i = 0; i < cmd_manager->num_pools_per_frame; ++i )
  {
    uint32 pool_index = pool_from_indices( cmd_manager, frame, i );
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
  crude_gfx_cmd_buffer                                    *cmd;
  uint32                                                   pool_index, current_used_buffer, cmd_index;

  pool_index = pool_from_indices( cmd_manager, frame, thread_index );
  current_used_buffer = cmd_manager->num_used_primary_cmd_buffers_per_frame[ pool_index ];
  cmd_index = ( pool_index * cmd_manager->num_primary_cmd_buffers_per_thread ) + current_used_buffer;
  
  cmd = crude_gfx_access_cmd_buffer( cmd_manager->gpu, cmd_manager->primary_cmd_buffers[ cmd_index ] );

  if ( begin )
  {  
    CRUDE_ASSERT( !cmd->is_recording );

    crude_gfx_cmd_reset( cmd );
    crude_gfx_cmd_begin_primary( cmd );
    CRUDE_ASSERT( current_used_buffer < cmd_manager->num_primary_cmd_buffers_per_thread );
    cmd_manager->num_used_primary_cmd_buffers_per_frame[ pool_index ] = current_used_buffer + 1;
    
#if CRUDE_GFX_GPU_PROFILER
    crude_gfx_cmd_pool                                    *cmd_pool;
    crude_gfx_gpu_time_query                              *time_query;

    cmd_pool = crude_gfx_access_cmd_pool( cmd->gpu, cmd->cmd_pool );

    if ( cmd_pool->profiler.enabled )
    {
      crude_gfx_gpu_time_query_tree_reset( cmd_pool->profiler.time_queries_trees );
      crude_gfx_rhi_command_buffer_reset_query_pool( cmd->rhi_cmd_buffer, cmd_pool->profiler.rhi_timestamp_query_pool, 0, 2 * cmd_pool->profiler.time_queries_trees->time_queries_count );
      crude_gfx_rhi_command_buffer_reset_query_pool( cmd->rhi_cmd_buffer, cmd_pool->profiler.rhi_pipeline_stats_query_pool, 0, CRUDE_GFX_GPU_PIPELINE_STATISTICS_COUNT );
      crude_gfx_rhi_command_buffer_begin_query( cmd->rhi_cmd_buffer, cmd_pool->profiler.rhi_pipeline_stats_query_pool, 0, 0 );
    }
#endif
  }
  
  return cmd;
}