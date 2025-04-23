#pragma once

#include <vulkan/vulkan.h>
#include <SDL3/SDL.h>

#include <core/alias.h>
#include <graphics/gpu_resources.h>
#include <graphics/command_buffer.h>

/************************************************
 *
 * GPU Device Structs
 * 
 ***********************************************/

typedef struct crude_gpu_device_creation
{
  SDL_Window                                        *sdl_window;
  char const                                        *vk_application_name;
  uint32                                             vk_application_version;
  crude_allocator                                    allocator;
  uint16                                             queries_per_frame;
  uint16                                             max_frames;
} crude_gpu_device_creation;

typedef struct crude_gpu_device                    
{
  /**
   * Holds the window for the surface.
   */
  SDL_Window                                        *sdl_window;
  /**
   * The maximum/current/previous swapchain frame index.
   */
  uint16                                             max_frames;
  uint32                                             previous_frame;
  uint32                                             current_frame;
  /**
   * The swapchain render pass and swapchain render
   * pass output.
   */
  crude_render_pass_handle                           swapchain_pass;
  crude_render_pass_output                           swapchain_output;
  /**
   * Serves as the primary dynamic buffer.
   * This buffer acts as a foundation for mapping 
   * other buffers to its memory space.
   */
  crude_buffer_handle                                dynamic_buffer;
  uint8                                             *dynamic_mapped_memory;
  uint32                                             dynamic_per_frame_size;
  uint32                                             dynamic_allocated_size;
  uint32                                             dynamic_max_per_frame_size;
  /**
   * Default sampler and texture references.
   * These fallback resources will be used when
   * sampler/texture is undefined. Ensures consistent
   * rendering behavior even with missing assets.
   */
  crude_sampler_handle                               default_sampler;
  crude_texture_handle                               depth_texture;
  /**
   * GPU Device resources memory pools.
   * For implementation details and usage guidelines
   * refer to "GPU Device Resource Macros"
   */
  crude_resource_pool                                buffers;
  crude_resource_pool                                textures;
  crude_resource_pool                                pipelines;
  crude_resource_pool                                samplers;
  crude_resource_pool                                descriptor_set_layouts;
  crude_resource_pool                                render_passes;
  crude_resource_pool                                command_buffers;
  crude_resource_pool                                shaders;
  /**
   * Queue to remove or update bindless texture.
   * Handle can be added to queue using the
   * CRUDE_RELEASE_RESOURCE macro.
   * Object will be released after the frame
   * is present.
   */
  crude_resource_update                             *resource_deletion_queue;
  crude_resource_update                             *texture_to_update_bindless;
  /**
   * Stores current command buffers added to the queue.
   */
  uint32                                             queued_command_buffers_count;
  crude_command_buffer                             **queued_command_buffers;
  /**
   * Vulkan handles and additional data related to the
   * foundation of the renderer.
   */
  VkInstance                                         vk_instance;
  VkDebugUtilsMessengerEXT                           vk_debug_utils_messenger;
  VkSurfaceKHR                                       vk_surface;
  VkSurfaceFormatKHR                                 vk_surface_format;
  VkPhysicalDevice                                   vk_physical_device;
  VkDevice                                           vk_device;
  VkSwapchainKHR                                     vk_swapchain;
  VkSemaphore                                        vk_render_finished_semaphores[ CRUDE_MAX_SWAPCHAIN_IMAGES ];
  VkSemaphore                                        vk_image_avalivable_semaphores[ CRUDE_MAX_SWAPCHAIN_IMAGES ];
  VkFence                                            vk_command_buffer_executed_fences[ CRUDE_MAX_SWAPCHAIN_IMAGES ];
  /**
   * Vulkan queues
   */
  VkQueue                                            vk_main_queue;
  VkQueue                                            vk_transfer_queue;
  uint32                                             vk_main_queue_family;
  uint32                                             vk_transfer_queue_family;
  /**
   * Vulkan handles and additional data related to
   * the swapchain.
   */
  uint32                                             vk_swapchain_images_count;
  VkImage                                            vk_swapchain_images[ CRUDE_MAX_SWAPCHAIN_IMAGES ];
  VkImageView                                        vk_swapchain_images_views[ CRUDE_MAX_SWAPCHAIN_IMAGES ];
  VkFramebuffer                                      vk_swapchain_framebuffers[ CRUDE_MAX_SWAPCHAIN_IMAGES ];
  uint16                                             vk_swapchain_width;
  uint16                                             vk_swapchain_height;
  uint32                                             vk_swapchain_image_index;
  /**
   * Descriptor pools/sets automatically generated
   * based on the reflection of the pipeline shaders.
   */
  VkDescriptorPool                                   vk_bindless_descriptor_pool;
  VkDescriptorSetLayout                              vk_bindless_descriptor_set_layout;
  VkDescriptorSet                                    vk_bindless_descriptor_set;
  /**
   * //!TODO
   */
  VkQueryPool                                        vk_timestamp_query_pool;
  /**
   * Allocators and callbacks
   */
  VkAllocationCallbacks                             *vk_allocation_callbacks;                               
  VmaAllocator                                       vma_allocator;
  crude_allocator                                    allocator;
  /**
   * UBO Buffers. //!TODO
   */
  crude_buffer_handle                                frame_buffer;
  crude_buffer_handle                                mesh_buffer;
} crude_gpu_device;                                

