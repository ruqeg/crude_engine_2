#pragma once

#include <engine/graphics/scene_renderer.h>
#include <engine/graphics/gpu_profiler.h>
#include <engine/graphics/imgui.h>

typedef struct crude_gui_node_inspector
{
  crude_components_serialization_manager                  *components_serialization_manager;
  crude_node_manager                                      *node_manager;
} crude_gui_node_inspector;

CRUDE_API void
crude_gui_node_inspector_initialize
(
  _In_ crude_gui_node_inspector                           *node_inspector,
  _In_ crude_components_serialization_manager             *components_serialization_manager,
  _In_ crude_node_manager                                 *node_manager
);

CRUDE_API void
crude_gui_node_inspector_deinitialize
(
  _In_ crude_gui_node_inspector                           *node_inspector
);

CRUDE_API void
crude_gui_node_inspector_update
(
  _In_ crude_gui_node_inspector                           *node_inspector
);

CRUDE_API void
crude_gui_node_inspector_queue_draw
(
  _In_ crude_gui_node_inspector                           *node_inspector,
  _In_ crude_ecs                                          *world,
  _In_ crude_entity                                        node
);