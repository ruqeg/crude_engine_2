#pragma once

#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>
#include <threads.h>

#include <engine/core/hashmapstr.h>
#include <engine/core/string.h>
#include <engine/core/assert.h>
#include <engine/graphics/command_buffer.h>
#include <engine/graphics/gpu_memory.h>
#include <engine/graphics/gpu_crash_tracker.h>

#include <engine/graphics/shaders/common/platform.h>

#if CRUDE_GFX_GPU_PROFILER
typedef struct crude_gfx_gpu_time_query crude_gfx_gpu_time_query;
typedef struct crude_gfx_gpu_time_query_tree crude_gfx_gpu_time_query_tree;
typedef struct crude_gfx_gpu_time_queries_manager crude_gfx_gpu_time_queries_manager;
#endif

typedef struct crude_gfx_device crude_gfx_device;

/************************************************
 *
 * GPU Device Structs
 * 
 ***********************************************/
typedef struct crude_gfx_resource_cache
{
  CRUDE_HASHMAPSTR( crude_gfx_technique* )                *techniques;
} crude_gfx_resource_cache;

typedef struct crude_gfx_cmd_buffer_manager
{
  crude_gfx_device                                        *gpu;

  crude_gfx_cmd_buffer                                    *primary_cmd_buffers;

  uint32                                                   num_pools_per_frame;
  uint32                                                   num_primary_cmd_buffers_per_thread;

  uint8                                                   *num_used_primary_cmd_buffers_per_frame;
} crude_gfx_cmd_buffer_manager;

typedef struct crude_gfx_device_creation
{
  SDL_Window                                              *sdl_window;
  char const                                              *vk_application_name;
  uint32                                                   vk_application_version;
  crude_heap_allocator                                    *allocator;
  crude_stack_allocator                                   *temporary_allocator;
  uint16                                                   queries_per_frame;
  uint16                                                   num_threads;
  
  char const                                              *working_absolute_directory;
  char const                                              *techniques_absolute_directory;
  char const                                              *compiled_shaders_absolute_directory;
  char const                                              *shaders_absolute_directory;
} crude_gfx_device_creation;

