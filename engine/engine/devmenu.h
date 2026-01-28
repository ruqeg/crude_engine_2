#pragma once

#if CRUDE_DEVELOP

#include <engine/graphics/scene_renderer.h>
#include <engine/graphics/gpu_profiler.h>
#include <engine/graphics/imgui.h>

typedef struct crude_engine crude_engine;
typedef struct crude_devmenu crude_devmenu;

typedef struct crude_devmenu_texture_inspector
{
  crude_devmenu                                           *devmenu;
  bool                                                     enabled;
  crude_gfx_texture_handle                                 texture_handle;
} crude_devmenu_texture_inspector;

typedef struct crude_devmenu_memory_visual_profiler
{
  crude_devmenu                                           *devmenu;
  bool                                                     enabled;
  crude_allocator_container                               *allocators_containers;
} crude_devmenu_memory_visual_profiler;

typedef struct crude_devmenu_gpu_visual_profiler
{
  crude_devmenu                                           *devmenu;
  crude_gfx_device                                        *gpu;
  float32                                                  max_duration;
  uint32                                                   max_frames;
  uint32                                                   max_queries_per_frame;
  uint32                                                   current_frame;
  int32                                                    max_visible_depth;
  float32                                                  average_time;
  float32                                                  max_time;
  float32                                                  min_time;
  float32                                                  new_average;
  uint32                                                   framebuffer_pixel_count;
  uint16                                                  *per_frame_active;
  crude_gfx_gpu_time_query                                *timestamps;
  crude_gfx_gpu_pipeline_statistics                       *pipeline_statistics;
  crude_heap_allocator                                    *allocator;
  struct { uint64 key; uint32 value; }                    *name_hashed_to_color_index;
  uint32                                                   initial_frames_paused;
  bool                                                     paused;
  bool                                                     enabled;
} crude_devmenu_gpu_visual_profiler;

typedef struct crude_devmenu_render_graph
{
  crude_devmenu                                           *devmenu;
  bool                                                     enabled;
} crude_devmenu_render_graph;

typedef struct crude_devmenu_gpu_pool
{
  crude_devmenu                                           *devmenu;
  bool                                                     enabled;
} crude_devmenu_gpu_pool;

typedef struct crude_devmenu_scene_renderer
{
  crude_devmenu                                           *devmenu;
  bool                                                     debug_probes_statuses;
  bool                                                     debug_probes_radiance;
  bool                                                     enabled;
} crude_devmenu_scene_renderer;

typedef struct crude_devmenu_nodes_tree
{
  crude_devmenu                                           *devmenu;
  bool                                                     enabled;
  
  crude_entity                                             selected_node;

  char                                                     dublicate_node_name[ 512 ];
  crude_entity                                             dublicate_node_reference;
} crude_devmenu_nodes_tree;

typedef struct crude_devmenu_node_inspector
{
  crude_devmenu                                           *devmenu;
  bool                                                     enabled;
} crude_devmenu_node_inspector;

typedef struct crude_devmenu_viewport
{
  crude_devmenu                                           *devmenu;
  bool                                                     enabled;
} crude_devmenu_viewport;


typedef enum crude_technique_editor_pin_type
{
  CRUDE_TECHNIQUE_EDITOR_PIN_TYPE_FLOW,
  CRUDE_TECHNIQUE_EDITOR_PIN_TYPE_BOOL,
  CRUDE_TECHNIQUE_EDITOR_PIN_TYPE_INT,
  CRUDE_TECHNIQUE_EDITOR_PIN_TYPE_FLOAT,
  CRUDE_TECHNIQUE_EDITOR_PIN_TYPE_STRING,
  CRUDE_TECHNIQUE_EDITOR_PIN_TYPE_OBJECT,
  CRUDE_TECHNIQUE_EDITOR_PIN_TYPE_FUNCTION,
  CRUDE_TECHNIQUE_EDITOR_PIN_TYPE_DELEGATE,
} crude_technique_editor_pin_type;

typedef enum crude_technique_editor_pin_kind
{
  CRUDE_TECHNIQUE_EDITOR_PIN_KIND_OUTPUT,
  CRUDE_TECHNIQUE_EDITOR_PIN_KIND_INPUT
} crude_technique_editor_pin_kind;

typedef enum crude_technique_editor_node_type
{
  CRUDE_TECHNIQUE_EDITOR_NODE_TYPE_BLUEPRINT,
  CRUDE_TECHNIQUE_EDITOR_NODE_TYPE_SIMPLE,
  CRUDE_TECHNIQUE_EDITOR_NODE_TYPE_TREE,
  CRUDE_TECHNIQUE_EDITOR_NODE_TYPE_COMMENT,
  CRUDE_TECHNIQUE_EDITOR_NODE_TYPE_HOUDINI
} crude_technique_editor_node_type;

