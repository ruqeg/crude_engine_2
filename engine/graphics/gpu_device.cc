#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>

#include <engine/core/profiler.h>
#include <engine/core/string.h>
#include <engine/core/file.h>
#include <engine/core/process.h>
#include <engine/core/hashmapstr.h>
#include <engine/graphics/gpu_profiler.h>

#include <engine/graphics/gpu_device.h>

static void
crude_gfx_create_swapchain_internal_
( 
  _In_ crude_gfx_device                                   *gpu
);

static void
crude_gfx_create_descriptor_pool_internal_
(
  _In_ crude_gfx_device                                   *gpu
);

static void
crude_gfx_create_texture_internal_
(
  _In_ crude_gfx_device                                   *gpu,
  _In_ crude_gfx_texture_creation const                   *creation,
  _In_ crude_gfx_texture_handle                            handle,
  _In_ crude_gfx_texture                                  *texture
);

static void
crude_gfx_create_texture_view_internal_
(
  _In_ crude_gfx_device                                   *gpu,
  _In_ crude_gfx_texture_view_creation const              *creation,
  _In_ crude_gfx_texture                                  *texture
);

static void
crude_gfx_resize_swapchain_internal_
(
  _In_ crude_gfx_device                                   *gpu
);

static crude_gfx_vertex_component_format
reflect_format_to_vk_format_
(
  _In_ SpvReflectFormat                                    format
);

static void
vk_reflect_shader_
(
  _In_ crude_gfx_device                                   *gpu,
  _In_ void const                                         *code,
  _In_ uint32                                              code_size,
  _In_ crude_gfx_shader_reflect                           *reflect
);

static void
crude_gfx_destroy_resources_instant_
(
  _In_ crude_gfx_device                                   *gpu,
  _In_ crude_gfx_resource_deletion_type                    type,
  _In_ crude_gfx_resource_index                            handle
);

static void
crude_gfx_update_frame_counters_
(
  _In_ crude_gfx_device                                   *gpu
);

static void
crude_gfx_push_resource_update_
(
  _In_ crude_gfx_device                                   *gpu,
  _In_ crude_gfx_resource_update const                    *update
);

/************************************************
 *
 * GPU Device Initialize/Deinitialize
 * 
 ***********************************************/
void
crude_gfx_device_initialize
(
  _Out_ crude_gfx_device                                  *gpu,
  _In_ crude_gfx_device_creation                          *creation
)
{
  CRUDE_LOG_INFO( CRUDE_CHANNEL_GRAPHICS, "Initialize Device" );

  gpu->sdl_window = creation->sdl_window;
  gpu->allocator = creation->allocator;
  gpu->previous_frame = 0;
  gpu->current_frame = 1;
  gpu->absolute_frame = 0;
  gpu->swapchain_image_index = 0;
  gpu->swapchain_resized_last_frame = false;
  gpu->timestamps_enabled = false;

  gpu->environment = creation->environment;

  crude_gfx_rhi_create_instance( &gpu->rhi_instance );
  crude_gfx_rhi_create_surface( gpu->rhi_instance, gpu->sdl_window, &gpu->rhi_surface );
  crude_gfx_rhi_create_device( gpu->rhi_instance, gpu->rhi_surface, gpu->allocator, &gpu->rhi_device );

  gpu->rhi_main_queue = crude_gfx_rhi_device_get_graphics_queue( &gpu->rhi_device );
  gpu->rhi_transfer_queue = crude_gfx_rhi_device_get_transfer_queue( &gpu->rhi_device );

#if CRUDE_GFX_NSIGHT_AFTERMATH
  crude_gfx_gpu_crash_tracker_initialize( &gpu->crash_tracker, gpu->allocator );
#endif

  crude_resource_pool_initialize( &gpu->buffers, crude_heap_allocator_pack( gpu->allocator ), 4096, sizeof( crude_gfx_buffer ) );
  crude_resource_pool_initialize( &gpu->textures, crude_heap_allocator_pack( gpu->allocator ), 512, sizeof( crude_gfx_texture ) );
  crude_resource_pool_initialize( &gpu->render_passes, crude_heap_allocator_pack( gpu->allocator ), 256, sizeof( crude_gfx_render_pass ) );
  crude_resource_pool_initialize( &gpu->descriptor_set_layouts, crude_heap_allocator_pack( gpu->allocator ), 256, sizeof( crude_gfx_descriptor_set_layout ) );
  crude_resource_pool_initialize( &gpu->descriptor_sets, crude_heap_allocator_pack( gpu->allocator ), 256, sizeof( crude_gfx_descriptor_set ) );
  crude_resource_pool_initialize( &gpu->pipelines, crude_heap_allocator_pack( gpu->allocator ), 128, sizeof( crude_gfx_pipeline ) );
  crude_resource_pool_initialize( &gpu->shaders, crude_heap_allocator_pack( gpu->allocator ), 128, sizeof( crude_gfx_shader_state ) );
  crude_resource_pool_initialize( &gpu->samplers, crude_heap_allocator_pack( gpu->allocator ), 32, sizeof( crude_gfx_sampler ) );
  crude_resource_pool_initialize( &gpu->framebuffers, crude_heap_allocator_pack( gpu->allocator ), 128, sizeof( crude_gfx_framebuffer ) );
  crude_resource_pool_initialize( &gpu->techniques, crude_heap_allocator_pack( gpu->allocator ), 256, sizeof( crude_gfx_technique ) );
  crude_resource_pool_initialize( &gpu->cmd_pools, crude_heap_allocator_pack( gpu->allocator ), 16, sizeof( crude_gfx_cmd_pool ) );
  crude_resource_pool_initialize( &gpu->cmd_buffers, crude_heap_allocator_pack( gpu->allocator ), 16, sizeof( crude_gfx_cmd_buffer ) );
  
  {
    gpu->gpu_timestamp_frequency = crude_gfx_rhi_get_timestamp_period( &gpu->rhi_device ) / ( 1000 * 1000 );
    gpu->gpu_time_queries_per_frame = 32;
    gpu->num_threads = 1u;

    CRUDE_ARRAY_INITIALIZE_WITH_LENGTH( gpu->thread_frame_pools, CRUDE_GFX_SWAPCHAIN_IMAGES_MAX_COUNT * gpu->num_threads, crude_heap_allocator_pack( gpu->allocator ) );
    
#if CRUDE_GFX_GPU_PROFILER
    gpu->gpu_time_queries_manager = CRUDE_CAST( crude_gfx_gpu_time_queries_manager*, crude_heap_allocator_allocate( gpu->allocator, sizeof( crude_gfx_gpu_time_queries_manager ) ) );
    crude_gfx_gpu_time_queries_manager_initialize( gpu->gpu_time_queries_manager, gpu->thread_frame_pools, crude_heap_allocator_pack( gpu->allocator ), gpu->gpu_time_queries_per_frame, gpu->num_threads, CRUDE_GFX_SWAPCHAIN_IMAGES_MAX_COUNT );
#endif
    for ( uint32 i = 0; i < CRUDE_ARRAY_LENGTH( gpu->thread_frame_pools ); ++i )
    {
      crude_gfx_cmd_pool_creation                          cmd_pool_creation;
      
      cmd_pool_creation = CRUDE_COMPOUNT_EMPTY( crude_gfx_cmd_pool_creation );
      cmd_pool_creation.queue = gpu->rhi_main_queue;
#if CRUDE_GFX_GPU_PROFILER
      cmd_pool_creation.profiler.enabled = true;
      cmd_pool_creation.profiler.time_queries_trees = &gpu->gpu_time_queries_manager->query_trees[ i ];;
      cmd_pool_creation.profiler.time_queries_per_frame = gpu->gpu_time_queries_per_frame;
#endif /* CRUDE_GFX_GPU_PROFILER */
      gpu->thread_frame_pools[ i ] = crude_gfx_create_cmd_pool( gpu, &cmd_pool_creation );
    }
    
    {
      crude_gfx_cmd_buffer_creation                        cmd_buffer_creation;
      crude_gfx_cmd_pool_creation                          cmd_pool_creation;
      
      cmd_pool_creation = CRUDE_COMPOUNT_EMPTY( crude_gfx_cmd_pool_creation );
      cmd_pool_creation.queue = gpu->rhi_main_queue;
#if CRUDE_GFX_GPU_PROFILER
      cmd_pool_creation.profiler.enabled = false;
#endif /* CRUDE_GFX_GPU_PROFILER */
      gpu->immediate_transfer_cmd_pool = crude_gfx_create_cmd_pool( gpu, &cmd_pool_creation );

      cmd_buffer_creation = CRUDE_COMPOUNT_EMPTY( crude_gfx_cmd_buffer_creation );
      cmd_buffer_creation.cmd_pool = gpu->immediate_transfer_cmd_pool;
      crude_string_copy( cmd_buffer_creation.name, "immediate_transfer_cmd_buffer", sizeof( cmd_buffer_creation.name ) );
      gpu->immediate_transfer_cmd_buffer = crude_gfx_create_cmd_buffer( gpu, &cmd_buffer_creation );
    }
  }

  for ( uint32 i = 0; i < CRUDE_GFX_SWAPCHAIN_IMAGES_MAX_COUNT; ++i )
  {
    crude_gfx_rhi_create_semaphore( &gpu->rhi_device, false, &gpu->rhi_image_avalivable_semaphores[ i ] );
    crude_gfx_rhi_create_semaphore( &gpu->rhi_device, false, &gpu->rhi_swapchain_updated_semaphore[ i ] );
    crude_gfx_rhi_create_semaphore( &gpu->rhi_device, false, &gpu->rhi_rendering_finished_semaphore[ i ] );
    crude_gfx_rhi_set_semaphore_debug_name( &gpu->rhi_device, gpu->rhi_image_avalivable_semaphores[ i ], "image_avalivable_semaphores" );
    crude_gfx_rhi_set_semaphore_debug_name( &gpu->rhi_device, gpu->rhi_swapchain_updated_semaphore[ i ], "swapchain_updated_semaphore" );
    crude_gfx_rhi_set_semaphore_debug_name( &gpu->rhi_device, gpu->rhi_rendering_finished_semaphore[ i ], "rendering_finished_semaphore" );
  }
  
  crude_gfx_rhi_create_fence( &gpu->rhi_device, true, &gpu->rhi_immediate_fence );
  crude_gfx_rhi_set_fence_debug_name( &gpu->rhi_device, gpu->rhi_immediate_fence, "immediate_fence" );
  
  crude_gfx_rhi_create_semaphore( &gpu->rhi_device, true, &gpu->rhi_graphics_semaphore );
  crude_gfx_rhi_set_semaphore_debug_name( &gpu->rhi_device, gpu->rhi_graphics_semaphore, "graphics_semaphore" );
  
  crude_gfx_cmd_manager_initialize( &gpu->cmd_buffer_manager, gpu, gpu->num_threads, 1u );
  CRUDE_ARRAY_INITIALIZE_WITH_CAPACITY( gpu->queued_command_buffers, 128, crude_heap_allocator_pack( gpu->allocator ) );
  CRUDE_ARRAY_INITIALIZE_WITH_CAPACITY( gpu->resource_deletion_queue, 16, crude_heap_allocator_pack( gpu->allocator ) );
  CRUDE_ARRAY_INITIALIZE_WITH_CAPACITY( gpu->texture_to_update_bindless, 16, crude_heap_allocator_pack( gpu->allocator ) );
  
  crude_gfx_create_swapchain_internal_( gpu );
  crude_gfx_create_descriptor_pool_internal_( gpu );
  
  {
    crude_gfx_sampler_creation default_sampler_creation = crude_gfx_sampler_creation_empty();
    default_sampler_creation.address_mode_u = CRUDE_GFX_RHI_SAMPLER_ADDRESS_MODE_REPEAT;
    default_sampler_creation.address_mode_v = CRUDE_GFX_RHI_SAMPLER_ADDRESS_MODE_REPEAT;
    default_sampler_creation.address_mode_w = CRUDE_GFX_RHI_SAMPLER_ADDRESS_MODE_REPEAT;
    default_sampler_creation.min_filter     = CRUDE_GFX_RHI_FILTER_LINEAR;
    default_sampler_creation.mag_filter     = CRUDE_GFX_RHI_FILTER_LINEAR;
    default_sampler_creation.mip_filter     = CRUDE_GFX_RHI_SAMPLER_MIPMAP_MODE_LINEAR;
    default_sampler_creation.name           = "sampler default";
    gpu->default_sampler = crude_gfx_create_sampler( gpu, &default_sampler_creation );
  }
  
  crude_gfx_linear_allocator_initialize( &gpu->frame_linear_allocator, gpu, CRUDE_RMEGA( 64 ), "frame_linear_allocator" );

  crude_gfx_rhi_get_device_ray_tracing_pipeline_properties( &gpu->rhi_device, &gpu->ray_tracing_pipeline_properties );

  gpu->swapchain_output.depth_stencil_format = CRUDE_GFX_RHI_FORMAT_D32_SFLOAT;
  gpu->swapchain_output.depth_stencil_final_layout = CRUDE_GFX_RHI_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

  gpu->swapchain_output.depth_operation = CRUDE_GFX_RENDER_PASS_OPERATION_CLEAR;
  gpu->swapchain_output.stencil_operation = CRUDE_GFX_RENDER_PASS_OPERATION_CLEAR;

  gpu->swapchain_output.color_final_layouts[ 0 ] = CRUDE_GFX_RHI_IMAGE_LAYOUT_PRESENT_SRC_KHR;
  gpu->swapchain_output.color_formats[ 0 ] = gpu->surface_format.format;
  gpu->swapchain_output.color_operations[ 0 ] = CRUDE_GFX_RENDER_PASS_OPERATION_CLEAR;
  gpu->swapchain_output.num_color_formats = 1u;
  
  CRUDE_HASHMAPSTR_INITIALIZE( gpu->resource_cache.techniques, crude_heap_allocator_pack( gpu->allocator ) );
  
  gpu->num_textures_to_update = 0;
  mtx_init( &gpu->texture_update_mutex, mtx_plain );
  mtx_init( &gpu->resource_deletion_queue_mutex, mtx_plain );

  gpu->mesh_shaders_extension_present = crude_gfx_rhi_get_device_optional_extensions( &gpu->rhi_device )->mesh_shaders_extension_present;
}

void
crude_gfx_device_deinitialize
(
  _In_ crude_gfx_device                                   *gpu
)
{
  crude_gfx_rhi_wait_idle( &gpu->rhi_device );
  
  crude_gfx_linear_allocator_clear( &gpu->frame_linear_allocator );
  crude_gfx_linear_allocator_deinitialize( &gpu->frame_linear_allocator );
  crude_gfx_destroy_sampler( gpu, gpu->default_sampler );
  crude_gfx_destroy_descriptor_set_layout( gpu, gpu->bindless_descriptor_set_layout_handle );
  crude_gfx_destroy_descriptor_set( gpu, gpu->bindless_descriptor_set_handle );
  
  for ( uint32 i = 0; i < CRUDE_HASHMAPSTR_CAPACITY( gpu->resource_cache.techniques ); ++i )
  {
    if ( crude_hashmapstr_backet_key_hash_valid( gpu->resource_cache.techniques[ i ].key.key_hash ) )
    {
      crude_gfx_destroy_technique_instant( gpu, gpu->resource_cache.techniques[ i ].value );
    }
  }
  CRUDE_HASHMAPSTR_DEINITIALIZE( gpu->resource_cache.techniques );
  
  mtx_destroy( &gpu->texture_update_mutex );
  mtx_destroy( &gpu->resource_deletion_queue_mutex );

  for ( uint32 i = 0; i < CRUDE_ARRAY_LENGTH( gpu->resource_deletion_queue ); ++i )
  {
    crude_gfx_resource_update* resource_deletion = &gpu->resource_deletion_queue[ i ];
  
    if ( resource_deletion->current_frame == -1 )
    {
      continue;
    }

    crude_gfx_destroy_resources_instant_( gpu, resource_deletion->type, resource_deletion->handle );
  }
  
  crude_gfx_rhi_destroy_descriptor_pool( &gpu->rhi_device, &gpu->rhi_bindless_descriptor_pool );
  crude_gfx_rhi_destroy_descriptor_pool( &gpu->rhi_device, &gpu->rhi_descriptor_pool );

  crude_gfx_rhi_destroy_swapchain( &gpu->rhi_device, gpu->rhi_swapchain );

  CRUDE_ARRAY_DEINITIALIZE( gpu->queued_command_buffers );
  CRUDE_ARRAY_DEINITIALIZE( gpu->resource_deletion_queue );
  CRUDE_ARRAY_DEINITIALIZE( gpu->texture_to_update_bindless );
  
  crude_gfx_cmd_manager_deinitialize( &gpu->cmd_buffer_manager );
  
  crude_gfx_rhi_destroy_fence( &gpu->rhi_device, gpu->rhi_immediate_fence );
  crude_gfx_rhi_destroy_semaphore( &gpu->rhi_device, gpu->rhi_graphics_semaphore );
  for ( uint32 i = 0; i < CRUDE_GFX_SWAPCHAIN_IMAGES_MAX_COUNT; ++i )
  {
    crude_gfx_rhi_destroy_semaphore( &gpu->rhi_device, gpu->rhi_image_avalivable_semaphores[ i ] );
    crude_gfx_rhi_destroy_semaphore( &gpu->rhi_device, gpu->rhi_swapchain_updated_semaphore[ i ] );
    crude_gfx_rhi_destroy_semaphore( &gpu->rhi_device, gpu->rhi_rendering_finished_semaphore[ i ] );
  }
  
  for ( uint32 i = 0; i < CRUDE_ARRAY_LENGTH( gpu->thread_frame_pools ); ++i )
  {
    crude_gfx_destroy_cmd_pool_instant( gpu, gpu->thread_frame_pools[ i ] );
  }
  
  crude_gfx_destroy_cmd_buffer_instant( gpu, gpu->immediate_transfer_cmd_buffer );
  crude_gfx_destroy_cmd_pool_instant( gpu, gpu->immediate_transfer_cmd_pool );
  

  if ( gpu->buffers.free_indices_head != 0 )
  {
    CRUDE_LOG_ERROR( CRUDE_CHANNEL_CORE, "BUFFER Resource pool has unfreed resources" );
  
    for ( uint32 i = 0; i < gpu->buffers.pool_size; ++i )
    {
      crude_gfx_buffer *buffer = CRUDE_CAST( crude_gfx_buffer*, crude_resource_pool_access_resource( &gpu->buffers, i ) );
      if ( buffer->name[ 0 ] )
      {
        CRUDE_LOG_ERROR( CRUDE_CHANNEL_CORE, "\tBUFFER MAY NOT BE FREE %s", buffer->name );
      }
    }
  }

#if CRUDE_GFX_GPU_PROFILER
  crude_gfx_gpu_time_queries_manager_deinitialize( gpu->gpu_time_queries_manager );
  crude_heap_allocator_deallocate( gpu->allocator, gpu->gpu_time_queries_manager );
#endif
  
  CRUDE_ARRAY_DEINITIALIZE( gpu->thread_frame_pools );

  crude_resource_pool_deinitialize( &gpu->buffers );
  crude_resource_pool_deinitialize( &gpu->textures );
  crude_resource_pool_deinitialize( &gpu->render_passes );
  crude_resource_pool_deinitialize( &gpu->descriptor_sets );
  crude_resource_pool_deinitialize( &gpu->descriptor_set_layouts );
  crude_resource_pool_deinitialize( &gpu->pipelines );
  crude_resource_pool_deinitialize( &gpu->shaders );
  crude_resource_pool_deinitialize( &gpu->samplers );
  crude_resource_pool_deinitialize( &gpu->framebuffers );
  crude_resource_pool_deinitialize( &gpu->techniques );
  crude_resource_pool_deinitialize( &gpu->cmd_pools );
  crude_resource_pool_deinitialize( &gpu->cmd_buffers );
  
#if CRUDE_GFX_NSIGHT_AFTERMATH
  crude_gfx_gpu_crash_tracker_deinitialize( &gpu->crash_tracker );
#endif
 
  crude_gfx_rhi_destroy_device( &gpu->rhi_device, gpu->rhi_instance );
  crude_gfx_rhi_destroy_surface( gpu->rhi_instance, gpu->rhi_surface );
  crude_gfx_rhi_destroy_instance( gpu->rhi_instance );
}

/************************************************
 *
 * GPU Device Common Functions
 * 
 ***********************************************/  
void
crude_gfx_new_frame
(
  _In_ crude_gfx_device                                   *gpu
)
{
  CRUDE_PROFILER_ZONE_NAME( "crude_gfx_new_frame" );
  if ( gpu->absolute_frame )
  {
    CRUDE_PROFILER_ZONE_NAME( "wait for rhi_graphics_semaphore" );
    crude_gfx_rhi_wait_semaphore( &gpu->rhi_device, gpu->rhi_graphics_semaphore, gpu->absolute_frame );
    CRUDE_PROFILER_ZONE_END;
  }
  
  {
    CRUDE_PROFILER_ZONE_NAME( "crude_gfx_rhi_acquire_next_image" );
    bool acquired = crude_gfx_rhi_acquire_next_image( &gpu->rhi_device, gpu->rhi_swapchain, UINT64_MAX, gpu->rhi_image_avalivable_semaphores[ gpu->current_frame ], &gpu->swapchain_image_index );
    if ( !acquired )
    {
      crude_gfx_resize_swapchain_internal_( gpu );
    }
    CRUDE_PROFILER_ZONE_END;
  }
  
  crude_gfx_cmd_manager_reset( &gpu->cmd_buffer_manager, gpu->current_frame );

  crude_gfx_linear_allocator_clear( &gpu->frame_linear_allocator );

  CRUDE_PROFILER_ZONE_END;
}

