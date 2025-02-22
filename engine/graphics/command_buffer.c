#include <graphics/gpu_device.h>

#include <graphics/command_buffer.h>

void crude_reset_command_buffer(
  _In_ crude_command_buffer *command_buffer )
{
  command_buffer->is_recording = false;
  command_buffer->current_render_pass = NULL;
}