typedef struct crude_technique_editor_node crude_technique_editor_node;

typedef struct crude_technique_editor_pin
{
  ax::NodeEditor::PinId                                    id;
  crude_technique_editor_node                             *node;
  char                                                     name[ 512 ];
  crude_technique_editor_pin_type                          type;
  crude_technique_editor_pin_kind                          kind;
} crude_technique_editor_pin;

typedef struct crude_technique_editor_node
{
  ax::NodeEditor::NodeId                                   id;
  char                                                     name[ 512 ];
  crude_technique_editor_pin                              *inputs;
  crude_technique_editor_pin                              *outputs;
  ImColor                                                  color;
  crude_technique_editor_node_type                         type;
  ImVec2                                                   size;
  char                                                     state[ 512 ];
  char                                                     saved_state[ 512 ];
} crude_technique_editor_node;

typedef struct crude_technique_editor_link
{
  ax::NodeEditor::LinkId                                   id;
  ax::NodeEditor::PinId                                    start_pin_id;
  ax::NodeEditor::PinId                                    end_pin_id;
  ImColor                                                  color;
} crude_technique_editor_link;

typedef struct crude_devmenu_technique_editor
{
  ax::NodeEditor::EditorContext                           *ax_context;
  int32                                                    next_id;
  int32                                                    pin_icon_size;
  crude_technique_editor_node                             *nodes;
  crude_technique_editor_link                             *links;
  crude_gfx_texture_handle                                 header_background_texture_handle;
  crude_gfx_texture_handle                                 save_icon_texture_handle;
  crude_gfx_texture_handle                                 restore_icon_texture_handle;
  float32                                                  touch_time;
  struct { uint64 key; float value; }                     *node_id_to_touch_time;
  bool                                                     show_ordinals;
  crude_devmenu                                           *devmenu;
  bool                                                     enabled;
} crude_devmenu_technique_editor;

typedef struct crude_devmenu
{
  crude_engine                                            *engine;
  bool                                                     enabled;
  crude_devmenu_gpu_visual_profiler                        gpu_visual_profiler;
  crude_devmenu_memory_visual_profiler                     memory_visual_profiler;
  crude_devmenu_texture_inspector                          texture_inspector;
  crude_devmenu_render_graph                               render_graph;
  crude_devmenu_gpu_pool                                   gpu_pool;
  crude_devmenu_scene_renderer                             scene_renderer;
  crude_devmenu_nodes_tree                                 nodes_tree;
  crude_devmenu_node_inspector                             node_inspector;
  crude_devmenu_viewport                                   viewport;
  crude_devmenu_technique_editor                           technique_editor;
  uint32                                                   selected_option;
  float32                                                  last_framerate_update_time;
  uint32                                                   previous_framerate;
  uint32                                                   current_framerate;
} crude_devmenu;

/***********************
 * 
 * Develop Menu
 * 
 ***********************/
CRUDE_API void
crude_devmenu_initialize
(
  _In_ crude_devmenu                                      *devmenu,
  _In_ crude_engine                                       *engine
);

CRUDE_API void
crude_devmenu_deinitialize
(
  _In_ crude_devmenu                                      *devmenu
);

CRUDE_API void
crude_devmenu_draw
(
  _In_ crude_devmenu                                      *devmenu
);

CRUDE_API void
crude_devmenu_update
(
  _In_ crude_devmenu                                      *devmenu
);

CRUDE_API void
crude_devmenu_handle_input
(
  _In_ crude_devmenu                                      *devmenu
);

/***********************
 * 
 * Common Commmads
 * 
 ***********************/
CRUDE_API void
crude_devmenu_debug_gltf_view_callback
(
  _In_ crude_devmenu                                      *devmenu
);

CRUDE_API bool
crude_devmenu_debug_gltf_view_callback_hotkey_pressed_callback
(
  _In_ crude_input                                        *input
);

CRUDE_API void
crude_devmenu_collisions_view_callback
(
  _In_ crude_devmenu                                      *devmenu
);

CRUDE_API bool
crude_devmenu_collisions_view_callback_hotkey_pressed_callback
(
  _In_ crude_input                                        *input
);

CRUDE_API void
crude_devmenu_free_camera_callback
(
  _In_ crude_devmenu                                      *devmenu
);

CRUDE_API bool
crude_devmenu_free_camera_callback_hotkey_pressed_callback
(
  _In_ crude_input                                        *input
);

