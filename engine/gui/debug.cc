#if CRUDE_DEVELOP

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
  if ( ImGui::CollapsingHeader( "Render Graph" ) )
  {
    ImGui::Checkbox( "Disable Shadows Pass", &debug->engine->scene_renderer.pointlight_shadow_pass.enabled );
  }

  if ( ImGui::Checkbox( "Enable Simulation", &debug->engine->physics.simulation_enabled ) )
  {
    crude_physics_enable_simulation( &debug->engine->physics, debug->engine->world, debug->engine->physics.simulation_enabled );
  }

  if ( ImGui::CollapsingHeader( "Editor Camera" ) )
  {
    crude_editor_camera *editor_camerae = CRUDE_ENTITY_GET_MUTABLE_COMPONENT( debug->engine->world, debug->engine->editor.editor_camera_node, crude_editor_camera );
    ImGui::DragFloat( "Walk Speed", &editor_camerae->walk_speed );
  }
}

#endif /* CRUDE_DEVELOP */