void
crude_gfx_present
(
  _In_ crude_gfx_device                                   *gpu,
  _In_ crude_gfx_texture                                  *texture
)
{
  crude_gfx_rhi_command_buffer                             enqueued_command_buffers[ 4 ];
  
  CRUDE_PROFILER_ZONE_NAME( "crude_gfx_present" );
  for ( uint32 i = 0; i < CRUDE_ARRAY_LENGTH( gpu->queued_command_buffers ); ++i )
  {
    crude_gfx_cmd_buffer                                *cmd;
    crude_gfx_cmd_pool                                  *cmd_pool;
    
    cmd = gpu->queued_command_buffers[ i ];
    enqueued_command_buffers[ i ] = cmd->rhi_cmd_buffer;
#if CRUDE_GFX_GPU_PROFILER
    cmd_pool = crude_gfx_access_cmd_pool( gpu, cmd->cmd_pool );

    if ( cmd_pool->profiler.enabled )
    {
      if ( cmd_pool->profiler.time_queries_trees->allocated_time_query )
      {
        crude_gfx_rhi_command_buffer_end_query( cmd->rhi_cmd_buffer, cmd_pool->profiler.rhi_pipeline_stats_query_pool, 0 );
      }
    }
#endif
    crude_gfx_cmd_end_render_pass( cmd );
    crude_gfx_cmd_end( cmd );
  }

  {
    crude_gfx_descriptor_set                              *bindless_descriptor_set;
    crude_gfx_rhi_write_descriptor_set                     bindless_descriptor_writes[ CRUDE_GFX_BINDLESS_RESOURCES_MAX_COUNT ];
    crude_gfx_rhi_descriptor_image_info                    bindless_image_infos[ CRUDE_GFX_BINDLESS_RESOURCES_MAX_COUNT ];
    uint32                                                 current_write_index;
    
    bindless_descriptor_set = crude_gfx_access_descriptor_set( gpu, gpu->bindless_descriptor_set_handle );

    current_write_index = 0;
    for ( int32 i = CRUDE_ARRAY_LENGTH( gpu->texture_to_update_bindless ) - 1; i >= 0; --i )
    {
      crude_gfx_resource_update                           *texture_to_update;
      crude_gfx_texture                                   *texture;
      
      texture_to_update = &gpu->texture_to_update_bindless[ i ];
      texture = crude_gfx_access_texture( gpu, CRUDE_COMPOUNT( crude_gfx_texture_handle, { texture_to_update->handle } ) );
      
      if ( !texture->ready )
      {
        continue;
      }
      
      CRUDE_ASSERT( texture->format != CRUDE_GFX_RHI_FORMAT_UNDEFINED );

      bindless_descriptor_writes[ current_write_index ] = CRUDE_COMPOUNT_EMPTY( crude_gfx_rhi_write_descriptor_set );
      bindless_descriptor_writes[ current_write_index ].descriptor_count = 1;
      bindless_descriptor_writes[ current_write_index ].dst_array_element = texture_to_update->handle;
      bindless_descriptor_writes[ current_write_index ].descriptor_type = CRUDE_GFX_RHI_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
      bindless_descriptor_writes[ current_write_index ].dst_binding = CRUDE_BINDLESS_TEXTURE_BINDING;
      bindless_descriptor_writes[ current_write_index ].image_info = &bindless_image_infos[ current_write_index ];
      
      bindless_image_infos[ current_write_index ].image_view = texture->rhi_image_view;
      bindless_image_infos[ current_write_index ].image_layout = CRUDE_GFX_RHI_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
      if ( texture->sampler )
      {
        bindless_image_infos[ current_write_index ].sampler = texture->sampler->rhi_sampler;
      }
      else
      {
        crude_gfx_sampler *default_sampler = crude_gfx_access_sampler( gpu, gpu->default_sampler );
        bindless_image_infos[ current_write_index ].sampler = default_sampler->rhi_sampler;
      }
      
      CRUDE_ARRAY_DELSWAP( gpu->texture_to_update_bindless, i );

      ++current_write_index;
      
      bool has_compute_mask = texture->flags & CRUDE_GFX_TEXTURE_MASK_COMPUTE;
      if ( CRUDE_RESOURCE_HANDLE_IS_VALID( texture->parent_texture_handle ) )
      {
        has_compute_mask |= crude_gfx_access_texture( gpu, texture->parent_texture_handle )->flags & CRUDE_GFX_TEXTURE_MASK_COMPUTE;
      }

      if ( has_compute_mask )
      {
        bindless_image_infos[ current_write_index ] = bindless_image_infos[ current_write_index - 1 ];
        bindless_image_infos[ current_write_index ].image_layout = CRUDE_GFX_RHI_IMAGE_LAYOUT_GENERAL;

        bindless_descriptor_writes[ current_write_index ] = bindless_descriptor_writes[ current_write_index - 1 ];
        bindless_descriptor_writes[ current_write_index ].dst_binding = CRUDE_BINDLESS_IMAGE_BINDING;
        bindless_descriptor_writes[ current_write_index ].descriptor_type = CRUDE_GFX_RHI_DESCRIPTOR_TYPE_STORAGE_IMAGE;
        bindless_descriptor_writes[ current_write_index ].image_info = &bindless_image_infos[ current_write_index ];

        ++current_write_index;
      }
    }

    if ( current_write_index )
    {
      crude_gfx_rhi_update_descriptor_set( &gpu->rhi_device, bindless_descriptor_set->rhi_descriptor_set, bindless_descriptor_writes, current_write_index );
    }
  }
  
  {
    crude_gfx_rhi_semaphore_submit_info wait_semaphores[] = {
      { gpu->rhi_graphics_semaphore, gpu->absolute_frame, CRUDE_GFX_RHI_PIPELINE_STAGE_TOP_OF_PIPE_BIT_KHR, 0 }
    };
      
    crude_gfx_rhi_semaphore_submit_info signal_semaphores[] = {
      { gpu->rhi_rendering_finished_semaphore[ gpu->swapchain_image_index ], 0, CRUDE_GFX_RHI_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT_KHR, 0 },
    };

    crude_gfx_rhi_command_buffer_submit_info command_buffers[] = {
      { enqueued_command_buffers[ 0 ], 0 },
      { enqueued_command_buffers[ 1 ], 0 },
      { enqueued_command_buffers[ 2 ], 0 },
      { enqueued_command_buffers[ 3 ], 0 },
    };
    
    crude_gfx_rhi_submit_info submit_info = CRUDE_COMPOUNT_EMPTY( crude_gfx_rhi_submit_info );
    submit_info.wait_semaphore_info_count   = gpu->absolute_frame ? 1 : 0;
    submit_info.wait_semaphore_infos        = wait_semaphores;
    submit_info.command_buffer_info_count   = CRUDE_ARRAY_LENGTH( gpu->queued_command_buffers );
    submit_info.command_buffer_infos        = command_buffers;
    submit_info.signal_semaphore_info_count = CRUDE_COUNTOF( signal_semaphores );
    submit_info.signal_semaphore_infos      = signal_semaphores;
    
    crude_gfx_device_queue_submit( gpu, gpu->rhi_main_queue, &submit_info, crude_gfx_rhi_fence_empty( ) );
  }
 
  {
    crude_gfx_cmd_buffer                                  *cmd;
    crude_gfx_rhi_image_copy                               region;

    region = CRUDE_COMPOUNT_EMPTY( crude_gfx_rhi_image_copy );
    region.src_subresource.aspect_mask = CRUDE_GFX_RHI_IMAGE_ASPECT_COLOR_BIT;
    region.src_subresource.mip_level = 0;
    region.src_subresource.base_array_layer = 0;
    region.src_subresource.layer_count = 1;
    region.dst_subresource.aspect_mask = CRUDE_GFX_RHI_IMAGE_ASPECT_COLOR_BIT;
    region.dst_subresource.mip_level = 0;
    region.dst_subresource.base_array_layer = 0;
    region.dst_subresource.layer_count = 1;
    region.extent.x = gpu->renderer_size.x;
    region.extent.y = gpu->renderer_size.y;
    region.extent.z = 1;
    
    cmd = crude_gfx_access_cmd_buffer( gpu, gpu->immediate_transfer_cmd_buffer );
    crude_gfx_cmd_begin_primary( cmd );

    crude_gfx_cmd_add_image_barrier( cmd, texture->handle, CRUDE_GFX_RHI_RESOURCE_STATE_COPY_SOURCE, 0, 1, false );
    crude_gfx_cmd_add_image_barrier_ext2( cmd, gpu->rhi_swapchain_images[ gpu->swapchain_image_index ], CRUDE_GFX_RHI_RESOURCE_STATE_PRESENT, CRUDE_GFX_RHI_RESOURCE_STATE_COPY_DEST, 0, 1, false );
    crude_gfx_rhi_command_buffer_copy_image( cmd->rhi_cmd_buffer, texture->rhi_image, CRUDE_GFX_RHI_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, gpu->rhi_swapchain_images[ gpu->swapchain_image_index ], CRUDE_GFX_RHI_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, &region );
    crude_gfx_cmd_add_image_barrier_ext2( cmd, gpu->rhi_swapchain_images[ gpu->swapchain_image_index ], CRUDE_GFX_RHI_RESOURCE_STATE_COPY_DEST, CRUDE_GFX_RHI_RESOURCE_STATE_PRESENT, 0, 1, false );
    
    crude_gfx_cmd_end( cmd );
  
    {
      crude_gfx_rhi_semaphore_submit_info wait_semaphores[] = {
        { gpu->rhi_rendering_finished_semaphore[ gpu->swapchain_image_index ], 0, CRUDE_GFX_RHI_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, 0 },
        { gpu->rhi_image_avalivable_semaphores[ gpu->current_frame ], 0, CRUDE_GFX_RHI_PIPELINE_STAGE_ALL_COMMANDS_BIT_KHR, 0 },
      };
      
      crude_gfx_rhi_semaphore_submit_info signal_semaphores[] = {
        { gpu->rhi_swapchain_updated_semaphore[ gpu->swapchain_image_index ], 0, CRUDE_GFX_RHI_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, 0 },
        { gpu->rhi_graphics_semaphore, gpu->absolute_frame + 1, CRUDE_GFX_RHI_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, 0 }
      };

      crude_gfx_rhi_command_buffer_submit_info command_buffers[] = {
        { cmd->rhi_cmd_buffer, 0 },
      };
      
      crude_gfx_rhi_submit_info submit_info = CRUDE_COMPOUNT_EMPTY( crude_gfx_rhi_submit_info );
      submit_info.wait_semaphore_info_count   = 2;
      submit_info.wait_semaphore_infos        = wait_semaphores;
      submit_info.command_buffer_info_count   = CRUDE_COUNTOF( command_buffers );
      submit_info.command_buffer_infos        = command_buffers;
      submit_info.signal_semaphore_info_count = CRUDE_COUNTOF( signal_semaphores );
      submit_info.signal_semaphore_infos      = signal_semaphores;
    
      crude_gfx_device_queue_submit( gpu, gpu->rhi_main_queue, &submit_info, crude_gfx_rhi_fence_empty( ) );
    }
  }
  
  {
    bool successful = crude_gfx_rhi_queue_present(
      gpu->rhi_main_queue, gpu->rhi_swapchain_updated_semaphore[ gpu->swapchain_image_index ], gpu->rhi_swapchain, &gpu->swapchain_image_index );
    
    CRUDE_ARRAY_SET_LENGTH( gpu->queued_command_buffers, 0u );
    
    gpu->swapchain_resized_last_frame = false;
    if ( !successful )
    {
      crude_gfx_resize_swapchain_internal_( gpu );
      crude_gfx_update_frame_counters_( gpu );
      return;
    }
  }
  
#if CRUDE_GFX_GPU_PROFILER
  if ( gpu->timestamps_enabled )
  {
    crude_gfx_gpu_pipeline_statistics_reset( &gpu->gpu_time_queries_manager->frame_pipeline_statistics );

    for ( uint32 i = 0; i < gpu->num_threads; ++i )
    {
      crude_gfx_cmd_pool                                  *cmd_pool;
      crude_gfx_gpu_time_query_tree                       *time_query;
      uint32                                               pool_index;

      pool_index = ( gpu->previous_frame * gpu->num_threads ) + i;
      cmd_pool = crude_gfx_access_cmd_pool( gpu, gpu->thread_frame_pools[ pool_index ] );

      if ( cmd_pool->profiler.enabled )
      {
        time_query = cmd_pool->profiler.time_queries_trees;

        if ( time_query && time_query->allocated_time_query )
        {
          uint64                                            *timestamps_data;
          uint64                                            *pipeline_statistics_data;
          uint32                                             query_offset, query_count;

          query_offset = ( pool_index * gpu->gpu_time_queries_manager->queries_per_thread );
          query_count = time_query->allocated_time_query;
          CRUDE_ARRAY_INITIALIZE_WITH_LENGTH( timestamps_data, query_count * 2, crude_heap_allocator_pack( gpu->allocator ) );
          crude_gfx_rhi_get_query_pool_results(
            &gpu->rhi_device,
            cmd_pool->profiler.rhi_timestamp_query_pool,
            0, query_count * 2,
            sizeof( uint64 ) * query_count * 2, timestamps_data,
            sizeof( uint64 ), CRUDE_GFX_RHI_QUERY_RESULT_64_BIT | CRUDE_GFX_RHI_QUERY_RESULT_WAIT_BIT
          );
          
          for ( uint32 i = 0; i < query_count; ++i )
          {
            crude_gfx_gpu_time_query                        *timestamp;
            float64                                          start, end, range, elapsed_time;

            timestamp = &gpu->gpu_time_queries_manager->timestamps[ query_offset + i ];

            start = timestamps_data[ ( i * 2 ) ];
            end = timestamps_data[ ( i * 2 ) + 1 ];
            range = end - start;
            elapsed_time = range * gpu->gpu_timestamp_frequency;

            timestamp->elapsed_ms = elapsed_time;
            timestamp->frame_index = gpu->absolute_frame;
          }
          
          CRUDE_ARRAY_INITIALIZE_WITH_LENGTH( pipeline_statistics_data, CRUDE_GFX_GPU_PIPELINE_STATISTICS_COUNT, crude_heap_allocator_pack( gpu->allocator ) );
          crude_gfx_rhi_get_query_pool_results(
            &gpu->rhi_device,
            cmd_pool->profiler.rhi_pipeline_stats_query_pool,
            0, 1,
            CRUDE_GFX_GPU_PIPELINE_STATISTICS_COUNT * sizeof( uint64 ), pipeline_statistics_data,
            sizeof( uint64 ), CRUDE_GFX_RHI_QUERY_RESULT_64_BIT );

          for ( uint32 i = 0; i < CRUDE_GFX_GPU_PIPELINE_STATISTICS_COUNT; ++i )
          {
            gpu->gpu_time_queries_manager->frame_pipeline_statistics.statistics[ i ] += pipeline_statistics_data[ i ];
          }

          CRUDE_ARRAY_DEINITIALIZE( pipeline_statistics_data );
          CRUDE_ARRAY_DEINITIALIZE( timestamps_data );
        }
      }
    }
  }
#endif

  crude_gfx_update_frame_counters_( gpu );

  for ( uint32 i = 0; i < CRUDE_ARRAY_LENGTH( gpu->resource_deletion_queue ); ++i )
  {
    crude_gfx_resource_update* resource_deletion = &gpu->resource_deletion_queue[ i ];
    
    if ( resource_deletion->current_frame != gpu->current_frame )
    {
      continue;
    }
    
    crude_gfx_destroy_resources_instant_( gpu, resource_deletion->type, resource_deletion->handle );
  
    resource_deletion->current_frame = UINT32_MAX;
    CRUDE_ARRAY_DELSWAP( gpu->resource_deletion_queue, i );
    --i;
  }

  CRUDE_PROFILER_ZONE_END;
}

crude_gfx_cmd_buffer*
crude_gfx_get_primary_cmd
(
  _In_ crude_gfx_device                                   *gpu,
  _In_ uint32                                              thread_index,
  _In_ bool                                                begin
)
{
  crude_gfx_cmd_buffer *cmd = crude_gfx_cmd_manager_get_primary_cmd( &gpu->cmd_buffer_manager, gpu->current_frame, thread_index, begin );
  return cmd;
}

void
crude_gfx_queue_cmd
(
  _In_ crude_gfx_cmd_buffer                               *cmd
)
{
  CRUDE_ARRAY_PUSH( cmd->gpu->queued_command_buffers, cmd );
}

void
crude_gfx_link_texture_sampler
(
  _In_ crude_gfx_device                                   *gpu,
  _In_ crude_gfx_texture_handle                            texture_handle,
  _In_ crude_gfx_sampler_handle                            sampler_handle
)
{
  crude_gfx_texture* texture = crude_gfx_access_texture( gpu, texture_handle );
  crude_gfx_sampler* sampler = crude_gfx_access_sampler( gpu, sampler_handle );
  
  texture->sampler = sampler;
}

crude_gfx_descriptor_set_layout_handle
crude_gfx_get_descriptor_set_layout
(
  _In_ crude_gfx_device                                   *gpu,
  _In_ crude_gfx_pipeline_handle                           pipeline_handle,
  _In_ uint32                                              layout_index
)
{
  crude_gfx_pipeline *pipeline = crude_gfx_access_pipeline( gpu, pipeline_handle );
  return pipeline->descriptor_set_layout_handle[ layout_index ];
}

bool
crude_gfx_buffer_ready
(
  _In_ crude_gfx_device                                   *gpu,
  _In_ crude_gfx_buffer_handle                             buffer_handle
)
{
  crude_gfx_buffer *buffer = crude_gfx_access_buffer( gpu, buffer_handle );
  return buffer->ready;
}

bool
crude_gfx_texture_ready
(
  _In_ crude_gfx_device                                   *gpu,
  _In_ crude_gfx_texture_handle                            texture_handle
)
{
  crude_gfx_texture *texture = crude_gfx_access_texture( gpu, texture_handle );
  return texture->ready;
}

void
crude_gfx_compile_shader
(
  _In_ crude_gfx_device                                   *gpu,
  _In_ char const                                         *code,
  _In_ uint32                                              code_size,
  _In_ crude_gfx_rhi_shader_stage_flag_bits                stage,
  _In_ bool                                                optimized,
  _In_ char const                                         *name,
  _In_ crude_heap_allocator                               *allocator,
  _Out_ char                                             **spirv_absolute_filepath
)
{
#if CRUDE_GFX_DX12
  crude_gfx_rhi_compile_spirv_to_dxil_description          spirv_to_dxil_desc;
#endif /* CRUDE_GFX_DX12 */
  crude_gfx_rhi_compile_glsl_to_spirv_description          glsl_to_spirv_desc;
  
  glsl_to_spirv_desc = CRUDE_COMPOUNT_EMPTY( crude_gfx_rhi_compile_glsl_to_spirv_description );
  glsl_to_spirv_desc.code = code;
  glsl_to_spirv_desc.code_size = code_size;
  glsl_to_spirv_desc.stage = stage;
  glsl_to_spirv_desc.pass_name = name;
  glsl_to_spirv_desc.temporary_absolute_directory = gpu->environment->directories.temporary_absolute_directory;
  glsl_to_spirv_desc.compiled_absolute_directory = gpu->environment->directories.compiled_shaders_absolute_directory;
  glsl_to_spirv_desc.optimized = optimized;
  crude_gfx_rhi_compile_shader_glsl_to_spirv( &glsl_to_spirv_desc, allocator, spirv_absolute_filepath );

#if CRUDE_GFX_DX12
  crude_gfx_rhi_compile_spirv_to_dxil_description          spirv_to_dxil_desc;

  spirv_to_dxil_desc = CRUDE_COMPOUNT_EMPTY( crude_gfx_rhi_compile_spirv_to_dxil_description );
  spirv_to_dxil_desc.stage = stage;
  spirv_to_dxil_desc.spirv_absolute_filepath = *spirv_absolute_filepath;
  spirv_to_dxil_desc.temporary_absolute_directory = gpu->temporary_absolute_directory;
  spirv_to_dxil_desc.compiled_absolute_directory = gpu->compiled_shaders_absolute_directory;

  crude_gfx_rhi_compile_shader_spirv_to_dxil( &spirv_to_dxil_desc, allocator, NULL );
#endif /* CRUDE_GFX_DX12 */
}

void
crude_gfx_update_frame_counters_
(
  _In_ crude_gfx_device                                   *gpu
)
{
  gpu->previous_frame = gpu->current_frame;
  gpu->current_frame = ( gpu->current_frame + 1u ) % gpu->swapchain_images_count;
  gpu->absolute_frame = gpu->absolute_frame + 1;
}

void
crude_gfx_push_resource_update_
(
  _In_ crude_gfx_device                                   *gpu,
  _In_ crude_gfx_resource_update const                    *update
)
{
  mtx_lock( &gpu->resource_deletion_queue_mutex );
  CRUDE_ARRAY_PUSH( gpu->resource_deletion_queue, *update );
  mtx_unlock( &gpu->resource_deletion_queue_mutex );
}

