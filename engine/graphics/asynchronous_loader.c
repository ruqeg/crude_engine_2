#include <stb_image.h>

#include <core/algorithms.h>
#include <core/time.h>

#include <graphics/asynchronous_loader.h>

void
crude_gfx_initialize_asynchronous_loader
(
  _In_ crude_gfx_asynchronous_loader            *asynloader,
  _In_ crude_renderer                           *renderer
)
{
  asynloader->renderer = renderer;

  asynloader->file_load_requests = NULL;
  asynloader->upload_requests = NULL;

  for ( uint32 i = 0; i < CRUDE_MAX_SWAPCHAIN_IMAGES; ++i )
  {
    VkCommandPoolCreateInfo cmd_pool_info = {
      .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
      .queueFamilyIndex = renderer->gpu->vk_transfer_queue_family,
      .flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
    };
    
    vkCreateCommandPool( renderer->gpu->vk_device, &cmd_pool_info, renderer->gpu->vk_allocation_callbacks, &asynloader->vk_cmd_pools[ i ] );
    
    VkCommandBufferAllocateInfo cmd = {
      .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
      .commandPool = asynloader->vk_cmd_pools[ i ],
      .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
      .commandBufferCount = 1,
    
    };
    
    vkAllocateCommandBuffers( renderer->gpu->vk_device, &cmd, &asynloader->cmd_buffers[ i ].vk_cmd_buffer );
    
    asynloader->cmd_buffers[ i ].is_recording = false;
    asynloader->cmd_buffers[ i ].gpu = renderer->gpu;
  }
  
  crude_buffer_creation buffer_creation = {
    .type_flags = VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
    .usage = CRUDE_RESOURCE_USAGE_TYPE_STREAM,
    .size = 64 * 1024 * 1024,
    .name = "staging buffer",
    .persistent = true
  };
  crude_buffer_handle staging_buffer_handle = crude_gfx_create_buffer( renderer->gpu, &buffer_creation );
  asynloader->staging_buffer = CRUDE_GFX_GPU_ACCESS_BUFFER( renderer->gpu, staging_buffer_handle );
  
  VkSemaphoreCreateInfo semaphore_info = {
    .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO
  };
  CRUDE_GFX_HANDLE_VULKAN_RESULT( vkCreateSemaphore( renderer->gpu->vk_device, &semaphore_info, renderer->gpu->vk_allocation_callbacks, &asynloader->vk_transfer_complete_semaphore ), "Failed to create semaphore" );
  
  VkFenceCreateInfo fence_info = {
    .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
    .flags = VK_FENCE_CREATE_SIGNALED_BIT
  };
  CRUDE_GFX_HANDLE_VULKAN_RESULT( vkCreateFence( renderer->gpu->vk_device, &fence_info, renderer->gpu->vk_allocation_callbacks, &asynloader->vk_transfer_fence ), "Failed to create fence" );
}

void
crude_gfx_asynchronous_loader_request_texture_data
(
  _In_ crude_gfx_asynchronous_loader            *asynloader,
  _In_ char const                               *filename,
  _In_ crude_texture_handle                      texture
)
{
  crude_gfx_file_load_request request;
  strcpy( request.path, filename );
  request.texture = texture;
  request.buffer = CRUDE_GFX_INVALID_BUFFER_HANDLE;
  CRUDE_ARR_PUSH( asynloader->file_load_requests, request );
}

void
crude_gfx_asynchronous_loader_update
(
  _In_ crude_gfx_asynchronous_loader            *asynloader
)
{
  
  if ( asynloader->texture_ready.index != CRUDE_GFX_INVALID_TEXTURE_HANDLE.index )
  {
    crude_gfx_renderer_add_texture_to_update( asynloader->renderer, asynloader->texture_ready );
    asynloader->texture_ready = CRUDE_GFX_INVALID_TEXTURE_HANDLE;
  }
  
  if ( CRUDE_ARR_LEN( asynloader->upload_requests ) )
  {
    if ( vkGetFenceStatus( asynloader->renderer->gpu->vk_device, asynloader->vk_transfer_fence ) != VK_SUCCESS )
    {
      return;
    }
    
    vkResetFences( asynloader->renderer->gpu->vk_device, 1, &asynloader->vk_transfer_fence );
    
    crude_gfx_upload_request request = CRUDE_ARR_POP( asynloader->upload_requests );

    crude_command_buffer *cmd = &asynloader->cmd_buffers[ asynloader->renderer->gpu->current_frame ];
    crude_gfx_cmd_begin_primary( cmd );

    if ( request.texture.index != CRUDE_GFX_INVALID_TEXTURE_HANDLE.index )
    {
      crude_texture *texture = CRUDE_GFX_GPU_ACCESS_TEXTURE( asynloader->renderer->gpu, request.texture );
      uint32 texture_channels = 4;
      uint32 texture_alignment = 4;
      uint64 aligned_image_size = crude_memory_align( texture->width * texture->height * texture_channels, texture_alignment );
      sizet current_offset = asynloader->staging_buffer_offset + aligned_image_size;
      
      crude_gfx_cmd_upload_texture_data( cmd, texture->handle, request.data, asynloader->staging_buffer->handle, current_offset );
     
      free( request.data );
    }
        
    crude_gfx_cmd_end( cmd );

    VkPipelineStageFlags wait_flag[] = { VK_PIPELINE_STAGE_TRANSFER_BIT };
    VkSemaphore wait_semaphore[] = { asynloader->vk_transfer_complete_semaphore };
    VkSubmitInfo submitInfo = { 
      .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
      .commandBufferCount = 1,
      .pCommandBuffers = &cmd->vk_cmd_buffer,
      .pWaitSemaphores = wait_semaphore,
      .pWaitDstStageMask = wait_flag,
    };

    VkQueue used_queue = asynloader->renderer->gpu->vk_transfer_queue;
    vkQueueSubmit( used_queue, 1, &submitInfo, asynloader->vk_transfer_fence );

    if ( request.texture.index != CRUDE_GFX_INVALID_TEXTURE_HANDLE.index )
    {
      CRUDE_ASSERT( asynloader->texture_ready.index == CRUDE_GFX_INVALID_TEXTURE_HANDLE.index );
      asynloader->texture_ready = request.texture;
    }
  }

  if ( CRUDE_ARR_LEN( asynloader->file_load_requests ) )
  {
    crude_gfx_file_load_request load_request = CRUDE_ARR_POP( asynloader->file_load_requests );
    
    int64 start_reading_file = crude_time_now();
    int x, y, comp;
    uint8 *texture_data = stbi_load( load_request.path, &x, &y, &comp, 4 );
    if ( texture_data )
    {
      CRUDE_LOG_INFO( CRUDE_CHANNEL_GRAPHICS, "File %s read in %fs", load_request.path, crude_time_delta_seconds( start_reading_file, crude_time_now() ) );

      crude_gfx_upload_request upload_request;
      upload_request.data = texture_data;
      upload_request.texture = load_request.texture;
      upload_request.cpu_buffer = CRUDE_GFX_INVALID_BUFFER_HANDLE;
      CRUDE_ARR_PUSH( asynloader->upload_requests, upload_request );
    }
    else
    {
      CRUDE_LOG_ERROR( CRUDE_CHANNEL_GRAPHICS, "Error reading file %s", load_request.path );
    }
  }
}