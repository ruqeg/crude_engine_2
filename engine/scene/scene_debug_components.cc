#pragma once

#if CRUDE_DEVELOP

#include <engine/core/assert.h>

#include <engine/scene/scene_debug_components.h>
#include <engine/graphics/imgui.h>
#include <engine/physics/physics_components.h>
#include <engine/scene/node_manager.h>

ECS_COMPONENT_DECLARE( crude_debug_collision );
ECS_COMPONENT_DECLARE( crude_debug_gltf );

CRUDE_COMPONENT_STRING_DEFINE( crude_debug_collision, "crude_debug_collision" );
CRUDE_COMPONENT_STRING_DEFINE( crude_debug_gltf, "crude_debug_gltf" );

CRUDE_ECS_MODULE_IMPORT_IMPL( crude_scene_debug_components )
{
  ECS_MODULE( world, crude_scene_debug_components );
  ECS_COMPONENT_DEFINE( world, crude_debug_collision );
  ECS_COMPONENT_DEFINE( world, crude_debug_gltf );
}

CRUDE_PARSE_COMPONENT_TO_IMGUI_FUNC_DECLARATION( crude_debug_collision )
{
  ImGui::Checkbox( "Visible", &component->visible );
}
CRUDE_PARSE_COMPONENT_TO_IMGUI_FUNC_DECLARATION( crude_debug_gltf )
{
  ImGui::Checkbox( "Visible", &component->visible );
}

#endif