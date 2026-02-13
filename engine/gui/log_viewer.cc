#include <engine/core/log.h>

#include <engine/gui/log_viewer.h>

void
crude_gui_log_viewer_initialize
(
  _In_ crude_gui_log_viewer                               *log_viewer
)
{
  log_viewer->prev_log_buffer_length = 0;
}

void
crude_gui_log_viewer_deinitialize
(
  _In_ crude_gui_log_viewer                               *log_viewer
)
{
}

void
crude_gui_log_viewer_update
(
  _In_ crude_gui_log_viewer                               *log_viewer
)
{
}

void
crude_gui_log_viewer_queue_draw
(
  _In_ crude_gui_log_viewer                               *log_viewer
)
{
  ImGui::TextUnformatted( crude_log_buffer( ) );
  if ( log_viewer->prev_log_buffer_length != crude_log_buffer_length( ) )
  {
    log_viewer->prev_log_buffer_length = crude_log_buffer_length( );
    ImGui::SetScrollHereY( 1.0f );
  }
}