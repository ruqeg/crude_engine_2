#pragma once

#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>

#include <core/string.h>
#include <core/assert.h>
#include <graphics/command_buffer.h>

/************************************************
 *
 * GPU Device Structs
 * 
 ***********************************************/

typedef struct crude_gfx_device_creation
{
  SDL_Window                                              *sdl_window;
  char const                                              *vk_application_name;
  uint32                                                   vk_application_version;
  crude_allocator_container                                allocator_container;
  crude_stack_allocator                                   *temporary_allocator;
  uint16                                                   queries_per_frame;
  uint16                                                   num_threads;
} crude_gfx_device_creation;

typedef struct crude_gfx_device                    
{
  SDL_Window                                              *sdl_window;
  uint32                                                   previous_frame;
  uint32                                                   current_frame;
  /**
   * Serves as the primary dynamic buffer.
   * This buffer acts as a foundation for mapping 
   * other buffers to its memory space.
   */
  crude_gfx_buffer_handle                                  dynamic_buffer;
  uint8                                                   *dynamic_mapped_memory;
  uint32                                                   dynamic_per_frame_size;
  uint32                                                   dynamic_allocated_size;
  uint32                                                   dynamic_max_per_frame_size;
  /**
   * Default sampler and texture references.
   * These fallback resources will be used when
   * sampler/texture is undefined. Ensures consistent
   * rendering behavior even with missing assets.
   */
  crude_gfx_sampler_handle                                 default_sampler;
  crude_gfx_texture_handle                                 depth_texture;
  /**
   * GPU Device resources memory pools.
   */
  crude_resource_pool                                      buffers;
  crude_resource_pool                                      textures;
  crude_resource_pool                                      pipelines;
  crude_resource_pool                                      samplers;
  crude_resource_pool                                      descriptor_set_layouts;
  crude_resource_pool                                      render_passes;
  crude_resource_pool                                      command_buffers;
  crude_resource_pool                                      shaders;
  crude_resource_pool                                      framebuffers;
  /**
   * Queue to remove or update bindless texture.
   */
  crude_gfx_resource_update                               *resource_deletion_queue;
  crude_gfx_resource_update                               *texture_to_update_bindless;
  /**
   * Stores current command buffers added to the queue.
   */
  crude_gfx_cmd_buffer                                   **queued_command_buffers;
  /**
   * Vulkan handles and additional data related to the
   * foundation of the renderer.
   */
  VkInstance                                               vk_instance;
  VkDebugUtilsMessengerEXT                                 vk_debug_utils_messenger;
  VkSurfaceKHR                                             vk_surface;
  VkSurfaceFormatKHR                                       vk_surface_format;
  VkPhysicalDevice                                         vk_physical_device;
  VkDevice                                                 vk_device;
  VkSwapchainKHR                                           vk_swapchain;
  VkSemaphore                                              vk_image_avalivable_semaphores[ CRUDE_GFX_MAX_SWAPCHAIN_IMAGES ];
  VkSemaphore                                              vk_rendering_finished_semaphore[ CRUDE_GFX_MAX_SWAPCHAIN_IMAGES ];
  VkSemaphore                                              vk_swapchain_updated_semaphore[ CRUDE_GFX_MAX_SWAPCHAIN_IMAGES ];
  VkFence                                                  vk_command_buffer_executed_fences[ CRUDE_GFX_MAX_SWAPCHAIN_IMAGES ];
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
  VkImage                                                  vk_swapchain_images[ CRUDE_GFX_MAX_SWAPCHAIN_IMAGES ];
  uint32                                                   vk_swapchain_images_count;
  uint16                                                   vk_swapchain_width;
  uint16                                                   vk_swapchain_height;
  uint32                                                   vk_swapchain_image_index;
  bool                                                     swapchain_resized_last_frame;
  /**
   * Descriptor pools/sets automatically generated
   * based on the reflection of the pipeline shaders.
   */
  VkDescriptorPool                                         vk_bindless_descriptor_pool;
  VkDescriptorSetLayout                                    vk_bindless_descriptor_set_layout;
  VkDescriptorSet                                          vk_bindless_descriptor_set;
  /**
   * //!TODO
   */
  VkQueryPool                                              vk_timestamp_query_pool;
  /**
   * Allocators and callbacks
   */
  VkAllocationCallbacks                                   *vk_allocation_callbacks;                               
  VmaAllocator                                             vma_allocator;
  crude_allocator_container                                allocator_container;
  crude_stack_allocator                                   *temporary_allocator;

  crude_string_buffer                                      objects_names_string_buffer;

  /**
   * UBO Buffers. //!TODO
   */
  crude_gfx_buffer_handle                                  frame_buffer;
  crude_gfx_buffer_handle                                  mesh_buffer;
  crude_gfx_render_pass_output                             swapchain_output;

  crude_gfx_cmd_buffer_manager                             cmd_buffer_manager;


  PFN_vkCmdBeginRenderingKHR                               vkCmdBeginRenderingKHR;
  PFN_vkCmdEndRenderingKHR                                 vkCmdEndRenderingKHR;
  PFN_vkCreateDebugUtilsMessengerEXT                       vkCreateDebugUtilsMessengerEXT;
  PFN_vkSetDebugUtilsObjectNameEXT                         vkSetDebugUtilsObjectNameEXT;
  PFN_vkDestroyDebugUtilsMessengerEXT                      vkDestroyDebugUtilsMessengerEXT;
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

CRUDE_API crude_gfx_cmd_buffer*
crude_gfx_get_secondary_cmd
(
  _In_ crude_gfx_device                                   *gpu,
  _In_ uint32                                              thread_index
);
                                                   
CRUDE_API void                                     
crude_gfx_queue_cmd                         
(                                                  
  _In_ crude_gfx_cmd_buffer                               *cmd
);
                                                   
CRUDE_API void*                                    
crude_gfx_map_buffer                               
(                                                  
  _In_ crude_gfx_device                                   *gpu,
  _In_ crude_gfx_map_buffer_parameters const              *parameters
);
                                                   
CRUDE_API void                                     
crude_gfx_unmap_buffer                             
(                                                  
  _In_ crude_gfx_device                                   *gpu,
  _In_ crude_gfx_buffer_handle                             handle
);
                                                   
CRUDE_API void*                                    
crude_gfx_dynamic_allocate                         
(                                                  
  _In_ crude_gfx_device                                   *gpu,
  _In_ uint32                                              size
);

CRUDE_API void
crude_gfx_link_texture_sampler
(
  _In_ crude_gfx_device                                   *gpu,
  _In_ crude_gfx_texture_handle                            texture,
  _In_ crude_gfx_sampler_handle                            sampler
);

CRUDE_API crude_gfx_descriptor_set_layout_handle
crude_gfx_get_descriptor_set_layout
(
  _In_ crude_gfx_device                                   *gpu,
  _In_ crude_gfx_pipeline_handle                           pipeline_handle,
  _In_ uint32                                              layout_index
);

CRUDE_API void
crude_gfx_query_buffer 
(
  _In_ crude_gfx_device                                   *gpu,
  _In_ crude_gfx_buffer_handle                             buffer,
  _Out_ crude_gfx_buffer_description                      *description
);

CRUDE_API bool
crude_gfx_buffer_ready
(
  _In_ crude_gfx_device                                   *gpu,
  _In_ crude_gfx_buffer_handle                             buffer_handle
);

CRUDE_API VkShaderModuleCreateInfo
crude_gfx_compile_shader
(
  _In_ char const                                         *code,
  _In_ uint32                                              code_size,
  _In_ VkShaderStageFlagBits                               stage,
  _In_ char const                                         *name,
  _In_ crude_stack_allocator                              *temporary_allocator
);

CRUDE_API VkShaderModuleCreateInfo
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