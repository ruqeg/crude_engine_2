#pragma once

#include <engine/core/ecs.h>
#include <engine/scene/node_manager.h>
#include <engine/platform/platform.h>
#include <engine/graphics/scene_renderer.h>
#include <engine/graphics/gpu_profiler.h>
#include <engine/graphics/imgui.h>

typedef struct crude_editor crude_editor;
typedef struct crude_devgui crude_devgui;

typedef struct crude_devgui_added_node_data
{
  char                                                     buffer[ 128 ];
} crude_devgui_added_node_data;

typedef struct crude_devgui_nodes_tree
{
  bool                                                     enabled;
} crude_devgui_nodes_tree;

typedef struct crude_devgui_node_inspector
{
  bool                                                     enabled;
} crude_devgui_node_inspector;

typedef struct crude_devgui_viewport
{
  crude_gfx_texture_handle                                 selected_texture;
} crude_devgui_scene_node_viewport;

typedef struct crude_devgui_editor_camera
{
  bool                                                     enabled;
} crude_devgui_editor_camera;

typedef struct crude_devgui
{
  crude_devgui_nodes_tree                                  dev_nodes_tree;
  crude_devgui_node_inspector                              dev_node_inspector;
  crude_devgui_viewport                                    dev_viewport;
  crude_devgui_editor_camera                               dev_editor_camera;
  bool                                                     menubar_enabled;
} crude_devgui;

/***********************
* 
 * Dev Gui
 * 
 ************************/
CRUDE_API void
crude_devgui_initialize
(
  _In_ crude_devgui                                       *devgui
);

CRUDE_API void
crude_devgui_deinitialize
(
  _In_ crude_devgui                                       *devgui
);

CRUDE_API void
crude_devgui_draw
(
  _In_ crude_devgui                                       *devgui,
  _In_ crude_ecs                                          *world
);

CRUDE_API void
crude_devgui_handle_input
(
  _In_ crude_devgui                                       *devgui,
  _In_ crude_input                                        *input
);

CRUDE_API void
crude_devgui_on_resize
(
  _In_ crude_devgui                                       *devgui
);

CRUDE_API void
crude_devgui_graphics_pre_update
(
  _In_ crude_devgui                                       *devgui
);

/******************************
 * 
 * Dev Gui Nodes Tree
 * 
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
  _In_ crude_ecs                                          *world
);

/******************************
 * 
 * Dev Gui Node Inspector
 * 
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
  _In_ crude_ecs                                          *world
);

/******************************
 * 
 * Dev Gui Viewport
 * 
 *******************************/
CRUDE_API void
crude_devgui_viewport_initialize
(
  _In_ crude_devgui_viewport                              *devgui_viewport
);

CRUDE_API void
crude_devgui_viewport_draw
(
  _In_ crude_devgui_viewport                              *devgui_viewport,
  _In_ crude_ecs                                          *world
);

CRUDE_API void
crude_devgui_viewport_input
(
  _In_ crude_devgui_viewport                              *devgui_viewport,
  _In_ crude_input                                        *input
);

/******************************
 * 
 * Editor Camera
 * 
 *******************************/
CRUDE_API void
crude_devgui_editor_camera_initialize
(
  _In_ crude_devgui_editor_camera                         *dev_editor_camera
);

CRUDE_API void
crude_devgui_editor_camera_draw
(
  _In_ crude_devgui_editor_camera                         *dev_editor_camera,
  _In_ crude_ecs                                          *world
);