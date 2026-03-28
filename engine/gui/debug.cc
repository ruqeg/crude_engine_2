#include <engine/gui/debug.h>

void
crude_gui_debug_initialize
(
  _In_ crude_gui_debug                                    *debug,
  _In_ crude_gfx_scene_renderer                           *scene_renderer
)
{
  debug->scene_renderer = scene_renderer;
}

void
crude_gui_debug_deinitialize
(
  _In_ crude_gui_debug                                    *debug
)
{
}

void
crude_gui_debug_update
(
  _In_ crude_gui_debug                                    *debug
)
{
}

void
crude_gui_debug_queue_draw
(
  _In_ crude_gui_debug                                    *debug
)
{
  if ( ImGui::CollapsingHeader( "Render Graph" ) )
  {
    ImGui::Checkbox( "Disable Shadows Pass", &debug->scene_renderer->pointlight_shadow_pass.enabled );
  }
}