typedef struct crude_gfx_device
{
  char const                                              *working_absolute_directory;
  char const                                              *techniques_absolute_directory;
  char const                                              *compiled_shaders_absolute_directory;
  char const                                              *shaders_absolute_directory;

  SDL_Window                                              *sdl_window;
  uint32                                                   previous_frame;
  uint32                                                   current_frame;
  uint32                                                   absolute_frame;

  crude_gfx_linear_allocator                               frame_linear_allocator;
  /**
   * Default sampler and texture references.
   * These fallback resources will be used when
   * sampler/texture is undefined.
   */
  crude_gfx_sampler_handle                                 default_sampler;
  /**
   * GPU Device resources memory pools.
   */
  crude_resource_pool                                      buffers;
  crude_resource_pool                                      textures;
  crude_resource_pool                                      pipelines;
  crude_resource_pool                                      samplers;
  crude_resource_pool                                      descriptor_set_layouts;
  crude_resource_pool                                      descriptor_sets;
  crude_resource_pool                                      render_passes;
  crude_resource_pool                                      command_buffers;
  crude_resource_pool                                      shaders;
  crude_resource_pool                                      framebuffers;
  crude_resource_pool                                      techniques;
  crude_resource_pool                                      cmd_pools;
  crude_resource_pool                                      cmd_buffers;
  /**
   * High Level resoruces managment (material, technique, textures updating) 
   */
  crude_gfx_texture_handle                                 textures_to_update[ 128 ];
  uint32                                                   num_textures_to_update;
  mtx_t                                                    texture_update_mutex;
  crude_gfx_resource_cache                                 resource_cache;
  /**
   * Queue to remove or update bindless texture.
   */
  crude_gfx_resource_update                               *resource_deletion_queue;
  crude_gfx_resource_update                               *texture_to_update_bindless;
  /**
   * Stores current command buffers added to the queue.
   */
  crude_gfx_cmd_buffer                                   **queued_command_buffers;
  /*
   * 
   */
#if CRUDE_GFX_USE_NSIGHT_AFTERMATH
  crude_gfx_gpu_crash_tracker                              crash_tracker;
#endif
  /**
   * Additional data related to the foundation of the renderer.
   */
  VkInstance                                               vk_instance;
  VkDebugUtilsMessengerEXT                                 vk_debug_utils_messenger;
  VkSurfaceKHR                                             vk_surface;
  VkSurfaceFormatKHR                                       vk_surface_format;
  VkPhysicalDevice                                         vk_physical_device;
  VkDevice                                                 vk_device;
  VkSwapchainKHR                                           vk_swapchain;
  VkSemaphore                                              vk_graphics_semaphore;
  VkSemaphore                                              vk_image_avalivable_semaphores[ CRUDE_GFX_SWAPCHAIN_IMAGES_MAX_COUNT ];
  VkSemaphore                                              vk_rendering_finished_semaphore[ CRUDE_GFX_SWAPCHAIN_IMAGES_MAX_COUNT ];
  VkSemaphore                                              vk_swapchain_updated_semaphore[ CRUDE_GFX_SWAPCHAIN_IMAGES_MAX_COUNT ];
  VkDescriptorPool                                         vk_descriptor_pool;
  /**
   * Vulkan queues
   */
  VkQueue                                                  vk_main_queue;
  VkQueue                                                  vk_transfer_queue;
  uint32                                                   vk_main_queue_family;
  uint32                                                   vk_transfer_queue_family;
  /**
   * Additional data related to the swapchain.
   */
  VkImage                                                  vk_swapchain_images[ CRUDE_GFX_SWAPCHAIN_IMAGES_MAX_COUNT ];
  uint32                                                   vk_swapchain_images_count;
  XMFLOAT2                                                 vk_swapchain_size;
  uint32                                                   vk_swapchain_image_index;
  VkPresentModeKHR                                         vk_selected_present_mode;
  bool                                                     swapchain_resized_last_frame;
  crude_gfx_render_pass_output                             swapchain_output;
  /**
   * Descriptor pools/sets automatically generated
   * based on the reflection of the pipeline shaders.
   */
  VkDescriptorPool                                         vk_bindless_descriptor_pool;
  crude_gfx_descriptor_set_layout_handle                   bindless_descriptor_set_layout_handle;
  crude_gfx_descriptor_set_handle                          bindless_descriptor_set_handle;
  /**
   * Allocators and callbacks
   */
  VkAllocationCallbacks                                   *vk_allocation_callbacks;                               
  VmaAllocator                                             vma_allocator;
  crude_heap_allocator                                    *allocator;
  crude_allocator_container                                allocator_container;
  crude_stack_allocator                                   *temporary_allocator;

  /**
   * !TODO 
   */
  XMFLOAT2                                                 renderer_size;

  crude_gfx_cmd_buffer_manager                             cmd_buffer_manager;

  uint32                                                   num_threads;
  crude_gfx_gpu_thread_frame_pools                        *thread_frame_pools;
  float32                                                  gpu_timestamp_frequency;

  bool                                                     timestamps_enabled;

  bool                                                     mesh_shaders_extension_present;
  bool                                                     fragment_shading_rate_extension_present;
  bool                                                     deferred_host_operations_extension_present;
  bool                                                     shader_relaxed_extended_instruction_extension_present;

  VkFence                                                  vk_immediate_fence;

#if CRUDE_GFX_GPU_PROFILER
  crude_gfx_gpu_time_queries_manager                      *gpu_time_queries_manager;
#endif

  PFN_vkCmdDrawMeshTasksEXT                                vkCmdDrawMeshTasksEXT;
  PFN_vkCmdDrawMeshTasksIndirectCountEXT                   vkCmdDrawMeshTasksIndirectCountEXT;
  PFN_vkCmdBeginRenderingKHR                               vkCmdBeginRenderingKHR;
  PFN_vkCmdEndRenderingKHR                                 vkCmdEndRenderingKHR;
  PFN_vkCreateDebugUtilsMessengerEXT                       vkCreateDebugUtilsMessengerEXT;
  PFN_vkSetDebugUtilsObjectNameEXT                         vkSetDebugUtilsObjectNameEXT;
  PFN_vkDestroyDebugUtilsMessengerEXT                      vkDestroyDebugUtilsMessengerEXT;
  PFN_vkCmdPipelineBarrier2KHR                             vkCmdPipelineBarrier2KHR;
  PFN_vkQueueSubmit2KHR                                    vkQueueSubmit2KHR;
  PFN_vkGetBufferDeviceAddressKHR                          vkGetBufferDeviceAddressKHR;

#if CRUDE_GRAPHICS_VALIDATION_LAYERS_ENABLED
  PFN_vkCmdBeginDebugUtilsLabelEXT                         vkCmdBeginDebugUtilsLabelEXT;
  PFN_vkCmdEndDebugUtilsLabelEXT                           vkCmdEndDebugUtilsLabelEXT;
#endif /* CRUDE_GRAPHICS_VALIDATION_LAYERS_ENABLED */

#if CRUDE_GFX_RAY_TRACING_ENABLED
  VkPhysicalDeviceRayTracingPipelinePropertiesKHR          ray_tracing_pipeline_properties;

  PFN_vkGetAccelerationStructureBuildSizesKHR              vkGetAccelerationStructureBuildSizesKHR;
  PFN_vkCreateAccelerationStructureKHR                     vkCreateAccelerationStructureKHR;
  PFN_vkCmdBuildAccelerationStructuresKHR                  vkCmdBuildAccelerationStructuresKHR;
  PFN_vkGetAccelerationStructureDeviceAddressKHR           vkGetAccelerationStructureDeviceAddressKHR;
  PFN_vkCreateRayTracingPipelinesKHR                       vkCreateRayTracingPipelinesKHR;
  PFN_vkGetRayTracingShaderGroupHandlesKHR                 vkGetRayTracingShaderGroupHandlesKHR;
  PFN_vkCmdTraceRaysKHR                                    vkCmdTraceRaysKHR;
  PFN_vkDestroyAccelerationStructureKHR                    vkDestroyAccelerationStructureKHR;
#endif /* CRUDE_GFX_RAY_TRACING_ENABLED */
} crude_gfx_device;                                

