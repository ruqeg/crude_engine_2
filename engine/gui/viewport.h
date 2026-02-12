#pragma once

#include <engine/graphics/scene_renderer.h>
#include <engine/graphics/gpu_profiler.h>
#include <engine/graphics/imgui.h>

typedef struct crude_gui_viewport
{
  crude_gfx_device                                        *gpu;
  crude_gfx_texture_handle                                 viewport_texture_handle;
  uint8                                                    viewport_ratio_index;
  ImGuizmo::OPERATION                                      selected_gizmo_operation;
  bool                                                     should_draw_grid;
} crude_gui_viewport;

CRUDE_API void
crude_gui_viewport_initialize
(
  _In_ crude_gui_viewport                                 *viewport,
  _In_ crude_gfx_device                                   *gpu,
  _In_ crude_gfx_texture_handle                            viewport_texture_handle,
  _In_ int32                                               viewport_ratio_index
);

CRUDE_API void
crude_gui_viewport_deinitialize
(
  _In_ crude_gui_viewport                                 *viewport
);

CRUDE_API void
crude_gui_viewport_update
(
  _In_ crude_gui_viewport                                 *viewport
);

CRUDE_API void
crude_gui_viewport_queue_draw
(
  _In_ crude_gui_viewport                                 *viewport,
  _In_ crude_ecs                                          *world,
  _In_ crude_entity                                        selected_node,
  _In_ crude_entity                                        camera_node
);