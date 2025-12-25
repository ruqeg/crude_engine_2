#include <stb_image.h>
#include <TaskScheduler_c.h>

#include <engine/core/profiler.h>
#include <engine/core/array.h>
#include <engine/core/time.h>

#include <engine/graphics/asynchronous_loader.h>

static void
crude_gfx_asynchronous_loader_push_upload_requests_
(
  _In_ crude_gfx_asynchronous_loader                      *asynloader,
  _In_ crude_gfx_upload_request                            upload_request
);

static void
crude_gfx_asynchronous_loader_push_file_load_request_
(
  _In_ crude_gfx_asynchronous_loader                      *asynloader,
  _In_ crude_gfx_file_load_request                         file_load_request
);

static crude_gfx_upload_request
crude_gfx_asynchronous_loader_pop_upload_requests_
(
  _In_ crude_gfx_asynchronous_loader                      *asynloader
);

static crude_gfx_file_load_request
crude_gfx_asynchronous_loader_pop_file_load_request_
(
  _In_ crude_gfx_asynchronous_loader                      *asynloader
);

/************************************************
 *
 * Asynchronous Functions Implementation
 * 
 ***********************************************/
void
crude_gfx_asynchronous_loader_initialize
(
  _In_ crude_gfx_asynchronous_loader                      *asynloader,
  _In_ crude_gfx_device                                   *gpu
)
{
  CRUDE_LOG_INFO( CRUDE_CHANNEL_GRAPHICS, "Intiailize asynchronous loader" );

  asynloader->gpu = gpu;

  asynloader->texture_ready = CRUDE_GFX_TEXTURE_HANDLE_INVALID;
  asynloader->cpu_allocation_ready = crude_gfx_memory_allocation_empty( );
  asynloader->gpu_allocation_ready = crude_gfx_memory_allocation_empty( );
  asynloader->gpu_old_allocation_ready = crude_gfx_memory_allocation_empty( );

  asynloader->total_requests_count = 0;

  asynloader->file_load_requests_lpos = asynloader->file_load_requests_rpos = 0;
  asynloader->upload_requests_lpos = asynloader->upload_requests_rpos = 0;

  mtx_init( &asynloader->request_mutex, mtx_plain );

  for ( uint32 i = 0; i < CRUDE_GRAPHICS_MAX_SWAPCHAIN_IMAGES; ++i )
  {
    VkCommandPoolCreateInfo                                vk_cmd_pool_info;
    VkCommandBufferAllocateInfo                            vk_cmd_info;

    vk_cmd_pool_info = CRUDE_COMPOUNT_EMPTY( VkCommandPoolCreateInfo );
    vk_cmd_pool_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    vk_cmd_pool_info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    vk_cmd_pool_info.queueFamilyIndex = gpu->vk_transfer_queue_family;
    vkCreateCommandPool( gpu->vk_device, &vk_cmd_pool_info, gpu->vk_allocation_callbacks, &asynloader->vk_cmd_pools[ i ] );
    
    vk_cmd_info = CRUDE_COMPOUNT_EMPTY( VkCommandBufferAllocateInfo );
    vk_cmd_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    vk_cmd_info.commandPool = asynloader->vk_cmd_pools[ i ];
    vk_cmd_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    vk_cmd_info.commandBufferCount = 1;
    vkAllocateCommandBuffers( gpu->vk_device, &vk_cmd_info, &asynloader->cmd_buffers[ i ].vk_cmd_buffer );

    crude_gfx_set_resource_name( gpu, VK_OBJECT_TYPE_COMMAND_BUFFER, ( uint64 )asynloader->cmd_buffers[ i ].vk_cmd_buffer, "asynchronous_loader_cmd" );
    
    asynloader->cmd_buffers[ i ].is_recording = false;
    asynloader->cmd_buffers[ i ].gpu = gpu;
  }

  crude_linear_allocator_initialize( &asynloader->linear_allocator, sizeof( crude_gfx_upload_request ) * CRUDE_GRAPHICS_ASYNCHRONOUS_LOADER_UPLOAD_REQUESTS_LIMIT + sizeof( crude_gfx_file_load_request ) * CRUDE_GRAPHICS_ASYNCHRONOUS_LOADER_FILE_LOAD_REQUESTS_LIMIT + 3 * sizeof( crude_array_header ), "asynchronous_loader_linear_allocator" );
  CRUDE_ARRAY_INITIALIZE_WITH_LENGTH( asynloader->file_load_requests, CRUDE_GRAPHICS_ASYNCHRONOUS_LOADER_FILE_LOAD_REQUESTS_LIMIT, crude_linear_allocator_pack( &asynloader->linear_allocator ) );
  CRUDE_ARRAY_INITIALIZE_WITH_LENGTH( asynloader->upload_requests, CRUDE_GRAPHICS_ASYNCHRONOUS_LOADER_UPLOAD_REQUESTS_LIMIT, crude_linear_allocator_pack( &asynloader->linear_allocator ) );
  
  asynloader->staging_allocation = crude_gfx_memory_allocate( asynloader->gpu, 64 * 1024 * 1024, CRUDE_GFX_MEMORY_TYPE_CPU_GPU );

  {
    VkFenceCreateInfo fence_info = CRUDE_COMPOUNT_EMPTY( VkFenceCreateInfo );
    fence_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    CRUDE_GFX_HANDLE_VULKAN_RESULT( vkCreateFence( gpu->vk_device, &fence_info, gpu->vk_allocation_callbacks, &asynloader->vk_transfer_completed_fence ), "Failed to create fence" );
    crude_gfx_set_resource_name( gpu, VK_OBJECT_TYPE_FENCE, CRUDE_CAST( uint64, asynloader->vk_transfer_completed_fence ), "transfer_completed_fence" );
  }
}

