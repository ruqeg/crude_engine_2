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
  crude_editor_camera                                     *editor_camera;

  ImGui::Checkbox( "Disable Render Graph Shadows Pass", &debug->engine->scene_renderer.pointlight_shadow_pass.enabled );

  if ( ImGui::Checkbox( "Enable Simulation", &debug->engine->physics.simulation_enabled ) )
  {
    crude_physics_enable_simulation( &debug->engine->physics, debug->engine->world, debug->engine->physics.simulation_enabled );
  }

#if CRUDE_EDITOR
  editor_camera = CRUDE_ENTITY_GET_MUTABLE_COMPONENT( debug->engine->world, debug->engine->editor.editor_camera_node, crude_editor_camera );
  ImGui::DragFloat( "Editor Camera Speed", &editor_camera->walk_speed );
#endif /* CRUDE_EDITOR */

  ImGui::Checkbox( "DDGI Debug", &debug->engine->scene_renderer.ddgi_debug );
  ImGui::DragFloat( "DDGI Rotation Scaler", &debug->engine->scene_renderer.rotation_scaler );
}

#endif /* CRUDE_DEVELOP */