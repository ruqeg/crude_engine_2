#pragma once

#include <engine/graphics/scene_renderer.h>
#include <engine/graphics/gpu_profiler.h>
#include <engine/graphics/imgui.h>

typedef enum crude_gui_node_type
{
  CRUDE_GUI_NODE_TYPE_EMPTY_3D = 0,
  CRUDE_GUI_NODE_TYPE_GLTF = 1,
  CRUDE_GUI_NODE_TYPE_COUNT,
} crude_gui_node_type;

typedef struct crude_gui_node_tree
{
  crude_node_manager                                      *node_manager;
  crude_entity                                             node_reference;
  char                                                     new_node_name[ 512 ];
  int32                                                    new_node_type_index;
  bool                                                     node_popup_should_be_opened;
  ImGuiTextFilter                                          im_text_filter;
} crude_gui_node_tree;

CRUDE_API void
crude_gui_node_tree_initialize
(
  _In_ crude_gui_node_tree                                *node_tree
);

CRUDE_API void
crude_gui_node_tree_deinitialize
(
  _In_ crude_gui_node_tree                                *node_tree
);

CRUDE_API void
crude_gui_node_tree_update
(
  _In_ crude_gui_node_tree                                *node_tree
);

CRUDE_API void
crude_gui_node_tree_queue_draw
(
  _In_ crude_gui_node_tree                                *node_tree,
  _In_ crude_ecs                                          *world,
  _In_ crude_entity                                        tree_node,
  _Inout_ crude_entity                                    *selected_node
);