void
crude_gfx_asynchronous_loader_deinitialize
(
  _In_ crude_gfx_asynchronous_loader                      *asynloader
)
{
  mtx_destroy( &asynloader->request_mutex );

  vkDestroyFence( asynloader->gpu->vk_device, asynloader->vk_transfer_completed_fence, asynloader->gpu->vk_allocation_callbacks );
  
  crude_linear_allocator_deinitialize( &asynloader->linear_allocator );
 
  for ( uint32 i = 0; i < CRUDE_GRAPHICS_MAX_SWAPCHAIN_IMAGES; ++i )
  {
    vkDestroyCommandPool( asynloader->gpu->vk_device, asynloader->vk_cmd_pools[ i ], asynloader->gpu->vk_allocation_callbacks );  
  }
}

void
crude_gfx_asynchronous_loader_request_texture_data
(
  _In_ crude_gfx_asynchronous_loader                      *asynloader,
  _In_ char const                                         *filename,
  _In_ crude_gfx_texture_handle                            texture_handle
)
{
  crude_gfx_file_load_request request;
  strcpy( request.path, filename );
  request.texture = texture_handle;
  request.allocation = crude_gfx_memory_allocation_empty( );

  crude_gfx_asynchronous_loader_push_file_load_request_( asynloader, request );

  crude_gfx_texture *texture = crude_gfx_access_texture( asynloader->gpu, texture_handle );
  texture->ready = false;
}

void
crude_gfx_asynchronous_loader_request_buffer_copy
(
  _In_ crude_gfx_asynchronous_loader                      *asynloader,
  _In_ crude_gfx_memory_allocation                         cpu_allocation,
  _In_ crude_gfx_memory_allocation                         gpu_allocation
)
{
  crude_gfx_buffer                                        *buffer;
  crude_gfx_upload_request                                 request;

  request = CRUDE_COMPOUNT_EMPTY( crude_gfx_upload_request );
  request.texture    = CRUDE_GFX_TEXTURE_HANDLE_INVALID;
  request.cpu_allocation = cpu_allocation;
  request.gpu_allocation = gpu_allocation;
  request.gpu_old_allocation = crude_gfx_memory_allocation_empty( );
  
  crude_gfx_asynchronous_loader_push_upload_requests_( asynloader, request );

  buffer = crude_gfx_access_buffer( asynloader->gpu, gpu_allocation.buffer_handle );
  buffer->ready = false;
}

