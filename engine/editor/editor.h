#pragma once

#if CRUDE_DEVELOP

#include <engine/graphics/scene_renderer.h>
#include <engine/graphics/gpu_profiler.h>
#include <engine/graphics/imgui.h>
#include <engine/gui/node_inspector.h>
#include <engine/gui/node_tree.h>
#include <engine/gui/viewport.h>
#include <engine/gui/log_viewer.h>
#include <engine/gui/content_browser.h>
#include <engine/gui/gpu_visual_profiler.h>
#include <engine/gui/debug.h>
#include <engine/editor/editor_camera_ecs.h>

typedef struct crude_engine crude_engine;

typedef struct crude_editor
{
  /* Context */
  crude_engine                                            *engine;

  /* Common */
  crude_entity                                             editor_camera_node;
  crude_editor_camera_system_context                       editor_camera_system_context;
  ImVec2                                                   viewport_position;
  ImVec2                                                   viewport_size;

  /* Gui */
  crude_gui_viewport                                       viewport;
  crude_gui_node_inspector                                 node_inspector;
  crude_gui_node_tree                                      node_tree;
  crude_gui_log_viewer                                     log_viewer;
  crude_gui_content_browser                                content_browser;
  crude_entity                                             selected_node;
  crude_gui_gpu_visual_profiler                            gpu_visual_profiler;
  crude_gui_debug                                          debug;
} crude_editor;

CRUDE_API void
crude_editor_initialize
(
  _In_ crude_editor                                       *editor,
  _In_ crude_engine                                       *engine
);

CRUDE_API void
crude_editor_deinitialize
(
  _In_ crude_editor                                       *editor
);

CRUDE_API void
crude_editor_queue_draw
(
  _In_ crude_editor                                       *editor
);

CRUDE_API void
crude_editor_update
(
  _In_ crude_editor                                       *editor,
  _In_ float32                                             delta_time
);

CRUDE_API void
crude_editor_handle_input
(
  _In_ crude_editor                                       *editor
);

CRUDE_API void
crude_editor_start_game
(
  _In_ crude_editor                                       *editor
);

CRUDE_API void
crude_editor_stop_game
(
  _In_ crude_editor                                       *editor
);

#endif