/************************************************
 *
 * GPU Device Initialize/Deinitialize
 * 
 ***********************************************/
CRUDE_API void                                     
crude_gfx_device_initialize                    
(                                                  
  _Out_ crude_gfx_device                                  *gpu,
  _In_ crude_gfx_device_creation                          *creation
);

CRUDE_API void                                     
crude_gfx_device_deinitialize
(
  _In_ crude_gfx_device                                   *gpu
);

/************************************************
 *
 * GPU Device Common Functions
 * 
 ***********************************************/
CRUDE_API void                                     
crude_gfx_set_resource_name                        
(
  _In_ crude_gfx_device                                   *gpu,
  _In_ VkObjectType                                        type,
  _In_ uint64                                              handle,
  _In_ char const                                         *name
);
                                                   
CRUDE_API void                                     
crude_gfx_new_frame                                
(                                                  
  _In_ crude_gfx_device                                   *gpu
);
                                                   
CRUDE_API void                                     
crude_gfx_present                                  
(                                                  
  _In_ crude_gfx_device                                   *gpu,
  _In_ crude_gfx_texture                                  *texture
);
                                                   
CRUDE_API crude_gfx_cmd_buffer*                    
crude_gfx_get_primary_cmd
(
  _In_ crude_gfx_device                                   *gpu,
  _In_ uint32                                              thread_index,
  _In_ bool                                                begin
);

CRUDE_API void                                     
crude_gfx_queue_cmd
(                                                  
  _In_ crude_gfx_device                                   *gpu,
  _In_ crude_gfx_cmd_buffer                               *cmd
);

CRUDE_API void
crude_gfx_link_texture_sampler
(
  _In_ crude_gfx_device                                   *gpu,
  _In_ crude_gfx_texture_handle                            texture_handle,
  _In_ crude_gfx_sampler_handle                            sampler_handle
);

CRUDE_API crude_gfx_descriptor_set_layout_handle
crude_gfx_get_descriptor_set_layout
(
  _In_ crude_gfx_device                                   *gpu,
  _In_ crude_gfx_pipeline_handle                           pipeline_handle,
  _In_ uint32                                              layout_index
);

CRUDE_API bool
crude_gfx_buffer_ready
(
  _In_ crude_gfx_device                                   *gpu,
  _In_ crude_gfx_buffer_handle                             buffer_handle
);

CRUDE_API bool
crude_gfx_texture_ready
(
  _In_ crude_gfx_device                                   *gpu,
  _In_ crude_gfx_texture_handle                            texture_handle
);

CRUDE_API VkShaderModuleCreateInfo
crude_gfx_read_shader
(
  _In_ crude_gfx_device                                   *gpu,
  _In_ char const                                         *name,
  _In_ VkShaderStageFlagBits                               stage,
  _In_ crude_stack_allocator                              *temporary_allocator
);

CRUDE_API VkShaderModuleCreateInfo
crude_gfx_compile_shader
(
  _In_ crude_gfx_device                                   *gpu,
  _In_ char const                                         *code,
  _In_ uint32                                              code_size,
  _In_ VkShaderStageFlagBits                               stage,
  _In_ char const                                         *name,
  _In_ crude_stack_allocator                              *temporary_allocator
);

CRUDE_API void
crude_gfx_resize_framebuffer
(
  _In_ crude_gfx_device                                   *gpu,
  _In_ crude_gfx_framebuffer_handle                        framebuffer_handle,
  _In_ uint32                                              width,
  _In_ uint32                                              height
);

CRUDE_API void
crude_gfx_resize_texture
(
  _In_ crude_gfx_device                                   *gpu,
  _In_ crude_gfx_texture_handle                            texture_handle,
  _In_ uint32                                              width,
  _In_ uint32                                              height
);

CRUDE_API void                                     
crude_gfx_device_queue_submit
(
  _In_ crude_gfx_device                                   *gpu,
  _In_ VkQueue                                             vk_queue,
  _In_ VkSubmitInfo2                                      *vk_submit_info,
  _In_ VkFence                                             vk_fence
);

#if CRUDE_GFX_GPU_PROFILER
CRUDE_API uint32
crude_gfx_copy_gpu_timestamps
(
  _In_ crude_gfx_device                                   *gpu,
  _Out_ crude_gfx_gpu_time_query                          *timestamps
);
#endif

CRUDE_API void
crude_gfx_gpu_set_timestamps_enable
(
  _In_ crude_gfx_device                                   *gpu,
  _In_ bool                                                value
);

