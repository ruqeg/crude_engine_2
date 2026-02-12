#pragma once

#include <engine/graphics/scene_renderer.h>
#include <engine/graphics/imgui.h>

typedef struct crude_gui_blueprint_node crude_gui_blueprint_node;

typedef void (*crude_gui_blueprint_create_node_callback)
(
  _In_ void                                               *ctx,
  _In_ crude_gui_blueprint_node                          **node
);

typedef struct crude_gui_blueprint_create_node_callback_container
{
  crude_gui_blueprint_create_node_callback                 fun;
  void                                                    *ctx;
} crude_gui_blueprint_create_node_callback_container;

typedef enum crude_gui_blueprint_pin_type
{
  CRUDE_GUI_BLUEPRINT_PIN_TYPE_FLOW,
  CRUDE_GUI_BLUEPRINT_PIN_TYPE_BOOL,
  CRUDE_GUI_BLUEPRINT_PIN_TYPE_INT,
  CRUDE_GUI_BLUEPRINT_PIN_TYPE_FLOAT,
  CRUDE_GUI_BLUEPRINT_PIN_TYPE_STRING,
  CRUDE_GUI_BLUEPRINT_PIN_TYPE_OBJECT,
  CRUDE_GUI_BLUEPRINT_PIN_TYPE_FUNCTION,
  CRUDE_GUI_BLUEPRINT_PIN_TYPE_DELEGATE,
} crude_gui_blueprint_pin_type;

typedef enum crude_gui_blueprint_pin_kind
{
  CRUDE_GUI_BLUEPRINT_PIN_KIND_OUTPUT,
  CRUDE_GUI_BLUEPRINT_PIN_KIND_INPUT
} crude_gui_blueprint_pin_kind;

typedef enum crude_gui_blueprint_node_type
{
  CRUDE_GUI_BLUEPRINT_NODE_TYPE_BLUEPRINT,
  CRUDE_GUI_BLUEPRINT_NODE_TYPE_SIMPLE,
  CRUDE_GUI_BLUEPRINT_NODE_TYPE_TREE,
  CRUDE_GUI_BLUEPRINT_NODE_TYPE_COMMENT,
  CRUDE_GUI_BLUEPRINT_NODE_TYPE_HOUDINI
} crude_gui_blueprint_node_type;

typedef struct crude_gui_blueprint_pin
{
  ax::NodeEditor::PinId                                    id;
  crude_gui_blueprint_node                                *node;
  char const                                              *name;
  crude_gui_blueprint_pin_type                             type;
  crude_gui_blueprint_pin_kind                             kind;
} crude_gui_blueprint_pin;

typedef struct crude_gui_blueprint_node
{
  ax::NodeEditor::NodeId                                   id;
  char const                                              *name;
  crude_gui_blueprint_pin                                 *inputs;
  crude_gui_blueprint_pin                                 *outputs;
  XMFLOAT4                                                 color;
  crude_gui_blueprint_node_type                            type;
  XMFLOAT2                                                 size;
  char                                                     state[ 512 ];
  char                                                     saved_state[ 512 ];
} crude_gui_blueprint_node;

typedef struct crude_gui_blueprint_link
{
  ax::NodeEditor::LinkId                                   id;
  ax::NodeEditor::PinId                                    start_pin_id;
  ax::NodeEditor::PinId                                    end_pin_id;
  XMFLOAT4                                                 color;
} crude_gui_blueprint_link;

typedef struct crude_gui_blueprint
{
  crude_gfx_device                                        *gpu;
  crude_heap_allocator                                    *allocator;
  crude_stack_allocator                                   *temporary_allocator;
  ax::NodeEditor::EditorContext                           *ax_context;
  int32                                                    next_id;
  int32                                                    pin_icon_size;
  crude_gui_blueprint_node                                *nodes;
  crude_gui_blueprint_link                                *links;
  crude_gfx_texture_handle                                 header_background_texture_handle;
  crude_gfx_texture_handle                                 save_icon_texture_handle;
  crude_gfx_texture_handle                                 restore_icon_texture_handle;
  float32                                                  touch_time;
  bool                                                     show_ordinals;
  crude_gui_blueprint_create_node_callback_container       create_node_callback_container;
  struct { uint64 key; float value; }                     *node_id_to_touch_time;
} crude_gui_blueprint;

CRUDE_API void
crude_gui_blueprint_link_initialize
(
  _In_ crude_gui_blueprint_link                           *link
);

CRUDE_API void
crude_gui_blueprint_link_deinitialize
(
  _In_ crude_gui_blueprint_link                           *link
);

CRUDE_API void
crude_gui_blueprint_node_initialize
(
  _In_ crude_gui_blueprint_node                           *node,
  _In_ crude_heap_allocator                               *allocator
);

CRUDE_API void
crude_gui_blueprint_node_deinitialize
(
  _In_ crude_gui_blueprint_node                           *node
);

CRUDE_API void
crude_gui_blueprint_initialize
(
  _In_ crude_gui_blueprint                                *blueprint,
  _In_ crude_gui_blueprint_create_node_callback_container  create_node_callback_container,
  _In_ crude_gfx_device                                   *gpu,
  _In_ crude_heap_allocator                               *allocator,
  _In_ crude_stack_allocator                              *temporary_allocator,
  _In_ char const                                         *settings_absolute_filepath
);

CRUDE_API void
crude_gui_blueprint_deinitialize
(
  _In_ crude_gui_blueprint                                *blueprint
);

CRUDE_API void
crude_gui_blueprint_queue_draw
(
  _In_ crude_gui_blueprint                                *blueprint,
  _In_ char const                                         *title
);

// !TODO maybe i should move nodes to resource pool? 
CRUDE_API crude_gui_blueprint_node*
crude_gui_blueprint_create_node_unsafe_ptr
(
  _In_ crude_gui_blueprint                                *blueprint,
  _In_ char const                                         *name,
  _In_ XMFLOAT4                                            color
);

CRUDE_API void
crude_gui_blueprint_add_input_pin_to_node
(
  _In_ crude_gui_blueprint                                *blueprint,
  _In_ crude_gui_blueprint_node                           *node,
  _In_ char const                                         *name,
  _In_ crude_gui_blueprint_pin_type                        type
);

CRUDE_API void
crude_gui_blueprint_add_output_pin_to_node
(
  _In_ crude_gui_blueprint                                *blueprint,
  _In_ crude_gui_blueprint_node                           *node,
  _In_ char const                                         *name,
  _In_ crude_gui_blueprint_pin_type                        type
);

CRUDE_API void
crude_gui_blueprint_build_nodes
(
  _In_ crude_gui_blueprint                                *blueprint
);