/************************************************
 *
 * GPU Device Initialize/Deinitialize
 * 
 ***********************************************/
CRUDE_API void                                     
crude_gfx_initialize_gpu_device                    
(                                                  
  _Out_ crude_gpu_device                            *gpu,
  _In_ crude_gpu_device_creation                    *creation
);

CRUDE_API void                                     
crude_gfx_deinitialize_gpu_device
(
  _In_ crude_gpu_device                             *gpu
);

/************************************************
 *
 * GPU Device Common Functions
 * 
 ***********************************************/  
CRUDE_API void                                     
crude_gfx_set_resource_name                        
(                                                  
  _In_ crude_gpu_device                             *gpu,
  _In_ VkObjectType                                  type,
  _In_ uint64                                        handle,
  _In_ char const                                   *name
);                                                 
                                                   
CRUDE_API void                                     
crude_gfx_new_frame                                
(                                                  
  _In_ crude_gpu_device                             *gpu
);
                                                   
CRUDE_API void                                     
crude_gfx_present                                  
(                                                  
  _In_ crude_gpu_device                             *gpu
);
                                                   
CRUDE_API crude_command_buffer*                    
crude_gfx_get_cmd_buffer                           
(                                                  
  _In_ crude_gpu_device                             *gpu,
  _In_ crude_queue_type                              type,
  _In_ bool                                          begin
);
                                                   
CRUDE_API void                                     
crude_gfx_queue_cmd_buffer                         
(                                                  
  _In_ crude_command_buffer                         *cmd
);
                                                   
CRUDE_API void*                                    
crude_gfx_map_buffer                               
(                                                  
  _In_ crude_gpu_device                             *gpu,
  _In_ crude_map_buffer_parameters const            *parameters
);
                                                   
CRUDE_API void                                     
crude_gfx_unmap_buffer                             
(                                                  
  _In_ crude_gpu_device                             *gpu,
  _In_ crude_buffer_handle                           handle
);
                                                   
CRUDE_API void*                                    
crude_gfx_dynamic_allocate                         
(                                                  
  _In_ crude_gpu_device                             *gpu,
  _In_ uint32                                        size
);

CRUDE_API void
crude_gfx_link_texture_sampler
(
  _In_ crude_gpu_device                             *gpu,
  _In_ crude_texture_handle                          texture,
  _In_ crude_sampler_handle                          sampler
);

CRUDE_API crude_descriptor_set_layout_handle
crude_gfx_get_descriptor_set_layout
(
  _In_ crude_gpu_device                             *gpu,
  _In_ crude_pipeline_handle                         pipeline_handle,
  _In_ uint32                                        layout_index
);

CRUDE_API void
crude_gfx_query_buffer 
(
  _In_ crude_gpu_device                             *gpu,
  _In_ crude_buffer_handle                           buffer,
  _Out_ crude_buffer_description                    *description
);

/************************************************
 *
 * GPU Device Resources Functions
 * 
 ***********************************************/
CRUDE_API crude_sampler_handle                     
crude_gfx_create_sampler                           
(                                                  
  _In_ crude_gpu_device                             *gpu,
  _In_ crude_sampler_creation const                 *creation
);                                                 
                                                   
CRUDE_API void                                     
crude_gfx_destroy_sampler                          
(                                                  
  _In_ crude_gpu_device                             *gpu,
  _In_ crude_sampler_handle                          handle
);                                                 
                                                   
CRUDE_API void                                     
crude_gfx_destroy_sampler_instant                  
(                                                  
  _In_ crude_gpu_device                             *gpu,
  _In_ crude_sampler_handle                          handle
);                                                 

CRUDE_API crude_texture_handle                     
crude_gfx_create_texture                           
(                                                  
  _In_ crude_gpu_device                             *gpu,
  _In_ crude_texture_creation const                 *creation
);

CRUDE_API void
crude_gfx_destroy_texture
(
  _In_ crude_gpu_device                             *gpu,
  _In_ crude_texture_handle                          handle
);