CRUDE_API void                                     
crude_gfx_generate_mipmaps
(
  _In_ crude_gfx_cmd_buffer                               *cmd_buffer,
  _In_ crude_gfx_texture                                  *texture
);

CRUDE_API VkDeviceAddress
crude_gfx_get_buffer_device_address
(
  _In_ crude_gfx_device                                   *gpu,
  _In_ crude_gfx_buffer_handle                             handle
);

CRUDE_API void 
crude_gfx_submit_immediate
(
  _In_ crude_gfx_cmd_buffer                               *cmd
);

CRUDE_API void
crude_gfx_add_texture_to_update
(
  _In_ crude_gfx_device                                   *gpu,
  _In_ crude_gfx_texture_handle                            texture
);

CRUDE_API void
crude_gfx_add_texture_update_commands
(
  _In_ crude_gfx_device                                   *gpu,
  _In_ crude_gfx_cmd_buffer                               *cmd
);

CRUDE_API crude_gfx_technique*
crude_gfx_access_technique_by_name
(
  _In_ crude_gfx_device                                   *gpu,
  _In_ char const                                         *technique_name
);

CRUDE_API crude_gfx_technique_pass*
crude_gfx_access_technique_pass_by_name
(
  _In_ crude_gfx_device                                   *gpu,
  _In_ char const                                         *technique_name,
  _In_ char const                                         *pass_name
);

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
);

CRUDE_API void
crude_gfx_cmd_deinitialize
(
  _In_ crude_gfx_cmd_buffer                               *cmd
);

CRUDE_API void
crude_gfx_cmd_reset
(
  _In_ crude_gfx_cmd_buffer                               *cmd
);

CRUDE_API void
crude_gfx_cmd_begin_primary
(
  _In_ crude_gfx_cmd_buffer                               *cmd
);

CRUDE_API void
crude_gfx_cmd_end
(
  _In_ crude_gfx_cmd_buffer                               *cmd
);

CRUDE_API void
crude_gfx_cmd_end_render_pass
(
  _In_ crude_gfx_cmd_buffer                               *cmd
);

CRUDE_API void
crude_gfx_cmd_bind_render_pass
(
  _In_ crude_gfx_cmd_buffer                               *cmd,
  _In_ crude_gfx_render_pass_handle                        render_pass_handle,
  _In_ crude_gfx_framebuffer_handle                        framebuffer_handle,
  _In_ bool                                                use_secondary
);

CRUDE_API void
crude_gfx_cmd_bind_pipeline
(
  _In_ crude_gfx_cmd_buffer                               *cmd,
  _In_ crude_gfx_pipeline_handle                           handle
);

CRUDE_API void
crude_gfx_cmd_copy_texture
(
  _In_ crude_gfx_cmd_buffer                               *cmd,
  _In_ crude_gfx_texture_handle                            src_handle,
  _In_ crude_gfx_texture_handle                            dst_handle
);

CRUDE_API void
crude_gfx_cmd_set_viewport
(
  _In_ crude_gfx_cmd_buffer                               *cmd,
  _In_opt_ crude_gfx_viewport const                       *dev_viewport
);

CRUDE_API void
crude_gfx_cmd_set_clear_color_f32
(
  _In_ crude_gfx_cmd_buffer                               *cmd,
  _In_ float32                                             r,
  _In_ float32                                             g,
  _In_ float32                                             b,
  _In_ float32                                             a,
  _In_ uint32                                              index
);

CRUDE_API void
crude_gfx_cmd_set_clear_depth_and_stencil
(
  _In_ crude_gfx_cmd_buffer                               *cmd,
  _In_ float32                                             depth,
  _In_ float32                                             stencil
);

CRUDE_API void
crude_gfx_cmd_set_scissor
(
  _In_ crude_gfx_cmd_buffer                               *cmd,
  _In_opt_ crude_gfx_rect2d_int const                     *rect
);


CRUDE_API void
crude_gfx_cmd_draw
(
  _In_ crude_gfx_cmd_buffer                               *cmd,
  _In_ uint32                                              first_vertex,
  _In_ uint32                                              vertex_count,
  _In_ uint32                                              first_instance,
  _In_ uint32                                              instance_count
);

CRUDE_API void
crude_gfx_cmd_draw_inderect
(
  _In_ crude_gfx_cmd_buffer                               *cmd,
  _In_ crude_gfx_buffer_handle                             buffer_handle,
  _In_ uint32                                              offset,
  _In_ uint32                                              draw_count,
  _In_ uint32                                              stride
);

CRUDE_API void
crude_gfx_cmd_draw_indirect_count
(
  _In_ crude_gfx_cmd_buffer                               *cmd,
  _In_ crude_gfx_buffer_handle                             argument_buffer_handle,
  _In_ uint32                                              argument_offset,
  _In_ crude_gfx_buffer_handle                             count_buffer_handle,
  _In_ uint32                                              count_offset,
  _In_ uint32                                              max_draws,
  _In_ uint32                                              stride
);