void
crude_gfx_resize_framebuffer
(
  _In_ crude_gfx_device                                   *gpu,
  _In_ crude_gfx_framebuffer_handle                        framebuffer_handle,
  _In_ uint32                                              width,
  _In_ uint32                                              height
)
{
  crude_gfx_framebuffer *framebuffer = crude_gfx_access_framebuffer( gpu, framebuffer_handle );
  if ( !framebuffer )
  {
    return;
  }

  if ( !framebuffer->resize )
  {
    return;
  }
  
  uint16 new_width = width * framebuffer->scale_x;
  uint16 new_height = height * framebuffer->scale_y;
  
  for ( size_t i = 0; i < framebuffer->num_color_attachments; ++i )
  {
    crude_gfx_resize_texture( gpu, framebuffer->color_attachments[ i ], new_width, new_height );
  }
  
  if ( CRUDE_RESOURCE_HANDLE_IS_VALID( framebuffer->depth_stencil_attachment ) )
  {
    crude_gfx_resize_texture( gpu, framebuffer->depth_stencil_attachment, new_width, new_height );
  }
  
  framebuffer->width = new_width;
  framebuffer->height = new_height;
}

void
crude_gfx_resize_texture
(
  _In_ crude_gfx_device                                   *gpu,
  _In_ crude_gfx_texture_handle                            texture_handle,
  _In_ uint32                                              width,
  _In_ uint32                                              height
)
{
  crude_gfx_texture *texture = crude_gfx_access_texture( gpu, texture_handle );
  
  if ( texture->width == width && texture->height == height )
  {
    return;
  }
  
  crude_gfx_texture_handle texture_to_delete_handle = crude_gfx_obtain_texture( gpu );
  crude_gfx_texture *texture_to_delete = crude_gfx_access_texture( gpu, texture_to_delete_handle );

  memcpy( texture_to_delete, texture, sizeof( crude_gfx_texture ) );
  texture_to_delete->handle = texture_to_delete_handle;
  
  crude_gfx_texture_creation texture_creation = crude_gfx_texture_creation_empty();
  texture_creation.flags = texture->flags;
  texture_creation.subresource = texture->subresource;
  texture_creation.format = texture->format;
  texture_creation.type = texture->type;
  crude_string_copy( texture_creation.name, texture->name, sizeof( texture_creation.name ) );
  texture_creation.width = width;
  texture_creation.height = height;
  texture_creation.depth = 1;

  crude_gfx_create_texture_internal_( gpu, &texture_creation, texture->handle, texture );
  
  crude_gfx_destroy_texture( gpu, texture_to_delete_handle );
}

void                                     
crude_gfx_device_queue_submit
(
  _In_ crude_gfx_device                                   *gpu,
  _In_ crude_gfx_rhi_queue                                 rhi_queue,
  _In_ crude_gfx_rhi_submit_info                          *submit_info,
  _In_ crude_gfx_rhi_fence                                 rhi_fence
)
{
  bool successful = crude_gfx_rhi_queue_submit( rhi_queue, 1, submit_info, rhi_fence );

  if ( !successful )
  {
#if CRUDE_GFX_NSIGHT_AFTERMATH
    crude_gfx_gpu_crash_tracker_handle_device_lost( &gpu->crash_tracker );
#endif
  }
}

#if CRUDE_GFX_GPU_PROFILER
uint32
crude_gfx_copy_gpu_timestamps
(
  _In_ crude_gfx_device                                   *gpu,
  _Out_ crude_gfx_gpu_time_query                          *timestamps
)
{
  return crude_gfx_gpu_time_queries_manager_resolve( gpu->gpu_time_queries_manager, gpu, gpu->previous_frame, timestamps );
}
#endif

void
crude_gfx_gpu_set_timestamps_enable
(
  _In_ crude_gfx_device                                   *gpu,
  _In_ bool                                                value
)
{
  gpu->timestamps_enabled = value;
}

void
crude_gfx_generate_mipmaps
(
  _In_ crude_gfx_cmd_buffer                               *cmd_buffer,
  _In_ crude_gfx_texture                                  *texture
)
{
  int32                                                    w, h;

  if ( texture->subresource.mip_level_count < 2 )
  {
    return;
  }

  crude_gfx_cmd_add_image_barrier( cmd_buffer, texture->handle, CRUDE_GFX_RHI_RESOURCE_STATE_COPY_SOURCE, 0, 1, false );

  w = texture->width;
  h = texture->height;

  for ( uint32 mip_index = 1; mip_index < texture->subresource.mip_level_count; ++mip_index )
  {
    crude_gfx_rhi_image_blit                               blit_region;

    crude_gfx_cmd_add_image_barrier_ext2( cmd_buffer, texture->rhi_image, CRUDE_GFX_RHI_RESOURCE_STATE_UNDEFINED, CRUDE_GFX_RHI_RESOURCE_STATE_COPY_DEST, mip_index, 1, false );

    blit_region = CRUDE_COMPOUNT_EMPTY( crude_gfx_rhi_image_blit );

    blit_region.src_subresource.aspect_mask = CRUDE_GFX_RHI_IMAGE_ASPECT_COLOR_BIT;
    blit_region.src_subresource.mip_level = mip_index - 1;
    blit_region.src_subresource.base_array_layer = 0;
    blit_region.src_subresource.layer_count = 1;
    
    blit_region.src_offsets[ 0 ].x = 0;
    blit_region.src_offsets[ 0 ].y = 0;
    blit_region.src_offsets[ 0 ].z = 0;
    blit_region.src_offsets[ 1 ].x = w;
    blit_region.src_offsets[ 1 ].y = h;
    blit_region.src_offsets[ 1 ].z = 1;

    w /= 2;
    h /= 2;

    blit_region.dst_subresource.aspect_mask = CRUDE_GFX_RHI_IMAGE_ASPECT_COLOR_BIT;
    blit_region.dst_subresource.mip_level = mip_index;
    blit_region.dst_subresource.base_array_layer = 0;
    blit_region.dst_subresource.layer_count = 1;

    blit_region.dst_offsets[ 0 ].x = 0;
    blit_region.dst_offsets[ 0 ].y = 0;
    blit_region.dst_offsets[ 0 ].z = 0;
    blit_region.dst_offsets[ 1 ].x = w;
    blit_region.dst_offsets[ 1 ].y = h;
    blit_region.dst_offsets[ 1 ].z = 1;

    crude_gfx_rhi_command_buffer_blit_image( cmd_buffer->rhi_cmd_buffer, texture->rhi_image, CRUDE_GFX_RHI_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, texture->rhi_image, CRUDE_GFX_RHI_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, &blit_region, CRUDE_GFX_RHI_FILTER_LINEAR );

    crude_gfx_cmd_add_image_barrier_ext2( cmd_buffer, texture->rhi_image, CRUDE_GFX_RHI_RESOURCE_STATE_COPY_DEST, CRUDE_GFX_RHI_RESOURCE_STATE_COPY_SOURCE, mip_index, 1, false );
  }

  crude_gfx_cmd_add_image_barrier_ext2( cmd_buffer, texture->rhi_image, CRUDE_GFX_RHI_RESOURCE_STATE_COPY_SOURCE, CRUDE_GFX_RHI_RESOURCE_STATE_SHADER_RESOURCE, 0, texture->subresource.mip_level_count, false );

  texture->state = CRUDE_GFX_RHI_RESOURCE_STATE_SHADER_RESOURCE;
}

crude_gfx_rhi_device_address
crude_gfx_get_buffer_device_address
(
  _In_ crude_gfx_device                                   *gpu,
  _In_ crude_gfx_buffer_handle                             handle
)
{
  crude_gfx_buffer *buffer = crude_gfx_access_buffer( gpu, handle );
  CRUDE_ASSERT( buffer );
  return crude_gfx_rhi_get_buffer_device_address( &gpu->rhi_device, buffer->rhi_buffer );
}

void 
crude_gfx_submit_immediate
(
  _In_ crude_gfx_cmd_buffer                               *cmd
)
{
  crude_gfx_cmd_pool                                      *cmd_pool;

#if CRUDE_GFX_GPU_PROFILER
  cmd_pool = crude_gfx_access_cmd_pool( cmd->gpu, cmd->cmd_pool );
  if ( cmd_pool->profiler.enabled )
  {
    crude_gfx_rhi_command_buffer_end_query( cmd->rhi_cmd_buffer, cmd_pool->profiler.rhi_pipeline_stats_query_pool, 0 );
  }
#endif

  crude_gfx_cmd_end( cmd );

  crude_gfx_rhi_reset_fence( &cmd->gpu->rhi_device, &cmd->gpu->rhi_immediate_fence );
  crude_gfx_rhi_queue_submit_simple( cmd->gpu->rhi_main_queue, cmd->rhi_cmd_buffer, cmd->gpu->rhi_immediate_fence );
  crude_gfx_rhi_wait_for_fence( &cmd->gpu->rhi_device, cmd->gpu->rhi_immediate_fence );
}

void
crude_gfx_add_texture_to_update
(
  _In_ crude_gfx_device                                   *gpu,
  _In_ crude_gfx_texture_handle                            texture
)
{
  mtx_lock( &gpu->texture_update_mutex );
  gpu->textures_to_update[ gpu->num_textures_to_update++ ] = texture;
  CRUDE_ASSERT( gpu->num_textures_to_update < 128 );
  mtx_unlock( &gpu->texture_update_mutex );
}

void
crude_gfx_add_texture_update_commands
(
  _In_ crude_gfx_device                                   *gpu,
  _In_ crude_gfx_cmd_buffer                               *cmd
)
{
  mtx_lock( &gpu->texture_update_mutex );
  
  if ( gpu->num_textures_to_update == 0 )
  {
    goto cleanup;
  }
  
  for ( uint32 i = 0; i < gpu->num_textures_to_update; ++i )
  {
    crude_gfx_texture *texture = crude_gfx_access_texture( gpu, gpu->textures_to_update[ i ] );
    CRUDE_LOG_INFO( CRUDE_CHANNEL_GRAPHICS, "crude_gfx_add_texture_update_commands %s", texture->name );
    crude_gfx_cmd_add_image_barrier_ext3( cmd, texture->rhi_image, CRUDE_GFX_RHI_RESOURCE_STATE_COPY_DEST, CRUDE_GFX_RHI_RESOURCE_STATE_SHADER_RESOURCE, 0, 1, false, gpu->rhi_transfer_queue, gpu->rhi_main_queue, CRUDE_GFX_RHI_QUEUE_TYPE_COPY_TRANSFER, CRUDE_GFX_RHI_QUEUE_TYPE_GRAPHICS );
    texture->ready = true;
    texture->state = CRUDE_GFX_RHI_RESOURCE_STATE_SHADER_RESOURCE;
    crude_gfx_generate_mipmaps( cmd, texture );
  }
  
  gpu->num_textures_to_update = 0;

cleanup:
  mtx_unlock( &gpu->texture_update_mutex );
}

crude_gfx_technique*
crude_gfx_access_technique_by_name
(
  _In_ crude_gfx_device                                   *gpu,
  _In_ char const                                         *technique_name
)
{
  int64 technique_index = CRUDE_HASHMAPSTR_GET_INDEX( gpu->resource_cache.techniques, technique_name );
  CRUDE_ASSERT( technique_index > -1 );

  crude_gfx_technique *technique = gpu->resource_cache.techniques[ technique_index ].value;
  return technique;
}

crude_gfx_technique_pass*
crude_gfx_access_technique_pass_by_name
(
  _In_ crude_gfx_device                                   *gpu,
  _In_ char const                                         *technique_name,
  _In_ char const                                         *pass_name
)
{
  crude_gfx_technique *technique = crude_gfx_access_technique_by_name( gpu, technique_name );
  uint64 pass_index = crude_gfx_technique_get_pass_index( technique, pass_name );
  return &technique->passes[ pass_index ];
}

void
crude_gfx_reset_cmd_pool
(
  _In_ crude_gfx_device                                   *gpu,
  _In_ crude_gfx_cmd_pool_handle                           handle
)
{
  crude_gfx_rhi_reset_command_pool( &gpu->rhi_device, crude_gfx_access_cmd_pool( gpu, handle )->rhi_cmd_pool );
}

/************************************************
 *
 * GPU Device Resources Functions
 * 
 ***********************************************/   
crude_gfx_sampler_handle
crude_gfx_create_sampler
(
  _In_ crude_gfx_device                                   *gpu,
  _In_ crude_gfx_sampler_creation const                   *creation
)
{
  crude_gfx_rhi_sampler_create_info                        rhi_creation;
  crude_gfx_sampler                                       *sampler;
  crude_gfx_sampler_handle                                 handle;

  handle = crude_gfx_obtain_sampler( gpu );
  if ( CRUDE_RESOURCE_HANDLE_IS_INVALID( handle ) )
  {
    return handle;
  }
  
  sampler = crude_gfx_access_sampler( gpu, handle );
  sampler->address_mode_u = creation->address_mode_u;
  sampler->address_mode_v = creation->address_mode_v;
  sampler->address_mode_w = creation->address_mode_w;
  sampler->min_filter = creation->min_filter;
  sampler->mag_filter = creation->mag_filter;
  sampler->mip_filter = creation->mip_filter;
  sampler->name = creation->name;
  
  rhi_creation = CRUDE_COMPOUNT_EMPTY( crude_gfx_rhi_sampler_create_info );
  rhi_creation.min_filter = creation->min_filter;
  rhi_creation.mag_filter = creation->mag_filter;
  rhi_creation.mipmap_mode = creation->mip_filter;
  rhi_creation.address_mode_u = creation->address_mode_u;
  rhi_creation.address_mode_v = creation->address_mode_v;
  rhi_creation.address_mode_w = creation->address_mode_w;
  rhi_creation.anisotropy_enable = 0;
  rhi_creation.compare_enable = 0;
  rhi_creation.border_color = CRUDE_GFX_RHI_BORDER_COLOR_INT_OPAQUE_WHITE;
  rhi_creation.unnormalized_coordinates = 0;
  rhi_creation.min_lod = 0.f;
  rhi_creation.max_lod = CRUDE_GFX_RHI_LOD_CLAMP_NONE;
  rhi_creation.reduction_mode = creation->reduction_mode;

  crude_gfx_rhi_create_sampler( &gpu->rhi_device, &rhi_creation, &sampler->rhi_sampler );  
  crude_gfx_rhi_set_sampler_debug_name( &gpu->rhi_device, sampler->rhi_sampler, creation->name );
  
  return handle;
}

void
crude_gfx_destroy_sampler
(
  _In_ crude_gfx_device                                   *gpu,
  _In_ crude_gfx_sampler_handle                            handle
)
{
  if ( handle.index >= gpu->samplers.pool_size )
  {
    CRUDE_LOG_ERROR( CRUDE_CHANNEL_GRAPHICS, "Trying to free invalid sampler %u", handle.index );
    return;
  }
  crude_gfx_resource_update sampler_update_event = { 
    .type          = CRUDE_GFX_RESOURCE_DELETION_TYPE_SAMPLER,
    .handle        = handle.index,
    .current_frame = gpu->current_frame };
  crude_gfx_push_resource_update_( gpu, &sampler_update_event );
}

void
crude_gfx_destroy_sampler_instant
(
  _In_ crude_gfx_device                                   *gpu,
  _In_ crude_gfx_sampler_handle                            handle
)
{
  crude_gfx_sampler *sampler = crude_gfx_access_sampler( gpu, handle );
  if ( sampler )
  {
    crude_gfx_rhi_destroy_sampler( &gpu->rhi_device, sampler->rhi_sampler );
  }
  crude_gfx_release_sampler( gpu, handle );
}

crude_gfx_texture_handle
crude_gfx_create_texture
(
  _In_ crude_gfx_device                                   *gpu,
  _In_ crude_gfx_texture_creation const                   *creation
)
{
  crude_gfx_texture                                       *texture;
  crude_gfx_texture_handle                                 handle;

  handle = crude_gfx_obtain_texture( gpu );
  if ( CRUDE_RESOURCE_HANDLE_IS_INVALID( handle ) )
  {
    return handle;
  }
  
  texture = crude_gfx_access_texture( gpu, handle );
  crude_gfx_create_texture_internal_( gpu, creation, handle, texture );

  if ( creation->initial_data )
  {
    crude_gfx_cmd_buffer                                  *cmd;
    void                                                  *destination_data;
    crude_gfx_rhi_buffer_create_info                       buffre_creation;
    crude_gfx_rhi_buffer_image_copy                        region;
    crude_gfx_rhi_buffer                                   staging_buffer;
    uint64                                                 image_size;
    
    image_size = creation->width * creation->height * 4u;

    buffre_creation = CRUDE_COMPOUNT_EMPTY( crude_gfx_rhi_buffer_create_info );
    buffre_creation.usage = CRUDE_GFX_RHI_BUFFER_USAGE_TRANSFER_SRC_BIT;
    buffre_creation.size = image_size;
    
    crude_gfx_rhi_create_buffer( &gpu->rhi_device, &buffre_creation, &staging_buffer );
    
    crude_gfx_rhi_map_buffer( &gpu->rhi_device, staging_buffer, &destination_data );
#if CRUDE_GFX_VULKAN
    memcpy( destination_data, creation->initial_data, image_size );
#elif CRUDE_GFX_DX12
#elif CRUDE_GFX_NAPI
#else
    CRUDE_GFX_RHI_TO_IMPLEMENTIT
#endif
    crude_gfx_rhi_unmap_buffer( &gpu->rhi_device, staging_buffer );
    
    cmd = crude_gfx_access_cmd_buffer( gpu, gpu->immediate_transfer_cmd_buffer );
    crude_gfx_cmd_begin_primary( cmd );

    crude_gfx_cmd_add_image_barrier( cmd, texture->handle, CRUDE_GFX_RHI_RESOURCE_STATE_COPY_DEST, 0, 1, false );

    region = CRUDE_COMPOUNT_EMPTY( crude_gfx_rhi_buffer_image_copy );
    region.buffer_offset = 0;
    region.buffer_row_length = 0;
    region.buffer_image_height = 0;
    region.image_subresource.aspect_mask = CRUDE_GFX_RHI_IMAGE_ASPECT_COLOR_BIT;
    region.image_subresource.mip_level = 0;
    region.image_subresource.base_array_layer = 0;
    region.image_subresource.layer_count = 1;
    region.image_offset = { 0, 0, 0 };
    region.image_extent.x = creation->width;
    region.image_extent.y = creation->height;
    region.image_extent.z = creation->depth;  
    crude_gfx_rhi_command_buffer_copy_buffer_to_image( cmd->rhi_cmd_buffer, staging_buffer, texture->rhi_image, &region );

    crude_gfx_generate_mipmaps( cmd, texture );
    
    crude_gfx_submit_immediate( cmd );

    crude_gfx_rhi_destroy_buffer( &gpu->rhi_device, staging_buffer );
  }

  texture->ready = true;

  return handle;
}

void
crude_gfx_destroy_texture
(
  _In_ crude_gfx_device                                   *gpu,
  _In_ crude_gfx_texture_handle                            handle
)
{
  if ( handle.index >= gpu->textures.pool_size )
  {
    CRUDE_LOG_ERROR( CRUDE_CHANNEL_GRAPHICS, "Trying to free invalid texture %u", handle.index );
    return;
  }
  crude_gfx_resource_update texture_update_event = { 
    .type          = CRUDE_GFX_RESOURCE_DELETION_TYPE_TEXTURE,
    .handle        = handle.index,
    .current_frame = gpu->current_frame };
  crude_gfx_push_resource_update_( gpu, &texture_update_event );
  
  crude_gfx_texture* buffer = crude_gfx_access_texture( gpu, handle );
  crude_gfx_resource_update texture_update_bindless_event = { 
    .type          = CRUDE_GFX_RESOURCE_DELETION_TYPE_TEXTURE,
    .handle        = handle.index,
    .current_frame = gpu->current_frame };
  CRUDE_ARRAY_PUSH( gpu->texture_to_update_bindless, texture_update_event );
}

void
crude_gfx_destroy_texture_instant
(
  _In_ crude_gfx_device                                   *gpu,
  _In_ crude_gfx_texture_handle                            handle
)
{
  crude_gfx_texture *texture = crude_gfx_access_texture( gpu, handle );
  
  if ( texture )
  {
    if ( CRUDE_RESOURCE_HANDLE_IS_VALID( texture->alias_texture_handle ) )
    {
      texture->rhi_image.vma_allocation = NULL;
    }

    crude_gfx_rhi_destroy_image_view( &gpu->rhi_device, texture->rhi_image_view );
    if ( CRUDE_RESOURCE_HANDLE_IS_INVALID( texture->parent_texture_handle ) )
    {
      crude_gfx_rhi_destroy_image( &gpu->rhi_device, texture->rhi_image );
    }
  }
  crude_gfx_release_texture( gpu, handle );
}

