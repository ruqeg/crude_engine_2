#include <stb_image.h>
#include <TaskScheduler_c.h>

#include <core/profiler.h>
#include <core/array.h>
#include <core/time.h>

#include <graphics/asynchronous_loader.h>

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

  asynloader->staging_buffer_offset = 0u;

  asynloader->texture_ready = CRUDE_GFX_TEXTURE_HANDLE_INVALID;
  asynloader->cpu_buffer_ready = CRUDE_GFX_BUFFER_HANDLE_INVALID;
  asynloader->gpu_buffer_ready = CRUDE_GFX_BUFFER_HANDLE_INVALID;
  asynloader->gpu_old_buffer_ready = CRUDE_GFX_BUFFER_HANDLE_INVALID;

  asynloader->total_requests_count = 0;

  asynloader->file_load_requests_lpos = asynloader->file_load_requests_rpos = 0;
  asynloader->upload_requests_lpos = asynloader->upload_requests_rpos = 0;

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
  
  {
    crude_gfx_buffer_creation                              buffer_creation;
    crude_gfx_buffer_handle                                staging_buffer_handle;

    buffer_creation = crude_gfx_buffer_creation_empty();
    buffer_creation.type_flags = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
    buffer_creation.usage = CRUDE_GFX_RESOURCE_USAGE_TYPE_STREAM;
    buffer_creation.size = 64 * 1024 * 1024;
    buffer_creation.name = "asynchronous_loader_staging_buffer";
    buffer_creation.persistent = true;
    
    staging_buffer_handle = crude_gfx_create_buffer( gpu, &buffer_creation );
    asynloader->staging_buffer = crude_gfx_access_buffer( gpu, staging_buffer_handle );
  }

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
  vkDestroyFence( asynloader->gpu->vk_device, asynloader->vk_transfer_completed_fence, asynloader->gpu->vk_allocation_callbacks );
  
  crude_gfx_destroy_buffer( asynloader->gpu, asynloader->staging_buffer->handle );
  
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
  request.buffer = CRUDE_GFX_BUFFER_HANDLE_INVALID;

  crude_gfx_asynchronous_loader_push_file_load_request_( asynloader, request );

  crude_gfx_texture *texture = crude_gfx_access_texture( asynloader->gpu, texture_handle );
  texture->ready = false;

  ++asynloader->total_requests_count;
}

void
crude_gfx_asynchronous_loader_request_buffer_copy
(
  _In_ crude_gfx_asynchronous_loader                      *asynloader,
  _In_ crude_gfx_buffer_handle                             cpu_buffer,
  _In_ crude_gfx_buffer_handle                             gpu_buffer
)
{
  crude_gfx_buffer                                        *buffer;
  crude_gfx_upload_request                                 request;

  request = CRUDE_COMPOUNT_EMPTY( crude_gfx_upload_request );
  request.texture    = CRUDE_GFX_TEXTURE_HANDLE_INVALID;
  request.cpu_buffer = cpu_buffer;
  request.gpu_buffer = gpu_buffer;
  request.gpu_old_buffer = CRUDE_GFX_BUFFER_HANDLE_INVALID;
  
  crude_gfx_asynchronous_loader_push_upload_requests_( asynloader, request );

  buffer = crude_gfx_access_buffer( asynloader->gpu, gpu_buffer );
  buffer->ready = false;

  ++asynloader->total_requests_count;
}

