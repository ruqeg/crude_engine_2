#include <stb_image.h>
#include <TaskScheduler_c.h>

#include <core/profiler.h>
#include <core/array.h>
#include <core/time.h>

#include <graphics/asynchronous_loader.h>

/************************************************
 *
 * Asynchronous Functions Implementation
 * 
 ***********************************************/
void
crude_gfx_asynchronous_loader_initialize
(
  _In_ crude_gfx_asynchronous_loader                      *asynloader,
  _In_ crude_gfx_renderer                                 *renderer
)
{
  asynloader->renderer = renderer;

  asynloader->staging_buffer_offset = 0u;

  asynloader->texture_ready = CRUDE_GFX_TEXTURE_HANDLE_INVALID;
  asynloader->cpu_buffer_ready = CRUDE_GFX_BUFFER_HANDLE_INVALID;
  asynloader->gpu_buffer_ready = CRUDE_GFX_BUFFER_HANDLE_INVALID;

  CRUDE_ARRAY_INITIALIZE_WITH_CAPACITY( asynloader->file_load_requests, 16, renderer->allocator_container );
  CRUDE_ARRAY_INITIALIZE_WITH_CAPACITY( asynloader->upload_requests, 16, renderer->allocator_container );

  for ( uint32 i = 0; i < CRUDE_GFX_MAX_SWAPCHAIN_IMAGES; ++i )
  {
    VkCommandPoolCreateInfo                                cmd_pool_info;
    VkCommandBufferAllocateInfo                            cmd;

    cmd_pool_info = CRUDE_COMPOUNT( VkCommandPoolCreateInfo, {
      .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
      .flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
      .queueFamilyIndex = renderer->gpu->vk_transfer_queue_family,
    } );
    vkCreateCommandPool( renderer->gpu->vk_device, &cmd_pool_info, renderer->gpu->vk_allocation_callbacks, &asynloader->vk_cmd_pools[ i ] );
    
    cmd = CRUDE_COMPOUNT( VkCommandBufferAllocateInfo, {
      .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
      .commandPool = asynloader->vk_cmd_pools[ i ],
      .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
      .commandBufferCount = 1,
    } );
    vkAllocateCommandBuffers( renderer->gpu->vk_device, &cmd, &asynloader->cmd_buffers[ i ].vk_cmd_buffer );

    crude_gfx_set_resource_name( renderer->gpu, VK_OBJECT_TYPE_COMMAND_BUFFER, ( uint64 )asynloader->cmd_buffers[ i ].vk_cmd_buffer, "asynchronous_loader_cmd" );
    
    asynloader->cmd_buffers[ i ].is_recording = false;
    asynloader->cmd_buffers[ i ].gpu = renderer->gpu;
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
    
    staging_buffer_handle = crude_gfx_create_buffer( renderer->gpu, &buffer_creation );
    asynloader->staging_buffer = crude_gfx_access_buffer( renderer->gpu, staging_buffer_handle );
  }

  {
    VkFenceCreateInfo fence_info = CRUDE_COMPOUNT_EMPTY( VkFenceCreateInfo );
    fence_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    CRUDE_GFX_HANDLE_VULKAN_RESULT( vkCreateFence( renderer->gpu->vk_device, &fence_info, renderer->gpu->vk_allocation_callbacks, &asynloader->vk_transfer_completed_fence ), "Failed to create fence" );
    crude_gfx_set_resource_name( renderer->gpu, VK_OBJECT_TYPE_FENCE, CRUDE_CAST( uint64, asynloader->vk_transfer_completed_fence ), "transfer_completed_fence" );
  }
}

void
crude_gfx_asynchronous_loader_deinitialize
(
  _In_ crude_gfx_asynchronous_loader                      *asynloader
)
{
  vkDestroyFence( asynloader->renderer->gpu->vk_device, asynloader->vk_transfer_completed_fence, asynloader->renderer->gpu->vk_allocation_callbacks );
  
  crude_gfx_destroy_buffer( asynloader->renderer->gpu, asynloader->staging_buffer->handle );
  
  for ( uint32 i = 0; i < CRUDE_GFX_MAX_SWAPCHAIN_IMAGES; ++i )
  {
    vkDestroyCommandPool( asynloader->renderer->gpu->vk_device, asynloader->vk_cmd_pools[ i ], asynloader->renderer->gpu->vk_allocation_callbacks );  
  }

  CRUDE_ARRAY_DEINITIALIZE( asynloader->file_load_requests );
  CRUDE_ARRAY_DEINITIALIZE( asynloader->upload_requests );
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
  CRUDE_ARRAY_PUSH( asynloader->file_load_requests, request );
  
  crude_gfx_texture *texture = crude_gfx_access_texture( asynloader->renderer->gpu, texture_handle );
  texture->ready = false;
}

