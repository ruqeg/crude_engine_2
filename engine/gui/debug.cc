#include <engine/engine.h>

#include <engine/gui/debug.h>

void
crude_gui_debug_initialize
(
  _In_ crude_gui_debug                                    *debug,
  _In_ crude_engine                                       *engine
)
{
  debug->engine = engine;
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
  if ( ImGui::CollapsingHeader( "Editor" ) )
  {
    if ( ImGui::Button( "Select Editor Camera" ) )
    {
      debug->engine->camera_node = debug->engine->editor_camera_node;
    }
    if ( ImGui::Button( "Reset Editor Camera" ) )
    {
      CRUDE_ENTITY_SET_COMPONENT( debug->engine->world, debug->engine->editor_camera_node, crude_transform, { crude_transform_empty( ) } );
    }
    crude_free_camera *free_camera = CRUDE_ENTITY_GET_MUTABLE_COMPONENT( debug->engine->world, debug->engine->editor_camera_node, crude_free_camera );
    ImGui::Checkbox( "Input Enabled", &free_camera->input_enabled );
  }
  if ( ImGui::CollapsingHeader( "Render Graph" ) )
  {
    ImGui::Checkbox( "Disable Shadows Pass", &debug->engine->scene_renderer.pointlight_shadow_pass.enabled );
  }
}