CRUDE_API void
crude_gfx_asynchronous_loader_request_buffer_reallocate_and_copy
(
  _In_ crude_gfx_asynchronous_loader                      *asynloader,
  _In_ crude_gfx_buffer_handle                             cpu_buffer,
  _In_ crude_gfx_buffer_handle                             gpu_buffer,
  _In_ crude_gfx_buffer_handle                             gpu_old_buffer
)
{
  crude_gfx_buffer                                        *buffer;
  crude_gfx_upload_request                                 request;

  request = CRUDE_COMPOUNT_EMPTY( crude_gfx_upload_request );
  request.texture    = CRUDE_GFX_TEXTURE_HANDLE_INVALID;
  request.cpu_buffer = cpu_buffer;
  request.gpu_buffer = gpu_buffer;
  request.gpu_old_buffer = gpu_old_buffer;
  
  crude_gfx_asynchronous_loader_push_upload_requests_( asynloader, request );

  buffer = crude_gfx_access_buffer( asynloader->gpu, gpu_buffer );
  buffer->ready = false;

  ++asynloader->total_requests_count;
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

    --asynloader->total_requests_count;
  }
  
  if ( CRUDE_RESOURCE_HANDLE_IS_VALID( asynloader->cpu_buffer_ready ) && CRUDE_RESOURCE_HANDLE_IS_VALID( asynloader->gpu_buffer_ready ) )
  {
    if ( CRUDE_RESOURCE_HANDLE_IS_VALID( asynloader->gpu_old_buffer_ready ) )
    {
      crude_gfx_destroy_buffer( asynloader->gpu, asynloader->gpu_old_buffer_ready );
    }

    crude_gfx_destroy_buffer( asynloader->gpu, asynloader->cpu_buffer_ready );

    crude_gfx_buffer *buffer = crude_gfx_access_buffer( asynloader->gpu, asynloader->gpu_buffer_ready );
    buffer->ready = true;

    asynloader->cpu_buffer_ready = CRUDE_GFX_BUFFER_HANDLE_INVALID;
    asynloader->gpu_buffer_ready = CRUDE_GFX_BUFFER_HANDLE_INVALID;
    asynloader->gpu_old_buffer_ready = CRUDE_GFX_BUFFER_HANDLE_INVALID;

    --asynloader->total_requests_count;
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
      current_offset = asynloader->staging_buffer_offset;
      
      crude_gfx_cmd_upload_texture_data( cmd, texture->handle, request.data, asynloader->staging_buffer->handle, current_offset );
     
      free( request.data );
    }
    else if ( CRUDE_RESOURCE_HANDLE_IS_VALID( request.cpu_buffer ) && CRUDE_RESOURCE_HANDLE_IS_VALID( request.gpu_buffer ) )
    {
      if ( CRUDE_RESOURCE_HANDLE_IS_VALID( request.gpu_old_buffer ) )
      {
        crude_gfx_buffer *gpu_old_buffer = crude_gfx_access_buffer( cmd->gpu, request.gpu_old_buffer );

        crude_gfx_cmd_upload_buffer_data( cmd, request.gpu_old_buffer, request.gpu_buffer, 0 );
        crude_gfx_cmd_upload_buffer_data( cmd, request.cpu_buffer, request.gpu_buffer, gpu_old_buffer->size );
      }
      else
      {
        crude_gfx_cmd_upload_buffer_data( cmd, request.cpu_buffer, request.gpu_buffer, 0 );
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
    else if ( CRUDE_RESOURCE_HANDLE_IS_VALID( request.cpu_buffer ) && CRUDE_RESOURCE_HANDLE_IS_VALID( request.gpu_buffer ) )
    {
      CRUDE_ASSERT( CRUDE_RESOURCE_HANDLE_IS_INVALID( asynloader->cpu_buffer_ready ) );
      CRUDE_ASSERT( CRUDE_RESOURCE_HANDLE_IS_INVALID( asynloader->gpu_buffer_ready ) );
      asynloader->cpu_buffer_ready = request.cpu_buffer;
      asynloader->gpu_buffer_ready = request.gpu_buffer;
      
      if ( CRUDE_RESOURCE_HANDLE_IS_VALID( request.gpu_old_buffer ) )
      {
        CRUDE_ASSERT( CRUDE_RESOURCE_HANDLE_IS_INVALID( asynloader->gpu_old_buffer_ready ) );
        asynloader->gpu_old_buffer_ready = request.gpu_old_buffer;
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

      crude_gfx_upload_request upload_request;
      upload_request.data = texture_data;
      upload_request.texture = load_request.texture;
      upload_request.cpu_buffer = CRUDE_GFX_BUFFER_HANDLE_INVALID;
      crude_gfx_asynchronous_loader_push_upload_requests_( asynloader, upload_request );
    }
    else
    {
      CRUDE_LOG_ERROR( CRUDE_CHANNEL_GRAPHICS, "Error reading file %s", load_request.path );
    }
  }

  asynloader->staging_buffer_offset = 0;
}

bool
crude_gfx_asynchronous_loader_has_requests
(
  _In_ crude_gfx_asynchronous_loader const                *asynloader
)
{
  return asynloader->total_requests_count;
;
}

/************************************************
 *
 * Asynchronous Loader Manager Utils Functions Implementatin
 * 
 ***********************************************/
static void
pinned_task_asynchronous_loader_loop_
(
  _In_ void                                               *args
)
{
  crude_gfx_asynchronous_loader_manager *ctx = CRUDE_REINTERPRET_CAST( crude_gfx_asynchronous_loader_manager*, args );

  CRUDE_PROFILER_SET_THREAD_NAME( "AsynchronousLoaderThread" );

  while ( ctx->async_loaders_valid )
  {
    mtx_lock( &ctx->task_mutex );
    for ( uint32 i = 0; i < CRUDE_ARRAY_LENGTH( ctx->async_loaders ); ++i )
    {
      crude_gfx_asynchronous_loader_update( ctx->async_loaders[ i ] );
    }
    mtx_unlock( &ctx->task_mutex );
  }
}

/************************************************
 *
 * Asynchronous Loader Manager Functions Implementatin
 * 
 ***********************************************/
void
crude_gfx_asynchronous_loader_manager_intiailize
(
  _In_ crude_gfx_asynchronous_loader_manager                *manager,
  _In_ crude_gfx_asynchronous_loader_manager_creation const *creation
)
{
  struct enkiTaskSchedulerConfig config = enkiGetTaskSchedulerConfig( CRUDE_REINTERPRET_CAST( enkiTaskScheduler*, creation->task_sheduler ) );
  manager->task_sheduler = creation->task_sheduler;
  manager->async_loaders_valid = true;
  manager->async_loader_task = enkiCreatePinnedTask( CRUDE_REINTERPRET_CAST( enkiTaskScheduler*, creation->task_sheduler ), pinned_task_asynchronous_loader_loop_, config.numTaskThreadsToCreate );
  enkiAddPinnedTaskArgs( CRUDE_REINTERPRET_CAST( enkiTaskScheduler*, creation->task_sheduler ), CRUDE_REINTERPRET_CAST( enkiPinnedTask*, manager->async_loader_task ), manager );
  mtx_init( &manager->task_mutex, mtx_plain );

  CRUDE_ARRAY_INITIALIZE_WITH_CAPACITY( manager->async_loaders, config.numTaskThreadsToCreate, creation->allocator_container );
}

void
crude_gfx_asynchronous_loader_manager_add_loader
(
  _In_ crude_gfx_asynchronous_loader_manager              *manager,
  _In_ crude_gfx_asynchronous_loader                      *async_loader
)
{
  CRUDE_ARRAY_PUSH( manager->async_loaders, async_loader );
}

void
crude_gfx_asynchronous_loader_manager_remove_loader
(
  _In_ crude_gfx_asynchronous_loader_manager              *manager,
  _In_ crude_gfx_asynchronous_loader                      *async_loader
)
{
  for ( uint32 i = 0; i < CRUDE_ARRAY_LENGTH( manager->async_loaders ); ++i )
  {
    if ( manager->async_loaders[ i ] == async_loader )
    {
      CRUDE_ARRAY_DELSWAP( manager->async_loaders, i );
    }
  }
}

void
crude_gfx_asynchronous_loader_manager_deintiailize
(
  _In_ crude_gfx_asynchronous_loader_manager              *manager
)
{
  manager->async_loaders_valid = false;
  CRUDE_ARRAY_DEINITIALIZE( manager->async_loaders );
  mtx_destroy( &manager->task_mutex );
}

void
crude_gfx_asynchronous_loader_push_upload_requests_
(
  _In_ crude_gfx_asynchronous_loader                      *asynloader,
  _In_ crude_gfx_upload_request                            upload_request
)
{
  asynloader->upload_requests[ asynloader->upload_requests_rpos ] = upload_request;
  asynloader->upload_requests_rpos = ( asynloader->upload_requests_rpos + 1 ) % CRUDE_GRAPHICS_ASYNCHRONOUS_LOADER_UPLOAD_REQUESTS_LIMIT;

  CRUDE_ASSERTM( CRUDE_CHANNEL_GRAPHICS, asynloader->upload_requests_rpos != asynloader->upload_requests_lpos, "Limit of upload requests in asynchronous loader!" );
}

void
crude_gfx_asynchronous_loader_push_file_load_request_
(
  _In_ crude_gfx_asynchronous_loader                      *asynloader,
  _In_ crude_gfx_file_load_request                         file_load_request
)
{
  asynloader->file_load_requests[ asynloader->file_load_requests_rpos ] = file_load_request;
  asynloader->file_load_requests_rpos = ( asynloader->file_load_requests_rpos + 1 ) % CRUDE_GRAPHICS_ASYNCHRONOUS_LOADER_FILE_LOAD_REQUESTS_LIMIT;
  
  CRUDE_ASSERTM( CRUDE_CHANNEL_GRAPHICS, asynloader->file_load_requests_rpos != asynloader->file_load_requests_lpos, "Limit of upload requests in asynchronous loader!" );
}

crude_gfx_upload_request
crude_gfx_asynchronous_loader_pop_upload_requests_
(
  _In_ crude_gfx_asynchronous_loader                      *asynloader
)
{
  int32 front_position = asynloader->upload_requests_lpos;
  asynloader->upload_requests_lpos = ( asynloader->upload_requests_lpos + 1 ) % CRUDE_GRAPHICS_ASYNCHRONOUS_LOADER_UPLOAD_REQUESTS_LIMIT;
  return asynloader->upload_requests[ front_position ];
}

crude_gfx_file_load_request
crude_gfx_asynchronous_loader_pop_file_load_request_
(
  _In_ crude_gfx_asynchronous_loader                      *asynloader
)
{
  int32 front_position = asynloader->file_load_requests_lpos;
  asynloader->file_load_requests_lpos = ( asynloader->file_load_requests_lpos + 1 ) % CRUDE_GRAPHICS_ASYNCHRONOUS_LOADER_FILE_LOAD_REQUESTS_LIMIT;
  return asynloader->file_load_requests[ front_position ];
}