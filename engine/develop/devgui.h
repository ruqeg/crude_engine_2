#pragma once

#include <ImGuizmo.h>

#include <graphics/render_graph.h>
#include <graphics/renderer.h>
#include <platform/platform_components.h>

typedef struct crude_devgui_nodes_tree
{
  bool                                                     enabled;
  uint32                                                   selected_node_index;
  crude_entity                                             selected_node;
} crude_devgui_nodes_tree;

typedef struct crude_devgui_node_inspector
{
  bool                                                     enabled;
} crude_devgui_node_inspector;

typedef struct crude_devgui_viewport
{
  crude_gfx_device                                        *gpu;
  crude_gfx_texture_handle                                 selected_texture;
  ImGuizmo::OPERATION                                      gizmo_operation;
  ImGuizmo::MODE                                           gizmo_mode;
} crude_devgui_scene_node_viewport;

typedef struct crude_devgui_render_graph
{
  crude_gfx_render_graph                                  *render_graph;
  bool                                                     enabled;
} crude_devgui_render_graph;

typedef struct crude_devgui
{
  bool                                                     menubar_enabled;
  crude_devgui_nodes_tree                                  dev_nodes_tree;
  crude_devgui_node_inspector                              dev_node_inspector;
  crude_devgui_viewport                                    dev_viewport;
  crude_devgui_render_graph                                dev_render_graph;
  crude_gfx_renderer                                      *renderer;
} crude_devgui;

/*********
 * Dev Gui
 *************/
CRUDE_API void
crude_devgui_initialize
(
  _In_ crude_devgui                                       *devgui,
  _In_ crude_gfx_render_graph                             *render_graph
);

CRUDE_API void
crude_devgui_draw
(
  _In_ crude_devgui                                       *devgui,
  _In_ crude_entity                                        main_scene_node,
  _In_ crude_entity                                        camera_node
);

CRUDE_API void
crude_devgui_handle_input
(
  _In_ crude_devgui                                       *devgui,
  _In_ crude_input                                        *input
);

/******************************
 * Dev Gui Nodes Tree
 *******************************/
CRUDE_API void
crude_devgui_nodes_tree_initialize
(
  _In_ crude_devgui_nodes_tree                            *devgui_nodes_tree
);

CRUDE_API void
crude_devgui_nodes_tree_draw
(
  _In_ crude_devgui_nodes_tree                            *devgui_nodes_tree,
  _In_ crude_entity                                        node
);

/******************************
 * Dev Gui Node Inspector
 *******************************/
CRUDE_API void
crude_devgui_node_inspector_initialize
(
  _In_ crude_devgui_node_inspector                        *devgui_inspector
);

CRUDE_API void
crude_devgui_node_inspector_draw
(
  _In_ crude_devgui_node_inspector                        *devgui_inspector,
  _In_ crude_entity                                        node
);

/******************************
 * Dev Gui Viewport
 *******************************/
CRUDE_API void
crude_devgui_viewport_initialize
(
  _In_ crude_devgui_viewport                              *devgui_viewport,
  _In_ crude_gfx_device                                   *gpu
);

CRUDE_API void
crude_devgui_viewport_draw
(
  _In_ crude_devgui_viewport                              *devgui_viewport,
  _In_ crude_entity                                        camera_node,
  _In_ crude_entity                                        selected_node
);

CRUDE_API void
crude_devgui_viewport_input
(
  _In_ crude_devgui_viewport                              *devgui_viewport,
  _In_ crude_input                                        *input
);

/******************************
 * Dev Gui Render Graph
 *******************************/
CRUDE_API void
crude_devgui_render_graph_initialize
(
  _In_ crude_devgui_render_graph                          *devgui_render_graph,
  _In_ crude_gfx_render_graph                             *render_graph
);

CRUDE_API void
crude_devgui_render_graph_draw
(
  _In_ crude_devgui_render_graph                          *devgui_render_graph
);