CRUDE_API void
crude_devmenu_reload_techniques_callback
(
  _In_ crude_devmenu                                      *devmenu
);

CRUDE_API bool
crude_devmenu_reload_techniques_hotkey_pressed_callback
(
  _In_ crude_input                                        *input
);

/***********************
 * 
 * Develop GPU Visual Profiler
 * 
 ***********************/
CRUDE_API void
crude_devmenu_gpu_visual_profiler_initialize
(
  _In_ crude_devmenu_gpu_visual_profiler                  *dev_gpu_profiler,
  _In_ crude_devmenu                                      *devmenu
);

CRUDE_API void
crude_devmenu_gpu_visual_profiler_deinitialize
(
  _In_ crude_devmenu_gpu_visual_profiler                  *dev_gpu_profiler
);

CRUDE_API void
crude_devmenu_gpu_visual_profiler_update
(
  _In_ crude_devmenu_gpu_visual_profiler                  *dev_gpu_profiler
);

CRUDE_API void
crude_devmenu_gpu_visual_profiler_draw
(
  _In_ crude_devmenu_gpu_visual_profiler                  *dev_gpu_profiler
);

CRUDE_API void
crude_devmenu_gpu_visual_profiler_callback
(
  _In_ crude_devmenu                                      *devmenu
);

/***********************
 * 
 * Develop Memory Visual Profiler
 * 
 ***********************/
CRUDE_API void
crude_devmenu_memory_visual_profiler_initialize
(
  _In_ crude_devmenu_memory_visual_profiler               *dev_mem_profiler,
  _In_ crude_devmenu                                      *devmenu
);

CRUDE_API void
crude_devmenu_memory_visual_profiler_deinitialize
(
  _In_ crude_devmenu_memory_visual_profiler               *dev_mem_profiler
);

CRUDE_API void
crude_devmenu_memory_visual_profiler_update
(
  _In_ crude_devmenu_memory_visual_profiler               *dev_mem_profiler
);

CRUDE_API void
crude_devmenu_memory_visual_profiler_draw
(
  _In_ crude_devmenu_memory_visual_profiler               *dev_mem_profiler
);

CRUDE_API void
crude_devmenu_memory_visual_profiler_callback
(
  _In_ crude_devmenu                                      *devmenu
);

/***********************
 * 
 * Develop Texture Inspector
 * 
 ***********************/
CRUDE_API void
crude_devmenu_texture_inspector_initialize
(
  _In_ crude_devmenu_texture_inspector                    *dev_texture_inspector,
  _In_ crude_devmenu                                      *devmenu
);

CRUDE_API void
crude_devmenu_texture_inspector_deinitialize
(
  _In_ crude_devmenu_texture_inspector                    *dev_texture_inspector
);

CRUDE_API void
crude_devmenu_texture_inspector_update
(
  _In_ crude_devmenu_texture_inspector                    *dev_texture_inspector
);

CRUDE_API void
crude_devmenu_texture_inspector_draw
(
  _In_ crude_devmenu_texture_inspector                    *dev_texture_inspector
);

CRUDE_API void
crude_devmenu_texture_inspector_callback
(
  _In_ crude_devmenu                                      *devmenu
);

/***********************
 * 
 * Develop Render Graph
 * 
 ***********************/
CRUDE_API void
crude_devmenu_render_graph_initialize
(
  _In_ crude_devmenu_render_graph                         *dev_render_graph,
  _In_ crude_devmenu                                      *devmenu
);

CRUDE_API void
crude_devmenu_render_graph_deinitialize
(
  _In_ crude_devmenu_render_graph                         *dev_render_graph
);

CRUDE_API void
crude_devmenu_render_graph_update
(
  _In_ crude_devmenu_render_graph                         *dev_render_graph
);

CRUDE_API void
crude_devmenu_render_graph_draw
(
  _In_ crude_devmenu_render_graph                         *dev_render_graph
);

CRUDE_API void
crude_devmenu_render_graph_callback
(
  _In_ crude_devmenu                                      *devmenu
);

/***********************
 * 
 * Develop GPU Pools
 * 
 ***********************/
CRUDE_API void
crude_devmenu_gpu_pool_initialize
(
  _In_ crude_devmenu_gpu_pool                             *dev_gpu_pool,
  _In_ crude_devmenu                                      *devmenu
);

CRUDE_API void
crude_devmenu_gpu_pool_deinitialize
(
  _In_ crude_devmenu_gpu_pool                             *dev_gpu_pool
);

CRUDE_API void
crude_devmenu_gpu_pool_update
(
  _In_ crude_devmenu_gpu_pool                             *dev_gpu_pool
);