crude_gfx_texture_handle                     
crude_gfx_create_texture_view
(                                                  
  _In_ crude_gfx_device                                   *gpu,
  _In_ crude_gfx_texture_view_creation const              *creation
)
{
  crude_gfx_texture_handle texture_handle = crude_gfx_obtain_texture( gpu );
  if ( CRUDE_RESOURCE_HANDLE_IS_INVALID( texture_handle ) )
  {
    return texture_handle;
  }
  
  crude_gfx_texture *parent_texture = crude_gfx_access_texture( gpu, creation->parent_texture_handle );
  crude_gfx_texture *texture_view = crude_gfx_access_texture( gpu, texture_handle );
  
  crude_memory_copy( texture_view, parent_texture, sizeof( crude_gfx_texture ) );
  
  texture_view->parent_texture_handle = creation->parent_texture_handle;
  texture_view->handle = texture_handle;
  texture_view->subresource.array_base_layer = creation->subresource.array_base_layer;
  texture_view->subresource.mip_base_level = creation->subresource.mip_base_level;
  
  crude_gfx_create_texture_view_internal_( gpu, creation, texture_view );
  
  crude_gfx_resource_update texture_update_event = CRUDE_COMPOUNT_EMPTY( crude_gfx_resource_update );
  texture_update_event.type = CRUDE_GFX_RESOURCE_DELETION_TYPE_TEXTURE;
  texture_update_event.handle = texture_handle.index;
  texture_update_event.current_frame = gpu->current_frame;
  CRUDE_ARRAY_PUSH( gpu->texture_to_update_bindless, texture_update_event );

  return texture_handle;
}

crude_gfx_shader_state_handle
crude_gfx_create_shader_state
(
  _In_ crude_gfx_device                                   *gpu,
  _In_ crude_gfx_shader_state_creation const              *creation
)
{
  crude_gfx_shader_state                                  *shader_state;
  crude_string_buffer                                      temporary_string_buffer;
  crude_gfx_shader_state_handle                            shader_state_handle;
  uint32                                                   compiled_shaders_count;
  bool                                                     creation_failed;

  if ( creation->stages_count == 0 || creation->stages == NULL )
  {
    CRUDE_LOG_ERROR( CRUDE_CHANNEL_GRAPHICS, "Shader %s does not contain shader stages.", creation->name );
    return CRUDE_GFX_SHADER_STATE_HANDLE_INVALID;
  }
  
  shader_state_handle = crude_gfx_obtain_shader_state( gpu );
  if ( CRUDE_RESOURCE_HANDLE_IS_INVALID( shader_state_handle ) )
  {
    return shader_state_handle;
  }

  compiled_shaders_count = 0u;

  shader_state = crude_gfx_access_shader_state( gpu, shader_state_handle );
  shader_state->pipeline_type = CRUDE_GFX_PIPELINE_TYPE_COUNT;
  shader_state->active_shaders = 0;
  
  crude_string_buffer_initialize( &temporary_string_buffer, CRUDE_RKILO( 1 ), crude_heap_allocator_pack( gpu->allocator ) );
  
  for ( compiled_shaders_count = 0; compiled_shaders_count < creation->stages_count; ++compiled_shaders_count )
  {
    crude_gfx_shader_stage const                          *stage;
    crude_gfx_rhi_pipeline_shader_stage_create_info       *shader_stage_info;
    uint32                                                *code;
    uint32                                                 code_size;
    crude_gfx_rhi_shader_module_create_info                rhi_creation_info;
    
    crude_string_buffer_clear( &temporary_string_buffer );

    stage = &creation->stages[ compiled_shaders_count ];
  
    if ( stage->type == CRUDE_GFX_RHI_SHADER_STAGE_VERTEX_BIT )
    {
      shader_state->pipeline_type = CRUDE_GFX_PIPELINE_TYPE_CLASSIC;
    }

    if ( stage->type == CRUDE_GFX_RHI_SHADER_STAGE_MESH_BIT_EXT || stage->type == CRUDE_GFX_RHI_SHADER_STAGE_TASK_BIT_EXT )
    {
      shader_state->pipeline_type = CRUDE_GFX_PIPELINE_TYPE_TASK;
    }

    if ( stage->type == CRUDE_GFX_RHI_SHADER_STAGE_COMPUTE_BIT )
    {
      shader_state->pipeline_type = CRUDE_GFX_PIPELINE_TYPE_COMPUTE;
    }
  
#if CRUDE_COMPILE_SHADERS
    {
      char                                                *spirv_absolute_filepath;

      crude_gfx_compile_shader( gpu, stage->code, stage->code_size, stage->type, creation->spv_input, creation->name, gpu->allocator, &spirv_absolute_filepath );
      
      code = NULL;
      code_size = 0;
      
      crude_read_file_binary( spirv_absolute_filepath, NULL, &code_size );
      
      code = CRUDE_CAST( uint32*, crude_heap_allocator_allocate( gpu->allocator, code_size ) );
      
      crude_read_file_binary( spirv_absolute_filepath, CRUDE_CAST( uint8*, code ), &code_size );
      
      crude_heap_allocator_deallocate( gpu->allocator, spirv_absolute_filepath );
    }
#else /* CRUDE_COMPILE_SHADERS */
    {
      char                                                *spirv_optimized_absolute_filepath;

      spirv_optimized_absolute_filepath = crude_string_buffer_append_use_f(
        &temporary_string_buffer,
        "%s\\%s.%s.shader_opt.spv",
        gpu->compiled_shaders_absolute_directory,
        creation->name ? creation->name : "unknown",
        crude_gfx_rhi_shader_stage_to_compiler_extension( stage->type ) );
      
      code = NULL;
      code_size = 0;
      
      crude_read_file_binary( spirv_optimized_absolute_filepath, NULL, &code_size );
      
      code = CRUDE_CAST( uint32*, crude_heap_allocator_allocate( gpu->allocator, code_size ) );
      
      crude_read_file_binary( spirv_optimized_absolute_filepath, CRUDE_CAST( uint8*, code ), &code_size );
    }
#endif /* CRUDE_COMPILE_SHADERS */

    rhi_creation_info = CRUDE_COMPOUNT_EMPTY( crude_gfx_rhi_shader_module_create_info );
    rhi_creation_info.code = code;
    rhi_creation_info.code_size = code_size;
  
    CRUDE_ASSERTM( CRUDE_CHANNEL_GRAPHICS, rhi_creation_info.code && rhi_creation_info.code_size, "\"%s\" shader code contains an error or empty!", creation->name ? creation->name : "unkown" );

    if ( !rhi_creation_info.code || !rhi_creation_info.code_size )
    {
      return CRUDE_GFX_SHADER_STATE_HANDLE_INVALID;
    }

    shader_stage_info = &shader_state->shader_stage_info[ compiled_shaders_count ];
    *shader_stage_info = CRUDE_COMPOUNT_EMPTY( crude_gfx_rhi_pipeline_shader_stage_create_info );
    shader_stage_info->name = "main";
    shader_stage_info->stage = stage->type;
    
    if ( !crude_gfx_rhi_create_shader_module( &gpu->rhi_device, &rhi_creation_info, gpu->allocator, &shader_state->shader_stage_info[ compiled_shaders_count ].rhi_module ) )
    {
      break;
    }
    
#if CRUDE_GFX_RAY_TRACING_ENABLED
    switch ( stage->type )
    {
      case CRUDE_GFX_RHI_SHADER_STAGE_RAYGEN_BIT_KHR:
      {
        crude_gfx_rhi_ray_tracing_shader_group_create_info *shader_group_info = &shader_state->shader_group_info[ compiled_shaders_count ];
        *shader_group_info = CRUDE_COMPOUNT_EMPTY( crude_gfx_rhi_ray_tracing_shader_group_create_info );
        shader_group_info->type = CRUDE_GFX_RHI_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR;
        shader_group_info->general_shader = compiled_shaders_count;
        shader_group_info->closest_hit_shader = CRUDE_GFX_RHI_SHADER_UNUSED_KHR;
        shader_group_info->any_hit_shader = CRUDE_GFX_RHI_SHADER_UNUSED_KHR;
        shader_group_info->intersection_shader = CRUDE_GFX_RHI_SHADER_UNUSED_KHR;
        shader_state->pipeline_type = CRUDE_GFX_PIPELINE_TYPE_RAY_TRACING;
        break;
      }
      case CRUDE_GFX_RHI_SHADER_STAGE_ANY_HIT_BIT_KHR:
      {
        crude_gfx_rhi_ray_tracing_shader_group_create_info *shader_group_info = &shader_state->shader_group_info[ compiled_shaders_count ];
        *shader_group_info = CRUDE_COMPOUNT_EMPTY( crude_gfx_rhi_ray_tracing_shader_group_create_info );
        shader_group_info->type = CRUDE_GFX_RHI_RAY_TRACING_SHADER_GROUP_TYPE_TRIANGLES_HIT_GROUP_KHR;
        shader_group_info->general_shader = CRUDE_GFX_RHI_SHADER_UNUSED_KHR;
        shader_group_info->closest_hit_shader = CRUDE_GFX_RHI_SHADER_UNUSED_KHR;
        shader_group_info->any_hit_shader = compiled_shaders_count;
        shader_group_info->intersection_shader = CRUDE_GFX_RHI_SHADER_UNUSED_KHR;
        shader_state->pipeline_type = CRUDE_GFX_PIPELINE_TYPE_RAY_TRACING;
        break;
      }
      case CRUDE_GFX_RHI_SHADER_STAGE_CLOSEST_HIT_BIT_KHR:
      {
        crude_gfx_rhi_ray_tracing_shader_group_create_info *shader_group_info = &shader_state->shader_group_info[ compiled_shaders_count ];
        *shader_group_info = CRUDE_COMPOUNT_EMPTY( crude_gfx_rhi_ray_tracing_shader_group_create_info );
        shader_group_info->type = CRUDE_GFX_RHI_RAY_TRACING_SHADER_GROUP_TYPE_TRIANGLES_HIT_GROUP_KHR;
        shader_group_info->general_shader = CRUDE_GFX_RHI_SHADER_UNUSED_KHR;
        shader_group_info->closest_hit_shader = compiled_shaders_count;
        shader_group_info->any_hit_shader = CRUDE_GFX_RHI_SHADER_UNUSED_KHR;
        shader_group_info->intersection_shader = CRUDE_GFX_RHI_SHADER_UNUSED_KHR;
        shader_state->pipeline_type = CRUDE_GFX_PIPELINE_TYPE_RAY_TRACING;
        break;
      }
      case CRUDE_GFX_RHI_SHADER_STAGE_MISS_BIT_KHR:
      {
        crude_gfx_rhi_ray_tracing_shader_group_create_info *shader_group_info = &shader_state->shader_group_info[ compiled_shaders_count ];
        *shader_group_info = CRUDE_COMPOUNT_EMPTY( crude_gfx_rhi_ray_tracing_shader_group_create_info );
        shader_group_info->type = CRUDE_GFX_RHI_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR;
        shader_group_info->general_shader = compiled_shaders_count;
        shader_group_info->closest_hit_shader = CRUDE_GFX_RHI_SHADER_UNUSED_KHR;
        shader_group_info->any_hit_shader = CRUDE_GFX_RHI_SHADER_UNUSED_KHR;
        shader_group_info->intersection_shader = CRUDE_GFX_RHI_SHADER_UNUSED_KHR;
        shader_state->pipeline_type = CRUDE_GFX_PIPELINE_TYPE_RAY_TRACING;
        break;
      }
    }
#endif /* CRUDE_GFX_RAY_TRACING_ENABLED */

    crude_gfx_rhi_set_shader_module_debug_name( &gpu->rhi_device, shader_state->shader_stage_info[ compiled_shaders_count ].rhi_module, creation->name );

    vk_reflect_shader_( gpu, rhi_creation_info.code, rhi_creation_info.code_size, &shader_state->reflect );

    crude_heap_allocator_deallocate( gpu->allocator, code );
  }

  crude_string_buffer_deinitialize( &temporary_string_buffer );
  
  shader_state->active_shaders = compiled_shaders_count;
  shader_state->name = creation->name;

  creation_failed = ( compiled_shaders_count != creation->stages_count );
  if ( creation_failed )
  {
    crude_gfx_destroy_shader_state( gpu, shader_state_handle );
    
    CRUDE_LOG_ERROR( CRUDE_CHANNEL_GRAPHICS, "Error in creation of shader %s. Dumping all shader informations.", creation->name );
    //for ( compiled_shaders_count = 0; compiled_shaders_count < creation->stages_count; ++compiled_shaders_count )
    //{
    //  crude_gfx_shader_stage const *stage = &creation->stages[ compiled_shaders_count ];
    //  CRUDE_LOG_ERROR( CRUDE_CHANNEL_GRAPHICS, "%u:\n%s", stage->type, stage->code );
    //}
    return CRUDE_GFX_SHADER_STATE_HANDLE_INVALID;
  }

  return shader_state_handle;
}

void
crude_gfx_destroy_shader_state
(
  _In_ crude_gfx_device                                   *gpu,
  _In_ crude_gfx_shader_state_handle                       handle
)
{
  if ( handle.index >= gpu->shaders.pool_size )
  {
    CRUDE_LOG_ERROR( CRUDE_CHANNEL_GRAPHICS, "Trying to free invalid shader state %u", handle.index );
    return;
  }
  crude_gfx_resource_update shader_state_update_event = { 
    .type          = CRUDE_GFX_RESOURCE_DELETION_TYPE_SHADER_STATE,
    .handle        = handle.index,
    .current_frame = gpu->current_frame };
  crude_gfx_push_resource_update_( gpu, &shader_state_update_event );
}

void
crude_gfx_destroy_shader_state_instant
(
  _In_ crude_gfx_device                                   *gpu,
  _In_ crude_gfx_shader_state_handle                       handle
)
{
  crude_gfx_shader_state *shader_state = crude_gfx_access_shader_state( gpu, handle );
  if ( shader_state )
  {
    CRUDE_ARRAY_DEINITIALIZE( shader_state->reflect.input.vertex_attributes );
    CRUDE_ARRAY_DEINITIALIZE( shader_state->reflect.input.vertex_streams );
    for ( uint32 i = 0; i < shader_state->active_shaders; ++i )
    {
      crude_gfx_rhi_destroy_shader_module( &gpu->rhi_device, shader_state->shader_stage_info[ i ].rhi_module );
    }
  }
  crude_gfx_release_shader_state( gpu, handle );
}

crude_gfx_render_pass_handle
crude_gfx_create_render_pass
(
  _In_ crude_gfx_device                                   *gpu,
  _In_ crude_gfx_render_pass_creation const               *creation
)
{
  crude_gfx_render_pass_handle handle = crude_gfx_obtain_render_pass( gpu);
  if ( CRUDE_RESOURCE_HANDLE_IS_INVALID( handle ) )
  {
    return handle;
  }
  
  crude_gfx_render_pass *render_pass = crude_gfx_access_render_pass( gpu, handle );
  render_pass->num_render_targets = creation->num_render_targets;
  render_pass->name               = creation->name;
  
  for ( uint32 i = 0 ; i < creation->num_render_targets; ++i )
  {
    render_pass->output.color_final_layouts[ i ] = creation->color_final_layouts[ i ];
    render_pass->output.color_formats[ i ] = creation->color_formats[ i ];
    render_pass->output.color_operations[ i ] = creation->color_operations[ i ];
    ++render_pass->output.num_color_formats;
  }
  if ( creation->depth_stencil_format != CRUDE_GFX_RHI_FORMAT_UNDEFINED )
  {
    render_pass->output.depth_stencil_final_layout = creation->depth_stencil_final_layout;
    render_pass->output.depth_stencil_format = creation->depth_stencil_format;
  }

  render_pass->output.depth_operation = creation->depth_operation;
  render_pass->output.stencil_operation = creation->stencil_operation;
  
  return handle;
}

void
crude_gfx_destroy_render_pass
(
  _In_ crude_gfx_device                                   *gpu,
  _In_ crude_gfx_render_pass_handle                        handle
)
{
  if ( handle.index >= gpu->render_passes.pool_size )
  {
    CRUDE_LOG_ERROR( CRUDE_CHANNEL_GRAPHICS, "Trying to free invalid render pass %u", handle.index );
    return;
  }
  crude_gfx_resource_update render_pass_update_event = { 
    .type          = CRUDE_GFX_RESOURCE_DELETION_TYPE_RENDER_PASS,
    .handle        = handle.index,
    .current_frame = gpu->current_frame };
  crude_gfx_push_resource_update_( gpu, &render_pass_update_event );
}

void
crude_gfx_destroy_render_pass_instant
(
  _In_ crude_gfx_device                                   *gpu,
  _In_ crude_gfx_render_pass_handle                        handle
)
{
  crude_gfx_render_pass *render_pass = crude_gfx_access_render_pass( gpu, handle );
  crude_gfx_release_render_pass( gpu, handle );
}