CRUDE_API void                                      
crude_gfx_destroy_texture_instant                   
(                                                   
  _In_ crude_gpu_device                             *gpu,
  _In_ crude_texture_handle                          handle
);    
                                                    
CRUDE_API crude_shader_state_handle                 
crude_gfx_create_shader_state                       
(                                                   
  _In_ crude_gpu_device                             *gpu,
  _In_ crude_shader_state_creation const            *creation
);

CRUDE_API void                                      
crude_gfx_destroy_shader_state                      
(                                                   
  _In_ crude_gpu_device                             *gpu,
  _In_ crude_shader_state_handle                     handle
); 
                                                    
CRUDE_API void                                      
crude_gfx_destroy_shader_state_instant              
(                                                   
  _In_ crude_gpu_device                             *gpu,
  _In_ crude_shader_state_handle                     handle
);

CRUDE_API crude_render_pass_handle                  
crude_gfx_create_render_pass                        
(                                                   
  _In_ crude_gpu_device                             *gpu,
  _In_ crude_render_pass_creation const             *creation
);
                                                       
CRUDE_API void                                      
crude_gfx_destroy_render_pass                       
(                                                   
  _In_ crude_gpu_device                             *gpu,
  _In_ crude_render_pass_handle                      handle
);                                                  
                                                    
CRUDE_API void                                      
crude_gfx_destroy_render_pass_instant               
(                                                   
  _In_ crude_gpu_device                             *gpu,
  _In_ crude_render_pass_handle                      handle
);

CRUDE_API crude_pipeline_handle                     
crude_gfx_create_pipeline                           
(                                                   
  _In_ crude_gpu_device                             *gpu,
  _In_ crude_pipeline_creation const                *creation
);

CRUDE_API void                                      
crude_gfx_destroy_pipeline                          
(                                                   
  _In_ crude_gpu_device                             *gpu,
  _In_ crude_pipeline_handle                         handle
);                                                  
                                                    
CRUDE_API void                                      
crude_gfx_destroy_pipeline_instant                  
(                                                   
  _In_ crude_gpu_device                             *gpu,
  _In_ crude_pipeline_handle                         handle
);                             
                                               
CRUDE_API crude_buffer_handle                       
crude_gfx_create_buffer                             
(                                                   
  _In_ crude_gpu_device                             *gpu,
  _In_ crude_buffer_creation const                  *creation
);

CRUDE_API void                                      
crude_gfx_destroy_buffer                            
(                                                   
  _In_ crude_gpu_device                             *gpu,
  _In_ crude_buffer_handle                           handle
);                                                  
                                                    
CRUDE_API void                                      
crude_gfx_destroy_buffer_instant                    
(                                                   
  _In_ crude_gpu_device                             *gpu,
  _In_ crude_buffer_handle                           handle
);

CRUDE_API crude_descriptor_set_layout_handle
crude_gfx_create_descriptor_set_layout
(
  _In_ crude_gpu_device                             *gpu,
  _In_ crude_descriptor_set_layout_creation const   *creation
);

CRUDE_API void                                      
crude_gfx_destroy_descriptor_set_layout
(                                                   
  _In_ crude_gpu_device                             *gpu,
  _In_ crude_descriptor_set_layout_handle            handle
);

CRUDE_API void
crude_gfx_destroy_descriptor_set_layout_instant
(                                                   
  _In_ crude_gpu_device                             *gpu,
  _In_ crude_descriptor_set_layout_handle            handle
);

CRUDE_API VkShaderModuleCreateInfo                  
crude_gfx_compile_shader                            
(                                                   
  _In_ const                                        *code,
  _In_ uint32                                        code_size,
  _In_ VkShaderStageFlagBits                         stage,
  _In_ char const                                   *name
);    

/************************************************
 *
 * GPU Device Resource Macros
 * 
 ***********************************************/  

#define CRUDE_OBTAIN_RESOURCE( resource_pool )\
(\
  crude_resource_pool_obtain_resource( &resource_pool )\
)

#define CRUDE_ACCESS_RESOURCE( resource_pool, resource_struct, handle )\
(\
  CAST( resource_struct*, crude_resource_pool_access_resource( &resource_pool, handle.index ) )\
)

#define CRUDE_RELEASE_RESOURCE( resource_pool, handle )\
{\
  crude_resource_pool_release_resource( &resource_pool, handle.index );\
}

#define CRUDE_GFX_GPU_OBTAIN_SAMPLER( gpu )                                 CRUDE_OBTAIN_RESOURCE( gpu->samplers )
#define CRUDE_GFX_GPU_ACCESS_SAMPLER( gpu, handle )                         CRUDE_ACCESS_RESOURCE( gpu->samplers, crude_sampler, handle  )
#define CRUDE_GFX_GPU_RELEASE_SAMPLER( gpu, handle )                        CRUDE_RELEASE_RESOURCE( gpu->samplers, handle )
                                                                            