CRUDE_API void
crude_gfx_cmd_draw_mesh_task
(
  _In_ crude_gfx_cmd_buffer                               *cmd,
  _In_ uint32                                              group_count_x,
  _In_ uint32                                              group_count_y,
  _In_ uint32                                              group_count_z
);

CRUDE_API void
crude_gfx_cmd_draw_mesh_task_indirect_count
(
  _In_ crude_gfx_cmd_buffer                               *cmd,
  _In_ crude_gfx_buffer_handle                             argument_buffer_handle,
  _In_ uint32                                              argument_offset,
  _In_ crude_gfx_buffer_handle                             count_buffer_handle,
  _In_ uint32                                              count_offset,
  _In_ uint32                                              max_draws,
  _In_ uint32                                              stride
);

CRUDE_API void
crude_gfx_cmd_dispatch
(
  _In_ crude_gfx_cmd_buffer                               *cmd,
  _In_ uint32                                              group_count_x,
  _In_ uint32                                              group_count_y,
  _In_ uint32                                              group_count_z
);

CRUDE_API void
crude_gfx_cmd_bind_bindless_descriptor_set
(
  _In_ crude_gfx_cmd_buffer                               *cmd
);

CRUDE_API void
crude_gfx_cmd_add_buffer_barrier
(
  _In_ crude_gfx_cmd_buffer                               *cmd,
  _In_ crude_gfx_buffer_handle                             buffer_handle,
  _In_ crude_gfx_resource_state                            old_state,
  _In_ crude_gfx_resource_state                            new_state
);

CRUDE_API void
crude_gfx_cmd_add_image_barrier
(
  _In_ crude_gfx_cmd_buffer                               *cmd,
  _In_ crude_gfx_texture                                  *texture,
  _In_ crude_gfx_resource_state                            new_state,
  _In_ uint32                                              base_mip_level,
  _In_ uint32                                              mip_count,
  _In_ bool                                                is_depth
);

CRUDE_API void
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
);

CRUDE_API void
crude_gfx_cmd_add_image_barrier_ext2
(
  _In_ crude_gfx_cmd_buffer                               *cmd,
  _In_ VkImage                                             vk_image,
  _In_ crude_gfx_resource_state                            old_state,
  _In_ crude_gfx_resource_state                            new_state,
  _In_ uint32                                              base_mip_level,
  _In_ uint32                                              mip_count,
  _In_ bool                                                is_depth
);

CRUDE_API void
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
);

CRUDE_API void
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
);

CRUDE_API void
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
);

CRUDE_API void
crude_gfx_cmd_global_debug_barrier
(
  _In_ crude_gfx_cmd_buffer                               *cmd
);

CRUDE_API void
crude_gfx_cmd_memory_copy_to_texture
(
  _In_ crude_gfx_cmd_buffer                               *cmd,
  _In_ crude_gfx_texture_handle                            texture_handle,
  _In_ crude_gfx_memory_allocation                         memory_allocation
);

CRUDE_API void
crude_gfx_cmd_memory_copy
(
  _In_ crude_gfx_cmd_buffer                               *cmd,
  _In_ crude_gfx_memory_allocation                         src_memory_allocation,
  _In_ crude_gfx_memory_allocation                         dst_memory_allocation,
  _In_ uint64                                              src_offset,
  _In_ uint64                                              dst_offset
);

CRUDE_API void
crude_gfx_cmd_push_marker
(
  _In_ crude_gfx_cmd_buffer                               *cmd,
  _In_ char const                                         *name
);

CRUDE_API void
crude_gfx_cmd_pop_marker
(
  _In_ crude_gfx_cmd_buffer                               *cmd
);

CRUDE_API void
crude_gfx_cmd_push_constant
(
  _In_ crude_gfx_cmd_buffer                               *cmd,
  _In_ void const                                         *data,
  _In_ uint64                                              size
);

CRUDE_API void
crude_gfx_cmd_fill_buffer
(
  _In_ crude_gfx_cmd_buffer                               *cmd,
  _In_ crude_gfx_buffer_handle                             handle,
  _In_ uint32                                              value
);

CRUDE_API void
crude_gfx_cmd_trace_rays
(
  _In_ crude_gfx_cmd_buffer                               *cmd,
  _In_ crude_gfx_pipeline_handle                           pipeline_handle,
  _In_ uint32                                              width,
  _In_ uint32                                              height,
  _In_ uint32                                              depth
);

/************************************************
 *
 * Command Buffer Manager Functions
 * 
 ***********************************************/
CRUDE_API void
crude_gfx_cmd_manager_initialize
(
  _In_ crude_gfx_cmd_buffer_manager                       *cmd_manager,
  _In_ crude_gfx_device                                   *gpu,
  _In_ uint32                                              num_pools_per_frame,
  _In_ uint32                                              num_primary_cmd_buffers_per_pool
);