crude_gfx_pipeline_handle
crude_gfx_create_pipeline
(
  _In_ crude_gfx_device                                   *gpu,
  _In_ crude_gfx_pipeline_creation const                  *creation
)
{
  crude_gfx_pipeline                                      *pipeline;
  crude_gfx_shader_state                                  *shader_state;
  crude_gfx_rhi_pipeline_layout_create_info                pipeline_layout_creation;
  crude_gfx_pipeline_handle                                pipeline_handle;
  crude_gfx_shader_state_handle                            shader_state_handle;

  pipeline_handle = crude_gfx_obtain_pipeline( gpu );
  if ( CRUDE_RESOURCE_HANDLE_IS_INVALID( pipeline_handle ) )
  {
    return pipeline_handle;
  }

  shader_state_handle = crude_gfx_create_shader_state( gpu, &creation->shaders );
  if ( CRUDE_RESOURCE_HANDLE_IS_INVALID( shader_state_handle ) )
  {
    crude_gfx_release_pipeline( gpu, pipeline_handle );
    return CRUDE_GFX_PIPELINE_HANDLE_INVALID;
  }

  shader_state = crude_gfx_access_shader_state( gpu, shader_state_handle );
  
  pipeline = crude_gfx_access_pipeline( gpu, pipeline_handle );
  
  crude_string_copy( pipeline->name, creation->name, sizeof( pipeline->name ) );
  pipeline->shader_state = shader_state_handle;
  
  pipeline_layout_creation = CRUDE_COMPOUNT_EMPTY( crude_gfx_rhi_pipeline_layout_create_info );
  
  pipeline_layout_creation.set_layout_count = shader_state->reflect.descriptor.sets_count;
  for ( uint32 i = 0; i < shader_state->reflect.descriptor.sets_count; ++i )
  {
    /* First set for bindless */
    if ( i == CRUDE_BINDLESS_DESCRIPTOR_SET_INDEX )
    {
      crude_gfx_descriptor_set_layout *bindless_descriptor_set_layout = crude_gfx_access_descriptor_set_layout( gpu, gpu->bindless_descriptor_set_layout_handle );
      pipeline_layout_creation.set_layouts[ i ] = bindless_descriptor_set_layout->rhi_descriptor_set_layout;
      pipeline->descriptor_set_layout_handle[ i ] = CRUDE_GFX_DESCRIPTOR_SET_LAYOUT_HANDLE_INVALID;
    }
    else
    {
      pipeline->descriptor_set_layout_handle[ i ] = crude_gfx_create_descriptor_set_layout( gpu, &shader_state->reflect.descriptor.sets[ i ] );
      crude_gfx_descriptor_set_layout *descriptor_set_layout = crude_gfx_access_descriptor_set_layout( gpu, pipeline->descriptor_set_layout_handle[ i ] );
      pipeline_layout_creation.set_layouts[ i ] = descriptor_set_layout->rhi_descriptor_set_layout;
      pipeline->descriptor_set_layout[ i ] = descriptor_set_layout;
    }
  }

  pipeline_layout_creation.has_push_constant_range = shader_state->reflect.push_constant.stride;
  pipeline_layout_creation.push_constant_range.offset = 0;
  pipeline_layout_creation.push_constant_range.size = shader_state->reflect.push_constant.stride;
  pipeline_layout_creation.push_constant_range.stage_flags = CRUDE_GFX_RHI_SHADER_STAGE_ALL;

  crude_gfx_rhi_create_pipeline_layout( &gpu->rhi_device, &pipeline_layout_creation, &pipeline->rhi_pipeline_layout );

  crude_gfx_rhi_set_pipeline_layout_debug_name(  &gpu->rhi_device, pipeline->rhi_pipeline_layout, creation->name );

  pipeline->num_active_layouts = shader_state->reflect.descriptor.sets_count;

  if ( shader_state->pipeline_type == CRUDE_GFX_PIPELINE_TYPE_CLASSIC || shader_state->pipeline_type == CRUDE_GFX_PIPELINE_TYPE_TASK )
  {
    crude_gfx_vertex_stream const                         *vertex_streams;
    crude_gfx_vertex_attribute const                      *vertex_attributes;
    crude_gfx_rhi_pipeline_viewport_state_create_info      rhi_viewport_creation;
    crude_gfx_rhi_pipeline_input_assembly_state_create_info rhi_input_assembly_creation;
    crude_gfx_rhi_pipeline_vertex_input_state_create_info  rhi_vertex_input_creation;
    crude_gfx_rhi_pipeline_rasterization_state_create_info rhi_rasterization_creation;
    crude_gfx_rhi_pipeline_multisample_state_create_info   rhi_multisample_state;
    crude_gfx_rhi_pipeline_depth_stencil_state_create_info rhi_depth_creation;
    crude_gfx_rhi_pipeline_color_blend_state_create_info   rhi_color_blending_creation;
    crude_gfx_rhi_pipeline_rendering_create_info           rhi_rendering_creation;
    uint32                                                 vertex_attributes_num, vertex_streams_num;

    if ( creation->relfect_vertex_input )
    {
      vertex_attributes = shader_state->reflect.input.vertex_attributes;
      vertex_streams = shader_state->reflect.input.vertex_streams;
      vertex_attributes_num = CRUDE_ARRAY_LENGTH( vertex_attributes );
      vertex_streams_num = CRUDE_ARRAY_LENGTH( vertex_streams );
      CRUDE_ASSERT( CRUDE_COUNTOF( rhi_vertex_input_creation.vertex_attribute_descriptions ) >= vertex_attributes_num );
      CRUDE_ASSERT( CRUDE_COUNTOF( rhi_vertex_input_creation.vertex_binding_descriptions ) >= vertex_streams_num );
    }
    else
    {
      vertex_attributes = creation->vertex_attributes;
      vertex_streams = creation->vertex_streams;
      vertex_attributes_num = creation->vertex_attributes_num;
      vertex_streams_num = creation->vertex_streams_num;
    }
    
    rhi_vertex_input_creation = CRUDE_COMPOUNT_EMPTY( crude_gfx_rhi_pipeline_vertex_input_state_create_info );
    if ( vertex_attributes_num )
    {
      rhi_vertex_input_creation.vertex_attribute_description_count = vertex_attributes_num;
      for ( uint32 i = 0; i < vertex_attributes_num; ++i )
      {
        rhi_vertex_input_creation.vertex_attribute_descriptions[ i ] = CRUDE_COMPOUNT_EMPTY( crude_gfx_rhi_pipeline_vertex_input_attribute_description );
        rhi_vertex_input_creation.vertex_attribute_descriptions[ i ].location = vertex_attributes[ i ].location;
        rhi_vertex_input_creation.vertex_attribute_descriptions[ i ].binding = vertex_attributes[ i ].binding;
        rhi_vertex_input_creation.vertex_attribute_descriptions[ i ].format = crude_gfx_to_vertex_format( vertex_attributes[ i ].format );
        rhi_vertex_input_creation.vertex_attribute_descriptions[ i ].offset = vertex_attributes[ i ].offset;
      }
    }
    else
    {
      rhi_vertex_input_creation.vertex_attribute_description_count = 0u;
    }

    if ( vertex_streams_num )
    {
      rhi_vertex_input_creation.vertex_binding_description_count = vertex_streams_num;
      for ( uint32 i = 0; i < vertex_streams_num; ++i )
      {
        rhi_vertex_input_creation.vertex_binding_descriptions[ i ] = CRUDE_COMPOUNT_EMPTY( crude_gfx_rhi_vertex_input_binding_description );
        rhi_vertex_input_creation.vertex_binding_descriptions[ i ].binding = vertex_streams[ i ].binding;
        rhi_vertex_input_creation.vertex_binding_descriptions[ i ].stride = vertex_streams[ i ].stride;
        rhi_vertex_input_creation.vertex_binding_descriptions[ i ].input_rate = vertex_streams[ i ].input_rate == CRUDE_GFX_VERTEX_INPUT_RATE_PER_VERTEX ? CRUDE_GFX_RHI_VERTEX_INPUT_RATE_VERTEX : CRUDE_GFX_RHI_VERTEX_INPUT_RATE_INSTANCE;
      }
    }
    else
    {
      rhi_vertex_input_creation.vertex_binding_description_count = 0u;
    }
    
    rhi_input_assembly_creation = CRUDE_COMPOUNT_EMPTY( crude_gfx_rhi_pipeline_input_assembly_state_create_info );
    rhi_input_assembly_creation.topology = creation->topology;
    rhi_input_assembly_creation.primitive_restart_enable = false;
    
    rhi_color_blending_creation = CRUDE_COMPOUNT_EMPTY( crude_gfx_rhi_pipeline_color_blend_state_create_info );
    
    rhi_color_blending_creation.attachments_count = creation->blend_state.active_states ? creation->blend_state.active_states : creation->render_pass_output.num_color_formats;
    CRUDE_ASSERT( rhi_color_blending_creation.attachments_count < CRUDE_COUNTOF( rhi_color_blending_creation.attachments ) );

    if ( creation->blend_state.active_states )
    {
      for ( uint32 i = 0; i < creation->blend_state.active_states; ++i )
      {
        crude_gfx_blend_state const *blend_state = &creation->blend_state.blend_states[i];
    
        rhi_color_blending_creation.attachments[ i ] = CRUDE_COMPOUNT_EMPTY( crude_gfx_rhi_pipeline_color_blend_attachment_state );

        rhi_color_blending_creation.attachments[ i ].color_write_mask = CRUDE_GFX_RHI_COLOR_COMPONENT_R_BIT | CRUDE_GFX_RHI_COLOR_COMPONENT_G_BIT | CRUDE_GFX_RHI_COLOR_COMPONENT_B_BIT | CRUDE_GFX_RHI_COLOR_COMPONENT_A_BIT;
        rhi_color_blending_creation.attachments[ i ].blend_enable = blend_state->blend_enabled;
        rhi_color_blending_creation.attachments[ i ].src_color_blend_factor = blend_state->source_color;
        rhi_color_blending_creation.attachments[ i ].dst_color_blend_factor = blend_state->destination_color;
        rhi_color_blending_creation.attachments[ i ].color_blend_op = blend_state->color_operation;
        
        if ( blend_state->separate_blend )
        {
          rhi_color_blending_creation.attachments[ i ].src_alpha_blend_factor = blend_state->source_alpha;
          rhi_color_blending_creation.attachments[ i ].dst_alpha_blend_factor = blend_state->destination_alpha;
          rhi_color_blending_creation.attachments[ i ].alpha_blend_op = blend_state->alpha_operation;
        }
        else
        {
          rhi_color_blending_creation.attachments[ i ].src_alpha_blend_factor = blend_state->source_color;
          rhi_color_blending_creation.attachments[ i ].dst_alpha_blend_factor = blend_state->destination_color;
          rhi_color_blending_creation.attachments[ i ].alpha_blend_op = blend_state->color_operation;
        }
      }
    }
    else
    {
      for ( uint32 i = 0; i < creation->render_pass_output.num_color_formats; ++i )
      {
        rhi_color_blending_creation.attachments[ i ] = CRUDE_COMPOUNT_EMPTY( crude_gfx_rhi_pipeline_color_blend_attachment_state );
        rhi_color_blending_creation.attachments[ i ].blend_enable = false;
        rhi_color_blending_creation.attachments[ i ].color_write_mask = CRUDE_GFX_RHI_COLOR_COMPONENT_R_BIT | CRUDE_GFX_RHI_COLOR_COMPONENT_G_BIT | CRUDE_GFX_RHI_COLOR_COMPONENT_B_BIT | CRUDE_GFX_RHI_COLOR_COMPONENT_A_BIT;
      }
    }
    
    rhi_color_blending_creation.logic_op_enable = false;
    rhi_color_blending_creation.logic_op = CRUDE_GFX_RHI_LOGIC_OP_COPY;
    rhi_color_blending_creation.blend_constants[ 0 ] = 0.0f;
    rhi_color_blending_creation.blend_constants[ 1 ] = 0.0f;
    rhi_color_blending_creation.blend_constants[ 2 ] = 0.0f;
    rhi_color_blending_creation.blend_constants[ 3 ] = 0.0f;
    
    rhi_depth_creation = CRUDE_COMPOUNT_EMPTY( crude_gfx_rhi_pipeline_depth_stencil_state_create_info );
    rhi_depth_creation.depth_test_enable = creation->depth_stencil.depth_enable;
    rhi_depth_creation.depth_write_enable = creation->depth_stencil.depth_write_enable;
    rhi_depth_creation.depth_compare_op = creation->depth_stencil.depth_comparison;
    rhi_depth_creation.stencil_test_enable = creation->depth_stencil.stencil_enable;
    
    if ( creation->depth_stencil.stencil_enable )
    {
      CRUDE_ABORT( CRUDE_CHANNEL_GRAPHICS, "TODO creation->depth_stencil.stencil_enable" );
    }
    
    rhi_multisample_state = CRUDE_COMPOUNT_EMPTY( crude_gfx_rhi_pipeline_multisample_state_create_info );
    rhi_multisample_state.rasterization_samples = creation->multisample.enabled ? CRUDE_GFX_SAMPLE_COUNT : CRUDE_GFX_RHI_SAMPLE_COUNT_1_BIT;
    rhi_multisample_state.alpha_to_coverage_enable = false;
    rhi_multisample_state.alpha_to_one_enable = false;
    if ( creation->multisample.enabled )
    {
#if CRUDE_GFX_SAMPLE_RATE_SHADING
      rhi_multisample_state.sample_shading_enable = true;
      rhi_multisample_state.min_sample_shading = 0.2f;
#endif
    }
    else
    {
      rhi_multisample_state.sample_shading_enable = false;
      rhi_multisample_state.min_sample_shading = 1.0f;
    }
    
    rhi_rasterization_creation = CRUDE_COMPOUNT_EMPTY( crude_gfx_rhi_pipeline_rasterization_state_create_info );
    rhi_rasterization_creation.depth_clamp_enable = false;
    rhi_rasterization_creation.rasterizer_discard_enable = false;
    rhi_rasterization_creation.polygon_mode = CRUDE_GFX_RHI_POLYGON_MODE_FILL;
    rhi_rasterization_creation.cull_mode = creation->rasterization.cull_mode;
    rhi_rasterization_creation.front_face = creation->rasterization.front;
    rhi_rasterization_creation.depth_bias_enable = false;
    rhi_rasterization_creation.depth_bias_constant_factor = 0.0f;
    rhi_rasterization_creation.depth_bias_clamp = 0.0f;
    rhi_rasterization_creation.depth_bias_slope_factor = 0.0f;
    rhi_rasterization_creation.line_width = 1.0f;
    
    rhi_viewport_creation = CRUDE_COMPOUNT_EMPTY( crude_gfx_rhi_pipeline_viewport_state_create_info ); 
    rhi_viewport_creation.viewport_count = 1;
    rhi_viewport_creation.scissor_count = 1;
    
    rhi_rendering_creation = CRUDE_COMPOUNT_EMPTY( crude_gfx_rhi_pipeline_rendering_create_info );
    rhi_rendering_creation.view_mask = 0;
    rhi_rendering_creation.color_attachment_count = creation->render_pass_output.num_color_formats;
    rhi_rendering_creation.color_attachment_formats = creation->render_pass_output.num_color_formats > 0 ? creation->render_pass_output.color_formats : nullptr;
    rhi_rendering_creation.depth_attachment_format = creation->render_pass_output.depth_stencil_format;
    rhi_rendering_creation.stencil_attachment_format = CRUDE_GFX_RHI_FORMAT_UNDEFINED;
    
    if ( shader_state->pipeline_type == CRUDE_GFX_PIPELINE_TYPE_CLASSIC )
    {
      crude_gfx_rhi_classic_pipeline_create_info           rhi_pipeline_creation;

      rhi_pipeline_creation = CRUDE_COMPOUNT_EMPTY( crude_gfx_rhi_classic_pipeline_create_info );
      rhi_pipeline_creation.stage_count = shader_state->active_shaders;
      rhi_pipeline_creation.stages = shader_state->shader_stage_info;
      rhi_pipeline_creation.vertex_input_state = &rhi_vertex_input_creation;
      rhi_pipeline_creation.input_assembly_state = &rhi_input_assembly_creation;
      rhi_pipeline_creation.viewport_state = &rhi_viewport_creation;
      rhi_pipeline_creation.rasterization_state = &rhi_rasterization_creation;
      rhi_pipeline_creation.multisample_state = &rhi_multisample_state;
      rhi_pipeline_creation.depth_stencil_state = &rhi_depth_creation;
      rhi_pipeline_creation.color_blend_state = &rhi_color_blending_creation;
      rhi_pipeline_creation.rendering_state = &rhi_rendering_creation;
      rhi_pipeline_creation.pipeline_layout = pipeline->rhi_pipeline_layout;
      
      crude_gfx_rhi_create_classic_pipeline( &gpu->rhi_device, &rhi_pipeline_creation, &pipeline->rhi_pipeline );
    }
    else if ( shader_state->pipeline_type == CRUDE_GFX_PIPELINE_TYPE_TASK )
    {
      crude_gfx_rhi_task_pipeline_create_info              rhi_pipeline_creation;

      rhi_pipeline_creation = CRUDE_COMPOUNT_EMPTY( crude_gfx_rhi_task_pipeline_create_info );
      rhi_pipeline_creation.stage_count = shader_state->active_shaders;
      rhi_pipeline_creation.stages = shader_state->shader_stage_info;
      rhi_pipeline_creation.input_assembly_state = &rhi_input_assembly_creation;
      rhi_pipeline_creation.viewport_state = &rhi_viewport_creation;
      rhi_pipeline_creation.rasterization_state = &rhi_rasterization_creation;
      rhi_pipeline_creation.multisample_state = &rhi_multisample_state;
      rhi_pipeline_creation.depth_stencil_state = &rhi_depth_creation;
      rhi_pipeline_creation.color_blend_state = &rhi_color_blending_creation;
      rhi_pipeline_creation.rendering_state = &rhi_rendering_creation;
      rhi_pipeline_creation.pipeline_layout = pipeline->rhi_pipeline_layout;
      
      crude_gfx_rhi_create_task_pipeline( &gpu->rhi_device, &rhi_pipeline_creation, &pipeline->rhi_pipeline );
    }
    
    pipeline->bind_point = CRUDE_GFX_RHI_PIPELINE_BIND_POINT_GRAPHICS;
  }
  else if ( shader_state->pipeline_type == CRUDE_GFX_PIPELINE_TYPE_COMPUTE )
  {
    crude_gfx_rhi_compute_pipeline_create_info             rhi_pipeline_creation;

    rhi_pipeline_creation = CRUDE_COMPOUNT_EMPTY( crude_gfx_rhi_compute_pipeline_create_info );
    rhi_pipeline_creation.stage = shader_state->shader_stage_info[ 0 ];
    rhi_pipeline_creation.pipeline_layout = pipeline->rhi_pipeline_layout;

    crude_gfx_rhi_create_compute_pipeline( &gpu->rhi_device, &rhi_pipeline_creation, &pipeline->rhi_pipeline );
    
    pipeline->bind_point = CRUDE_GFX_RHI_PIPELINE_BIND_POINT_COMPUTE;
  }
  else if ( shader_state->pipeline_type == CRUDE_GFX_PIPELINE_TYPE_RAY_TRACING )
  {
#if CRUDE_GFX_RAY_TRACING_ENABLED
    uint8                                                 *shader_binding_table_data;
    crude_gfx_rhi_ray_tracing_pipeline_create_info         rhi_pipeline_creation;
    crude_gfx_buffer_creation                              shader_binding_table_creation;
    uint64                                                 shader_binding_table_size, group_count, group_handle_size;
    
    rhi_pipeline_creation = CRUDE_COMPOUNT_EMPTY( crude_gfx_rhi_ray_tracing_pipeline_create_info );
    rhi_pipeline_creation.stage_count = shader_state->active_shaders;
    rhi_pipeline_creation.stages = shader_state->shader_stage_info;
    rhi_pipeline_creation.group_count = shader_state->active_shaders;
    rhi_pipeline_creation.groups = shader_state->shader_group_info;
    rhi_pipeline_creation.max_pipeline_ray_recursion_depth = 1;
    rhi_pipeline_creation.pipeline_layout = pipeline->rhi_pipeline_layout;
    crude_gfx_rhi_create_ray_tracing_pipeline( &gpu->rhi_device, &rhi_pipeline_creation, &pipeline->rhi_pipeline );

    group_count = shader_state->active_shaders;
    group_handle_size = gpu->ray_tracing_pipeline_properties.shader_group_handle_size;
    shader_binding_table_size = group_handle_size * group_count;

    CRUDE_ARRAY_INITIALIZE_WITH_LENGTH( shader_binding_table_data, shader_binding_table_size, crude_heap_allocator_pack( gpu->allocator ) );

    crude_gfx_rhi_get_ray_tracing_shader_group_handles( &gpu->rhi_device, pipeline->rhi_pipeline, 0, shader_state->active_shaders, shader_binding_table_size, shader_binding_table_data );
    
    shader_binding_table_creation = crude_gfx_buffer_creation_empty( );
    shader_binding_table_creation.type_flags = CRUDE_GFX_RHI_BUFFER_USAGE_SHADER_BINDING_TABLE_BIT_KHR | CRUDE_GFX_RHI_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT_KHR;
    shader_binding_table_creation.usage = CRUDE_GFX_RESOURCE_USAGE_TYPE_IMMUTABLE;
    shader_binding_table_creation.size = group_handle_size;

    shader_binding_table_creation.initial_data = shader_binding_table_data;
    crude_string_copy( shader_binding_table_creation.name, "shader_binding_table_raygen", sizeof( shader_binding_table_creation.name ) );
    pipeline->shader_binding_table_raygen = crude_gfx_create_buffer( gpu, &shader_binding_table_creation );
    
    shader_binding_table_creation.initial_data = shader_binding_table_data + group_handle_size;
    crude_string_copy( shader_binding_table_creation.name, "shader_binding_table_hit", sizeof( shader_binding_table_creation.name ) );
    pipeline->shader_binding_table_hit = crude_gfx_create_buffer( gpu, &shader_binding_table_creation );
    
    shader_binding_table_creation.initial_data = shader_binding_table_data + ( 2 * group_handle_size );
    crude_string_copy( shader_binding_table_creation.name, "shader_binding_table_miss", sizeof( shader_binding_table_creation.name ) );
    pipeline->shader_binding_table_miss = crude_gfx_create_buffer( gpu, &shader_binding_table_creation );
    
    CRUDE_ARRAY_DEINITIALIZE( shader_binding_table_data );

    pipeline->bind_point = CRUDE_GFX_RHI_PIPELINE_BIND_POINT_RAY_TRACING_KHR;
#endif /* CRUDE_GFX_RAY_TRACING_ENABLED */
  }

  crude_gfx_rhi_set_pipeline_debug_name( &gpu->rhi_device, pipeline->rhi_pipeline, pipeline->name );

  return pipeline_handle;
}

void
crude_gfx_destroy_pipeline
(
  _In_ crude_gfx_device                                   *gpu,
  _In_ crude_gfx_pipeline_handle                           handle
)
{
  if ( handle.index >= gpu->pipelines.pool_size )
  {
    CRUDE_LOG_ERROR( CRUDE_CHANNEL_GRAPHICS, "Trying to free invalid pipeline %u", handle.index );
    return;
  }
  crude_gfx_resource_update pipeline_update_event = { 
    .type          = CRUDE_GFX_RESOURCE_DELETION_TYPE_PIPELINE,
    .handle        = handle.index,
    .current_frame = gpu->current_frame };
  crude_gfx_push_resource_update_( gpu, &pipeline_update_event );

  crude_gfx_pipeline *pipeline = crude_gfx_access_pipeline( gpu, handle );
  crude_gfx_destroy_shader_state( gpu, pipeline->shader_state );
}

void
crude_gfx_destroy_pipeline_instant
(
  _In_ crude_gfx_device                                   *gpu,
  _In_ crude_gfx_pipeline_handle                           handle
)
{
  crude_gfx_pipeline *pipeline = crude_gfx_access_pipeline( gpu, handle );

  if ( pipeline )
  {
    for ( uint32 i = 0; i < pipeline->num_active_layouts; ++i )
    {
      if ( i != CRUDE_BINDLESS_DESCRIPTOR_SET_INDEX )
      {
        crude_gfx_destroy_descriptor_set_layout( gpu, pipeline->descriptor_set_layout_handle[ i ] );
      }
    }
    
#if CRUDE_GFX_RAY_TRACING_ENABLED
    if ( pipeline->bind_point == CRUDE_GFX_RHI_PIPELINE_BIND_POINT_RAY_TRACING_KHR )
    {
      crude_gfx_destroy_buffer( gpu, pipeline->shader_binding_table_raygen );
      crude_gfx_destroy_buffer( gpu, pipeline->shader_binding_table_hit );
      crude_gfx_destroy_buffer( gpu, pipeline->shader_binding_table_miss );
    }
#endif /* CRUDE_GFX_RAY_TRACING_ENABLED */

    crude_gfx_rhi_destroy_pipeline( &gpu->rhi_device, pipeline->rhi_pipeline );
    crude_gfx_rhi_destroy_pipeline_layout( &gpu->rhi_device, pipeline->rhi_pipeline_layout );
  }

  crude_gfx_release_pipeline( gpu, handle );
}