CRUDE_API void
crude_gfx_asynchronous_loader_request_buffer_reallocate_and_copy
(
  _In_ crude_gfx_asynchronous_loader                      *asynloader,
  _In_ crude_gfx_memory_allocation                         cpu_allocation,
  _In_ crude_gfx_memory_allocation                         gpu_allocation,
  _In_ crude_gfx_memory_allocation                         gpu_old_allocation
)
{
  crude_gfx_buffer                                        *buffer;
  crude_gfx_upload_request                                 request;

  request = CRUDE_COMPOUNT_EMPTY( crude_gfx_upload_request );
  request.texture    = CRUDE_GFX_TEXTURE_HANDLE_INVALID;
  request.cpu_allocation = cpu_allocation;
  request.gpu_allocation = gpu_allocation;
  request.gpu_old_allocation = gpu_old_allocation;
  
  crude_gfx_asynchronous_loader_push_upload_requests_( asynloader, request );

  buffer = crude_gfx_access_buffer( asynloader->gpu, gpu_allocation.buffer_handle );
  buffer->ready = false;
}

void
crude_gfx_asynchronous_loader_update
(
  _In_ crude_gfx_asynchronous_loader                      *asynloader
)
{
  if ( CRUDE_RESOURCE_HANDLE_IS_VALID( asynloader->texture_ready ) )
  {
    crude_gfx_add_texture_to_update( asynloader->gpu, asynloader->texture_ready );

    asynloader->texture_ready = CRUDE_GFX_TEXTURE_HANDLE_INVALID;
    
    mtx_lock( &asynloader->request_mutex );
    --asynloader->total_requests_count;
    mtx_unlock( &asynloader->request_mutex );
  }
  
  if ( crude_gfx_memory_allocation_valid( &asynloader->cpu_allocation_ready ) && crude_gfx_memory_allocation_valid( &asynloader->gpu_allocation_ready ) )
  {
    if ( crude_gfx_memory_allocation_valid( &asynloader->gpu_old_allocation_ready ) )
    {
      crude_gfx_memory_deallocate( asynloader->gpu, asynloader->gpu_old_allocation_ready );
    }

    crude_gfx_memory_deallocate( asynloader->gpu, asynloader->cpu_allocation_ready );

    crude_gfx_buffer *buffer = crude_gfx_access_buffer( asynloader->gpu, asynloader->gpu_allocation_ready.buffer_handle );
    buffer->ready = true;

    asynloader->cpu_allocation_ready = crude_gfx_memory_allocation_empty( );
    asynloader->gpu_allocation_ready = crude_gfx_memory_allocation_empty( );
    asynloader->gpu_old_allocation_ready = crude_gfx_memory_allocation_empty( );
    
    mtx_lock( &asynloader->request_mutex );
    --asynloader->total_requests_count;
    mtx_unlock( &asynloader->request_mutex );
  }
  
  if ( asynloader->upload_requests_lpos != asynloader->upload_requests_rpos )
  {
    crude_gfx_upload_request                               request;
    crude_gfx_cmd_buffer                                  *cmd;
    
    cmd = &asynloader->cmd_buffers[ asynloader->gpu->current_frame ];
    
    request = crude_gfx_asynchronous_loader_pop_upload_requests_( asynloader );
    
    crude_gfx_cmd_begin_primary( cmd );

    if ( CRUDE_RESOURCE_HANDLE_IS_VALID( request.texture ) )
    {
      crude_gfx_texture                                   *texture;
      uint32                                               texture_channels;
      uint32                                               texture_alignment;
      uint64                                               aligned_image_size;
      size_t                                               current_offset;

      texture = crude_gfx_access_texture( asynloader->gpu, request.texture );
      texture_channels = 4;
      texture_alignment = 4;
      aligned_image_size = crude_memory_align( texture->width * texture->height * texture_channels, texture_alignment );
      
      crude_memory_copy( asynloader->staging_allocation.cpu_address, request.data, aligned_image_size );

      crude_gfx_cmd_memory_copy_to_texture( cmd, texture->handle, asynloader->staging_allocation );
     
      free( request.data );
    }
    else if ( crude_gfx_memory_allocation_valid( &request.cpu_allocation ) && crude_gfx_memory_allocation_valid( &request.gpu_allocation ) )
    {
      if ( crude_gfx_memory_allocation_valid( &request.gpu_old_allocation ) )
      {
        crude_gfx_cmd_memory_copy( cmd, request.gpu_old_allocation, request.gpu_allocation, 0, 0 );
        crude_gfx_cmd_memory_copy( cmd, request.cpu_allocation, request.gpu_allocation, 0, request.gpu_old_allocation.size );
      }
      else
      {
        crude_gfx_cmd_memory_copy( cmd, request.cpu_allocation, request.gpu_allocation, 0, 0 );
      }
    }  
    crude_gfx_cmd_end( cmd );

    {
      VkCommandBufferSubmitInfo command_buffers[] = {
        { VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO_KHR, NULL, cmd->vk_cmd_buffer, 0 },
      };
      
      VkSubmitInfo2 submit_info = CRUDE_COMPOUNT_EMPTY( VkSubmitInfo2 );
      submit_info.sType                    = VK_STRUCTURE_TYPE_SUBMIT_INFO_2_KHR;
      submit_info.commandBufferInfoCount   = CRUDE_COUNTOF( command_buffers );
      submit_info.pCommandBufferInfos      = command_buffers;
    
      VkQueue used_queue = asynloader->gpu->vk_transfer_queue;
      CRUDE_GFX_HANDLE_VULKAN_RESULT( asynloader->gpu->vkQueueSubmit2KHR( used_queue, 1, &submit_info, asynloader->vk_transfer_completed_fence ), "Failed to sumbit queue" );
    }
    
    if ( vkGetFenceStatus( asynloader->gpu->vk_device, asynloader->vk_transfer_completed_fence ) != VK_SUCCESS )
    {
      vkWaitForFences( asynloader->gpu->vk_device, 1u, &asynloader->vk_transfer_completed_fence, VK_TRUE, UINT64_MAX );
    }
    vkResetFences( asynloader->gpu->vk_device, 1u, &asynloader->vk_transfer_completed_fence );

    if ( CRUDE_RESOURCE_HANDLE_IS_VALID( request.texture ) )
    {
      CRUDE_ASSERT( CRUDE_RESOURCE_HANDLE_IS_INVALID( asynloader->texture_ready ) );
      asynloader->texture_ready = request.texture;
    }
    else if ( crude_gfx_memory_allocation_valid( &request.cpu_allocation ) && crude_gfx_memory_allocation_valid( &request.gpu_allocation ) )
    {
      CRUDE_ASSERT( !crude_gfx_memory_allocation_valid( &asynloader->cpu_allocation_ready ) );
      CRUDE_ASSERT( !crude_gfx_memory_allocation_valid( &asynloader->gpu_allocation_ready ) );
      asynloader->cpu_allocation_ready = request.cpu_allocation;
      asynloader->gpu_allocation_ready = request.gpu_allocation;
      
      if ( crude_gfx_memory_allocation_valid( &request.gpu_old_allocation ) )
      {
        CRUDE_ASSERT( !crude_gfx_memory_allocation_valid( &asynloader->gpu_old_allocation_ready ) );
        asynloader->gpu_old_allocation_ready = request.gpu_old_allocation;
      }
    }
  }

  if ( asynloader->file_load_requests_lpos != asynloader->file_load_requests_rpos )
  {
    crude_gfx_file_load_request                            load_request;
    uint8                                                 *texture_data;
    int64                                                  start_reading_file;
    int32                                                  x, y, comp;

    load_request = crude_gfx_asynchronous_loader_pop_file_load_request_( asynloader );
    
    start_reading_file = crude_time_now();
    texture_data = stbi_load( load_request.path, &x, &y, &comp, 4 );
    if ( texture_data )
    {
      CRUDE_LOG_INFO( CRUDE_CHANNEL_GRAPHICS, "File %s read in %fs", load_request.path, crude_time_delta_seconds( start_reading_file, crude_time_now() ) );
      
      mtx_lock( &asynloader->request_mutex );
      --asynloader->total_requests_count;
      mtx_unlock( &asynloader->request_mutex );

      crude_gfx_upload_request upload_request;
      upload_request.data = texture_data;
      upload_request.texture = load_request.texture;
      upload_request.cpu_allocation = crude_gfx_memory_allocation_empty( );
      crude_gfx_asynchronous_loader_push_upload_requests_( asynloader, upload_request );
    }
    else
    {
      CRUDE_ASSERTM( CRUDE_CHANNEL_GRAPHICS, "Error reading file %s", load_request.path );
    }
  }
}