CRUDE_API void
crude_gfx_cmd_manager_deinitialize
(
  _In_ crude_gfx_cmd_buffer_manager                       *cmd_manager
);

CRUDE_API void
crude_gfx_cmd_manager_reset
(
  _In_ crude_gfx_cmd_buffer_manager                       *cmd_manager,
  _In_ uint32                                              frame
);

CRUDE_API crude_gfx_cmd_buffer*
crude_gfx_cmd_manager_get_primary_cmd
(
  _In_ crude_gfx_cmd_buffer_manager                       *cmd_manager,
  _In_ uint32                                              frame,
  _In_ uint32                                              thread_index,
  _In_ bool                                                begin
);

/************************************************
 *
 * GPU Device Resources Functions
 * 
 ***********************************************/
CRUDE_API crude_gfx_sampler_handle                     
crude_gfx_create_sampler                           
(                                                  
  _In_ crude_gfx_device                                   *gpu,
  _In_ crude_gfx_sampler_creation const                   *creation
);                                                 
                                                   
CRUDE_API void                                     
crude_gfx_destroy_sampler                          
(                                                  
  _In_ crude_gfx_device                                   *gpu,
  _In_ crude_gfx_sampler_handle                            handle
);                                                 
                                                   
CRUDE_API void                                     
crude_gfx_destroy_sampler_instant                  
(                                                  
  _In_ crude_gfx_device                                   *gpu,
  _In_ crude_gfx_sampler_handle                            handle
);

CRUDE_API crude_gfx_texture_handle                     
crude_gfx_create_texture                           
(                                                  
  _In_ crude_gfx_device                                   *gpu,
  _In_ crude_gfx_texture_creation const                   *creation
);

CRUDE_API void
crude_gfx_destroy_texture
(
  _In_ crude_gfx_device                                   *gpu,
  _In_ crude_gfx_texture_handle                            handle
);

CRUDE_API void                                      
crude_gfx_destroy_texture_instant                   
(                                                   
  _In_ crude_gfx_device                                   *gpu,
  _In_ crude_gfx_texture_handle                            handle
);

CRUDE_API crude_gfx_texture_handle                     
crude_gfx_create_texture_view
(                                                  
  _In_ crude_gfx_device                                   *gpu,
  _In_ crude_gfx_texture_view_creation const              *creation
);

CRUDE_API crude_gfx_shader_state_handle                 
crude_gfx_create_shader_state                       
(                                                   
  _In_ crude_gfx_device                                   *gpu,
  _In_ crude_gfx_shader_state_creation const              *creation
);

CRUDE_API void                                      
crude_gfx_destroy_shader_state                      
(                                                   
  _In_ crude_gfx_device                                   *gpu,
  _In_ crude_gfx_shader_state_handle                       handle
); 
                                                    
CRUDE_API void                                      
crude_gfx_destroy_shader_state_instant              
(                                                   
  _In_ crude_gfx_device                                   *gpu,
  _In_ crude_gfx_shader_state_handle                       handle
);

CRUDE_API crude_gfx_render_pass_handle                  
crude_gfx_create_render_pass                        
(                                                   
  _In_ crude_gfx_device                                   *gpu,
  _In_ crude_gfx_render_pass_creation const               *creation
);
                                                       
CRUDE_API void                                      
crude_gfx_destroy_render_pass                       
(                                                   
  _In_ crude_gfx_device                                   *gpu,
  _In_ crude_gfx_render_pass_handle                        handle
);                                                  

CRUDE_API void                                      
crude_gfx_destroy_render_pass_instant               
(                                                   
  _In_ crude_gfx_device                                   *gpu,
  _In_ crude_gfx_render_pass_handle                        handle
);

CRUDE_API crude_gfx_pipeline_handle                     
crude_gfx_create_pipeline                           
(                                                   
  _In_ crude_gfx_device                                   *gpu,
  _In_ crude_gfx_pipeline_creation const                  *creation
);

CRUDE_API void                                      
crude_gfx_destroy_pipeline                          
(                                                   
  _In_ crude_gfx_device                                   *gpu,
  _In_ crude_gfx_pipeline_handle                           handle
);                                                  
                                                    
CRUDE_API void                                      
crude_gfx_destroy_pipeline_instant                  
(                                                   
  _In_ crude_gfx_device                                   *gpu,
  _In_ crude_gfx_pipeline_handle                           handle
);                             
                                               
CRUDE_API crude_gfx_buffer_handle                       
crude_gfx_create_buffer                             
(                                                   
  _In_ crude_gfx_device                                   *gpu,
  _In_ crude_gfx_buffer_creation const                    *creation
);

CRUDE_API void                                      
crude_gfx_destroy_buffer                            
(                                                   
  _In_ crude_gfx_device                                   *gpu,
  _In_ crude_gfx_buffer_handle                             handle
);                                                  
                                                    