crude_gfx_buffer_handle
crude_gfx_create_buffer
(
  _In_ crude_gfx_device                                   *gpu,
  _In_ crude_gfx_buffer_creation const                    *creation
)
{
  crude_gfx_buffer                                        *buffer;
  crude_gfx_buffer_handle                                  handle;
  crude_gfx_rhi_buffer_create_info                         rhi_creation;

  handle = crude_gfx_obtain_buffer( gpu );
  if ( CRUDE_RESOURCE_HANDLE_IS_INVALID( handle ) )
  {
    return handle;
  }

  //CRUDE_LOG_INFO( CRUDE_CHANNEL_GRAPHICS, "Allocate buffer %s", creation->name ? creation->name : "Unknown" );

  buffer = crude_gfx_access_buffer( gpu, handle );
  crude_string_copy( buffer->name, creation->name, sizeof( buffer->name ) );
  buffer->size = creation->size;
  buffer->type_flags = creation->type_flags;
  buffer->usage = creation->usage;
  buffer->handle = handle;
  buffer->ready = true;

  rhi_creation.size = creation->size > 0 ? creation->size : 1;
  rhi_creation.usage = CRUDE_GFX_RHI_BUFFER_USAGE_TRANSFER_SRC_BIT | CRUDE_GFX_RHI_BUFFER_USAGE_TRANSFER_DST_BIT | creation->type_flags;
  rhi_creation.persistent = creation->persistent;
  rhi_creation.device_only = creation->device_only;
  
  crude_gfx_rhi_create_buffer( &gpu->rhi_device, &rhi_creation, &buffer->rhi_buffer );
  
  crude_gfx_rhi_set_buffer_debug_name(  &gpu->rhi_device, buffer->rhi_buffer, creation->name );

  if ( creation->initial_data )
  {
    void* data;
    crude_gfx_rhi_map_buffer( &gpu->rhi_device, buffer->rhi_buffer, &data );
    memcpy( data, creation->initial_data, creation->size );
    crude_gfx_rhi_unmap_buffer( &gpu->rhi_device, buffer->rhi_buffer );
  }
  
  if ( creation->persistent )
  {
    buffer->mapped_data = CRUDE_CAST( uint8*, crude_gfx_rhi_get_buffer_mapped_data( buffer->rhi_buffer ) );
  }

  return handle;
}

void
crude_gfx_destroy_buffer
(
  _In_ crude_gfx_device                                   *gpu,
  _In_ crude_gfx_buffer_handle                             handle
)
{
  mtx_lock( &gpu->texture_update_mutex );
  mtx_unlock( &gpu->texture_update_mutex );

  if ( handle.index >= gpu->buffers.pool_size )
  {
    CRUDE_LOG_ERROR( CRUDE_CHANNEL_GRAPHICS, "Trying to free invalid buffer %u", handle.index );
    return;
  }
  
  crude_gfx_resource_update buffer_update_event = { 
    .type          = CRUDE_GFX_RESOURCE_DELETION_TYPE_BUFFER,
    .handle        = handle.index,
    .current_frame = gpu->current_frame
  };
  crude_gfx_push_resource_update_( gpu, &buffer_update_event );
}

void
crude_gfx_destroy_buffer_instant
(
  _In_ crude_gfx_device                                   *gpu,
  _In_ crude_gfx_buffer_handle                             handle
)
{
  crude_gfx_buffer *buffer = crude_gfx_access_buffer( gpu, handle );
  
  //CRUDE_LOG_INFO( CRUDE_CHANNEL_GRAPHICS, "Destroy buffer %s", buffer->name ? buffer->name : "Unknown" );
  buffer->name[ 0 ] = 0;

  if ( buffer )
  {
    crude_gfx_rhi_destroy_buffer( &gpu->rhi_device, buffer->rhi_buffer );
  }

  crude_gfx_release_buffer( gpu, handle );
}

crude_gfx_descriptor_set_layout_handle
crude_gfx_create_descriptor_set_layout
(
  _In_ crude_gfx_device                                   *gpu,
  _In_ crude_gfx_descriptor_set_layout_creation const     *creation
)
{
  crude_gfx_descriptor_set_layout                       *descriptor_set_layout;
  uint8                                                 *memory;
  crude_gfx_descriptor_set_layout_handle                 descriptor_set_layout_handle;
  crude_gfx_rhi_descriptor_set_layout_create_info        rhi_creation;
  uint32                                                 used_bindings;

  descriptor_set_layout_handle = crude_gfx_obtain_descriptor_set_layout( gpu );
  if ( CRUDE_RESOURCE_HANDLE_IS_INVALID( descriptor_set_layout_handle ) )
  {
    return descriptor_set_layout_handle;
  }
  
  descriptor_set_layout = crude_gfx_access_descriptor_set_layout( gpu, descriptor_set_layout_handle );
  
  memory = CRUDE_CAST( uint8*, crude_heap_allocator_allocate( gpu->allocator, ( sizeof( crude_gfx_rhi_descriptor_set_layout_binding ) + sizeof( crude_gfx_descriptor_binding ) ) * creation->num_bindings ) );
  descriptor_set_layout->num_bindings = creation->num_bindings;
  descriptor_set_layout->bindings     = CRUDE_CAST( crude_gfx_descriptor_binding*, memory );
  descriptor_set_layout->rhi_bindings = CRUDE_CAST( crude_gfx_rhi_descriptor_set_layout_binding*, memory + sizeof( crude_gfx_descriptor_binding ) * creation->num_bindings );
  descriptor_set_layout->handle       = descriptor_set_layout_handle;
  descriptor_set_layout->set_index    = creation->set_index;
  descriptor_set_layout->bindless = creation->bindless;

  used_bindings = 0;
  for ( uint32 i = 0; i < creation->num_bindings; ++i )
  {
    crude_gfx_descriptor_binding                        *binding;
    crude_gfx_descriptor_set_layout_binding const       *input_binding;
    crude_gfx_rhi_descriptor_set_layout_binding         *rhi_binding;
  
    binding = &descriptor_set_layout->bindings[ i ];
    memset( binding, 0, sizeof( *binding ) );
  
    input_binding = &creation->bindings[ i ];
    binding->start = ( input_binding->start == UINT16_MAX ) ? i : input_binding->start;
    binding->count = input_binding->count;
    binding->type = input_binding->type;
    binding->set = descriptor_set_layout->set_index;
    
    rhi_binding = &descriptor_set_layout->rhi_bindings[ used_bindings++ ];
    rhi_binding->binding = binding->start;
    rhi_binding->descriptor_type = input_binding->type;
    rhi_binding->descriptor_type = ( rhi_binding->descriptor_type == CRUDE_GFX_RHI_DESCRIPTOR_TYPE_UNIFORM_BUFFER ) ? CRUDE_GFX_RHI_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC : rhi_binding->descriptor_type;
    rhi_binding->descriptor_count = input_binding->count;
    rhi_binding->stage_flags = CRUDE_GFX_RHI_SHADER_STAGE_ALL;

    CRUDE_ASSERT( binding->start >= 0 && binding->start < CRUDE_COUNTOF( descriptor_set_layout->binding_to_index ) );
    descriptor_set_layout->binding_to_index[ binding->start ] = i;
  }

  rhi_creation = CRUDE_COMPOUNT_EMPTY( crude_gfx_rhi_descriptor_set_layout_create_info );
  rhi_creation.binding_count = used_bindings;
  rhi_creation.bindings = descriptor_set_layout->rhi_bindings;
  rhi_creation.bindless = creation->bindless;
  crude_gfx_rhi_create_descriptor_set_layout( &gpu->rhi_device, &rhi_creation, &descriptor_set_layout->rhi_descriptor_set_layout );

  return descriptor_set_layout_handle;
}

void                                      
crude_gfx_destroy_descriptor_set_layout
(                                                   
  _In_ crude_gfx_device                                   *gpu,
  _In_ crude_gfx_descriptor_set_layout_handle              handle
)
{
  if ( handle.index >= gpu->descriptor_set_layouts.pool_size )
  {
    CRUDE_LOG_ERROR( CRUDE_CHANNEL_GRAPHICS, "Trying to free invalid descriptor set layout %u", handle.index );
    return;
  }
  crude_gfx_resource_update descriptor_set_layout_update_event = { 
    .type          = CRUDE_GFX_RESOURCE_DELETION_TYPE_DESCRIPTOR_SET_LAYOUT,
    .handle        = handle.index,
    .current_frame = gpu->current_frame };
  crude_gfx_push_resource_update_( gpu, &descriptor_set_layout_update_event );
}

void
crude_gfx_destroy_descriptor_set_layout_instant
(                                                   
  _In_ crude_gfx_device                                   *gpu,
  _In_ crude_gfx_descriptor_set_layout_handle              handle
)
{
  crude_gfx_descriptor_set_layout *descriptor_set_layout = crude_gfx_access_descriptor_set_layout( gpu, handle );
  crude_heap_allocator_deallocate( gpu->allocator, descriptor_set_layout->bindings );
  crude_gfx_rhi_destroy_descriptor_set_layout( &gpu->rhi_device, descriptor_set_layout->rhi_descriptor_set_layout );
  crude_gfx_release_descriptor_set_layout( gpu, handle );
}


