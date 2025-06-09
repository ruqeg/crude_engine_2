#ifndef __cplusplus
#error "TODO"
#else
#include <imgui.h>
#include <backends/imgui_impl_sdl3.h>
#include <backends/imgui_impl_vulkan.h>

#include <gui/gui.h>

void
crude_gui_initialize
(
)
{
  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  ImGuiIO& io = ImGui::GetIO(); (void)io;
  io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
  io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;
  
  ImGui::StyleColorsDark();
}

void
crude_gui_deinitialize
(
)
{
  ImGui::DestroyContext();
}

static void
imgui_check_vk_result_
(
  _In_ VkResult                                            err
)
{
  CRUDE_GFX_HANDLE_VULKAN_RESULT( err, "Imgui vulkan error!" );
}

void
crude_gui_create
(
  _In_ crude_gfx_device                                   *gpu 
)
{
  return;
  VkPipelineRenderingCreateInfo                         pipeline_rendering_create_info;
  VkFormat                                              color_attachment_format;
  VkFormat                                              depth_attachment_format;

  ImGui_ImplSDL3_InitForVulkan( gpu->sdl_window );
  
  color_attachment_format = VK_FORMAT_B8G8R8A8_UNORM;
  depth_attachment_format = VK_FORMAT_D32_SFLOAT;
  pipeline_rendering_create_info = CRUDE_COMPOUNT( VkPipelineRenderingCreateInfo, { 
    .sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO_KHR,
    .viewMask = 0,
    .colorAttachmentCount = 1u,
    .pColorAttachmentFormats = &color_attachment_format,
    .depthAttachmentFormat = depth_attachment_format,
    .stencilAttachmentFormat = VK_FORMAT_UNDEFINED,
  } );
  ImGui_ImplVulkan_InitInfo imgui_vk_init_info = {
    .Instance = gpu->vk_instance,
    .PhysicalDevice = gpu->vk_physical_device,
    .Device = gpu->vk_device,
    .QueueFamily = gpu->vk_main_queue_family,
    .Queue = gpu->vk_main_queue,
    .RenderPass = VK_NULL_HANDLE,
    .MinImageCount = gpu->vk_swapchain_images_count,
    .ImageCount = gpu->vk_swapchain_images_count,
    .MSAASamples = VK_SAMPLE_COUNT_1_BIT,
    .PipelineCache = VK_NULL_HANDLE,
    .DescriptorPoolSize = 2,
    .UseDynamicRendering = true,
    .PipelineRenderingCreateInfo = pipeline_rendering_create_info,
    .Allocator = gpu->vk_allocation_callbacks,
    .CheckVkResultFn = imgui_check_vk_result_,
  };
  ImGui_ImplVulkan_Init( &imgui_vk_init_info );
}

void
crude_gui_destroy
(
  _In_ crude_gfx_device                                   *gpu 
)
{
  return;
  vkDeviceWaitIdle( gpu->vk_device );
  ImGui_ImplVulkan_Shutdown();
  ImGui_ImplSDL3_Shutdown();
}

void
crude_gui_process_event
(
  _In_ void const                                         *event
)
{
  ImGui_ImplSDL3_ProcessEvent( CRUDE_REINTERPRET_CAST( SDL_Event const*, event ) );
}
#endif