void
crude_gfx_asynchronous_loader_request_buffer_copy
(
  _In_ crude_gfx_asynchronous_loader                      *asynloader,
  _In_ crude_gfx_buffer_handle                             cpu_buffer,
  _In_ crude_gfx_buffer_handle                             gpu_buffer
)
{
  crude_gfx_upload_request request = {
    .texture    = CRUDE_GFX_TEXTURE_HANDLE_INVALID,
    .cpu_buffer = cpu_buffer,
    .gpu_buffer = gpu_buffer,
  };
  CRUDE_ARRAY_PUSH( asynloader->upload_requests, request );

  crude_gfx_buffer *buffer = crude_gfx_access_buffer( asynloader->renderer->gpu, gpu_buffer );
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
    crude_gfx_renderer_add_texture_to_update( asynloader->renderer, asynloader->texture_ready );

    asynloader->texture_ready = CRUDE_GFX_TEXTURE_HANDLE_INVALID;
  }
  
  if ( CRUDE_RESOURCE_HANDLE_IS_VALID( asynloader->cpu_buffer_ready ) && CRUDE_RESOURCE_HANDLE_IS_VALID( asynloader->gpu_buffer_ready ) )
  {
    crude_gfx_destroy_buffer( asynloader->renderer->gpu, asynloader->cpu_buffer_ready );

    crude_gfx_buffer *buffer = crude_gfx_access_buffer( asynloader->renderer->gpu, asynloader->gpu_buffer_ready );
    buffer->ready = true;

    asynloader->cpu_buffer_ready = CRUDE_GFX_BUFFER_HANDLE_INVALID;
    asynloader->gpu_buffer_ready = CRUDE_GFX_BUFFER_HANDLE_INVALID;
  }
  
  if ( CRUDE_ARRAY_LENGTH( asynloader->upload_requests ) )
  {
    crude_gfx_upload_request                               request;
    crude_gfx_cmd_buffer                                  *cmd;

    request = CRUDE_ARRAY_POP( asynloader->upload_requests );

    cmd = &asynloader->cmd_buffers[ asynloader->renderer->gpu->current_frame ];
    crude_gfx_cmd_begin_primary( cmd );

    if ( CRUDE_RESOURCE_HANDLE_IS_VALID( request.texture ) )
    {
      crude_gfx_texture                                   *texture;
      uint32                                               texture_channels;
      uint32                                               texture_alignment;
      uint64                                               aligned_image_size;
      size_t                                               current_offset;

      texture = crude_gfx_access_texture( asynloader->renderer->gpu, request.texture );
      texture_channels = 4;
      texture_alignment = 4;
      aligned_image_size = crude_memory_align( texture->width * texture->height * texture_channels, texture_alignment );
      current_offset = asynloader->staging_buffer_offset;
      
      crude_gfx_cmd_upload_texture_data( cmd, texture->handle, request.data, asynloader->staging_buffer->handle, current_offset );
     
      free( request.data );
    }
    else if ( CRUDE_RESOURCE_HANDLE_IS_VALID( request.cpu_buffer ) && CRUDE_RESOURCE_HANDLE_IS_VALID( request.gpu_buffer ) )
    {
      crude_gfx_cmd_upload_buffer_data( cmd, request.cpu_buffer, request.gpu_buffer );
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
    
      VkQueue used_queue = asynloader->renderer->gpu->vk_transfer_queue;
      CRUDE_GFX_HANDLE_VULKAN_RESULT( asynloader->renderer->gpu->vkQueueSubmit2KHR( used_queue, 1, &submit_info, asynloader->vk_transfer_completed_fence ), "Failed to sumbit queue" );
    }
    
    
    if ( vkGetFenceStatus( asynloader->renderer->gpu->vk_device, asynloader->vk_transfer_completed_fence ) != VK_SUCCESS )
    {
      vkWaitForFences( asynloader->renderer->gpu->vk_device, 1u, &asynloader->vk_transfer_completed_fence, VK_TRUE, UINT64_MAX );
    }
    vkResetFences( asynloader->renderer->gpu->vk_device, 1u, &asynloader->vk_transfer_completed_fence );

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
    }
  }

  if ( CRUDE_ARRAY_LENGTH( asynloader->file_load_requests ) )
  {
    crude_gfx_file_load_request                            load_request;
    uint8                                                 *texture_data;
    int64                                                  start_reading_file;
    int32                                                  x, y, comp;

    load_request = CRUDE_ARRAY_POP( asynloader->file_load_requests );
    
    start_reading_file = crude_time_now();
    texture_data = stbi_load( load_request.path, &x, &y, &comp, 4 );
    if ( texture_data )
    {
      CRUDE_LOG_INFO( CRUDE_CHANNEL_GRAPHICS, "File %s read in %fs", load_request.path, crude_time_delta_seconds( start_reading_file, crude_time_now() ) );

      crude_gfx_upload_request upload_request;
      upload_request.data = texture_data;
      upload_request.texture = load_request.texture;
      upload_request.cpu_buffer = CRUDE_GFX_BUFFER_HANDLE_INVALID;
      CRUDE_ARRAY_PUSH( asynloader->upload_requests, upload_request );
    }
    else
    {
      CRUDE_LOG_ERROR( CRUDE_CHANNEL_GRAPHICS, "Error reading file %s", load_request.path );
    }
  }

  asynloader->staging_buffer_offset = 0;
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