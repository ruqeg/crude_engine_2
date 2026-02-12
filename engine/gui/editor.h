#pragma once

#if CRUDE_DEVELOP

#include <engine/graphics/scene_renderer.h>
#include <engine/graphics/gpu_profiler.h>
#include <engine/graphics/imgui.h>
#include <engine/gui/node_inspector.h>
#include <engine/gui/node_tree.h>
#include <engine/gui/viewport.h>

typedef struct crude_engine crude_engine;

typedef struct crude_gui_editor
{
  crude_engine                                            *engine;
  crude_gui_viewport                                       viewport;
  crude_gui_node_inspector                                 node_inspector;
  crude_gui_node_tree                                      node_tree;
  crude_entity                                             selected_node;
} crude_gui_editor;

CRUDE_API void
crude_gui_editor_initialize
(
  _In_ crude_gui_editor                                   *editor,
  _In_ crude_engine                                       *engine
);

CRUDE_API void
crude_gui_editor_deinitialize
(
  _In_ crude_gui_editor                                   *editor
);

CRUDE_API void
crude_gui_editor_queue_draw
(
  _In_ crude_gui_editor                                   *editor
);

CRUDE_API void
crude_gui_editor_update
(
  _In_ crude_gui_editor                                   *editor
);

CRUDE_API void
crude_gui_editor_handle_input
(
  _In_ crude_gui_editor                                   *editor
);

#endif