bool
crude_gfx_asynchronous_loader_has_requests
(
  _In_ crude_gfx_asynchronous_loader                      *asynloader
)
{
  mtx_lock( &asynloader->request_mutex );
  bool has_requests = asynloader->total_requests_count;
  mtx_unlock( &asynloader->request_mutex );
  return has_requests;
;
}

void
crude_gfx_asynchronous_loader_push_upload_requests_
(
  _In_ crude_gfx_asynchronous_loader                      *asynloader,
  _In_ crude_gfx_upload_request                            upload_request
)
{
  mtx_lock( &asynloader->request_mutex );
  asynloader->upload_requests[ asynloader->upload_requests_rpos ] = upload_request;
  asynloader->upload_requests_rpos = ( asynloader->upload_requests_rpos + 1 ) % CRUDE_GRAPHICS_ASYNCHRONOUS_LOADER_UPLOAD_REQUESTS_LIMIT;

  ++asynloader->total_requests_count;

  CRUDE_ASSERTM( CRUDE_CHANNEL_GRAPHICS, asynloader->upload_requests_rpos != asynloader->upload_requests_lpos, "Limit of upload requests in asynchronous loader!" );
  mtx_unlock( &asynloader->request_mutex );
}

void
crude_gfx_asynchronous_loader_push_file_load_request_
(
  _In_ crude_gfx_asynchronous_loader                      *asynloader,
  _In_ crude_gfx_file_load_request                         file_load_request
)
{
  mtx_lock( &asynloader->request_mutex );
  asynloader->file_load_requests[ asynloader->file_load_requests_rpos ] = file_load_request;
  asynloader->file_load_requests_rpos = ( asynloader->file_load_requests_rpos + 1 ) % CRUDE_GRAPHICS_ASYNCHRONOUS_LOADER_FILE_LOAD_REQUESTS_LIMIT;
  
  ++asynloader->total_requests_count;

  CRUDE_ASSERTM( CRUDE_CHANNEL_GRAPHICS, asynloader->file_load_requests_rpos != asynloader->file_load_requests_lpos, "Limit of upload requests in asynchronous loader!" );
  mtx_unlock( &asynloader->request_mutex );
}

