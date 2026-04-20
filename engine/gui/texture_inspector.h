#pragma once

#if CRUDE_DEVELOP

#include <engine/graphics/render_graph.h>

typedef struct crude_editor crude_editor;

typedef struct crude_gui_texture_inspector
{
  crude_gfx_device                                        *gpu;
  crude_gfx_render_graph_builder                          *render_graph_builder;
  crude_gfx_texture_handle                                 texture_handle;
} crude_gui_texture_inspector;

CRUDE_API void
crude_gui_texture_inspector_initialize
(
  _In_ crude_gui_texture_inspector                        *inspector,
  _In_ crude_gfx_render_graph_builder                     *render_graph_builder
);

CRUDE_API void
crude_gui_texture_inspector_deinitialize
(
  _In_ crude_gui_texture_inspector                        *inspector
);

CRUDE_API void
crude_gui_texture_inspector_update
(
  _In_ crude_gui_texture_inspector                        *inspector
);

CRUDE_API void
crude_gui_texture_inspector_queue_draw
(
  _In_ crude_gui_texture_inspector                        *inspector
);

#endif /* CRUDE_DEVELOP */