#pragma once

#include <engine/core/alias.h>
#include <engine/core/math.h>
#include <engine/graphics/imgui.h>

typedef enum crude_gui_hardcoded_icon_type
{
  CRUDE_GUI_HARDCODED_ICON_TYPE_FLOW,
  CRUDE_GUI_HARDCODED_ICON_TYPE_CIRCLE,
  CRUDE_GUI_HARDCODED_ICON_TYPE_SQUARE,
  CRUDE_GUI_HARDCODED_ICON_TYPE_GRID,
  CRUDE_GUI_HARDCODED_ICON_TYPE_ROUND_SQUARE,
  CRUDE_GUI_HARDCODED_ICON_TYPE_DIAMOND
} crude_gui_hardcoded_icon_type;

CRUDE_API void
crude_gui_queue_render_hardcoded_icon_im
(
  _In_ ImDrawList                                         *draw_list,
  _In_ ImVec2 const                                       *a,
  _In_ ImVec2 const                                       *b,
  _In_ crude_gui_hardcoded_icon_type                       type,
  _In_ bool                                                filled,
  _In_ ImU32                                               color,
  _In_ ImU32                                               inner_color
);

CRUDE_API void
crude_gui_queue_render_icon
(
  _In_ XMFLOAT2                                            size,
  _In_ crude_gui_hardcoded_icon_type                       type,
  _In_ bool                                                filled,
  _In_ XMFLOAT4                                            color,
  _In_ XMFLOAT4                                            inner_color
);