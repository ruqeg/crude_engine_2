#pragma once

#include <nfd.h>

#include <engine.h>
#include <scene/scene.h>
#include <graphics/scene_renderer.h>
#include <core/ecs.h>
#include <platform/platform_components.h>
#include <scene/collisions_resources_manager.h>
#include <devgui.h>

typedef enum crude_editor_queue_command_type
{
  CRUDE_EDITOR_QUEUE_COMMAND_TYPE_RELOAD_SCENE,
  CRUDE_EDITOR_QUEUE_COMMAND_TYPE_RELOAD_TECHNIQUES,
  CRUDE_EDITOR_QUEUE_COMMAND_TYPE_COUNT,
} crude_editor_queue_command_type;

typedef struct crude_editor_queue_command
{
  crude_editor_queue_command_type                          type;
  union
  {
    struct 
    {
      nfdu8char_t                                         *filepath;
    } reload_scene;
    struct 
    {
    } reload_techniques;
  };
} crude_game_queue_command;

typedef struct crude_editor
{
  /* Context */
  crude_engine                                            *engine;
  /* Common */
  crude_heap_allocator                                     allocator;
  crude_heap_allocator                                     resources_allocator;
  crude_heap_allocator                                     cgltf_temporary_allocator;
  crude_stack_allocator                                    temporary_allocator;
  crude_stack_allocator                                    model_renderer_resources_manager_temporary_allocator;
  /* Graphics */
  crude_gfx_device                                         gpu;
  crude_gfx_render_graph                                   render_graph;
  crude_gfx_render_graph_builder                           render_graph_builder;
  crude_gfx_asynchronous_loader                            async_loader;
  crude_gfx_scene_renderer                                 scene_renderer;
  crude_gfx_model_renderer_resources_manager               model_renderer_resources_manager;
  void                                                    *imgui_context;
  /* Dev */
  crude_devgui                                             devgui;
  /* Scene */
  crude_scene                                              scene;
  /* Window & Input */
  crude_entity                                             platform_node;
  XMFLOAT2                                                 last_unrelative_mouse_position;
  /* Game */
  crude_entity                                             focused_camera_node;
  crude_entity                                             editor_camera_node;

  /* Other */
  uint32                                                   framerate;
  float32                                                  last_graphics_update_time;

  crude_game_queue_command                                *commands_queue;
} crude_editor;

CRUDE_API void
crude_editor_initialize
(
  _In_ crude_editor                                       *editor,
  _In_ crude_engine                                       *engine
);

CRUDE_API void
crude_editor_postupdate
(
  _In_ crude_editor                                       *editor
);

CRUDE_API void
crude_editor_deinitialize
(
  _In_ crude_editor                                       *editor
);

CRUDE_API void
crude_editor_push_reload_scene_command
(
  _In_ crude_editor                                       *editor,
  _In_ nfdu8char_t                                        *filepath
);

CRUDE_API void
crude_editor_push_reload_techniques_command
(
  _In_ crude_editor                                       *editor
);

CRUDE_API void
crude_editor_instance_intialize
(
);

CRUDE_API void
crude_editor_instance_deintialize
(
);

CRUDE_API crude_editor*
crude_editor_instance
(
);