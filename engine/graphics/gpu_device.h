#pragma once

#include <vulkan/vulkan.h>
#include <SDL3/SDL.h>

#include <core/alias.h>
#include <graphics/gpu_resources.h>
#include <graphics/command_buffer.h>

#define CRUDE_HANDLE_VULKAN_RESULT( result, msg ) if ( result != VK_SUCCESS ) CRUDE_ABORT( CRUDE_CHANNEL_GRAPHICS, "vulkan result isn't success: %i %s", result, msg );

#define CRUDE_MAX_SWAPCHAIN_IMAGES 3

typedef struct crude_gpu_device_creation
{
  SDL_Window                *sdl_window;
  char const                *vk_application_name;
  uint32                     vk_application_version;
  crude_allocator            allocator;
  uint16                     queries_per_frame;
  uint16                     max_frames;
} crude_gpu_device_creation;

typedef struct crude_gpu_device
{
  crude_render_pass_handle          swapchain_pass;
  uint16                            swapchain_width;
  uint16                            swapchain_height;
  crude_render_pass_output          swapchain_output;
  crude_sampler_handle              default_sampler;
  crude_texture_handle              depth_texture;
  crude_resource_pool               buffers;
  crude_resource_pool               textures;
  crude_resource_pool               pipelines;
  crude_resource_pool               samplers;
  crude_resource_pool               descriptor_set_layouts;
  crude_resource_pool               descriptor_sets;
  crude_resource_pool               render_passes;
  crude_resource_pool               command_buffers;
  crude_resource_pool               shaders;
  uint32                            queued_command_buffers_count;
  crude_command_buffer            **queued_command_buffers;
  VkInstance                        vk_instance;
  VkDebugUtilsMessengerEXT          vk_debug_utils_messenger;
  VkSurfaceKHR                      vk_surface;
  VkPhysicalDevice                  vk_physical_device;
  VkDevice                          vk_device;
  int32                             vk_queue_family_index;
  VkQueue                           vk_queue;
  VkSwapchainKHR                    vk_swapchain;
  uint32                            vk_swapchain_images_count;
  VkImage                           vk_swapchain_images[ CRUDE_MAX_SWAPCHAIN_IMAGES ];
  VkImageView                       vk_swapchain_images_views[ CRUDE_MAX_SWAPCHAIN_IMAGES ];
  VkFramebuffer                     vk_swapchain_framebuffers[ CRUDE_MAX_SWAPCHAIN_IMAGES ];
  VkSurfaceFormatKHR                vk_surface_format;
  VkDescriptorPool                  vk_descriptor_pool;
  VkQueryPool                       vk_timestamp_query_pool;
  VkSemaphore                       vk_render_finished_semaphores[ CRUDE_MAX_SWAPCHAIN_IMAGES ];
  VkSemaphore                       vk_image_avalivable_semaphores[ CRUDE_MAX_SWAPCHAIN_IMAGES ];
  VkFence                           vk_command_buffer_executed_fences[ CRUDE_MAX_SWAPCHAIN_IMAGES ];
  uint32                            current_frame;
  uint32                            vk_swapchain_image_index;
  crude_resource_update            *resource_deletion_queue;
  VmaAllocator                      vma_allocator;
  VkAllocationCallbacks            *vk_allocation_callbacks;
  crude_allocator                   allocator;
  uint16                            max_frames;
  SDL_Window                       *sdl_window;
} crude_gpu_device;

CRUDE_API void crude_initialize_gpu_device( _Out_ crude_gpu_device *gpu, _In_ crude_gpu_device_creation *creation );
CRUDE_API void crude_deinitialize_gpu_device( _In_ crude_gpu_device *gpu );

CRUDE_API void crude_set_resource_name( _In_ crude_gpu_device *gpu, _In_ VkObjectType type, _In_ uint64 handle, _In_ char const *name );

CRUDE_API crude_sampler_handle crude_create_sampler( _In_ crude_gpu_device *gpu, _In_ crude_sampler_creation const *creation );
CRUDE_API void crude_destroy_sampler( _In_ crude_gpu_device *gpu, _In_ crude_sampler_handle sampler );
CRUDE_API void crude_destroy_sampler_instant( _In_ crude_gpu_device *gpu, _In_ crude_resource_handle handle );

CRUDE_API crude_buffer_handle crude_create_buffer( _In_ crude_gpu_device *gpu, _In_ crude_buffer_creation const *creation );

CRUDE_API crude_texture_handle crude_create_texture( _In_ crude_gpu_device *gpu, _In_ crude_texture_creation const *creation );
CRUDE_API void crude_destroy_texture( _In_ crude_gpu_device *gpu, _In_ crude_texture_handle texture );
CRUDE_API void crude_destroy_texture_instant( _In_ crude_gpu_device *gpu, _In_ crude_resource_handle handle );

CRUDE_API crude_render_pass_handle crude_create_render_pass( _In_ crude_gpu_device *gpu, _In_ crude_render_pass_creation const *creation );
CRUDE_API void crude_destroy_render_pass( _In_ crude_gpu_device *gpu, _In_ crude_render_pass_handle render_pass );
CRUDE_API void crude_destroy_render_pass_instant( _In_ crude_gpu_device *gpu, _In_ crude_resource_handle handle );

CRUDE_API void crude_new_frame( _In_ crude_gpu_device *gpu );
CRUDE_API void crude_present( _In_ crude_gpu_device *gpu );

CRUDE_API crude_command_buffer* crude_get_command_buffer( _In_ crude_gpu_device *gpu, _In_ crude_queue_type type, _In_ bool begin );
CRUDE_API void crude_queue_command_buffer( _In_ crude_command_buffer *command_buffer );

CRUDE_API crude_shader_state_handle crude_create_shader_state( _In_ crude_gpu_device *gpu, _In_ crude_shader_state_creation const *creation );
CRUDE_API void crude_destroy_shader_state( _In_ crude_gpu_device *gpu, _In_ crude_shader_state_handle shader_state );
CRUDE_API void crude_destroy_shader_state_instant( _In_ crude_gpu_device *gpu, _In_ crude_resource_handle handle );

CRUDE_API crude_pipeline_handle crude_create_pipeline( _In_ crude_gpu_device *gpu, _In_ crude_pipeline_creation const *creation );
CRUDE_API void crude_destroy_pipeline( _In_ crude_gpu_device *gpu, _In_ crude_pipeline_handle pipeline );
CRUDE_API void crude_destroy_pipeline_instant( _In_ crude_gpu_device *gpu, _In_ crude_resource_handle handle );

CRUDE_API VkShaderModuleCreateInfo crude_compile_shader( _In_ const *code, _In_ uint32 code_size, _In_ VkShaderStageFlagBits stage, _In_ char const *name );

#define CRUDE_GFX_GPU_ACCESS_SHADER( gpu, handle )\
(\
  CAST( crude_shader_state*, crude_resource_pool_access_resource( &gpu->shaders, handle.index ) )\
)

#define CRUDE_GFX_GPU_ACCESS_RENDER_PASS( gpu, handle )\
(\
  CAST( crude_render_pass*, crude_resource_pool_access_resource( &gpu->render_passes, handle.index ) )\
)

#define CRUDE_GFX_GPU_ACCESS_PIPELINE( gpu, handle )\
(\
  CAST( crude_pipeline*, crude_resource_pool_access_resource( &gpu->pipelines, handle.index ) )\
)