CRUDE_API void
crude_devmenu_gpu_pool_draw
(
  _In_ crude_devmenu_gpu_pool                             *dev_gpu_pool
);

CRUDE_API void
crude_devmenu_gpu_pool_callback
(
  _In_ crude_devmenu                                      *devmenu
);

/***********************
 * 
 * Develop Scene Renderer
 * 
 ***********************/
CRUDE_API void
crude_devmenu_scene_renderer_initialize
(
  _In_ crude_devmenu_scene_renderer                       *dev_scene_rendere,
  _In_ crude_devmenu                                      *devmenu
);

CRUDE_API void
crude_devmenu_scene_renderer_deinitialize
(
  _In_ crude_devmenu_scene_renderer                       *dev_scene_rendere
);

CRUDE_API void
crude_devmenu_scene_renderer_update
(
  _In_ crude_devmenu_scene_renderer                       *dev_scene_rendere
);

CRUDE_API void
crude_devmenu_scene_renderer_draw
(
  _In_ crude_devmenu_scene_renderer                       *dev_scene_rendere
);

CRUDE_API void
crude_devmenu_scene_renderer_callback
(
  _In_ crude_devmenu                                      *devmenu
);

/***********************
 * 
 * Develop Nodes Tree
 * 
 ***********************/
CRUDE_API void
crude_devmenu_nodes_tree_initialize
(
  _In_ crude_devmenu_nodes_tree                           *dev_nodes_tree,
  _In_ crude_devmenu                                      *devmenu
);

CRUDE_API void
crude_devmenu_nodes_tree_deinitialize
(
  _In_ crude_devmenu_nodes_tree                           *dev_nodes_tree
);

CRUDE_API void
crude_devmenu_nodes_tree_update
(
  _In_ crude_devmenu_nodes_tree                           *dev_nodes_tree
);

CRUDE_API void
crude_devmenu_nodes_tree_draw
(
  _In_ crude_devmenu_nodes_tree                           *dev_nodes_tree
);

CRUDE_API void
crude_devmenu_nodes_tree_callback
(
  _In_ crude_devmenu                                      *devmenu
);

/***********************
 * 
 * Develop Node Inspector
 * 
 ***********************/
CRUDE_API void
crude_devmenu_node_inspector_initialize
(
  _In_ crude_devmenu_node_inspector                       *dev_node_inspector,
  _In_ crude_devmenu                                      *devmenu
);

CRUDE_API void
crude_devmenu_node_inspector_deinitialize
(
  _In_ crude_devmenu_node_inspector                       *dev_node_inspector
);

CRUDE_API void
crude_devmenu_node_inspector_update
(
  _In_ crude_devmenu_node_inspector                       *dev_node_inspector
);

CRUDE_API void
crude_devmenu_node_inspector_draw
(
  _In_ crude_devmenu_node_inspector                       *dev_node_inspector
);

CRUDE_API void
crude_devmenu_node_inspector_callback
(
  _In_ crude_devmenu                                      *devmenu
);

/***********************
 * 
 * Develop Viewport
 * 
 ***********************/
CRUDE_API void
crude_devmenu_viewport_initialize
(
  _In_ crude_devmenu_viewport                             *dev_viewport,
  _In_ crude_devmenu                                      *devmenu
);

CRUDE_API void
crude_devmenu_viewport_deinitialize
(
  _In_ crude_devmenu_viewport                             *dev_viewport
);

CRUDE_API void
crude_devmenu_viewport_update
(
  _In_ crude_devmenu_viewport                             *dev_viewport
);

CRUDE_API void
crude_devmenu_viewport_draw
(
  _In_ crude_devmenu_viewport                             *dev_viewport
);

CRUDE_API void
crude_devmenu_viewport_callback
(
  _In_ crude_devmenu                                      *devmenu
);

/***********************
 * 
 * Develop Technique Editor
 * 
 ***********************/
CRUDE_API void
crude_devmenu_technique_editor_initialize
(
  _In_ crude_devmenu_technique_editor                     *devmenu_technique_editor,
  _In_ crude_devmenu                                      *devmenu
);

CRUDE_API void
crude_devmenu_technique_editor_deinitialize
(
  _In_ crude_devmenu_technique_editor                     *devmenu_technique_editor
);

CRUDE_API void
crude_devmenu_technique_editor_update
(
  _In_ crude_devmenu_technique_editor                     *devmenu_technique_editor
);

CRUDE_API void
crude_devmenu_technique_editor_draw
(
  _In_ crude_devmenu_technique_editor                     *devmenu_technique_editor
);

CRUDE_API void
crude_devmenu_technique_editor_callback
(
  _In_ crude_devmenu                                      *devmenu
);

#endif