crude_gfx_descriptor_set_handle
crude_gfx_create_descriptor_set
(
  _In_ crude_gfx_device                                   *gpu,
  _In_ crude_gfx_descriptor_set_creation const            *creation
)
{
  crude_gfx_descriptor_set                                *descriptor_set;
  crude_gfx_descriptor_set_layout                         *descriptor_set_layout;
  crude_gfx_rhi_descriptor_image_info                      image_info[ CRUDE_GFX_DESCRIPTORS_PER_SET_MAX_COUNT ];
  crude_gfx_rhi_descriptor_buffer_info                     buffer_info[ CRUDE_GFX_DESCRIPTORS_PER_SET_MAX_COUNT ];
  crude_gfx_rhi_descriptor_acceleration_structure_info     acceleration_info[ CRUDE_GFX_DESCRIPTORS_PER_SET_MAX_COUNT ];
  crude_gfx_rhi_write_descriptor_set                       descriptor_write[ CRUDE_GFX_DESCRIPTORS_PER_SET_MAX_COUNT ];
  crude_gfx_rhi_descriptor_set_create_info                 rhi_creation;
  crude_gfx_descriptor_set_handle                          handle;
  uint32                                                   num_resources;

  handle = crude_gfx_obtain_descriptor_set( gpu );
  if ( CRUDE_RESOURCE_HANDLE_IS_INVALID( handle ) )
  {
    return handle;
  }
  
  descriptor_set = crude_gfx_access_descriptor_set( gpu, handle );
  descriptor_set_layout = crude_gfx_access_descriptor_set_layout( gpu, creation->layout );

  CRUDE_ASSERT( creation->name );
  descriptor_set->name = creation->name;

  num_resources = 0u;
  for ( uint32 i = 0; i < creation->num_resources; i++ )
  {
    crude_gfx_descriptor_binding const *binding = &descriptor_set_layout->bindings[ descriptor_set_layout->binding_to_index[ creation->bindings[ i ] ] ];
    
    if ( binding->set == CRUDE_BINDLESS_DESCRIPTOR_SET_INDEX && ( binding->type == CRUDE_GFX_RHI_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER || binding->type == CRUDE_GFX_RHI_DESCRIPTOR_TYPE_STORAGE_IMAGE ) )
    {
      continue;
    }

    descriptor_write[ i ] = CRUDE_COMPOUNT_EMPTY( crude_gfx_rhi_write_descriptor_set );
    descriptor_write[ i ].dst_binding = binding->start;
    descriptor_write[ i ].dst_array_element = 0u;
    descriptor_write[ i ].descriptor_count = 1u;
    descriptor_write[ i ].descriptor_type = binding->type;
    descriptor_write[ i ].image_info = NULL;
    descriptor_write[ i ].buffer_info = NULL;
    descriptor_write[ i ].acceleration_info = NULL;

    switch ( binding->type )
    {
    case CRUDE_GFX_RHI_DESCRIPTOR_TYPE_UNIFORM_BUFFER:
    {
      crude_gfx_buffer *buffer = crude_gfx_access_buffer( gpu, CRUDE_COMPOUNT( crude_gfx_buffer_handle, { creation->resources[ i ] } ) );
      CRUDE_ASSERT( buffer );

      buffer_info[ i ].buffer = buffer->rhi_buffer;
      buffer_info[ i ].offset = 0;
      buffer_info[ i ].range = buffer->size;

      descriptor_write[ i ].descriptor_type = ( buffer->usage = CRUDE_GFX_RESOURCE_USAGE_TYPE_DYNAMIC ) ? CRUDE_GFX_RHI_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC : CRUDE_GFX_RHI_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
      descriptor_write[ i ].buffer_info = &buffer_info[ i ];
      break;
    }
    case CRUDE_GFX_RHI_DESCRIPTOR_TYPE_STORAGE_BUFFER:
    {
      crude_gfx_buffer *buffer = crude_gfx_access_buffer( gpu, CRUDE_COMPOUNT( crude_gfx_buffer_handle, { creation->resources[ i ] } ) );
      CRUDE_ASSERT( buffer );

      buffer_info[ i ].buffer = buffer->rhi_buffer;

      buffer_info[ i ].offset = 0;
      buffer_info[ i ].range = buffer->size;
      
      descriptor_write[ i ].descriptor_type = CRUDE_GFX_RHI_DESCRIPTOR_TYPE_STORAGE_BUFFER;
      descriptor_write[ i ].buffer_info = &buffer_info[ i ];
      break;
    }
    case CRUDE_GFX_RHI_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER:
    {
      crude_gfx_texture_handle texture_handle = CRUDE_COMPOUNT( crude_gfx_texture_handle, { creation->resources[ i ] } );
      crude_gfx_texture *texture = crude_gfx_access_texture( gpu, texture_handle );
      crude_gfx_texture *parent_texture = CRUDE_RESOURCE_HANDLE_IS_VALID( texture->parent_texture_handle ) ? crude_gfx_access_texture( gpu, texture->parent_texture_handle ) : NULL;

      if ( texture->sampler )
      {
        image_info[ i ].sampler = texture->sampler->rhi_sampler;
      }
      else if ( parent_texture && parent_texture->sampler )
      {
        image_info[ i ].sampler = parent_texture->sampler->rhi_sampler;
      }
      else
      {
        crude_gfx_sampler *default_sampler = crude_gfx_access_sampler( gpu, gpu->default_sampler );
        image_info[ i ].sampler = default_sampler->rhi_sampler;
      }

      image_info[ i ].image_view = texture->rhi_image_view;
      image_info[ i ].image_layout = crude_gfx_rhi_format_has_depth( texture->format ) ? CRUDE_GFX_RHI_IMAGE_LAYOUT_DEPTH_READ_ONLY_OPTIMAL : CRUDE_GFX_RHI_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

      descriptor_write[ i ].descriptor_type = CRUDE_GFX_RHI_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
      descriptor_write[ i ].image_info = &image_info[ i ];
      break;
    }
    case CRUDE_GFX_RHI_DESCRIPTOR_TYPE_STORAGE_IMAGE:
    {
      crude_gfx_texture_handle texture_handle = CRUDE_COMPOUNT( crude_gfx_texture_handle, { creation->resources[ i ] } );
      crude_gfx_texture *texture = crude_gfx_access_texture( gpu, texture_handle );
      image_info[ i ].sampler = crude_gfx_rhi_sampler_empty( );
      image_info[ i ].image_view = texture->rhi_image_view;
      image_info[ i ].image_layout = CRUDE_GFX_RHI_IMAGE_LAYOUT_GENERAL;

      descriptor_write[ i ].descriptor_type = CRUDE_GFX_RHI_DESCRIPTOR_TYPE_STORAGE_IMAGE;
      descriptor_write[ i ].image_info = &image_info[ i ];
      break;
    }
    case CRUDE_GFX_RHI_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR:
    {
#if CRUDE_GFX_RAY_TRACING_ENABLED
      acceleration_info[ i ].acceleration_sturcture = creation->rhi_acceleration_structure;

      descriptor_write[ i ].descriptor_type = CRUDE_GFX_RHI_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR;
      descriptor_write[ i ].acceleration_info = &acceleration_info[ i ];
#endif /* CRUDE_GFX_RAY_TRACING_ENABLED */
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

#if CRUDE_GFX_RAY_TRACING_ENABLED
  descriptor_set->rhi_acceleration_structure = creation->rhi_acceleration_structure;
#endif /* CRUDE_GFX_RAY_TRACING_ENABLED */

  descriptor_set->layout = descriptor_set_layout;
  
  rhi_creation = CRUDE_COMPOUNT_EMPTY( crude_gfx_rhi_descriptor_set_create_info );
  rhi_creation.bindless = descriptor_set_layout->bindless;
  rhi_creation.descriptor_pool = descriptor_set_layout->bindless ? gpu->rhi_bindless_descriptor_pool : gpu->rhi_descriptor_pool;
  rhi_creation.descriptor_set_layout = descriptor_set_layout->rhi_descriptor_set_layout;
  crude_gfx_rhi_create_descriptor_set( &gpu->rhi_device, &rhi_creation, &descriptor_set->rhi_descriptor_set );
  
  crude_gfx_rhi_update_descriptor_set( &gpu->rhi_device, descriptor_set->rhi_descriptor_set, descriptor_write, num_resources );

  crude_gfx_rhi_set_descriptor_set_debug_name( &gpu->rhi_device, descriptor_set->rhi_descriptor_set, creation->name );

  return handle;
}

void                                      
crude_gfx_destroy_descriptor_set
(                                                   
  _In_ crude_gfx_device                                   *gpu,
  _In_ crude_gfx_descriptor_set_handle                     handle
)
{
  if ( handle.index >= gpu->descriptor_sets.pool_size )
  {
    CRUDE_LOG_ERROR( CRUDE_CHANNEL_GRAPHICS, "Trying to free invalid descriptor set %u", handle.index );
    return;
  }
  crude_gfx_resource_update descriptor_set_update_event = { 
    .type          = CRUDE_GFX_RESOURCE_DELETION_TYPE_DESCRIPTOR_SET,
    .handle        = handle.index,
    .current_frame = gpu->current_frame };
  crude_gfx_push_resource_update_( gpu, &descriptor_set_update_event );
}

void
crude_gfx_destroy_descriptor_set_instant
(                                                   
  _In_ crude_gfx_device                                   *gpu,
  _In_ crude_gfx_descriptor_set_handle                     handle
)
{
  crude_gfx_descriptor_set *descriptor_set = crude_gfx_access_descriptor_set( gpu, handle );
  crude_gfx_release_descriptor_set( gpu, handle );
}

crude_gfx_framebuffer_handle
crude_gfx_create_framebuffer
(
  _In_ crude_gfx_device                                   *gpu,
  _In_ crude_gfx_framebuffer_creation const               *creation
)
{
  crude_gfx_framebuffer_handle handle = crude_gfx_obtain_framebuffer( gpu );
  if ( CRUDE_RESOURCE_HANDLE_IS_INVALID( handle ) )
  {
    return handle;
  }
  
  crude_gfx_framebuffer *framebuffer = crude_gfx_access_framebuffer( gpu, handle );
  framebuffer->num_color_attachments = creation->num_render_targets;
  for ( uint32 i = 0; i < creation->num_render_targets; ++i )
  {
    framebuffer->color_attachments[ i ] = creation->output_textures[ i ];
  }
  framebuffer->depth_stencil_attachment = creation->depth_stencil_texture;
  framebuffer->width = creation->width;
  framebuffer->height = creation->height;
  framebuffer->resize = creation->resize;
  crude_string_copy( framebuffer->name, creation->name, sizeof( framebuffer->name ) );
  framebuffer->scale_x = 1.0;
  framebuffer->scale_y = 1.0;
  framebuffer->manual_resources_free = creation->manual_resources_free;
  
  return handle;
}

void                                      
crude_gfx_destroy_framebuffer
(                                                   
  _In_ crude_gfx_device                                   *gpu,
  _In_ crude_gfx_framebuffer_handle                        handle
)
{
  if ( handle.index >= gpu->framebuffers.pool_size )
  {
    CRUDE_LOG_ERROR( CRUDE_CHANNEL_GRAPHICS, "Trying to free invalid framebuffer %u", handle.index );
    return;
  }
  crude_gfx_resource_update framebuffer_update_event = { 
    .type          = CRUDE_GFX_RESOURCE_DELETION_TYPE_FRAMEBUFFER,
    .handle        = handle.index,
    .current_frame = gpu->current_frame };
  crude_gfx_push_resource_update_( gpu, &framebuffer_update_event );
}

void
crude_gfx_destroy_framebuffer_instant
(                                                   
  _In_ crude_gfx_device                                   *gpu,
  _In_ crude_gfx_framebuffer_handle                        handle
)
{
  crude_gfx_framebuffer *framebuffer = crude_gfx_access_framebuffer( gpu, handle );

  framebuffer->name[ 0 ] = 0;

  if ( framebuffer && !framebuffer->manual_resources_free )
  {
    for ( uint32 i = 0; i < framebuffer->num_color_attachments; ++i )
    {
      crude_gfx_destroy_texture_instant( gpu, framebuffer->color_attachments[ i ] );
    }
    
    if ( CRUDE_RESOURCE_HANDLE_IS_VALID( framebuffer->depth_stencil_attachment ) )
    {
      crude_gfx_destroy_texture_instant( gpu, framebuffer->depth_stencil_attachment );
    }
  }
  crude_gfx_release_framebuffer( gpu, handle );
}

crude_gfx_cmd_pool_handle
crude_gfx_create_cmd_pool
(
  _In_ crude_gfx_device                                   *gpu,
  _In_ crude_gfx_cmd_pool_creation const                  *creation
)
{
  crude_gfx_cmd_pool                                      *cmd_pool;
  crude_gfx_cmd_pool_handle                                cmd_pool_handle;
  crude_gfx_rhi_command_pool_create_info                   rhi_cmd_pool_creation;
  
  cmd_pool_handle = CRUDE_COMPOUNT( crude_gfx_cmd_pool_handle, { crude_resource_pool_obtain_resource( &gpu->cmd_pools ) } );
  if ( CRUDE_RESOURCE_HANDLE_IS_INVALID( cmd_pool_handle ) )
  {
    return cmd_pool_handle;
  }
  
  cmd_pool = crude_gfx_access_cmd_pool( gpu, cmd_pool_handle );
  
  rhi_cmd_pool_creation = CRUDE_COMPOUNT_EMPTY( crude_gfx_rhi_command_pool_create_info );
  rhi_cmd_pool_creation.queue = creation->queue;
  crude_gfx_rhi_create_command_pool( &gpu->rhi_device, &rhi_cmd_pool_creation, &cmd_pool->rhi_cmd_pool );

#if CRUDE_GFX_GPU_PROFILER
  if ( creation->profiler.enabled )
  {
    crude_gfx_rhi_queru_pool_create_info                   rhi_timestamp_pool_info, rhi_statistics_pool_info;

    cmd_pool->profiler.enabled = true;
    cmd_pool->profiler.time_queries_trees = creation->profiler.time_queries_trees;

    rhi_timestamp_pool_info = CRUDE_COMPOUNT_EMPTY( crude_gfx_rhi_queru_pool_create_info );
    rhi_timestamp_pool_info.query_type = CRUDE_GFX_RHI_QUERY_TYPE_TIMESTAMP;
    rhi_timestamp_pool_info.query_count = 2u * creation->profiler.time_queries_per_frame;
    crude_gfx_rhi_create_query_pool( &gpu->rhi_device, &rhi_timestamp_pool_info, &cmd_pool->profiler.rhi_timestamp_query_pool );
      
    rhi_statistics_pool_info = CRUDE_COMPOUNT_EMPTY( crude_gfx_rhi_queru_pool_create_info );
    rhi_statistics_pool_info.query_type = CRUDE_GFX_RHI_QUERY_TYPE_PIPELINE_STATISTICS;
    rhi_statistics_pool_info.query_count = CRUDE_GFX_GPU_PIPELINE_STATISTICS_COUNT;
    rhi_statistics_pool_info.pipeline_statistics = CRUDE_GFX_RHI_QUERY_PIPELINE_STATISTIC_INPUT_ASSEMBLY_VERTICES_BIT |
      CRUDE_GFX_RHI_QUERY_PIPELINE_STATISTIC_INPUT_ASSEMBLY_PRIMITIVES_BIT |
      CRUDE_GFX_RHI_QUERY_PIPELINE_STATISTIC_VERTEX_SHADER_INVOCATIONS_BIT |
      CRUDE_GFX_RHI_QUERY_PIPELINE_STATISTIC_CLIPPING_INVOCATIONS_BIT |
      CRUDE_GFX_RHI_QUERY_PIPELINE_STATISTIC_CLIPPING_PRIMITIVES_BIT |
      CRUDE_GFX_RHI_QUERY_PIPELINE_STATISTIC_FRAGMENT_SHADER_INVOCATIONS_BIT |
      CRUDE_GFX_RHI_QUERY_PIPELINE_STATISTIC_COMPUTE_SHADER_INVOCATIONS_BIT |
      CRUDE_GFX_RHI_QUERY_PIPELINE_STATISTIC_TASK_SHADER_INVOCATIONS_BIT_EXT |
      CRUDE_GFX_RHI_QUERY_PIPELINE_STATISTIC_MESH_SHADER_INVOCATIONS_BIT_EXT;
    crude_gfx_rhi_create_query_pool( &gpu->rhi_device, &rhi_statistics_pool_info, &cmd_pool->profiler.rhi_pipeline_stats_query_pool );
  }
#endif /* CRUDE_GFX_GPU_PROFILER */

  return cmd_pool_handle;
}

void
crude_gfx_destroy_cmd_pool_instant
(
  _In_ crude_gfx_device                                   *gpu,
  _In_ crude_gfx_cmd_pool_handle                           handle
)
{
  crude_gfx_cmd_pool                                      *cmd_pool;

  cmd_pool = crude_gfx_access_cmd_pool( gpu, handle );

  crude_gfx_rhi_destroy_command_pool( &gpu->rhi_device, cmd_pool->rhi_cmd_pool );
  
#if CRUDE_GFX_GPU_PROFILER
  if ( cmd_pool->profiler.enabled )
  {
    crude_gfx_rhi_destroy_query_pool( &gpu->rhi_device, cmd_pool->profiler.rhi_timestamp_query_pool );
    crude_gfx_rhi_destroy_query_pool( &gpu->rhi_device, cmd_pool->profiler.rhi_pipeline_stats_query_pool );
  }
#endif

  crude_resource_pool_release_resource( &gpu->cmd_pools, handle.index );
}

crude_gfx_cmd_buffer_handle
crude_gfx_create_cmd_buffer
(
  _In_ crude_gfx_device                                   *gpu,
  _In_ crude_gfx_cmd_buffer_creation const                *creation
)
{
  crude_gfx_cmd_buffer                                     *cmd_buffer;
  crude_gfx_cmd_buffer_handle                               cmd_buffer_handle;
  crude_gfx_rhi_command_buffer_create_info                  rhi_creation;

  cmd_buffer_handle = CRUDE_COMPOUNT( crude_gfx_cmd_buffer_handle, { crude_resource_pool_obtain_resource( &gpu->cmd_buffers ) } );
  if ( CRUDE_RESOURCE_HANDLE_IS_INVALID( cmd_buffer_handle ) )
  {
    return cmd_buffer_handle;
  }
  
  cmd_buffer = crude_gfx_access_cmd_buffer( gpu, cmd_buffer_handle );
  
  *cmd_buffer = CRUDE_COMPOUNT_EMPTY( crude_gfx_cmd_buffer );

  crude_string_copy( cmd_buffer->name, creation->name, sizeof( cmd_buffer->name ) );

  cmd_buffer->cmd_pool = creation->cmd_pool;
  cmd_buffer->gpu = gpu;

  rhi_creation = CRUDE_COMPOUNT_EMPTY( crude_gfx_rhi_command_buffer_create_info );
  rhi_creation.command_pool = crude_gfx_access_cmd_pool( gpu, cmd_buffer->cmd_pool )->rhi_cmd_pool;
  crude_gfx_rhi_create_command_buffer( &gpu->rhi_device, &rhi_creation, &cmd_buffer->rhi_cmd_buffer );
  
  crude_gfx_rhi_set_command_buffer_debug_name( &gpu->rhi_device, cmd_buffer->rhi_cmd_buffer, creation->name );

  crude_gfx_cmd_reset( cmd_buffer );

  return cmd_buffer_handle;
}

void
crude_gfx_destroy_cmd_buffer_instant
(
  _In_ crude_gfx_device                                   *gpu,
  _In_ crude_gfx_cmd_buffer_handle                         handle
)
{
  crude_gfx_cmd_buffer                                    *cmd_buffer;

  cmd_buffer = crude_gfx_access_cmd_buffer( gpu, handle );
  crude_gfx_rhi_destroy_command_buffer( &gpu->rhi_device, cmd_buffer->rhi_cmd_buffer );

  crude_resource_pool_release_resource( &gpu->cmd_buffers, handle.index );
}

crude_gfx_technique*
crude_gfx_create_technique
(
  _In_ crude_gfx_device                                   *gpu,
  _In_ crude_gfx_technique_creation const                 *creation
)
{
  crude_gfx_technique                                     *technique;
  crude_gfx_technique_handle                               technique_handle;

  technique_handle = crude_gfx_obtain_technique( gpu );
  if ( CRUDE_RESOURCE_HANDLE_IS_INVALID( technique_handle ) )
  {
    return NULL;
  }

  technique = crude_gfx_access_technique( gpu, technique_handle );
  
#if CRUDE_DEVELOP
  crude_string_copy( technique->technique_relative_filepath, creation->technique_relative_filepath, sizeof( technique->technique_relative_filepath ) );
#endif
  crude_string_copy( technique->name, creation->name, sizeof( technique->name ) );

  CRUDE_ARRAY_INITIALIZE_WITH_CAPACITY( technique->passes, creation->passes_count, crude_heap_allocator_pack( gpu->allocator ) );
  CRUDE_HASHMAPSTR_INITIALIZE( technique->name_to_pass_index, crude_heap_allocator_pack( gpu->allocator ) );

  for ( size_t i = 0; i < creation->passes_count; ++i )
  {
    crude_gfx_technique_pass                              *pass;
    crude_gfx_technique_pass_creation const               *pass_creation;
    crude_gfx_pipeline                                    *pipeline;
    crude_gfx_technique_pass                               technique_pass;
    uint64                                                 pass_name_hashed;

    pass = &technique->passes[ i ];
    
    pass_creation = &creation->passes[ i ];
    technique_pass = CRUDE_COMPOUNT( crude_gfx_technique_pass, { pass_creation->pipeline } );
    if ( CRUDE_RESOURCE_HANDLE_IS_INVALID( technique_pass.pipeline ) )
    {
      return NULL;
    }

    pipeline = crude_gfx_access_pipeline( gpu, technique_pass.pipeline );

    CRUDE_ARRAY_PUSH( technique->passes, technique_pass );
    
    CRUDE_HASHMAPSTR_SET( technique->name_to_pass_index, CRUDE_COMPOUNT( crude_string_link, { pipeline->name } ), i );
    
    for ( uint32 i = 0; i < pipeline->num_active_layouts; ++i )
    {
      crude_gfx_descriptor_set_layout const *descriptor_set_layout = pipeline->descriptor_set_layout[ i ];
      if ( descriptor_set_layout == NULL )
      {
        continue;
      }
    }

  }
  
  if ( creation->name )
  {
    CRUDE_HASHMAPSTR_SET( gpu->resource_cache.techniques, CRUDE_COMPOUNT( crude_string_link, { technique->name } ), technique );
  }

  technique->pool_index = technique_handle.index;
  return technique;
}

void
crude_gfx_destroy_technique_instant
(
  _In_ crude_gfx_device                                   *gpu,
  _In_ crude_gfx_technique                                *technique
)
{
  if ( !technique )
  {
    return;
  }

  for ( size_t i = 0; i < CRUDE_ARRAY_LENGTH( technique->passes ); ++i )
  {
    crude_gfx_destroy_pipeline( gpu, technique->passes[ i ].pipeline );
    if ( technique->passes[ i ].name_hashed_to_descriptor_index )
    {
      CRUDE_HASHMAPSTR_DEINITIALIZE( technique->passes[ i ].name_hashed_to_descriptor_index );
    }
  }
  
  CRUDE_ARRAY_DEINITIALIZE( technique->passes );
  CRUDE_HASHMAPSTR_DEINITIALIZE( technique->name_to_pass_index );
  CRUDE_HASHMAPSTR_REMOVE( gpu->resource_cache.techniques, technique->name );
  crude_gfx_release_technique( gpu, CRUDE_COMPOUNT( crude_gfx_technique_handle, { technique->pool_index } ) );
}

/************************************************
 *
 * GPU Device Resource Macros
 * 
 ***********************************************/  
crude_gfx_sampler_handle
crude_gfx_obtain_sampler
(
  _In_ crude_gfx_device                                   *gpu
)
{
  return CRUDE_COMPOUNT( crude_gfx_sampler_handle, { crude_resource_pool_obtain_resource( &gpu->samplers ) } );
}

crude_gfx_sampler*
crude_gfx_access_sampler
(
  _In_ crude_gfx_device                                   *gpu,
  _In_ crude_gfx_sampler_handle                            handle
)
{
  return CRUDE_REINTERPRET_CAST( crude_gfx_sampler*, crude_resource_pool_access_resource( &gpu->samplers, handle.index ) );
}

void
crude_gfx_release_sampler
(
  _In_ crude_gfx_device                                   *gpu,
  _In_ crude_gfx_sampler_handle                            handle
)
{
  crude_resource_pool_release_resource( &gpu->samplers, handle.index );
}

crude_gfx_texture_handle
crude_gfx_obtain_texture
(
  _In_ crude_gfx_device                                   *gpu
)
{
  return CRUDE_COMPOUNT( crude_gfx_texture_handle, { crude_resource_pool_obtain_resource( &gpu->textures ) } );
}

crude_gfx_texture*
crude_gfx_access_texture
(
  _In_ crude_gfx_device                                   *gpu,
  _In_ crude_gfx_texture_handle                            handle
)
{
  return CRUDE_REINTERPRET_CAST( crude_gfx_texture*, crude_resource_pool_access_resource( &gpu->textures, handle.index ) );
}

void
crude_gfx_release_texture
(
  _In_ crude_gfx_device                                   *gpu,
  _In_ crude_gfx_texture_handle                            handle
)
{
  crude_resource_pool_release_resource( &gpu->textures, handle.index );
}
           
crude_gfx_render_pass_handle
crude_gfx_obtain_render_pass
(
  _In_ crude_gfx_device                                   *gpu
)
{
  return CRUDE_COMPOUNT( crude_gfx_render_pass_handle, { crude_resource_pool_obtain_resource( &gpu->render_passes ) } );
}

crude_gfx_render_pass*
crude_gfx_access_render_pass
(
  _In_ crude_gfx_device                                   *gpu,
  _In_ crude_gfx_render_pass_handle                        handle
)
{
  return CRUDE_REINTERPRET_CAST( crude_gfx_render_pass*, crude_resource_pool_access_resource( &gpu->render_passes, handle.index ) );
}

void
crude_gfx_release_render_pass
(
  _In_ crude_gfx_device                                   *gpu,
  _In_ crude_gfx_render_pass_handle                        handle
)
{
  crude_resource_pool_release_resource( &gpu->render_passes, handle.index );
}

crude_gfx_shader_state_handle
crude_gfx_obtain_shader_state
(
  _In_ crude_gfx_device                                   *gpu
)
{
  return CRUDE_COMPOUNT( crude_gfx_shader_state_handle, { crude_resource_pool_obtain_resource( &gpu->shaders ) } );
}

crude_gfx_shader_state*
crude_gfx_access_shader_state
(
  _In_ crude_gfx_device                                   *gpu,
  _In_ crude_gfx_shader_state_handle                       handle
)
{
  return CRUDE_REINTERPRET_CAST( crude_gfx_shader_state*, crude_resource_pool_access_resource( &gpu->shaders, handle.index ) );
}

void
crude_gfx_release_shader_state
(
  _In_ crude_gfx_device                                   *gpu,
  _In_ crude_gfx_shader_state_handle                       handle
)
{
  crude_resource_pool_release_resource( &gpu->shaders, handle.index );
}

crude_gfx_pipeline_handle
crude_gfx_obtain_pipeline
(
  _In_ crude_gfx_device                                   *gpu
)
{
  return CRUDE_COMPOUNT( crude_gfx_pipeline_handle,  { crude_resource_pool_obtain_resource( &gpu->pipelines ) } );
}

crude_gfx_pipeline*
crude_gfx_access_pipeline
(
  _In_ crude_gfx_device                                   *gpu,
  _In_ crude_gfx_pipeline_handle                           handle
)
{
  return CRUDE_REINTERPRET_CAST( crude_gfx_pipeline*, crude_resource_pool_access_resource( &gpu->pipelines, handle.index ) );
}

void
crude_gfx_release_pipeline
(
  _In_ crude_gfx_device                                   *gpu,
  _In_ crude_gfx_pipeline_handle                           handle
)
{
  crude_resource_pool_release_resource( &gpu->pipelines, handle.index );
}

crude_gfx_buffer_handle
crude_gfx_obtain_buffer
(
  _In_ crude_gfx_device                                   *gpu
)
{
  return CRUDE_COMPOUNT( crude_gfx_buffer_handle, { crude_resource_pool_obtain_resource( &gpu->buffers ) } );
}

crude_gfx_buffer*
crude_gfx_access_buffer
(
  _In_ crude_gfx_device                                   *gpu,
  _In_ crude_gfx_buffer_handle                             handle
)
{
  return CRUDE_REINTERPRET_CAST( crude_gfx_buffer*, crude_resource_pool_access_resource( &gpu->buffers, handle.index ) );
}

void
crude_gfx_release_buffer
(
  _In_ crude_gfx_device                                   *gpu,
  _In_ crude_gfx_buffer_handle                             handle
)
{
  crude_resource_pool_release_resource( &gpu->buffers, handle.index );
}

crude_gfx_descriptor_set_layout_handle
crude_gfx_obtain_descriptor_set_layout
(
  _In_ crude_gfx_device                                   *gpu
)
{
  return CRUDE_COMPOUNT( crude_gfx_descriptor_set_layout_handle, { crude_resource_pool_obtain_resource( &gpu->descriptor_set_layouts ) } );
}

crude_gfx_descriptor_set_layout*
crude_gfx_access_descriptor_set_layout
(
  _In_ crude_gfx_device                                   *gpu,
  _In_ crude_gfx_descriptor_set_layout_handle              handle
)
{
  return CRUDE_REINTERPRET_CAST( crude_gfx_descriptor_set_layout*, crude_resource_pool_access_resource( &gpu->descriptor_set_layouts, handle.index ) );
}

void
crude_gfx_release_descriptor_set_layout
(
  _In_ crude_gfx_device                                   *gpu,
  _In_ crude_gfx_descriptor_set_layout_handle              handle
)
{
  crude_resource_pool_release_resource( &gpu->descriptor_set_layouts, handle.index );
}

crude_gfx_descriptor_set_handle
crude_gfx_obtain_descriptor_set
(
  _In_ crude_gfx_device                                   *gpu
)
{
  return CRUDE_COMPOUNT( crude_gfx_descriptor_set_handle, { crude_resource_pool_obtain_resource( &gpu->descriptor_sets ) } );
}

crude_gfx_descriptor_set*
crude_gfx_access_descriptor_set
(
  _In_ crude_gfx_device                                   *gpu,
  _In_ crude_gfx_descriptor_set_handle                     handle
)
{
  return CRUDE_REINTERPRET_CAST( crude_gfx_descriptor_set*, crude_resource_pool_access_resource( &gpu->descriptor_sets, handle.index ) );
}

void
crude_gfx_release_descriptor_set
(
  _In_ crude_gfx_device                                   *gpu,
  _In_ crude_gfx_descriptor_set_handle                     handle
)
{
  crude_resource_pool_release_resource( &gpu->descriptor_sets, handle.index );
}

crude_gfx_framebuffer_handle
crude_gfx_obtain_framebuffer
(
  _In_ crude_gfx_device                                   *gpu
)
{
  return CRUDE_COMPOUNT( crude_gfx_framebuffer_handle, { crude_resource_pool_obtain_resource( &gpu->framebuffers ) } );
}

crude_gfx_framebuffer*
crude_gfx_access_framebuffer
(
  _In_ crude_gfx_device                                   *gpu,
  _In_ crude_gfx_framebuffer_handle                        handle
)
{
  return CRUDE_REINTERPRET_CAST( crude_gfx_framebuffer*, crude_resource_pool_access_resource( &gpu->framebuffers, handle.index ) );
}

void
crude_gfx_release_framebuffer
(
  _In_ crude_gfx_device                                   *gpu,
  _In_ crude_gfx_framebuffer_handle                        handle
)
{
  crude_resource_pool_release_resource( &gpu->framebuffers, handle.index );
}

crude_gfx_cmd_pool*
crude_gfx_access_cmd_pool
(
  _In_ crude_gfx_device                                   *gpu,
  _In_ crude_gfx_cmd_pool_handle                           handle
)
{
  return CRUDE_REINTERPRET_CAST( crude_gfx_cmd_pool*, crude_resource_pool_access_resource( &gpu->cmd_pools, handle.index ) );
}

crude_gfx_cmd_buffer*
crude_gfx_access_cmd_buffer
(
  _In_ crude_gfx_device                                   *gpu,
  _In_ crude_gfx_cmd_buffer_handle                         handle
)
{
  return CRUDE_REINTERPRET_CAST( crude_gfx_cmd_buffer*, crude_resource_pool_access_resource( &gpu->cmd_buffers, handle.index ) );
}

crude_gfx_technique_handle
crude_gfx_obtain_technique
(
  _In_ crude_gfx_device                                   *gpu
)
{
  return CRUDE_COMPOUNT( crude_gfx_technique_handle, { crude_resource_pool_obtain_resource( &gpu->techniques ) } );
}

crude_gfx_technique*
crude_gfx_access_technique
(
  _In_ crude_gfx_device                                   *gpu,
  _In_ crude_gfx_technique_handle                          handle
)
{
  return CRUDE_REINTERPRET_CAST( crude_gfx_technique*, crude_resource_pool_access_resource( &gpu->techniques, handle.index ) );
}

void
crude_gfx_release_technique
(
  _In_ crude_gfx_device                                   *gpu,
  _In_ crude_gfx_technique_handle                          handle
)
{
  crude_resource_pool_release_resource( &gpu->techniques, handle.index );
}

void
crude_gfx_create_swapchain_internal_
( 
  _In_ crude_gfx_device                                   *gpu
)
{
  crude_gfx_cmd_buffer                                    *cmd;
  crude_gfx_rhi_swapchain_create_info                      swapchain_creation;

  swapchain_creation = CRUDE_COMPOUNT_EMPTY( crude_gfx_rhi_swapchain_create_info );
  swapchain_creation.surface = gpu->rhi_surface;

  crude_gfx_rhi_create_swapchain(
    &gpu->rhi_device, &swapchain_creation, gpu->allocator,
    &gpu->rhi_swapchain, &gpu->swapchain_images_count, &gpu->swapchain_size, gpu->rhi_swapchain_images );

  gpu->renderer_size = gpu->swapchain_size;
  
  cmd = crude_gfx_access_cmd_buffer( gpu, gpu->immediate_transfer_cmd_buffer );
  crude_gfx_cmd_begin_primary( cmd );
  for ( size_t i = 0; i < gpu->swapchain_images_count; ++i )
  {
    crude_gfx_cmd_add_image_barrier_ext2( cmd, gpu->rhi_swapchain_images[ i ], CRUDE_GFX_RHI_RESOURCE_STATE_UNDEFINED, CRUDE_GFX_RHI_RESOURCE_STATE_PRESENT, 0, 1, false );
  }
  crude_gfx_submit_immediate( cmd );
}

void
crude_gfx_create_descriptor_pool_internal_
(
  _In_ crude_gfx_device                                   *gpu
)
{
  crude_gfx_descriptor_set_layout_creation                 dsl_creation;
  crude_gfx_descriptor_set_creation                        ds_creation;

  crude_gfx_rhi_create_descriptor_pool( &gpu->rhi_device, true, &gpu->rhi_bindless_descriptor_pool );
  crude_gfx_rhi_set_descriptor_pool_debug_name( &gpu->rhi_device, gpu->rhi_bindless_descriptor_pool, "rhi_bindless_descriptor_pool" );
  
  crude_gfx_rhi_create_descriptor_pool( &gpu->rhi_device, false, &gpu->rhi_descriptor_pool );
  crude_gfx_rhi_set_descriptor_pool_debug_name( &gpu->rhi_device, gpu->rhi_descriptor_pool, "rhi_descriptor_pool" );

  dsl_creation = CRUDE_COMPOUNT_EMPTY( crude_gfx_descriptor_set_layout_creation );
  dsl_creation.name = "bindless_descriptor_set_layout";
  dsl_creation.bindless = true;
  dsl_creation.set_index = 0u;
  crude_gfx_descriptor_set_layout_creation_add_binding( &dsl_creation, CRUDE_COMPOUNT( crude_gfx_descriptor_set_layout_binding, {
    .type = CRUDE_GFX_RHI_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
    .start = CRUDE_BINDLESS_TEXTURE_BINDING,
    .count = CRUDE_GFX_BINDLESS_RESOURCES_MAX_COUNT,
  } ) );
  crude_gfx_descriptor_set_layout_creation_add_binding( &dsl_creation, CRUDE_COMPOUNT( crude_gfx_descriptor_set_layout_binding, {
    .type = CRUDE_GFX_RHI_DESCRIPTOR_TYPE_STORAGE_IMAGE,
    .start = CRUDE_BINDLESS_TEXTURE_BINDING + 1,
    .count = CRUDE_GFX_BINDLESS_RESOURCES_MAX_COUNT,
  } ) );
  gpu->bindless_descriptor_set_layout_handle = crude_gfx_create_descriptor_set_layout( gpu, &dsl_creation );
  
  ds_creation = crude_gfx_descriptor_set_creation_empty();
  ds_creation.name = "bindless_descriptor_set";
  ds_creation.layout = gpu->bindless_descriptor_set_layout_handle;
  gpu->bindless_descriptor_set_handle = crude_gfx_create_descriptor_set( gpu, &ds_creation );
}

void
crude_gfx_create_texture_internal_
(
  _In_ crude_gfx_device                                   *gpu,
  _In_ crude_gfx_texture_creation const                   *creation,
  _In_ crude_gfx_texture_handle                            handle,
  _In_ crude_gfx_texture                                  *texture
)
{
  crude_gfx_rhi_image_create_info                          rhi_creation;
  bool                                                     is_render_target, is_compute_used;

  texture->width = creation->width;
  texture->height = creation->height;
  texture->depth = creation->depth;
  texture->subresource = creation->subresource;
  texture->type = creation->type;
  texture->format = creation->format;
  texture->sampler = NULL;
  texture->flags = creation->flags;
  texture->handle = handle;
  texture->parent_texture_handle = CRUDE_GFX_TEXTURE_HANDLE_INVALID;
  texture->alias_texture_handle = creation->alias;
  texture->state = CRUDE_GFX_RHI_RESOURCE_STATE_UNDEFINED;
  crude_string_copy( texture->name, creation->name, sizeof( texture->name ) );

  rhi_creation = CRUDE_COMPOUNT_EMPTY( crude_gfx_rhi_image_create_info );
  rhi_creation.image_type = crude_gfx_texture_type_to_image_type( creation->type );
  rhi_creation.format = texture->format;
  rhi_creation.extent.x = creation->width;
  rhi_creation.extent.y = creation->height;
  rhi_creation.extent.z = creation->depth;
  rhi_creation.mip_levels = creation->subresource.mip_level_count;
  rhi_creation.array_layers = creation->subresource.array_layer_count;
  rhi_creation.tiling = CRUDE_GFX_RHI_IMAGE_TILING_OPTIMAL;
  rhi_creation.sharing_mode = CRUDE_GFX_RHI_SHARING_MODE_EXCLUSIVE;
  rhi_creation.samples = creation->multisampled ? CRUDE_GFX_SAMPLE_COUNT : CRUDE_GFX_RHI_SAMPLE_COUNT_1_BIT;

  is_render_target = ( creation->flags & CRUDE_GFX_TEXTURE_MASK_RENDER_TARGET ) == CRUDE_GFX_TEXTURE_MASK_RENDER_TARGET;
  is_compute_used = ( creation->flags & CRUDE_GFX_TEXTURE_MASK_COMPUTE ) == CRUDE_GFX_TEXTURE_MASK_COMPUTE;
    
  rhi_creation.usage = CRUDE_GFX_RHI_IMAGE_USAGE_SAMPLED_BIT;

  if ( is_compute_used )
  {
    rhi_creation.usage |= CRUDE_GFX_RHI_IMAGE_USAGE_STORAGE_BIT;
  }
  
  if ( crude_gfx_rhi_format_has_depth_or_stencil( creation->format ) )
  {
    rhi_creation.usage |= CRUDE_GFX_RHI_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
  }
  else
  {
    rhi_creation.usage |= CRUDE_GFX_RHI_IMAGE_USAGE_TRANSFER_DST_BIT | CRUDE_GFX_RHI_IMAGE_USAGE_TRANSFER_SRC_BIT;
    rhi_creation.usage |= is_render_target ? CRUDE_GFX_RHI_IMAGE_USAGE_COLOR_ATTACHMENT_BIT : 0;
  }

  if ( CRUDE_RESOURCE_HANDLE_IS_INVALID( texture->alias_texture_handle ) )
  {
    rhi_creation.alias_image = NULL;
    crude_gfx_rhi_create_image( &gpu->rhi_device, &rhi_creation, &texture->rhi_image );
    crude_gfx_rhi_set_image_allocation_debug_name( &gpu->rhi_device, texture->rhi_image, creation->name );
  }
  else
  {
    crude_gfx_texture* alias_texture = crude_gfx_access_texture( gpu, texture->alias_texture_handle );
    CRUDE_ASSERT( alias_texture );
  
    rhi_creation.alias_image = &alias_texture->rhi_image;
    crude_gfx_rhi_create_image( &gpu->rhi_device, &rhi_creation, &texture->rhi_image );
  }
    
  crude_gfx_rhi_set_image_debug_name( &gpu->rhi_device, texture->rhi_image, creation->name );
  
  crude_gfx_texture_view_creation view_creation = crude_gfx_texture_view_creation_empty();
  view_creation.parent_texture_handle = texture->handle;
  view_creation.view_type = crude_gfx_texture_type_to_image_view_type( creation->type );
  view_creation.subresource = texture->subresource;
  view_creation.name = texture->name;
  crude_gfx_create_texture_view_internal_( gpu, &view_creation, texture );

  {
    crude_gfx_resource_update texture_update_event = { 
      .type          = CRUDE_GFX_RESOURCE_DELETION_TYPE_TEXTURE,
      .handle        = handle.index,
      .current_frame = gpu->current_frame
    };
    CRUDE_ARRAY_PUSH( gpu->texture_to_update_bindless, texture_update_event );
  }
}

void
crude_gfx_create_texture_view_internal_
(
  _In_ crude_gfx_device                                   *gpu,
  _In_ crude_gfx_texture_view_creation const              *creation,
  _In_ crude_gfx_texture                                  *texture
)
{
  crude_gfx_rhi_image_view_create_info                     rhi_creation;
  
  rhi_creation = CRUDE_COMPOUNT_EMPTY( crude_gfx_rhi_image_view_create_info );
  rhi_creation.image = texture->rhi_image;
  rhi_creation.format = texture->format;
  
  if ( crude_gfx_rhi_format_has_depth_or_stencil( texture->format ))
  {
    rhi_creation.subresource_range.aspect_mask = crude_gfx_rhi_format_has_depth( texture->format ) ? ( CRUDE_GFX_RHI_IMAGE_ASPECT_DEPTH_BIT | CRUDE_GFX_RHI_IMAGE_ASPECT_DEPTH_BIT ) : 0;
  }
  else
  {
    rhi_creation.subresource_range.aspect_mask = CRUDE_GFX_RHI_IMAGE_ASPECT_COLOR_BIT;
  }
  
  rhi_creation.view_type = creation->view_type;
  rhi_creation.subresource_range.base_mip_level = creation->subresource.mip_base_level;
  rhi_creation.subresource_range.level_count = creation->subresource.mip_level_count;
  rhi_creation.subresource_range.base_array_layer = creation->subresource.array_base_layer;
  rhi_creation.subresource_range.layer_count = creation->subresource.array_layer_count;

  crude_gfx_rhi_create_image_view( &gpu->rhi_device, &rhi_creation, &texture->rhi_image_view );
  crude_gfx_rhi_set_image_view_debug_name( &gpu->rhi_device, texture->rhi_image_view, creation->name );
}

void
crude_gfx_resize_swapchain_internal_
(
  _In_ crude_gfx_device                                   *gpu
)
{
  XMFLOAT2                                                 new_surface_extent;

  crude_gfx_rhi_wait_idle( &gpu->rhi_device );
  
  new_surface_extent = crude_gfx_rhi_get_surface_extent( &gpu->rhi_device, gpu->rhi_surface );

  if ( new_surface_extent.x == 0 || new_surface_extent.y == 0 )
  {
    return;
  }

  crude_gfx_rhi_destroy_swapchain( &gpu->rhi_device, gpu->rhi_swapchain );
  crude_gfx_rhi_destroy_surface( gpu->rhi_instance, gpu->rhi_surface );
  crude_gfx_rhi_create_surface( gpu->rhi_instance, gpu->sdl_window, &gpu->rhi_surface );

  crude_gfx_create_swapchain_internal_( gpu );

  gpu->swapchain_resized_last_frame = true;

  crude_gfx_rhi_wait_idle( &gpu->rhi_device );
}

crude_gfx_vertex_component_format
reflect_format_to_vk_format_
(
  _In_ SpvReflectFormat                                    format
)
{
  switch ( format )
  {
    case SPV_REFLECT_FORMAT_R16G16_SINT         : return CRUDE_GFX_VERTEX_COMPONENT_FORMAT_SHORT2;
    case SPV_REFLECT_FORMAT_R16G16B16A16_SINT   : return CRUDE_GFX_VERTEX_COMPONENT_FORMAT_SHORT4;
    case SPV_REFLECT_FORMAT_R32_UINT            : return CRUDE_GFX_VERTEX_COMPONENT_FORMAT_UINT;
    case SPV_REFLECT_FORMAT_R32_SINT            : return CRUDE_GFX_VERTEX_COMPONENT_FORMAT_UINT;
    case SPV_REFLECT_FORMAT_R32_SFLOAT          : return CRUDE_GFX_VERTEX_COMPONENT_FORMAT_FLOAT;
    case SPV_REFLECT_FORMAT_R32G32_UINT         : return CRUDE_GFX_VERTEX_COMPONENT_FORMAT_UINT2;
    case SPV_REFLECT_FORMAT_R32G32_SFLOAT       : return CRUDE_GFX_VERTEX_COMPONENT_FORMAT_FLOAT2;
    case SPV_REFLECT_FORMAT_R32G32B32_SFLOAT    : return CRUDE_GFX_VERTEX_COMPONENT_FORMAT_FLOAT3;
    case SPV_REFLECT_FORMAT_R32G32B32A32_UINT   : return CRUDE_GFX_VERTEX_COMPONENT_FORMAT_UINT4;
    case SPV_REFLECT_FORMAT_R32G32B32A32_SFLOAT : return CRUDE_GFX_VERTEX_COMPONENT_FORMAT_FLOAT4;
  };
  CRUDE_LOG_ERROR( CRUDE_CHANNEL_GRAPHICS, "Don't support reflect format %i", format );
}

void
vk_reflect_shader_
(
  _In_ crude_gfx_device                                   *gpu,
  _In_ void const                                         *code,
  _In_ uint32                                              code_size,
  _In_ crude_gfx_shader_reflect                           *reflect
)
{
  SpvReflectShaderModule                                   spv_reflect;
  SpvReflectResult                                         result;
  
  result = spvReflectCreateShaderModule( code_size, code, &spv_reflect );
  CRUDE_ASSERT( result == SPV_REFLECT_RESULT_SUCCESS );
  
  if ( spv_reflect.push_constant_block_count )
  {
    reflect->push_constant.stride = spv_reflect.push_constant_blocks[ 0 ].size;
  }

  if ( spv_reflect.shader_stage == SPV_REFLECT_SHADER_STAGE_VERTEX_BIT )
  {
    CRUDE_ARRAY_INITIALIZE_WITH_CAPACITY( reflect->input.vertex_attributes, spv_reflect.input_variable_count, crude_heap_allocator_pack( gpu->allocator ) );
    CRUDE_ARRAY_INITIALIZE_WITH_CAPACITY( reflect->input.vertex_streams, spv_reflect.input_variable_count, crude_heap_allocator_pack( gpu->allocator ) );

    for ( uint32 input_index = 0; input_index < spv_reflect.input_variable_count; ++input_index )
    {
      SpvReflectInterfaceVariable const                   *spv_input;
      uint32                                               stride;

      spv_input = spv_reflect.input_variables[ input_index ];
      
      if ( spv_input->decoration_flags & SPV_REFLECT_DECORATION_BUILT_IN )
      {
        continue;
      }

      CRUDE_ARRAY_PUSH( reflect->input.vertex_attributes, CRUDE_COMPOUNT( crude_gfx_vertex_attribute, {
        .location = spv_input->location,
        .binding = spv_input->location,
        .offset = 0,
        .format = reflect_format_to_vk_format_( spv_input->format )
      } ) );
      
      stride = ( spv_input->numeric.vector.component_count * spv_input->numeric.scalar.width ) / 8;
      CRUDE_ARRAY_PUSH( reflect->input.vertex_streams, CRUDE_COMPOUNT( crude_gfx_vertex_stream, {
        .binding = spv_input->location,
        .stride = stride,
        .input_rate = CRUDE_GFX_VERTEX_INPUT_RATE_PER_VERTEX
      } ) );
    }
  }

  for ( uint32 set_index = 0; set_index < spv_reflect.descriptor_set_count; ++set_index )
  {
    SpvReflectDescriptorSet const                         *spv_descriptor_set;
    crude_gfx_descriptor_set_layout_creation              *set_layout;
    
    spv_descriptor_set = &spv_reflect.descriptor_sets[ set_index ];
    set_layout = &reflect->descriptor.sets[ spv_descriptor_set->set ];
    set_layout->set_index = spv_descriptor_set->set;
    set_layout->num_bindings = 0;

    for ( uint32 binding_index = 0; binding_index < spv_descriptor_set->binding_count; ++binding_index )
    {
      SpvReflectDescriptorBinding const                   *spv_binding;
      crude_gfx_descriptor_set_layout_binding             *binding;

      spv_binding = spv_descriptor_set->bindings[ binding_index ];

      if ( set_index != CRUDE_BINDLESS_DESCRIPTOR_SET_INDEX )
      {
        //CRUDE_ASSERT( spv_binding->accessed );
        //if ( !spv_binding->accessed )
        //{
        //  continue;
        //}
      }

      binding = &set_layout->bindings[ binding_index ];
      memset( binding, 0, sizeof( crude_gfx_descriptor_set_layout_binding ) );
      binding->start = spv_binding->binding;
      binding->count = spv_binding->count;
      
      switch ( spv_binding->descriptor_type )
      {
        case SPV_REFLECT_DESCRIPTOR_TYPE_UNIFORM_BUFFER:
        {
          reflect->descriptor.sets_count = CRUDE_MAX( reflect->descriptor.sets_count, ( spv_descriptor_set->set + 1 ) );
          binding->type = CRUDE_GFX_RHI_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
          ++set_layout->num_bindings;
          break;
        }
        case SPV_REFLECT_DESCRIPTOR_TYPE_STORAGE_BUFFER:
        {
          reflect->descriptor.sets_count = CRUDE_MAX( reflect->descriptor.sets_count, ( spv_descriptor_set->set + 1 ) );
          binding->type = CRUDE_GFX_RHI_DESCRIPTOR_TYPE_STORAGE_BUFFER;
          ++set_layout->num_bindings;
          break;
        }
        case SPV_REFLECT_DESCRIPTOR_TYPE_STORAGE_IMAGE:
        {
          reflect->descriptor.sets_count = CRUDE_MAX( reflect->descriptor.sets_count, ( spv_descriptor_set->set + 1 ) );
          binding->type = CRUDE_GFX_RHI_DESCRIPTOR_TYPE_STORAGE_IMAGE;
          ++set_layout->num_bindings;
          break;
        }
        case SPV_REFLECT_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER:
        {
          reflect->descriptor.sets_count = CRUDE_MAX( reflect->descriptor.sets_count, ( spv_descriptor_set->set + 1 ) );
          binding->type = CRUDE_GFX_RHI_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
          ++set_layout->num_bindings;
          break;
        }
        case SPV_REFLECT_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR:
        {
#if CRUDE_GFX_RAY_TRACING_ENABLED
          reflect->descriptor.sets_count = CRUDE_MAX( reflect->descriptor.sets_count, ( spv_descriptor_set->set + 1 ) );
          binding->type = CRUDE_GFX_RHI_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR;
          ++set_layout->num_bindings;
#endif
          break;
        }
      }
    }
  }

  spvReflectDestroyShaderModule( &spv_reflect );
}

void
crude_gfx_destroy_resources_instant_
(
  _In_ crude_gfx_device                                   *gpu,
  _In_ crude_gfx_resource_deletion_type                    type,
  _In_ crude_gfx_resource_index                            handle
)
{
  switch ( type )
  {
    case CRUDE_GFX_RESOURCE_DELETION_TYPE_SAMPLER:
    {
      crude_gfx_destroy_sampler_instant( gpu, CRUDE_COMPOUNT( crude_gfx_sampler_handle, { handle } ) );
      break;
    }
    case CRUDE_GFX_RESOURCE_DELETION_TYPE_TEXTURE:
    {
      crude_gfx_destroy_texture_instant( gpu, CRUDE_COMPOUNT( crude_gfx_texture_handle, { handle } ) );
      break;
    }
    case CRUDE_GFX_RESOURCE_DELETION_TYPE_RENDER_PASS:
    {
      crude_gfx_destroy_render_pass_instant( gpu, CRUDE_COMPOUNT( crude_gfx_render_pass_handle, { handle } ) );
      break;
    }
    case CRUDE_GFX_RESOURCE_DELETION_TYPE_SHADER_STATE:
    {
      crude_gfx_destroy_shader_state_instant( gpu, CRUDE_COMPOUNT( crude_gfx_shader_state_handle, { handle } ) );
      break;
    }
    case CRUDE_GFX_RESOURCE_DELETION_TYPE_PIPELINE:
    {
      crude_gfx_destroy_pipeline_instant( gpu, CRUDE_COMPOUNT( crude_gfx_pipeline_handle, { handle } ) );
      break;
    }
    case CRUDE_GFX_RESOURCE_DELETION_TYPE_BUFFER:
    {
      crude_gfx_destroy_buffer_instant( gpu, CRUDE_COMPOUNT( crude_gfx_buffer_handle, { handle } ) );
      break;
    }
    case CRUDE_GFX_RESOURCE_DELETION_TYPE_DESCRIPTOR_SET_LAYOUT:
    {
      crude_gfx_destroy_descriptor_set_layout_instant( gpu, CRUDE_COMPOUNT( crude_gfx_descriptor_set_layout_handle, { handle } ) );
      break;
    }
    case CRUDE_GFX_RESOURCE_DELETION_TYPE_DESCRIPTOR_SET:
    {
      crude_gfx_destroy_descriptor_set_instant( gpu, CRUDE_COMPOUNT( crude_gfx_descriptor_set_handle, { handle } ) );
      break;
    }
    case CRUDE_GFX_RESOURCE_DELETION_TYPE_FRAMEBUFFER:
    {
      crude_gfx_destroy_framebuffer_instant( gpu, CRUDE_COMPOUNT( crude_gfx_framebuffer_handle, { handle } ) );
      break;
    }
  }
}