#define CRUDE_GFX_GPU_OBTAIN_TEXTURE( gpu )                                 CRUDE_OBTAIN_RESOURCE( gpu->textures )
#define CRUDE_GFX_GPU_ACCESS_TEXTURE( gpu, handle )                         CRUDE_ACCESS_RESOURCE( gpu->textures, crude_texture, handle  )
#define CRUDE_GFX_GPU_RELEASE_TEXTURE( gpu, handle )                        CRUDE_RELEASE_RESOURCE( gpu->textures, handle )
                                                                            
#define CRUDE_GFX_GPU_OBTAIN_RENDER_PASS( gpu )                             CRUDE_OBTAIN_RESOURCE( gpu->render_passes )
#define CRUDE_GFX_GPU_ACCESS_RENDER_PASS( gpu, handle )                     CRUDE_ACCESS_RESOURCE( gpu->render_passes, crude_render_pass, handle  )
#define CRUDE_GFX_GPU_RELEASE_RENDER_PASS( gpu, handle )                    CRUDE_RELEASE_RESOURCE( gpu->render_passes, handle )
                                                                            
#define CRUDE_GFX_GPU_OBTAIN_SHADER_STATE( gpu )                            CRUDE_OBTAIN_RESOURCE( gpu->shaders )
#define CRUDE_GFX_GPU_ACCESS_SHADER_STATE( gpu, handle )                    CRUDE_ACCESS_RESOURCE( gpu->shaders, crude_shader_state, handle  )
#define CRUDE_GFX_GPU_RELEASE_SHADER_STATE( gpu, handle )                   CRUDE_RELEASE_RESOURCE( gpu->shaders, handle )
                                                                            
#define CRUDE_GFX_GPU_OBTAIN_PIPELINE( gpu )                                CRUDE_OBTAIN_RESOURCE( gpu->pipelines )
#define CRUDE_GFX_GPU_ACCESS_PIPELINE( gpu, handle )                        CRUDE_ACCESS_RESOURCE( gpu->pipelines, crude_pipeline, handle  )
#define CRUDE_GFX_GPU_RELEASE_PIPELINE( gpu, handle )                       CRUDE_RELEASE_RESOURCE( gpu->pipelines, handle )
                                                                            
#define CRUDE_GFX_GPU_OBTAIN_DESCRIPTOR_SET( gpu )                          CRUDE_OBTAIN_RESOURCE( gpu->descriptor_sets )
#define CRUDE_GFX_GPU_ACCESS_DESCRIPTOR_SET( gpu, handle )                  CRUDE_ACCESS_RESOURCE( gpu->descriptor_sets, crude_descriptor_set, handle  )
#define CRUDE_GFX_GPU_RELEASE_DESCRIPTOR_SET( gpu, handle )                 CRUDE_RELEASE_RESOURCE( gpu->descriptor_sets, handle )
                                                                            
#define CRUDE_GFX_GPU_OBTAIN_BUFFER( gpu )                                  CRUDE_OBTAIN_RESOURCE( gpu->buffers )
#define CRUDE_GFX_GPU_ACCESS_BUFFER( gpu, handle )                          CRUDE_ACCESS_RESOURCE( gpu->buffers, crude_buffer, handle  )
#define CRUDE_GFX_GPU_RELEASE_BUFFER( gpu, handle )                         CRUDE_RELEASE_RESOURCE( gpu->buffers, handle )

#define CRUDE_GFX_GPU_OBTAIN_DESCRIPTOR_SET_LAYOUT( gpu )                   CRUDE_OBTAIN_RESOURCE( gpu->descriptor_set_layouts )
#define CRUDE_GFX_GPU_ACCESS_DESCRIPTOR_SET_LAYOUT( gpu, handle )           CRUDE_ACCESS_RESOURCE( gpu->descriptor_set_layouts, crude_descriptor_set_layout, handle  )
#define CRUDE_GFX_GPU_RELEASE_DESCRIPTOR_SET_LAYOUT( gpu, handle )          CRUDE_RELEASE_RESOURCE( gpu->descriptor_set_layouts, handle )

/************************************************
 *
 * GPU Device Utils Macros
 * 
 ***********************************************/  

#define CRUDE_GFX_HANDLE_VULKAN_RESULT( result, msg )\
{\
  if ( result != VK_SUCCESS )\
  {\
    CRUDE_ABORT( CRUDE_CHANNEL_GRAPHICS, "vulkan result isn't success: %i %s", result, msg );\
  }\
}