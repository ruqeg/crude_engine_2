#include <graphics/gpu_resources.h>

void crude_reset_render_pass_output( _In_ crude_render_pass_output *output )
{
  output->num_color_formats = 0;
  for ( uint32 i = 0; i < CRUDE_MAX_IMAGE_OUTPUTS; ++i )
  {
    output->color_formats[i] = VK_FORMAT_UNDEFINED;
  }
  output->depth_stencil_format = VK_FORMAT_UNDEFINED;
  output->color_operation = output->depth_operation = output->stencil_operation = CRUDE_RENDER_PASS_OPERATION_DONT_CARE;
}

VkImageType crude_to_vk_image_type( _In_ crude_texture_type type )
{
  static VkImageType s_vk_target[ CRUDE_TEXTURE_TYPE_TEXTURE_COUNT ] = { VK_IMAGE_TYPE_1D, VK_IMAGE_TYPE_2D, VK_IMAGE_TYPE_3D, VK_IMAGE_TYPE_1D, VK_IMAGE_TYPE_2D, VK_IMAGE_TYPE_3D };
  return s_vk_target[ type ];
}

bool crude_has_depth_or_stencil( _In_ VkFormat value )
{
  return value >= VK_FORMAT_D16_UNORM && value <= VK_FORMAT_D32_SFLOAT_S8_UINT;
}

bool crude_has_depth( _In_ VkFormat value )
{
  return ( value >= VK_FORMAT_D16_UNORM && value < VK_FORMAT_S8_UINT ) || ( value >= VK_FORMAT_D16_UNORM_S8_UINT && value <= VK_FORMAT_D32_SFLOAT_S8_UINT );
}

VkImageViewType crude_to_vk_image_view_type(crude_texture_type type)
{
  static VkImageViewType s_vk_data[ CRUDE_TEXTURE_TYPE_TEXTURE_COUNT ] = { VK_IMAGE_VIEW_TYPE_1D, VK_IMAGE_VIEW_TYPE_2D, VK_IMAGE_VIEW_TYPE_3D, VK_IMAGE_VIEW_TYPE_1D_ARRAY, VK_IMAGE_VIEW_TYPE_2D_ARRAY, VK_IMAGE_VIEW_TYPE_CUBE_ARRAY };
  return s_vk_data[ type ];
}
