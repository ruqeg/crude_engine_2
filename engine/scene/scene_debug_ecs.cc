#pragma once

#if CRUDE_DEVELOP

#include <engine/core/assert.h>
#include <engine/scene/scene_debug_ecs.h>
#include <engine/scene/node_manager.h>
#include <engine/physics/physics_debug_ecs.h>
#include <engine/graphics/imgui.h>

/**********************************************************
 *
 *                 Components
 *
 *********************************************************/
ECS_COMPONENT_DECLARE( crude_debug_collision );
ECS_COMPONENT_DECLARE( crude_debug_gltf );

CRUDE_COMPONENT_STRING_DEFINE( crude_debug_collision, "crude_debug_collision" );
CRUDE_COMPONENT_STRING_DEFINE( crude_debug_gltf, "crude_debug_gltf" );

void
crude_scene_debug_components_import
(
  _In_ crude_ecs                                          *world,
  _In_ crude_components_serialization_manager             *manager
)
{
  CRUDE_ECS_MODULE( world, crude_scene_debug_components );
  CRUDE_ECS_COMPONENT_DEFINE( world, crude_debug_collision );
  CRUDE_ECS_COMPONENT_DEFINE( world, crude_debug_gltf );
  CRUDE_PARSE_COMPONENT_TO_IMGUI_FUNC_DEFINE( manager, crude_debug_collision );
  CRUDE_PARSE_COMPONENT_TO_IMGUI_FUNC_DEFINE( manager, crude_debug_gltf );
}

CRUDE_PARSE_COMPONENT_TO_IMGUI_FUNC_IMPLEMENTATION( crude_debug_collision )
{
  ImGui::Checkbox( "Visible", &component->visible );
}

CRUDE_PARSE_COMPONENT_TO_IMGUI_FUNC_IMPLEMENTATION( crude_debug_gltf )
{
  ImGui::Checkbox( "Visible", &component->visible );
}

#endif