CRUDE_API void                                      
crude_gfx_destroy_buffer_instant                    
(                                                   
  _In_ crude_gfx_device                                   *gpu,
  _In_ crude_gfx_buffer_handle                             handle
);

CRUDE_API crude_gfx_descriptor_set_layout_handle
crude_gfx_create_descriptor_set_layout
(
  _In_ crude_gfx_device                                   *gpu,
  _In_ crude_gfx_descriptor_set_layout_creation const     *creation
);

CRUDE_API void                                      
crude_gfx_destroy_descriptor_set_layout
(                                                   
  _In_ crude_gfx_device                                   *gpu,
  _In_ crude_gfx_descriptor_set_layout_handle              handle
);

CRUDE_API void
crude_gfx_destroy_descriptor_set_layout_instant
(                                                   
  _In_ crude_gfx_device                                   *gpu,
  _In_ crude_gfx_descriptor_set_layout_handle              handle
);

CRUDE_API crude_gfx_descriptor_set_handle
crude_gfx_create_descriptor_set
(
  _In_ crude_gfx_device                                   *gpu,
  _In_ crude_gfx_descriptor_set_creation const            *creation
);

CRUDE_API void                                      
crude_gfx_destroy_descriptor_set
(                                                   
  _In_ crude_gfx_device                                   *gpu,
  _In_ crude_gfx_descriptor_set_handle                     handle
);

CRUDE_API void
crude_gfx_destroy_descriptor_set_instant
(                                                   
  _In_ crude_gfx_device                                   *gpu,
  _In_ crude_gfx_descriptor_set_handle                     handle
);

CRUDE_API crude_gfx_framebuffer_handle
crude_gfx_create_framebuffer
(
  _In_ crude_gfx_device                                   *gpu,
  _In_ crude_gfx_framebuffer_creation const               *creation
);   

CRUDE_API void                                      
crude_gfx_destroy_framebuffer
(                                                   
  _In_ crude_gfx_device                                   *gpu,
  _In_ crude_gfx_framebuffer_handle                        handle
);

CRUDE_API void
crude_gfx_destroy_framebuffer_instant
(                                                   
  _In_ crude_gfx_device                                   *gpu,
  _In_ crude_gfx_framebuffer_handle                        handle
);

CRUDE_API crude_gfx_cmd_pool_handle
crude_gfx_create_cmd_pool
(
  _In_ crude_gfx_device                                   *gpu,
  _In_ crude_gfx_cmd_pool_creation const                  *creation
);

CRUDE_API void
crude_gfx_destroy_cmd_pool_instant
(
  _In_ crude_gfx_device                                   *gpu,
  _In_ crude_gfx_cmd_pool_handle                           handle
);

CRUDE_API crude_gfx_cmd_buffer_handle
crude_gfx_create_cmd_buffer
(
  _In_ crude_gfx_device                                   *gpu,
  _In_ crude_gfx_cmd_buffer_creation const                *creation
);

CRUDE_API void
crude_gfx_destroy_cmd_buffer_instant
(
  _In_ crude_gfx_device                                   *gpu,
  _In_ crude_gfx_cmd_buffer_handle                         handle
);

CRUDE_API crude_gfx_technique*
crude_gfx_create_technique
(
  _In_ crude_gfx_device                                   *gpu,
  _In_ crude_gfx_technique_creation const                 *creation
);

CRUDE_API void
crude_gfx_destroy_technique_instant
(
  _In_ crude_gfx_device                                   *gpu,
  _In_ crude_gfx_technique                                *technique
);

/************************************************
 *
 * GPU Device Resources Pools Functions
 * 
 ***********************************************/
CRUDE_API crude_gfx_sampler_handle
crude_gfx_obtain_sampler
(
  _In_ crude_gfx_device                                   *gpu
);

CRUDE_API crude_gfx_sampler*
crude_gfx_access_sampler
(
  _In_ crude_gfx_device                                   *gpu,
  _In_ crude_gfx_sampler_handle                            handle
);

CRUDE_API void
crude_gfx_release_sampler
(
  _In_ crude_gfx_device                                   *gpu,
  _In_ crude_gfx_sampler_handle                            handle
);

CRUDE_API crude_gfx_texture_handle
crude_gfx_obtain_texture
(
  _In_ crude_gfx_device                                   *gpu
);

CRUDE_API crude_gfx_texture*
crude_gfx_access_texture
(
  _In_ crude_gfx_device                                   *gpu,
  _In_ crude_gfx_texture_handle                            handle
);

CRUDE_API void
crude_gfx_release_texture
(
  _In_ crude_gfx_device                                   *gpu,
  _In_ crude_gfx_texture_handle                            handle
);

CRUDE_API crude_gfx_render_pass_handle
crude_gfx_obtain_render_pass
(
  _In_ crude_gfx_device                                   *gpu
);

CRUDE_API crude_gfx_render_pass*
crude_gfx_access_render_pass
(
  _In_ crude_gfx_device                                   *gpu,
  _In_ crude_gfx_render_pass_handle                        handle
);

