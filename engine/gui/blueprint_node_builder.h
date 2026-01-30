/**
 * Original was written by Michal Cichon 
 * imgui-node-editor/examples/blueprints-example/blueprints-example.cpp
 */

#pragma once

#include <engine/core/alias.h>
#include <engine/graphics/gpu_resources.h>
#include <engine/graphics/imgui.h>

typedef struct crude_gfx_device crude_gfx_device;

typedef enum crude_gui_blueprint_node_builder_stage
{
  CRUDE_GUI_BLUEPRINT_NODE_BUILDER_STAGE_INVALID,
  CRUDE_GUI_BLUEPRINT_NODE_BUILDER_STAGE_BEGIN,
  CRUDE_GUI_BLUEPRINT_NODE_BUILDER_STAGE_HEADER,
  CRUDE_GUI_BLUEPRINT_NODE_BUILDER_STAGE_CONTENT,
  CRUDE_GUI_BLUEPRINT_NODE_BUILDER_STAGE_INPUT,
  CRUDE_GUI_BLUEPRINT_NODE_BUILDER_STAGE_OUTPUT,
  CRUDE_GUI_BLUEPRINT_NODE_BUILDER_STAGE_MIDDLE,
  CRUDE_GUI_BLUEPRINT_NODE_BUILDER_STAGE_END
} crude_gui_blueprint_node_builder_stage;

typedef struct crude_gui_blueprint_node_builder
{
  crude_gfx_device                                        *gpu;
  crude_gfx_texture_handle                                 header_texture_handle;
  ax::NodeEditor::NodeId                                   current_node_id;
  crude_gui_blueprint_node_builder_stage                   current_stage;
  ImU32                                                    header_color;
  ImVec2                                                   node_min;
  ImVec2                                                   node_max;
  ImVec2                                                   header_min;
  ImVec2                                                   header_max;
  ImVec2                                                   content_min;
  ImVec2                                                   content_max;
  bool                                                     has_header;
} crude_gui_blueprint_node_builder;

CRUDE_API crude_gui_blueprint_node_builder
crude_gui_blueprint_node_builder_empty
(
  _In_ crude_gfx_device                                   *gpu
);

CRUDE_API void
crude_gui_blueprint_node_builder_begin
(
  _In_ crude_gui_blueprint_node_builder                   *builder,
  _In_ ax::NodeEditor::NodeId                              id
);

CRUDE_API void
crude_gui_blueprint_node_builder_end
(
  _In_ crude_gui_blueprint_node_builder                   *builder
);

CRUDE_API void
crude_gui_blueprint_node_builder_header
(
  _In_ crude_gui_blueprint_node_builder                   *builder,
  _In_ XMFLOAT4                                           color
);

CRUDE_API void
crude_gui_blueprint_node_builder_end_header
(
  _In_ crude_gui_blueprint_node_builder                   *builder
);

CRUDE_API void
crude_gui_blueprint_node_builder_input
(
  _In_ crude_gui_blueprint_node_builder                   *builder,
  _In_ ax::NodeEditor::PinId                               id
);

CRUDE_API void
crude_gui_blueprint_node_builder_end_input
(
  _In_ crude_gui_blueprint_node_builder                   *builder
);

CRUDE_API void
crude_gui_blueprint_node_builder_middle
(
  _In_ crude_gui_blueprint_node_builder                   *builder
);

CRUDE_API void
crude_gui_blueprint_node_builder_output
(
  _In_ crude_gui_blueprint_node_builder                   *builder,
  _In_ ax::NodeEditor::PinId                               id
);

CRUDE_API void
crude_gui_blueprint_node_builder_end_output
(
  _In_ crude_gui_blueprint_node_builder                   *builder
);