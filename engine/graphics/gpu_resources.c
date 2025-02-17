#include <graphics/gpu_resources.h>

crude_reset_render_pass_output( _In_ crude_render_pass_output *output )
{
  output->num_color_formats = 0;
  for ( uint32 i = 0; i < CRUDE_MAX_IMAGE_OUTPUTS; ++i )
  {
    output->color_formats[i] = VK_FORMAT_UNDEFINED;
  }
  output->depth_stencil_format = VK_FORMAT_UNDEFINED;
  output->color_operation = output->depth_operation = output->stencil_operation = CRUDE_RENDER_PASS_OPERATION_DONT_CARE;
}