crude_gfx_upload_request
crude_gfx_asynchronous_loader_pop_upload_requests_
(
  _In_ crude_gfx_asynchronous_loader                      *asynloader
)
{
  mtx_lock( &asynloader->request_mutex );
  int32 front_position = asynloader->upload_requests_lpos;
  asynloader->upload_requests_lpos = ( asynloader->upload_requests_lpos + 1 ) % CRUDE_GRAPHICS_ASYNCHRONOUS_LOADER_UPLOAD_REQUESTS_LIMIT;
  crude_gfx_upload_request request = asynloader->upload_requests[ front_position ];
  mtx_unlock( &asynloader->request_mutex );
  return request;
}

crude_gfx_file_load_request
crude_gfx_asynchronous_loader_pop_file_load_request_
(
  _In_ crude_gfx_asynchronous_loader                      *asynloader
)
{
  mtx_lock( &asynloader->request_mutex );
  int32 front_position = asynloader->file_load_requests_lpos;
  asynloader->file_load_requests_lpos = ( asynloader->file_load_requests_lpos + 1 ) % CRUDE_GRAPHICS_ASYNCHRONOUS_LOADER_FILE_LOAD_REQUESTS_LIMIT;
  crude_gfx_file_load_request request = asynloader->file_load_requests[ front_position ];
  mtx_unlock( &asynloader->request_mutex ); 
  return request;
}