CRUDE_API void
crude_gfx_release_render_pass
(
  _In_ crude_gfx_device                                   *gpu,
  _In_ crude_gfx_render_pass_handle                        handle
);

CRUDE_API crude_gfx_shader_state_handle
crude_gfx_obtain_shader_state
(
  _In_ crude_gfx_device                                   *gpu
);

CRUDE_API crude_gfx_shader_state*
crude_gfx_access_shader_state
(
  _In_ crude_gfx_device                                   *gpu,
  _In_ crude_gfx_shader_state_handle                       handle
);

CRUDE_API void
crude_gfx_release_shader_state
(
  _In_ crude_gfx_device                                   *gpu,
  _In_ crude_gfx_shader_state_handle                       handle
);

CRUDE_API crude_gfx_pipeline_handle
crude_gfx_obtain_pipeline
(
  _In_ crude_gfx_device                                   *gpu
);

CRUDE_API crude_gfx_pipeline*
crude_gfx_access_pipeline
(
  _In_ crude_gfx_device                                   *gpu,
  _In_ crude_gfx_pipeline_handle                           handle
);

CRUDE_API void
crude_gfx_release_pipeline
(
  _In_ crude_gfx_device                                   *gpu,
  _In_ crude_gfx_pipeline_handle                           handle
);

CRUDE_API crude_gfx_buffer_handle
crude_gfx_obtain_buffer
(
  _In_ crude_gfx_device                                   *gpu
);

CRUDE_API crude_gfx_buffer*
crude_gfx_access_buffer
(
  _In_ crude_gfx_device                                   *gpu,
  _In_ crude_gfx_buffer_handle                             handle
);

CRUDE_API void
crude_gfx_release_buffer
(
  _In_ crude_gfx_device                                   *gpu,
  _In_ crude_gfx_buffer_handle                             handle
);

CRUDE_API crude_gfx_descriptor_set_layout_handle
crude_gfx_obtain_descriptor_set_layout
(
  _In_ crude_gfx_device                                   *gpu
);

CRUDE_API crude_gfx_descriptor_set_layout*
crude_gfx_access_descriptor_set_layout
(
  _In_ crude_gfx_device                                   *gpu,
  _In_ crude_gfx_descriptor_set_layout_handle              handle
);

CRUDE_API void
crude_gfx_release_descriptor_set_layout
(
  _In_ crude_gfx_device                                   *gpu,
  _In_ crude_gfx_descriptor_set_layout_handle              handle
);

CRUDE_API crude_gfx_descriptor_set_handle
crude_gfx_obtain_descriptor_set
(
  _In_ crude_gfx_device                                   *gpu
);

CRUDE_API crude_gfx_descriptor_set*
crude_gfx_access_descriptor_set
(
  _In_ crude_gfx_device                                   *gpu,
  _In_ crude_gfx_descriptor_set_handle                     handle
);

CRUDE_API void
crude_gfx_release_descriptor_set
(
  _In_ crude_gfx_device                                   *gpu,
  _In_ crude_gfx_descriptor_set_handle                     handle
);

CRUDE_API crude_gfx_framebuffer_handle
crude_gfx_obtain_framebuffer
(
  _In_ crude_gfx_device                                   *gpu
);

CRUDE_API crude_gfx_framebuffer*
crude_gfx_access_framebuffer
(
  _In_ crude_gfx_device                                   *gpu,
  _In_ crude_gfx_framebuffer_handle                        handle
);

CRUDE_API void
crude_gfx_release_framebuffer
(
  _In_ crude_gfx_device                                   *gpu,
  _In_ crude_gfx_framebuffer_handle                        handle
);

CRUDE_API crude_gfx_cmd_pool*
crude_gfx_access_cmd_pool
(
  _In_ crude_gfx_device                                   *gpu,
  _In_ crude_gfx_cmd_pool_handle                           handle
);

CRUDE_API crude_gfx_cmd_buffer*
crude_gfx_access_cmd_buffer
(
  _In_ crude_gfx_device                                   *gpu,
  _In_ crude_gfx_cmd_buffer_handle                         handle
);

CRUDE_API crude_gfx_technique*
crude_gfx_access_technique
(
  _In_ crude_gfx_device                                   *gpu,
  _In_ crude_gfx_technique_handle                          handle
);

CRUDE_API void
crude_gfx_release_technique
(
  _In_ crude_gfx_device                                   *gpu,
  _In_ crude_gfx_technique_handle                          handle
);

/************************************************
 *
 * GPU Device Utils
 * 
 ***********************************************/ 
#define CRUDE_GFX_HANDLE_VULKAN_RESULT( result, ... )\
{\
  if ( result != VK_SUCCESS )\
  {\
    CRUDE_ABORT( CRUDE_CHANNEL_GRAPHICS, "vulkan result isn't success: %i %s", result, ##__VA_ARGS__ );\
  }\
}